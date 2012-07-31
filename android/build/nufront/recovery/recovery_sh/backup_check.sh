#!/system/bin/sh
######################################################################
# Function: check sdcard space
#######################################################################
# Author: Bai Nianfu
# Date: 10/20/2011
# Version: debug 1.0.0
# Modification:
######################################################################
#return value
#0 -- sdcard space is enough
#1 -- mount sdcard failed
#2 -- mount boot partition failed
#3 -- mount system partition failed
#4 -- mount data partition failed
#5 -- mount cache partition failed
#6 -- mount misc partition failed
# return value large than 10 indicates that sdcard needs more than return 
# value space
######################################################################

umountremove()
{
	umount $1
	ret=$?
	if [ $ret -eq 0 ];then
		rm -rf $1
	fi
}

removetemp()
{
	case $1 in
		2)
			umountremove /tmp/backup/mmc1
			;;
		3)
			umountremove /tmp/backup/mmc1
			umountremove /tmp/backup/mmc2
			;;
		4)
			umountremove /tmp/backup/mmc1
			umountremove /tmp/backup/mmc2
			umountremove /tmp/backup/mmc3
			;;
		5)
			umountremove /tmp/backup/mmc1
			umountremove /tmp/backup/mmc2
			umountremove /tmp/backup/mmc3
			umountremove /tmp/backup/mmc5
			;;
		6)
			umountremove /tmp/backup/mmc1
			umountremove /tmp/backup/mmc2
			umountremove /tmp/backup/mmc3
			umountremove /tmp/backup/mmc5
			umountremove /tmp/backup/mmc6
			;;
		7)
			umountremove /tmp/backup/mmc1
			umountremove /tmp/backup/mmc2
			umountremove /tmp/backup/mmc3
			umountremove /tmp/backup/mmc5
			umountremove /tmp/backup/mmc6
			umountremove /tmp/backup/mmc7
			;;
	esac
}

calsize()
{
	strsize=""
	kunit=1024
	case $1 in
		1)
			#sdcard
			strsize=$(awk '{if($0 ~/\/dev\/block\/'mmcblk$2p1'/) print $4}' /tmp/backup/backuplog)
			echo "sdcard free size is $strsize">>/tmp/backup/backuplog	
			;;
		2)
			#boot
			strsize=$(awk '{if($0 ~/\/dev\/block\/'mmcblk$2p2'/) print $3}' /tmp/backup/backuplog)	
			echo "boot partition size is $strsize">>/tmp/backup/backuplog	
			;;
		3)
			#system
			strsize=$(awk '{if($0 ~/\/dev\/block\/'mmcblk$2p3'/) print $3}' /tmp/backup/backuplog)	
			echo "system partition size is $strsize">>/tmp/backup/backuplog	
			;;
		5)
			#data
			strsize=$(awk '{if($0 ~/\/dev\/block\/'mmcblk$2p5'/) print $3}' /tmp/backup/backuplog)	
			echo "data partition size is $strsize">>/tmp/backup/backuplog	
			;;
		6)
			#cache
			strsize=$(awk '{if($0 ~/\/dev\/block\/'mmcblk$2p6'/) print $3}' /tmp/backup/backuplog)	
			echo "cache partition size is $strsize">>/tmp/backup/backuplog	
			;;
		7)
			#misc
			strsize=$(awk '{if($0 ~/\/dev\/block\/'mmcblk$2p7'/) print $4}' /tmp/backup/backuplog)	
			echo "misc partition size is $strsize">>/tmp/backup/backuplog	
	esac

	echo "strsize is $strsize">>/tmp/backup/backuplog
	unit=`echo $strsize | grep "G"`
	if [ "$unit" ];then
		sizeg=`echo $strsize|sed 's/G//g'`
		sizetmp=$sizeg

		#for the bash dose not support floating point arithmetic, so we have to use awk or bc
		#but busybox does not support bc, so we have to use awk
		size=$(awk 'BEGIN{print '$sizetmp'*'$kunit'}')
	else
		unit=`echo $strsize | grep "M"`
		if [ "$unit" ];then
			sizeg=`echo $strsize|sed 's/M//g'`
			size=$sizeg
		else
			size=1
		fi
	fi

	echo "size is $size before process">>/tmp/backup/backuplog
	#return value must be int type
	size=`echo $size | awk '{print sprintf("%d", $0);}'`
	
	echo "return size is $size">>/tmp/backup/backuplog
	return $size	
}

if [ -d /tmp/backup ];then
	rm -rf /tmp/backup
fi

mkdir -p /tmp/backup/mmc1
umount /dev/block/mmcblk$1p1
mount /dev/block/mmcblk$1p1 /tmp/backup/mmc1
ret=$?
echo "mount sdcard partition return value is $ret">>/tmp/backup/backuplog
if [ $ret -ne 0 ];then
	removetemp 1
	exit 1
fi

cd /tmp/backup
if [ -e /tmp/backup/backuplog ];then
	rm /tmp/backup/backuplog
fi

echo "start time is as follow:">/tmp/backup/backuplog
echo $(date +%Y"."%m"."%d"--"%k":"%M":"%S)>>/tmp/backup/backuplog
echo "mount sdcard return value is $ret">>/tmp/backup/backuplog


mkdir -p /tmp/backup/mmc2
umount /dev/block/mmcblk$1p2
mount /dev/block/mmcblk$1p2 /tmp/backup/mmc2
ret=$?
echo "mount boot partition return value is $ret">>/tmp/backup/backuplog
if [ $ret -ne 0 ];then
	removetemp 2
	exit 2
fi


mkdir -p /tmp/backup/mmc3
umount /dev/block/mmcblk$1p3
mount /dev/block/mmcblk$1p3 /tmp/backup/mmc3
ret=$?
echo "mount system partition return value is $ret">>/tmp/backup/backuplog
if [ $ret -ne 0 ];then
	removetemp 3
	exit 3
fi


mkdir -p /tmp/backup/mmc5
umount /dev/block/mmcblk$1p5
mount /dev/block/mmcblk$1p5 /tmp/backup/mmc5
ret=$?
echo "mount data partition return value is $ret">>/tmp/backup/backuplog
if [ $ret -ne 0 ];then
	removetemp 4
	exit 4
fi


mkdir -p /tmp/backup/mmc6
umount /dev/block/mmcblk$1p6
mount /dev/block/mmcblk$1p6 /tmp/backup/mmc6
ret=$?
echo "mount cache partition return value is $ret">>/tmp/backup/backuplog
if [ $ret -ne 0 ];then
	removetemp 5
	exit 5
fi


mkdir -p /tmp/backup/mmc7
umount /dev/block/mmcblk$1p7
mount /dev/block/mmcblk$1p7 /tmp/backup/mmc7
ret=$?
echo "mount misc partition return value is $ret">>/tmp/backup/backuplog
if [ $ret -ne 0 ];then
	removetemp 6
	exit 6
fi

df -h >>/tmp/backup/backuplog

#calculate sdcard free size
calsize 1 $1
sdsize=$?
echo "sdsize is $sdsize">>/tmp/backup/backuplog

#calculate boot partition size
calsize 2 $1
bootsize=$?
echo "boot size is $bootsize">>/tmp/backup/backuplog

#system size
calsize 3 $1
systemsize=$?
echo "system size is $systemsize">>/tmp/backup/backuplog

#data size
calsize 5 $1
datasize=$?
echo "data size is $datasize">>/tmp/backup/backuplog

#cache size
calsize 6 $1
cachesize=$?
echo "cache size is $cachesize">>/tmp/backup/backuplog

#misc size
calsize 7 $1
miscsize=$?
echo "misc size is $miscsize">>/tmp/backup/backuplog

imgsize=$(($bootsize+$systemsize+$datasize+$cachesize+$miscsize))
echo "total image size is $imgsize">>/tmp/backup/backuplog

#check whether sdcard free space is enough
safesize=110
sdsize=$(($sdsize-$safesize))
if [ $imgsize -gt $sdsize ];then
	val=$(($imgsize-$sdsize))
	if [ $val -ge 10 ];then
		echo "sdcard does not has enough space">>/tmp/backup/backuplog
		removetemp 6
		exit $val	
	fi
else
	echo "sdcard free space is enough">>/tmp/backup/backuplog
fi

freespace=$(($sdsize-$imgsize))
echo "free sdcard size is $freespace M bytes">>/tmp/backup/backuplog


if [ ! -d /tmp/backup/mmc1/nandroid ];then
	mkdir -p /tmp/backup/mmc1/nandroid
fi

name=$(date +%Y"-"%m"-"%d"---"%H"."%M"."%S)
echo "image storage name is $name">>/tmp/backup/backuplog
mkdir -p /tmp/backup/mmc1/nandroid/$name
umountremove /tmp/backup/mmc1
umountremove /tmp/backup/mmc2
umountremove /tmp/backup/mmc3
umountremove /tmp/backup/mmc5
umountremove /tmp/backup/mmc6
umountremove /tmp/backup/mmc7
echo "check sdcard space end">>/tmp/backup/backuplog
cd ../../

exit 0
