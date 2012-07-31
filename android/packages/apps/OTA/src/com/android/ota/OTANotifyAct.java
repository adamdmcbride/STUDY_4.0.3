package com.android.ota;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import com.android.ota.R;

import android.content.Context;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.Activity;
import android.content.Intent;
import android.nfc.Tag;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.os.Bundle;

public class OTANotifyAct extends Activity {

	private Button mButton;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.ota_notify_act);
		mButton = (Button) findViewById(R.id.button_start_update);
		mButton.setOnClickListener(mClickListener);
		NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		mNotificationManager.cancel(R.string.app_name);
		// Log.d("cuixiyuan++++++++++=",this.getIntent().getExtras().getString(OTAService.BUNDLE_KEY_VERSION_PACKAGE));
	}

	private Button.OnClickListener mClickListener = new Button.OnClickListener() {
		public void onClick(View v) {
			// RecoverySystem.rebootWipeUserData(getApplicationContext());
			// Log.d("cuixiyuan~~~~~~~~",
			// "android.intent.action.SYSTEM_UPDATE");
			if (writeMISC())
				sendBroadcast(new Intent("android.intent.action.SYSTEM_UPDATE"));
		}
	};

	private boolean writeMISC() {
		boolean writeOK = false;
		File MISC_RECOVERY_DIR = new File("/misc");
		File MISC_COMMAND_FILE = new File(MISC_RECOVERY_DIR, "boot_config");
		File MISC_OTA_CONFIG = new File(MISC_RECOVERY_DIR, "ota_config");
		Bundle versionInfo = this.getIntent().getExtras();

		MISC_RECOVERY_DIR.mkdirs();
		MISC_COMMAND_FILE.delete();
		MISC_OTA_CONFIG.delete();
		FileWriter command_misc;
		FileWriter ota_config;

		try {
			command_misc = new FileWriter(MISC_COMMAND_FILE);
		} catch (IOException e) {
			Log.e(OTAActivity.TAG, "command_misc new FileWriter() error!");
			command_misc = null;
		}
		try {
			ota_config = new FileWriter(MISC_OTA_CONFIG);
		} catch (IOException e) {
			Log.e(OTAActivity.TAG, "ota_config new FileWriter() error!");
			ota_config = null;
		}
		if (command_misc != null && ota_config != null) {
			try {
				command_misc.write("Boot system = recovery;");
				command_misc.write("\n");
				command_misc.write("Boot device = emmc;");
				command_misc.write("\n");
				command_misc.write("Clear data = no;");
				command_misc.write("\n");
				command_misc.write("Clear cache = no;");
				command_misc.write("\n");
				command_misc.write("OTA decide = yes;");
				command_misc.write("\n");
				ota_config
						.write("[parameters which OTA client transfer to Recovery]");
				ota_config.write("\n");
				ota_config
						.write("package_version = "
								+ versionInfo
										.getString(OTAService.BUNDLE_KEY_VERSION_PACKAGE)
								+ ";");
				ota_config.write("\n");
				ota_config.write("based_version = "
						+ versionInfo
								.getString(OTAService.BUNDLE_KEY_VERSION_BASE)
						+ ";");
				ota_config.write("\n");
				ota_config
						.write("[external parameters which OTA client transfer to Recovery]");
				ota_config.write("\n");
				ota_config
						.write("model_number = "
								+ versionInfo
										.getString(OTAService.BUNDLE_KEY_VERSION_HARDWARE)
								+ ";");
				ota_config.write("\n");
				ota_config
						.write("android_version = "
								+ versionInfo
										.getString(OTAService.BUNDLE_KEY_VERSION_ANDROID)
								+ ";");
				ota_config.write("\n");
				ota_config
						.write("baseband_version = "
								+ versionInfo
										.getString(OTAService.BUNDLE_KEY_VERSION_BASEBAND)
								+ ";");
				ota_config.write("\n");
				ota_config
						.write("kernel_version = "
								+ versionInfo
										.getString(OTAService.BUNDLE_KEY_VERSION_KERNEL)
								+ ";");
				ota_config.write("\n");
				ota_config.write("build_number = "
						+ versionInfo
								.getString(OTAService.BUNDLE_KEY_VERSION_BUILD)
						+ ";");
				ota_config.write("\n");
				ota_config.write("bootloader_version = "
						+ versionInfo
								.getString(OTAService.BUNDLE_KEY_VERSION_BOOT)
						+ ";");
				ota_config.write("\n");
				writeOK = true;
			} catch (IOException e) {
				Log.e(OTAActivity.TAG, "command_misc.write error!");
			}
		}
		if (command_misc != null) {
			try {
				command_misc.close();
			} catch (IOException e) {
				Log.e(OTAActivity.TAG, "command_misc close error!");
			}
		}
		if (ota_config != null) {
			try {
				ota_config.close();
			} catch (IOException e) {
				Log.e(OTAActivity.TAG, "ota_config close error!");
			}
		}
		if (!writeOK) {
			MISC_COMMAND_FILE.delete();
			MISC_OTA_CONFIG.delete();
		}
		return writeOK;
	}
}
