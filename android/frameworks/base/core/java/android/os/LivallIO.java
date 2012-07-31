/*
 * Copyright (C) 2010~2012 hengai halfhq@163.com
 *
 * LivallService only for Livall company.
 */

package android.os;

import android.util.Log;
import android.os.SystemProperties;

/**
 * @hide 
 */
public class LivallIO
{
    private static final String TAG = "LivallIO";

    public static Key getKeyInstance() {
        ILivallService lvService = getService();
        if(lvService == null)
            return null;
        return new Key(lvService);
    }

    public static CPU getCPUInstance() {
        ILivallService lvService = getService();
        if(lvService == null)
            return null;
        return new CPU(lvService);
    }

    public LivallIO() {
    }

    public static class Key {
        private Key(ILivallService lvService) {
            mService = lvService;
            try{
                mSerialNumber = mService.getSerialNumber();
            } catch (RemoteException e) {
                Log.w(TAG, "Failed to Livall.", e);
            }
        }

        public String getSerialNumber() {
            return mSerialNumber;
        }
        private String mSerialNumber = null;
        private ILivallService mService = null;
    }

    /**
     * CPU
     */
    public static class CPU {
        public static final int CPUGOVERNOR_PERFORMANCE = 0;    //
        public static final int CPUGOVERNOR_CONSERVATIVE = 1;   //
        public static final int CPUGOVERNOR_ONDEMAND = 2;           //
        public static final int CPUGOVERNOR_POWERSAVE = 3;      //

        private CPU(ILivallService lvService) {
            mService = lvService;
        }

        public int getGovernor(){
            int governor = -1;
            try{
                governor = mService.getCpuGovernor(0, false);
            } catch (RemoteException e) {
                Log.w(TAG, "Failed to Livall.", e);
            }
            return governor;
        }

        public int setGovernor(int governor) {
            int ret = -1;
            try{
                ret = mService.setCpuGovernor(3, governor, 5*1000);
            } catch (RemoteException e) {
                Log.w(TAG, "Failed to Livall.", e);
            }
            return ret;
        }

        private ILivallService mService = null;
    }

    private static ILivallService getService() {
        return ILivallService.Stub.asInterface(
                    ServiceManager.getService("livall"));
    }
}
