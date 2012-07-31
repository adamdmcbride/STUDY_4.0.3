#!/bin/bash
if [ "$TARGET_PRODUCT" = "" ] ; then
    echo "please do lunch"
    exit 0
fi

make -j64 dist

TIME=`date +%Y%m%d%H%M`
DIST_DIR=dist.${TARGET_PRODUCT}.${TIME}

if [ -d ./${DIST_DIR} ] ;then
    rm -rf ${DIST_DIR}
fi

mkdir -p ${DIST_DIR}

./build/tools/releasetools/sign_target_files_apks --extra_apks_dir system/etc/.pre_apk,system/vendor= -d build/nufront/security out/dist/*target_files*.zip ${DIST_DIR}/signed-target-files.zip

./build/nufront/make.cts.py ${DIST_DIR}/signed-target-files.zip ${DIST_DIR}/signed-cts.zip

./build/tools/releasetools/img_from_target_files ${DIST_DIR}/signed-target-files.zip ${DIST_DIR}/signed-img.zip

