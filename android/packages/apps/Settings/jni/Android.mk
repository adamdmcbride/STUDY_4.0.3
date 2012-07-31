# Copyright (C) 2009 The Android Open Source Project
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
# 
#
# add by alex
#
# This module just for getting modem log and saving them to SD card.
#
# You can close this function by set config_mlc_enable in device/nufront/nusmart3_pad/overlay/packages/apps/Settings/res/values/config.xml to be false
# and set NEED_MODEM_LOG in device/nufront/nusmart3_pad/BoardConfig.mk to be false.
# 
#


LOCAL_PATH := $(call my-dir)
#first lib to do real op
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE    := libMLC
LOCAL_SRC_FILES := MLControler.c
LOCAL_SHARED_LIBRARIES := \
         libutils \
         libcutils
LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../system/core/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../bionic/libc/include/

include $(BUILD_SHARED_LIBRARY)

#binary for setting log when booting
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= boot_main.c

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
         libcutils \
         libMLC

ifeq ($(TARGET_ARCH),arm)
LOCAL_SHARED_LIBRARIES += libdl
endif # arm

LOCAL_MODULE:= boot_log

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../system/core/include/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../bionic/libc/include/

include $(BUILD_EXECUTABLE)

#second lib interact with java layer
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE    := liblogControl-jni
LOCAL_SRC_FILES := logControl-jni.c

LOCAL_REQUIRED_MODULES := boot_log

LOCAL_SHARED_LIBRARIES := \
         libMLC \
         libcutils

#LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../system/core/include/
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../bionic/libc/include/
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/

include $(BUILD_SHARED_LIBRARY)


