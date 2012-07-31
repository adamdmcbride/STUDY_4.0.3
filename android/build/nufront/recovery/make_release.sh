#!/bin/bash
if [ "$FSDROID" = "" ] ; then
    echo "please exec make.fs.android.sh first"
    exit 1
fi

nusmart2_smp=`echo $FSDROID |grep "nusmart2_smp"`
nusmart3_pad=`echo $FSDROID |grep "nusmart3_pad"`
if [ "$nusmart3_pad" ];then
echo $(pwd)
        cp ./modules/*.ko ./fsdroid-full_nusmart3_pad/system/lib/modules/

fi

if [ "$nusmart2_smp" ];then
	echo "start releasing  ns2816 file system"
	#make fsrecovery
	if [ -d ./fsrecovery ];then
		rm -rf ./fsrecovery
	fi

	mkdir ./fsrecovery
	cd ./fsrecovery
	mkdir ./bin
	mkdir -p ./bin/ns2816
	cd ../
	cp -ra ${ANDROID_PRODUCT_OUT}/recovery/root/* ./fsrecovery
	cp -ra ./${FSDROID}/system/bin ./fsrecovery/system
	cp -ra ./${FSDROID}/system/xbin ./fsrecovery/system
	cp -ra ./${FSDROID}/system/lib ./fsrecovery/system

	#copy busybox tools
	cp -ra ./build/nufront/recovery/busybox/bin/* ./fsrecovery/bin
	cp -ra ./build/nufront/recovery/busybox/sbin/* ./fsrecovery/sbin
	cp -ra ./build/nufront/recovery/busybox/usr/ ./fsrecovery

	#copy mkfs tools
	cp ./${FSDROID}/system/bin/mke2fs ./fsrecovery/sbin
	cp ./fsrecovery/sbin/mke2fs ./fsrecovery/sbin/mkfs.ext2
	cp ./fsrecovery/sbin/mke2fs ./fsrecovery/sbin/mkfs.ext3

	#copy relative script
	cp ./build/nufront/recovery/recovery_sh/* ./fsrecovery/bin/ns2816
	cp ./build/nufront/recovery/FlashTool.out ./fsrecovery/bin/
	cp ./build/nufront/recovery/dwsupd ./fsrecovery/bin/


	#copy init.rc, vold, recovery.fstab
	cp ./build/nufront/recovery/init.rc ./fsrecovery
	cp ./build/nufront/recovery/recovery.fstab ./fsrecovery/etc

	if [ -d ./modules ];then
		cp -ra ./modules ./${FSDROID}
	fi

	if [ -d ./tempfs ];then
		rm -rf ./tempfs
	fi
	mkdir ./tempfs
	cp -ra ./${FSDROID} ./tempfs
	mv ./fsrecovery ./tempfs/
	cp ./build/nufront/recovery/make.package.sh ./tempfs
	cd ./tempfs
	mv ./fsrecovery ./recovery
	./make.package.sh
	cp ../../linux_kernel/arch/arm/boot/uImage ./boot
	cp ../../WIFI_BT_NM387_8787/bin/*.ko ./system/lib/modules/
	cp ../../WIFI_BT_NM387_8787/bin/*.bin ./system/etc/firmware/mrvl/
	cp ./boot/uImage ./recovery/uImage_recovery
	cp -af ../${FSDROID}/init.*.rc ./boot/
	rm -rf ./make.package.sh


	TIME=`date +%Y%m%d%H%M`
	FSNAME=${FSDROID}.${TIME}.tgz
	tar czf ${FSNAME} *
	mv ./${FSDROID}* ../
	cd ..
else
	echo "start releasing ns115 image files, including boot.img.ext4, system.img.ext4, recovery.img.ext4"
	TIME=`date +%Y%m%d%H%M`
	FSNAME=release.multi.partition.${FSDROID}.${TIME}
	mkdir ./${FSNAME}
	cp ${ANDROID_PRODUCT_OUT}/boot.img ${FSNAME}
	cp ${ANDROID_PRODUCT_OUT}/system.img ${FSNAME}
	cp ${ANDROID_PRODUCT_OUT}/recovery.img ${FSNAME}
	./out/host/linux-x86/bin/simg2img ${FSNAME}/boot.img ${FSNAME}/boot.img.ext4
	./out/host/linux-x86/bin/simg2img ${FSNAME}/system.img ${FSNAME}/system.img.ext4
	./out/host/linux-x86/bin/simg2img ${FSNAME}/recovery.img ${FSNAME}/recovery.img.ext4
	rm ${FSNAME}/boot.img
	rm ${FSNAME}/system.img
	rm ${FSNAME}/recovery.img
fi

echo "release file system completely"
