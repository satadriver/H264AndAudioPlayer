
#pragma once
#include <windows.h>
#include "H264AndAudioPlayer.h"



#define LOG_FILENAME "log.txt"

int KeepDotRemoveSuffixNameFromFileName(char* lpFileName, char* lpFilePath) {
	for (int i = lstrlenA(lpFileName); i > 0; i--)
	{
		if (lpFileName[i] == '.')
		{
			i++;
			memmove(lpFilePath, lpFileName, i);
			*(lpFilePath + i) = 0;
			return TRUE;
		}
	}

	return FALSE;
}

int GetPathFromFileName(char* lpFileName, char* lpFilePath) {
	for (int i = lstrlenA(lpFileName); i > 0; i--)
	{
		if (lpFileName[i] == '\\')
		{
			i++;
			memmove(lpFilePath, lpFileName, i);
			*(lpFilePath + i) = 0;
			return TRUE;
		}
	}

	return FALSE;
}



int WriteLog(char* pLog)
{
	HANDLE hFile = CreateFileA(LOG_FILENAME, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	int iRet = SetFilePointer(hFile, 0, 0, FILE_END);

	DWORD dwCnt = 0;
	iRet = WriteFile(hFile, pLog, lstrlenA(pLog), &dwCnt, 0);
	CloseHandle(hFile);

	if (iRet == FALSE || dwCnt != lstrlenA(pLog)) {
		return FALSE;
	}

	return TRUE;
}