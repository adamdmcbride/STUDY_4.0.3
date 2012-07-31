package com.android.ota;

import java.io.File;
import java.io.IOException;
import java.security.GeneralSecurityException;

import com.android.ota.R;

import android.widget.TextView;
import android.content.SharedPreferences;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DownloadManager;
import android.app.DownloadManager.Request;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.os.RecoverySystem;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.Toast;

public class OTAActivity extends Activity {

	protected static final String TAG = "OTAActivity";
	protected static final int IMAGE_DOWNLOAD_FAILED = 0x00;
	protected static final int IMAGE_DOWNLOAD_SUCCESSFUL = 0x01;
	protected static final int CHECK_VERSION_START = 0x02;
	protected static final int CHECK_VERSION_END = 0x03;
	protected static final int CHECK_VERSION_SUCCESS = 0x04;
	protected static final int CHECK_VERSION_ERROR = 0x05;
	protected static final int INSUFFICIENT_SPACE = 0x06;
    protected static final int CHECK_CURRENT_VERSION_ERROR = 0x07;

	private VersionInfo mPackageInfo = new VersionInfo();
	private Button mButton;
	private DownloadManager mDownloadManager = null;
	private OTAReceiver mOTAReceiver;

	public class OTAReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			Log.d(TAG, action);
			if (action.equals(OTAService.ACTION_CHECK_VERSION_RESULT)) {
				Bundle bundle = intent.getExtras();
				Message message = new Message();

				int check_status = bundle
						.getInt(OTAService.BUNDLE_KEY_CHECK_VERSION_STATUS);
				if (check_status == OTAService.STATUS_NEW_VERSION) {

					mPackageInfo.setPackageVersion(bundle.getString(
							OTAService.BUNDLE_KEY_VERSION_PACKAGE, null));
					mPackageInfo.setBaseVersion(bundle.getString(
							OTAService.BUNDLE_KEY_VERSION_BASE, null));
					mPackageInfo.setName(bundle.getString(
							OTAService.BUNDLE_KEY_FILE_NAME, null));
					mPackageInfo.setReleaseNotes(bundle.getString(
							OTAService.BUNDLE_KEY_RELEASE_NOTES, null));
					mPackageInfo.setMd5(bundle.getString(
							OTAService.BUNDLE_KEY_FILE_MD5, null));
					mPackageInfo.setFileSize(bundle.getLong(
							OTAService.BUNDLE_KEY_FILE_SIZE, 0));
					mPackageInfo.setUrl(bundle.getString(
							OTAService.BUNDLE_KEY_FILE_URL, null));
					mPackageInfo.setAndroidVersion(bundle.getString(
							OTAService.BUNDLE_KEY_VERSION_ANDROID, null));
					mPackageInfo.setBasebandVersion(bundle.getString(
							OTAService.BUNDLE_KEY_VERSION_BASEBAND, null));
					mPackageInfo.setKernelVersion(bundle.getString(
							OTAService.BUNDLE_KEY_VERSION_KERNEL, null));
					mPackageInfo.setModelNumber(bundle.getString(
							OTAService.BUNDLE_KEY_VERSION_HARDWARE, null));
					mPackageInfo.setBuildNumber(bundle.getString(
							OTAService.BUNDLE_KEY_VERSION_BUILD, null));
					mPackageInfo.setBootVersion(bundle.getString(
							OTAService.BUNDLE_KEY_VERSION_BOOT, null));
					if (checkSdcardSpace(mPackageInfo.getFileSize())) {
						message.what = OTAActivity.CHECK_VERSION_SUCCESS;
					} else {
						message.what = OTAActivity.INSUFFICIENT_SPACE;
					}
				} else if (check_status == OTAService.STATUS_NO_NEED_TO_UPDATE) {
					message.what = OTAActivity.CHECK_VERSION_END;
				} else if (check_status == OTAService.STATUS_CURRENT_VERSION_ERROR) {
					message.what = OTAActivity.CHECK_CURRENT_VERSION_ERROR;
				} else {
					message.what = OTAActivity.CHECK_VERSION_ERROR;
				}

				mHandler.sendMessage(message);
			}
		}
	}

	private Button.OnClickListener mClickListener = new Button.OnClickListener() {
		public void onClick(View v) {

			IntentFilter filter = new IntentFilter(
					OTAService.ACTION_CHECK_VERSION_RESULT);
			mOTAReceiver = new OTAReceiver();
			registerReceiver(mOTAReceiver, filter);
			Intent intent = new Intent(OTAService.ACTION_CHECK_VERSION);
			startService(intent);

			Message message = new Message();
			message.what = OTAActivity.CHECK_VERSION_START;
			mHandler.sendMessage(message);

		}
	};

	private boolean checkSdcardSpace(long fileSize) {

		if (!Environment.getExternalStorageState().equals(
				Environment.MEDIA_MOUNTED)) {
			Log.e(TAG, "SD card didn't mounted.");
			return false;
		}

		StatFs sdcardStat = new StatFs(Environment
				.getExternalStorageDirectory().getPath());
		long blockSize = sdcardStat.getBlockSize();
		int avaliableBlocks = sdcardStat.getAvailableBlocks();

		if (avaliableBlocks * blockSize < fileSize * 2) {
			Log.e(TAG, avaliableBlocks * blockSize + "Insufficient space!!");
			return false;
		}

		return true;
	}

	private long startDownload(String url, String imageName) {
		long lastDownload = -1L;
		Request dwreq;
		Uri uri = Uri.parse(url);

		mDownloadManager = (DownloadManager) getSystemService(DOWNLOAD_SERVICE);
		File dir = Environment
				.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
		dir.mkdir();
		File file = new File(dir, imageName);
		if (file.exists()) {
			Log.e(TAG, "file exists!!!");
			file.delete();
		}

		try {
			dwreq = new DownloadManager.Request(uri);
		} catch (IllegalArgumentException e) {
			Toast.makeText(getApplicationContext(), R.string.cannot_download,
					Toast.LENGTH_SHORT).show();
			return lastDownload;
		}
		dwreq.setTitle(getString(R.string.download_title));
		dwreq.setDescription(imageName);
		dwreq.setDestinationInExternalPublicDir(
				Environment.DIRECTORY_DOWNLOADS, imageName);
		dwreq.setNotificationVisibility(Request.VISIBILITY_VISIBLE);
		dwreq.setShowRunningNotification(true);
		lastDownload = mDownloadManager.enqueue(dwreq);
		Toast.makeText(getApplicationContext(), R.string.download_pending,
				Toast.LENGTH_SHORT).show();
		return lastDownload;
	}

	private Handler mHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case OTAActivity.CHECK_VERSION_START:
				findViewById(R.id.progressBar).setVisibility(
						ProgressBar.VISIBLE);
				break;
			case OTAActivity.CHECK_VERSION_END:
				unregisterReceiver(mOTAReceiver);
				findViewById(R.id.progressBar).setVisibility(
						ProgressBar.INVISIBLE);
				Toast.makeText(getApplicationContext(),
						R.string.no_need_to_update, Toast.LENGTH_SHORT).show();
                updateCheckTime();
				break;
			case OTAActivity.CHECK_VERSION_ERROR:
				unregisterReceiver(mOTAReceiver);
				findViewById(R.id.progressBar).setVisibility(
						ProgressBar.INVISIBLE);
				Toast.makeText(getApplicationContext(),
						R.string.check_version_failed, Toast.LENGTH_SHORT)
						.show();
				break;
			case OTAActivity.CHECK_CURRENT_VERSION_ERROR:
				unregisterReceiver(mOTAReceiver);
				findViewById(R.id.progressBar).setVisibility(
						ProgressBar.INVISIBLE);
				Toast.makeText(getApplicationContext(),
						R.string.current_version_error, Toast.LENGTH_SHORT)
						.show();
				break;
			case OTAActivity.INSUFFICIENT_SPACE:
				unregisterReceiver(mOTAReceiver);
				findViewById(R.id.progressBar).setVisibility(
						ProgressBar.INVISIBLE);
				Toast.makeText(getApplicationContext(),
						R.string.insufficient_space, Toast.LENGTH_SHORT).show();
				break;
			case OTAActivity.CHECK_VERSION_SUCCESS:
				unregisterReceiver(mOTAReceiver);
				findViewById(R.id.progressBar).setVisibility(
						ProgressBar.INVISIBLE);
                updateCheckTime();

				Dialog dialog = new AlertDialog.Builder(OTAActivity.this)
						.setTitle(R.string.app_name)
						.setMessage(R.string.find_new_version)
						.setPositiveButton(R.string.ok,
								new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog,
											int whichButton) {
										long downloadID = startDownload(
												mPackageInfo.getUrl(),
												mPackageInfo.getName());
										if (downloadID == -1)
											return;
										queryDownloadStatus(downloadID);
									}
								})
						.setNeutralButton(R.string.cancel,
								new DialogInterface.OnClickListener() {
									public void onClick(DialogInterface dialog,
											int whichButton) {
										/*
										 * User clicked Something so do some
										 * stuff
										 */
									}
								}).create();
				dialog.show();
				break;
			case OTAActivity.IMAGE_DOWNLOAD_FAILED:
				Toast.makeText(getApplicationContext(),
						R.string.cannot_download, Toast.LENGTH_SHORT).show();
				break;
			case OTAActivity.IMAGE_DOWNLOAD_SUCCESSFUL:

				String packageName = Environment
						.getExternalStoragePublicDirectory(
								Environment.DIRECTORY_DOWNLOADS).getPath()
						+ "/" + mPackageInfo.getName();
				String imageMd5 = Md5.md5sum(packageName);
				boolean verifyResult = false;
				try {
					RecoverySystem.verifyPackage(new File(packageName), null,
							null);
					verifyResult = true;
					Log.d(TAG, "verifyPackage Successful.");
				} catch (IOException e) {
					Log.e(TAG, "verifyPackage error!!");
				} catch (GeneralSecurityException e) {
					Log.e(TAG, "verifyPackage failed!!");
				}

				if (imageMd5.equals(mPackageInfo.getMd5()) && verifyResult) {
					Bundle MISCInfo = new Bundle();
					MISCInfo.putString(OTAService.BUNDLE_KEY_VERSION_PACKAGE,
							mPackageInfo.getPackageVersion());
					MISCInfo.putString(OTAService.BUNDLE_KEY_VERSION_BASE,
							mPackageInfo.getBaseVersion());
					MISCInfo.putString(OTAService.BUNDLE_KEY_VERSION_HARDWARE,
							mPackageInfo.getModelNumber());
					MISCInfo.putString(OTAService.BUNDLE_KEY_VERSION_ANDROID,
							mPackageInfo.getAndroidVersion());
					MISCInfo.putString(OTAService.BUNDLE_KEY_VERSION_BASEBAND,
							mPackageInfo.getBasebandVersion());
					MISCInfo.putString(OTAService.BUNDLE_KEY_VERSION_KERNEL,
							mPackageInfo.getKernelVersion());
					MISCInfo.putString(OTAService.BUNDLE_KEY_VERSION_BUILD,
							mPackageInfo.getBuildNumber());
					MISCInfo.putString(OTAService.BUNDLE_KEY_VERSION_BOOT,
							mPackageInfo.getBootVersion());

					PendingIntent pendingIntent = PendingIntent.getActivity(
							getApplicationContext(), 0,
							new Intent(getApplicationContext(),
									OTANotifyAct.class).putExtras(MISCInfo), 0);
					NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
					Notification mNotification = new Notification(
							R.drawable.ic_notification,
							getString(R.string.complete_download),
							System.currentTimeMillis());
					mNotification.setLatestEventInfo(getApplicationContext(),
							getString(R.string.app_name),
							getString(R.string.complete_download),
							pendingIntent);
					// mNotification.flags|=Notification.FLAG_AUTO_CANCEL;
					mNotification.defaults |= Notification.DEFAULT_SOUND;
					mNotificationManager.notify(R.string.app_name, mNotification);
				} else {
					PendingIntent pendingIntent = PendingIntent.getActivity(
							getApplicationContext(), 0,
							new Intent(getApplicationContext(),
									OTAActivity.class), 0);
					NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
					Notification mNotification = new Notification(
							R.drawable.ic_notification,
							getString(R.string.cannot_download),
							System.currentTimeMillis());
					mNotification.setLatestEventInfo(getApplicationContext(),
							getString(R.string.app_name),
							getString(R.string.cannot_download), pendingIntent);
					mNotification.flags |= Notification.FLAG_AUTO_CANCEL;
					mNotification.defaults |= Notification.DEFAULT_SOUND;
					mNotificationManager.notify(R.string.app_name, mNotification);
				}
				break;
			default:
				break;
			}
			super.handleMessage(msg);
		}
	};

	private void queryDownloadStatus(final long downloadID) {

		Runnable queryRunable = new Runnable() {

			int downok = -1;
			Cursor downloadStatus = null;
			int timeOut = 5;

			@Override
			public void run() {
				while (downok != DownloadManager.STATUS_SUCCESSFUL) {
					downloadStatus = mDownloadManager
							.query(new DownloadManager.Query()
									.setFilterById(downloadID));
					downloadStatus.moveToFirst();
					if (downloadStatus == null) {
						Log.e(TAG, "downloadStatus is null!");
					} else {
						downok = downloadStatus.getInt(downloadStatus
								.getColumnIndex(DownloadManager.COLUMN_STATUS));
						Log.d(TAG, "downok = " + downok);
					}
					downloadStatus.close();

					if (downok == DownloadManager.STATUS_FAILED) {
						Message message = new Message();
						message.what = OTAActivity.IMAGE_DOWNLOAD_FAILED;
						mHandler.sendMessage(message);
						mDownloadManager.remove(downloadID);
						break;
					}

					if (downok == DownloadManager.STATUS_PAUSED) {
						timeOut--;
						if (timeOut < 0) {
							Log.e(TAG, "Donwload package time out!");
							Message message = new Message();
							message.what = OTAActivity.IMAGE_DOWNLOAD_FAILED;
							mHandler.sendMessage(message);
							mDownloadManager.remove(downloadID);
							break;
						}
					}

					try {
						Thread.sleep(2000);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}

				if (downok == DownloadManager.STATUS_SUCCESSFUL) {
					Message message = new Message();
					message.what = OTAActivity.IMAGE_DOWNLOAD_SUCCESSFUL;
					mHandler.sendMessage(message);
				}
			}
		};
		Thread background = new Thread(queryRunable);
		background.start();
	}

    private void updateCheckTime() {
        SharedPreferences lastCheckPref = getSharedPreferences(OTAService.LAST_CHECK_TIME, 0);
        String time = lastCheckPref.getString(OTAService.LAST_CHECK_TIME, "2012-05-20 05:21:00");
        String s = getString(R.string.last_check) + time;
        TextView checkTimeView = (TextView) findViewById(R.id.text_last_check_time);
        checkTimeView.setText(s);
    }

	@Override
	public void onCreate(Bundle savedState) {
		super.onCreate(savedState);
		setContentView(R.layout.main);
		mButton = (Button) findViewById(R.id.button_check_version);
		mButton.setOnClickListener(mClickListener);
		NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		mNotificationManager.cancel(R.string.app_name);
	}

    @Override
    public void onResume() {
        super.onResume();
        updateCheckTime();
    }
	@Override
	public void onPause() {
		super.onPause();
	}
}
