
#pragma once
#include <Windows.h>

#define WAIT_START_STOP_INTERVAL 200

extern int PCM_BITS_PERCHANNAL;
extern int PCM_CHANNEL_COUNT;
extern int PCM_SAMPLE_FREQUENCY;
extern int PLAYRATE;

extern char* lpaudiodata;
extern int audiodatasize;
extern char* AudioDataPtr;

extern unsigned char* lpPlayBmpData;
extern HWND hwndPlay;
extern int iH264Height;
extern int iH264Width;
extern int iStartStopFlag;
extern int iPlayQuitCode;


BOOL __stdcall PlayWindow(int width, int height, int h264framerate, char* h264filename, char* pcmfilename);