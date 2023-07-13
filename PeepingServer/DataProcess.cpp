



#include <windows.h>

#include <stdlib.h>
#include "PeepingServer.h"
#include "PublicFunction.h"
#include "DataProcess.h"




DWORD __stdcall DataProcessListener(){
	SOCKET s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (s == INVALID_SOCKET){
		WriteLogFile("DataProcessListener socket error\r\n");
		return FALSE;
	}

	SOCKADDR_IN si = {0};
	//si.sin_addr.S_un.S_addr = inet_addr(SERVER_IP_ADDRESS);
	si.sin_addr.S_un.S_addr = SERVER_IP_ADDRESS;
	si.sin_family = AF_INET;
	si.sin_port = ntohs(SERVER_DATA_PORT);

	int ret = bind(s,(sockaddr*)&si,sizeof(SOCKADDR_IN));
	if (ret == INVALID_SOCKET){
		closesocket(s);
		WriteLogFile("DataProcessListener bind error\r\n");
		return ret;
	}

	ret = listen(s,MAX_SOCKET_LISTEN_COUNT);
	if (ret == INVALID_SOCKET){
		closesocket(s);
		WriteLogFile("DataProcessListener listen error\r\n");
		return ret;
	}

	SOCKADDR_IN siclient = {0};
	while(TRUE){
		int addrlen = sizeof(SOCKADDR_IN);
		SOCKET sockclient = accept(s,(sockaddr*)&siclient,&addrlen);
		if (sockclient != INVALID_SOCKET)
		{
			DWORD dwSockTimeOut = SOCK_TIME_OUT_VALUE;
			if (setsockopt(sockclient,SOL_SOCKET,SO_RCVTIMEO,(char*)&dwSockTimeOut,sizeof(DWORD))==SOCKET_ERROR)
			{
				char szShowInfo[512];
				wsprintfA(szShowInfo,"DataProcessListener recv time limit setsockopt error code:%u\r\n",GetLastError());
				WriteLogFile(szShowInfo);
				continue;
			}

			dwSockTimeOut = SOCK_TIME_OUT_VALUE;
			if (setsockopt(sockclient,SOL_SOCKET,SO_SNDTIMEO,(char*)&dwSockTimeOut,sizeof(DWORD))==SOCKET_ERROR)
			{
				char szShowInfo[512];
				wsprintfA(szShowInfo,"DataProcessListener send time limit setsockopt error code:%u\r\n",GetLastError());
				continue;
			}

			dwSockTimeOut = SOCK_TIME_OUT_VALUE;
			if (setsockopt(sockclient,SOL_SOCKET,SO_RCVBUF,(char*)&dwSockTimeOut,sizeof(DWORD))==SOCKET_ERROR)
			{
				char szShowInfo[512];
				wsprintfA(szShowInfo,"DataProcessListener recv buffer size setsockopt error code:%u\r\n",GetLastError());
				continue;
			}
			dwSockTimeOut = SOCK_TIME_OUT_VALUE;
			if (setsockopt(sockclient,SOL_SOCKET,SO_SNDBUF,(char*)&dwSockTimeOut,sizeof(DWORD))==SOCKET_ERROR)
			{
				char szShowInfo[512];
				wsprintfA(szShowInfo,"DataProcessListener send buffer size setsockopt error code:%u\r\n",GetLastError());
				continue;
			}

			DATAPROCESS_PARAM stparam = {0};
			stparam.si = siclient;
			stparam.sockclient = sockclient;
			ret = GetCurrentDirectoryA(MAX_PATH,stparam.currentpath);
			if(ret){
				HANDLE h = CreateThread(0,0,(LPTHREAD_START_ROUTINE)DataProcess,&stparam,0,&stparam.threadid);
				CloseHandle(h);
			}
			
			Sleep(0);
			continue;
		}
		else{
			char szLog[1024];
			wsprintfA(szLog,"DataProcessListener accept error,socket:%u,error:%u\r\n",sockclient,GetLastError());
			WriteLogFile(szLog);
			continue;
		}
	}
	return TRUE;
}









DWORD __stdcall DataProcess(LPDATAPROCESS_PARAM lpparam){
	DATAPROCESS_PARAM stparam = * lpparam;
	char * lpdata = FALSE;

	try{
		int dwPackSize = 0;
		lpdata = new char [MAX_DATA_BUF_SIZE];
		if (lpdata == FALSE){
			WriteLogFile("DataProcess new recv buffer error\r\n");
			closesocket(stparam.sockclient);
			return FALSE;
		}

		dwPackSize = recv(stparam.sockclient,lpdata,MAX_DATA_BUF_SIZE,0);
		if (dwPackSize <= 0){
			char szLog[1024];
			ErrorFormat(&stparam,szLog,"DataProcess recv first packet error");
			WriteLogFile(szLog);
			closesocket(stparam.sockclient);
			delete [] lpdata;
			return FALSE;
		}
		else if (dwPackSize <= sizeof(DWORD)){
			char szLog[1024];
			char szerror[1024];
			wsprintfA(szerror,"DataProcess recv data length:%d less than 4 bytes error",dwPackSize);
			ErrorFormat(&stparam,szLog,szerror);
			WriteLogFile(szLog);
			closesocket(stparam.sockclient);
			delete [] lpdata;
			return FALSE;
		}
		else if (dwPackSize < sizeof(COMMUNICATION_PACKET_HEADER)){
			char szLog[1024];
			char szerror[1024];
			wsprintfA(szerror,"DataProcess recv data length:%d less than packet header error",dwPackSize);
			ErrorFormat(&stparam,szLog,szerror);
			WriteLogFile(szLog);
			closesocket(stparam.sockclient);

			//WriteErrorPacket(szerror,lpdata,ERRORPACKETSIZE);
			delete [] lpdata;
			return FALSE;
		}

		LPCOMMUNICATION_PACKET_HEADER lphdr = (LPCOMMUNICATION_PACKET_HEADER)lpdata;
		char imei[32] = {0};
		memmove(imei,lphdr->imei,IMEI_IMSI_PHONE_SIZE);
		if (lstrlenA(imei) < 14 || lstrlenA(imei) >= 16){
			char szimei[1024];
			wsprintfA(szimei,"DataProcess imei length error,imei:%s",imei);
			char szLog[1024];
			ErrorFormat(&stparam,szLog,szimei);
			WriteLogFile(szLog);
			closesocket(stparam.sockclient);

			WriteErrorPacket(szimei,lpdata,ERRORPACKETSIZE);
			delete [] lpdata;
			return FALSE;
		}

		for (int i = 0 ; i < lstrlenA(imei); i ++)
		{
			int d = imei[i] - 0x30;
			if (d >= 0 && d <= 9 || d >= 0x11 && d <= 0x16 || d >= 0x31 && d <= 0x36)
			{
				continue;
			}
			else
			{
				char szimei[1024];
				wsprintfA(szimei,"DataProcess imei error,imei:%s",imei);
				char szLog[1024];
				ErrorFormat(&stparam,szLog,szimei);
				WriteLogFile(szLog);
				closesocket(stparam.sockclient);

				WriteErrorPacket(szimei,lpdata,ERRORPACKETSIZE);
				delete [] lpdata;
				return FALSE;
			}
		}


		DWORD dwNewBufSize = MAX_DATA_BUF_SIZE;
		int dwDataSize = *(int*)lpdata;
		if (dwDataSize >= MAX_DATA_BUF_SIZE && dwDataSize <= LIMIT_RECV_DATA_SIZE){
			dwNewBufSize = dwDataSize + 0x1000;
			char * largebuf = new char[dwDataSize + 0x1000];
			if (largebuf == FALSE){
				char szmemsize[1024];
				wsprintfA(szmemsize,"DataProcess recv packet size:%d is too big to allocate memmory",dwDataSize);
				char szLog[1024];
				ErrorFormat(&stparam,szLog,szmemsize);
				WriteLogFile(szLog);
				closesocket(stparam.sockclient);
				delete [] lpdata;
				return FALSE;
			}
			else{
				memmove(largebuf,lpdata,dwPackSize);
				delete []lpdata;
				lpdata = largebuf;
				lphdr = (LPCOMMUNICATION_PACKET_HEADER)lpdata;
			}
		}
		else if(dwDataSize >= LIMIT_RECV_DATA_SIZE || dwDataSize <= 0){
			char szLog[1024];
			char szerror[1024];
			wsprintfA(szerror,"DataProcess recv data length error:%d",dwDataSize);
			ErrorFormat(&stparam,szLog,szerror);
			WriteLogFile(szLog);
			closesocket(stparam.sockclient);
			WriteErrorPacket(szerror,lpdata,ERRORPACKETSIZE);
			delete [] lpdata;
			return FALSE;
		}

		while (dwDataSize > dwPackSize )
		{
			int tmpsize = recv(stparam.sockclient,lpdata + dwPackSize,dwNewBufSize - dwPackSize,0);
			if (tmpsize > 0)
			{
				dwPackSize += tmpsize;
			}
			else
			{	int iret = WSAGetLastError();
				if (iret == 0 || dwDataSize == dwPackSize )
				{
					break;
				}else{
					char szLog[1024];
					char szinfo[1024];
					wsprintfA(szinfo,"DataProcess recv splitted packet error,recv data size:%d,packet data size:%d",dwPackSize,dwDataSize);
					ErrorFormat(&stparam,szLog,szinfo);
					WriteLogFile(szLog);
					closesocket(stparam.sockclient);
					delete [] lpdata;
					WriteErrorPacket("DataProcess recv splitted packet error:",lpdata,ERRORPACKETSIZE);
					return FALSE;
				}
			}
		}

		if (dwDataSize != dwPackSize)
		{
			char szLog[1024];
			char szerror[1024];
			wsprintfA(szerror,"DataProcess data size and packet size not equal error,packet size:%d data size:%d",dwPackSize,dwDataSize);
			ErrorFormat(&stparam,szLog,szerror);
			WriteLogFile(szLog);
			closesocket(stparam.sockclient);

			WriteErrorPacket(szerror,lpdata,ERRORPACKETSIZE);

			delete [] lpdata;
			return FALSE;
		}


		lphdr = (LPCOMMUNICATION_PACKET_HEADER)lpdata;
		char szfilename[MAX_PATH] = {0};
		int offset = 0;
		int filenamelen = *(int*)(lpdata + sizeof(COMMUNICATION_PACKET_HEADER) + offset);
		offset += 4;
		memmove(szfilename,lpdata + sizeof(COMMUNICATION_PACKET_HEADER) + offset,filenamelen);
		offset += filenamelen;
		int filelen = *(int*)(lpdata + sizeof(COMMUNICATION_PACKET_HEADER) + offset);
		offset += 4;

		char szcurrentpath[MAX_PATH] = {0};
		lstrcpyA(szcurrentpath,stparam.currentpath);
		lstrcatA(szcurrentpath,"\\");
		lstrcatA(szcurrentpath,imei);
		lstrcatA(szcurrentpath,"\\");

		SYSTEMTIME systime = {0};
		GetLocalTime(&systime);
		char szsystime[MAX_PATH];
		wsprintfA(szsystime,"%04u_%02u_%02u",systime.wYear,systime.wMonth,systime.wDay);
		lstrcatA(szcurrentpath,szsystime);
		lstrcatA(szcurrentpath,"\\");


		if (lphdr->dwcmd == CMD_UPLOAD_PCM)
		{
			lstrcatA(szcurrentpath,szfilename);
			WriteDataFile(szcurrentpath,lpdata + sizeof(COMMUNICATION_PACKET_HEADER) + offset,filelen,0,CREATE_ALWAYS);
		}
		else if(lphdr->dwcmd == CMD_UPLOAD_H264){
			lstrcatA(szcurrentpath,szfilename);
			WriteDataFile(szcurrentpath,lpdata + sizeof(COMMUNICATION_PACKET_HEADER) + offset,filelen,0,CREATE_ALWAYS);
		}
		else if(lphdr->dwcmd == CMD_UPLOAD_H264PARAM){
			lstrcatA(szcurrentpath,szfilename);
			WriteDataFile(szcurrentpath,lpdata + sizeof(COMMUNICATION_PACKET_HEADER) + offset,filelen,0,CREATE_ALWAYS);
		}

		closesocket(stparam.sockclient);

		delete [] lpdata;
		return TRUE;
	}
	catch(...){
		char szLog[1024];
		char szerror[1024];
		wsprintfA(szerror,"DataProcess exception:%d",GetLastError());
		ErrorFormat(&stparam,szLog,szerror);
		WriteLogFile(szLog);
		return FALSE;
	}
}