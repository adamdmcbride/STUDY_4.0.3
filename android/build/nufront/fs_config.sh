#!/bin/bash
while read file uid gid mode
do
    if [ -e "$file" ]; then
        sudo chown $uid:$gid $file
        sudo chmod $mode $file
        if [ "sbin" = "$file" ] ;then
            sudo chmod 755 sbin
        fi
    fi
done < META/boot_filesystem_config.txt

while read file uid gid mode
do
    if [ -e "$file" ]; then
        sudo chown $uid:$gid $file
        sudo chmod $mode $file
    fi
done < META/filesystem_config.txt
