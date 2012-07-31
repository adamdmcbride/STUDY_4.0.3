LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/base/media/libstagefright/codecs/common/Config.mk

### a@nufront begin
USE_FFMPEG_EXTRACTOR := "true"
#USE_NUAUDIO_DECODER := "true"
### a@nufront end

LOCAL_SRC_FILES:=                         \
        ACodec.cpp                        \
        AACExtractor.cpp                  \
        AACWriter.cpp                     \
        AMRExtractor.cpp                  \
        AMRWriter.cpp                     \
        AudioPlayer.cpp                   \
        AudioSource.cpp                   \
        AwesomePlayer.cpp                 \
        CameraSource.cpp                  \
        CameraSourceTimeLapse.cpp         \
        VideoSourceDownSampler.cpp        \
        DataSource.cpp                    \
        DRMExtractor.cpp                  \
        ESDS.cpp                          \
        FileSource.cpp                    \
        FLACExtractor.cpp                 \
        HTTPBase.cpp                      \
        JPEGSource.cpp                    \
        MP3Extractor.cpp                  \
        MPEG2TSWriter.cpp                 \
        MPEG4Extractor.cpp                \
        MPEG4Writer.cpp                   \
        MediaBuffer.cpp                   \
        MediaBufferGroup.cpp              \
        MediaDefs.cpp                     \
        MediaExtractor.cpp                \
        MediaSource.cpp                   \
        MediaSourceSplitter.cpp           \
        MetaData.cpp                      \
        NuCachedSource2.cpp               \
        OMXClient.cpp                     \
        OMXCodec.cpp                      \
        OggExtractor.cpp                  \
        SampleIterator.cpp                \
        SampleTable.cpp                   \
        StagefrightMediaScanner.cpp       \
        StagefrightMetadataRetriever.cpp  \
        SurfaceMediaSource.cpp            \
        ThrottledSource.cpp               \
        TimeSource.cpp                    \
        TimedEventQueue.cpp               \
        Utils.cpp                         \
        VBRISeeker.cpp                    \
        WAVExtractor.cpp                  \
        WVMExtractor.cpp                  \
        XINGSeeker.cpp                    \
        avc_utils.cpp                     \

LOCAL_C_INCLUDES:= \
	$(JNI_H_INCLUDE) \
        $(TOP)/frameworks/base/include/media/stagefright/openmax \
        $(TOP)/external/flac/include \
        $(TOP)/external/tremolo \
        $(TOP)/external/openssl/include \
        $(TOP)/external/nufront/ffmpeg \

ifeq ($(MEDIA_VERSION),VER_NS2816)
LOCAL_C_INCLUDES += \
        $(TOP)/frameworks/base/media/libstagefright/nufront/hardwarerenderer_2816
else
LOCAL_C_INCLUDES += \
        $(TOP)/frameworks/base/media/libstagefright/nufront/hardwarerenderer_115
LOCAL_CFLAGS += -DNS115 
endif

### a@nufront begin.
ifdef  USE_FFMPEG_EXTRACTOR
LOCAL_C_INCLUDES += $(TOP)/external/nufront/ffmpeg
endif
### a@nufront end.

LOCAL_SHARED_LIBRARIES := \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libui             \
        libsonivox        \
        libvorbisidec     \
        libstagefright_yuv \
        libcamera_client \
        libdrmframework  \
        libcrypto        \
        libssl           \
        libgui           \
        libavformat      \
        libavutil        \
        libavcodec       \

LOCAL_STATIC_LIBRARIES := \
        libstagefright_color_conversion \
        libstagefright_aacenc \
        libstagefright_amrnbenc \
        libstagefright_amrwbenc \
        libstagefright_avcenc \
        libstagefright_m4vh263enc \
        libstagefright_matroska \
        libstagefright_timedtext \
        libvpx \
        libstagefright_mpeg2ts \
        libstagefright_httplive \
        libstagefright_id3 \
        libFLAC \
        libhardwarerenderer \
        ### a@nufront begin.
        ifdef  USE_FFMPEG_EXTRACTOR
        LOCAL_STATIC_LIBRARIES += libffmpeg_extractor
        endif
        ### a@nufront end.



################################################################################

# The following was shamelessly copied from external/webkit/Android.mk and
# currently must follow the same logic to determine how webkit was built and
# if it's safe to link against libchromium.net

# V8 also requires an ARMv7 CPU, and since we must use jsc, we cannot
# use the Chrome http stack either.
ifneq ($(strip $(ARCH_ARM_HAVE_ARMV7A)),true)
  USE_ALT_HTTP := true
endif

# See if the user has specified a stack they want to use
HTTP_STACK = $(HTTP)
# We default to the Chrome HTTP stack.
DEFAULT_HTTP = chrome
ALT_HTTP = android

ifneq ($(HTTP_STACK),chrome)
  ifneq ($(HTTP_STACK),android)
    # No HTTP stack is specified, pickup the one we want as default.
    ifeq ($(USE_ALT_HTTP),true)
      HTTP_STACK = $(ALT_HTTP)
    else
      HTTP_STACK = $(DEFAULT_HTTP)
    endif
  endif
endif

ifeq ($(HTTP_STACK),chrome)

LOCAL_SHARED_LIBRARIES += \
        liblog           \
        libicuuc         \
        libicui18n       \
        libz             \
        libdl            \

LOCAL_STATIC_LIBRARIES += \
        libstagefright_chromium_http

LOCAL_SHARED_LIBRARIES += libstlport libchromium_net
include external/stlport/libstlport.mk

LOCAL_CPPFLAGS += -DCHROMIUM_AVAILABLE=1

endif  # ifeq ($(HTTP_STACK),chrome)

################################################################################

LOCAL_SHARED_LIBRARIES += \
        libstagefright_amrnb_common \
        libstagefright_enc_common \
        libstagefright_avc_common \
        libstagefright_foundation \
        libdl

LOCAL_CFLAGS += -Wno-multichar

### a@nufront begin
ifdef USE_FFMPEG_EXTRACTOR
LOCAL_CFLAGS += -DUSE_FFMPEG_EXTRACTOR
endif
ifdef USE_NUAUDIO_DECODER
LOCAL_CFLAGS += -DUSE_NUAUDIO_DECODER
endif
### a@nufront end

ifeq ($(USE_GC_CONVERTER), true)
	LOCAL_CFLAGS += -DUSE_GC_CONVERTER
endif

ifeq ($(strip $(BOARD_USES_APU_CODEC)),true)
    LOCAL_CFLAGS += -DWITH_APU_CODEC
ifeq ($(strip $(BOARD_USES_MP3_APU_CODEC)),true)
    LOCAL_CFLAGS += -DWITH_MP3_APU_CODEC
endif
ifeq ($(strip $(BOARD_USES_AAC_APU_CODEC)),true)
    LOCAL_CFLAGS += -DWITH_AAC_APU_CODEC
endif
endif

LOCAL_MODULE:= libstagefright

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
