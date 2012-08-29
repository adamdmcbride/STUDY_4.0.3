# Environment variable
export ROOT_DIR=${PWD}
export ANDROID_DIR=${ROOT_DIR}/android
export KERNEL_DIR=${ROOT_DIR}/linux_kernel
export UBOOT_DIR=${ROOT_DIR}/nusmart3_uboot
export RELEASE_DIR=${ROOT_DIR}/release

export ARCH=arm
#export CROSS_COMPILE=${ANDROID_DIR}/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
#export CROSS_COMPILE=/mnt/backup/common/arm-2009q3/bin/arm-none-linux-gnueabi-
# for kernel
export PATH=$PATH:/mnt/backup/common/renesas/arm-2009q3/bin
# for uboot
export PATH=$PATH:/mnt/backup/common/arm-2009q1-none-eabi/bin

#JDK1.6
export JAVA_HOME=/usr/lib/jvm/java-6-sun
export PATH=$JAVA_HOME/bin:$PATH
export CLASSPATH=$JAVA_HOME/lib.tools.jar

# android lunch select
cd ${ANDROID_DIR}
source build/envsetup.sh
if [ "$1" == "user" ]; then
	lunch full_nusmart3_pad-user
else
	lunch full_nusmart3_pad-eng
fi
cd -

# ccache set
export USE_CCACHE=1
export CCACHE_DIR=${ROOT_DIR}/release/.ccache
${ANDROID_DIR}/prebuilt/linux-x86/ccache/ccache -M 5G

