/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server;

import android.content.Context;
import android.os.ILivallService;
import android.os.SystemProperties;
import android.util.Slog;
import android.provider.Settings;
import android.content.Intent;
import android.os.ServiceManager;
import android.os.Handler;
import android.os.Message;

/** {@hide} */
public class LivallService extends ILivallService.Stub {
    private static final String TAG = "LivallService";
    public static final int CPUGOVERNOR_PERFORMANCE = 0;    //
    public static final int CPUGOVERNOR_CONSERVATIVE = 1;   //
    public static final int CPUGOVERNOR_ONDEMAND = 2;           //
    public static final int CPUGOVERNOR_POWERSAVE = 3;      //

    public static final int MAXCPUS = 2;
    private int[] mCpuGovernor = new int[MAXCPUS];

    private final H mHandler = new H();
    @SuppressWarnings("unused")

    LivallService(Context context) {
        mContext = context;
        nativeInit();
    }

    void systemReady() {
        mCpuGovernor[0] = Settings.Secure.getInt(mContext.getContentResolver(), Settings.Secure.CPUSETTINGS_CPU0GOVERNOR, 0);
        mCpuGovernor[1] = Settings.Secure.getInt(mContext.getContentResolver(), Settings.Secure.CPUSETTINGS_CPU1GOVERNOR, 0);
        mHandler.sendMessageDelayed(mHandler.obtainMessage(H.HANDLE_CPUGOVERNOR), 5*60*1000);
    }

    public String getSerialNumber() {
        return nativeGetSerialNumber();
    }

    public int getCpuGovernor(int cpu, boolean force) {
        if(cpu<0 || cpu>=MAXCPUS) cpu = 0;
        if(force) {
            return nativeGetCpuGovernor(cpu);
        } else {
            return mCpuGovernor[cpu];
        }
    }

    public int setCpuGovernor(int cpus, int governor, int timeOut) {
        int ret = nativeSetCpuGovernor(cpus, governor, timeOut);
        if(ret == 0) {
            mHandler.removeMessages(H.HANDLE_CPUGOVERNOR);
            for(int i=0; i<cpus; i++) {
                if( 0!=(cpus & (1<<i))) {
                    mCpuGovernor[i] = governor;
                }
            }
            Settings.Secure.putInt(mContext.getContentResolver(), Settings.Secure.CPUSETTINGS_CPU0GOVERNOR, mCpuGovernor[0]);
            Settings.Secure.putInt(mContext.getContentResolver(), Settings.Secure.CPUSETTINGS_CPU1GOVERNOR, mCpuGovernor[1]);
        }
        return ret;
    }

    native private boolean nativeInit();
    native private String nativeGetSerialNumber();
    native private int nativeGetCpuGovernor(int cpu);
    native private int nativeSetCpuGovernor(int cpus, int governor, int timeOut);
    private final Context mContext;

    final class H extends Handler {
        public static final int HANDLE_CPUGOVERNOR = 1;
        public H() {
        }
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case HANDLE_CPUGOVERNOR:
                setCpuGovernor(3, mCpuGovernor[0], 2*1000);
                break;
            }
        }
    }
}
