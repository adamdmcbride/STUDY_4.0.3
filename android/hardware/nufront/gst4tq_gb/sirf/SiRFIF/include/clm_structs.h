/**
 * @addtogroup clm_main
 * @ingroup SiRFInstantFix
 * @{
 */

/*
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006- 2011 by SiRF Technology, a CSR plc Company.
 *    All rights reserved.
 *
 *    This Software is protected by United States copyright laws and
 *    international treaties.  You may not reverse engineer, decompile
 *    or disassemble this Software.
 *
 *    WARNING:
 *    This Software contains SiRF Technology Inc.’s confidential and
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this
 *    Software without SiRF Technology, Inc.’s  express written
 *    permission.   Use of any portion of the contents of this Software
 *    is subject to and restricted by your signed written agreement with
 *    SiRF Technology, Inc.
 *
 *
 */

/**
 * @file: clm_structs.h
 * @brief: Definition of the CLM Common Structures used by different modules.
 */

#ifndef __CLM_STRUCTS_H__
#define __CLM_STRUCTS_H__


#include "sirf_types.h"

/******************************************************************************
 *                                 Defines                                    *
 ******************************************************************************/
/* define macro if CLM is needed to be run with multiple threads */
#define CLM_MULTITHREAD_SAFE 1

/*Operating mode*/
#define CLM_CONFIG_MODE_SGEE                0x0
#define CLM_CONFIG_MODE_CGEE                0x1
#define CLM_CONFIG_MODE_MIXED               0x2
#define CLM_CONFIG_MODE_MASK                0xF
/* Operating mode : SIF STORAGE Modes */
#define CLM_CTRL_MODE_STORAGE_NONE           0x00
#define CLM_CTRL_MODE_STORAGE_PERIODIC_ALL   0x10
#define CLM_CTRL_MODE_STORAGE_ON_DEMAND      0x20
#define CLM_CTRL_MODE_STORAGE_ON_STOP        0x30
#define CLM_CTRL_MODE_STORAGE_MASK           0xF0

/*File formats*/
#define CLM_CONFIG_FORMAT_UNKNOWN           0x0
#define CLM_CONFIG_OTA_FF1_STORAGE_FF1      0x1
#define CLM_CONFIG_OTA_FF2_STORAGE_FF3      0x2
#define CLM_CONFIG_OTA_FF2_STORAGE_FF4      0x3
#define CLM_CONFIG_OTA_FF2_STORAGE_FF4_COMP 0x4

/*CGEE Input Method*/
#define CLM_CONFIG_INPUT_UNKNOWN            0x0
#define CLM_CONFIG_INPUT_BE_IONO            0x1
#define CLM_CONFIG_INPUT_SUBFRAME           0x2
#define CLM_CONFIG_INPUT_MASK                0xF

/* External GPS Time Souce Availability */
#define CLM_CONFIG_INPUT_EXT_GPSTIME_AVAIL  0x00
#define CLM_CONFIG_INPUT_EXT_GPSTIME_NONE   0x10
#define CLM_CONFIG_INPUT_EXT_GPSTIME_MASK   0xF0

/*Debug*/
#define CLM_CONFIG_DEBUG_CONSOLE            0x1
#define CLM_CONFIG_DEBUG_FILE               0x0

/* Macros for CLM_EE_AGE structure Fields */
#define CLM_EE_AGE_EPHFLAG_INVALID            (0x0)
#define CLM_EE_AGE_EPHFLAG_BE                 (0x1)
#define CLM_EE_AGE_EPHFLAG_SGEE               (0x2)
#define CLM_EE_AGE_EPHFLAG_CGEE               (0x3)

/**
 * Maximum number of satellites
 */
#define MAX_NUM_SV               32
#define MAX_PATH_NAME            256

/* Maximum length of SGEE Module version string */
#define MAX_LEN_SGEE_VERSION_STRING     32

#ifdef EMB_SIF
/* OTA File Header Size: To be used by SGEE module and EE file downloader */
#define     MAX_SIZE_OF_CONVET_BUFFER_EMB   (2000)
#define     CONVERT_OTA_LARGESTCHUNK_EMB    (2000)

/* Maximum size of version string to be reported by CLM */
#define     MAX_CLM_VERSION_STRING      64



/* should be atleast 1 day for storage one of the documented compile-time parameters used by CGEE */
#define MAX_CGEE_PREDICT_DAYS                   ( 3 )

#define MAX_AVAILABLE_SGEE_DAYS 31

/* Maximum number of days which can be stored in storage, supplied age limit shouldnot be more than this value */
/* This also reflects space required for SGEE clock storage in all storages */
/* NOTE:I2C has only 64KB of storage allocated for SGEE */
#ifdef SIRF_INSTANT_FIX_MAX_SGEE_DAYS
    #if ((SIRF_INSTANT_FIX_MAX_SGEE_DAYS == 7) ||\
        (SIRF_INSTANT_FIX_MAX_SGEE_DAYS == 14) ||\
        (SIRF_INSTANT_FIX_MAX_SGEE_DAYS == 31))
        #define MAX_EPHEMERIS_DAYS_IN_STORAGE   SIRF_INSTANT_FIX_MAX_SGEE_DAYS
    #else
        #error Incorrect value of SIRF_INSTANT_FIX_MAX_SGEE_DAYS defined in pvt_common.mak. It must be either 7,14 or 31.
    #endif
#else
    #define MAX_EPHEMERIS_DAYS_IN_STORAGE   MAX_AVAILABLE_SGEE_DAYS
#endif

/* Minimum header data required before start processing downloaded SGEE file */
#define MIN_HEADER_REQ_LEN                  18

#define DAYS_PER_WEEK 7

#define MAX_SGEE_WEEKS         (ROUND_I32(MAX_EPHEMERIS_DAYS_IN_STORAGE/DAYS_PER_WEEK))

#else  /* EMB_SIF */

#define CLM_CONFIG_AGE_LIMIT_UNKNOWN         0x0

/* OTA File Header Size: To be used by SGEE module and EE file downloader */
#define     OTA_FILE_HEADER_SIZE_FF1    26
#define     OTA_FILE_HEADER_SIZE_FF2    38
#define     MAX_SIZE_OF_CONVET_BUFFER   (6000)
#define     CONVERT_OTA_LARGESTCHUNK    (3000)

/* Maximum size of version string to be reported by CLM */
#define     MAX_CLM_VERSION_STRING      256

/* Maximum length of CGEE Module version string */
#define MAX_LEN_CGEE_VERSION_STRING     32
/* Maximum length of MTL Module version string */
#define MAX_LEN_MTL_VERSION_STRING      32

#define MAX_AVAILABLE_CGEE_DAYS                   3

#define MAX_AVAILABLE_SGEE_DAYS                   7

#ifdef SIRF_INSTANT_FIX_MAX_SGEE_DAYS
    #if (SIRF_INSTANT_FIX_MAX_SGEE_DAYS > 0) && (SIRF_INSTANT_FIX_MAX_SGEE_DAYS <= MAX_AVAILABLE_SGEE_DAYS)
        #define INTERNAL_SIRF_INSTANT_FIX_MAX_SGEE_DAYS SIRF_INSTANT_FIX_MAX_SGEE_DAYS
    #else
        #error Incorrect value of SIRF_INSTANT_FIX_MAX_SGEE_DAYS defined in the project. It must be between 1 and MAX_AVAILABLE_SGEE_DAYS.
    #endif
#else
    #define INTERNAL_SIRF_INSTANT_FIX_MAX_SGEE_DAYS   MAX_AVAILABLE_SGEE_DAYS
#endif

#ifdef SIRF_INSTANT_FIX_MAX_CGEE_DAYS
    #if (SIRF_INSTANT_FIX_MAX_CGEE_DAYS > 0) && (SIRF_INSTANT_FIX_MAX_CGEE_DAYS <= MAX_AVAILABLE_CGEE_DAYS)
        #define INTERNAL_SIRF_INSTANT_FIX_MAX_CGEE_DAYS SIRF_INSTANT_FIX_MAX_CGEE_DAYS
    #else
        #error Incorrect value of SIRF_INSTANT_FIX_MAX_CGEE_DAYS defined in the project. It must be between 1 and MAX_AVAILABLE_CGEE_DAYS.
    #endif
#else
    #define INTERNAL_SIRF_INSTANT_FIX_MAX_CGEE_DAYS   MAX_AVAILABLE_CGEE_DAYS
#endif

/* Maximum number of days which can be stored in storage, supplied age limit shouldnot be more than this value */
#define MAX_EPHEMERIS_DAYS_IN_STORAGE      ( INTERNAL_SIRF_INSTANT_FIX_MAX_SGEE_DAYS > INTERNAL_SIRF_INSTANT_FIX_MAX_CGEE_DAYS ? \
                                             INTERNAL_SIRF_INSTANT_FIX_MAX_SGEE_DAYS : INTERNAL_SIRF_INSTANT_FIX_MAX_CGEE_DAYS)



/* Returned by CLM_GetStatus() when CLM is in normal mode */
#define CLM_STATE_NORMAL            0
/* Returned by CLM_GetStatus() when CLM is in prediction mode */
#define CLM_STATE_IN_PREDICTION        1

#endif /* EMB_SIF */
/**
 * Used to represent the status of an operation.
 */
/**
 * @enum tCLM_STATUS
 * @brief tCLM_STATUS
 *
 * @description Used to represent the status of an operation.
 */
typedef enum {

    CLM_SUCCESS = 0,            /*  Operation completed successfully */
    CLM_GENERIC_FAILURE = -1,   /*  Operation encountered generic failure */

    CLM_COMMON_STATUS_BASE = 0x01000000,
    CLM_COMMON_INVALID_CONFIG,      /* Input configuration is invalid. Normally during a call to CLM_Config() */
    CLM_COMMON_INVALID_PARAM,       /* Input parameters are invalid for requested operation */
    CLM_COMMON_INVALID_OPS,         /* Requested operation is invalid, most probably due to mismatched configuration or unsupported operation.*/
    CLM_COMMON_ALREADY_STARTED,     /* CLM start is attempted when it is already started. */
    CLM_COMMON_NOT_RUNNING,         /* Operation is attempted when CLM is not started. */
    CLM_COMMON_NOT_CONFIGURED,      /* Operation is attempted when CLM is not configured.*/
    CLM_COMMON_INVALID_MODULE_ID,   /* Invalid Module ID is input during registering its operations with CLM.*/
    CLM_COMMON_INCOMPLETE_OPS,      /* One or more of the mandatory operations are missing in operation registration structure.*/
    CLM_COMMON_ALREADY_REGISTERED,  /* Input module ID has already registered its operations with CLM.*/
    CLM_COMMON_INVALID_TIME,        /* Input GPS time is invalid.*/
    CLM_COMMON_SGEE_DOWNLOAD_IN_PROGRESS,        /* SGEE Download in Progress, No access to EE Storage File.*/
    CLM_COMMON_NO_VALID_EE,        /* No Valid EE data Data available for any of the 32 Sats for requested Time.*/

    CLM_STORAGE_STATUS_BASE = 0x02000000,
    CLM_STORAGE_INVALID_CONFIG,     /* Input storage configuration is invalid.*/
    CLM_STORAGE_INVALID_PARAM,      /* Input parameters are invalid for requested storage operation.*/
    CLM_STORAGE_ALREADY_INITIALISED,    /* Storage module is already initialized.*/
    CLM_STORAGE_NOT_RUNNING,        /* Storage operation is attempted when it is not initialized.*/
    CLM_STORAGE_CORRUPTFILE,        /* Storage file is corrupted.*/
    CLM_STORAGE_READ_ERROR,         /* Storage operation could not read from storage file.*/
    CLM_STORAGE_WRITE_ERROR,        /* Storage operation could not write to storage file.*/
    CLM_STORAGE_VERSION_MISMATCH,   /* Storage file version string is corrupted or storage
                                       module is initialized with different than existing version
                                       attributes (like different storage format, age limit, etc.)*/
    CLM_STORAGE_CONTINUE_OK,        /* Not an error:SGEE update record is successful, and updating
                                       further records, if any, can be continued. It is not an error. */
    CLM_STORAGE_HOST_OFFSET_INVALID,/* Storage does not have updated host time offsets.*/
    CLM_STORAGE_NO_VALID_BE,        /* No valid BE found in Storage.*/
    CLM_STORAGE_WAIT_STR_CMP,       /* Actual storage update is still pending*/
    CLM_STORAGE_COMPLETE,           /* Storage update finish*/
    CLM_STORAGE_INVALID_OPS,        /* Requested operation is invalid, most probably due to mismatched configuration or unsupported operation.*/
    CLM_STORAGE_SGEE_DOWNLOAD_IN_PROGRESS, /* SGEE Download in Progress, No access to EE Storage File.*/
    CLM_STORAGE_CGEE_ENGINE_BUSY,   /* CGEE Engine busy in doing prediction*/
    CLM_STORAGE_SGEE_PREDICT_INTERVAL_INVALID,   /* SGEE Prediction Interval exceeded allowed Max SGEE Days limit */
    CLM_PAL_STATUS_BASE = 0x03000000,
    CLM_PAL_RESOURCE_ERROR,         /* PAL returns Error due to lack of resources e.g. failing to
                                       create/take/release mutex, semaphore or error in storage
                                       open/read/write/close operation etc.*/

    CLM_CGEE_STATUS_BASE = 0x04000000,
    CLM_CGEE_INVALID_CONFIG,        /* Input CGEE configuration data is not valid*/
    CLM_CGEE_INVALID_PARAM,         /* Input parameters are invalid for the requested CGEE operation.*/
    CLM_CGEE_ALREADY_STARTED,       /* CGEE module has already been initialized.*/
    CLM_CGEE_NOT_RUNNING,           /* CGEE operation is attempted before CGEE is started.*/
    CLM_CGEE_CLEANUP_NOT_ALLOWED,   /* CGEE operation is not allowed to cleanup as either
                                       CGEE module is not initialized or started. */
    CLM_CGEE_RETURNED_EE,           /* CGEE is returning EE data.*/
    CLM_CGEE_NOTNEWEPHEMERIS,       /* CGEE engine is supplied with old Ephemeris.*/
    CLM_CGEE_NOEPHEMERIS,           /* CGEE Engine is not having any Ephemeris.*/
    CLM_CGEE_RETURNED_BE,           /* CGEE is returning BE data.*/
    CLM_CGEE_NOMORECGEE,            /* CGEE engine is not giving more CGEE data.*/
    CLM_CGEE_INCORRECT_TOE,         /* CGEE has encountered invalid time of Ephemeris passed to it.*/
    CLM_CGEE_DIVIDEBYZERO,          /* CGEE has encountered divide by zero error.*/
    CLM_CGEE_OUTOFMEMORY,           /* CGEE has encountered out of memory error.*/
    CLM_CGEE_BADINTEGRTIYCHECK,     /* CGEE has encountered integrity check failure.*/
    CLM_CGEE_KEPLERFITFAILED,       /* CGEE has encountered failure in fitting Kepler parameters.*/
    CLM_CGEE_SHUTDOWN_INITIATED,    /* CGEE process shutdown is initiated*/
    CLM_CGEE_FACTORY_RESET_REQ,     /* CGEE process factory reset is initiated*/

    CLM_SGEE_STATUS_BASE = 0x05000000,
    CLM_SGEE_INVALID_CONFIG,        /* Input SGEE configuration data is not valid*/
    CLM_SGEE_INVALID_PARAM,         /* Input parameters are invalid for the requested SGEE operation.*/
    CLM_SGEE_ALREADY_STARTED,       /* SGEE has already started and requested operation is not permitted.*/
    CLM_SGEE_NOT_RUNNING,           /* Operation is attempted when SGEE module is not started.*/
    CLM_SGEE_CLEANUP_NOT_ALLOWED,   /* SGEE cleanup is not allowed possibly because it is not stopped.*/
    CLM_STORAGE_SGEE_NONEWFILE,     /* Input SGEE file is not new and hence not used for extracting EE data*/
    CLM_SGEE_INVALIDFILE,           /* Input SGEE file is invalid, probably incomplete, or not in
                                       supported/configured OTA format */
    CLM_SGEE_CORRUPTFILE,           /* Input SGEE file is corrupted, checksum is not valid */
    CLM_SGEE_PARSE_ERROR,           /* Input SGEE file is corrupted, parser failed */
    CLM_SGEE_FF3_SIZE_ERROR,        /* Size of FF3 file is not matching with expected size.*/

    CLM_MTL_STATUS_BASE = 0x06000000,
    CLM_MTL_INVALID_CONFIG,         /* Input configuration parameters passed are invalid.*/
    CLM_MTL_INVALID_PARAM,          /* Input parameters are invalid for the requested MTL operation.*/
    CLM_MTL_ALREADY_INITIALISED,    /* MTL has already been initialized.*/
    CLM_MTL_NOT_INITIALISED,        /* Operation is attempted when MTL module is not initialized.*/
    CLM_MTL_INVALID_MSG_LENGTH,     /* MTL is passed with invalid length message */
    CLM_MTL_UNSUPPORTED_MID,        /* MTL is passed with unsupported MID */

    CLM_DBG_STATUS_BASE = 0x07000000,
    CLM_DBG_INVALID_CONFIG,         /* Input configuration parameters are invalid.*/
    CLM_DBG_INVALID_PARAM,          /* Input parameters are invalid for the requested operation.*/
    CLM_DBG_ALREADY_INITIALSED,     /* Debug module has already been initialized.*/
    CLM_DBG_NOT_RUNNING,            /* Operation is attempted when debug module is not initialized.*/
    CLM_DBG_WRITE_ERROR,            /* Write failure in logging debug message to log file.*/
    CLM_DBG_CONSOLE_ERROR           /* Write failure in writing debug message to console.*/

}tCLM_STATUS;

/**
 * Used to specify the various CLM configuration options.
 */
/**
 * @struct tCLM_CONFIG
 * @brief tCLM_CONFIG
 *
 * @description Used to specify the various CLM configuration options.
 */
typedef struct{
    /*CLM operation mode to specify SIF operation mode and SIF storage mode*/
    tSIRF_UINT8 operationMode;  /*(Lower 4 bits of operationMode :
                                                Bit0 - Bit3   : SIF Operation Mode
                                                      0       : SGEE mode,
                                                      1       : CGEE mode,
                                                      2       : SGEE-CGEE-MIXED mode)|
                                  (Upper 4 bits of operationMode:
                                                Bit4 - Bit7   : SIF Storage Mode
                                                     0        : CLM_CTRL_MODE_STORAGE_NONE,
                                                     1        : CLM_CTRL_MODE_STORAGE_PERIODIC_ALL,
                                                     2        : CLM_CTRL_MODE_STORAGE_ON_DEMAND,
                                                     3        : CLM_CTRL_MODE_STORAGE_ON_STOP)*/

    /*OTA and storage file format:*/
    tSIRF_UINT8 fileFormat;     /*0: Unknown, 1: FF1(OTA and Storage), 2: FF2-OTA and FF3-Storage,3:FF2-OTA & FF4-Storage*/

    /*Age limit in days for SGEE file : Chopped, if exceeds, to maximum storage size in days (at present 7 days)*/
    tSIRF_UINT8 sgeeAgeLimit;

    /*Age limit in days for CGEE mode: Chopped, if exceeds, to maximum storage size in days (at present 7 days)*/
    tSIRF_UINT8 cgeeAgeLimit;

    /*Input method for CGEE generation */
    tSIRF_UINT8 cgeeInputMethod;    /* Bit0-Bit3(lower 4 bits) : (Input BE Method for CGEE prediction)
                                          0: Unknown, 1: BE and Ionosphere data, 2: Sub frame data
                                       Bit4-Bit7(upper 4 bits) : (External GPS Source Availability)
                                            0: External GPS time source available , Validate Time in BE when using SubFrame,
                                            1: External GPS time source NOT available, Do Not Validate Time in BE when using SubFrame */

    /*Bit map for debug options*/
    /*
     Bit10-29: Corresponding 2 bit tells severity for the sub module
     Bit0-9  : Corresponding bit for sub Module log (1 : enabled) (0: Disabled )
     Bit30   : 1->logging is enable for all modules with common severity level (bit:10-11)
               0->logging is enabled as per module ID(bit:0-9) with severity level (bit:10-29)
     Bit31   : 1->logging is disabled for all modules
               0->logging is enabled as per module ID(bit:0-9) with severity level (bit:10-29 or 30)
     Default : 0x0
    */
    tSIRF_UINT32 debugOptions;

    tSIRF_UINT8 debugConsole;   /*1: Logging on console*/
                                /*0: Logging in a file*/

    /* Input name for Log File, max length can be MAX_PATH_NAME = 256 char ,Use NULL for CLM generated Name*/
    tSIRF_CHAR  logFileName[MAX_PATH_NAME];
}tCLM_CONFIG;

/**
 * @struct tCLM_EMB_CONFIG
 * @brief tCLM_EMB_CONFIG
 *
 * @description Used to specify the various CLM configuration options.
 */
typedef struct{

    /*OTA and storage file format:*/
    tSIRF_UINT8 fileFormat;     /*0: Unknown, 1: FF1(OTA and Storage), 2: FF2-OTA and FF3-Storage,3:FF2-OTA & FF4-Storage
                                                         4:FF2-OTA & FF4_COMP-Storage*/

    /*Age limit in days for SGEE file : Chopped, if exceeds, to maximum storage size in days (at present 31 days)*/
    tSIRF_UINT8 sgeeAgeLimit;


}tCLM_EMB_CONFIG;

/**
 * Used to represent the ephemeris (SGEE, EE or CGEE) age of a satellite available with CLM.
 */
/**
 * @struct tCLM_EE_AGE
 * @brief tCLM_EE_AGE
 *
 * @description Used to represent the ephemeris (SGEE, EE or CGEE) age
 *              of a satellite available with CLM.
 */
typedef struct {

    /*PRN number of satellite for which age is indicated in other fields.*/
    tSIRF_UINT8 prnNum;

    /*Ephemeris flag to indicate the type of ephemeris available for the satellite:(Position Age)*/
    tSIRF_UINT8 ephPosFlag;    /*0: Invalid ephemeris, not available, 1: BE, 2: SGEE, 3: CGEE*/

    /*Age of EE in 0.01 days (Position Age)*/
    tSIRF_UINT16 eePosAge;

    /*GPS week of BE used in the CGEE generation; 0 if ephPosFlag is not set to 3 or set to 0.(Position Age)*/
    tSIRF_UINT16 cgeePosGPSWeek;

    /*TOE of BE used in the CGEE generation; 0 if ephPosFlag is not set to 3.or set to 0(Position Age)*/
    tSIRF_UINT16 cgeePosTOE;

    /*Ephemeris flag to indicate the type of ephemeris available for the satellite:(Clock Age)*/
    tSIRF_UINT8 ephClkFlag;    /*0: Invalid ephemeris, not available, 1: BE, 2: SGEE, 3: CGEE*/

    /*Age of EE in 0.01 days(Clock Age) */
    tSIRF_UINT16 eeClkAge;

    /*GPS week of BE used in the CGEE generation; 0 if ephClkFlag is not set to 3 or set to 0.(Clock Age)*/
    tSIRF_UINT16 cgeeClkGPSWeek;

    /*TOE of BE used in the CGEE generation; 0 if ephClkFlag is not set to 3.or set to 0(Clock Age)*/
    tSIRF_UINT16 cgeeClkTOE;
}tCLM_EE_AGE;


/**
 * Used to represent the position, clock and health of 32 satellites.
 */
/**
 * @struct tCLM_INTEGRITY_WARNING
 * @brief tCLM_INTEGRITY_WARNING
 *
 * @description Used to represent the position, clock and health of 32 satellites.
 */
typedef struct {

    /*32-bit long bitmap, with a bit set indicating invalid position of corresponding
      satellite (bit0 – SATID1, bit31: SATID-32).                                   */
    tSIRF_UINT32    positionValidity;

    /*32-bit long bitmap, with a bit set indicating invalid clock of corresponding
      satellite (bit0 – SATID1, bit31: SATID-32).                                   */
    tSIRF_UINT32    clockValidity;

    /*32-bit long bitmap, with a bit set indicating invalid health of corresponding
      satellite (bit0 – SATID1, bit31: SATID-32).                                   */
    tSIRF_UINT32    healthValidity;

}tCLM_INTEGRITY_WARNING;



/**
 * Used to represent the clock bias and adjustment values for a satellite.
 */
/**
 * @struct tCLM_CLK_BIAS_ADJ
 * @brief tCLM_CLK_BIAS_ADJ
 *
 * @description Used to represent the clock bias and adjustment values for a satellite.
 */
typedef struct {

    /*PRN number of satellite*/
    tSIRF_UINT8 prnNum;

    /*Clock bias value for this settalite */
    tSIRF_INT32    clockBiasAdj;

    /*TOE of this satellite*/
    tSIRF_UINT16    TOE;

}tCLM_CLK_BIAS_ADJ;

/**
 * Used to represent the extended ephmeris data provided by CLM.
 */
/**
 * @struct tCLM_EE_DATA
 * @brief tCLM_EE_DATA
 *
 * @description Used to represent the extended ephmeris data provided by CLM.
 */
typedef struct {
    /*PRN number of satellite*/
    tSIRF_UINT8   prn;

    /*0: Invalid ephemeris, 1: BE, 2: SGEE/CGEE/MIXED*/
    tSIRF_UINT8   flag;

    /* As per the standard definitions of ephmeris data terms */
    tSIRF_UINT8   uraInd;
    tSIRF_UINT8   iode;
    tSIRF_INT16  crs;
    tSIRF_INT16  deltaN;
    tSIRF_INT32  m0;
    tSIRF_INT16  cuc;
    tSIRF_UINT32  eccentricity;
    tSIRF_INT16  cus;
    tSIRF_UINT32  asqrt;
    tSIRF_UINT16  toe;
    tSIRF_INT16  cic;
    tSIRF_INT32  omega0;
    tSIRF_INT16  cis;
    tSIRF_INT32  angIncline;
    tSIRF_INT16  crc;
    tSIRF_INT32  omega;
    tSIRF_INT32  omegaDot;
    tSIRF_INT16  idot;
    tSIRF_UINT16  toc;
    tSIRF_INT8   tgd;
    tSIRF_INT8   af2;
    tSIRF_INT16  af1;
    tSIRF_INT32  af0;

    /*Age of the SGEE or CGEE ephemeris*/
    tSIRF_UINT8   age;
} tCLM_EE_DATA;

/**
 * Used to represent the sub frame data of a satellite.
 */
/**
 * @struct tCLM_SF_DATA
 * @brief tCLM_SF_DATA
 *
 * @description Used to represent the sub frame data of a satellite.
 */
typedef struct {

    /*PRN number of satellite*/
    tSIRF_UINT8 prnNum;

    /*10-Words of sub frame data*/
    tSIRF_UINT32 subFrameData[10];
}tCLM_SF_DATA;

/**
 * Used to represent the GPS time Information.
 */
/**
 * @struct tCLM_GPS_TIME_INFO
 * @brief tCLM_GPS_TIME_INFO
 *
 * @description Used to represent the GPS time Information.
 */
typedef struct {

    /* GPS week number in weeks                   */
    tSIRF_UINT16 gpsWeek;

    /* GPS time of week in 10 ms; 0..60,479,900 ms */
    tSIRF_UINT32 gpsTow;

    /* number of SVs used, 0..12                   */
    tSIRF_UINT8  svUsedCnt;

    /* clock Drift in Hz                          */
    tSIRF_UINT32 clkOffset;

    /* clock Bias in nanoseconds                  */
    tSIRF_UINT32 clkBias;

    /* estimated gps time in milliseconds         */
    tSIRF_UINT32 estGpsTime;

}tCLM_GPS_TIME_INFO;

/**
 * Used to represent the ionosphere correction data.
 */
/**
 * @struct tCLM_IONO_DATA
 * @brief tCLM_IONO_DATA
 *
 * @description Used to represent the ionosphere correction data.
 */
typedef struct {

    tSIRF_INT8  alpha0;
    /*  As per the standard definitions of ionosphere correction terms*/
    tSIRF_INT8  alpha1;
    tSIRF_INT8  alpha2;
    tSIRF_INT8  alpha3;
    tSIRF_INT8  beta0;
    tSIRF_INT8  beta1;
    tSIRF_INT8  beta2;
    tSIRF_INT8  beta3;
}tCLM_IONO_DATA;

/**
 * Used to represent the broadcast ephemeris data.
 */
/**
 * @struct tCLM_BE_DATA
 * @brief tCLM_BE_DATA
 *
 * @description Used to represent the broadcast ephemeris data.
 */
typedef struct {

    /*flag   0: Invalid 1: Valid*/
    tSIRF_UINT8     flag;

    /*PRN number of satellite*/
    tSIRF_UINT8     prn;

    /*GPS week number of the ephemeris*/
    tSIRF_UINT16    gpsWeek;

    tSIRF_UINT8     uraInd;

    /*as per the standard definitions of ionosphere correction terms*/
    tSIRF_UINT8     iode;
    tSIRF_INT16     crs;
    tSIRF_INT16     deltaN;
    tSIRF_INT32     m0;
    tSIRF_INT16     cuc;
    tSIRF_UINT32    eccentricity;
    tSIRF_INT16     cus;
    tSIRF_UINT32    asqrt;
    tSIRF_UINT16    toe;
    tSIRF_INT16     cic;
    tSIRF_INT32     omega0;
    tSIRF_INT16     cis;
    tSIRF_INT32     angIncline;
    tSIRF_INT16     crc;
    tSIRF_INT32     omega;
    tSIRF_INT32     omegaDot;
    tSIRF_INT32     idot;
    tSIRF_UINT16    toc;
    tSIRF_INT8      tgd;
    tSIRF_INT8      af2;
    tSIRF_INT16     af1;
    tSIRF_INT32     af0;
}tCLM_BE_DATA;

/**
 * @struct tCLM_CGEE_STATUS
 * @brief tCLM_CGEE_STATUS
 *
 * @description Used to describe the CGEE Engine Prediction status.
 */
typedef struct{
   tSIRF_CHAR     state;                               /* CGEE Prediction engine state */
   tSIRF_UINT32   satPredictedMask;                    /* Satellites predicted */
   tSIRF_UINT32   satPredictionPendingMask;            /* Satellites prediction pending  */
} tCLM_CGEE_STATUS;

/**
 * @struct tCLM_CGEE_PREDICTION_INFO
 * @brief tCLM_CGEE_PREDICTION_INFO
 *
 * @description Used to get the CGEE prediction info TOE and week number for 32 satellites.
 */
typedef struct{
   tSIRF_UINT32 lastTOE[MAX_NUM_SV];
   tSIRF_UINT16 weekNum[MAX_NUM_SV];
   tSIRF_UINT8 svidInPrediction;
} tCLM_CGEE_PREDICTION_INFO;


#endif /*__CLM_STRUCTS_H__ */

/**
 * @}
 * End of file.
 */

