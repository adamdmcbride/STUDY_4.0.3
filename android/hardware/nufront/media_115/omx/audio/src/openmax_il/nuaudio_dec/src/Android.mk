#ifeq ($(BUILD_AAC_DECODER),1)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

#FFMPEG_PATH := $(NU_OMX_AUDIO)/../../../../../../../external/nufront/ffmpeg
FFMPEG_PATH := $(TOPDIR)external/nufront/ffmpeg

LOCAL_SRC_FILES:= \
    OMX_NuAudioDec_CompThread.c     \
    OMX_NuAudioDec_Utils.c      \
    OMX_NuAudioDecoder.c        \
    OMX_BufQ.c

LOCAL_C_INCLUDES := $(NU_OMX_COMP_C_INCLUDES) \
    $(NU_OMX_SYSTEM)/common/inc     \
    $(NU_OMX_AUDIO)/nuaudio_dec/inc \
    $(FFMPEG_PATH) \
    $(FFMPEG_PATH)/libavutil \
    $(FFMPEG_PATH)/libavcodec \
    $(FFMPEG_PATH)/libavformat \
    $(FFMPEG_PATH)/ffpresets \
    $(FFMPEG_PATH)/libavdevice \
    $(FFMPEG_PATH)/libavfilter \
    $(FFMPEG_PATH)/libpostproc \
    $(FFMPEG_PATH)/libswscale \
    $(FFMPEG_PATH)/tools \

LOCAL_SHARED_LIBRARIES := $(NU_OMX_COMP_SHARED_LIBRARIES) \
        liblog libavformat libavcodec libavfilter libavutil


LOCAL_LDLIBS += \
    -lpthread \
    -ldl \
    -lsdl

LOCAL_CFLAGS := $(NU_OMX_CFLAGS)
ifneq ($(ALSA_DEFAULT_SAMPLE_RATE),)
    LOCAL_CFLAGS += -DNU_OUTPUT_SAMPLERATE=$(ALSA_DEFAULT_SAMPLE_RATE)
endif
LOCAL_MODULE:= libOMX.NU.Audio.Decoder

include $(BUILD_SHARED_LIBRARY)
#endif
