#!/system/bin/sh

if [ $1 -eq 1 ];then
	echo "start mount..."
	echo 0 > /sys/class/android_usb/android0/enable 0
	echo 0x18d1 > /sys/class/android_usb/android0/idVendor
	echo 0x4e21 > /sys/class/android_usb/android0/idProduct
	echo mass_storage > /sys/class/android_usb/android0/functions
	echo "/dev/block/mmcblk1p1" > /sys/class/android_usb/f_mass_storage/lun/file
	echo 1 > /sys/class/android_usb/android0/enable 
	echo "mount ok"
else
	echo "start umount..."
	echo 0 > /sys/class/android_usb/android0/enable 
	echo "umount ok"
fi

exit 0
