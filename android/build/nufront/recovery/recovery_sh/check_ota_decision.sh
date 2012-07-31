#!/system/bin/sh

cd /tmp
if [ -d "signed" ];then
	rm -rf signed
fi
mkdir ./signed
umount mount /dev/block/mmcblk1p7
mount /dev/block/mmcblk1p7 ./signed
ret=$?
if [ $ret -ne 0 ];then
	echo "check ota decison mount failed"
	exit 0
fi

cd ./signed
reboot=
if [ ! -e boot_config ];then
	echo "check ota decison no found boot_config"
	exit 0
else
	#set boot system as android
	echo "check ota decison start check ..."
	system=`cat boot_config |grep "OTA"`
	flag=`echo $system |grep "yes"`
#	if [ "$flag" ];then
#		if [ -e tempfile ];then
#			rm tempfile
#		fi
#		sed '/OTA/c\OTA decide = no;' boot_config >tempfile
#		rm boot_config
#		mv tempfile boot_config
#	fi
	echo "check ota decison check end, ret is [$flag]"
	
fi
cd ..
umount ./signed
ret=$?
if [ $ret -eq 0 ];then
	rm -rf ./signed
fi

if [ "$flag" ];then
	#add this for realize the action that copy update.zip to /tmp/sideload which original realizition in the recovery.c
	if [ ! -e /tmp/reboot_temp ];then
		echo "check ota decison start copy update.zip"
		cd /tmp/
		if [ -d "copy" ];then
			rm -rf copy
		fi
		mkdir copy	
		umount mount /dev/block/mmcblk1p1
		mount /dev/block/mmcblk1p1 ./copy
		
		umount mount /dev/block/mmcblk1p5
		mount /dev/block/mmcblk1p5 /data

		if [ -d "/data/sideload" ];then
			rm -rf /data/sideload
		fi

		mkdir /data/sideload
		cp -ar /tmp/copy/Download/update.zip /data/sideload/package.zip
	#	rm -rf /tmp/copy/Download/update.zip
		sync
		umount ./copy
		umount /data
	fi
	echo "check ota decison all ok,found ota"
	exit 1
else
	echo "check ota decison all ok,no need ota"
	exit 0
fi

cd ../
