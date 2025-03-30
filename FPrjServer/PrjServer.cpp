#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include <commctrl.h>

#define SERVERPORT 9000
#define BUFSIZE    512
#define WM_ADDSOCKET (WM_USER+1)
#define WM_RMSOCKET  (WM_USER+2)
#define SERVER_MESSAGE 1111

// 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
	SOCKET sock;
	bool   isIPv6;
	char   buf[BUFSIZE];
	char   userID[20];
	int sendbytes;
	int recvbytes;
	int    state = 0;
	SOCKETINFO* next;
};

struct COMM_MSG {
	int type;
	int size;
};

int nTotalSockets = 0;
SOCKETINFO *SocketInfoArray[FD_SETSIZE];
static SOCKET			 g_sockv4;
static SOCKET			 g_sockv6;

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

// 편집 컨트롤 출력 함수
void DisplayText(char* fmt, ...);

void sendToAllSocket(COMM_MSG* comm_msg, char* buf);

// 소켓 관리 함수
BOOL AddSocketInfo(SOCKET sock, bool isIPv6,  char *userID);
void RemoveSocketInfo(int nIndex);

// 오류 출력 함수
void err_quit(char *msg);
void err_display(char *msg);

//사용자 정의 함수
int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

static HINSTANCE     g_hInst; // 응용 프로그램 인스턴스 핸들
static HWND			 g_hDlg;
static HANDLE		 g_hServerThread;
static HWND			 g_hEditMsg;

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI ServerMain(LPVOID arg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	// 대화상자 생성
	g_hInst = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hUserList;
	static HWND hEditMsg;
	g_hDlg = hDlg;
	LVITEM user;
	memset(&user, 0, sizeof(user));
	user.mask = LVCF_TEXT;
	user.cchTextMax = 200;


	switch (uMsg) {
	case WM_INITDIALOG:
		//컨트롤 핸들 얻기
		hEditMsg = GetDlgItem(hDlg, IDC_EDIT1);
		hUserList = GetDlgItem(hDlg, IDC_LIST1);

		// 컨트롤 초기화
		g_hServerThread = CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);
		g_hEditMsg = hEditMsg;

		return TRUE;


	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDC_BUTTON1: {
				char buf[BUFSIZ];
				int i = SendMessage(hUserList, LB_GETCURSEL, 0, 0);
				SOCKETINFO* ptr = SocketInfoArray[i];
				SendMessage(hUserList, LB_DELETESTRING, (WPARAM)i, 0);	
				ptr->state = 1;
				RemoveSocketInfo(i);
				return TRUE;
			}
		}
		return DefWindowProc(hDlg, uMsg, wParam, lParam);

	case WM_ADDSOCKET: {
		SendMessage(hUserList, LB_ADDSTRING, 0, (LPARAM)lParam);
		return TRUE;
	}

	case WM_RMSOCKET: {
		int index = SendMessage(hUserList, LB_FINDSTRINGEXACT, -1, (LPARAM)wParam);
		SendMessage(hUserList, LB_DELETESTRING, index, 0);
		return TRUE;
	}
	default:
		return DefWindowProc(hDlg, uMsg, wParam, lParam);
	}
	return FALSE;
}

DWORD WINAPI ServerMain(LPVOID arg)
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;

	/*-----TCP/IPv4 소켓 초기화 시작 -----*/
	// socket()
	SOCKET listen_sockv4 = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sockv4 == INVALID_SOCKET) err_quit("socket()");

	u_long on = 1;
	retval = ioctlsocket(listen_sockv4, FIONBIO, &on);
	if (retval == SOCKET_ERROR) err_display("ioctlsocket()");

	// bind()
	SOCKADDR_IN serveraddrv4;
	ZeroMemory(&serveraddrv4, sizeof(serveraddrv4));
	serveraddrv4.sin_family = AF_INET;
	serveraddrv4.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddrv4.sin_port = htons(SERVERPORT);
	retval = bind(listen_sockv4, (SOCKADDR *)&serveraddrv4, sizeof(serveraddrv4));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sockv4, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");
	/*-----TCP/IPv4 소켓 초기화 끝 -----*/


	/*----- TCP/IPv6 소켓 초기화 시작 -----*/
	// socket()
	SOCKET listen_sockv6 = socket(AF_INET6, SOCK_STREAM, 0);
	if(listen_sockv6 == INVALID_SOCKET) err_quit("socket()");

	u_long on6 = 1;
	retval = ioctlsocket(listen_sockv4, FIONBIO, &on6);
	if (retval == SOCKET_ERROR) err_display("ioctlsocket()");

	// bind()
	SOCKADDR_IN6 serveraddrv6;
	ZeroMemory(&serveraddrv6, sizeof(serveraddrv6));
	serveraddrv6.sin6_family = AF_INET6;
	serveraddrv6.sin6_addr = in6addr_any;
	serveraddrv6.sin6_port = htons(SERVERPORT);
	retval = bind(listen_sockv6, (SOCKADDR *)&serveraddrv6, sizeof(serveraddrv6));
	if(retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sockv6, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");
	/*----- TCP/IPv6 소켓 초기화 끝 -----*/

	// 데이터 통신에 사용할 변수(공통)
	FD_SET rset;
	SOCKET client_sock;
	int addrlen, i, j;
	// 데이터 통신에 사용할 변수(IPv4)
	SOCKADDR_IN clientaddrv4;
	// 데이터 통신에 사용할 변수(IPv6)
	SOCKADDR_IN6 clientaddrv6;

	while (1) {
		// 소켓 셋 초기화
		FD_ZERO(&rset);
		FD_SET(listen_sockv4, &rset);
		FD_SET(listen_sockv6, &rset);
		for (i = 0; i < nTotalSockets; i++) {
			FD_SET(SocketInfoArray[i]->sock, &rset);
		}

		// select()
		retval = select(0, &rset, NULL, NULL, NULL);
		if (retval == SOCKET_ERROR) {
			err_display("select()");
			break;
		}

		// 소켓 셋 검사(1): 클라이언트 접속 수용
		//TCP IPv4 소켓
		if (FD_ISSET(listen_sockv4, &rset)) {
			addrlen = sizeof(clientaddrv4);
			client_sock = accept(listen_sockv4, (SOCKADDR*)&clientaddrv4, &addrlen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
			else {
				DisplayText("[TCPv4 서버] 클라이언트 접속: IP = %s, PORT = %d\r\n",
					inet_ntoa(clientaddrv4.sin_addr), ntohs(clientaddrv4.sin_port));
				// 소켓 정보 추가
				char userID[20];
				retval = recv(client_sock, userID, 20, 0);
				AddSocketInfo(client_sock, false, userID);

				SendMessage(g_hDlg, WM_ADDSOCKET, (WPARAM)client_sock, (LPARAM)userID);
			}
		}

		//TCP IPv6 소켓
		if (FD_ISSET(listen_sockv6, &rset)) {
			addrlen = sizeof(clientaddrv6);
			client_sock = accept(listen_sockv6, (SOCKADDR*)&clientaddrv6, &addrlen);
			if (client_sock == INVALID_SOCKET) {
				err_display("accept()");
				break;
			}
			else {
				// 접속한 클라이언트 정보 출력
				char ipaddr[50];
				DWORD ipaddrlen = sizeof(ipaddr);
				WSAAddressToString((SOCKADDR*)&clientaddrv6, sizeof(clientaddrv6),
					NULL, ipaddr, &ipaddrlen);
				DisplayText("[TCPv6 서버] 클라이언트 접속: %s \r\n", ipaddr);

				// 소켓 정보 추가
				char userID[20];
				retval = recv(client_sock, userID, 20, 0);
				AddSocketInfo(client_sock, true, userID);

				SendMessage(g_hDlg, WM_ADDSOCKET, (WPARAM)client_sock, (LPARAM)userID);
			}
		}

		// 소켓 셋 검사(2): 데이터 통신
		for (i = 0; i < nTotalSockets; i++) {
			COMM_MSG comm_msg;
			SOCKETINFO* ptr = SocketInfoArray[i];

			//TCP 소켓 데이터 통신
			if (FD_ISSET(ptr->sock, &rset)) {
				// 데이터 받기
				retval = recv(ptr->sock, (char*)&comm_msg, sizeof(COMM_MSG), 0);
				if (retval == 0 || retval == SOCKET_ERROR) {
					RemoveSocketInfo(i);
					continue;
				}
				Sleep(1);
				retval = recv(ptr->sock, ptr->buf, comm_msg.size, 0);
				if (retval == 0 || retval == SOCKET_ERROR) {
					RemoveSocketInfo(i);
					continue;
				}
				ptr->recvbytes += retval;
				if (ptr->recvbytes == BUFSIZE) {
					// 받은 바이트 수 리셋
					ptr->recvbytes = 0;
				}
				sendToAllSocket(&comm_msg, ptr->buf);
			}
		}
	}
	return 0;
}

void sendToAllSocket(COMM_MSG* comm_msg, char* buf) { //TCP 전용
	// 현재 접속한 모든 클라이언트에게 데이터를 보냄!
	int retval, j;
	for (j = 0; j < nTotalSockets; j++) {
		SOCKETINFO* ptr2 = SocketInfoArray[j];
		retval = send(ptr2->sock, (char*)comm_msg, sizeof(COMM_MSG), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");

			RemoveSocketInfo(j);
			--j; // 루프 인덱스 보정
			continue;
		}
		retval = send(ptr2->sock, buf, comm_msg->size, 0);
		
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			RemoveSocketInfo(j);
			--j; // 루프 인덱스 보정
			continue;
		}
	}
}
// 소켓 정보 추가
BOOL AddSocketInfo(SOCKET sock, bool isIPv6,  char * userID)
{
	if(nTotalSockets >= FD_SETSIZE){
		DisplayText("[오류] 소켓 정보를 추가할 수 없습니다! \r\n");
		return FALSE;
	}

	SOCKETINFO *ptr = new SOCKETINFO;
	if(ptr == NULL){
		DisplayText("[오류] 메모리가 부족합니다! \r\n");
		return FALSE;
	}

	ptr->sock = sock;
	ptr->isIPv6 = isIPv6;
	strcpy(ptr->userID, userID);
	ptr->recvbytes = 0;
	SocketInfoArray[nTotalSockets++] = ptr;

	return TRUE;
}

// 소켓 정보 삭제
void RemoveSocketInfo(int nIndex)
{
	int retval;
	SOCKETINFO* ptr = SocketInfoArray[nIndex];
	if (ptr->state != 1) {
		SendMessage(g_hDlg, WM_RMSOCKET, (WPARAM)ptr->userID, 0);
	}
	DisplayText("");
	if (ptr->isIPv6 == false) { //TCP IPv4
		SOCKADDR_IN clientaddrv4;
		int addrlen = sizeof(clientaddrv4);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddrv4, &addrlen);
		
		DisplayText("[TCPv4 서버] 클라이언트 종료: IP = %s, PORT = %d \r\n",
			inet_ntoa(clientaddrv4.sin_addr), ntohs(clientaddrv4.sin_port));
		closesocket(ptr->sock);
		delete ptr;
		if (nIndex != (nTotalSockets - 1))
			SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];
		--nTotalSockets;
	}
	else if (ptr->isIPv6 == true) { //TCP IPv6
		SOCKADDR_IN6 clientaddrv6;
		int addrlen = sizeof(clientaddrv6);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddrv6, &addrlen);
		char ipaddr[50];
		DWORD ipaddrlen = sizeof(ipaddr);
		WSAAddressToString((sockaddr*)&clientaddrv6, sizeof(clientaddrv6), NULL, ipaddr, &ipaddrlen);
		DisplayText("[TCPv6 서버] 클라이언트 종료: %s \r\n", ipaddr);
		closesocket(ptr->sock);
		delete ptr;
		if (nIndex != (nTotalSockets - 1))
			SocketInfoArray[nIndex] = SocketInfoArray[nTotalSockets - 1];
		--nTotalSockets;
	}
}

// 에디트 컨트롤에 문자열 출력
void DisplayText(char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[1024];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(g_hEditMsg);
	SendMessage(g_hEditMsg, EM_SETSEL, nLength, nLength);
	SendMessage(g_hEditMsg, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}