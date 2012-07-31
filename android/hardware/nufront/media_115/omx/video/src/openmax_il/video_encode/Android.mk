LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_C_INCLUDES := $(NU_OMX_COMP_C_INCLUDES) \
	$(NU_OMX_VIDEO)/video_encode/source \
	$(NU_OMX_VIDEO)/video_encode/h1_encoder \
    $(TOPDIR)hardware/nufront/media_115/vpu/encode/inc \
    $(TOPDIR)frameworks/base/include \
    $(TOPDIR)frameworks/base/include/utils \
    $(TOPDIR)frameworks/base/include/binder 

SRC_ENC:= \
    source/h1_encoder/encoder.c \
    source/h1_encoder/encoder_h264.c    \
    source/h1_encoder/encoder_jpeg.c    \
    source/h1_encoder/encoder_vp8.c     \
    source/h1_encoder/encoder_webp.c    
#    source/h1_encoder/library_entry_point.c     \
#    source/h1_encoder/encoder_constructor_image.c   \
#    source/h1_encoder/encoder_constructor_video.c   \

SRC_MISC:= \
    source/basecomp.c   \
    source/msgque.c \
    source/OSAL.c   \
    source/port.c   \
    source/util.c

LOCAL_SRC_FILES := $(SRC_ENC) ${SRC_MISC}


LOCAL_SHARED_LIBRARIES := $(NU_OMX_COMP_SHARED_ENC_LIBRARIES)
#LOCAL_SHARED_LIBRARIES := \
#    libdl \
#    libOMX_Core \
#    libcutils \
#    liblog\
#    libh1enc    
#    $(NU_OMX_COMP_SHARED_LIBRARIES) 


LOCAL_LDLIBS += \
        libbinder \
	-lpthread \

#The output data of omx is RGB when compling with CONFIG_VPU_PP
LOCAL_CFLAGS := $(NU_OMX_CFLAGS) -DENCH1 -DANDROID -std=c99 -D__OMX_DEBUG_ANDROID__ -DOMX_ENCODER_VIDEO_DOMAIN  -DCONFORMANCE#-DOMX_ENCODER_IMAGE_DOMAIN 
#The output data of omx is YUV when compling without CONFIG_VPU_PP
#LOCAL_CFLAGS := $(NU_OMX_CFLAGS) -DANDROID -std=c99 -D__OMX_DEBUG_ANDROID__ -DAUTO_SWITCH_32_16 -DINPUT_VPUMEM

LOCAL_MODULE:= libOMX.NU.Video.Encoder

include $(BUILD_SHARED_LIBRARY)
