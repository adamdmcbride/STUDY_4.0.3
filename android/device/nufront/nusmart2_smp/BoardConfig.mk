# These definitions override the defaults in config/config.make for armboard_v7a
#
# TARGET_NO_BOOTLOADER := false

TARGET_HARDWARE_3D := false 

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi 

TARGET_NO_KERNEL := false
TARGET_ARCH_VARIANT := armv7-a-neon

ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_BOARD_PLATFORM := ns2816

#m@nufront start: modify for alsa
#BOARD_USES_GENERIC_AUDIO := true
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true
ALSA_DEFAULT_SAMPLE_RATE := 48000
#m@nufront end

MEDIA_VERSION := VER_NS2816

#USE_CAMERA_STUB := true
USE_CAMERA_STUB := false
CAMERA_VERSION := VER_NS2816

USE_OPENGL_RENDERER := true

TARGET_CPU_SMP := true

TARGET_NO_BOOTLOADER := true
TARGET_NO_RECOVERY := false

BOARD_EGL_CFG := device/nufront/nusmart2_smp/egl.cfg

#add by zhangjun.
BOARD_HAVE_BLUETOOTH := true
#BOARD_HAVE_BLUETOOTH := false

#add by chen NuSmart for wifi
#WIFI_NOT_USE_S3 := true
WIFI_NOT_USE_S3 := false 

BOARD_WPA_SUPPLICANT_DRIVER := WEXT
#BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION := VER_0_8_X
#WPA_SUPPLICANT_VERSION := VER_0_6_X
#WPA_SUPPLICANT_VERSION := VER_0_5_X
WIFI_DRIVER_LOAD_DYNAMIC := false 
NUFRONT_NUSMART := true
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_wext_nl80211
WIFI_DRIVER_MODULE_NAME1    :=  "mwifiex"
WIFI_DRIVER_MODULE_NAME2    :=  "mwifiex_sdio"
WIFI_DRIVER_MODULE_PATH1     := "/system/lib/modules/mwifiex.ko"
WIFI_DRIVER_MODULE_PATH2     := "/system/lib/modules/mwifiex_sdio.ko"
WIFI_DRIVER_FW_PATH_PARAM := "/system/etc/firmware/mrvl/sd8787_uapsta.bin"
BOARD_WLAN_DEVICE := mlan0
#add end.
