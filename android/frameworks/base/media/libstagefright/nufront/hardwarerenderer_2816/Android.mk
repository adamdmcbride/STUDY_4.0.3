ifeq ($(MEDIA_VERSION),VER_NS2816)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := HardwareRenderer.cpp

LOCAL_C_INCLUDES:=  $(TOP)/frameworks/base/include/media/stagefright/openmax \
        $(TOP)/frameworks/base/media/libstagefright/include/hardwarerenderer/hardwarerenderer_2816 \
		$(TOP)/frameworks/base/ \
		$(TOP)/hardware/msm7k

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libhardwarerenderer

include $(BUILD_STATIC_LIBRARY)
endif
