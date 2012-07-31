package com.android.ota;

public class VersionInfo {

	private String packageVersion;
	private String baseVersion;
	private String releaseNotes;
	private String fileName;
	private String downloadUrl;
	private String fileMd5;
	private long fileSize;
	private String modelNumber;
	private String androidVersion;
	private String basebandVersion;
	private String kernelVersion;
	private String buildNumber;
	private String bootVersion;

	public VersionInfo() {
		packageVersion = null;
		baseVersion = null;
		releaseNotes = null;
		fileName = null;
		downloadUrl = null;
		fileMd5 = null;
		fileSize = 0L;
		modelNumber = null;
		kernelVersion = null;
		buildNumber = null;
		androidVersion = null;
		basebandVersion = null;
		bootVersion = null;
	}

	public void setPackageVersion(String s) {
		packageVersion = s;
	}

	public void setName(String name) {
		fileName = name;
	}

	public void setUrl(String url) {
		downloadUrl = url;
	}

	public void setMd5(String md5) {
		fileMd5 = md5;
	}

	public void setFileSize(long size) {
		fileSize = size;
	}

	public void setModelNumber(String s) {
		modelNumber = s;
	}

	public void setKernelVersion(String s) {
		kernelVersion = s;
	}

	public void setBuildNumber(String s) {
		buildNumber = s;
	}

	public void setAndroidVersion(String s) {
		androidVersion = s;
	}

	public void setBasebandVersion(String s) {
		basebandVersion = s;
	}

	public void setBootVersion(String s) {
		bootVersion = s;
	}

	public void setBaseVersion(String s) {
		baseVersion = s;
	}

	public void setReleaseNotes(String s) {
		releaseNotes = s;
	}

	public String getPackageVersion() {
		return packageVersion;
	}

	public String getName() {
		return fileName;
	}

	public String getUrl() {
		return downloadUrl;
	}

	public String getMd5() {
		return fileMd5;
	}

	public long getFileSize() {
		return fileSize;
	}

	public String getModelNumber() {
		return modelNumber;
	}

	public String getKernelVersion() {
		return kernelVersion;
	}

	public String getBuildNumber() {
		return buildNumber;
	}

	public String getAndroidVersion() {
		return androidVersion;
	}

	public String getBasebandVersion() {
		return basebandVersion;
	}

	public String getBootVersion() {
		return bootVersion;
	}

	public String getBaseVersion() {
		return baseVersion;
	}

	public String getReleaseNotes() {
		return releaseNotes;
	}
}
