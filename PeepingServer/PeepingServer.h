#pragma once
#include <windows.h>
#include "zconf.h"
#include "zlib.h"

#define WINSOCK_VERSION					0x0202
#define MUTEX_NAME						"PeepingServer"
#define IP_CONFIG_FILE					"ip.txt"
#define LOG_FILENAME					"log.txt"
#define ERROR_PACKET_FILENAME			"errorpacket.dat"
#define IMEI_IMSI_PHONE_SIZE			16
#define SERVER_DATA_PORT				65535
#define SOCK_TIME_OUT_VALUE				1800000
#define MAX_SOCKET_LISTEN_COUNT			16
#define MAX_DATA_BUF_SIZE				0x80000
#define ERRORPACKETSIZE					64
#define LIMIT_RECV_DATA_SIZE			0x7fffffff

#define CMD_UPLOAD_PCM					1
#define CMD_UPLOAD_H264					2
#define CMD_UPLOAD_H264PARAM			3


typedef struct{
	DWORD dwsize;
	DWORD dwcmd;
	char imei[IMEI_IMSI_PHONE_SIZE];
}COMMUNICATION_PACKET_HEADER,*LPCOMMUNICATION_PACKET_HEADER;


typedef struct{
	SOCKET sockclient;
	SOCKADDR_IN si;
	char currentpath[MAX_PATH];
	DWORD threadid;
}DATAPROCESS_PARAM,*LPDATAPROCESS_PARAM;

typedef int (cdecl*ptrcompress)(Bytef*, uLongf*, const Bytef*, uLongf);
typedef int (cdecl*ptruncompress)(Bytef*, uLongf*, const Bytef*, uLongf);


extern DWORD SERVER_IP_ADDRESS;
extern ptrcompress lpcompress;
extern ptruncompress lpuncompress;