#!/system/bin/sh
echo "start find power config..."

umount /dev/block/mmcblk1p5
mount /dev/block/mmcblk1p5 /data
if [ ! -e /data/sideload/package.zip ];then
	umount /dev/block/mmcblk1p5
	rm -rf /tmp/power
	rm -rf ./pt
	echo "find power config, cannot find /tmp/sideload/update.zip"
	exit 1
fi

cd /tmp
if [ -d power ];then
	rm -rf ./power
fi
mkdir ./power

if [ -d pt ];then
	rm -rf ./pt
fi
mkdir ./pt

#move file to /tmp/power/
unzip /data/sideload/package.zip -d ./pt/
if [ ! -e ./pt/power_config.txt ];then
	rm -rf ./power
	rm -rf ./pt
	umount /data
	echo "find power config, cannot find power_config.txt in update.zip"
	exit 1
fi

cp -ar ./pt/power_config.txt ./power/
rm -rf ./pt
umount /data
echo "find power config OK,the end!"
exit 0
