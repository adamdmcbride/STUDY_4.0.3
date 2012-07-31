LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES:= ami_daemon.c
LOCAL_LDFLAGS := $(LOCAL_PATH)/libami6axis.a
LOCAL_SHARED_LIBRARIES := libcutils libc
LOCAL_MODULE := amid
#LOCAL_CFLAGS := -W -Wall -D DBG_MODE
include $(BUILD_EXECUTABLE)
