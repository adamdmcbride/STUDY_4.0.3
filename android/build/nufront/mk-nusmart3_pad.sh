
source ./build/envsetup.sh

#choosecombo
#choosecombo 1 1 armboard_v7a eng
lunch full_nusmart3_pad-eng
#make PRODUCT-armboard_v7a-eng -j64

make -j64

if [ $? -eq 0 ] ; then
    ./build/nufront/make.fs.android.sh
else
	echo "ERROR: build project failed"
fi

