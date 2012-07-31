#!/system/bin/sh
value1="value=succsee"
value2="value=false"

echo $value2 > /tmp/file.prop

if [ -e /tfcard/update.zip ];then
path="/tfcard/sideload"
packagename=package.zip
cd $path
unzip ./$packagename
ret=$?
if [ $ret -ne 0 ]; then
	rm -rf *.sh
fi

if [ -e /tfcard/sideload/update_rom.sh ];then
cp update_rom.sh /tmp
echo $value1 > /tmp/file.prop
fi

if [ -e /tfcard/sideload/update.sh ];then
cp update.sh /tmp
echo $value1 > /tmp/file.prop
fi

cd -
fi

if [ -e /sdcard/update.zip ];then
path="/sdcard/sideload"
packagename=package.zip
cd $path
unzip ./$packagename
ret=$?
if [ $ret -ne 0 ]; then
	rm -rf *.sh
fi

if [ -e /sdcard/sideload/update_rom.sh ];then
cp update_rom.sh /tmp
echo $value1 > /tmp/file.prop
fi

if [ -e /sdcard/sideload/update.sh ];then
cp update.sh /tmp
echo $value1 > /tmp/file.prop
fi

cd -
fi





