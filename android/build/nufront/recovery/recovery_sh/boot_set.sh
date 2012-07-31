#!/system/bin/sh

cd /tmp
if [ -d "copy" ];then
	rm -rf copy
fi
mkdir ./copy
umount mount /dev/block/mmcblk$1p1
mount /dev/block/mmcblk$1p1 ./copy
rm -rf ./copy/Download/update.zip
umount /dev/block/mmcblk$1p1
rm -rf ./copy

if [ -d "dst" ];then
	rm -rf dst
fi
if [ -e "reboot_temp" ];then
	rm -rf reboot_temp
fi
if [ -d "signed" ];then
	rm -rf signed
fi
mkdir ./signed
umount mount /dev/block/mmcblk$1p7
mount /dev/block/mmcblk$1p7 ./signed
ret=$?
if [ $ret -ne 0 ];then
	exit 1
fi

cd ./signed
reboot=
if [ ! -e boot_config ];then
	echo "Boot system = android;">>boot_config
	#aahadd 20120512 0->2816 else 115
	if [ $1 -eq 0 ];then
		echo "Boot device = emmc;" >>boot_config
	else
		echo "OTA decide = no;" >>boot_config
	fi
	echo "Clear data = no;" >>boot_config
	echo "Clear cache = no;" >>boot_config
else
	#aah modify 20120519
	if [ $1 -eq 0 ];then
	#start ns2816
		#set boot system as android
		system=`cat boot_config |grep "system"`
		flag=`echo $system |grep "recovery"`
		if [ "$flag" ];then
			if [ -e tempfile ];then
				rm tempfile
			fi
			sed '/system/c\Boot system = android;' boot_config >tempfile
			rm boot_config
			mv tempfile boot_config
		fi	

		#set boot device as emmc
		system=`cat boot_config |grep "device"`
		flag=`echo $system |grep "usb"`
		if [ "$flag" ];then
			if [ -e tempfile ];then
				rm tempfile
			fi
			sed '/device/c\Boot device = emmc;' boot_config >tempfile
			rm boot_config
			mv tempfile boot_config
		fi

		#check clear data flag
		clear=`cat boot_config |grep "data"`
		flag=`echo $clear |grep "yes"`
		if [ "$flag" ];then
			mkfs ext3 /dev/mmcblk$1p5
			mkfs ext3 -L /dev/mmcblk$1p5 data
			if [ -e tempfile ];then
				rm tempfile
			fi
			sed '/data/c\Clear data = no;' boot_config >tempfile
			rm boot_config
			mv tempfile boot_config
			reboot=yes
		fi

		#check clear cache flag	
		clear=`cat boot_config |grep "cache"`
		flag=`echo $clear |grep "yes"`
		if [ "$flag" ];then
			mkfs ext3 /dev/mmcblk$1p6
			mkfs ext3 -L /dev/mmcblk$1p6 cache
			if [ -e tempfile ];then
				rm tempfile
			fi
			sed '/cache/c\Clear cache = no;' boot_config >tempfile
			rm boot_config
			mv tempfile boot_config
			reboot=yes
		fi	
		#ns2816 end
	else
		#start ns115
		#set boot system as android
		system=`cat boot_config |grep "system"`
		if [ -z "$system" ];then
			echo "Boot system = android;" >>boot_config
		else
			flag=`echo $system |grep "recovery"`
			if [ "$flag" ];then
				if [ -e tempfile ];then
					rm tempfile
				fi
				sed '/system/c\Boot system = android;' boot_config >tempfile
				rm boot_config
				mv tempfile boot_config
			fi
		fi

		#check clear data flag
		clear=`cat boot_config |grep "data"`
		if [ -z "$clear" ];then
			echo "Clear data = no;" >>boot_config
		else
			flag=`echo $clear |grep "yes"`
			if [ "$flag" ];then
				mkfs ext3 /dev/mmcblk$1p5
				mkfs ext3 -L /dev/mmcblk$1p5 data
				if [ -e tempfile ];then
					rm tempfile
				fi
				sed '/data/c\Clear data = no;' boot_config >tempfile
				rm boot_config
				mv tempfile boot_config
				reboot=yes
			fi
		fi

		#check clear cache flag	
		clear=`cat boot_config |grep "cache"`
		if [ -z "$clear" ];then
			echo "Clear cache = no;" >>boot_config
		else
			flag=`echo $clear |grep "yes"`
			if [ "$flag" ];then
				mkfs ext3 /dev/mmcblk$1p6
				mkfs ext3 -L /dev/mmcblk$1p6 cache
				if [ -e tempfile ];then
					rm tempfile
				fi
				sed '/cache/c\Clear cache = no;' boot_config >tempfile
				rm boot_config
				mv tempfile boot_config
				reboot=yes
			fi	
		fi

		#check ota decide flag
		clear=`cat boot_config |grep "OTA"`
		if [ -z "$clear" ];then
			echo "OTA decide = no;" >>boot_config
		else
			flag=`echo $clear |grep "yes"`
			if [ "$flag" ];then
				if [ -e tempfile ];then
					rm tempfile
				fi
				sed '/OTA/c\OTA decide = no;' boot_config >tempfile
				rm boot_config
				mv tempfile boot_config
				reboot=yes
			fi
		fi
	fi
    #ns115 end
fi
cd ..
umount ./signed
ret=$?
if [ $ret -eq 0 ];then
	rm -rf ./signed
else
	cd ../
	exit 2
fi

if [ "$reboot" ];then
	/system/bin/reboot
fi

cd ../
