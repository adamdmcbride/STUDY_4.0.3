#!/system/bin/sh

volume=$3
cache="cache"
data="data"
case $volume in
	/cache)
		case $2 in
			ext2)
				mkfs.ext2 $1
				mkfs.ext2 -L $cache $1
				;;
			ext3)
				mkfs.ext3 $1
				mkfs.ext3 -L $cache $1
				;;
			*)
				exit 2
		esac
		;;
	/data)
		case $2 in
			ext2)
				mkfs.ext2 $1
				mkfs.ext2 -L $data $1
				;;
			ext3)
				mkfs.ext3 $1
				mkfs.ext3 -L $data $1
				;;
			*)
				exit 2
		esac
		;;
	*)
		exit 1
esac


exit 0

