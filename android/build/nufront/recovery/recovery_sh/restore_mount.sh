#!/system/bin/sh

if [ -d /tmp/restore ];then
	rm -rf  /tmp/restore
fi
mkdir -p /tmp/restore
umount /dev/block/mmcblk$1p1
mount /dev/block/mmcblk$1p1 /tmp/restore
ret=$?
if [ $ret -ne 0 ];then
	exit 1
fi

exit 0





