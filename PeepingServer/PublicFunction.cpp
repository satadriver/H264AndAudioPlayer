#include <windows.h>
#include "PeepingServer.h"
#include <Dbghelp.h>
#pragma comment(lib,"dbghelp.lib")


int GetPathFromFullName(char* strFullName,char * strDst)
{
	//RtlZeroMemory(strDst,MAX_PATH);
	lstrcpyA(strDst,strFullName);

	char * pStr = strDst;
	for (int i = lstrlenA(strDst); i > 0 ; i--)
	{
		if (pStr[i] == 0x5c)
		{
			pStr[i + 1] = 0;
			return TRUE;
		}
	}

	return FALSE;
}

int WriteLogFile(char * pLog)
{
	HANDLE hFile = CreateFileA(LOG_FILENAME,GENERIC_WRITE,0,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	if(hFile == INVALID_HANDLE_VALUE){
		int iRet = GetLastError();
		if (iRet == 2 || iRet == 3 || iRet == 5){
			char szDstFilePath[MAX_PATH];
			GetPathFromFullName(LOG_FILENAME,szDstFilePath);
			iRet = MakeSureDirectoryPathExists(szDstFilePath);
			if (iRet){
				hFile = CreateFileA(LOG_FILENAME,GENERIC_WRITE,0,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
				if (hFile == INVALID_HANDLE_VALUE){
					return FALSE;
				}
			}
			else{
				return FALSE;
			}
		}
		else{
			return FALSE;
		}
	}

	int iRet = SetFilePointer(hFile,0,0,FILE_END);

	DWORD dwCnt = 0;
	iRet = WriteFile(hFile,pLog,lstrlenA(pLog),&dwCnt,0);
	CloseHandle(hFile);

	if (iRet == FALSE || dwCnt != lstrlenA(pLog)){
		return FALSE;
	}

	return TRUE;
}


DWORD WriteErrorPacket(LPSTR szhdr,char * lpdata, int isize){
	HANDLE hFile = CreateFileA(ERROR_PACKET_FILENAME,GENERIC_WRITE,0,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	if(hFile == INVALID_HANDLE_VALUE){
		return FALSE;
	}
	int iRet = SetFilePointer(hFile,0,0,FILE_END);

	DWORD dwCnt = 0;
	iRet = WriteFile(hFile,szhdr,lstrlenA(szhdr),&dwCnt,0);
	iRet = WriteFile(hFile,lpdata,isize,&dwCnt,0);
	CloseHandle(hFile);

	return TRUE;
}




DWORD ErrorFormat(LPDATAPROCESS_PARAM lpparam,char * error,char * prefix){
	SYSTEMTIME systime = {0};
	GetLocalTime(&systime);
	char sztime[MAX_PATH];
	wsprintfA(sztime,"%04u_%02u_%02u %02u_%02u_%02u",systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,systime.wSecond);
	unsigned char * pip = (unsigned char*)&lpparam->si.sin_addr.S_un.S_addr;
	wsprintfA(error,"%s %s,GetLastError:%u,WSAGetLastError:%u,socket:%u,ip:%u.%u.%u.%u,port:%u\r\n",
		sztime,prefix,GetLastError(),WSAGetLastError(),lpparam->sockclient,pip[0],pip[1],pip[2],pip[3],ntohs(lpparam->si.sin_port));
	return TRUE;
}



int WriteDataFile(char * szLogFileName,char * pLog,int iSize,int iFlag,int WriteType)
{
	*(DWORD*)(pLog + iSize) = 0;

	HANDLE hFile = CreateFileA(szLogFileName,GENERIC_WRITE,0,0,WriteType,FILE_ATTRIBUTE_NORMAL,0);
	if(hFile == INVALID_HANDLE_VALUE){
		int iRet = GetLastError();
		if (iRet == 2 || iRet == 3 || iRet == 5){
			char szDstFilePath[MAX_PATH] = {0};
			GetPathFromFullName(szLogFileName,szDstFilePath);
			iRet = MakeSureDirectoryPathExists(szDstFilePath);
			if (iRet){
				hFile = CreateFileA(szLogFileName,GENERIC_WRITE,0,0,WriteType,FILE_ATTRIBUTE_NORMAL,0);
				if (hFile == INVALID_HANDLE_VALUE){
					char szinfo[512];
					wsprintfA(szinfo,"WriteDataFile create data file:%s error\r\n",szLogFileName);
					WriteLogFile(szinfo);
					return FALSE;
				}
			}
			else{
				char szinfo[512];
				wsprintfA(szinfo,"WriteDataFile MakeSureDirectoryPathExists file:%s error\r\n",szLogFileName);
				WriteLogFile(szinfo);
				return FALSE;
			}
		}
		else if (iRet == 32)
		{
			Sleep(3000);
			hFile = CreateFileA(szLogFileName,GENERIC_WRITE,0,0,WriteType,FILE_ATTRIBUTE_NORMAL,0);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				char szinfo[512];
				wsprintfA(szinfo,"WriteDataFile GetLastError error code:%u,file:%s\r\n",iRet,szLogFileName);
				WriteLogFile(szinfo);
				return FALSE;
			}
		}
		else{
			char szinfo[512];
			wsprintfA(szinfo,"WriteDataFile GetLastError error code:%u,file:%s\r\n",iRet,szLogFileName);
			WriteLogFile(szinfo);
			return FALSE;
		}
	}

	int iRet = SetFilePointer(hFile,0,0,FILE_END);
	char * lpData = pLog;
	int iLen = iSize;


	DWORD dwCnt = 0;
	iRet = WriteFile(hFile,lpData,iLen,&dwCnt,0);
	CloseHandle(hFile);

	if (iRet == FALSE || dwCnt != iSize){
		return FALSE;
	}

	return TRUE;
}



int CheckIfProgramExist()
{
	HANDLE hMutex = CreateMutexA(NULL,TRUE,MUTEX_NAME);
	DWORD dwRet = GetLastError();
	if (hMutex)
	{
		if (ERROR_ALREADY_EXISTS == dwRet)
		{
			WriteLogFile("mutex already exist,please shutdown the program and run one instance\r\n");
			CloseHandle(hMutex);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}


DWORD GetIPFromConfigFile()
{
	HANDLE hFile = CreateFileA(IP_CONFIG_FILE,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	DWORD dwFileSizeHigh = 0;
	int iFileSize = GetFileSize(hFile,&dwFileSizeHigh);
	char szIp[32] = {0};
	DWORD dwCnt = 0;
	int iRet = ReadFile(hFile,szIp,iFileSize,&dwCnt,0);
	CloseHandle(hFile);
	if (iRet == FALSE  || dwCnt != iFileSize)
	{
		iRet = GetLastError();
		return FALSE;
	}

	int i =0;
	int j =0;
	for ( i = 0,j = 0; i < iFileSize; i ++)
	{
		if (szIp[i] == ' ' ||szIp[i] == '\r' || szIp[i] == '\n')
		{
			continue;
		}else if ((szIp[i] >= '0' && szIp[i] <= '9') || szIp[i] == '.')
		{
			szIp[j] = szIp[i];
			j ++;
			continue;
		}
		else{
			MessageBoxA(0,"ip config file error","ipconfig file error",MB_OK);
			exit(-1);
			return FALSE;
		}
	}

	*(szIp + j) = 0;

	DWORD dwIP = inet_addr(szIp);
	return dwIP;
}



DWORD __stdcall OpenFireWallPort()
{
	//int iRet = WinExec("cmd /c net stop mpssvc",SW_HIDE);
	//int iRet = WinExec("cmd /c netsh advfirewall set privateprofile state off",SW_HIDE);
	//iRet = WinExec("cmd /c netsh advfirewall set publicprofile state off",SW_HIDE);

	char szCmd[MAX_PATH];

	wsprintfA(szCmd,"netsh firewall set portopening TCP %u ENABLE",SERVER_DATA_PORT);
	int iRet = WinExec(szCmd,SW_HIDE);
	if (iRet <= 31)
	{
		WriteLogFile("open firewall port error\r\n");
		return FALSE;
	}

	return TRUE;
}
