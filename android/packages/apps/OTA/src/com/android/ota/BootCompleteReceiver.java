package com.android.ota;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BootCompleteReceiver extends BroadcastReceiver {
	static final String ACTION = "android.intent.action.BOOT_COMPLETED";
	public void onReceive(Context context, Intent intent) {
		if (intent.getAction().equals(ACTION)) {
			context.startService(new Intent(OTAService.SYSTEM_INIT_COMPLETE));
		}
	}
}
