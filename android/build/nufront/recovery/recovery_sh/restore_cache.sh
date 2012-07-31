#!/system/bin/sh


cd /tmp/restore
if [ ! -d ./emmc ];then
	mkdir ./emmc
fi

mount /dev/block/mmcblk$2p6 ./emmc
ret=$?
if [ -e ./restore.log ];then
	rm ./restore.log
fi
if [ $ret -ne 0 ];then
	echo "mount cache partition faied when restore cache.img">>/tmp/restore/restore.log
	exit 1
fi
rm -rf ./emmc/*
cd ./emmc
gunzip -c ../nandroid/$1/cache.img | cpio -i
ret=$?
if [ $ret -ne 0 ];then
	echo "restore cache.img failed">>/tmp/restore/restore.log
fi

cd ../
umount ./emmc

exit $ret




