#!/system/bin/sh

boot=`cat /proc/cmdline | grep mmcblk0`
if [ "$boot" ];then
	umount /dev/block/mmcblk1p1
	mount /dev/block/mmcblk1p1 /tmp
	ret=$?
	if [ $ret -ne 0 ];then
		echo "mount mmcblk1p1 failed"
		exit 1
	fi
else
	umount /dev/block/mmcblk0p1
	mount /dev/block/mmcblk0p1 /tmp
	ret=$?
	if [ $ret -ne 0 ];then
		echo "mount /dev/block/mmcblk0p1 failed"
		exit 1
	fi
fi


case $1 in
	0)
		if [ ! -e /tmp/u-boot.bin ];then
			umount /tmp
			exit 2
		fi
		echo "program ns2816 uboot now......"
		dwsupd uboot /tmp/u-boot.bin
		;;
	1)
		if [ ! -e /tmp/u-boot.img ];then
			umount /tmp
			exit 2
		fi
		echo "program ns115 uboot now......"
		if [ "$boot" ];then
			dd if=/tmp/u-boot.img of=/dev/block/mmcblk0 seek=4608 bs=512
		else
			dd if=/tmp/u-boot.img of=/dev/block/mmcblk1 seek=4608 bs=512
		fi
		if [ $? -ne 0 ];then
			echo "Error:cannot find u-boot.img, dd u-boot failed"
			umount /tmp
			exit 3
		fi
		;;
	*)
		echo "Error:unknown command number!"
		umount /tmp
		exit 4
		;;
esac

umount /tmp

exit 0
 


