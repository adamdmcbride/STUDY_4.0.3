#!/system/bin/sh

#mount -t vfat -o rw,dirsync,nosuid,nodev,noexec,relatime,uid=1000,gid=1015,fmask=0002,dmask=0002,allow_utime=0020,codepage=cp437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro /dev/block/mmcblk1p1 /mnt/sdcard

/system/bin/boot_log &

#logcat -b radio -v time -r 2000 -n 3 -f /mnt/sdcard/log/logcat-radio.txt &
#logcat -v time -r 1500 -n 3 -f /mnt/sdcard/log/logcat.txt &

