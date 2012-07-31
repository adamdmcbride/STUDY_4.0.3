LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    AudioFlinger.cpp            \
    AudioMixer.cpp.arm          \
    AudioResampler.cpp.arm      \
    AudioResamplerSinc.cpp.arm  \
    AudioResamplerCubic.cpp.arm \
    AudioPolicyService.cpp

LOCAL_C_INCLUDES := \
    system/media/audio_effects/include

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    libmedia \
    libhardware \
    libhardware_legacy \
    libeffects \
    libdl \
    libpowermanager

LOCAL_STATIC_LIBRARIES := \
    libcpustats \
    libmedia_helper

ifeq ($(BOARD_USES_GENERIC_AUDIO),true)
  LOCAL_CFLAGS += -DWITH_GENERIC_AUDIO
endif
ifeq ($(BOARD_USES_ALSA_AUDIO),true)
  LOCAL_CFLAGS += -DWITH_ALSA_AUDIO
endif

LOCAL_MODULE:= libaudioflinger

include $(BUILD_SHARED_LIBRARY)
