package com.android.ota;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.XMLReader;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.content.SharedPreferences;
import android.content.Context;
import android.app.PendingIntent;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Build;
import android.util.Log;
import org.xml.sax.InputSource;

public class OTAService extends Service {

	protected static final String TAG = "OTAService";

	private static final String FILENAME_MSV = "/sys/board_properties/soc/msv";
	private static final String FILENAME_PROC_VERSION = "/proc/version";
	
	/* 查基带版本时使用 */
	// private static final String BASEBAND_PROPERTY_STRING =
	// "gsm.version.baseband";
	private static final String OTASERVICE_URL_PROPERTY_STRING = "ro.nufront.OTAService";
	private static final String OTASERVICE_VERSION_PROPERTY_STRING = "ro.livall.OTA.version";

	public static final String SYSTEM_INIT_COMPLETE = "com.nufront.otaservice.boot_completed";
	public static final String ACTION_CHECK_VERSION_RESULT = "com.nufront.otaservice.check_version_result";
	public static final String ACTION_CHECK_VERSION = "com.nufront.otaservice.check_version";

	public static final String BUNDLE_KEY_CHECK_VERSION_STATUS = "check_version_status";
	public static final String BUNDLE_KEY_FILE_NAME = "file_name";
	public static final String BUNDLE_KEY_FILE_MD5 = "file_md5";
	public static final String BUNDLE_KEY_FILE_URL = "file_URL";
	public static final String BUNDLE_KEY_FILE_SIZE = "file_size";
	public static final String BUNDLE_KEY_VERSION_HARDWARE = "hardware_version";
	public static final String BUNDLE_KEY_VERSION_KERNEL = "kernel_version";
	public static final String BUNDLE_KEY_VERSION_BUILD = "build_version";
	public static final String BUNDLE_KEY_VERSION_ANDROID = "android_version";
	public static final String BUNDLE_KEY_VERSION_BASEBAND = "baseband_version";
	public static final String BUNDLE_KEY_VERSION_BOOT = "boot_version";
	public static final String BUNDLE_KEY_VERSION_BASE = "base_version";
	public static final String BUNDLE_KEY_RELEASE_NOTES = "release_notes";
	public static final String BUNDLE_KEY_VERSION_PACKAGE = "package_version";

    public static final String LAST_CHECK_TIME = "last_check_time";

	public static final int STATUS_UNKNOW = 0x00;
	public static final int STATUS_NEW_VERSION = 0x01;
	public static final int STATUS_NO_NEED_TO_UPDATE = 0x02;
	public static final int STATUS_NO_CONNECTION = 0x03;
	public static final int STATUS_PARSER_XML_ERROR = 0x04;
    public static final int STATUS_CURRENT_VERSION_ERROR = 0x05;

	private int checkStatus;

	private String mServerPath;

	private ArrayList<VersionInfo> parserXMl() {

		ArrayList<VersionInfo> info = new ArrayList<VersionInfo>();
		InputStream inStream = null;

		try {
			mServerPath = getOTAServiceURL();
			Log.d(TAG, mServerPath);
			URL url = new URL(mServerPath);
			HttpURLConnection conn = (HttpURLConnection) url.openConnection();
			conn.setReadTimeout(5 * 1000);
			conn.setRequestMethod("GET");
			inStream = conn.getInputStream();
		} catch (Exception e) {
			//e.printStackTrace();
			Log.e(TAG, "Get XML error!");
			return null;
		}

		SAXParserFactory factory = SAXParserFactory.newInstance();

		try {
			XMLReader reader = factory.newSAXParser().getXMLReader();

			reader.setContentHandler(new VersionXMLHandler(info));
			if (inStream != null)
				reader.parse(new InputSource(inStream));

		} catch (Exception e) {
			// TODO: handle exception
			e.printStackTrace();
			Log.e(TAG, "XMLReader parse error!!!");
			return null;
		}
		return info;
	}

	@Override
	public IBinder onBind(Intent arg0) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void onCreate() {
		//Log.d(TAG, "onCreate");

	}

	@Override
	public void onStart(final Intent intent, int startId) {
		//Log.v(TAG, "onStart");
		Runnable checkVersionRunable = new Runnable() {

			@Override
			public void run() {

				checkStatus = STATUS_UNKNOW;

				if (intent != null) {

					ArrayList<VersionInfo> info = parserXMl();
					VersionInfo targetInfo = null;
					if ((info != null) && (info.size() != 0)) {
						targetInfo = comparisonVersion(info);
                        SimpleDateFormat formatter = new SimpleDateFormat ("yyyy-MM-dd HH:mm:ss");
                        Date curDate = new Date(System.currentTimeMillis());
                        String checkTime = formatter.format(curDate);
                        SharedPreferences lastCheckTime = getSharedPreferences(LAST_CHECK_TIME, 0);
                        lastCheckTime.edit().putString(LAST_CHECK_TIME,checkTime).commit();
					} else {
						checkStatus = STATUS_PARSER_XML_ERROR;
						Log.e(TAG, "parserXMl error!");
					}

					String action = intent.getAction();
					if (action.equals(ACTION_CHECK_VERSION)) {
						Intent resultIntent = new Intent(
								OTAService.ACTION_CHECK_VERSION_RESULT);
						Bundle result = new Bundle();
						result.putInt(BUNDLE_KEY_CHECK_VERSION_STATUS,
								checkStatus);
						if (checkStatus == STATUS_NEW_VERSION) {
							result.putString(BUNDLE_KEY_VERSION_PACKAGE,
									targetInfo.getPackageVersion());
							result.putString(BUNDLE_KEY_VERSION_BASE,
									targetInfo.getBaseVersion());
							result.putString(BUNDLE_KEY_RELEASE_NOTES,
									targetInfo.getReleaseNotes());
							result.putString(BUNDLE_KEY_FILE_NAME,
									targetInfo.getName());
							result.putString(BUNDLE_KEY_FILE_URL,
									targetInfo.getUrl());
							result.putString(BUNDLE_KEY_FILE_MD5,
									targetInfo.getMd5());
							result.putLong(BUNDLE_KEY_FILE_SIZE,
									targetInfo.getFileSize());
							result.putString(BUNDLE_KEY_VERSION_HARDWARE,
									targetInfo.getModelNumber());
							result.putString(BUNDLE_KEY_VERSION_KERNEL,
									targetInfo.getKernelVersion());
							result.putString(BUNDLE_KEY_VERSION_BUILD,
									targetInfo.getBuildNumber());
							result.putString(BUNDLE_KEY_VERSION_ANDROID,
									targetInfo.getAndroidVersion());
							result.putString(BUNDLE_KEY_VERSION_BASEBAND,
									targetInfo.getBasebandVersion());
							result.putString(BUNDLE_KEY_VERSION_BOOT,
									targetInfo.getBootVersion());
						}
						resultIntent.putExtras(result);
						sendBroadcast(resultIntent);
					} else if (action.equals(SYSTEM_INIT_COMPLETE)) {
						/* 有更新则提示用户有更新，无更新则不提示 */
						if (checkStatus == STATUS_NEW_VERSION) {
							PendingIntent pendingIntent = PendingIntent.getActivity(
								getApplicationContext(),0,new Intent(getApplicationContext(),
									OTAActivity.class),0);
							NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
							Notification mNotification = new Notification(
									R.drawable.ic_notification,
									getString(R.string.find_new_version),
									System.currentTimeMillis());
							mNotification.setLatestEventInfo(
									getApplicationContext(),
									getString(R.string.app_name),
									getString(R.string.find_new_version),
									pendingIntent);
							// mNotification.flags|=Notification.FLAG_AUTO_CANCEL;
							mNotification.defaults |= Notification.DEFAULT_SOUND;
							mNotificationManager.notify(R.string.app_name, mNotification);
						}
					}
				}
			}

		};
		Thread background = new Thread(checkVersionRunable);
		background.start();
	}

	@Override
	public void onDestroy() {
		//Log.v(TAG, "onDestroy");

	}

	/**
	 * Reads a line from the specified file.
	 * 
	 * @param filename
	 *            the file to read from
	 * @return the first line, if any.
	 * @throws IOException
	 *             if the file couldn't be read
	 */
	private String readLine(String filename) throws IOException {
		BufferedReader reader = new BufferedReader(new FileReader(filename),
				256);
		try {
			return reader.readLine();
		} finally {
			reader.close();
		}
	}

	/**
	 * Returns " (ENGINEERING)" if the msv file has a zero value, else returns
	 * "".
	 * 
	 * @return a string to append to the model number description.
	 */

	private String getMsvSuffix() {
		// Production devices should have a non-zero value. If we can't read it,
		// assume it's a
		// production device so that we don't accidentally show that it's an
		// ENGINEERING device.
		try {
			String msv = readLine(FILENAME_MSV);
			// Parse as a hex number. If it evaluates to a zero, then it's an
			// engineering build.
			if (Long.parseLong(msv, 16) == 0) {
				return " (ENGINEERING)";
			}
		} catch (IOException ioe) {
			// Fail quietly, as the file may not exist on some devices.
		} catch (NumberFormatException nfe) {
			// Fail quietly, returning empty string should be sufficient
		}
		return "";
	}

	private String getFormattedKernelVersion() {
		String procVersionStr;

		try {
			procVersionStr = readLine(FILENAME_PROC_VERSION);

			final String PROC_VERSION_REGEX = "\\w+\\s+" + /* ignore: Linux */
			"\\w+\\s+" + /* ignore: version */
			"([^\\s]+)\\s+" + /* group 1: 2.6.22-omap1 */
			"\\(([^\\s@]+(?:@[^\\s.]+)?)[^)]*\\)\\s+" + /*
														 * group 2:
														 * (xxxxxx@xxxxx
														 * .constant)
														 */
			"\\((?:[^(]*\\([^)]*\\))?[^)]*\\)\\s+" + /* ignore: (gcc ..) */
			"([^\\s]+)\\s+" + /* group 3: #26 */
			"(?:PREEMPT\\s+)?" + /* ignore: PREEMPT (optional) */
			"(.+)"; /* group 4: date */

			Pattern p = Pattern.compile(PROC_VERSION_REGEX);
			Matcher m = p.matcher(procVersionStr);

			if (!m.matches()) {
				Log.e(TAG, "Regex did not match on /proc/version: "
						+ procVersionStr);
				return "Unavailable";
			} else if (m.groupCount() < 4) {
				Log.e(TAG,
						"Regex match on /proc/version only returned "
								+ m.groupCount() + " groups");
				return "Unavailable";
			} else {
				return (new StringBuilder(m.group(1)).append("\n")
						.append(m.group(2)).append(" ").append(m.group(3))
						.append("\n").append(m.group(4))).toString();
			}
		} catch (IOException e) {
			Log.e(TAG,
					"IO Exception when getting kernel version for Device Info screen",
					e);

			return "Unavailable";
		}
	}

	/*
	 * 使用反射机制查当前基带版本的方法，暂时没有用 private String checkBasebandVersion() { Object
	 * result = null; try { Class cl =
	 * Class.forName("android.os.SystemProperties"); Object invoker =
	 * cl.newInstance(); Method m = cl.getMethod("get", new Class[] {
	 * String.class, String.class }); result = m.invoke(invoker, new Object[] {
	 * BASEBAND_PROPERTY_STRING, "no message" }); } catch (Exception e) {
	 * Log.e(TAG, "get currentBasebandVersion error!!"); }
	 * return (String) result; }
	 */
	private String getOTAServiceURL() {
		Object result = null;
		try {
			Class cl = Class.forName("android.os.SystemProperties");
			Object invoker = cl.newInstance();
			Method m = cl.getMethod("get", new Class[] { String.class,
					String.class });
			result = m.invoke(invoker, new Object[] {
					OTASERVICE_URL_PROPERTY_STRING, "null" });
		} catch (Exception e) {
			Log.e(TAG, "get OTASERVICE_URL_PROPERTY_STRING error!!");
		}
		return (String) result;
	}

	private String getOTAVersion() {
		Object result = null;
		try {
			Class cl = Class.forName("android.os.SystemProperties");
			Object invoker = cl.newInstance();
			Method m = cl.getMethod("get", new Class[] { String.class, String.class });
			result = m.invoke(invoker, new Object[] { OTASERVICE_VERSION_PROPERTY_STRING, "null" });
		} catch (Exception e) {
			Log.e(TAG, "get OTASERVICE_VERSION_PROPERTY_STRING error!!");
		}
		return (String) result;
	}

	private VersionInfo comparisonVersion(ArrayList<VersionInfo> info) {
		VersionInfo choice = null;
		int currentVersion = parseVersion();
        if (currentVersion < 0) {
            Log.e(TAG,"currentVersion error!!");
            checkStatus = STATUS_CURRENT_VERSION_ERROR;
            return null;
        }
		// String currentBasebandVersion;
		try {
			for (VersionInfo pInfo : info) {
				String str = pInfo.getPackageVersion();
				int pkgVersion = Integer.parseInt(str.substring(str
						.indexOf('b') + 1));
				if (choice != null) {
					str = choice.getPackageVersion();
					int choiceVersion = Integer.parseInt(str.substring(str
							.indexOf('b') + 1));
					if (pkgVersion < choiceVersion)
						continue;
				}
				if (pkgVersion > currentVersion) {
					str = pInfo.getBaseVersion();
					ArrayList<Integer> baseVersion = new ArrayList<Integer>();
					int start;
					int end;
					while (str != null) {
						start = str.indexOf('b') + 1;
						end = str.indexOf(',', start);
						if (end != -1) {
							baseVersion.add(Integer.parseInt(str.substring(
									start, end)));
							str = str.substring(end + 1);
						} else {
							baseVersion.add(Integer.parseInt(str
									.substring(start)));
							break;
						}
					}
					for (int i = 0; i < baseVersion.size(); i++) {
						if (currentVersion == baseVersion.get(i)) {
							choice = pInfo;
							checkStatus = STATUS_NEW_VERSION;
							break;
						}
					}
				} else {
					Log.d(TAG, "no need to update.");
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
			Log.e(TAG, "comparisonVersion error!");
			checkStatus = STATUS_PARSER_XML_ERROR;
			return null;
		}
		// currentBasebandVersion = checkBasebandVersion();
		// Log.d(TAG, "Build.DISPLAY: " + currentVersion);
		// Log.d(TAG, "Build.MODEL: " + Build.MODEL + getMsvSuffix());
		// Log.d(TAG, "Build.VERSION.RELEASE: " + Build.VERSION.RELEASE);
		// Log.d(TAG, "KernelVersion: " + getFormattedKernelVersion());
		/* 目前还不知道bootloader的版本怎么取 */

		if (checkStatus != STATUS_NEW_VERSION) {
			checkStatus = STATUS_NO_NEED_TO_UPDATE;
		} else {
			Log.d(TAG, "choiceVersion = " + choice.getPackageVersion());
		}
		return choice;
	}

    private int parseVersion() {
        String s = getOTAVersion(); 
        //range: b1~b999
        final String VERSION_REGEX = "b\\d{1,3}";
        Pattern p = Pattern.compile(VERSION_REGEX);
        Matcher m = p.matcher(s);
        if (!m.matches()) {
            Log.e(TAG, "Regex did not match!!");
            return -1;
        }
		return Integer.parseInt(s.substring(s.indexOf('b') + 1));
	}
}
