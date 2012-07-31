package com.android.ota;

import java.util.ArrayList;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import android.util.Log;

public class VersionXMLHandler extends DefaultHandler {

	private static final String TAG = "VersionXMLHandler";
	private ArrayList<VersionInfo> mInfo;
	private String preTAG;
	private int pkgCount;

	public VersionXMLHandler() {
		// TODO Auto-generated constructor stub
		super();
	}

	public VersionXMLHandler(ArrayList<VersionInfo> info) {
		super();
		mInfo = info;
		pkgCount = 0;
	}

	@Override
	public void startDocument() throws SAXException {
		Log.d(TAG, "startDocument.");
		super.startDocument();
	}

	@Override
	public void startElement(String uri, String localName, String qName,
			Attributes attributes) throws SAXException {
		Log.d(TAG, "localName:" + localName);
		if (localName.equals("package")) {
			mInfo.add(new VersionInfo());
		} else {
			preTAG = localName;
		}
		super.startElement(uri, localName, qName, attributes);
	}

	@Override
	public void endElement(String uri, String localName, String qName)
			throws SAXException {
		Log.d(TAG, "endElement");
		preTAG = "";
		if (localName.equals("package")) {
			pkgCount++;
		}
		super.endElement(uri, localName, qName);
	}

	@Override
	public void characters(char[] ch, int start, int length)
			throws SAXException {

		String dateString;
		if ("package_version".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setPackageVersion(dateString);
			//Log.d("speciality=", mInfo.get(pkgCount).getPackageVersion());
		} else if ("base_version".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setBaseVersion(dateString);
			//Log.d("base_version=",
					//String.valueOf(mInfo.get(pkgCount).getBaseVersion()));
		} else if ("release_notes".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setReleaseNotes(dateString);
			//Log.d("release_notes=",
					//String.valueOf(mInfo.get(pkgCount).getReleaseNotes()));
		} else if ("fileName".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setName(dateString);
			//Log.d("speciality=", mInfo.get(pkgCount).getName());
		} else if ("downloadUrl".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setUrl(dateString);
			//Log.d("speciality=", mInfo.get(pkgCount).getUrl());
		} else if ("fileMd5".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setMd5(dateString);
			//Log.d("fileMd5=", String.valueOf(mInfo.get(pkgCount).getMd5()));
		} else if ("fileSize".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setFileSize(Long.parseLong(dateString));
			//Log.d("fileSize", "fileSize = " + mInfo.get(pkgCount).getFileSize());
		} else if ("model_number".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setModelNumber(dateString);
			//Log.d("hardware_version=",
					//String.valueOf(mInfo.get(pkgCount).getModelNumber()));
		} else if ("android_version".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setAndroidVersion(dateString);
			//Log.d("android_version=",
					//String.valueOf(mInfo.get(pkgCount).getAndroidVersion()));
		} else if ("baseband_version".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setBasebandVersion(dateString);
			//Log.d("baseband_version=",
					//String.valueOf(mInfo.get(pkgCount).getBasebandVersion()));
		} else if ("kernel_version".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setKernelVersion(dateString);
			//Log.d("kernel_version=",
					//String.valueOf(mInfo.get(pkgCount).getKernelVersion()));
		} else if ("build_number".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setBuildNumber(dateString);
			//Log.d("build_version=",
					//String.valueOf(mInfo.get(pkgCount).getBuildNumber()));
		} else if ("boot_version".equals(preTAG)) {
			dateString = new String(ch, start, length);
			mInfo.get(pkgCount).setBootVersion(dateString);
			//Log.d("boot_version=",
					//String.valueOf(mInfo.get(pkgCount).getBootVersion()));
		}

		super.characters(ch, start, length);
	}

	public ArrayList<VersionInfo> getVersionInfo() {
		return mInfo;
	}

	public void setVersionInfo(ArrayList<VersionInfo> info) {
		mInfo = info;
	}

}
