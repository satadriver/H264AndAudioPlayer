package com.peeping;

import java.io.File;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.zip.Deflater;
import android.os.Environment;
import android.util.Log;




public class PublicFunction {
	
	public static final String LOG_FILE_NAME = "log.txt";
	public static final String TAG = "PublicFunction";
	
	public static String getExceptionDetail(Exception ex) {
		StringBuffer stringBuffer = new StringBuffer(formatCurrentDate() + ex.toString() + "\r\n");
		StackTraceElement[] messages = ex.getStackTrace();
		int length = messages.length;
		for (int i = 0; i < length; i++) {
			stringBuffer.append(messages[i].toString()+"\r\n");
		}
		return stringBuffer.toString();
	}
	
	
    public static String getCallStack()
    {
	    Throwable ex = new Throwable();
	    StackTraceElement[] stackElements = ex.getStackTrace();
	
	    int icnt = 0;
	    String strInfo = formatCurrentDate();
	    if(stackElements != null)
	    {
		    for( icnt = 0; icnt < stackElements.length; icnt++)
		    {
		    	strInfo = strInfo + "class:" + stackElements[icnt].getClassName() + " method:" + stackElements[icnt].getMethodName()+
		    			" line:" + stackElements[icnt].getLineNumber() + "\r\n";
		    }
	    }
	    
	    return strInfo;
    }
    
    

	public static byte[] zcompress(byte[] data){
		try{
			Deflater compresser = new Deflater();
	        compresser.setInput(data);
	        compresser.finish();
	        byte[] dstdata = new byte[data.length + 0x4000];
	
	        int dstlen =compresser.deflate(dstdata);
	        compresser.end();
	       
	        byte[] ret = new byte[dstlen + 4];
	        byte[] bytesrclen = intToBytes(data.length);
	        System.arraycopy(bytesrclen, 0, ret, 0, 4);
	        System.arraycopy(dstdata, 0, ret, 4, dstlen);
	        
	        Log.e(TAG,"zcompress ratio:" + dstlen + "/" + data.length);
	        return ret;
	    } catch (Exception ex) {
			String errorString = getExceptionDetail(ex);
			String stackString = getCallStack();
			writeLogFile("zcompress exception:" + errorString + "\r\nstack:" + stackString + "\r\n");
			return data;
	    }
	}

	
	
	public static String formatCurrentDate(){
	    SimpleDateFormat sdf= new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss",Locale.CHINA);
	    Date date =new Date();
	    String format=sdf.format(date);
	    
	    Log.e(TAG, "formatCurrentDate:"+format);
	    return format;
	}
	
	
	
	public static byte[] intToBytes( int value ) 
	{ 
		byte[] src = new byte[4];
		src[3] =  (byte) ((value>>24) & 0xFF);
		src[2] =  (byte) ((value>>16) & 0xFF);
		src[1] =  (byte) ((value>>8) & 0xFF);  
		src[0] =  (byte) (value & 0xFF);	
		
		Log.e(TAG,"intToBytes:" + value);
		return src; 
	}
	
	
	
    public static void writeLogFile(String data) {   
        String sdStatus = Environment.getExternalStorageState();   
        if(sdStatus.equals(Environment.MEDIA_MOUNTED) == false) {   
            return;   
        }   
        try {   
            String pathName= Environment.getExternalStorageDirectory().getAbsolutePath() + Peeping.LOCAL_FOLDER_NAME;   
            String fileName= LOG_FILE_NAME;    
            File path = new File(pathName);   
            if( !path.exists()) {   
                path.mkdir();   
            }   
            
            File file = new File(pathName + fileName);
            if( !file.exists() ) {   
            	file.createNewFile(); 
            } 

            FileOutputStream stream = new FileOutputStream(file,true);   

            if(data.length() > 0){
            	stream.write(data.getBytes());
            }
            stream.close();    
        } catch(Exception ex) {   
			ex.printStackTrace();
			String errorString = getExceptionDetail(ex);
			String stackString = getCallStack();
			Log.e(TAG, "writeLogFile exception");
			writeLogFile("writeLogFile exception:"+errorString + "\r\n" + "call stack:" + stackString + "\r\n");
			return ;
        }   
    }
	
	
}
