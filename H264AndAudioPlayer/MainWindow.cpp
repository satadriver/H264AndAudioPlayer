

#pragma once

#include<stdint.h>

#include <windows.h>
#include "H264AndAudioPlayer.h"
#include "H264Decode.h"
#include "PublicFunction.h"
#include "PlayAudio.h"
#include "PlayWindow.h"
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#define _CRT_SECURE_NO_WARNINGS



#pragma warning(disable: 4996)

#pragma warning(disable: 4005)


#pragma pack(1)
typedef struct {
	int h264width;
	int h264height;
	int h264framerate;
	int h264keyframes_persec;
	int h264bitrate;
}H264DECODEPARAM, * LPH264DECODEPARAM;
#pragma pack()


#define MAIN_ICON				0x1000
#define IDR_MENU				0x2000
#define IDR_FILE_OPEN			0x2001
#define IDR_FILE_CLOSE			0x2002
#define IDR_ABOUTBOX			0x2003

#define PCM_SUFFIX_NAME_WITH_DOT		".pcm"
#define PCM_SUFFIX_NAME					"pcm"
#define H264_SUFFIX_NAME				"h264"
#define H264_SUFFIX_NAME_WITH_DOT		".h264"


char SelectedFileName[MAX_PATH] = { 0 };






int GetH264DecodeParam(char* h264filename, LPH264DECODEPARAM lpparam) {

	HANDLE hfparam = CreateFileA(h264filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hfparam == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	int paramfilesize = GetFileSize(hfparam, 0);
	// 	if (paramfilesize != sizeof(H264DECODEPARAM))
	// 	{
	// 		CloseHandle(hfparam);
	// 		return FALSE;
	// 	}

	int ret = SetFilePointer(hfparam, paramfilesize - sizeof(H264DECODEPARAM), 0, FILE_BEGIN);
	if (ret <= 0)
	{
		return FALSE;
	}

	DWORD dwcnt = 0;
	ret = ReadFile(hfparam, (char*)lpparam, sizeof(H264DECODEPARAM), &dwcnt, 0);
	CloseHandle(hfparam);
	if (ret == 0 || dwcnt != sizeof(H264DECODEPARAM)) {
		return FALSE;
	}
	return TRUE;
}



int __stdcall PlayWindowEntry(char* lpfilename) {

	char h264filename[MAX_PATH];
	int ret = KeepDotRemoveSuffixNameFromFileName(lpfilename, h264filename);
	lstrcatA(h264filename, H264_SUFFIX_NAME);


	H264DECODEPARAM stparam = { 0 };
	ret = GetH264DecodeParam(h264filename, &stparam);
	if (stparam.h264width == FALSE || stparam.h264height == FALSE || stparam.h264bitrate == FALSE ||
		stparam.h264framerate == FALSE || stparam.h264keyframes_persec == FALSE)
	{
		MessageBoxA(0, "h264 param file not found", "h264 param file not found", MB_OK);
		return FALSE;
	}

	char pcmfilename[MAX_PATH];
	ret = KeepDotRemoveSuffixNameFromFileName(lpfilename, pcmfilename);
	lstrcatA(pcmfilename, PCM_SUFFIX_NAME);

	ret = PlayWindow(stparam.h264width, stparam.h264height, stparam.h264framerate, h264filename, pcmfilename);

	return TRUE;
}



int __stdcall GetOpenFile() {
	char szFileName[MAX_PATH] = { 0 };
	OPENFILENAMEA openFileName = { 0 };
	openFileName.lStructSize = sizeof(OPENFILENAME);
	openFileName.nMaxFile = MAX_PATH;
	openFileName.lpstrFilter = "\0\0";
	openFileName.lpstrFile = szFileName;
	openFileName.nFilterIndex = 1;
	openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&openFileName)/* && (strstr(openFileName.lpstrFile, H264_SUFFIX_NAME_WITH_DOT) || strstr(openFileName.lpstrFile, PCM_SUFFIX_NAME_WITH_DOT))*/)
	{
		PlayWindowEntry(openFileName.lpstrFile);
	}
	else {
		return FALSE;
	}

	return TRUE;
}





LRESULT CALLBACK MainWindowProcess(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
	case WM_CREATE:
	{
		DragAcceptFiles(hWnd, TRUE);
		return 0;
	}


	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDR_FILE_OPEN:
		{
			CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)GetOpenFile, 0, 0, 0));
			break;
		}

		case IDR_ABOUTBOX:
		{
			MessageBoxA(hWnd, "ÍøÂçÒôÊÓÆµ²¥·ÅÆ÷", "ÍøÂçÒôÊÓÆµ²¥·ÅÆ÷", MB_OK);
			break;
		}

		case IDR_FILE_CLOSE:
		{
			SendMessageA(hWnd, WM_DESTROY, 0, 0);
			break;
		}
		}

		return 0;
	}


	case WM_DROPFILES:
	{
		HDROP hDrop = (HDROP)wParam;
		int nFileNum = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
		for (int i = 0; i < nFileNum; i++)
		{
			int ret = DragQueryFileA(hDrop, i, SelectedFileName, MAX_PATH);
			if (ret && (strstr(SelectedFileName, H264_SUFFIX_NAME_WITH_DOT) || strstr(SelectedFileName, PCM_SUFFIX_NAME_WITH_DOT)))
			{
				CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PlayWindowEntry, SelectedFileName, 0, 0));
			}
			else {
				MessageBoxA(hWnd, "not video format file", "not video format file", MB_OK);
			}
		}
		DragFinish(hDrop);
		return 0;
	}

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	}

	return DefWindowProcA(hWnd, nMsg, wParam, lParam);
}










BOOL __stdcall MainWindow(HINSTANCE hInstance)
{
	char szWinCap[] = "ÒôÊÓÆµ²¥·ÅÆ÷";
	char szWinClassName[] = "MainWindow";
	WNDCLASSEXA stWCE = { 0 };
	stWCE.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
	stWCE.lpfnWndProc = MainWindowProcess;
	stWCE.cbClsExtra = 0;
	stWCE.cbWndExtra = 0;
	stWCE.hInstance = hInstance;
	stWCE.hCursor = LoadCursor(NULL, IDC_ARROW);
	stWCE.hbrBackground = (HBRUSH)(COLOR_BTNFACE);
	stWCE.lpszMenuName = NULL;
	stWCE.lpszClassName = szWinClassName;
	stWCE.cbSize = sizeof(WNDCLASSEXA);
	stWCE.hIcon = LoadIconA(hInstance, (LPCSTR)MAIN_ICON);
	stWCE.hIconSm = LoadIconA(hInstance, (LPCSTR)MAIN_ICON);

	ATOM nAtom = RegisterClassExA(&stWCE);
	if (0 == nAtom)
	{
		return FALSE;
	}

	//char szScreenDCName[] = {'D','I','S','P','L','A','Y',0};
	//HDC  hdc = CreateDCA(szScreenDCName, NULL, NULL, NULL);
	//int xscrn = GetDeviceCaps(hdc, HORZRES);
	//int yscrn = GetDeviceCaps(hdc, VERTRES);
	//DeleteDC(hdc);

	RECT deskworkrect = { 0 };
	int ret = SystemParametersInfoA(SPI_GETWORKAREA, 0, &deskworkrect, 0);
	int xscrn = deskworkrect.right - deskworkrect.left;
	int yscrn = deskworkrect.bottom - deskworkrect.top;

	int winwidth = xscrn / 2;
	int winheight = yscrn / 2;
	int winleft = (xscrn - winwidth) / 2;
	int wintop = (yscrn - winheight) / 2;

	HMENU hmenu = LoadMenuA(hInstance, MAKEINTRESOURCEA(IDR_MENU));
	hwndPlay = CreateWindowExA(WS_EX_CLIENTEDGE, szWinClassName, szWinCap, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_MAXIMIZE,
		winleft, wintop, winwidth, winheight, NULL, hmenu, hInstance, NULL);
	if (NULL == hwndPlay)
	{
		return NULL;
	}

	int iRet = ShowWindow(hwndPlay, SW_SHOWNORMAL);
	iRet = UpdateWindow(hwndPlay);

	while (TRUE)
	{
		MSG msg = { 0 };
		iRet = GetMessageA(&msg, NULL, 0, 0);		//postquitmessage can make this funtion return 0,error -1,normal message >0
		if (iRet <= 0)
		{
			//hwnd can not be run with closehandle
			break;
		}
		else {
			TranslateMessage(&msg);
			iRet = DispatchMessageA(&msg);
		}
	}

	nAtom = UnregisterClassA(szWinClassName, 0);
	return TRUE;
}