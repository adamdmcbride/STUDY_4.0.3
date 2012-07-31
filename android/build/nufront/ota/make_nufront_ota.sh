#!/bin/bash

help()
{
	sudo echo "usage:"
	sudo echo "     -- sh make_nufront_ota.sh src dst"
	sudo echo "         --src: the folder wait for compare to selected"
	sudo echo "         --dst: the folder selected for compare"
}

echo "start make nufront ota..."

if [ "$1" = "-h" ];then
	help
	exit 1
fi

echo "compare $1 to $2"
if [ ! -d $1 ] || [ ! -d $2 ];then
	echo "folder may not exit"
	exit 1
fi

src1=$1/boot.img.ext4
src2=$1/system.img.ext4
dst1=$2/boot.img.ext4
dst2=$2/system.img.ext4
if [ ! -e $src1 ] || [ ! -e $src2 ] || [ ! -e $dst1 ] || [ ! -e $dst2 ];then
	echo "file may be lost"
	exit 1
fi

if [ -d "nufront_ota" ];then
	rm -rf ./nufront_ota
fi
mkdir ./nufront_ota
cd ./nufront_ota
if [ -e "temp.txt" ];then
	rm temp.txt
fi
rm -rf s1 s2 d1 d2 >temp.txt
mkdir s1 s2 d1 d2
#echo "src1=$src1"
sudo mount $src1 ./s1 -o loop
ret1=$?
sudo mount $src2 ./s2 -o loop
ret2=$?
sudo mount $dst1 ./d1 -o loop
ret3=$?
sudo mount $dst2 ./d2 -o loop
ret4=$?
if [ $ret1 -ne 0 ] || [ $ret2 -ne 0 ] ||[ $ret3 -ne 0 ] ||[ $ret4 -ne 0 ];then
	echo "mount image failed"
	exit 1
fi
echo "mount image OK"
rm ../zip/boot.patch -rf
rm ../zip/system.patch -rf
diff -uNra s1/ d1/ >../zip/boot.patch 
diff -uNra s2/ d2/ >../zip/system.patch 
#cp -ar boot.patch system.patch ../zip/
sudo umount ./s1
sudo umount ./s2
sudo umount ./d1
sudo umount ./d2

cd ../zip/
#rm bootloader
if [ -e x-load.img ];then
	rm x-load.img
fi
if [ -e u-boot.img ];then
	rm u-boot.img
fi
if [ -e logo_32bit.bmp ];then
	rm logo_32bit.bmp
fi
if [ -e logo_16bit.bmp ];then
	rm logo_16bit.bmp
fi

cd ..
if [ -d ./bootloader ];then
	bootload=`ls bootloader/ -l|grep total |grep 0`
	if [ "$bootload" ];then
		echo "no bootloader need to update"
	else
		echo "found bootloader to update"
		cp -ar ./bootloader/* ./zip/
	fi
fi

cd zip/
if [ -e ../sign/unsigned.zip ];then
	rm ../sign/unsigned.zip
fi

zip -qry ../sign/unsigned.zip ./
if [ $? -ne 0 ];then
	echo "make unsigned.zip failed"
	exit 1
fi
rm *.bmp *.img *.patch -rf

cd ../sign/
java -jar signapk.jar -w testkey.x509.pem testkey.pk8 unsigned.zip update.zip
if [ -e ../update.zip ];then
	sudo rm ../update.zip
fi
mv update.zip ../
rm -rf unsigned.zip
cd ..
rm -rf ./nufront_ota
sync

echo "make nufront ota OK!"
