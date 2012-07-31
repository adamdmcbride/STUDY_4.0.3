package com.android.settings.sntp;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.Calendar;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.text.format.DateFormat;
import android.util.Log;

import android.content.Context;
import android.os.Handler;
import android.app.Service;
import android.os.IBinder;

/**
* {@hide}
*
* Simple SNTP client class for retrieving network time.
*
* Sample usage:
* <pre>
* SntpClient client = new SntpClient();
* if (client.requestTime("203.117.180.36")) {
*     long now = client.getNtpTime() + SystemClock.elapsedRealtime() - client.getNtpTimeReference();
* }
* </pre>
*/

public class SNTP extends Service {

    private static final String TAG = "SNTP--->";
    private static final boolean DBUG = true;
    private static Context mContext ;

    /*public static void SNTP( Context context ) {
        mContext = context ;
    }*/

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void onDestroy() {
        mSntpHandler.removeCallbacks(mSntpTask);
        if(DBUG) Log.d(TAG," SNTP Service end !! " );
    }

    @Override
    public void onStart(Intent intent, int startId) {
        super.onStart(intent, startId);

        if(DBUG) Log.d(TAG," start SNTP Service !! " );
        startSyncSNTP();
    }

    public void startSyncSNTP() {
        mSntpHandler.postDelayed(mSntpTask, 10);
    }

    private Handler mSntpHandler = new Handler();

    private Runnable mSntpTask = new Runnable(){
        public void run(){
            mSntpHandler.removeCallbacks(mSntpTask);
            if( ! syncSNTP() ) {
                mSntpHandler.postDelayed(mSntpTask, 3000);
            }
        }
    };

    public boolean syncSNTP( ) {
        SntpClient client = new SntpClient();
        if(client.requestTime("203.117.180.36",3000)) {
            long now = client.getNtpTime() + SystemClock.elapsedRealtime() - client.getNtpTimeReference();

            CharSequence ch=DateFormat.format("hh:mm:ss", now );
            if(DBUG) Log.d(TAG,"ch="+ch);
            CharSequence date=DateFormat.format("yyyy MM dd", now);
            if(DBUG) Log.d(TAG,"date="+date);

            if( SystemClock.setCurrentTimeMillis(now) ){
                if(DBUG) Log.d(TAG,"set Current Time ="+System.currentTimeMillis());
            }else{
                if(DBUG) Log.d(TAG," set Current Time  false !" );
            }

        }else{
            if(DBUG) Log.d(TAG,"sntp request time false !!" );
            return false ;
        }

        return true ;
    }

    public class SntpClient    {
        private static final int NTP_PACKET_SIZE = 48 ;
        private static final int NTP_PORT = 123;
        private static final int NTP_MODE_CLIENT = 3;
        private static final int NTP_VERSION = 3;
        /*
        *  Number of seconds between Jan 1, 1900 and Jan 1, 1970
        *  70 years plus 17 leap days
        */
        private static final long OFFSET_1900_TO_1970 = ((365L * 70L) + 17L) * 24L * 60L * 60L;
        private static final int TRANSMIT_TIME_OFFSET = 40;

        private static final int ORIGINATE_TIME_OFFSET = 24;
        private static final int RECEIVE_TIME_OFFSET = 32;

        // system time computed from NTP server response
        private long mNtpTime;
        // value of SystemClock.elapsedRealtime() corresponding to mNtpTime
        private long mNtpTimeReference;
        // round trip time in milliseconds
        private long mRoundTripTime;

        public boolean requestTime(String host, int timeout) {
            try {
                DatagramSocket socket = new DatagramSocket();
                socket.setSoTimeout(timeout);

                InetAddress address = InetAddress.getByName(host);
                byte[] buffer = new byte[NTP_PACKET_SIZE];
                DatagramPacket request = new DatagramPacket(buffer, buffer.length, address, NTP_PORT);
                /*
                * set mode = 3 (client) and version = 3
                * mode is in low 3 bits of first byte
                * version is in bits 3-5 of first byte
                */
                buffer[0] = NTP_MODE_CLIENT | (NTP_VERSION << 3);
                //get current time and write it to the request packet
                long requestTime = System.currentTimeMillis();
                if(DBUG) Log.d(TAG, "System.currentTimeMillis(): " + requestTime + " ms");
                long requestTicks = SystemClock.elapsedRealtime();
                writeTimeStamp(buffer, TRANSMIT_TIME_OFFSET, requestTime);
                socket.send(request);


                // read the response
                DatagramPacket response = new DatagramPacket(buffer, buffer.length);
                socket.receive(response);
                long responseTicks = SystemClock.elapsedRealtime();
                long responseTime = requestTime + (responseTicks - requestTicks);
                socket.close();

                // extract the results
                long originateTime = readTimeStamp(buffer, ORIGINATE_TIME_OFFSET);
                long receiveTime = readTimeStamp(buffer, RECEIVE_TIME_OFFSET);
                long transmitTime = readTimeStamp(buffer, TRANSMIT_TIME_OFFSET);
                long roundTripTime = responseTicks - requestTicks - (transmitTime - receiveTime);
                long clockOffset = (receiveTime - originateTime) + (transmitTime - responseTime);
                if(DBUG) Log.d(TAG, "round trip: " + roundTripTime + " ms");
                if(DBUG) Log.d(TAG, "clock offset: " + clockOffset + " ms");

                // save our results
                mNtpTime = receiveTime;//requestTime + clockOffset;
                mNtpTimeReference = requestTicks;
                mRoundTripTime = roundTripTime;

            }catch (Exception e) {
                // TODO: handle exception
                if(DBUG) Log.d(TAG, "error: " + e);
                return false;
            }

            return true ;
        }

        /**
        * Returns the time computed from the NTP transaction.
        *
        * @return time value computed from NTP server response.
        */
        public long getNtpTime() {
            return mNtpTime;
        }

        /**
        * Returns the reference clock value (value of SystemClock.elapsedRealtime())
        * corresponding to the NTP time.
        *
        * @return reference clock corresponding to the NTP time.
        */
        public long getNtpTimeReference() {
            return mNtpTimeReference;
        }

        /**
        * Returns the round trip time of the NTP transaction
        *
        * @return round trip time in milliseconds.
        */
        public long getRoundTripTime() {
            return mRoundTripTime;
        }

        /**
        * Reads an unsigned 32 bit big endian number from the given offset in the buffer.
        */
        private long read32(byte[] buffer, int offset) {
            byte b0 = buffer[offset];
            byte b1 = buffer[offset+1];
            byte b2 = buffer[offset+2];
            byte b3 = buffer[offset+3];

            // convert signed bytes to unsigned values
            int i0 = ((b0 & 0x80) == 0x80 ? (b0 & 0x7F) + 0x80 : b0);
            int i1 = ((b1 & 0x80) == 0x80 ? (b1 & 0x7F) + 0x80 : b1);
            int i2 = ((b2 & 0x80) == 0x80 ? (b2 & 0x7F) + 0x80 : b2);
            int i3 = ((b3 & 0x80) == 0x80 ? (b3 & 0x7F) + 0x80 : b3);
            return ((long)i0 << 24) + ((long)i1 << 16) + ((long)i2 << 8) + (long)i3;
        }

        /**
        * Reads the NTP time stamp at the given offset in the buffer and returns
        * it as a system time (milliseconds since January 1, 1970).
        */
        private long readTimeStamp(byte[] buffer, int offset) {
            long seconds = read32(buffer, offset);
            long fraction = read32(buffer, offset + 4);
            return ((seconds - OFFSET_1900_TO_1970) * 1000) + ((fraction * 1000L) / 0x100000000L);
        }

        /**
        * Writes system time (milliseconds since January 1, 1970) as an NTP time stamp
        * at the given offset in the buffer.
        */
        private void writeTimeStamp(byte[] buffer, int offset, long time) {
            long seconds = time / 1000L;
            long milliseconds = time - seconds * 1000L;
            seconds += OFFSET_1900_TO_1970;

            // write seconds in big endian format
            buffer[offset++] = (byte)(seconds >> 24);
            buffer[offset++] = (byte)(seconds >> 16);
            buffer[offset++] = (byte)(seconds >> 8);
            buffer[offset++] = (byte)(seconds >> 0);
            long fraction = milliseconds * 0x100000000L / 1000L;

            // write fraction in big endian format
            buffer[offset++] = (byte)(fraction >> 24);
            buffer[offset++] = (byte)(fraction >> 16);
            buffer[offset++] = (byte)(fraction >> 8);

            // low order bits should be random data
            buffer[offset++] = (byte)(Math.random() * 255.0);

        }
    }//class SntpClient


}