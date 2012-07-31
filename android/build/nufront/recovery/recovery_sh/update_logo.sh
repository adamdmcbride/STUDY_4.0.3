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

if [ ! -e /tmp/logo-32bit.bmp ];then
	umount /tmp
	exit 2
fi
if [ ! -e /tmp/logo-16bit.bmp ];then
	umount /tmp
	exit 2
fi

	echo "program ns115 logo now......"
if [ "$boot" ];then
	dd if=/tmp/logo-32bit.bmp of=/dev/block/mmcblk0 seek=5210 bs=512
	if [ $? -ne 0 ];then
		echo "Error: dd logo 32bit failed"
		umount /tmp
		exit 3
	fi
	dd if=/tmp/logo-16bit.bmp of=/dev/block/mmcblk0 seek=8800 bs=512
else
	dd if=/tmp/logo-32bit.bmp of=/dev/block/mmcblk1 seek=5210 bs=512
	if [ $? -ne 0 ];then
		echo "Error: dd logo 32bit failed"
		umount /tmp
		exit 3
	fi
	dd if=/tmp/logo-16bit.bmp of=/dev/block/mmcblk1 seek=8800 bs=512
fi
if [ $? -ne 0 ];then
	echo "Error: dd logo 16bit failed"
	umount /tmp
	exit 3
fi

umount /tmp
exit 0
 
