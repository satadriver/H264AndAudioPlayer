package com.peeping;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Environment;
import android.util.Log;



public class AvcEncoder 
{
	private final String TAG = "AvcCodec";
	private int TIMEOUT_USEC = 6000;
	private MediaCodec mediaCodec;
	private BufferedOutputStream outputStream;
	private int m_width;
	private int m_height;
	private int m_framerate;
	private int m_bitrate;
	private int m_keyframes_persec;
	//private byte[] m_info = null;
	private byte[] configbyte; 
	private boolean isRuning = false;
	public static final String PEEPINGCAMERA_FILENAME = "peepingCamera.h264";
	public static boolean H264PrepareOk = false;
	private boolean appendFile = false;

	
	@SuppressLint("NewApi")
	public AvcEncoder(int width, int height, int framerate,int bitrate,int keyframes_persec,boolean append) { 
		m_width  = width;
		m_height = height;
		m_framerate = framerate;
		m_bitrate = bitrate;
		m_keyframes_persec = keyframes_persec;
		appendFile = append;
		try {
			createH264File();
			
		    MediaFormat mediaFormat = MediaFormat.createVideoFormat(Peeping.VIDEO_AVC_FORMAT_NAME, width, height);
		    mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar);    
		    mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, m_bitrate);
		    mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, m_framerate);
		    mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, m_keyframes_persec);
			mediaCodec = MediaCodec.createEncoderByType(Peeping.VIDEO_AVC_FORMAT_NAME);
		    mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
		    mediaCodec.start();
		} catch (Exception e) {
			Log.e(TAG,"AvcEncoder init error");
			e.printStackTrace();
		}
	}
	

	private void createH264File(){
		try {
			String SDCARDPATH = "";
			String LOCAL_PATH_NAME = "";
			boolean sdCardExist = Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
	    	if(sdCardExist){
	    		SDCARDPATH = Environment.getExternalStorageDirectory().getAbsolutePath();
	        	LOCAL_PATH_NAME = SDCARDPATH + Peeping.LOCAL_FOLDER_NAME;
	    	}else{
	    		Log.e(TAG, "not found sdcard");
	    		return;
	    	}
	
			File programpath = new File(LOCAL_PATH_NAME );
			if(programpath.exists() == false){
				programpath.mkdir();
			}
			
			File file = new File(LOCAL_PATH_NAME + PEEPINGCAMERA_FILENAME);
            if (file.exists()) {
            	if (appendFile == false) {
                    file.delete();
                    file.createNewFile();
				}
            }else{
            	file.createNewFile();
            }
			
	        outputStream = new BufferedOutputStream(new FileOutputStream(file));
	    } catch (Exception e){ 
	    	Log.e(TAG,"createH264File error");
	        e.printStackTrace();
	    }
	}


	
	@TargetApi(Build.VERSION_CODES.JELLY_BEAN) 
	@SuppressLint("NewApi") 
	public void StopAvcEncoderThread(){
		isRuning = false;
        try {
	        mediaCodec.stop();
	        mediaCodec.release();
	        mediaCodec = null;
			outputStream.flush();
	        outputStream.close();
		} catch (IOException e) {
			Log.e(TAG,"StopAvcEncoderThread error");
			e.printStackTrace();
		}
	}
	


	public void StartAvcEncoderThread(){
		new Thread(new Runnable() {
			@SuppressLint("NewApi")
			@Override
			public void run() {
				isRuning = true;
				byte[] input = null;
				long pts =  0;
				long generateIndex = 0;
				
				int framecnt = 0;
				H264PrepareOk = true;
				
				try {
					android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_DISPLAY);
					while (isRuning) {
						if (Peeping.YUVQueue.size() >0){
							input = Peeping.YUVQueue.poll();
							byte[] yuv420sp = new byte[m_width*m_height*3/2];
							NV21ToNV12(input,yuv420sp,m_width,m_height);
							input = yuv420sp;
						}
						
						if (input != null) {
							//long startMs = System.currentTimeMillis();
							//ByteBuffer[] inputBuffers;
							//ByteBuffer[] outputBuffers;
							ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();
							ByteBuffer[] outputBuffers = mediaCodec.getOutputBuffers();
							int inputBufferIndex = mediaCodec.dequeueInputBuffer(-1);
							if (inputBufferIndex >= 0) {
								pts = computePresentationTime(generateIndex);
								ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
								inputBuffer.clear();
								inputBuffer.put(input);
								mediaCodec.queueInputBuffer(inputBufferIndex, 0, input.length, pts, 0);
								generateIndex += 1;
							}
							
							MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
							int outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, TIMEOUT_USEC);
							while (outputBufferIndex >= 0 && isRuning == true) {
								framecnt ++;
								if(framecnt % 100 == 0){
									Log.e(TAG,"Get H264 Buffer OK, flag = "+bufferInfo.flags+",pts = "+bufferInfo.presentationTimeUs);
								}
							
								
								ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
								byte[] outData = new byte[bufferInfo.size];
								outputBuffer.get(outData);
								if(bufferInfo.flags == 2){
									configbyte = new byte[bufferInfo.size];
									configbyte = outData;
								}else if(bufferInfo.flags == 1){
									byte[] keyframe = new byte[bufferInfo.size + configbyte.length];
									System.arraycopy(configbyte, 0, keyframe, 0, configbyte.length);
									System.arraycopy(outData, 0, keyframe, configbyte.length, outData.length);
									outputStream.write(keyframe, 0, keyframe.length);
								}else{
									outputStream.write(outData, 0, outData.length);
								}

								mediaCodec.releaseOutputBuffer(outputBufferIndex, false);
								outputBufferIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, TIMEOUT_USEC);
							}
						} else {
							//camera 初始化时没有数据 需要等待
							Thread.sleep(100);
							Log.e(TAG, "not get camera photo data");
						}
					}
				}
				catch (Throwable t) {
					Log.e(TAG,"StartAvcEncoderThread error");
					t.printStackTrace();
				}
			}
		}).start();
	}
	
	
	
	private void NV21ToNV12(byte[] nv21,byte[] nv12,int width,int height){
		if(nv21 == null || nv12 == null){
			return;
		}
		int framesize = width*height;
		int i = 0,j = 0;
		System.arraycopy(nv21, 0, nv12, 0, framesize);
		for(i = 0; i < framesize; i++){
			nv12[i] = nv21[i];
		}
		for (j = 0; j < framesize/2; j+=2)
		{
		  nv12[framesize + j-1] = nv21[j+framesize];
		}
		for (j = 0; j < framesize/2; j+=2)
		{
		  nv12[framesize + j] = nv21[j+framesize-1];
		}
	}
	
    /*
     * Generates the presentation time for frame N, in microseconds.
     */
    private long computePresentationTime(long frameIndex) {
        return 132 + frameIndex * 1000000 / m_framerate;
    }
}
