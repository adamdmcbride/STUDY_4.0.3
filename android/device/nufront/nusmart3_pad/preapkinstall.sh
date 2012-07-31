#!/system/bin/busybox sh

echo "do preinstall job"
BUSYBOX="/system/bin/busybox"

if [ ! -e /data/misc/.pre_apk ]; then		
	/system/bin/sh /system/bin/pm preapk	
fi

echo "preinstall ok"

