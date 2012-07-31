LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng 

NU_OMX_CFLAGS := -Wall -fpic -pipe -DSTATIC_TABLE -O0

NU_OMX_TOP := $(LOCAL_PATH)
NU_OMX_SYSTEM := $(NU_OMX_TOP)/system/src/openmax_il
NU_OMX_VIDEO := $(NU_OMX_TOP)/video/src/openmax_il
NU_OMX_AUDIO := $(NU_OMX_TOP)/audio/src/openmax_il
NU_OMX_INCLUDES := \
	$(NU_OMX_SYSTEM)/omx_core/inc

NU_APU_TOP := $(LOCAL_PATH)/../apu
NU_APU_DSPBRIDGE := $(NU_APU_TOP)/dspbridge
NU_APU_CODEC := $(NU_APU_TOP)/apucodec

NU_APU_INCLUDES := \
	$(NU_APU_TOP)/inc \
	$(NU_APU_CODEC)/inc \
    $(NU_APU_DSPBRIDGE)/libbridge/inc

NU_OMX_COMP_SHARED_LIBRARIES := \
	libdl \
	libOMX_Core \
	libcutils \
        libdwlx170 \
        libdecx170h \
        libdecx170m \
        libdecx170m2 \
        libdecx170rv \
        libdecx170v \
        libdec8190vp6 \
        libdec8190vp8 \
        libdecx170jpeg \
	    liblog	
#	libdecx170p \

NU_OMX_COMP_SHARED_ENC_LIBRARIES := \
    libdl \
    libOMX_Core \
    libcutils \
    liblog\
    libh1enc

NU_OMX_COMP_C_INCLUDES := \
	$(NU_OMX_INCLUDES) \
	$(NU_OMX_SYSTEM)/common/inc 
#call to common omx & system components
include $(NU_OMX_SYSTEM)/omx_core/src/Android.mk
#TODO
include $(NU_OMX_AUDIO)/nuaudio_dec/src/Android.mk

include $(NU_OMX_VIDEO)/video_decode/Android.mk
include $(NU_OMX_VIDEO)/video_encode/Android.mk

ifeq ($(strip $(BOARD_USES_APU_CODEC)),true)
include $(NU_OMX_AUDIO)/nuapu_codec/src/Android.mk
endif
