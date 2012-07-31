#!/system/bin/sh

umount /tmp/restore
ret=$?
if [ $ret -ne 0 ];then
	exit 1
fi

if [ -d /tmp/restore ];then
	rm -rf  /tmp/restore
fi

exit 0





