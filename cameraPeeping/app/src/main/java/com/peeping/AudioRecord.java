package com.peeping;

import java.io.File;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.media.MediaRecorder;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

public class AudioRecord {
	private MediaRecorder mediarecorder;
	private final String TAG = "AudioCodec";
	public static final String AUDIORECORD_FILENAME = "peepingAudio.aac";
	
	public void stopAudioRecord(){
		if(mediarecorder!=null){
			mediarecorder.stop();
			mediarecorder.release();
			mediarecorder=null;
		}
	}
	
	public void startAudioRecord(){
		new Thread(new Runnable (){
			@TargetApi(Build.VERSION_CODES.JELLY_BEAN) @SuppressLint("InlinedApi") 
			@Override
			public void run() {
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

				File path = new File(LOCAL_PATH_NAME);
				if(path.exists() == false){
					try{
						path.mkdir();
					}catch(Exception ex){
			    		Log.e(TAG, "startAudioRecord createNewFile() except");
			    		return;
					}
				}
				
				String filename = LOCAL_PATH_NAME + AUDIORECORD_FILENAME;
				File file=new File(filename);
				if(file.exists()){
					file.delete();
				}
				
				if (mediarecorder==null) {
					mediarecorder=new MediaRecorder();
				}
				mediarecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
				mediarecorder.setOutputFormat(MediaRecorder.OutputFormat.AAC_ADTS);
				mediarecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);
				mediarecorder.setOutputFile(file.getAbsolutePath());

				try {
					mediarecorder.prepare();
				} catch (Exception e) {
					e.printStackTrace();
				}
				mediarecorder.start();
				Log.e(TAG, "recoding audio start");
			}
		}).start();
	}
}
