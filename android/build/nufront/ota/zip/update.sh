#!/bin/bash
if [ -e /tfcard/sideload/update.sh ];then
 DATA=/tfcard
fi

if [ -e /sdcard/sideload/update.sh ];then
 DATA=/sdcard
fi

cd /tmp
if [ -d ./boot ];then
	rm -rf ./boot
fi
mkdir ./boot
if [ -d ./system ];then
	rm -rf ./system
fi
mkdir ./system

umount /dev/block/mmcblk1p2
umount /dev/block/mmcblk1p3
mount /dev/block/mmcblk1p2 ./boot
if [ $? -ne 0 ];then
	echo "mount boot failed"
	exit 1
fi
mount /dev/block/mmcblk1p3 ./system
if [ $? -ne 0 ];then
	echo "mount system failed"
	umount ./boot
	exit 1
fi

if [ -e /$DATA/sideload/boot.patch ];then
	cd ./boot
	patch -Np1 -r - </$DATA/sideload/boot.patch
	cd ..
fi

if [ -e /$DATA/sideload/system.patch ];then
	cd ./system
	patch -Np1 -r - </$DATA/sideload/system.patch
	cd ..
fi

umount ./boot
umount ./system
#to update bootloader if exist
if [ -e /$DATA/sideload/x-load.img ];then
	echo "update x-load.img"
	dd if=/$DATA/sideload/x-load.img of=/dev/block/mmcblk1 bs=512 seek=4096
fi

if [ -e /$DATA/sideload/u-boot.img ];then
	echo "update u-boot.img"
	dd if=/$DATA/sideload/u-boot.img of=/dev/block/mmcblk1 bs=512 seek=4608
fi

if [ -e /$DATA/sideload/logo_32bit.bmp ];then
	echo "update logo_32bit.bmp"
	dd if=/$DATA/sideload/logo_32bit.bmp of=/dev/block/mmcblk1 bs=512 seek=5210
fi

if [ -e /$DATA/sideload/logo_16bit.bmp ];then
	echo "update logo_16bit.bmp"
	dd if=/$DATA/sideload/logo_16bit.bmp of=/dev/block/mmcblk1 bs=512 seek=8800
fi
#update end
rm /$DATA/sideload/ -rf
rm -rf ./boot ./system
sync
sleep 1
umount /$DATA
