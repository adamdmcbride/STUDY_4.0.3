LOCAL_PATH := $(call my-dir)
LD_PATH := $(LOCAL_PATH)/../../../lib


#PRODUCT_COPY_FILES += \
 #         $(LOCAL_PATH)/sirfgps.conf:system/lib/hw/sirfgps.conf \
 #         $(LOCAL_PATH)/csr_gps.conf:system/lib/hw/csr_gps.conf


include $(CLEAR_VARS)
SOURCE_PATH := $(LOCAL_PATH)/../../../sirf

LOCAL_C_INCLUDES += \
$(SOURCE_PATH) \
$(SOURCE_PATH)/include \
$(SOURCE_PATH)/pal \
$(SOURCE_PATH)/pal/Posix \
$(SOURCE_PATH)/SiRFNavIV/cfg \
$(SOURCE_PATH)/SiRFNavIV/include \
$(SOURCE_PATH)/SiRFNavDemo/cfg \
$(SOURCE_PATH)/SiRFLPL/cfg \
$(SOURCE_PATH)/SiRFLPL/include \
$(SOURCE_PATH)/SiRFIF/cfg \
$(SOURCE_PATH)/SiRFIF/include/clm_main \
$(SOURCE_PATH)/SiRFIF/include/diff_to_ee_converter \
$(SOURCE_PATH)/SiRFIF/include/ee_download\
$(SOURCE_PATH)/SiRFIF/src/clm_cgee/clientee \
$(SOURCE_PATH)/util/codec \
$(SOURCE_PATH)/util/ext \
$(SOURCE_PATH)/util/proto \
$(SOURCE_PATH)/stringlib \
$(SOURCE_PATH)/util/config_parser \
$(SOURCE_PATH)/util/demo \
$(SOURCE_PATH)/util/codec \
$(SOURCE_PATH)/util/ext \
$(SOURCE_PATH)/util/proto \
$(SOURCE_PATH)/util/stringlib \
$(SOURCE_PATH)/util/lpl/TestAppConfigMgr/include \
$(SOURCE_PATH)/util/lpl/XMLConfigParser/include \
$(SOURCE_PATH)/util/lpl/gprs_at_command_server \
$(SOURCE_PATH)/util/lpl/gprs_modem_manager \
$(LOCAL_PATH)/../include \
$(LOCAL_PATH)/../include/pxa920 \
hardware/libhardware/include/hardware/ \
external/openssl/include/

LOCAL_SRC_FILES :=  ../source/gps_sirf.c \
	../source/gps_sirf_tcp.c \
	../source/gps_sirf_cpa.c \
	../source/gps_logging.c \
	../../../sirf/util/demo/sirf_demo.c \
	../../../sirf/util/ext/sirf_ext_log.c \
	../../../sirf/util/config_parser/sirf_sensor_config_parser.c \
	../../../sirf/util/codec/sirf_codec.c \
	../../../sirf/util/codec/sirf_codec_csv.c \
	../../../sirf/util/codec/sirf_codec_ascii.c \
	../../../sirf/util/codec/sirf_codec_ssb.c \
	../../../sirf/util/codec/sirf_codec_ssb_agps.c \
	../../../sirf/util/codec/sirf_codec_nmea.c \
	../../../sirf/util/proto/sirf_proto_common.c \
	../../../sirf/util/proto/sirf_proto_parse.c \
	../../../sirf/util/proto/sirf_proto_nmea.c \
        ../../../sirf/pal/Common/sirf_pal.c \
        ../../../sirf/pal/Common/sirf_pal_os_mem_libc.c \
        ../../../sirf/pal/Common/sirf_pal_os_time_libc_crt.c \
        ../../../sirf/pal/Common/sirf_pal_storage_file.c \
        ../../../sirf/pal/Common/sirf_pal_hw_frequency_aiding.c \
        ../../../sirf/pal/Posix/sirf_pal_com_uart.c \
        ../../../sirf/pal/Posix/sirf_pal_com_i2c.c \
        ../../../sirf/pal/Posix/sirf_pal_com_spi.c \
        ../../../sirf/pal/Posix/sirf_pal_hw_on.c \
        ../../../sirf/pal/Posix/sirf_pal_hw_reset.c \
        ../../../sirf/pal/Posix/sirf_pal_log_file.c \
        ../../../sirf/pal/Posix/sirf_pal_os_mutex.c \
        ../../../sirf/pal/Posix/sirf_pal_os_semaphore.c \
        ../../../sirf/pal/Posix/sirf_pal_os_thread.c \
        ../../../sirf/pal/Posix/sirf_pal_os_time.c \
        ../../../sirf/pal/Posix/sirf_threads.c \
        ../../../sirf/pal/Posix/sirf_pal_tcpip_openssl.c 
#	../../../sirf/util/lpl/TestAppConfigMgr/src/TestAppConfigMgr.c \
#	../../../sirf/util/lpl/TestAppConfigMgr/src/RegistryReader.c \
#	../../../sirf/util/lpl/TestAppConfigMgr/src/TagManager.c \
#	../../../sirf/util/lpl/TestAppConfigMgr/src/XMLReader.c \
#	../../../sirf/util/lpl/XMLConfigParser/src/XmlConfigParser.c
	
	
LOCAL_CFLAGS := -D_GNU_SOURCE -DSIRF_HOST -DSIRF_AGPS -DSIRF_LPL \
                -DSS4NAV_SENSOR -DSENSOR_POINT_N_TELL -DSIRF_EXT_LOG -DOS_POSIX -DOS_LINUX -DOS_ANDROID \
		-DSIRF_CLM -DSIRF_INSTANT_FIX \
                -DCPU_ARM -D_ENDIAN_LITTLE -DLPL_CLM -DLPL_SIF -DSIRF_CLM -DSIRF_EXT_TCPIP -DSENS_SSB_DATA_INPUT_MODE

LOCAL_CFLAGS += -DENABLE_NMEA_LISTENER=1 -DENABLE_NMEA_FILE=1
# Check if the next switch is defined externally, in the BoardConfig.mk .
# This switch enables to read configuration from external file.
#ifeq "$(CSR_GPS_CONFIG)" "true"
LOCAL_CFLAGS += -DCSR_GPS_CONFIG
#else
#LOCAL_CFLAGS += -DCUSTOMER_GPS_CONFIG
#endif

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

LOCAL_LDFLAGS := \
	$(LD_PATH)/libsirf_lsm.a \
	$(LD_PATH)/libsirf_lpl_gsd4t.a \
	$(LD_PATH)/libsirf_nav.a \
	$(LD_PATH)/libsirf_rrlp_v5_12.a \
	$(LD_PATH)/libsirf_rrc_v5_13.a \
	$(LD_PATH)/libsirf_supl_v1.a \
	$(LD_PATH)/libsirf_rtper.a \
	$(LD_PATH)/libsirf_rt.a \
	$(LD_PATH)/libsirf_rtx.a \
	$(LD_PATH)/libsirf_sif.a


#add by libenqi start

ifeq ($(TARGET_BOARD_PLATFORM), ns115)
#    LOCAL_CFLAGS := -DNS115
	LOCAL_CFLAGS += -DNS115
endif

#add bu libenqi end




LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  libdl \
  libc \
  libcrypto \
  libssl \
  libnetutils

LOCAL_LDLIBS += -ldl -lm -lpthread
#LOCAL_MODULE := liblsm_gsd4t
LOCAL_MODULE := gps.default
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

include $(BUILD_SHARED_LIBRARY)
