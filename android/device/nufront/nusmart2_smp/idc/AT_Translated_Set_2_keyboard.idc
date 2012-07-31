# Copyright (C) 2010 The Android Open Source Project
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

<<<<<<< HEAD:nusmart3_pad/idc/AT_Translated_Set_2_keyboard.idc
#
# Nusmart keyboard configuration file #1.
#

keyboard.layout = qwerty
#keyboard.characterMap = qwerty
keyboard.orientationAware = 1
keyboard.builtIn = 1
=======
PRODUCT_PROPERTY_OVERRIDES := \
	ro.opengles.version=131072

PRODUCT_PROPERTY_OVERRIDES += \
	hwui.render_dirty_regions=false

# Use our keyboard layout, whereby F1 and F10 are menu buttons.
#
PRODUCT_COPY_FILES += \
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
>>>>>>> android403r1:nusmart2_smp/device.mk

