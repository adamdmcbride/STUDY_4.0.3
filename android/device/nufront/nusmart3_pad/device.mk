#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

DEVICE_PACKAGE_OVERLAYS := $(LOCAL_PATH)/overlay

PRODUCT_PROPERTY_OVERRIDES := \
	ro.product.locale.language=zh \
	ro.product.locale.region=CN

PRODUCT_PROPERTY_OVERRIDES += \
	ro.opengles.version=131072

PRODUCT_PROPERTY_OVERRIDES += \
	hwui.render_dirty_regions=false

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mass_storage

# Set for recovery compiler
PRODUCT_COPY_FILES += \
    ${KERNEL_DIR}/arch/arm/boot/uImage:kernel \
    $(LOCAL_PATH)/recovery.fstab:recovery.fstab \
    ${KERNEL_DIR}/arch/arm/boot/uImage:uImage \
    ${KERNEL_DIR}/arch/arm/boot/uImage:uImage_recovery \
    $(LOCAL_PATH)/patch:patch \

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    device/nufront/nusmart3_pad/overlay/frameworks/base/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/base/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/base/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    device/nufront/nusmart3_pad/overlay/frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/base/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/base/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml

# add the full apn config
PRODUCT_COPY_FILES += device/sample/etc/apns-full-conf.xml:system/etc/apns-conf.xml

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/vold.fstab:system/etc/vold.fstab

PRODUCT_COPY_FILES += \
    hardware/nufront/libcamera/ns115/media_profiles.xml:system/etc/media_profiles.xml

# Use our keyboard layout, whereby F1 and F10 are menu buttons.
#
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/qwerty.kl:system/usr/keylayout/qwerty.kl \
	$(LOCAL_PATH)/touchscreen.kl:system/usr/keylayout/touchscreen.kl

# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

# Bluetooth config file
PRODUCT_COPY_FILES += \
    system/bluetooth/data/main.nonsmartphone.conf:system/etc/bluetooth/main.conf \
	$(LOCAL_PATH)/bluetooth/nvram.txt:system/etc/bluetooth/nvram.txt


# Init files
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/init.nufront-ns115.rc:root/init.nufront-ns115.rc \
	$(LOCAL_PATH)/init.nufront-ns115-pad-test.rc:root/init.nufront-ns115-pad-test.rc \
	$(LOCAL_PATH)/init.nufront-ns115-pad-ref.rc:root/init.nufront-ns115-pad-ref.rc \
	$(LOCAL_PATH)/init.nufront-ns115.usb.rc:root/init.nufront-ns115.usb.rc \
	$(LOCAL_PATH)/init.nusmart.debug.rc:root/init.nusmart.debug.rc \
	$(LOCAL_PATH)/ueventd.nufront-ns115-pad-test.rc:root/ueventd.nufront-ns115-pad-test.rc \
	$(LOCAL_PATH)/ueventd.nufront-ns115-pad-ref.rc:root/ueventd.nufront-ns115-pad-ref.rc

# idc files for touch/mouse/keyboard
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/HID/Goodix_Capacitive_TouchScreen.idc:system/usr/idc/Goodix_Capacitive_TouchScreen.idc \
	$(LOCAL_PATH)/HID/nusmart-gpio-keys.kcm:system/usr/keychars/nusmart-gpio-keys.kcm \
	$(LOCAL_PATH)/HID/nusmart-gpio-keys.kl:system/usr/keylayout/nusmart-gpio-keys.kl \
	$(LOCAL_PATH)/HID/ricoh583_pwrkey.kcm:system/usr/keychars/ricoh583_pwrkey.kcm \
	$(LOCAL_PATH)/HID/ricoh583_pwrkey.kl:system/usr/keylayout/ricoh583_pwrkey.kl

# Add wifi config by chen.
PRODUCT_COPY_FILES += \
        $(LOCAL_PATH)/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
        $(LOCAL_PATH)/wifi/dhcpcd.conf:system/etc/dhcpcd/dhcpcd.conf \
        $(LOCAL_PATH)/wifi/dhcpcd-mlan0.conf:system/etc/dhcpcd/dhcpcd-mlan0.conf \
        $(LOCAL_PATH)/wifi/wl_4330:system/bin/wl_4330 \
        $(LOCAL_PATH)/wifi/fw_bcmdhd.bin:system/etc/firmware/fw_bcmdhd.bin \
        $(LOCAL_PATH)/wifi/fw_bcmdhd_apsta.bin:system/etc/firmware/fw_bcmdhd_apsta.bin \
        $(LOCAL_PATH)/wifi/fw_bcmdhd_p2p.bin:system/etc/firmware/fw_bcmdhd_p2p.bin \
        $(LOCAL_PATH)/wifi/bcmdhd.cal.40183.37.4MHz:system/etc/firmware/bcmdhd.cal 

# Add wifi nw51/nw53 firmware by chen & lilele.
PRODUCT_COPY_FILES += \
        $(LOCAL_PATH)/wifi/fw_bcm40181a0.bin:system/etc/firmware/fw_bcm40181a0.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40181a0_apsta.bin:system/etc/firmware/fw_bcm40181a0_apsta.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40181a0_mfg.bin:system/etc/firmware/fw_bcm40181a0_mfg.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40181a2.bin:system/etc/firmware/fw_bcm40181a2.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40181a2_apsta.bin:system/etc/firmware/fw_bcm40181a2_apsta.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40181a2_p2p.bin:system/etc/firmware/fw_bcm40181a2_p2p.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40181a2_mfg.bin:system/etc/firmware/fw_bcm40181a2_mfg.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40183b2.bin:system/etc/firmware/fw_bcm40183b2.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40183b2_apsta.bin:system/etc/firmware/fw_bcm40183b2_apsta.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40183b2_p2p.bin:system/etc/firmware/fw_bcm40183b2_p2p.bin \
        $(LOCAL_PATH)/wifi/fw_bcm40183b2_mfg.bin:system/etc/firmware/fw_bcm40183b2_mfg.bin \
        $(LOCAL_PATH)/wifi/bcmdhd.cal.40181:system/etc/firmware/bcmdhd.cal.40181 \
        $(LOCAL_PATH)/wifi/bcmdhd.cal.40183.26MHz:system/etc/firmware/bcmdhd.cal.40183.26MHz \
        $(LOCAL_PATH)/wifi/bcmdhd.cal.40183.37.4MHz:system/etc/firmware/bcmdhd.cal.40183.37.4MHz

#Add IMEI config
PRODUCT_COPY_FILES += $(LOCAL_PATH)/imei.conf:system/etc/imei.conf

#gps astart
PRODUCT_COPY_FILES += \
        hardware/nufront/gst4tq_gb/app/AndroidSharedLib/config/sirfgps.conf:system/etc/gps/sirfgps.conf \
        hardware/nufront/gst4tq_gb/app/AndroidSharedLib/config/csr_gps.conf:system/etc/gps/csr_gps.conf 

# Add apu config by duguiyu
PRODUCT_COPY_FILES += \
        $(LOCAL_PATH)/apu/apu_codecs.bin:system/etc/firmware/apu_codecs.bin \
        $(LOCAL_PATH)/apu/apu_system.bin:system/etc/firmware/apu_system.bin

# hengai 2012-05-21. Remove gps support, N751 does not include gps
##gps so file
#PRODUCT_PACKAGES += \
#        gps.default

PRODUCT_PACKAGES += \
	mke2fs \
	FiDirectDemo \
	Camera

# charger
PRODUCT_PACKAGES += \
    charger \
    charger_res_images

PRODUCT_PACKAGES += \
	LiveWallpapers \
	LiveWallpapersPicker \
	MagicSmokeWallpapers \
	VisualizationWallpapers \
	librs_jni

#nusmart packages
PRODUCT_PACKAGES += \
	camera.default \
	lights.default \
	audio.a2dp.default \
	sensors.default

PRODUCT_PACKAGES += \
	libMali  \
	libUMP   \
	libGLESv1_CM_mali \
	libGLESv2_mali  \
	libEGL_mali

# hwui
PRODUCT_PACKAGES += \
	libhwui

PRODUCT_PACKAGES += \
	libGAL

#vpu modules
PRODUCT_PACKAGES += \
    ffmpeg \
    libavfilter \
    libavutil \
    libswscale \
    libdwlx170 \
    libdecx170h \
    libdecx170m \
    libdecx170m2 \
    libdecx170a \
    libdec8190vp8 \
    libdecx170jpeg \
    libdecx170rv \
    libdecx170v \
    libdec8190vp6 \
    libstagefrighthw \
    libOMX.NU.Audio.Decoder \
    libOMX_Core \
    libOMX.NU.Video.Decoder \
    libh1enc \
    libOMX.NU.Video.Encoder

#alsa modules
PRODUCT_PACKAGES += \
    acoustics.default  \
	alsa.default  \
	audio.alsa.default \
    libasound

#apu modules
PRODUCT_PACKAGES += \
    libbridge  \
    libNuAudioDspCodec \
    libOMX.NU.APU.codec

#recovery modlue
PRODUCT_PACKAGES += \
	recovery

#OTA module
#PRODUCT_PACKAGES += \
#    Ota

#MMS module
PRODUCT_PACKAGES += \
    Mms

#add for full phone func
PRODUCT_PACKAGES += \
    VoiceDialer

#NTFS module
PRODUCT_PACKAGES += \
    libfuse \
    libntfs-3g \
    ntfs-3g \
    ntfsfix

#add for owner modules
PRODUCT_PACKAGES += \
    LivallService

PRODUCT_PROPERTY_OVERRIDES += \
    keyguard.no_require_sim=true \
    ro.com.android.dataroaming=true


PRODUCT_CHARACTERISTICS := tablet

PRODUCT_COPY_FILES += \
	hardware/nufront/mali/src/libGLESv1_CM_mali.so:system/lib/egl/libGLESv1_CM_mali.so \
	hardware/nufront/mali/src/libGLESv2_mali.so:system/lib/egl/libGLESv2_mali.so \
	hardware/nufront/mali/src/libEGL_mali.so:system/lib/egl/libEGL_mali.so


# hengai 20120525 initlog0.rle
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/initlogo_1024x600.rle:root/initlogo_1024x600.rle \
	$(LOCAL_PATH)/initlogo_800x480.rle:root/initlogo_800x480.rle \
	$(LOCAL_PATH)/ulogo_1024x600.bin:root/ulogo_1024x600.bin \
	$(LOCAL_PATH)/ulogo_800x480.bin:root/ulogo_800x480.bin \
	$(LOCAL_PATH)/initlogo_1024x600.rle:recovery/root/initlogo_1024x600.rle \
	$(LOCAL_PATH)/initlogo_800x480.rle:recovery/root/initlogo_800x480.rle \

# preapk install
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/preapkinstall.sh:system/bin/preapkinstall.sh

$(call inherit-product, frameworks/base/build/tablet-dalvik-heap.mk)
