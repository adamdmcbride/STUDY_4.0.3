#!/system/bin/sh


cd /tmp/restore
if [ ! -d ./emmc ];then
	mkdir ./emmc
fi

mount /dev/block/mmcblk$2p5 ./emmc
ret=$?
if [ -e ./restore.log ];then
	rm ./restore.log
fi
if [ $ret -ne 0 ];then
	echo "mount data partition faied when restore data.img">>/tmp/restore/restore.log
	exit 1
fi
rm -rf ./emmc/*
cd ./emmc
gunzip -c ../nandroid/$1/data.img | cpio -i
ret=$?
if [ $ret -ne 0 ];then
	echo "restore data.img failed">>/tmp/restore/restore.log
fi

cd ../
umount ./emmc

exit $ret




