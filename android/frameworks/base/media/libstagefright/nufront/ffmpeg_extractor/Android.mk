LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include frameworks/base/media/libstagefright/codecs/common/Config.mk

LOCAL_SRC_FILES:=                         \
        def.cpp          \
        FFmpegSafeReader.cpp  \
        FFmpegExtractor.cpp   \
        FFmpeg.cpp            \
        androidSource.cpp     \
        FFmpegUtils.cpp     \
        md5.c

LOCAL_C_INCLUDES:= \
        $(JNI_H_INCLUDE) \
        $(TOP)/frameworks/base/include/media/stagefright \
        $(TOP)/frameworks/base/media/libstagefright \
        $(TOP)/frameworks/base/include/media/stagefright/openmax \
        $(TOP)/external/flac/include \
        $(TOP)/external/tremolo \
        $(TOP)/external/openssl/include \
        $(TOP)/external/nufront/ffmpeg

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

################################################################################

LOCAL_SHARED_LIBRARIES += \
        libstagefright_amrnb_common \
        libstagefright_enc_common \
        libstagefright_avc_common \
        libstagefright_foundation \
        libdl

LOCAL_CFLAGS += -Wno-multichar  -DUSE_FFMPEG_EXTRACTOR

ifeq ($(MEDIA_VERSION),VER_NS115)
    LOCAL_C_INCLUDES += \
        $(TOP)/hardware/nufront/media_115/vpu/decode/source/inc
else
    LOCAL_C_INCLUDES += \
        $(TOP)/hardware/nufront/media_2816/vpu/decode/source/inc
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

LOCAL_MODULE:= libffmpeg_extractor

include $(BUILD_STATIC_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
