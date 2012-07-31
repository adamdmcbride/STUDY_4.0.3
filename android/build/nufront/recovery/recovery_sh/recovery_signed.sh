#!/system/bin/sh

cd /tmp
if [ -d "signed" ];then
	rm -rf signed
fi
mkdir ./signed
umount /dev/block/mmcblk$1p7
mount /dev/block/mmcblk$1p7 ./signed
ret=$?
if [ $ret -ne 0 ];then
	exit 1
fi

cd ./signed
#aahadd 20120512 0->2816 else 115
if [ $1 -eq 0 ];then
	start=""
	storage=`cat /porc/cmdline |grep "sda"`
	if [ "$storage" ];then
		start="usb"
	else
		start="emmc"
	fi

	device="Boot device = "
	device=$device$start
	semicolon=";"
	device=$device$semicolon
fi

if [ -e boot_config ];then
	rm boot_config
fi
echo "Boot system = recovery;">>boot_config
if [ $1 -eq 0 ];then
	echo $device >>boot_config
fi
echo "Clear data = no;" >>boot_config
echo "Clear cache = no;" >>boot_config
cd ..
umount ./signed
ret=$?
if [ $ret -eq 0 ];then
	rm -rf ./signed
else
	cd ../
	exit 2
fi
cd ../
exit 0
