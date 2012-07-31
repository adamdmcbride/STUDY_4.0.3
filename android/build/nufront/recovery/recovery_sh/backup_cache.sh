#!/system/bin/sh

#######################################################################
# Author: Bai Nianfu
# Date: 10/27/2011
# Version: debug 1.0.0
######################################################################
#return value
#0 -- make cache.img succeed
#1 -- mount sdcard partition failed when making cache.img
#2 -- mount cache partition failed when making cache.img
#3 -- make cache.img failed
######################################################################


#mount sdcard partition
echo "start procesing cache image">>/tmp/backup/backuplog
name=$(awk '{if($0 ~/image/ && $2 ~/storage/ && $3 ~/name/ && $4 ~/is/) print $5}' /tmp/backup/backuplog)
echo "get image name is $name">>/tmp/backup/backuplog
if [ -d /tmp/backup/sdcard ];then
	rm -rf /tmp/backup/sdcard
fi
mkdir -p /tmp/backup/sdcard
umount /dev/block/mmcblk$1p1
mount /dev/block/mmcblk$1p1 /tmp/backup/sdcard
ret=$?
if [ $ret -ne 0 ];then
	echo "mount sdcard partition failed when making cache.img">>/tmp/backup/backuplog
	rm -rf /tmp/backup/mmc
	exit 1
fi
echo "mount sdcard partition end">>/tmp/backup/backuplog

cd /tmp/backup/sdcard
if [ ! -d ./nandroid/$name ];then
	mkdir -p ./nandroid/$name
fi
cd ./nandroid/$name
if [ -e cache.img ];then
	rm cache.img
fi
cd /tmp
cd ../ 

#mount cache partition
if [ -d /tmp/backup/emmc ];then
	rm -rf /tmp/backup/emmc
fi
mkdir -p /tmp/backup/emmc
umount /dev/block/mmcblk$1p6
mount /dev/block/mmcblk$1p6 /tmp/backup/emmc
ret=$?
if [ $ret -ne 0 ];then
	echo "mount cache partition failed when making cache.img">>/tmp/backup/backuplog
	cd /tmp
	cd ../
	rm -rf /tmp/backup/emmc
	umount /tmp/backup/sdcard
	exit 2
fi
echo "mount cache partition end">>/tmp/backup/backuplog

echo "start making cache image">>/tmp/backup/backuplog
cd /tmp/backup/emmc
find . -print0 | cpio -0 -H newc -ov | gzip -c >../sdcard/nandroid/$name/cache.img
ret=$?
if [ $ret -ne 0 ];then
	cd /tmp
	cd ../
	umount /tmp/backup/sdcard
	umount /tmp/backup/emmc
	echo "make cache.img failed">>/tmp/backup/backuplog
	exit 3
fi
cd /tmp
cd ../
umount /tmp/backup/sdcard
umount /tmp/backup/emmc

echo "make cache.img succeed">>/tmp/backup/backuplog

exit 0
