LOCAL_PATH:= $(call my-dir)

#LVParam lib
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
  LVParams.cpp
LOCAL_MODULE := LVParams
LOCAL_SHARED_LIBRARIES := libc\
  libcutils \
  libutils \
  libbinder \
  libsysutils
include $(BUILD_STATIC_LIBRARY)


# Service
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
  livall_service.cpp \
  LVParams.cpp \
  LVServer.cpp \
  global.cpp \
  cpu.cpp
LOCAL_MODULE := LivallService
LOCAL_SHARED_LIBRARIES := libc\
  libcutils \
  libutils \
  libbinder \
  libsysutils
LOCAL_STATIC_LIBRARIES := LVParams
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
  client_test.cpp
LOCAL_MODULE := lct
LOCAL_SHARED_LIBRARIES := libc\
  libcutils \
  libutils \
  libbinder \
  libsysutils
LOCAL_STATIC_LIBRARIES := LVParams
include $(BUILD_EXECUTABLE)
