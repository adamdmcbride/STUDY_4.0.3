# Copyright (C) 2010 The Android Open Source Project
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

ifeq ($(TARGET_PRODUCT),nusmart2_smp)
LOCAL_PATH := $(call my-dir)

#$(call add-prebuilt-file,CIT.apk,APPS)
#$(call add-prebuilt-file,ES.apk,APPS)
#$(call add-prebuilt-file app/google_pinyin_1.4.2.apk,APPS)

include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/vendor/app/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -r $(LOCAL_PATH)/app/*.apk $(TARGET_OUT)/vendor/app/)

include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/vendor/lib/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -r $(LOCAL_PATH)/lib/*.so $(TARGET_OUT)/vendor/lib/)

include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/xbin/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -r $(LOCAL_PATH)/busybox $(TARGET_OUT)/xbin/)

endif