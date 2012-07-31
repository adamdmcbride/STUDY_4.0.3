#!/bin/sh

if [ "$FSDROID" = "" ] ; then
    echo "please exec make.fs.android.sh first"
    exit 1
fi
if [ -d ./boot ];then
	rm -rf ./boot
fi

if [ -d ./system ];then
	rm -rf ./system
fi

cp -r ./${FSDROID}/system/ ./
rm -rf ./${FSDROID}/system/*
mv ./${FSDROID} ./boot

