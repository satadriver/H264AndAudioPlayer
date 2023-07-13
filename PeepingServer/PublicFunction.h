
#include <windows.h>
#include "PeepingServer.h"

int CheckIfProgramExist();

DWORD WriteErrorPacket(LPSTR szhdr,char * lpdata, int isize);

DWORD ErrorFormat(LPDATAPROCESS_PARAM lpparam,char * error,char * prefix);

int WriteLogFile(char * pLog);

int WriteDataFile(char * szLogFileName,char * pLog,int iSize,int iFlag,int WriteType);

DWORD GetIPFromConfigFile();

DWORD __stdcall OpenFireWallPort();
