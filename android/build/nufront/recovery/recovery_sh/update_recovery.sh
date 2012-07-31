#!/system/bin/sh

#######################################################################
# Author: Bai Nianfu
# Date: 10/20/2011
# Version: 1.0.1
# Modification:
# 1. Changed image file format(24/11/2011, Bai Nianfu);

##########################################################################
# return value:
# 1 -- recovery boot from emmc
# 2 -- mount usb device failed
# 3 -- update_recovery.img is not existence
# 4 -- mount emmc recovery partition failed
# 5 -- mount update_recovery.img failed
##########################################################################
removetmp()
{
	cd /tmp/recovery
	if [ -d ./loop ];then
		umount ./loop
		ret=$?
		if [ $ret -eq 0 ];then
			rm -rf ./loop
		fi
	fi

	if [ -d ./sda1 ];then
		umount ./sda1
		ret=$?
		if [ $ret -eq 0 ];then
			rm -rf ./sda1
		fi
	fi

	if [ -d ./mmc7 ];then
		umount ./mmc7
		ret=$?
		if [ $ret -eq 0 ];then
			rm -rf ./mmc7
		fi
	fi
}
	


bootargs=`cat /proc/cmdline | grep mmcblk0p`
if [ "$bootargs" ];then
	exit 1
fi

#mount usb first partition
cd /tmp
if [ -d ./recovery ];then
	rm -rf ./recovery
fi
mkdir ./recovery
cd ./recovery
mkdir ./sda1
mount /dev/block/sda1 ./sda1
ret=$?
if [ $ret -ne 0 ];then
	rm -rf ./sda1
	removetmp
	exit 2
fi

cd ./sda1
if [ ! -e update_recovery.img ];then
	removetmp
	exit 3
fi
cd ..

#process recovery partition
mkdir ./mmc7
umount /dev/block/mmcblk0p7
mount /dev/block/mmcblk0p7 ./mmc7
ret=$?
if [ $ret -ne 0 ];then
	rm -rf ./mmc7
	removetmp
	exit 4	
fi


#process update_recovery.img
mkdir ./loop
cd ./loop
gunzip -c ../sda1/update_recovery.img | cpio -i
ret=$?
if [ $ret -ne 0 ];then
	rm -rf ./loop
	removetmp
	exit 5	
fi
cd ../

#install recovery file system
rm -rf ./mmc7/*
cp -ra ./loop/* ./mmc7


#remove temp file
removetmp


exit 0


