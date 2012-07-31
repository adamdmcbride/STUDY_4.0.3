# These definitions override the defaults in config/config.make for armboard_v6a
#
# TARGET_NO_BOOTLOADER := false

TARGET_HARDWARE_3D := false

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi

TARGET_NO_KERNEL := false
TARGET_ARCH_VARIANT := armv7-a-neon

ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_BOARD_PLATFORM := ns115

#m@nufront start: modify for alsa
#BOARD_USES_GENERIC_AUDIO := true
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_ALSA_AUDIO := true
BUILD_WITH_ALSA_UTILS := true
ALSA_DEFAULT_SAMPLE_RATE := 48000
#m@nufront end
#a@nufront start: modify for apu
BOARD_USES_APU_CODEC := false 
ifeq ($(strip $(BOARD_USES_APU_CODEC)),true)
BOARD_USES_MP3_APU_CODEC := false 
endif
ifeq ($(strip $(BOARD_USES_APU_CODEC)),true)
BOARD_USES_AAC_APU_CODEC := false
endif
#a@nufront end

#set image file system format
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 20971520
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 251658240
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 125829120
BOARD_USERDATAIMAGE_PARTITION_SIZE := 1073741824
BOARD_FLASH_BLOCK_SIZE := 4096


#Camera defines begin--[
#USE_CAMERA_STUB := true
USE_CAMERA_STUB := false
#Camera defines end  --]

MEDIA_VERSION := VER_NS115
#MEDIA_VERSION := VER_NS2816

# mali400 hwversion=r1p1, single pp in ns115
TARGET_MALI400_HWVER := 257
TARGET_MALI_SINGLE_PP := true

BOARD_USES_HDMI := true
USE_GC_CONVERTER := true
USE_OPENGL_RENDERER := true

TARGET_CPU_SMP := true

BOARD_HAVE_BLUETOOTH := true
TARGET_NO_BOOTLOADER := true
TARGET_NO_RECOVERY := false

BOARD_EGL_CFG := device/nufront/nusmart3_pad/egl.cfg


#BOARD_WPA_SUPPLICANT_DRIVER := WEXT
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
#BOARD_WLAN_DEVICE := bcmdhb
WPA_SUPPLICANT_VERSION := VER_0_8_X
#WPA_SUPPLICANT_VERSION := VER_0_6_X
#WPA_SUPPLICANT_VERSION := VER_0_5_X
#WIFI_DRIVER_LOAD_DYNAMIC := false
#NUFRONT_NUSMART := true
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_wext_nl80211
#WIFI_DRIVER_MODULE_NAME    :=  "dhd"
#WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/dhd/dhd.ko"
#WIFI_DRIVER_FW_PATH_PARAM := "/system/lib/wifi/firmware/dhd"
#WIFI_DRIVER_MODULE_ARG  := "firmware_path=/system/lib/wifi/firmware/dhd/sdio-dhd.bin nvram_path=/system/lib/wifi/firmware/dhd/nvram.txt"
WIFI_DRIVER_MODULE_ARG  := "iface_name=mlan firmware_path=/system/etc/firmware/fw_bcmdhd.bin nvram_path=/system/etc/firmware/bcmdhd.cal"
#WIFI_DRIVER_FW_PATH_STA := "/system/lib/wifi/firmware/dhd/sdio-dhd.bin"
#WIFI_DRIVER_FW_PATH_STA := "/system/lib/wifi/firmware/dhd/fw_bcm4330b2.bin"
WIFI_DRIVER_FW_PATH_STA := "/system/etc/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_AP := "/system/etc/firmware/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_P2P := "/system/etc/firmware/fw_bcmdhd_p2p.bin"
WIFI_DRIVER_FW_PATH_PARAM := "/sys/module/bcmdhd/parameters/firmware_path"

APU_CODECS_FW_PATH := "/system/etc/firmware/apu_codecs.bin"
APU_SYSTEM_FW_PATH := "/system/etc/firmware/apu_system.bin"

NEED_MODEM_LOG := true
