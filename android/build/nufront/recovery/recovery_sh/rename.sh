#!/system/bin/sh
cd /tmp

echo $1>>rename.log 
echo $2>>rename.log 
mv $1 $2
ret=$?
exit $ret
