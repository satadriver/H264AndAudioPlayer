#pragma once

#include <windows.h>
#include "PlayWindow.h"
#include "H264Decode.h"
#include "PlayAudio.h"
#include "MainWindow.h"
#include "PublicFunction.h"






int __stdcall WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd) {

	int ret = MainWindow(hInstance);

	return TRUE;
}