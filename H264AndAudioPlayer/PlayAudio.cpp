
#pragma once
#include <windows.h>
#include "zlib.h"
#include "zconf.h"
#include "PlayWindow.h"
#include "H264AndAudioPlayer.h"
#include "PublicFunction.h"
#pragma comment (lib,"winmm.lib")


typedef struct {
	int pcm_sample_frequency;
	int pcm_bits_perchannel;
	int pcm_channel_count;
	int undefined;
}PCM_COMPRESSED_HEADER, * LPPCM_COMPRESSED_HEADER;



#define PCM_PLAY_INTERVAL  100


HWAVEOUT hWavOut = NULL;
char* lpaudiofiledata = NULL;
char* PlayBuf_0 = NULL;
char* PlayBuf_1 = NULL;
WAVEHDR	WavHdr_0 = { 0 };
WAVEHDR	WavHdr_1 = { 0 };








int ReleaseAudio() {
	waveOutUnprepareHeader(hWavOut, &WavHdr_0, sizeof(WAVEHDR));
	waveOutUnprepareHeader(hWavOut, &WavHdr_1, sizeof(WAVEHDR));
	waveOutClose(hWavOut);
	delete[]PlayBuf_0;
	delete[]PlayBuf_1;
	delete[]lpaudiofiledata;
	return TRUE;
}





int __stdcall PlayAudioProcess(DWORD dwParams[])
{
	char* szaudiofilename = (char*)dwParams[0];
	DWORD* dwPcmPrepareOK = (DWORD*)dwParams[1];
	DWORD* dwPcmCompleteFlag = (DWORD*)dwParams[2];
	DWORD dwPlayOffset = dwParams[3];

	try
	{
		HANDLE hfaudio = CreateFileA(szaudiofilename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hfaudio == INVALID_HANDLE_VALUE)
		{
			MessageBoxA(0, "PlayAudioProcess CreateFileA error", "PlayAudioProcess CreateFileA error", MB_OK);
			return FALSE;
		}

		int audiofilesize = GetFileSize(hfaudio, 0);
		lpaudiofiledata = new char[audiofilesize];
		if (lpaudiofiledata == FALSE)
		{
			CloseHandle(hfaudio);
			return FALSE;
		}

		DWORD dwcnt = 0;
		int ret = ReadFile(hfaudio, lpaudiofiledata, audiofilesize, &dwcnt, 0);
		CloseHandle(hfaudio);
		if (dwcnt != audiofilesize)
		{
			delete[]lpaudiofiledata;
			return FALSE;
		}

		/*
		int uncompresssize = *(int*)lpaudiofiledata;
		int uncomressbufsize = uncompresssize + 0x1000;
		lpuncompress = new char[uncomressbufsize];
		ret = uncompress((unsigned char*)lpuncompress,(unsigned long*)&uncomressbufsize,(unsigned char*)lpaudiofiledata + 4,audiofilesize - 4);
		delete []lpaudiofiledata;
		if (ret )
		{
			WriteLog("PlayAudioProcess uncompress error\r\n");
			delete [] lpuncompress;
			return FALSE;
		}

		wsprintfA(szinfo,"uncompress size:%u,source pcm size:%u\r\n",uncomressbufsize,audiofilesize - 4);
		WriteLog(szinfo);
		*/

		char szinfo[1024];

		LPPCM_COMPRESSED_HEADER lphdr = (LPPCM_COMPRESSED_HEADER)lpaudiofiledata;
		PCM_BITS_PERCHANNAL = lphdr->pcm_bits_perchannel;
		PCM_CHANNEL_COUNT = lphdr->pcm_channel_count;
		PCM_SAMPLE_FREQUENCY = lphdr->pcm_sample_frequency;
		PLAYRATE = (PCM_SAMPLE_FREQUENCY * (PCM_BITS_PERCHANNAL / 8) * PCM_PLAY_INTERVAL) / 1000;
		lpaudiodata = lpaudiofiledata + sizeof(PCM_COMPRESSED_HEADER);
		AudioDataPtr = lpaudiodata + dwPlayOffset;
		audiodatasize = audiofilesize - sizeof(PCM_COMPRESSED_HEADER);
		PlayBuf_0 = new char[PLAYRATE];
		PlayBuf_1 = new char[PLAYRATE];


		WAVEFORMATEX				WavFmtEx = { 0 };
		WavFmtEx.wFormatTag = WAVE_FORMAT_PCM;
		WavFmtEx.nChannels = PCM_CHANNEL_COUNT;
		WavFmtEx.wBitsPerSample = PCM_BITS_PERCHANNAL;
		WavFmtEx.nAvgBytesPerSec = PCM_SAMPLE_FREQUENCY * PCM_BITS_PERCHANNAL / 8;
		WavFmtEx.nBlockAlign = WavFmtEx.wBitsPerSample / 8;
		WavFmtEx.nSamplesPerSec = PCM_SAMPLE_FREQUENCY;
		WavFmtEx.cbSize = 0;

		hWavOut = NULL;
		ret = waveOutOpen(&hWavOut, WAVE_MAPPER, &WavFmtEx, (unsigned long)hwndPlay, 0, CALLBACK_WINDOW);
		if (ret != MMSYSERR_NOERROR)
		{
			delete[]lpaudiofiledata;
			delete[]PlayBuf_0;
			delete[]PlayBuf_1;
			MessageBoxA(0, "wavOutOpen error!", "wavOutOpen error!", MB_OK);
			return FALSE;
		}


		WavHdr_0.dwBufferLength = PLAYRATE;
		WavHdr_0.dwBytesRecorded = 0;
		WavHdr_0.dwFlags = 0;
		WavHdr_0.dwLoops = 0;
		WavHdr_0.dwUser = 0;
		WavHdr_0.lpData = PlayBuf_0;
		WavHdr_0.lpNext = 0;
		WavHdr_0.reserved = 0;
		ret = waveOutPrepareHeader(hWavOut, &WavHdr_0, sizeof(WAVEHDR));
		if (ret != MMSYSERR_NOERROR)
		{
			delete[]lpaudiofiledata;
			delete[]PlayBuf_0;
			delete[]PlayBuf_1;
			waveOutClose(hWavOut);
			MessageBoxA(0, "waveOutPrepareHeader error!", "waveOutPrepareHeader error!", MB_OK);
			return FALSE;
		}

		WavHdr_1.dwBufferLength = PLAYRATE;
		WavHdr_1.dwBytesRecorded = 0;
		WavHdr_1.dwFlags = 0;
		WavHdr_1.dwLoops = 0;
		WavHdr_1.dwUser = 0;
		WavHdr_1.lpData = PlayBuf_1;
		WavHdr_1.lpNext = 0;
		WavHdr_1.reserved = 0;
		ret = waveOutPrepareHeader(hWavOut, &WavHdr_1, sizeof(WAVEHDR));
		if (ret != MMSYSERR_NOERROR)
		{
			delete[]lpaudiofiledata;
			waveOutUnprepareHeader(hWavOut, &WavHdr_0, sizeof(WAVEHDR));
			waveOutClose(hWavOut);
			delete[]PlayBuf_0;
			delete[]PlayBuf_1;
			MessageBoxA(0, "waveOutPrepareHeader error!", "waveOutPrepareHeader error!", MB_OK);
			return FALSE;
		}


		ret = waveOutSetVolume(hWavOut, 0xffffffff);
		if (ret != MMSYSERR_NOERROR)
		{
			ReleaseAudio();
			MessageBoxA(0, "waveOutSetVolume error!", "waveOutSetVolume error!", MB_OK);
		}

		*dwPcmPrepareOK = TRUE;
		int pcmplaysecs = 0;
		while (TRUE) {

			while (iStartStopFlag == FALSE && hwndPlay && iPlayQuitCode == FALSE)
			{
				Sleep(WAIT_START_STOP_INTERVAL);
				continue;
			}

			if (hwndPlay == FALSE || iPlayQuitCode == TRUE)
			{
				ReleaseAudio();
				break;
			}

			if (AudioDataPtr + PLAYRATE > lpaudiodata + audiodatasize)
			{
				wsprintfA(szinfo, "pcm play seconds:%u\r\n", (pcmplaysecs * PCM_PLAY_INTERVAL) / 1000);
				WriteLog(szinfo);
				ReleaseAudio();
				break;
			}

			memmove(PlayBuf_0, AudioDataPtr, PLAYRATE);
			AudioDataPtr += PLAYRATE;
			int ret = waveOutWrite(hWavOut, &WavHdr_0, sizeof(WAVEHDR));
			if (ret)
			{
				ret = GetLastError();
				ReleaseAudio();
				WriteLog("PlayAudioProcess waveOutWrite error\r\n");
				break;
			}
			Sleep(PCM_PLAY_INTERVAL);
			pcmplaysecs++;

			while (iStartStopFlag == FALSE && hwndPlay && iPlayQuitCode == FALSE)
			{
				Sleep(WAIT_START_STOP_INTERVAL);
				continue;
			}

			if (hwndPlay == FALSE || iPlayQuitCode == TRUE)
			{
				ReleaseAudio();
				break;
			}

			if (AudioDataPtr + PLAYRATE > lpaudiodata + audiodatasize)
			{
				wsprintfA(szinfo, "pcm play seconds:%u\r\n", (pcmplaysecs * PCM_PLAY_INTERVAL) / 1000);
				WriteLog(szinfo);
				ReleaseAudio();
				break;
			}

			memmove(PlayBuf_1, AudioDataPtr, PLAYRATE);
			AudioDataPtr += PLAYRATE;
			ret = waveOutWrite(hWavOut, &WavHdr_1, sizeof(WAVEHDR));
			if (ret)
			{
				ret = GetLastError();
				ReleaseAudio();
				WriteLog("PlayAudioProcess waveOutWrite error\r\n");
				break;
			}

			Sleep(PCM_PLAY_INTERVAL);
			pcmplaysecs++;
		}

		*dwPcmCompleteFlag = TRUE;
		return TRUE;
	}
	catch (...)
	{
		WriteLog("audio play exception\r\n");
		MessageBoxA(0, "audio play exception", "audio play exception", MB_OK);
		return FALSE;
	}
}