ifeq ($(MEDIA_VERSION),VER_NS115)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := HardwareRenderer.cpp

LOCAL_C_INCLUDES:=  $(TOP)/frameworks/base/include/media/stagefright/openmax \
        $(TOP)/frameworks/base/media/libstagefright/include/hardwarerenderer/hardwarerenderer_115 \
		$(TOP)/frameworks/base/ \
		$(TOP)/hardware/msm7k

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog

ifeq ($(USE_GC_CONVERTER), true)
	LOCAL_CFLAGS += -DUSE_GC_CONVERTER
endif

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libhardwarerenderer

include $(BUILD_STATIC_LIBRARY)
endif

