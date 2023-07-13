#pragma once
#include <windows.h>
#include "H264AndAudioPlayer.h"
#include "H264Decode.h"
#include "PublicFunction.h"
#include "PlayAudio.h"
#include <shellapi.h>
#include "PlayWindow.h"
#include <map>
#pragma comment(lib, "shell32.lib")


#define H264_DECODED_BMP_BITS	24

//#define MSG_PLAY_MEDIA_COMPLETE WM_USER + 1

int iSecondsEclapsed = 0;



int PCM_BITS_PERCHANNAL = 0;
int PCM_CHANNEL_COUNT = 0;
int PCM_SAMPLE_FREQUENCY = 0;
int PLAYRATE = 0;
char* lpaudiodata = NULL;
int audiodatasize = 0;
char* AudioDataPtr = NULL;


unsigned char* lpPlayBmpData = NULL;
HWND hwndPlay = NULL;
int iH264Height = 0;
int iH264Width = 0;
int iPlayQuitCode = FALSE;
int iStartStopFlag = FALSE;
int iOrientFlag = FALSE;
char* lpH264FileName = 0;
char* lpPcmFileName = 0;
int iFrameRate = 0;

DWORD dwPcmPrepareOK = FALSE;
DWORD dwPcmCompleteFlag = FALSE;
DWORD dwH264CompleteFlag = FALSE;
DWORD dwH264PrepareOK = FALSE;


#define TMP_H264_PROGRESS_FILENAME "tmph264.h264"
#define TMP_PCM_PROGRESS_FILENAME "tmppcm.pcm"

using namespace std;

map <int, int> H264IDRFrameMap;
map <int, int> ::iterator H264IDRFrameMapIt;



int GetAndSetScrollPosFromAudio(HWND hwndPlay, char* AudioDataPtr) {
	int offset = (AudioDataPtr - lpaudiodata) * 100 / audiodatasize;
	if (offset > 100 || offset < 0)
	{
		return FALSE;
	}

	int ret = SetScrollPos(hwndPlay, SB_HORZ, offset, TRUE);

	return TRUE;
}


int GetH264FrameCount(char* h264filename) {

	HANDLE hf264 = CreateFileA(h264filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hf264 == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	int h264filesize = GetFileSize(hf264, 0);

	char* lpdata = new char[h264filesize + 0x1000];
	if (lpdata == NULL)
	{
		CloseHandle(hf264);
		return FALSE;
	}

	DWORD dwcnt = 0;
	int ret = ReadFile(hf264, (char*)lpdata, h264filesize, &dwcnt, 0);
	CloseHandle(hf264);
	if (ret == 0 || dwcnt != h264filesize) {
		delete[] lpdata;
		return FALSE;
	}


	int counter = 0;
	H264IDRFrameMap.clear();

	for (int i = 0; i < h264filesize; i++)
	{
		if (memcmp(lpdata + i, "\x00\x00\x00\x01\x67\x42", 6) == 0)
		{
			H264IDRFrameMap.insert(map<int, int>::value_type(counter, i));
			counter++;
		}
	}

	delete[] lpdata;

	char szinfo[1024];
	wsprintfA(szinfo, "find h264 idr frame total:%u\r\n", counter);
	WriteLog(szinfo);
	return counter;
}


/*
int __stdcall TimerProc(HWND hWnd,UINT message,UINT iTimerID,DWORD dwTime){
	iSecondsEclapsed ++;
	return TRUE;

	HDC hdc=GetDC(hWnd);
	RECT rect = {0};
	GetClientRect(hWnd,&rect);
	char szseconds[16];
	wsprintfA(szseconds,"%d",iSecondsEclapsed);
	int ret =DrawTextA(hdc,szseconds,-1,&rect,DT_BOTTOM|DT_RIGHT);
	//EndPaint(hWnd,&ps);
	ReleaseDC(hWnd,hdc);
	//ValidateRect(hWnd,NULL);
	return TRUE;
}
*/




int PlayMediaFile(DWORD  dwParams[]) {
	char* h264filename = (char*)dwParams[0];
	char* pcmfilename = (char*)dwParams[1];
	DWORD h264framerate = dwParams[2];

	int idrframecnt = GetH264FrameCount(h264filename);
	if (idrframecnt <= 0)
	{
		MessageBoxA(0, "idr count zero", "idr frame count is 0", MB_OK);
		return FALSE;
	}

	while (TRUE) {

		dwPcmPrepareOK = FALSE;
		dwPcmCompleteFlag = FALSE;
		DWORD dwPcmParams[16] = { 0 };
		dwPcmParams[0] = (DWORD)pcmfilename;
		dwPcmParams[1] = (DWORD)&dwPcmPrepareOK;
		dwPcmParams[2] = (DWORD)&dwPcmCompleteFlag;
		dwPcmParams[3] = 0;
		int ret = CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PlayAudioProcess, dwPcmParams, 0, 0));

		dwH264CompleteFlag = FALSE;
		dwH264PrepareOK = FALSE;
		DWORD dwH264Params[16] = { 0 };
		dwH264Params[0] = (DWORD)h264filename;
		dwH264Params[1] = (DWORD)&lpPlayBmpData;
		dwH264Params[2] = (DWORD)&iH264Width;
		dwH264Params[3] = (DWORD)&iH264Height;
		dwH264Params[4] = (DWORD)h264framerate;
		dwH264Params[5] = (DWORD)&dwH264PrepareOK;
		dwH264Params[6] = (DWORD)&dwH264CompleteFlag;
		ret = CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)H264Decode, dwH264Params, 0, 0));

		while ((dwPcmPrepareOK == FALSE || dwH264PrepareOK == FALSE) && hwndPlay && iPlayQuitCode == FALSE)
		{
			Sleep(WAIT_START_STOP_INTERVAL);
		}

		if (hwndPlay == FALSE || iPlayQuitCode == TRUE)
		{
			return TRUE;
		}

		iStartStopFlag = TRUE;

		iSecondsEclapsed = 0;
		while ((dwPcmCompleteFlag == FALSE || dwH264CompleteFlag == FALSE) && hwndPlay && iPlayQuitCode == FALSE) {
			ret = GetAndSetScrollPosFromAudio(hwndPlay, AudioDataPtr);
			Sleep(WAIT_START_STOP_INTERVAL);
			iSecondsEclapsed += WAIT_START_STOP_INTERVAL;
		}

		if (hwndPlay == FALSE || iPlayQuitCode == TRUE)
		{
			return TRUE;
		}

		//iStartStopFlag = FALSE;
		//while (iStartStopFlag == FALSE&& hwndPlay &&iPlayQuitCode == FALSE)
		//{
		//	Sleep(WAIT_START_STOP_INTERVAL);
		//}

		//if (hwndPlay == FALSE|| iPlayQuitCode == TRUE)
		//{
		//	return TRUE;
		//}
	}

	return TRUE;
}








int PlayNewProgress(int progressValue) {

	iStartStopFlag = FALSE;
	iPlayQuitCode = TRUE;
	Sleep(WAIT_START_STOP_INTERVAL + 100);
	iPlayQuitCode = FALSE;

	//int lpaudiopos = (int)((int)lpaudiodata + (audiodatasize * progressValue)/100 / PLAYRATE *PLAYRATE );
	int lpaudiopos = (audiodatasize * progressValue) / 100 / PLAYRATE * PLAYRATE;
	if (lpaudiopos > (int)(audiodatasize) || lpaudiopos < 0)
	{
		return FALSE;
	}

	dwPcmPrepareOK = FALSE;
	dwPcmCompleteFlag = FALSE;
	DWORD dwPcmParams[16] = { 0 };
	dwPcmParams[0] = (DWORD)lpPcmFileName;
	dwPcmParams[1] = (DWORD)&dwPcmPrepareOK;
	dwPcmParams[2] = (DWORD)&dwPcmCompleteFlag;
	dwPcmParams[3] = lpaudiopos;
	int ret = CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PlayAudioProcess, dwPcmParams, 0, 0));

	int h264idrpos = (progressValue * H264IDRFrameMap.size()) / 100;
	H264IDRFrameMapIt = H264IDRFrameMap.find(h264idrpos);
	if (H264IDRFrameMapIt == H264IDRFrameMap.end())
	{
		MessageBoxA(0, "not found idr frame in map", "not found idr frame in map", MB_OK);
		return FALSE;
	}
	int h264progpos = H264IDRFrameMapIt->second;

	HANDLE hfh264 = CreateFileA(lpH264FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hfh264 == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	int h264filesize = GetFileSize(hfh264, 0);
	if (h264filesize == FALSE)
	{
		CloseHandle(hfh264);
		return FALSE;
	}
	ret = SetFilePointer(hfh264, h264progpos, 0, FILE_BEGIN);
	if (ret == INVALID_SET_FILE_POINTER)
	{
		CloseHandle(hfh264);
		return FALSE;
	}
	int h264progsize = h264filesize - h264progpos;
	char* lph264progbuf = new char[h264progsize + 0x1000];
	if (lph264progbuf == FALSE)
	{
		CloseHandle(hfh264);
		return FALSE;
	}
	DWORD dwcnt = 0;
	ret = ReadFile(hfh264, lph264progbuf, h264progsize, &dwcnt, 0);
	CloseHandle(hfh264);
	if (ret == FALSE || dwcnt != h264progsize)
	{
		delete[] lph264progbuf;
		return FALSE;
	}


	char h264tmpfilename[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, h264tmpfilename);
	lstrcatA(h264tmpfilename, "\\");
	lstrcatA(h264tmpfilename, TMP_H264_PROGRESS_FILENAME);
	HANDLE hftmpprog = CreateFileA(h264tmpfilename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY /*| FILE_FLAG_DELETE_ON_CLOSE*/ | FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if (hftmpprog == INVALID_HANDLE_VALUE)
	{
		delete[] lph264progbuf;
		return FALSE;
	}

	dwcnt = 0;
	ret = WriteFile(hftmpprog, lph264progbuf, h264progsize, &dwcnt, 0);
	CloseHandle(hftmpprog);
	delete[] lph264progbuf;
	if (ret == FALSE || dwcnt != h264progsize)
	{
		return FALSE;
	}


	dwH264CompleteFlag = FALSE;
	dwH264PrepareOK = FALSE;
	DWORD dwH264Params[16] = { 0 };
	dwH264Params[0] = (DWORD)h264tmpfilename;
	dwH264Params[1] = (DWORD)&lpPlayBmpData;
	dwH264Params[2] = (DWORD)&iH264Width;
	dwH264Params[3] = (DWORD)&iH264Height;
	dwH264Params[4] = (DWORD)iFrameRate;
	dwH264Params[5] = (DWORD)&dwH264PrepareOK;
	dwH264Params[6] = (DWORD)&dwH264CompleteFlag;
	ret = CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)H264Decode, dwH264Params, 0, 0));


	while (dwH264PrepareOK == FALSE)
	{
		Sleep(WAIT_START_STOP_INTERVAL);
	}

	iStartStopFlag = TRUE;

	while ((dwPcmCompleteFlag == FALSE || dwH264CompleteFlag == FALSE) && hwndPlay && iPlayQuitCode == FALSE) {
		ret = GetAndSetScrollPosFromAudio(hwndPlay, AudioDataPtr);
		Sleep(WAIT_START_STOP_INTERVAL);

	}

	if (hwndPlay == FALSE || iPlayQuitCode == TRUE)
	{
		return TRUE;
	}

	DWORD dwparams[16] = { 0 };
	dwparams[0] = (DWORD)lpH264FileName;
	dwparams[1] = (DWORD)lpPcmFileName;
	dwparams[2] = iFrameRate;
	CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PlayMediaFile, dwparams, 0, 0));
	Sleep(6000);
	return TRUE;
}




LRESULT CALLBACK PlayWindowProcess(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT stPS = { 0 };
		HDC hdcDst = BeginPaint(hWnd, &stPS);
		if (iH264Height && iH264Width && lpPlayBmpData)
		{
			char szScreenDCName[] = { 'D','I','S','P','L','A','Y',0 };
			HDC  hdcScreen = CreateDCA(szScreenDCName, NULL, NULL, NULL);
			HDC hdcSrc = CreateCompatibleDC(hdcScreen);

			int oldmode_widthbytes = (iH264Width * H264_DECODED_BMP_BITS + 31) / 32 * 4;
			DWORD dwDibtsSize = oldmode_widthbytes * iH264Height;
			BITMAPINFO pBMIH = { 0 };
			pBMIH.bmiHeader.biBitCount = H264_DECODED_BMP_BITS;
			pBMIH.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pBMIH.bmiHeader.biHeight = iH264Width;
			pBMIH.bmiHeader.biWidth = iH264Height;
			pBMIH.bmiHeader.biPlanes = 1;
			pBMIH.bmiHeader.biSizeImage = dwDibtsSize;
			char* pSrnData = 0;
			HBITMAP hBM = CreateDIBSection(0, (LPBITMAPINFO)&pBMIH, DIB_RGB_COLORS, (void**)&pSrnData, 0, 0);	//GetDIBits get get dibts from bitmap structor
			if (hBM && pSrnData)
			{
				char* lptmpbuf = new char[dwDibtsSize];
				char* lptmpbufptr = lptmpbuf + dwDibtsSize - oldmode_widthbytes;
				unsigned char* lpPlayBmpDataPtr = lpPlayBmpData;
				for (int i = 0; i < iH264Height; i++)
				{
					memmove(lptmpbufptr, lpPlayBmpDataPtr, oldmode_widthbytes);
					lptmpbufptr -= oldmode_widthbytes;
					lpPlayBmpDataPtr += oldmode_widthbytes;
				}

				//int newmode_widthbytes = (iH264Height * H264_DECODED_BMP_BITS + 31)/32* 4;
				char* lptmpdst = new char[dwDibtsSize];
				char* lptmpdstptr = lptmpdst + dwDibtsSize - H264_DECODED_BMP_BITS / 8;
				for (int j = 0; j < iH264Width; j++) {
					lptmpbufptr = lptmpbuf + dwDibtsSize - oldmode_widthbytes + j * (H264_DECODED_BMP_BITS / 8);
					for (int i = 0; i < iH264Height; i++)
					{
						memmove(lptmpdstptr, lptmpbufptr, H264_DECODED_BMP_BITS / 8);
						lptmpbufptr -= oldmode_widthbytes;
						lptmpdstptr -= (H264_DECODED_BMP_BITS / 8);
					}
				}

				memmove(pSrnData, lptmpdst, dwDibtsSize);


				/*
				char * lpSrnDataPtr = pSrnData + dwDibtsSize - H264_DECODED_BMP_BITS/8;
				for(int j = 0; j < iH264Width; j ++){
					lptmpbufptr = lptmpbuf + dwDibtsSize - oldmode_widthbytes + j*(H264_DECODED_BMP_BITS/8);
					for (int i = 0; i < iH264Height; i ++)
					{
						memmove(lpSrnDataPtr,lptmpbufptr,H264_DECODED_BMP_BITS/8);
						lptmpbufptr -= oldmode_widthbytes;
						lpSrnDataPtr -= (H264_DECODED_BMP_BITS/8);
					}
				}
				*/

				delete[] lptmpdst;
				delete[] lptmpbuf;

				HBITMAP hSrcBM = (HBITMAP)SelectObject(hdcSrc, hBM);
				int iX = pBMIH.bmiHeader.biWidth;
				int iY = pBMIH.bmiHeader.biHeight;
				RECT stRect = { 0 };
				int iRet = GetClientRect(hWnd, &stRect);
				iRet = StretchBlt(hdcDst, 0, 0, stRect.right - stRect.left, stRect.bottom - stRect.top, hdcSrc, 0, 0, iX, iY, SRCCOPY);
				DeleteObject(hSrcBM);
				DeleteObject(hBM);
			}

			DeleteDC(hdcScreen);
			DeleteDC(hdcSrc);
		}

		RECT scrnrect = { 0 };
		int ret = GetClientRect(hWnd, &scrnrect);
		char sztotalseconds[16];
		wsprintfA(sztotalseconds, "%d", iSecondsEclapsed / 1000);
		ret = DrawTextA(hdcDst, sztotalseconds, -1, &scrnrect, DT_SINGLELINE | DT_BOTTOM | DT_RIGHT);
		ReleaseDC(hWnd, hdcDst);

		EndPaint(hWnd, &stPS);
		return 0;
	}


	case WM_HSCROLL:
	{
		if ((wParam & 0xffff) == SB_THUMBTRACK)
		{
			int curpos = (wParam & 0xffff0000) >> 16;

		}
		else if ((wParam & 0xffff) == SB_THUMBPOSITION)
		{
			int dstpos = (wParam & 0xffff0000) >> 16;
			int prevpos = SetScrollPos(hWnd, SB_HORZ, dstpos, TRUE);
			int ret = CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PlayNewProgress, (LPVOID)dstpos, 0, 0));
		}
		return 0;
	}

	case WM_LBUTTONDOWN:
	{
		iStartStopFlag = iStartStopFlag ^ 1;
		return 0;
	}

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	case WM_CREATE: {
		//iSecondsEclapsed = 0;
		//SetTimer(hWnd,-1,1000,(TIMERPROC)TimerProc);
	}
	}

	return DefWindowProcA(hWnd, nMsg, wParam, lParam);
}









BOOL __stdcall PlayWindow(int width, int height, int h264framerate, char* h264filename, char* pcmfilename)
{
	lpH264FileName = h264filename;
	lpPcmFileName = pcmfilename;
	iFrameRate = h264framerate;

	if (width % 4 || height % 4)
	{
		return FALSE;
	}

	char szWinCap[] = "ÒôÊÓÆµ²¥·ÅÆ÷";
	char szWinClassName[] = "PlayWindow²¥·ÅÆ÷";
	WNDCLASSEXA stWCE = { 0 };
	stWCE.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
	stWCE.lpfnWndProc = PlayWindowProcess;
	stWCE.cbClsExtra = 0;
	stWCE.cbWndExtra = 0;
	stWCE.hInstance = NULL;
	stWCE.hCursor = LoadCursor(NULL, IDC_ARROW);
	stWCE.hbrBackground = (HBRUSH)(COLOR_BTNFACE);
	stWCE.lpszMenuName = NULL;
	stWCE.lpszClassName = szWinClassName;
	stWCE.cbSize = sizeof(WNDCLASSEXA);

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

	int iWindowWidth = 0;
	int iWindowHeight = 0;
	//mobile camera width is larger than height
	if (width >= height)
	{
		if (width >= yscrn)
		{
			iWindowHeight = yscrn;
		}
		else {
			iWindowHeight = width;
		}

		iWindowWidth = (height * iWindowHeight) / width;

	}
	else {
		WriteLog("height is larger than width\r\n");
		iOrientFlag = TRUE;
		if (height >= yscrn)
		{
			iWindowHeight = yscrn;
			iWindowWidth = (width * iWindowHeight) / height;

		}
		else {
			iWindowHeight = height;
			iWindowWidth = width;
		}
	}

	int windowleft = (xscrn - iWindowWidth) / 2;
	int windowtop = (yscrn - iWindowHeight) / 2;

	hwndPlay = CreateWindowExA(WS_EX_CLIENTEDGE, szWinClassName, szWinCap, WS_OVERLAPPEDWINDOW | WS_HSCROLL,
		windowleft, windowtop, iWindowWidth, iWindowHeight, NULL, 0, NULL, NULL);
	if (NULL == hwndPlay)
	{
		return NULL;
	}

	int iRet = ShowWindow(hwndPlay, SW_SHOW);
	iRet = UpdateWindow(hwndPlay);


	DWORD dwparams[16] = { 0 };
	dwparams[0] = (DWORD)h264filename;
	dwparams[1] = (DWORD)pcmfilename;
	dwparams[2] = h264framerate;
	CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PlayMediaFile, dwparams, 0, 0));

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

	hwndPlay = FALSE;
	iStartStopFlag = FALSE;
	lpPlayBmpData = NULL;
	iH264Width = NULL;
	iH264Height = NULL;

	iOrientFlag = FALSE;
	return TRUE;
}