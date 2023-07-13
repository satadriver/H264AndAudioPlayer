
#pragma once


#include "avformat.h"

#include "stdint.h"






#include <Windows.h>


//#ifdef int_fast8_t
#undef int_fast8_t
//#endif

//#ifdef int_fast16_t
#undef int_fast16_t
//#endif

//#ifdef uint_fast16_t
#undef uint_fast16_t
//#endif

//#ifdef uint_fast32_t
#undef uint_fast32_t
//#endif

//#ifdef uint_fast64_t
#undef uint_fast64_t
//#endif





//int __stdcall H264Decode(char * filename,unsigned char ** lpPlayBmpDataPtr,int * iH264Width,int * iH264Height,HWND * hWnd,int h264framerate);
int __stdcall H264Decode(DWORD dwParams[]);