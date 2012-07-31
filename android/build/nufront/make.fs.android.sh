#!/bin/bash
#
#
#
#
#
#
#
#
#############################################################
#
# DO NOT MODIFY THIS FILE !!!!!
#
#############################################################
#
# if any question please feel free contact longjiang.dong@
#
#############################################################
#
#
#
#
#
#
#
#
#
#

echo
echo "=================================================================="
echo making fsdroid ...
echo "=================================================================="
echo

#check the env args.
if [ "$TARGET_PRODUCT" = "" ] ; then
    source build/envsetup.sh
    lunch
fi

export FSDROID=fsdroid-${TARGET_PRODUCT}
if [ -d ./${FSDROID} ] ;then
    rm -rf ${FSDROID}
fi

ROOTSRC="${ANDROID_PRODUCT_OUT}/root"
SYSTEMSRC="${ANDROID_PRODUCT_OUT}/system"
echo "rootsrc is : ${ROOTSRC}"
echo "systemsrc is : ${SYSTEMSRC}"

sleep 2

mkdir -p ./${FSDROID}
rsync -az ${ROOTSRC}/ ${FSDROID}/
rsync -az ${SYSTEMSRC}/ ${FSDROID}/system/

if [ "$TARGET_PRODUCT" != "nusmart2_smp" ] ; then
find ${FSDROID}/init.nufront*.rc|while read file
do
    grep -v "^on fs" $file|grep -v "^    mount rootfs"|grep -v "^    mount ext"> init_rc
    diff -q $file init_rc > /dev/null
    if [ $? -eq 1 ] ;then
        cp -f init_rc $file
    fi
done
rm init_rc
fi

#we can support inputing file name and compress format by args.
TIME=`date +%Y%m%d%H%M`
FSNAME=${TARGET_BUILD_TYPE}.${FSDROID}.${TIME}.tgz

tar zcvf ${FSNAME} ${FSDROID}

./build/nufront/recovery/make_release.sh

