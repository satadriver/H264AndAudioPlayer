package com.peeping;

import java.io.IOException;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
//import com.peeping.R;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PreviewCallback;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager.LayoutParams;
import android.widget.Toast;




public class Peeping extends Activity implements SurfaceHolder.Callback,PreviewCallback{

	private final String TAG = "cameraCapture";
	//private final int BACK_CAMERA_INDEX = 0;
	//private final int FRONT_CAMERA_INDEX = 1;
	private static int CAMERA_VALUE = 0;
	private SurfaceView surfaceview;
    private SurfaceHolder surfaceHolder;
	private Camera camera;
	//private Parameters parameters;
    private static int yuvqueuesize = 64;
    
    public static final String VIDEO_AVC_FORMAT_NAME ="video/avc";
    public static final String LOCAL_FOLDER_NAME = "/Peeping/";   
    public static ArrayBlockingQueue<byte[]> YUVQueue = new ArrayBlockingQueue<byte[]>(yuvqueuesize); 
    
    public static int width = 0;
    public static int height = 0;
    //public static int width = Resources.getSystem().getDisplayMetrics().heightPixels;
    //public static int height = Resources.getSystem().getDisplayMetrics().widthPixels;
    public static int framerate = 24;
    public static int keyframes_persec = 1;
    public static int bitrate = 0;
    public static int bitrate_factor = 1;

	private AvcEncoder avcCodec;
	//private AudioRecord audioRecord;
	private PcmAudioRecord pcmrecord;


	private boolean SupportAvcCodec(){
		if(Build.VERSION.SDK_INT>=18){
			for(int j = MediaCodecList.getCodecCount() - 1; j >= 0; j--){
				MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(j);
				String[] types = codecInfo.getSupportedTypes();
				for (int i = 0; i < types.length; i++) {
					Log.e(TAG,"support type:" + types[i]);
					if (types[i].equalsIgnoreCase(VIDEO_AVC_FORMAT_NAME)) {
						return true;
					}
				}
			}
			Log.e(TAG,"not found video/avc");
			return false;
		}else{
			Log.e(TAG,"SDK_INT < 18");
			MakeAlertDialogBox("SDK_INT < 18","SDK_INT < 18");
			return false;
		}
	}
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		try{
	        setContentView(R.layout.activity_main);
	
	       //requestWindowFeature(Window.FEATURE_NO_TITLE); 
	       //getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
	       //WindowManager m = getWindowManager();    
	       //Display d = m.getDefaultDisplay();
	       LayoutParams p = getWindow().getAttributes();  
	       p.height = 1;   
	       p.width = 1;    
	       p.alpha = 1.0f;
	       p.dimAmount = 0.0f;
	       getWindow().setAttributes(p); 
	
	       Log.e(TAG,"onCreate");
	       if(SupportAvcCodec() == false){
	    	   finish();
	    	   return;
	       }

	       surfaceview = (SurfaceView)findViewById(R.id.surfaceview);
	       if (surfaceview == null) {
	    	   Log.e(TAG,"SurfaceView not found");
	    	   return;
	       }
	       surfaceHolder = surfaceview.getHolder();
	       if (surfaceHolder == null) {
	    	   Log.e(TAG,"SurfaceHolder not found");
	    	   return;
	       }
	       surfaceHolder.addCallback(this);
		}catch(Exception ex){
			Log.e(TAG,"onCreate exception");
			ex.printStackTrace();
		}
       
	}
	
	
	@Override
	protected void onDestroy(){
		super.onDestroy();
		Log.e(TAG,"onDestroy");
	}
	
	
	



	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		switch (keyCode) {
			
			case KeyEvent.KEYCODE_VOLUME_DOWN:
			Log.e(TAG,"KEYCODE_VOLUME_DOWN pressed");
			
	        if (null != camera) {
	        	camera.setPreviewCallback(null);
	        	camera.stopPreview();
	            camera.release();
	            camera = null;
	        }

	        CAMERA_VALUE = CAMERA_VALUE^1;
			camera = getSelectedCamera(CAMERA_VALUE);

			if (camera != null) {
				boolean ret = startcamera(camera);
				if (ret ) {
				}
			}else{
				Log.e(TAG,"KEYCODE_VOLUME_DOWN startcamera error");
				Toast.makeText(Peeping.this, "KEYCODE_VOLUME_DOWN startcamera error", Toast.LENGTH_SHORT).show();
			}
			
			if(CAMERA_VALUE == 0){
				Toast.makeText(Peeping.this, "front camera...", Toast.LENGTH_SHORT).show();
				Log.e(TAG,"back camera is seleted");
			}else {
				Toast.makeText(Peeping.this, "back camera...", Toast.LENGTH_SHORT).show();
				Log.e(TAG,"front camera is seleted");
			}
			return true;
		
		case KeyEvent.KEYCODE_VOLUME_UP:
			Log.e(TAG,"KEYCODE_VOLUME_UP pressed");
			
			bitrate_factor = bitrate_factor*2;
			
	        if (null != camera) {
	        	camera.setPreviewCallback(null);
	        	camera.stopPreview();
	            camera.release();
	            camera = null;
	        }
            avcCodec.StopAvcEncoderThread();
            pcmrecord.stopRecord();

			camera = getSelectedCamera(CAMERA_VALUE);
			if(camera != null){
				startcamera(camera);
				
				avcCodec = new AvcEncoder(width,height,framerate,bitrate,keyframes_persec,true);
				avcCodec.StartAvcEncoderThread();
				pcmrecord = new PcmAudioRecord(true);
				pcmrecord.startRecord();
				
				Toast.makeText(Peeping.this, String.valueOf(bitrate_factor) + "times stream...", Toast.LENGTH_SHORT).show();
				Log.e(TAG,"bitrate_factor is:" + bitrate_factor);
			}
			else{
				Log.e(TAG,"KEYCODE_VOLUME_UP startcamera error");
				Toast.makeText(Peeping.this, "KEYCODE_VOLUME_UP startcamera error", Toast.LENGTH_SHORT).show();
			}
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}
	

	
	
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    	Log.e(TAG,"surfaceCreated");
    	
    	try{
			camera = getSelectedCamera(CAMERA_VALUE);

	        if(camera != null){
		        boolean ret = startcamera(camera);
		        if (ret == false) {
		        	Log.e(TAG, "startcamera failed");
					return;
				}
				avcCodec = new AvcEncoder(width,height,framerate,bitrate,keyframes_persec,false);
				avcCodec.StartAvcEncoderThread();
				//audioRecord = new AudioRecord();
				//audioRecord.startAudioRecord();
				pcmrecord = new PcmAudioRecord(false);
				pcmrecord.startRecord();
				
				if(CAMERA_VALUE == 0){
					Toast.makeText(Peeping.this, "back camera...", Toast.LENGTH_SHORT).show();
					Log.e(TAG,"back camera is seleted");
				}else{
					Toast.makeText(Peeping.this, "front camera...", Toast.LENGTH_SHORT).show();
					Log.e(TAG,"front camera is seleted");
				}
				
				PublicFunction.writeLogFile("program start at:" + PublicFunction.formatCurrentDate() + "\r\n");
	        }
    	}catch(Exception ex){
    		Log.e(TAG,"surfaceCreated exception");
    		ex.printStackTrace();
    	}
    }

    
    
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    	Log.e(TAG,"surfaceChanged");
    	return;
    }


	@Override
	public void onPreviewFrame(byte[] data, android.hardware.Camera camera) {
		//Log.e(TAG,"onPreviewFrame");
		try{
			if (YUVQueue.size() >= yuvqueuesize) {
				YUVQueue.poll();
			}
	
			YUVQueue.add(data);
		}catch(Exception ex){
			Log.e(TAG,"onPreviewFrame exception");
			ex.printStackTrace();
		}
	}
	
	
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
    	Log.e(TAG,"surfaceDestroyed");
    	
        if (null != camera) {
        	camera.setPreviewCallback(null);
        	camera.stopPreview();
            camera.release();
            camera = null;
            avcCodec.StopAvcEncoderThread();
            pcmrecord.stopRecord();
            //audioRecord.stopAudioRecord();
        }
        
    	Context context = getApplicationContext();
        Intent innerIntent = new Intent(context, ServiceNetWork.class);
        innerIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startService(innerIntent);
        
        PublicFunction.writeLogFile("program end at:" + PublicFunction.formatCurrentDate() + "\r\n");
    }
	
	
    @TargetApi(9)
	private Camera getSelectedCamera(int index) {
        Camera camera = null;
        try {
        	camera = Camera.open(index);
            Log.e(TAG,"getSelectedCamera " + index + " ok");
        } catch (Exception e) {
        	Log.e(TAG,"getSelectedCamera exception");
            e.printStackTrace();
        }
        return camera; 				
    }
    
    



    private boolean startcamera(Camera mCamera){
        if(mCamera != null){
            try {
                mCamera.setPreviewCallback(this);
                //mCamera.setDisplayOrientation(90);
                
                Parameters parameters = mCamera.getParameters();
                
                int picwidth[] = new int[16];
                int picheight[] = new int [16];
                picwidth[0] = 1280;
                picheight[0] = 720;
                picwidth[1] = 1600;
                picheight[1] = 1200;
                picwidth[2] = 1024;
                picheight[2] = 768;
                picwidth[3] = 800;
                picheight[3] = 600;
                picwidth[4] = 640;
                picheight[4] = 480;
                
                List<Camera.Size> size = parameters.getSupportedPictureSizes();
                if (size.size() >= 1) {
                	size.listIterator();
					
					for (int i = 0; i < picheight.length; i++) {
						if (picwidth[i] == 0 || picheight[i] == 0) {
							continue;
						}
						
						Iterator<Camera.Size> it = size.iterator();
						while(it.hasNext()){
							Camera.Size constsize = it.next();
							if (constsize.width == picwidth[i] && constsize.height == picheight[i]) {
								width = picwidth[i];
								height = picheight[i];
								//default bitrate is width*height/4
								bitrate = (width*height/2)*bitrate_factor;
								break;
							}
						}
						
						if (width != 0 && height != 0) {
							break;
						}
					}
					
					if (width == 0 && height == 0) {
						Toast.makeText(Peeping.this, "not found suitable screen solution mode", Toast.LENGTH_SHORT).show();
						Log.e(TAG,"not found suitable screen solution mode");
						return false;
					}
				}else{
					Toast.makeText(Peeping.this, "screen solution not support", Toast.LENGTH_SHORT).show();
					Log.e(TAG,"screen solution not support");
					return false;
				}
                
                parameters.setPreviewFormat(ImageFormat.NV21);
                parameters.setPreviewSize(width, height);
                mCamera.setParameters(parameters);
                mCamera.setPreviewDisplay(surfaceHolder);
                mCamera.startPreview();
                mCamera.autoFocus(myAutoFocus);
                Log.e(TAG,"startcamera ok");
                return true;
            } catch (IOException e) {
            	Log.e(TAG,"startcamera exception");
                e.printStackTrace();
            }
        }
        return false;
    }
    

	private AutoFocusCallback myAutoFocus = new AutoFocusCallback() {
		@Override
		public void onAutoFocus(boolean success, Camera camera) {
			try{
				Log.e(TAG,"AutoFocusCallback ok");
			}catch(Exception ex){
				Log.e(TAG,"AutoFocusCallback exception");
				ex.printStackTrace();
			}
		}
	};



    
    int MakeAlertDialogBox(String message,String title){
	    new AlertDialog.Builder(this)
	    .setTitle(title)
	    .setMessage(message)
	    .show();
	    return 0;
    }
    
}
