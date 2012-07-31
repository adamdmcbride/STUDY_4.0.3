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

LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_PRODUCT),full_nusmart3_pad)

#$(call add-prebuilt-file,CIT.apk,APPS)
#$(call add-prebuilt-file,ES.apk,APPS)
#$(call add-prebuilt-file app/google_pinyin_1.4.2.apk,APPS)

include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/vendor/app/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/app/*.apk $(TARGET_OUT)/vendor/app/)

include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/vendor/lib/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/lib/*.so $(TARGET_OUT)/vendor/lib/)

include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/xbin/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/busybox $(TARGET_OUT)/xbin/)

#add by alex
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/etc/ppp/peers/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/ppp_cfg/ip-up-ppp0 $(TARGET_OUT)/etc/ppp)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/ppp_cfg/ip-down-ppp0 $(TARGET_OUT)/etc/ppp)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/ppp_cfg/gprs $(TARGET_OUT)/etc/ppp/peers)

include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/bin/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/ppp_cfg/chat $(TARGET_OUT)/bin/)

include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/vendor/bin/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/bin/telinklog $(TARGET_OUT)/vendor/bin/)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/bin/run_modem_log.sh $(TARGET_OUT)/vendor/bin/)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/bin/run_no_modem_log.sh $(TARGET_OUT)/vendor/bin/)
#end add

# hengai 2012-06-02 copy preapk/*.apk to /system/etc/.pre_apk/
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/etc/.pre_apk/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/preapk/*.apk $(TARGET_OUT)/etc/.pre_apk/)

# hengai 2012-07-04 copy permissions(xml)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/etc/permissions/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/permissions/*.xml $(TARGET_OUT)/etc/permissions/)
# hengai 2012-07-04 copy *.jar to /system/framework/
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell mkdir -p $(TARGET_OUT)/framework/)
include $(CLEAR_VARS)
LOCAL_POST_PROCESS_COMMAND := $(shell cp -rf $(LOCAL_PATH)/framework/* $(TARGET_OUT)/framework/)



endif