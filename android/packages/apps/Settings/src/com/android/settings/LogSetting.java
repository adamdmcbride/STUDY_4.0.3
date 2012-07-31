/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.settings;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Toast;

public class LogSetting extends Activity {
	private final String TAG = "phone";

	private final boolean DBG = true;

	private static final String KEY_MODEM_LOG_CONTROLER = "modem_log";

	private CheckBox modem_log;
	private Button btn1 = null;
	SharedPreferences pres;
	private static boolean log_old_stat = false;

	static {
		System.loadLibrary("logControl-jni");
	}

	public native boolean openModemLog();

	public native boolean closeModemLog();
	
	public native boolean clearLogs();

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.log_info);

		modem_log = (CheckBox) findViewById(R.id.log_controler);
		modem_log.setOnCheckedChangeListener(c_listener);
		
		btn1 = (Button)findViewById(R.id.clr_btn);
		btn1.setOnClickListener(new OnClickListener()
		{
			@Override
			public void onClick(View v) {
				modem_log.setChecked(false);
				if(clearLogs())
				{
					Toast.makeText(LogSetting.this, "Clear modem log successful.",
							Toast.LENGTH_SHORT).show();
				}
				else
				{
					Toast.makeText(LogSetting.this, "Clear modem log failure!",
							Toast.LENGTH_SHORT).show();
				}
			}
			
		});
	}

	private OnCheckedChangeListener c_listener = new OnCheckedChangeListener() {

		@Override
		public void onCheckedChanged(CompoundButton buttonView,
				boolean isChecked) {
			// TODO Auto-generated method stub
			pres = PreferenceManager
					.getDefaultSharedPreferences(LogSetting.this);
			// pres =
			// LogSetting.this.getSharedPreferences("save_log",Context.MODE_PRIVATE);
			if (buttonView.getId() == R.id.log_controler) {
				if (isChecked) {
					if (!log_old_stat) {
                       if (openModemLog()) {
                          log_old_stat = true;
                          Editor editor = pres.edit();
                          editor.putBoolean(KEY_MODEM_LOG_CONTROLER, true);
                          editor.apply();
                          Toast.makeText(LogSetting.this, "Open modem log successful.",
                                Toast.LENGTH_SHORT).show();
                       } else {
                          log_old_stat = false;
                          Toast.makeText(LogSetting.this,
                                "Open modem log failure!", Toast.LENGTH_SHORT)
                             .show();
                          modem_log.setChecked(false);
                       }
					}
					if (DBG)
						Log.d(TAG, " isChecked is true.");
				} else {
					if (log_old_stat) {
                       if (closeModemLog()) {
                          log_old_stat = false;
                          Editor editor = pres.edit();
                          editor.putBoolean(KEY_MODEM_LOG_CONTROLER, false);
                          editor.apply();
                          Toast.makeText(LogSetting.this, "Close modem log successful.",
                                Toast.LENGTH_SHORT).show();
                       } else {
                          log_old_stat = true;
                          Toast.makeText(LogSetting.this,
                                "Close modem log failure!", Toast.LENGTH_SHORT)
                             .show();
                          modem_log.setChecked(true);
                       }
					}
					if (DBG)
						Log.d(TAG, " isChecked is false.");
				}
			}
		}

	};

	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		super.onResume();
		if (DBG)
			Log.d(TAG, "LogSetting---onResume");
		pres = PreferenceManager.getDefaultSharedPreferences(LogSetting.this);
		// pres =
		// LogSetting.this.getSharedPreferences("save_log",Context.MODE_PRIVATE);
		if (pres.getBoolean(KEY_MODEM_LOG_CONTROLER, false)) {
			log_old_stat = true;
			if (DBG)
				Log.d(TAG, "LogSetting---onResume---preference-true");
			modem_log.setChecked(true);
		}
        else
        {
			log_old_stat = false;
        }
	}

	@Override
	protected void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
	}

}
