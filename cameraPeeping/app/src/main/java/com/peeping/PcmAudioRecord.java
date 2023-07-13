package com.peeping;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder.AudioSource;
import android.os.Environment;
import android.util.Log;


public class PcmAudioRecord {
	private final String TAG = "PcmAudioRecord";
    private AudioRecord mRecorder;
    private DataOutputStream dos;
    private Thread recordThread;
    private boolean isStart = false;
    private int bufferSize;
    public static final String AUDIORECORD_FILENAME = "peepingAudio.pcm";
    public static final int PCM_SAMPLE_FREQUENCY = 8000;
    public static int PCM_BITS_PER_CHANNEL = 16;
    public static int PCM_CHANNEL_COUNT = 1;
    public static boolean PcmPrepareOk = false;
    private boolean appendFile = false;

    public PcmAudioRecord(boolean append) {
    	if(PCM_BITS_PER_CHANNEL == 8){
	        bufferSize = AudioRecord.getMinBufferSize(PCM_SAMPLE_FREQUENCY, 
	        		AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_8BIT);
	        bufferSize = PCM_SAMPLE_FREQUENCY * PCM_CHANNEL_COUNT * PCM_BITS_PER_CHANNEL/8;
	        mRecorder = new AudioRecord(AudioSource.MIC, PCM_SAMPLE_FREQUENCY, 
	        		AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_8BIT, bufferSize * 2);
		}else if (PCM_BITS_PER_CHANNEL == 16) {
	        bufferSize = AudioRecord.getMinBufferSize(PCM_SAMPLE_FREQUENCY, 
	        		AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
	        bufferSize = PCM_SAMPLE_FREQUENCY * PCM_CHANNEL_COUNT * PCM_BITS_PER_CHANNEL/8;
	        mRecorder = new AudioRecord(AudioSource.MIC, PCM_SAMPLE_FREQUENCY, 
	        		AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, bufferSize * 2);
		}else{
			
		}

        appendFile = append;
    }



    Runnable recordRunnable = new Runnable() {
        @Override
        public void run() {
            try {
            	
                android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
                int bytesRecord;
                byte[] tempBuffer = new byte[bufferSize];
                if (mRecorder.getState() != AudioRecord.STATE_INITIALIZED) {
                    stopRecord();
                    return;
                }
                mRecorder.startRecording();
                long timecur = System.currentTimeMillis();
                int framecnt = 0;
                
                PcmPrepareOk = true;
                
                while (isStart) {
                    if (null != mRecorder) {
                        bytesRecord = mRecorder.read(tempBuffer, 0, bufferSize);
                        if (bytesRecord == AudioRecord.ERROR_INVALID_OPERATION || bytesRecord == AudioRecord.ERROR_BAD_VALUE) {
                        	Log.e(TAG,"ERROR_INVALID_OPERATION or ERROR_BAD_VALUE");
                            continue;
                        }
                        if (bytesRecord != 0 && bytesRecord != -1) {

                            dos.write(tempBuffer, 0, bytesRecord);
                            
                            long timenow = System.currentTimeMillis();
                            long timecost = timenow - timecur;
                            
                            framecnt ++;
                            if (framecnt % 100 == 0) {
                            	Log.e(TAG, "pcm sample time cost:" + String.valueOf(timecost) );
							}
                            
                            timecur = timenow;
                        } else {
                        	Log.e(TAG, "pcm sample error");
                            break;
                        }
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "run() exception");
                e.printStackTrace();
            }
        }
    };
    
    






    public void startRecord() {
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

			File programpath = new File(LOCAL_PATH_NAME);
			if(programpath.exists() == false){
				programpath.mkdir();
			}
			
			String filename = LOCAL_PATH_NAME + AUDIORECORD_FILENAME;
            File file = new File(filename);
            if (file.exists()) {
            	if (appendFile == false) {
                    file.delete();
                    file.createNewFile();
				}
            }else{
            	file.createNewFile();
            }
            
            dos = new DataOutputStream(new FileOutputStream(file, true));
            
            byte audioinfo[] = new byte[16];
            int offset = 0;
			byte[] bfrequency = PublicFunction.intToBytes(PCM_SAMPLE_FREQUENCY);
			for (int i = 0; i < 4; i++) {
				audioinfo[offset + i] = bfrequency[i];
			}
			offset += 4;
			
			byte[] bbitschannal = PublicFunction.intToBytes(PCM_BITS_PER_CHANNEL);
			for (int i = 0; i < 4; i++) {
				audioinfo[offset + i] = bbitschannal[i];
			}
			offset += 4;
			
			byte[] bchannelcnt = PublicFunction.intToBytes(PCM_CHANNEL_COUNT);
			for (int i = 0; i < 4; i++) {
				audioinfo[offset + i] = bchannelcnt[i];
			}
			offset += 4;

            dos.write(audioinfo,0,16);
            
            startThread();
        } catch (Exception e) {
        	Log.e(TAG, "startRecord exception");
            e.printStackTrace();
        }
    }


    public void stopRecord() {
        try {
	        destroyThread();
	        if (mRecorder != null) {
	            if (mRecorder.getState() == AudioRecord.STATE_INITIALIZED) {
	                mRecorder.stop();
	            }
	            if (mRecorder != null) {
	                mRecorder.release();
	                mRecorder = null;
	            }
	        }
	        if (dos != null) {
	            dos.flush();
	            dos.close();
	            dos = null;
	        }
        } catch (Exception e) {
        	Log.e(TAG, "stopRecord exception");
            e.printStackTrace();
        }
    }
    
    
    private void destroyThread() {
        try {
            isStart = false;
            if (null != recordThread && Thread.State.RUNNABLE == recordThread.getState()) {
                try {
                    Thread.sleep(500);
                    recordThread.interrupt();
                } catch (Exception e) {
                	Log.e(TAG, "destroyThread exception");
                    recordThread = null;
                }
            }
            recordThread = null;
        } catch (Exception e) {
        	Log.e(TAG, "destroyThread exception");
            e.printStackTrace();
        } finally {
            recordThread = null;
        }
    }


    private void startThread() {
        destroyThread();
        isStart = true;
        if (recordThread == null) {
            recordThread = new Thread(recordRunnable);
            recordThread.start();
        }
    }

}
