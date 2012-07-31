#!/system/bin/sh

#######################################################################
# Author: Bai Nianfu
# Date: 10/27/2011
# Version: debug 1.0.0
######################################################################
#return value
#0 -- make data.img succeed
#1 -- mount sdcard partition failed when making data.img
#2 -- mount data partition failed when making data.img
#3 -- make data.img failed
######################################################################


#mount sdcard partition
echo "start procesing data image">>/tmp/backup/backuplog
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
	echo "mount sdcard partition failed when making data.img">>/tmp/backup/backuplog
	rm -rf /tmp/backup/mmc
	exit 1
fi
echo "mount sdcard partition end">>/tmp/backup/backuplog

cd /tmp/backup/sdcard
if [ ! -d ./nandroid/$name ];then
	mkdir -p ./nandroid/$name
fi
cd ./nandroid/$name
if [ -e data.img ];then
	rm data.img
fi
cd /tmp
cd ../ 

#mount data partition
if [ -d /tmp/backup/emmc ];then
	rm -rf /tmp/backup/emmc
fi
mkdir -p /tmp/backup/emmc
umount /dev/block/mmcblk$1p5
mount /dev/block/mmcblk$1p5 /tmp/backup/emmc
ret=$?
if [ $ret -ne 0 ];then
	echo "mount data partition failed when making data.img">>/tmp/backup/backuplog
	cd /tmp
	cd ../
	rm -rf /tmp/backup/emmc
	umount /tmp/backup/sdcard
	exit 2
fi
echo "mount data partition end">>/tmp/backup/backuplog

echo "start making data image">>/tmp/backup/backuplog
cd /tmp/backup/emmc
find . -print0 | cpio -0 -H newc -ov | gzip -c >../sdcard/nandroid/$name/data.img
ret=$?
if [ $ret -ne 0 ];then
	cd /tmp
	cd ../
	umount /tmp/backup/sdcard
	umount /tmp/backup/emmc
	echo "make data.img failed">>/tmp/backup/backuplog
	exit 3
fi
cd /tmp
cd ../
umount /tmp/backup/sdcard
umount /tmp/backup/emmc

echo "make data.img succeed">>/tmp/backup/backuplog

exit 0
