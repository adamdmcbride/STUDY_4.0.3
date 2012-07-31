#!/system/bin/sh

 mkdir ./mntfile;
 mount -t ext3 /dev/block/mmcblk$1p2 ./mntfile;
 cd ./mntfile;
 rm -r ./data;
 cd ../;
 umount ./mntfile;
 rm -r ./mntfile;
