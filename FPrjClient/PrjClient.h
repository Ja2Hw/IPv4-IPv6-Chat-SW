#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <Windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <Commctrl.h>
#include "resource.h"
#include <winresrc.h>
#include <math.h>
#include <cmath>
//#include "pch.h"
//#include "tipsware.h"

#define PI			3.141592653589793238
#define SERVERIPV4  "127.0.0.1"
#define SERVERIPV6  "::1"
#define DEFAULTID	"USER"
#define SERVERPORT  9000

#define BUFSIZE     512                    // 전송 메시지 전체 크기
#define IDSIZE		20						//사용자 ID 길이
#define MSGSIZE     (BUFSIZE-sizeof(int)-IDSIZE)  // 채팅 메시지 최대 길이

#define CHATTING    1000                   // 메시지 타입: 채팅
#define DRAWLINE    1001                   // 메시지 타입: 선 그리기
#define DRAWTRI		1002					// 메시지 타입: 삼각형 그리기
#define DRAWRECT	1003					// 메시지 타입: 사각형 그리기
#define DRAWELLIPSE	1004					// 메시지 타입: 타원 그리기
#define DRAWSTRICTLINE 1005					// 메시지 타입: 직선 그리기
#define DRAWCIRCLE  1006					// 메시지 타입: 원 그리기
#define SERVER_MESSAGE 1111
#define DRAWERASER 1007             // 메시지 타입: 그림 지우기
#define TYPE_ERASEPIC 1008				//메시지 타입: 그림 전체 지우기
#define NOTICE 1009                  // 메시지 타입: 공지 //예린


#define WM_DRAWIT (WM_USER+1)         // 사용자 정의 윈도우 메시지(1)
#define WM_ERASEPIC (WM_USER+2)         // 사용자 정의 윈도우 메시지(2)

#define SIZE_TOT 256                    // 전송 패킷(헤더 + 데이터) 전체 크기
#define SIZE_DAT (SIZE_TOT-sizeof(int)) // 헤더를 제외한 데이터 부분만의 크기

int a;
int b;
class RoundShape {
private:
	int rad;
	int x, y;

public:

};
// 공통 메시지 형식
// sizeof(COMM_MSG) == 8
struct COMM_MSG
{
	int  type;
	int  size;
};

// 채팅 메시지 형식
// sizeof(CHAT_MSG) == 256
struct CHAT_MSG
{
	int  type;
	char nickname[IDSIZE];
	char buf[MSGSIZE];
};

struct NOTICE_MSG
{
	int  type;
	char nickname[IDSIZE];
	char buf[MSGSIZE];
};


// 선 그리기 메시지 형식
// sizeof(DRAWLINE_MSG) == 256
struct DRAWLINE_MSG
{
	int  type;
	int  color;
	int  x0, y0;
	int  x1, y1;
	int lineWidth;
	BOOL isFill;
	//char dummy[BUFSIZE - 8 * sizeof(int)];
};

//그림 전체 지우기 메시지 형식
//sizeof(ERASERPIC_MSG)==256
typedef struct ERASEPIC_MSG
{
	int type;
	char dummy[SIZE_DAT];
} ;

//컬러 선택 
COLORREF Color = RGB(0, 0, 0);

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);
DWORD WINAPI ReadThread(LPVOID arg);
DWORD WINAPI WriteThread(LPVOID arg);
// 자식 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// 편집 컨트롤 출력 함수
void DisplayText(char* fmt, ...);
void DisplayNOTICE(char* fmt, ...);
// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags);
// 오류 출력 함수
void err_quit(char* msg);
void err_display(char* msg);
char* replaceAll(char* s, const char* olds, const char* news);