#!/system/bin/sh

umount /dev/block/mmcblk1p1
mount /dev/block/mmcblk1p1 /tmp
ret=$?
if [ $ret -ne 0 ];then
	exit 1
fi

if [ ! -e /tmp/EC.BIN ];then
	umount /tmp
	exit 2
fi

cp /tmp/EC.BIN /bin/
umount /tmp
cd /bin/
chmod 777 FlashTool.out
./FlashTool.out

exit 0
 


