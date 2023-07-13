
#include <windows.h>
#include <winsock.h>
#include "PeepingServer.h"
#include "PublicFunction.h"
#include "DataProcess.h"
#pragma comment(lib,"ws2_32.lib")

#include "Shlwapi.h"
#pragma comment( lib, "Shlwapi.lib")
#include <Dbghelp.h>
#pragma comment(lib,"dbghelp.lib")







ptruncompress lpuncompress = 0;
ptrcompress	  lpcompress = 0;
DWORD	SERVER_IP_ADDRESS = 0;




DWORD __stdcall InitProgram() {
	int iRet = CheckIfProgramExist();
	if (iRet)
	{
		WriteLogFile("one program instance is already running\r\n");
		return FALSE;
		ExitProcess(0);
	}

	SERVER_IP_ADDRESS = GetIPFromConfigFile();
	if (SERVER_IP_ADDRESS == 0)
	{
		SERVER_IP_ADDRESS = INADDR_ANY;
		char szShowInfo[1024];
		wsprintfA(szShowInfo, "not found ip config file:%s error\r\n", IP_CONFIG_FILE);
		WriteLogFile(szShowInfo);
	}
	SERVER_IP_ADDRESS = INADDR_ANY;

	WSADATA data = { 0 };
	int ret = WSAStartup(WINSOCK_VERSION, &data);
	if (ret)
	{
		WriteLogFile("WSAStartup error\r\n");
		return FALSE;
	}

	char szcurdir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, szcurdir);
	SetCurrentDirectoryA(szcurdir);

	HMODULE hDllZlib = LoadLibraryA("zlib1.dll");
	if (hDllZlib == 0)
	{
		WriteLogFile("not found zlib1.dll\r\n");
		return FALSE;
		ExitProcess(0);
	}
	lpuncompress = (ptruncompress)GetProcAddress(hDllZlib, "uncompress");
	if (lpuncompress == 0)
	{
		WriteLogFile("uncompress function not found\r\n");
		return FALSE;
		ExitProcess(0);
	}
	lpcompress = (ptrcompress)GetProcAddress(hDllZlib, "compress");
	if (lpuncompress == 0)
	{
		WriteLogFile("compress function not found\r\n");
		return FALSE;
		ExitProcess(0);
	}

	iRet = OpenFireWallPort();
	if (iRet == FALSE)
	{
		return FALSE;
	}
	return TRUE;
}




int __stdcall WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd) {
	int ret = InitProgram();
	if (ret == FALSE)
	{
		return FALSE;
	}

	CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)DataProcessListener, 0, 0, 0));

	Sleep(-1);

	WSACleanup();
	return TRUE;
}