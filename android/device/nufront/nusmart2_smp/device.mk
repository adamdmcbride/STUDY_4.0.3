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
	ro.opengles.version=131072

PRODUCT_PROPERTY_OVERRIDES += \
	hwui.render_dirty_regions=false

# Use our keyboard layout, whereby F1 and F10 are menu buttons.
#
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/qwerty.kl:system/usr/keylayout/qwerty.kl \
	$(LOCAL_PATH)/armboard_v7a.kl:system/usr/keylayout/armboard_v7a.kl \
    $(LOCAL_PATH)/kernel:kernel

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
	frameworks/base/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
	frameworks/base/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
	frameworks/base/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
	frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
	frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml

# add the full apn config
PRODUCT_COPY_FILES += device/sample/etc/apns-full-conf.xml:system/etc/apns-conf.xml

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/vold.fstab:system/etc/vold.fstab

PRODUCT_COPY_FILES += \
    hardware/nufront/libcamera/ns2816/media_profiles.xml:system/etc/media_profiles.xml

# Init files
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.nufront-ns2816.rc:root/init.nufront-ns2816.rc \
    $(LOCAL_PATH)/init.nusmart.debug.rc:root/init.nusmart.debug.rc \
    $(LOCAL_PATH)/ueventd.nufront-ns2816.rc:root/ueventd.nufront-ns2816.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/idc/AT_Translated_Set_2_keyboard.idc:system/usr/idc/AT_Translated_Set_2_keyboard.idc \
    $(LOCAL_PATH)/idc/Goodix_Capacitive_TouchScreen.idc:system/usr/idc/Goodix_Capacitive_TouchScreen.idc \
    $(LOCAL_PATH)/idc/io373x_hotkey.idc:system/usr/idc/io373x_hotkey.idc \
    $(LOCAL_PATH)/idc/io373x_pwrbutton.idc:system/usr/idc/io373x_pwrbutton.idc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    $(LOCAL_PATH)/wifi/dhcpcd.conf:system/etc/dhcpcd/dhcpcd.conf \
    $(LOCAL_PATH)/wifi/dhcpcd-mlan0.conf:system/etc/dhcpcd/dhcpcd-mlan0.conf \
    $(LOCAL_PATH)/wifi/sd8787_uapsta.bin:system/etc/firmware/mrvl/sd8787_uapsta.bin \
    $(LOCAL_PATH)/wifi/mlan.ko:system/lib/modules/mlan.ko \
    $(LOCAL_PATH)/wifi/sd8xxx.ko:system/lib/modules/sd8xxx.ko

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/bt/bt8xxx.ko:system/lib/modules/bt8xxx.ko

PRODUCT_PACKAGES += \
	Camera \
	mke2fs \
    audio.a2dp.default \
    libmbm-ril \
	libjni_pinyinime

# Live Wallpapers
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
	sensors.default

# Mali OpenGL ES/EGL modules
PRODUCT_PACKAGES += \
	libMali  \
	libUMP   \
	libGLESv1_CM_mali \
	libGLESv2_mali  \
	libEGL_mali

# hwui
PRODUCT_PACKAGES += \
	libhwui

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
	libdecx170p \
	libdecx170rv \
	libdecx170v \
	libdec8190vp6 \
    libdecx170jpeg \
    libstagefrighthw \
	libOMX.NU.Audio.Decoder \
	libOMX_Core \
	libOMX.NU.Video.Decoder

#alsa modules
PRODUCT_PACKAGES += \
    acoustics.default  \
	alsa.default  \
	audio.alsa.default \
    libasound

#modules module
PRODUCT_PACKAGES += \
	recovery

PRODUCT_CHARACTERISTICS := tablet

# Bluetooth config file
PRODUCT_COPY_FILES += \
	system/bluetooth/data/main.nonsmartphone.conf:system/etc/bluetooth/main.conf

# Screen size is "normal", density is "hdpi"
#PRODUCT_AAPT_CONFIG := normal hdpi

$(call inherit-product, frameworks/base/build/tablet-dalvik-heap.mk)
