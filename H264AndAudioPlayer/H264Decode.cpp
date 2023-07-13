
#pragma once


#include "H264Decode.h"

#include "H264AndAudioPlayer.h"
#include "PublicFunction.h"
#include "PlayWindow.h"
#include <map>
//#include <windows.h>



//#include "ffmpeg\avformat.h"

#pragma comment(lib,"lib\\avcodec.lib")
#pragma comment(lib,"lib\\avformat.lib")
#pragma comment(lib,"lib\\avutil.lib")






#if _MSC_VER >= 1600
#define _MSC_STDINT_H_
#endif

AVFormatContext* m_pFormatCtx;
AVCodecContext* m_pCodecCtx;
AVCodec* m_pCodec;
AVFrame* m_pFrameRGB;
AVFrame* m_pFrame;
uint8_t* m_buffer;
int             m_videoStream;


void ReleaseH264()
{
	if (m_buffer != NULL)
		free(m_buffer);
	if (m_pFrameRGB != NULL)
		av_free(m_pFrameRGB);
	// Free the YUV frame
	if (m_pFrame != NULL)
		av_free(m_pFrame);
	// Close the codec
	if (m_pCodecCtx != NULL)
		avcodec_close(m_pCodecCtx);
	// Close the video file
	if (m_pFormatCtx != NULL)
		av_close_input_file(m_pFormatCtx);
}





bool GetNextFrame()
{

	static AVPacket  packet;
	static bool      fFirstTime = true;
	static int       bytesRemaining = 0;
	static uint8_t* RawData;
	int              bytesdecoded;
	int              frameFinished;

	//第一次执行时，将packet.data  置空表明他没必要被释放
	if (fFirstTime)
	{
		fFirstTime = false;
		packet.data = NULL;
	}

	//解码数据包直到解满一桢
	while (true)
	{
		//不断得解码当前数据包直到解完全部数据
		while (bytesRemaining > 0)
		{
			//解码下一段截获的数据
			bytesdecoded = avcodec_decode_video(m_pCodecCtx, m_pFrame, &frameFinished, RawData, bytesRemaining);
			//判断是否出错
			if (bytesdecoded < 0)
			{
				fprintf(stderr, "Error while decoding frame\n");
				return false;
			}

			bytesRemaining -= bytesdecoded;
			RawData += bytesdecoded;

			//判断是否解完当前帧
			if (frameFinished) {
				return true;
			}
		}

		//读取下一个数据包，跳过非当前码流的数据包
		do
		{
			//释放前一个数据包
			if (packet.data != NULL) {
				av_free_packet(&packet);
			}

			//读取新的包
			if (av_read_frame(m_pFormatCtx, &packet) < 0) {
				goto loop_exit;
			}

		} while (packet.stream_index != m_videoStream);

		bytesRemaining = packet.size;
		RawData = packet.data;

	}

loop_exit:
	//解码最后一桢剩下的包
	bytesdecoded = avcodec_decode_video(m_pCodecCtx, m_pFrame, &frameFinished, RawData, bytesRemaining);

	//释放最后一个数据包
	if (packet.data != NULL) {
		av_free_packet(&packet);
	}

	return frameFinished != 0;
}



bool InitDecode(char* infile)
{
	int             i;
	int             i_frame = 0;
	int             numBytes;

	// Register all formats and codecs
	av_register_all();
	// Open video file
	if (av_open_input_file(&m_pFormatCtx, infile, NULL, 0, NULL) != 0)
	{
		// Couldn't open file
		return false;
	}

	// Retrieve stream information
	if (av_find_stream_info(m_pFormatCtx) < 0)
	{
		// Couldn't find stream information
		return false;
	}
	// Dump information about file onto standard error
	dump_format(m_pFormatCtx, 0, infile, false);

	// Find the first video stream
	m_videoStream = -1;
	for (i = 0; i < m_pFormatCtx->nb_streams; i++) {
		if (m_pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			m_videoStream = i;
			break;
		}
	}
	if (m_videoStream == -1)
	{
		// Didn't find a video stream
		return false;
	}
	// Get a pointer to the codec context for the video stream
	m_pCodecCtx = m_pFormatCtx->streams[m_videoStream]->codec;
	// Find the decoder for the video stream
	m_pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	if (m_pCodec == NULL)
	{
		// Codec not found
		return false;
	}
	// Inform the codec that we can handle truncated bitstreams -- i.e.,
	// bitstreams where frame boundaries can fall in the middle of packets
	if (m_pCodec->capabilities & CODEC_CAP_TRUNCATED) {
		m_pCodecCtx->flags |= CODEC_FLAG_TRUNCATED;
	}
	// Open codec
	if (avcodec_open(m_pCodecCtx, m_pCodec) < 0)
	{
		// Could not open codec
		return false;
	}
	// Allocate video frame
	m_pFrame = avcodec_alloc_frame();
	// Allocate an AVFrame structure
	m_pFrameRGB = avcodec_alloc_frame();
	if (m_pFrameRGB == NULL) {
		return false;
	}
	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(PIX_FMT_RGB24, m_pCodecCtx->width, m_pCodecCtx->height);
	m_buffer = new uint8_t[numBytes];
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	avpicture_fill((AVPicture*)m_pFrameRGB, m_buffer, PIX_FMT_RGB24, m_pCodecCtx->width, m_pCodecCtx->height);
	return true;
}









int __stdcall H264Decode(DWORD dwParams[]) {
	try {
		char* h264filename = (char*)dwParams[0];
		unsigned char** lpPlayBmpDataPtr = (unsigned char**)dwParams[1];
		int* iH264Width = (int*)dwParams[2];
		int* iH264Height = (int*)dwParams[3];
		int h264framerate = dwParams[4];
		DWORD* dwH264PrepareOK = (DWORD*)dwParams[5];
		DWORD* dwH264CompleteFlag = (DWORD*)dwParams[6];

		bool ret = InitDecode(h264filename);
		if (ret == false)
		{
			MessageBoxA(0, "h264 decode init error", "h264 decode init error", MB_OK);
			return FALSE;
		}

		int frameinterval = 1000 / h264framerate;

		int h264frametotal = 0;

		*dwH264PrepareOK = TRUE;
		while (TRUE) {

			while (iStartStopFlag == FALSE && hwndPlay && iPlayQuitCode == FALSE)
			{
				Sleep(WAIT_START_STOP_INTERVAL);
				continue;
			}

			if (hwndPlay == FALSE || iPlayQuitCode == TRUE)
			{
				break;
			}

			ret = GetNextFrame();
			if (ret == false)
			{
				WriteLog("H264Decode GetNextFrame error\r\n");
				break;
			}
			int ires = img_convert((AVPicture*)m_pFrameRGB, PIX_FMT_BGR24, (AVPicture*)m_pFrame, m_pCodecCtx->pix_fmt, m_pCodecCtx->width, m_pCodecCtx->height);
			if (ires)
			{
				WriteLog("H264Decode img_convert error\r\n");
				break;
			}

			*lpPlayBmpDataPtr = m_pFrameRGB->data[0];
			*iH264Width = m_pCodecCtx->width;
			*iH264Height = m_pCodecCtx->height;

			RECT rect = { 0 };
			int iReturn = GetClientRect(hwndPlay, &rect);
			InvalidateRect(hwndPlay, &rect, FALSE);

			h264frametotal++;
			Sleep(frameinterval);
		}

		*lpPlayBmpDataPtr = NULL;
		*iH264Width = NULL;
		*iH264Height = NULL;
		*dwH264CompleteFlag = TRUE;
		ReleaseH264();

		char szinfo[1024];
		wsprintfA(szinfo, "h264 play total frames count:%u\r\n", h264frametotal);
		WriteLog(szinfo);
		return TRUE;
	}
	catch (...) {
		WriteLog("h264 decode exception\r\n");
		MessageBoxA(0, "h264 decode exception", "h264 decode exception", MB_OK);
		return FALSE;
	}
}






