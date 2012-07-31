
source ./build/envsetup.sh

#choosecombo
#choosecombo 1 1 armboard_v7a eng
lunch nusmart2_smp-eng
#make PRODUCT-armboard_v7a-eng -j64

make -j64

if [ $? -eq 0 ] ; then
    ./build/nufront/make.fs.android.sh
fi

