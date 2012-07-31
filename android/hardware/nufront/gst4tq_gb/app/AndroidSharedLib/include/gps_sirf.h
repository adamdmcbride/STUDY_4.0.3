/*
*
*                   SiRF Technology, Inc. GPS Software
*
*    Copyright (c) 2011 by SiRF Technology, a CSR plc Company.
*    All rights reserved.
*
*    This Software is protected by United States copyright laws and
*    international treaties.  You may not reverse engineer, decompile
*    or disassemble this Software.
*
*    WARNING:
*    This Software contains SiRF Technology Inc.s confidential and
*    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,
*    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED
*    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this
*    Software without SiRF Technology, Inc.s  express written
*    permission.   Use of any portion of the contents of this Software
*    is subject to and restricted by your signed written agreement with
*    SiRF Technology, Inc.
*/

/* SiRF GSD4t SharedLib Version */
#define GSD4t_SHAREDLIB_VERSION "### CSR SiRF build for Nufront ###"

/* Standard Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>

/* Android Header Files */
#include <hardware_legacy/power.h>
#include <dlfcn.h>

/* SiRF Header Files */
#include "string_sif.h"
#include "sirf_pal.h"
#include "sirf_heap_sif.h"
#include "LSM_APIs.h"
#include "SiRFNav.h"
#ifdef LPL_CLM
#include "ee_download_ui.h"
#include "clm_ui.h"
#include "clm_structs.h"
#endif

/* Android Interface Header Files */
#include "gps.h"
#include "sirf_sv_status.h"
#include "gps_sirf_cpa.h"

#define LOG_TAG "gps_sirf"
#include <cutils/log.h>
#define GPS_DEBUG 1
#define DFR(...)   LOGD(__VA_ARGS__)
#if GPS_DEBUG
#define D(...)   LOGD(__VA_ARGS__)
#else
#define D(...)   ((void)0)
#endif


//#define TEST_MODE_SHARED_LIB

#ifdef TEST_MODE_SHARED_LIB

#include "sirf_testmode_interface.h"
#define GPS_TESTMODE_SHARED_LIB_PATH  "/system/lib/lib_gsd4t_testmode.so"

static const SiRFGpsTestModeInterface* (*p_get_gps_testmode_interface)() = NULL;
static const SiRFGpsTestModeInterface* sGpsTestModeInterface = NULL;
static void* dlHandle = NULL;

#endif

#define ENABLE_NMEA_LISTENER           1
#define ENABLE_SSB_LISTENER            0
#define ENABLE_DEBUG_LISTENER          0
#define ENABLE_SERIAL_LISTENER         0
#define ENABLE_TESTMODE_LISTENER       1
#define ENABLE_SUPL_INIT_LISTENER      1
#ifndef ENABLE_NMEA_FILE
#define ENABLE_NMEA_FILE               0
#endif

#define NVM_FILENAME_LENGTH            32

#define LPL_HEAP (128*1024)  /* 128KB */

#define DEFAULT_HOR_ERROR              1000
#define DEFAULT_VER_ERROR              3000

#define GSD4t_INITIALIZED   0x00
#define GSD4t_UNINITIALIZED 0x01

#define INVAILD_SESSION_HANDLE 0

#define NO_NI_SESSION      0
#define NI_SESSION_RUNNING 1
#define NI_SESSION_CLOSING 2

#define SIRF_INTERFACE_LOG   "/sirf_interface_log.txt"



#define UART_DRIVER_PATH     "/dev/ttyS1"
#define RESET_GPIO           "/dev/gps_sirf"
#define ONOFF_GPIO           "/dev/gps_sirf"

#define GPS_SIRF_CMD_TEST_MODE_OFF		0
#define GPS_SIRF_CMD_TEST_MODE_4		4
#define GPS_SIRF_CMD_TEST_MODE_5		5
#define GPS_SIRF_CMD_TEST_MODE_6		6
#define GPS_SIRF_CMD_TEST_MODE_7		7
#define GPS_SIRF_CMD_TEST_MODE_READ		0x20
#define GPS_SIRF_CMD_TEST_MODE_INIT		0x30
#define GPS_SIRF_CMD_TEST_MODE_DEINIT	0x40

typedef struct
{
   tSIRF_UINT32 freq;
   float cno;
}CW_TESTMODE_RESULT;

static CW_TESTMODE_RESULT CWTestModeResultPeakValue; 
int        sCWTestCnt = 0;


typedef struct _GSD4tConfiguration
{
   /* LSM Common settings */
   tSIRF_UINT16        numOfSessions;
   tSIRF_UINT16        serverPort;
   tSIRF_CHAR          serverAddress[256];
   tSIRF_UINT8         isSecure;
   tSIRF_UINT8         ssl_enabled; /*for common configuration */

   /* LSM Session settings */
   tSIRF_UINT8         resetType;

   /* SET Capabilities */
   tSIRF_BOOL          isPosTechMSB ;
   tSIRF_BOOL          isPosTechMSA;
   tSIRF_BOOL          isPosTechAuto;
   tSIRF_BOOL          isPosTechECID;
   tSIRF_UINT8         prefLocMethod;

   /* Position Mode */
   eLSM_AIDING_TYPE    aidingType;
   LSM_UINT            isPrefLocationMethodSet;
   tSIRF_BOOL          networkStatus;

   tSIRF_UINT8         numberOfFixes;
   tSIRF_UINT8         timeBetweenFixes;

   tSIRF_UINT8         AssistProtocol;
   tSIRF_BOOL          isApproximateLocationPresent;
   tSIRF_UINT16        approximateAltitude;
   tSIRF_DOUBLE        approximateLongitude;
   tSIRF_DOUBLE        approximateLatitude;

   tSIRF_BOOL          QoSinSUPLSTART;
   tSIRF_UINT8         maxLocAge;
   tSIRF_UINT8         QoSMaxResponseTime;
   tSIRF_UINT8         QoSVerticalAccuracy;
   tSIRF_UINT8         QoSHorizontalAccuracy;

   tSIRF_UINT8         identification;

   tSIRF_BOOL          isAPM;
   tSIRF_UINT8         powerControl;
   tSIRF_UINT8         triggers;

   tSIRF_UINT8         loggingType;
   tSIRF_CHAR          briefLogFileName[256];
   tSIRF_CHAR          agpsLogFileName[256];
   tSIRF_CHAR          detailedLogFileName[256];
   tSIRF_CHAR          prevLocFileName[256];
   tSIRF_CHAR          slcLogFileName[256];
   tSIRF_CHAR          sirfInterfaceLogFileName[256];
   tSIRF_CHAR          nmeaLogFileName[256];

   tSIRF_BOOL          isCapNavBitAiding;
   tSIRF_BOOL          isCapCGEE;
   tSIRF_BOOL          isCapSGEE;

   /* SET Info and Cell Info */
   LSM_netCellID       CellIDInfo;
   LSM_BOOL            isCellInfoValid;
   SETID_Info          SETIDInfo;
   LSM_BOOL            isSETIDInfoValid;

   /* SGEE server specific parameter*/
   tSIRF_CHAR          SGEEserverAddress[2][80];
   tSIRF_UINT16        SGEEserverPort;
   tSIRF_CHAR          SGEEserverFile[256];
   tSIRF_CHAR          SGEEserverAuth[256];

   /* Test Mode Configuration */
   LSM_BOOL            isTestModeEnable;
   eLSM_TEST_MODE_TYPE testModeType;
   LSM_UINT16          testModeSvId;
   LSM_UINT16          testModePeriod;
   LSM_BOOL            isCNOTest;
   /* use different timeout values for SUPL and CP */
   int                 defaultResponseTimeoutofNI;

   /* 4t configuration */
   tSIRF_CONFIG        trackerConfig;

   tSIRF_CHAR          project_name[100];
   tSIRF_UINT8         frequency_aiding;
   tSIRF_UINT8         sensor_aiding;
   tSIRF_UINT8         set_id_imsi;
   tSIRF_UINT8         debugging;
   char                log_path[100];
   tSIRF_UINT8         reaidingSVLimit;
   tSIRF_UINT8         isReAidingParamSet;
   tSIRF_UINT32        reAidingTimeIntervalLimit;
   tSIRF_UINT8         isControlPlaneEnabled;
   tSIRF_UINT8         isATTNetworkOperator;
   tSIRF_UINT8         isEMC_ENABLE;
}  GSD4tConfig;

tSIRF_UINT8 ATTNetworkOperatorEnabled = 0;
tSIRF_UINT8 AndroidIceCreamNetworkRequirement = 0; /* for Android IceCream */


int LPL_QOS_MET_check = 0;

static struct gps_device_t  sirf_gps_device;

static LSM_commonCfgData     LSM_CommonCfg;
static LSM_SIsessionCfgData  LSM_SessionCfg;
static GSD4tConfig           g_GSD4tConfig;
tPAL_CONFIG                  g_userPalConfig;
tSIRF_UINT8 PAL_ssl_version;
tSIRF_UINT8 CP_response_time;

GpsCallbacks       gpsCallbacks;
AGpsCallbacks      agpsCallbacks;
GpsNiCallbacks     gpsNiCallbacks;
AGpsRilCallbacks   gpsRilCallbacks;
#if 0 //Sensor input, not used 
AGpsMiscCallbacks  agpsMiscCallbacks;
#endif

GpsStatus        gpsStatus;
GpsLocation      locationInfo;
SiRFGpsSvStatus  svStatus;
GpsSvStatus      androidSVStatus;

GpsNiNotification  niNotification;
AGpsStatus  agpsStatus;

static int GPS_NI_RESPONSE = GPS_NI_RESPONSE_NORESP;

tSIRF_SEMAPHORE semAndroidSessionHandler = NULL;
/* semaphore to wait for position */
tSIRF_SEMAPHORE semMoPosition = NULL;
tSIRF_SEMAPHORE semNiPosition = NULL;
tSIRF_SEMAPHORE semNiResponse = NULL;
tSIRF_SEMAPHORE semRilEvent = NULL;
tSIRF_SEMAPHORE semwaitstop = NULL;
tSIRF_SEMAPHORE semwaitinit = NULL;


static unsigned long g_SessionID = INVAILD_SESSION_HANDLE;
static int g_NI_Session = NO_NI_SESSION;
static int g_Initialization_Status = GSD4t_UNINITIALIZED;
static int GPS_POSITION_MODE = GPS_POSITION_MODE_MS_BASED; /* default MSB!!! */
static GpsUtcTime gpsUtcTimeLocation = 0;
static int posCnt = 0;
static int posQoSMet = 0;
static uint32_t  saved_used_in_fix_mask = 0;

#define AGPS_DATA_CONNECTION_CLOSED   0
#define AGPS_DATA_CONNECTION_OPENING  1
#define AGPS_DATA_CONNECTION_OPEN     2

static int DataConnectionStatus = AGPS_DATA_CONNECTION_CLOSED;

#define SUPL_INIT_MSG_LENGTH 256
static unsigned char SUPL_INIT_MESSAGE[SUPL_INIT_MSG_LENGTH];
static int supl_init_msg_length = 0;

typedef enum
{
   IdleState,
   MOSession,
   NISession,
   CPSession,
   UnknownSession,
   ErrorState
} eAndroidSessionHandlerState;

static int run_session_handler = 0;

 typedef enum
 {
	ANDROID_SESSION_NOT_READY,
	ANDROID_SESSION_NONE,
	ANDROID_MO_SESSION,
	ANDROID_NI_SESSION,
	ANDROID_CP_SESSION
 } eAndroidSession;

#define ANDROID_GPS_PWR_ON  0
#define ANDROID_GPS_PWR_OFF 1

static volatile eAndroidSession android_session_check = ANDROID_SESSION_NOT_READY;

typedef enum
{
   IdleStatusReport,
   SVStatusReport,
   LocationStatusReport,
   NMEAStatusReport,
   DataConnectionStatusReport,
   NINotificationStatusReport,
   ExitStatusReport
} eAndroidStatusReportHandlerState;

typedef enum
{
   ANDROID_STATUS_REPORT_SATELITES               = 0x0001,
   ANDROID_STATUS_REPORT_NMEA                    = 0x0002,
   ANDROID_STATUS_REPORT_LOCATION                = 0x0004,
   ANDROID_STATUS_REPORT_DATACONNECTION          = 0x0008,
   ANDROID_STATUS_REPORT_NI_NOTIFICATION         = 0x0010,
   ANDROID_STATUS_REPORT_CP_NI_NOTIFICATION      = 0x0020,
   ANDROID_STATUS_REPORT_REF_LOC_REQ             = 0x0040,
   ANDROID_STATUS_REPORT_SETID_REQ               = 0x0080,
   ANDROID_STATUS_REPORT_POWER_SAVING            = 0x0100,
   ANDROID_STATUS_REPORT_EXIT                    = 0x0200
} eAndroidStatusReport;

#define NMEA_SENTENCE_LENGTH    100
#define NMEA_SENTENCE_COUNT     40
typedef struct
{
   GpsUtcTime  timestamp;
   char        nmea[NMEA_SENTENCE_LENGTH];
   int         length;
} NmeaSentence;

static NmeaSentence sNmeaBuffer[NMEA_SENTENCE_COUNT];
static int sNmeaSentenceCount = 0;

char NMEA_GPGGA[NMEA_SENTENCE_LENGTH];
int NMEA_GPGGA_LEN = 0;
GpsUtcTime NMEA_GPGGA_TIMESTAMP;

char NMEA_GPRMC[NMEA_SENTENCE_LENGTH];
int NMEA_GPRMC_LEN = 0;
GpsUtcTime NMEA_GPRMC_TIMESTAMP;

char NMEA_GPGLL[NMEA_SENTENCE_LENGTH];
int NMEA_GPGLL_LEN = 0;
GpsUtcTime NMEA_GPGLL_TIMESTAMP;

static int sAndroidStatusPendingCallbacks = 0;

GpsSvStatus androidSVStatusCopy;
GpsLocation  locationInfoCopy;
static NmeaSentence sNmeaBufferCopy[NMEA_SENTENCE_COUNT];
AGpsStatus  agpsStatusCopy;

static int android_session_handler_suspend(void);
static int android_session_handler_resume(eAndroidSession session);

static pthread_mutex_t sAndroidStatusEventMutex   = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sAndroidSessionMutex       = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sAndroidRilMutex           = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t sPosQosMetMutex            = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sAndroidStatusEventCond     = PTHREAD_COND_INITIALIZER;

static int sTrackerUploadDone = 0;
static pthread_mutex_t sTrackerUploadMutex   = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sTrackerUploadCond     = PTHREAD_COND_INITIALIZER;



static LSM_BOOL getLSMCommonConfiguration(LSM_commonCfgData *pCommonCfgData);
static LSM_BOOL getLSMSessionCfgData(LSM_SIsessionCfgData *pSessionCfgData);
static tSIRF_RESULT gpioInitialization(tSIRF_VOID);
static tSIRF_RESULT gpioDeInitialization(tSIRF_VOID);

LSM_BOOL controlPowerWakeLock(int enable);
static int sirf_gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence, \
                                      uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time);

static LSM_BOOL requestDataConnection(void);
static LSM_BOOL getSETIDInfo (void);
static LSM_BOOL getCELLInfo(void);
static LSM_BOOL requestAllRilInfo(void);
LSM_BOOL releaseDataConnection(void);

/* Customer GPS Configuration */
//#define CSR_GPS_CONFIG  //CUSTOMER_GPS_CONFIG // moved to makefiles

#ifdef CUSTOMER_GPS_CONFIG
/* Customer Requirement for default response timeout for notification */
#define SUPL_NI_NOTIFICATION_DEFAULT_RESPONSE_TIMEOUT  6000
#define CP_NI_NOTIFICATION_DEFAULT_RESPONSE_TIMEOUT    20000
#else
#define SUPL_NI_NOTIFICATION_DEFAULT_RESPONSE_TIMEOUT  15000
#define CP_NI_NOTIFICATION_DEFAULT_RESPONSE_TIMEOUT    15000
#endif

#ifdef CSR_GPS_CONFIG
//#define CSR_GPS_CONFIG_FILE		"/opl/etc/gps/csr_gps.conf"
//app/AndroidSharedLib/config/csr_gps.conf
//#define CSR_GPS_CONFIG_FILE           "/app/AndroidSharedLib/config/csr_gps.conf"
//#define CSR_GPS_CONFIG_FILE           "/system/lib/hw/csr_gps.conf"
#define CSR_GPS_CONFIG_FILE           "/system/etc/gps/csr_gps.conf"
static int csr_property_load_config(void);
static int csr_property_load_cmcc_config(void);

#elif defined CUSTOMER_GPS_CONFIG
#define SEC_CONFIG_FILE			"/data/data/com.android.angryGps/secgps.conf"

static int sirf_sec_property_load_config(void);
static LSM_BOOL sirf_sec_check_factory_test_mode(void);


#endif /* CUSTOMER_GPS_CONFIG */

//#define SIRF_GPS_CONFIG_FILE  "/opl/etc/gps/sirfgps.conf"
//./app/AndroidSharedLib/config/sirfgps.conf
//#define SIRF_GPS_CONFIG_FILE  "/system/lib/hw/sirfgps.conf"
#define SIRF_GPS_CONFIG_FILE  "/system/etc/gps/sirfgps.conf"
static int sCNOTestCnt = 0;

#define CNOTestDiscardCnt 6

#define CW_FREQUENCE_RANGE 200000
#define CW_SAMPLES 3
float     CW_MEASURED[CW_SAMPLES];

struct property_item
{
   char *name;
   char *value;
};

#define MAX_PROPERTY_NUM 25
static struct property_item property_list[MAX_PROPERTY_NUM];

int set_property(char *name, char *value);
char *get_property(char *name);

int load_properties(const char *path);
void print_properties();
int free_properties();
int sirf_sgee_download_start(void);
int sirf_sgee_download_stop(void);
static void sirf_gps_load_default_config(void);
static int sirf_gps_load_config(void);
static LSM_BOOL isSETIDIMSI(void);


