/**
 * @addtogroup lpl3.0
 * @{
 */


/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, a CSR plc Company                    *
 *                           GPS Software                                  *
 *                                                                         *
 *    Copyright (c) 2007-2010 by SiRF Technology, a CSR plc Company        *
 *                        Inc. All rights reserved.                        *
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.s confidential and       *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.s  express written            *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 ***************************************************************************
 *
 * MODULE:  LPLCore
 *
 * FILENAME: AGPS.h
 *
 * DESCRIPTION:  LPL API Header file
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/SiRFLPL/src/include/AGPS.h $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************
*/

#ifndef __AGPS_H__
#define __AGPS_H__

#include "SiRFstudio.h"
#ifdef SIRF_AGPS
#include "sirf_errors.h"
#endif
/**************************************************** */
/** This Defines what version of the AI3 ICD to use   */
/**************************************************** */

#define AI3_REV_NUM_16          0x16
#define AI3_REV_NUM_19          0x19
#define AI3_REV_NUM_20          0x20
#define AI3_REV_NUM_21          0x21
#define AI3_REV_NUM_22          0x22

#define F_REV_NUM_19            0x19
#define F_REV_NUM_20            0x20
#define F_REV_NUM_21            0x21

#define MAX_NI_MRSMENTS         10
#define MAX_NI_CELLS            16

#define MIN_SVLIMIT_REAIDING    6

#define SIRF_DEFAULT_FREQ_MAX_REQUEST_TIME  0
#define SIRF_DEFAULT_FREQ_REQUEST_PERIOD    0

#define SIRF_DEFAULT_MAX_REQUEST_TIME   1000       /* in millisesonds */
#define SIRF_DEFAULT_REQUEST_PERIOD     100        /* in millisesonds */

#define SIRF_FREQ_PARAM_OUT_OF_RANGE_ERROR  0x02

/* The following are defined in SUPL 1.0 specification */
#define MAX_TIME_SLOT 14
#define MAX_CELL_MEAS 32
#define MAX_FREQ 8

#ifdef DLL
#ifndef DLL_EXPORT
#define DLL_EXPORT
#endif /*DLL_EXPORT*/
#endif /* DLL */

/**
* SET Identification Types
*/
enum SET_IDENTIFICATION
{
   /** Indicates that IMSI is to be used */
   IMSI,

   /** Indicates that MSISDN is to be used */
   MSISDN
};

/**
* Location Method Types
*/
enum LOCATION_METHOD
{
   /** Indicates that the SUPL Autonomous Location Method is to be used */
   LOCATION_METHOD_AUTONOMOUS,

   /** Indicates that the MS-Based (Network Assisted) Location Method is to be used
   and Measure Position Request OTA */
   LOCATION_METHOD_MS_BASED,

   /** Indicates that the MS-Assisted (Network Centric) Location Method is to be used */
   LOCATION_METHOD_MS_ASSISTED,

   /** Indicates that a combination of methods is to be used. The final Location method
   *   will be determined by the handshaking between SET and SLC
   */
   LOCATION_METHOD_COMBO,

   /** Indicates that the Standalone Location Method is to be used */
   LOCATION_METHOD_STANDALONE,

   /** Indicates that the Enhanced Cell ID Location Method is to be used */
   LOCATION_METHOD_ECID
};

/**
* SUPL2.0 Triger Types
*/
typedef enum
{
   /** No trigger, normal session */
   TRIGGER_TYPE_NONE,

   /** Periodic trigger is used */
   TRIGGER_TYPE_PERIODIC,

   /** Event trigger is used */
   TRIGGER_TYPE_EVENT
}   eTriggerType;

/**
* Location Preferred Method Types
*/
enum LOCATION_PREFERRED_METHOD
{
   /** Indicates that the MS-Based (Network Assisted) Location Method is preferred */
   LOCATION_PREFERRED_METHOD_MS_BASED,

   /** Indicates that the MS-Assisted (Network Centric) Location Method is preferred */
   LOCATION_PREFERRED_METHOD_MS_ASSISTED,

   /** Indicates that no Location Method is preferred */
   LOCATION_PREFERRED_METHOD_NONE
};


/**
* The types of major LPL Progress states
*/

typedef enum
{
   /* ====== Network socket related status ======= */
   /**
       Network socket connection closed, which is normal termination.
   */
   ENetworkConnectionClose = 0,

   /**
    Network socket connection information just before creating a socket connection.
   */
   ENetworkConnectionResume,

   /**
       Network socket connection failed when creating a socket connection
   */
   ENetworkConnectionFailed,

   /**
       Network socket read error, which could mean the socket is closed involuntarily
   */
   ENetworkReadError,

   /**
       Network socket write error, which could mean the socket is closed involuntarily
   */
   ENetworkWriteError,

   /* ====== SUPL message flow related status ======= */
   /**
       Time out occurred between two SUPL Messages
   */
   ESUPLTimeOut,
   /**
       SUPL_END w/o status code is received.
   */
   ESUPL_END_Received,

   EHWConfigRequestReceived,

   EGPSResetSent,

   /* last Control Plane message is sent */
   ELastCPMsgSent,

   /* ======= Invalid state ======= */
   EInvalidProgState
} SiRFstudioProgressStates;

/*
* eAssistProtocol can be user-plane; control-plane (RRC/RRC_SIB15 or RRLP); AGPS
assistance request (for msg from LPL to CP only, for SS MO-LR)
*/
typedef enum eAssistProtocolTag
{
   SUPL_RRLP_OR_RRC,
   CP_RRLP,
   CP_RRC,
   RRC_SIB15,
   SS_AGPS_ASSIST_REQ,
   TEXT_AIDING,
   EE_PUSH,
#ifdef SUPL_FRAMEWORK
   SUPL_FRAMEWORK_RRLP,
   SUPL_FRAMEWORK_RRC,
   SUPL_FRAMEWORK_TIA801,
#endif /*SUPL_FRAMEWORK*/
   INVALID_OTA_TYPE = (~0)
}   eAssistProtocol;

typedef enum
{
   RRC_STATE_CELL_DCH,
   RRC_STATE_CELL_FACH,
   RRC_STATE_CELL_PCH,
   RRC_STATE_URA_PCH,
   RRC_STATE_IDLE,
   RRC_INVALID_STATE
}   LPL_RRCState;

typedef enum
{
   RRC_MEAS_REPORT = 0,
   RRC_MEAS_FAILURE,
   RRC_STATUS
}   LPL_ULRRC__MSG_TYPE;

typedef struct LPL_lplOtaParams
{
   LPL_ULRRC__MSG_TYPE ul_smg_type;
   struct
   {   /* valid if msg_type = RRC_MEAS_REPORT */
      tSIRF_UINT8 measurement_identity;
      /* indicate if p_msg_data & msg_size contains measurement
       * m//results (1) or event results (0) */
      tSIRF_UINT8 measurement_or_event;
   }   meas_report;
   tSIRF_BOOL is_final_response;
}   LPL_lplOtaParams;

/**
 * The setCapabilities structure contains the SET capabilities,
 * that is the positioning methods the SET is supporting. This structure is
 * to be used when the positioning method selected by the user is LOCATION_METHOD_COMBO. </p>
 */
typedef struct SETcapabilities
{
   /**
    * This indicates that the SET supports MS Based positioning method
    */
   tSIRF_UINT8   msBased;
   /**
    * This indicates that the SET supports MS Assisted positioning method
    */
   tSIRF_UINT8   msAssisted;
   /**
    * This indicates that the SET supports Autonomous positioning method
    */
   tSIRF_UINT8   autonomous;

   /**
   * This indicates that the SET supports Enhanced Cell ID positioning method
   */
   tSIRF_UINT8      eCID;
} SETcapabilities;

/**
 * The AGPS_QoP structure contains the QoP for SUPL_INIT messages.
 */
typedef struct AGPS_QoP
{
   /**
    * This indicates what information is present in the structure
    */
   struct
   {
      /**
      * This indicates that Vertical Accuracy is present
      */
      unsigned veraccPresent : 1;
      /**
      * This indicates that Max Loc Age is present
      */
      unsigned maxLocAgePresent : 1;
      /**
      * This indicates that Delay is present
      */
      unsigned delayPresent : 1;
   } m;
   /**
   * Horizontal Accuracy value
   */
   tSIRF_UINT8 horacc;
   /**
   * Vertical Accuracy value
   */
   tSIRF_UINT8 veracc;
   /**
   * Max Loc Age value
   */
   tSIRF_UINT16 maxLocAge;
   /**
   * Delay value
   */
   tSIRF_UINT8 delay;
} AGPS_QoP;

/**
* Positioning Method Types (for AGPS_SUPL_INIT structure)
*/
typedef enum
{
   AGPS_agpsSETassisted = 0,
   AGPS_agpsSETbased = 1,
   AGPS_agpsSETassistedpref = 2,
   AGPS_agpsSETbasedpref = 3,
   AGPS_autonomousGPS = 4,
   AGPS_aFLT = 5,
   AGPS_eCID = 6,
   AGPS_eOTD = 7,
   AGPS_oTDOA = 8,
   AGPS_noPosition = 9
} AGPS_PosMethod;

/**
* Notification Types (for AGPS_SUPL_INIT structure)
*/
typedef enum
{
   AGPS_noNotificationNoVerification = 0,
   AGPS_notificationOnly = 1,
   AGPS_notificationAndVerficationAllowedNA = 2,
   AGPS_notificationAndVerficationDeniedNA = 3,
   AGPS_privacyOverride = 4
} AGPS_NotificationType;

/**
* Requestor ID and Client ID Types (for AGPS_SUPL_INIT structure)
*/
typedef enum
{
   AGPS_logicalName = 0,
   AGPS_e_mailAddress = 1,
   AGPS_msisdn = 2,
   AGPS_url = 3,
   AGPS_sipUrl = 4,
   AGPS_min = 5,
   AGPS_mdn = 6
} AGPS_FormatIndicator;

/**
 * The AGPS_Notification structure contains the Notification for
 * SUPL_INIT messages. 
 */
typedef struct AGPS_Notification
{
   /**
    * This indicates what information is present in the structure
    */
   struct
   {
      /**
      * This indicates that Encoding Type is present
      */
      unsigned encodingTypePresent : 1;
      /**
      * This indicates that Requestor ID is present
      */
      unsigned requestorIdPresent : 1;
      /**
      * This indicates that Requestor ID Type is present
      */
      unsigned requestorIdTypePresent : 1;
      /**
      * This indicates that Client Name is present
      */
      unsigned clientNamePresent : 1;
      /**
      * This indicates that Client Name Type is present
      */
      unsigned clientNameTypePresent : 1;
   } m;
   /**
   * Notification type
   */
   tSIRF_UINT32  notificationType;

   /**
     encoding type
   **/
   tSIRF_UINT32 encodingType;

   /**
   * Requestor ID Length
   */
   tSIRF_UINT32 requestorIdLength;

   /**
   * Requestor ID
   */
   tSIRF_UINT8* requestorId;
   
   /**
   * Requestor ID Type
   */
   tSIRF_UINT32  requestorIdType;

   /**
   * Client Name Length
   */
   tSIRF_UINT32 clientNameLength;

   /**
   * Client Name
   */
   tSIRF_UINT8* clientName;

   /**
   * Client Name Type
   */
   tSIRF_UINT32  clientNameType;
}   AGPS_Notification;

/**
 * The AGPS_SUPL_INIT structure contains the SUPL_INIT information.
 * This structure is to be used by the SuplInitListener.
 */
typedef struct AGPS_SUPL_INIT
{
   /**
    * This indicates what fields are present in the message
    */
   struct
   {
      /**
      * This indicates that Notification is present
      */
      unsigned notificationPresent : 1;
      /**
      * This indicates that QoP is present
      */
      unsigned qoPPresent : 1;
   } m;
   /**
   * Positioning Method
   */
   tSIRF_UINT32              posMethod;
   /**
   * Notification structure
   */
   AGPS_Notification   notification;
   /**
   * QoP structure
   */
   AGPS_QoP            qoP;
}   AGPS_SUPL_INIT;

typedef enum
{
   AGPS_ACCEPT,
   AGPS_REJECT,
   AGPS_IGNORE
}   eUserResponse;

/**
 * The AGPS_UserResponse structure contains the user response
 to be used by the SuplInitListener.
 */
typedef struct AGPS_UserResponse
{
   /**
   * User Response: 0 = Accept (AGPS_ACCEPT)
                    1 = Reject (AGPS_REJECT)
                    2 = Ignore (No answer) (AGPS_IGNORE)
   */
   tSIRF_UINT8   UserResponse;
}   AGPS_UserResponse;

/**
* Reset Types
*/
enum RESET_TYPE
{
   /** Indicates that Factory Reset command should be sent to the GPS Client at the start of a session */
   RESET_TYPE_FACTORY_RESET,

   /** Indicates a Cold Start Reset command should be sent to the GPS Client at the start of a session */
   RESET_TYPE_COLD_RESET,

   /** Indicates a Hot Start Reset command should be sent to the GPS Client at the start of a session */
   RESET_TYPE_HOT_RESET,

   /** Indicates a warm Start Reset command should be sent to the GPS Client at the start of a session */
   /** This option is only available with 3tw*/
   RESET_TYPE_WARM_RESET,

   /** Indicates a Auto Start Reset command should be sent to the GPS Client at the start of a session */
   /** This option is only available with 3tw*/
   RESET_TYPE_AUTO_RESET,
   
    /** for test mode 7 **/
   RESET_TYPE_TEST_MODE
};

/**
 * The LPL_ERROR_CODE enum shows type error returned by LPL.
 */
typedef enum
{
   LPL_NO_ERROR                         = 0x00,  /* no error */
   LPL_INSTANCE_ALREADY_RUNNING         = 0x02,  /*LPL is already running*/
   LPL_HEAP_ALLOCATION_FAILURE          = 0x03,  /*LPL Heap allocation failure */
   LPL_ALREADY_INITIALIZED              = 0x04,  /*LPL is already Inititalized */
   LPL_NOT_INITIALIZED                  = 0x05,  /*LPL is NOT Initialized */
   LPL_SIRF_PAL_INITIATIZATION_FAILED   = 0x06,  /*LPL SIRF PAL Initialization Failed */
   LPL_NULL_POINTERS_GM_DATA            = 0x07,  /*LPL Input parameters are NULL */
   LPL_GPS_UNIT_NOT_SUPPORTED           = 0x08,  /*LPL does not support the GPS UNIT */
   LPL_STARTUP_FAILED                   = 0x09,  /*LPL startup failed */
   LPL_HEAP_DEALLOCATION_FAILURE        = 0x0A
}   LPL_ERROR_CODE;


/**
* Time Transfer Types
*/
enum TIME_TRANSFER
{
   /** Indicates that Time Transfer is Not Available */
   TIME_TRANSFER_NOT_AVAILABLE,

   /** Indicates that Coarse Time Transfer is being used */
   TIME_TRANSFER_COARSE,

   /** Indicates that Precise Time Transfer is being used */
   TIME_TRANSFER_PRECISE
};

/**
* Frequency Transfer Types
*/
enum FREQUENCY_TRANSFER
{
   /** Indicates that Frequency Transfer is Not Available */
   FREQUENCY_TRANSFER_NOT_AVAILABLE,

   /** Indicates that Frequency Transfer is available and the
       Non Counter Method is being used */
   FREQUENCY_TRANSFER_METHOD_NON_COUNTER,

   /** Indicates that Frequency Transfer is available and the Counter
       Method PULL is being used */
   FREQUENCY_TRANSFER_METHOD_COUNTER_PULL,

   /** Indicates that Frequency Transfer is available and the Counter
       Method PUSH is being used */
   FREQUENCY_TRANSFER_METHOD_COUNTER_PUSH
};

/**
* RTC Availability
*/
enum RTC_AVAILABILITY
{
   /** Indicates that no RTC is Available to the GPS Receiver */
   RTC_NOT_AVAILABLE,

   /** Indicates that an RTC is available, and that it is External
       to the GPS receiver */
   RTC_INTERNAL,

   /** Indicates that an RTC is available, and that it is Internal
       to the GPS receiver */
   RTC_EXTERNAL
};

/**
* Operation
 */
enum OPERATION
{
   NORMAL,
   ST1_TEST,
   ST2_TEST
};

/*
Network Type
*/

typedef enum
{
   GSM,
   WCDMA
}   NETWORK_TYPE;

typedef enum
{
   CELL_CHANGE = 0,    /*cell id changed */
   INTERAT_HO,         /*inter-RAT handover */
   SIGNAL_LOSS         /*signal loss */
}   LPL_netCellEventType;

typedef struct
{
   tSIRF_UINT16 mcc;
   tSIRF_UINT16 mnc;
   tSIRF_UINT16 lac;
   tSIRF_UINT16 cid;
}   LPL_netGSMCellID;

typedef struct
{
   tSIRF_UINT16 mcc;
   tSIRF_UINT16 mnc;
   tSIRF_UINT32 ucid;
}   LPL_netWCDMACellID;

typedef struct
{
   NETWORK_TYPE network_type;  /*existing LPL data structure */
   union
   {
      LPL_netGSMCellID gsm_cellid;
      LPL_netWCDMACellID wcdma_cellid;
   }   m;
}   LPL_netCellID;

/**
 * @struct GSM_Measurements
 * Structure definition for the GSM measurements.
 * This structure should be filled with the measurements.
 */
typedef struct
{
   tSIRF_UINT32 firstMeasurementTimeStamp;
   tSIRF_UINT8  numMeasurements;
   struct
   {
      tSIRF_UINT16 deltaTime;
      tSIRF_UINT16 mcc;
      tSIRF_UINT16 mnc;
      tSIRF_UINT16 lac;
      tSIRF_UINT16 cid;
      tSIRF_UINT8  ta;
      tSIRF_UINT8  numCells;
      struct
      {
         tSIRF_UINT16 arfcn;
         tSIRF_UINT8  bsic;
         tSIRF_UINT8  rxlev;
      }   cellInfo[MAX_NI_CELLS];
   }   measurements [MAX_NI_MRSMENTS];
}   GSM_Measurements;

/**
 * @struct LPL_FrequencyInfoFDD
 * Structure definition for the WCDMA Frequency information for FDD.
 * This structure should be filled with the FDD frequency information.
 */
typedef struct
{
   tSIRF_UINT8  uarfcn_ULPresent;
   tSIRF_UINT16 uarfcn_UL;
   tSIRF_UINT16 uarfcn_DL;
} LPL_FrequencyInfoFDD;

/**
 * @struct LPL_FrequencyInfoTDD
 * Structure definition for the WCDMA Frequency information for TDD.
 * This structure should be filled with the TDD frequency information.
 */
typedef struct
{
   tSIRF_UINT16 uarfcn_Nt;
} LPL_FrequencyInfoTDD;

/**
 * @enum LPL_FrequencyInfoType
 * Enum definition for supported frequency types.
 */
typedef enum
{
   FREQUENCY_INFO_FDD = 1,
   FREQUENCY_INFO_TDD = 2
} LPL_FrequencyInfoType;

/**
 * @struct LPL_FrequencyInfo
 * Structure definition for the WCDMA Frequency information.
 * This structure should be filled with the frequency information.
 */
typedef struct
{
   LPL_FrequencyInfoType type;
   union
   {
      LPL_FrequencyInfoFDD fdd;
      LPL_FrequencyInfoTDD tdd;
   } modeSpecificInfo;
} LPL_FrequencyInfo;

/**
 * @enum LPL_FrequencyInfoType
 * Enum definition for supported types of cell measurements.
 */
typedef enum
{
   CELL_MEASURED_RESULTS_FDD = 1,
   CELL_MEASURED_RESULTS_TDD = 2
} LPL_CellMeasuredResultsType;

/**
 * @struct LPL_CellMeasuredResults_fdd
 * Structure definition for the WCDMA Cell Measurements for FDD.
 * This structure should be filled with the cell measurements.
 */
typedef struct
{
   tSIRF_UINT8  cpich_Ec_N0Present;
   tSIRF_UINT8  cpich_RSCPPresent;
   tSIRF_UINT8  pathlossPresent;
   tSIRF_UINT16 primaryScramblingCode;
   tSIRF_UINT8  cpich_Ec_N0;
   tSIRF_UINT8  cpich_RSCP;
   tSIRF_UINT8  pathloss;
} LPL_CellMeasuredResults_fdd;

/**
 * @struct LPL_TimeslotISCP
 * Structure definition for the WCDMA time slot information for TDD.
 * This structure should be filled with the time slot information.
 */
typedef struct
{
   tSIRF_UINT32 n;
   tSIRF_UINT8  elem[MAX_TIME_SLOT];
} LPL_TimeslotISCP;

/**
 * @struct LPL_CellMeasuredResults_tdd
 * Structure definition for the WCDMA Cell Measurements for TDD.
 * This structure should be filled with the cell measurements.
 */
typedef struct
{
   tSIRF_UINT8  proposedTGSNPresent;
   tSIRF_UINT8  primaryCCPCH_RSCPPresent;
   tSIRF_UINT8  pathlossPresent;
   tSIRF_UINT8  timeslotISCP_ListPresent;
   tSIRF_UINT8  cellParametersID;
   tSIRF_UINT8  proposedTGSN;
   tSIRF_UINT8  primaryCCPCH_RSCP;
   tSIRF_UINT8  pathloss;
   LPL_TimeslotISCP timeslotISCP;
} LPL_CellMeasuredResults_tdd;

/**
 * @struct LPL_CellMeasuredResults
 * Structure definition for the WCDMA Cell Measurements.
 * This structure should be filled with the cell measurements.
 */
typedef struct
{
   tSIRF_UINT8  cellIdentityPresent;
   tSIRF_UINT32 cellIdentity;
   LPL_CellMeasuredResultsType type;
   union
   {
      LPL_CellMeasuredResults_fdd fdd;
      LPL_CellMeasuredResults_tdd tdd;
   } modeSpecificInfo;
} LPL_CellMeasuredResults;

/**
 * @struct LPL_WCDMA_MeasuredResults
 * Structure definition for the WCDMA Cell Measured Results.
 * This structure should be filled with the cell measurements.
 */
typedef struct
{
   tSIRF_UINT8  frequencyInfoPresent;
   tSIRF_UINT8  utra_CarrierRSSIPresent;
   tSIRF_UINT8  cellMeasuredResultsPresent;
   LPL_FrequencyInfo frequencyInfo;
   tSIRF_UINT8  utra_CarrierRSSI;
   LPL_CellMeasuredResults cellMeasuredResults[MAX_CELL_MEAS];
} LPL_WCDMA_MeasuredResults;

/**
 * @struct WCDMA_Measurements
 * Structure definition for the WCDMA measurements.
 * This structure should be filled with the measurements.
 */
typedef struct
{
   tSIRF_UINT8  frequencyInfoPresent;
   tSIRF_UINT8  primaryScramblingCodePresent;
   tSIRF_UINT8  measuredResultsPresent;
   tSIRF_UINT16 mcc;
   tSIRF_UINT16 mnc;
   tSIRF_UINT32 uc;
   LPL_FrequencyInfo frequencyInfo;
   tSIRF_UINT16 primaryScramblingCode;
   LPL_WCDMA_MeasuredResults measuredResults[MAX_FREQ];
} WCDMA_Measurements;

/**
 * @struct Network_Measurements
 * This structure incorporates all possible types of Network measurements.
 */
typedef struct
{
   NETWORK_TYPE  networkType;

   union
   {
      GSM_Measurements   gsmMeasurements;
      WCDMA_Measurements wcdmaMeasurements;
   }   m;

}   Network_Measurements;


typedef enum
{
   NO_AIDING             = 0x00,
   LOCAL_AIDING          = 0x01,
   NETWORK_AIDING        = 0x02,
   LOCAL_AIDING_PREFERED = 0x03
}   AIDING_TYPE;

#ifdef LPL_SIF

typedef struct
{
   tSIRF_UINT8  LPL_NAV_BITS_AIDING  : 1;
   tSIRF_UINT8  LPL_SGEE_AIDING      : 1;
   tSIRF_UINT8  LPL_CGEE_AIDING      : 1;
}   LPL_agpsAidingCapabilities;


#endif /*LPL_SIF*/

/**
* Logging types for LPL
*/
typedef enum
{
   LPL_NO_LOGGING = 0,
   LPL_SINGLE_SESSION_LOGGING = 1,
   LPL_ALL_SESSION_LOGGING = 2,
} LoggingType;

typedef struct LPL_LOGFiles
{
   tSIRF_CHAR *briefLogFileName;
   tSIRF_CHAR *detailedLogFileName;
   tSIRF_CHAR *agpsLogFileName;
   tSIRF_CHAR *previousLocationFileName;
   tSIRF_CHAR *slcLogFileName;
}   LPL_LOGFiles;

typedef struct LPL_loggingInformation
{
   LoggingType    loggingtype;
   LPL_LOGFiles   logfiles;
}   LPL_loggingInformation;

typedef enum
{
   LPL_SUCCESS,         /* the request was done successfully */
   LPL_FAILURE,         /* the request failed */
   LPL_NOT_SUPPORTED    /* the request is unsupported by platform */
} LPL_STATUS;           /* for GM_setDeviceState */

typedef enum
{
   LPL_GPS_FP_ON,       /* GPS in full power mode */
   LPL_GPS_OFF,         /* GPS power is off */
   LPL_GPS_HIBERNATE    /* GPS power in hibernation mode. */
} LPL_curGPSState;      /* for LPLGetGPSPowerState */

typedef enum
{
   PLATFORM_SLEEP,      /* platform requests to sleep */
   PLATFORM_WAKEUP,     /* platform requests to wakeup*/
   PLATFORM_INIT        /* platform requests to initialize */
} LPL_platformState;    /* for GM_setDeviceState */

typedef enum
{
   LPL_GPS_ON_REQUEST,         /* request to turn GPS power on */
   LPL_GPS_OFF_REQUEST,        /* request to turn GPS power off */
   LPL_GPS_HIBERNATE_REQUEST,  /* request to put GPS into hibernate mode via HW pulse */
   LPL_GPS_WAKEUP_REQUEST      /* request to wake up GPS from hibernation via HW pulse */
} LPL_requestedGPSAction;    /* for LPLSetGPSState */

#ifdef SUPL_FRAMEWORK

typedef struct _PosProtocol
{
   tSIRF_BOOL tia801;
   tSIRF_BOOL rrlp;
   tSIRF_BOOL rrc;
}  LPL_PosProtocol;

typedef struct _PosTechnology
{
   tSIRF_BOOL agpsSETassisted;
   tSIRF_BOOL agpsSETBased;
   tSIRF_BOOL autonomousGPS;
   tSIRF_BOOL aFLT;
   tSIRF_BOOL eCID;
   tSIRF_BOOL eOTD;
   tSIRF_BOOL oTDOA;
}  LPL_PosTechnology;

typedef struct _SETCapabilities
{
   LPL_PosTechnology    posTechnology;
   /* refer to the LOCATION_PREFERRED_METHOD enum in AGPS.h file */
   tSIRF_UINT32               prefMethod;
   LPL_PosProtocol      posProtocol;
}  LPL_SETCapabilities;

typedef struct _SatelliteInfoElement
{
   tSIRF_UINT8 satId;
   tSIRF_UINT8 iODE;
} LPL_SatelliteInfo;

typedef struct _NavigationModel
{
   struct
   {
      unsigned satInfoPresent : 1;
   } m;
   tSIRF_UINT16 gpsWeek;
   tSIRF_UINT8 gpsToe;
   tSIRF_UINT8 nSAT;
   tSIRF_UINT8 toeLimit;
   LPL_SatelliteInfo satInfo[12];
} LPL_NavigationModel;

typedef struct _RequestedAssistData
{
   struct
   {
      unsigned navigationModelDataPresent : 1;
   } m;
   tSIRF_BOOL almanacRequested;
   tSIRF_BOOL utcModelRequested;
   tSIRF_BOOL ionosphericModelRequested;
   tSIRF_BOOL dgpsCorrectionsRequested;
   tSIRF_BOOL referenceLocationRequested;
   tSIRF_BOOL referenceTimeRequested;
   tSIRF_BOOL acquisitionAssistanceRequested;
   tSIRF_BOOL realTimeIntegrityRequested;
   tSIRF_BOOL navigationModelRequested;
   LPL_NavigationModel navigationModelData;
} LPL_RequestedAssistData;

#endif /*SUPL_FRAMEWORK*/

/** Frequency update related defines */
/** Indicates frequency value is invalid and should not be used. */
#define SIRF_LPL_FREQUENCY_UPDATE_INVALID            0x00
/** Indicates frequency will be the absolute center frequency
   of the ECLK (Nominal frequency + delta) */
#define SIRF_LPL_FREQUENCY_UPDATE_CENTER             0x01
/** Indicates freq will be the delta from the nominal frequency
   entered by GM_setFrequencyNominalValue*/
#define SIRF_LPL_FREQUENCY_UPDATE_DELTA_FROM_CENTER  0x02

/* Frequency Accuracy related defines */
/** Indicates that Frequency accuracy is invalid and should not be used */
#define SIRF_LPL_FREQUENCY_ACCURACY_INVALID          0x00
/** Indicates that Frequency accuracy is in units of PPM */
#define SIRF_LPL_FREQUENCY_ACCURACY_PPM              0x01
/** Indicates that Frequency accuracy is in units of Hz */
#define SIRF_LPL_FREQUENCY_ACCURACY_HZ               0x02

typedef struct  _tSIRF_LPL_HW_FREQ_UPDATE_tag
{
   tSIRF_UINT32 request_id;
   tSIRF_UINT32 frequency_data_type;
   tSIRF_DOUBLE frequency;
   tSIRF_UINT32 accuracy_data_type;
   tSIRF_DOUBLE accuracy;
} tSIRF_LPL_HW_FREQ_UPDATE;

#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/

#ifdef MEM_LOG
   tSIRF_VOID GM_setMallocLogFile(FILE* mLogFile);
#endif /*MEM_LOG*/


   /**
   * This function is called by the GeoSession object whenever a new serial message (SiRF Binary, NMEA, Statistics, AI3 or F)
   * is available.
   * @param message A String object containing the serial message
   * @param messageLength An integer representing the length of the serial message
   */
   typedef tSIRF_VOID (*SerialListener) (tSIRF_UINT8* message, tSIRF_UINT32 messageLength);

   /**
   * This function is called by the GeoSession object whenever
   * a SiRF Binary (SSB) message is available.
   * @param message A String object containing the SiRF Binary message
   * @param messageLength An integer representing the length of the SiRF Binary message
   */
   typedef tSIRF_VOID (*SSBListener) (tSIRF_UINT8* message, tSIRF_UINT32 messageLength);

   
   /**
	 * This function is called by the GeoSession object whenever
	 * a test mode message is available.
	 * @param message A String object containing the F channel message
	 * @param messageLength An integer representing the length of the F Channel message
   */
   typedef tSIRF_VOID (*tmodeListner) (tSIRF_UINT32 msg_id, tSIRF_UINT8* message, tSIRF_UINT16 messageLength);

   /**
    * This function is called by the GeoSession object whenever
    * a AI3 message is available.
    * @param message A String object containing the AI3 message
    * @param messageLength An integer representing the length of the AI3 message
    */
   typedef tSIRF_VOID (*AI3Listener) (tSIRF_UINT8* message, tSIRF_UINT16 messageLength);

   /**
    * This function is called by the GeoSession object whenever
    * a F Channel message is available.
    * @param message A String object containing the F channel message
    * @param messageLength An integer representing the length of the F Channel message
    */
   typedef tSIRF_VOID (*FListener) (tSIRF_UINT8* message, tSIRF_UINT16 messageLength);

   /**
   * This function is called by the GeoSession object whenever a new debug message is available.
   * @param pMsg A String object containing the debug message
   */
   typedef tSIRF_VOID (*DebugListener) (tSIRF_CHAR* pMsg);


   /**
   * This function is called by the GeoSession object whenever a new RRLP(RRC) message
   * is available. This is to be used in Control Plane architectures.
   *
   * @param message A pointer to the byte array containing the message
   * @param messageLength An integer representing the length of the message
   */

   typedef tSIRF_BOOL (*LPL_sendNetMessage) (eAssistProtocol ota_type,
         LPL_lplOtaParams *p_ota_params,
         tSIRF_UINT8 *p_msg_data,
         tSIRF_UINT32 msg_size);

   typedef struct
   {
      LPL_sendNetMessage reportMessageCb;
   }   MessageListener;

   /**
   * This function is called by the GeoSession object whenever a SUPL_INIT message
   * is available.
   *
   * @param pSuplInitData A pointer to a structure containing most of the information
                          from the SUPL_INIT message
   * @param pUserResponse An pointer to a structure with the user's response
   * @param pUserData     Pointer to user specific data
   */
   typedef tSIRF_VOID (*SuplInitListener) (AGPS_SUPL_INIT* pSuplInitData,
                                           AGPS_UserResponse* pUserResponse,
                                           tSIRF_VOID* pUserData);

   /**
   * This function is called by the GeoSession object whenever a new LPL Progress state (both status and error)  is available.
   * @param type A enum value indicating the progress state code (refer to SiRFstudioProgressStates enum for more details)
   * @param message A String object containing brief description about the progress state code.
   */

   typedef tSIRF_BOOL (* ProgressListener) (SiRFstudioProgressStates progressState, tSIRF_CHAR *progressDescriptor);

   /**
   * This function is called by the location engine object whenever the SiRFLoc Client versionis available
   */
   typedef tSIRF_VOID (*FWVersionListener) (tSIRF_VOID);

typedef enum
{
    LOCATION_RESULT,       // UE-based
    MEAS_RESULT,           // UE-assisted
    POSITION_ERROR         // need additional assistance data will be the part of it
}CPAgent_Msg_type;
   /**
   * This function is called by the GeoSession object whenever a network measurements are required.
   *
   * @param networktype Type of network ( GSM , WCDMA , etc.)
   * @param mesurements A pointer the structure in which measurements of the network need to be filled.
   *
   */

   typedef tSIRF_VOID (*NetworkMeasurementsListener) (Network_Measurements * mesurements);

   /* Deprecated: GeoManager functions */
   /* Following are used for LPL only, to have more flexibility */
   /* Will be initialized to their default cases for others */
   /**
   * GM_setLocationMethod is used to set the Location Method
   * @param locationMethod This parameter should be set to the desired Location Method
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setLocationMethod(tSIRF_UINT8 locationMethod);

   /**
   * GM_setCapabilites is used to set the SET capabilities
   * @param pSetCapabilities Pointer to a SETcapabilities structure containing the SET capabilities
   * @param locationPreferredMethod This parameter should be set to the desired Location Preferred Method
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setCapabilities(SETcapabilities* pSetCapabilities, tSIRF_UINT8 locationPreferredMethod);

   /**
   * GM_setLogFiles is used to specify the absolute files paths for the log files.
   * @param BriefLogfile This parameter should contain the absolute path of the brief log file name
   * @param Detailedlogfile This parameter should ontain the absolute path of the detailed log file name
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setLogFiles(tSIRF_CHAR* briefLogFile , tSIRF_CHAR* detailedLogFile, tSIRF_CHAR* previousPositionLogFile, LoggingType  loggingType );


   /**
   * GM_setSETIdentification is used to set the SET Identification
   * @param SETidType This parameter should be set to IMSI or MSISDN
   * @param SETidValue This parameter should be set to the value of SETid
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setSETIdentification(tSIRF_UINT8 SETidType, tSIRF_UINT8* SETidValue);

   /**
   * GM_setWCDMAVariableParameters is used to set WCDMA Variable Parameters
   * @param wcdmaParametersValid tSIRF_BOOL indicating if the WCDMA parameters are valid. If set to false, the WCDMA parameters will not be used.
   * @param mcc This parameter should be set MCC (mobile country code)
   * @param mcc This parameter should be set MNC (mobile network code)
   * @param cid This parameter should be set UC (cell ID)
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setWCDMAParameters(tSIRF_BOOL wcdmaParametersValid, tSIRF_UINT16 mcc, tSIRF_UINT16 mnc, tSIRF_UINT32 uc);

   /**
   * GM_setGSMVariableParameters is used to set GSM Variable Parameters
   * @param gsmParametersValid tSIRF_BOOL indicating if the GSM parameters are valid. If set to false, the GSM parameters will not be used.
   * @param mcc This parameter should be set MCC (mobile country code)
   * @param mcc This parameter should be set MNC (mobile network code)
   * @param lac This parameter should be set LAC (location area center)
   * @param cid This parameter should be set CID (cell ID)
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setGSMParameters(tSIRF_BOOL gsmParametersValid, tSIRF_UINT16 mcc, tSIRF_UINT16 mnc, tSIRF_UINT16 lac, tSIRF_UINT16 cid);


   /**
   * GM_setResetType is used to set the reset type
   * @param resetType This parameter is the reset type
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setResetType(tSIRF_UINT8 resetType);

   /**
   * GM_setTimeTransferType is used to set the time transfer type
   * @param timeTransferType This parameter is the time transfer type
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setTimeTransferType(tSIRF_UINT8 timeTransferType);

   /**
   * GM_setTimeAccuracy is used to set the time transfer precision and the time transfer skew
   * @param timeTransferPrecision This parameter is the time transfer precision
   * @param timeTransferSkew This parameter is the time transfer skew
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setTimeAccuracy(tSIRF_INT32 timeTransferPrecision, tSIRF_INT32 timeTransferSkew);

   /**
    * @fn tSIRF_RESULT  GM_setFrequencyTransferMethod(tSIRF_UINT8 frequencyTransferMethod)
    * This function pass the incoming parameter to the Location engine
    *
    * @param[in] frequencyTransferMethod could be one of these enums:
    *            FREQUENCY_TRANSFER_NOT_AVAILABLE
    *            FREQUENCY_TRANSFER_METHOD_NON_COUNTER
    *            FREQUENCY_TRANSFER_METHOD_COUNTER_PULL
    *            FREQUENCY_TRANSFER_METHOD_COUNTER_PUSH
    *
    * @return    SIRF_SUCCESS if the transfer method is supported.
    *            SIRF_FAILURE otherwise
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_RESULT      GM_setFrequencyTransferMethod(tSIRF_UINT8 frequencyTransferMethod);

   /**
    * @fn tSIRF_RESULT  GM_setFrequencyNominalValue(tSIRF_DOUBLE frequencyTransferNominalValue)
    * This function pass the nominal frequency of reference clock.
    *
    * @param[in] frequencyTransferNominalValue - frequencyTransferNominalValue is
    *            the specified clock frequency to ECLK pin of GPS(unit in 0.001Hz)
    *
    * @return    SIRF_SUCCESS if the frequency is in the supported range of values
    *            SIRF_FAILURE otherwise
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_RESULT        GM_setFrequencyNominalValue(tSIRF_DOUBLE  frequencyTransferNominalValue);


   /**
    * @fn tSIRF_RESULT GM_setFrequencyTransferParameters( tSIRF_UINT32 max_request_time,
    *                                                 tSIRF_UINT32 request_period )
    * This function is used to set the max_request_time and request_period
    * at runtime.
    *
    * @param[in] max_request_time (in ms) - Maximum amount of time to wait for the
    *            frequency update before turning off the ECLK by calling
    *            SIRF_PAL_HW_FrequencyTransferStop. Use 0 for the default
    *            of 1 second
    *
    * @param[in] request_period (in ms) - In the
    *            FREQUENCY_TRANSFER_METHOD_COUNTER_PULL the time to wait after
    *            receiving a SIRF_FREQUENCY_UPDATE_INVALID and issuing another
    *            request. Use 0 for the default of 100ms.
    *
    * @return SIRF_SUCCESS     The parameters were accepted.
    *         SIRF_FAILURE     The parameters are not ready to be received. Usually
    *                          occurs if the frequency transfer method is not
    *                          counter method.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_RESULT GM_setFrequencyTransferParameters( tSIRF_UINT32 max_request_time,
         tSIRF_UINT32 request_period);

   /**
    * @fn tSIRF_RESULT GM_updateFrequency(tSIRF_LPL_HW_FREQ_UPDATE* frequency_update)
    * This API can be called after the ECLK has been turned on by the PAL function
    * SIRF_PAL_HW_FrequencyTransferStart. If the frequency transfer method is
    * FREQUENCY_TRANSFER_METHOD_COUNTER_PULL then the software must wait until
    * SIRF_PAL_HW_FrequencyTransferUpdateRequest has been called. If
    * FREQUENCY_TRANSFER_METHOD_COUNTER_PUSH is the the frequency transfer
    * method then the request_id is ignored
    *
    * @param[out] frequency_update  pointer to the tSIRF_LPL_HW_FREQ_UPDATE structure
    *
    * @return SIRF_SUCCESS     If the frequency was updated successfully
    *         SIRF_FAILURE     There is no pending request or frequency transfer
    *                          operation currently taking place
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_RESULT GM_updateFrequency(tSIRF_LPL_HW_FREQ_UPDATE * frequency_update);

   /**
   * GM_setRTCType is used to set the RTC type
   * @param RTCType This parameter is the RTC type
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setRTCType(tSIRF_UINT8 RTCType);

   /* GeoSession functions */
   /* Following are used for LPL only, to have more flexibility */
   /* Will be initialized to their default cases for others */
   /**
   * GS_setSerialListener() Specify a SerialListener to get continuous serial stream from this GeoSession.
   * Only one SerialListener is allowed per session, and setting a new listener overrides the previous one.
   *
   * @param pGS Pointer to the GeoSession object
   * @param listener reference to a SerialListener object.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setSerialListener(GeoSession* pGS, SerialListener listener);

   /**
    * @fn GS_setSSBListener(GeoSession* pGS, SSBListener listener)
    * This API is used to specify a SSBListener to get continuous
    * SiRF Binary(SSB)data from this GeoSession. Only one SSBListener
    * is allowed per session, and setting a new listener overrides
    * the previous one.
   *
    * @param[in]  pGS Pointer to the GeoSession object
    * @param[in]  listener reference to a SSBListener object
    *
    * @return     None
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GS_setSSBListener(GeoSession* pGS, SSBListener listener);

/**
 * @fn GS_setTestModeListener(GeoSession* pGS, tmodeListner listener)
 * @param[in]  pGS Pointer to the GeoSession object
 * @param[in]  listener reference to a tmodeListner object
 *
 * @return     None
 */
#ifdef DLL
   DLL_EXPORT
#endif   /* DLL */
tSIRF_VOID GS_setTestModeListener(GeoSession* pGS, tmodeListner listener);

   /**
    * @fn GS_setAI3Listener(GeoSession* pGS, AI3Listener listener)
    * This API is used to specify a AI3Listener to get AI3 data
    * from this GeoSession. Only one AI3Listener is allowed per session,
   * and setting a new listener overrides the previous one.
   *
    * @param[in]  pGS Pointer to the GeoSession object
    * @param[in]  listener reference to a AI3Listener object
    *
    * @return     None
    */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GS_setAI3Listener(GeoSession* pGS, AI3Listener listener);

   /**
    * @fn GS_setFListener(GeoSession* pGS, FListener listener)
    * This API is used to specify a FListener to get F channel
    * data from this GeoSession. Only one FListener is allowed
    * per session, and setting a new listener overrides the
    * previous one.
    *
    * @param[in]  pGS Pointer to the GeoSession object.
    * @param[in]  listener reference to a FListener object.
    *
    * @return     None.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GS_setFListener(GeoSession* pGS, FListener listener);

   /**
   * GS_setDebugListener() Specify a DebugListener to get continuous serial stream from this GeoSession.
   * Only one DebugListener is allowed per session, and setting a new listener overrides the previous one.
   *
   * @param pGS Pointer to the GeoSession object
   * @param listener reference to a DebugListener object.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setDebugListener(GeoSession* pGS, DebugListener listener);

   /**
   * GS_setMessageListener() Specify a MessageListener to get RRLP (RRC) messages from this GeoSession.
   * Only one MessageListener is allowed per session, and setting a new listener overrides the previous one.
   *
   * @param pGS Pointer to the GeoSession object
   * @param listener reference to a MessageListener object.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setMessageListener(GeoSession* pGS, MessageListener listener);

   /**
   * GS_setSuplInitListener() Specify a SuplInitListener to get the SUPL INIT information from this GeoSession.
   * Only one SuplInitListener is allowed per session, and setting a new listener overrides the previous one.
   *
   * @param pGS Pointer to the GeoSession object
   * @param listener reference to a SuplInitListener object
   * @param pUserData Pointer to user specific data
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setSuplInitListener(GeoSession* pGS, SuplInitListener listener, tSIRF_VOID* pUserData);

   /**
   * GS_setProgressListener() Specify a ProgressListener to get continuous Progress state updates  from this GeoSession.
   * Only one ProgressListener is allowed per session, and setting a new listener overrides the previous one.
   *
   * @param pGS Pointer to the GeoSession object
   * @param listener pointer to a ProgressListener Function.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setProgressListener(GeoSession* pGS, ProgressListener listener);

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   /**
   * GS_setFWVersionListener() Specify a {@link FWVersionListener} to get SiRF Loc Client version en consecutive locations.
   * @param listener pointer to a {@link FWVersionListener} object.
   */
   tSIRF_UINT8       GS_setFWVersionListener(FWVersionListener listener);


#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   /**
   * GS_setNetworkMeasurementsListener() Specify a {@link NetworkMeasurementsListener} to get Network measurements from the Application.
   * @param pGS Pointer to the GeoSession object
   * @param listener pointer to a {@link NetworkMeasurementsListener} object.
   */
   tSIRF_VOID        GS_setNetworkMeasurementsListener(GeoSession* pGS, NetworkMeasurementsListener listener);


   /**
   * GS_sendNetMessage() Transmits an RRLP or RRC message from the application to the LPL.
   *
   * @param message A pointer to the byte array containing the message
   * @param messageLength An integer representing the length of the message
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_BOOL GS_sendNetMessage(eAssistProtocol ota_type,
                                LPL_RRCState rrc_state,       /* initial RRC state */
                                eSessionPriority priority,
                                tSIRF_UINT8 *p_msg_data,
                                tSIRF_UINT32 msg_size,
                                tSIRF_UINT64 *pTimeStamp);


   /**
   * GS_setOperation() Sets the LPL mode.
   *
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setOperation(tSIRF_UINT8 mode);

   /**
   * GM_getLPLversion() Gets the LPL version.
   *
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_CHAR *       GM_getLPLversion(tSIRF_VOID);

   /**
   * GM_getSiRFLocClientversion() Gets the SiRFLocClient version.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_CHAR *    GM_getSiRFLocClientversion(tSIRF_VOID);

   /**
   * GS_setQoPinSUPLSTART() Sets QoPinSUPLSTART.
   *
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setQoPinSUPLSTART(GeoSession* pGS, tSIRF_BOOL QoPinSUPLSTART);

   /**
       This structure defines the OTA message
   */
   typedef struct tagOTAMsg
   {
      tSIRF_UINT8*  pbBody;
      tSIRF_UINT16  cbBodyLength;
   }   OTAMSG;

   /**
   * GS_setOTAMessage is used to set parameters related to OTA message for Network Initiated sessions
   * @param bNetworkInitiated This parameter should be set true for Network Initiated sessions, false otherwise
   * @param otaMessage This parameter contains the OTA message
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setOTAMessage(tSIRF_BOOL bNetworkInitiated, OTAMSG* otaMessage);

   /**
   * GM_setBind is used to configure which interface the GeoServices platform should bind to. This
   * applies to all sessions in the platform.
   * @param bBind = 1 to use bind, 0 not to use it
   * @param ipAddress The IP Address of the interface
   * @param portNumber The TCP Port of the interface
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setBind(tSIRF_BOOL bBind, tSIRF_CHAR* ipAddress, tSIRF_UINT16 portNumber);


#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_shutdownSession (GeoSession* pSession);

   /**
   * GM_SetNumberOfMeasurements is used to configure the set of measurements used for MSB/MSA. This
   * applies to all sessions in the platform.
   * @param NumberOfMeasurements = Range 1 to 5
   */

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_SetNumberOfMeasurements(tSIRF_UINT8 NumberOfMeasurements);


#if (!(defined(LPL_SUPL2_0_0) || defined(LPL_SUPL1_5_0)))
   /**
   * GS_getReaiding() To get the re-aiding data from the server
   ** @param None
      @return Flag specifying whether re-aidng can be achieved or not
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_UINT8 GS_getReaiding(tSIRF_VOID);
#endif /* LPL_SUPL2_0_0 or LPL_SUPL1_5_0 */

#define NMEA_PORT 0x01
#define SSB_PORT  0x02
#define STAT_PORT 0x03

   /**
   * GS_setPort() is used to enable/disable NMEA and SSB ports
   * @param port    NMEA/SSB
   * @param bEnable 1 - enable, 0 - disable
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GS_setPort(tSIRF_UINT8 port, tSIRF_BOOL bEnable);

   /**
   * GM_SetSecure is used to control security for SUPL session.
   * @param isSecure = 1 to enable TLS, 0 disable TLS
   */

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setSecure(tSIRF_BOOL bSecure);

   /**
   * GM_SetUTtimersValue is used to set the value of UT1timerValue, UT2timerValue and UT3timerValue,
   * that is the timeout between various SUPL messages.
   * @param timer1Value = the value to be set in seconds
   * @param timer2Value = the value to be set in seconds
   * @param timer3Value = the value to be set in seconds
   */

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setUTtimersValue(tSIRF_UINT8 timer1Value, tSIRF_UINT8 timer2Value, tSIRF_UINT8 timer3Value);


   /**
   * The SUPL version numbers in GM_setSUPLVersion will be used to connect to a SUPL server when starting a SET session.
   * If the SUPL server does not support "desired" SUPL version, the first SUPL message SUPL_START shall fail, and the
   * application can start a new session with a different SUPL version.
   * @param suplVersion The SUPL major and minor version numbers.
   * 8/1/2007: This function is not used since each LPL release can only support one SUPL version.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setSUPLVersion(SUPL_version suplVersion);

#if defined(LPL_SUPL2_0_0) || defined(LPL_SUPL1_5_0)
   /**
   * For the application to set connection mode.
   * @isDisconBtwTrigger : when setting to SIRF_TRUE, which is default, LPL will disconnect SET from server for each reported position.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setPeriodicTriggerConnection(tSIRF_BOOL isDisconBtwTrigger);

   /**
   * For the application to set SET trigger parameters, not for NI trigger parameters.
   * @periodicParams : it's set by the application that read users' settings from GUI or configuration file.
   * @return : none
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setTriggerMode(eTriggerType triggerType);

#endif /* LPL_SUPL2_0_0 or LPL_SUPL1_5_0 */

   /**
   * GM_decodeSuplInit is used to decode the SUPL INIT packet before starting the LPL session.
   * @param otaMessage This parameter contains the received OTA message
   * @param pSuplInitData This parameter will contain the decoded SUPL INIT message
   */

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_BOOL GM_decodeSuplInit(OTAMSG* otaMessage, AGPS_SUPL_INIT* pSuplInitData);


#define LPL_SHORT_NI_SESSION 1 /* if GS_cfgSession is not called, a session will be a normal NI session. 
   Otherwise, the session will be a short session if the previous fix,
   aiding data are good, and is within 2 minutes. The session will be
   over after SUPL-POSINIT to a server and the server sends SUPL-END.*/
   /**
   * GS_cfgSession is used to config some LPL features in a more generic way.
   * @param config: There could be multiple settings, and each corresponds to a unique feature.
   *                See all definitions in this file.
   * @param enable: This parameter tells LPL to use or not use a feature.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GS_cfgSession(tSIRF_INT32 config, tSIRF_BOOL enable);

   /**
   * GM_setAPM is used to set the APM in SLC.
   * @param enable specifies whether APM should be enabled or disabled.
            if this variable is SIRF_TRUE then the Time between the fixes selects either of regular APM mode or Tricle Power Mode.
            if Time Between the fixes is greater than or equal to 10sec and less than 180 sec , the regular APM mode is selected.
            if Time Between the fixes is less than 10sec then Tricle power mode is enabled.
   * @param powerDutyCycle specifies .
   *       The values will range from 1 to 20. 1 shall represent a 5% duty cycle and 20 shall represent a 100% duty cycle.
   *       Duty cycle can be calculated by the formula:  % = value * 5. Any other value will invalidate the whole message
   * @param timeDutyPriority specifies the priority between the time between the fixes and power duty cycle.
   *       If set  to "1" if guarantying "time between fixes" has higher priority than the power reduction.
            if set  to "2" if the power duty cycle has higher priority than the time between fixes. Any other value will invalidate the whole message
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */

   tSIRF_VOID GM_setAPM (tSIRF_BOOL enable , tSIRF_UINT8 powerDutyCycle, tSIRF_UINT8 timeDutyPriority);

   /**
   * GM_setLoggingInformation is used to provide the Logging information to the LPL.
   * @param logginginfo This parameter contains the file names with creation path used for logging.
   *
   */

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setLoggingInformation (LPL_loggingInformation *logginginfo);


#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setAidingType (AIDING_TYPE aidingType);

#ifdef LPL_SIF

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setAidingCapabilities (LPL_agpsAidingCapabilities aidingCapabilities);

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_UpdateEEFile(tSIRF_BOOL updatefile);

#endif /*LPL_SIF*/
   /* For the application to set the Assist Protocol. This can be SUPL_RRLP, SUPL_RRC, CP_RRLP or CP_RRC
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setAssistProtocol(eAssistProtocol AssistProtocol);

   /*
   */
   typedef tSIRF_VOID (*ICDsListener) (tSIRF_UINT8 AI3_icd, tSIRF_UINT8 F_icd);

   /* For the application to set the AI3 and F ICDs or to use auto-detect feature to detect them
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setICDs(tSIRF_UINT8 AI3_icd, tSIRF_UINT8 F_icd, tSIRF_BOOL ICDsAutoDetect, ICDsListener listener);


#if (!(defined(LPL_SUPL2_0_0) || defined(LPL_SUPL1_5_0)))

   /** For the application to configure how LPL should request re-aiding data in navigation mode.
   @param SVLimit: LPL won't request aiding if the number of SVs used in the current fix is more
                   than SMLimit. Defaul is 6.
   @param reaidingTimeIntervalLimit (in minutes): LPL won't request aiding if the previous aiding
                   data was obtained within this interval. Defaul is 20 minutes.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setReAidingParameters(tSIRF_UINT8 SVLimit, tSIRF_UINT16 reaidingTimeIntervalLimit);

#endif /* LPL_SUPL2_0_0 or LPL_SUPL1_5_0 */

   /* ================= CGPSC related code ================= */

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   LPL_STATUS GM_getGPSPowerState(LPL_curGPSState *state);

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   LPL_STATUS GM_setDeviceState(LPL_platformState state);

   /* ============= End of CGPSC related code ============== */

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_UINT8 GM_updateIntervalBetweenPositions(GeoSession* pGS ,tSIRF_UINT8 interval);

#ifdef SUPL_FRAMEWORK
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */

   /**
      * @fn tSIRF_VOID GS_getSETCapabilities(LPL_SETCapabilities *setCapabilities)
      * This function retreives the SET capabilities for SUPL session.
      *
      * @param[out] setCapabilities pointer to a variable to retrieve the SET Capabilities..
      *
      * @return None
      */
   tSIRF_VOID GS_getSETCapabilities(LPL_SETCapabilities *setCapabilities);

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   /**
      * @fn tSIRF_VOID GS_getRequiredAssistanceData(LPL_RequestedAssistData *requiredAssistanceData)
      * This function retreives the required assistance data for SUPL session.
      *
      * @param[out] requiredAssistanceData pointer to a variable to retrieve the required assistance data.
      *
      * @return None.
      */

   tSIRF_VOID GS_getRequiredAssistanceData(LPL_RequestedAssistData *requiredAssistanceData);

#endif /*SUPL_FRAMEWORK*/

#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   /**
      * @fn LPL_ERROR_CODE GM_getLastError( tSIRF_VOID )
      * This function retreives the last error occured during the GM Initialization.
      *
      * @return LPL error codes defined in LPL_ERROR_CODE enum.
      */

   LPL_ERROR_CODE GM_getLastError( tSIRF_VOID );

   /**
    * @brief GM_setNetworkStatus
    *
    * Description: This function allows the Application to set the network
    *              status to LPL library.
    *
    * @param[in] networkStatus   Network Status
    *
    ***************************************************************************/
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setNetworkStatus(tSIRF_BOOL networkStatus);

   /**
   * @fn tSIRF_RESULT GM_ReleaseTracker( tSIRF_VOID )
   * This function release the tracker loading wait lock and is only meant for LSM
   *
   * @return SIRF_SUCCESS/SIRF_FAILURE
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
tSIRF_RESULT GM_ReleaseTracker( tSIRF_VOID );

   /**
   * @fn tSIRF_RESULT GM_GetSGEEFileAge(tSIRF_UINT32 *pAge, tSIRF_UINT32 *pPrediction)
   * This function gets the age information of SGEE file.
   * @param[out] pAge pointer to age.
   * @param[out] pPrediction pointer to prediction.
   * @return SIRF_SUCCESS/SIRF_FAILURE
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
tSIRF_RESULT GM_GetSGEEFileAge(tSIRF_UINT32 *pAge, tSIRF_UINT32 *pPrediction);
   
  /**
   * @fn tSIRF_RESULT GM_setSiRFAware( tSIRF_BOOL  sirfAware )
   * This function set the SiRFAware ON/OFF.
   * @param[in] sirfAware SIRF_TRUE sets SiRFAware On; SIRF_FALSE sets SiRFAware Off.
   * @return SIRF_SUCCESS/SIRF_FAILURE
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
tSIRF_RESULT GM_setSiRFAware( tSIRF_BOOL  sirfAware );



/**
 * @fn tSIRF_RESULT GM_setTestMode( tSIRF_BOOL  enable )
 * This function set the TestMode on LPL ON/OFF.
 * @param[in] SIRF_TRUE sets TestMode On; SIRF_FALSE sets TestMode Off.
 * @return SIRF_SUCCESS/SIRF_FAILURE
 */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
tSIRF_RESULT GM_setTestMode( tSIRF_BOOL  enable );


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif      /* __AGPS_H__ */

/**
 * @}
 * End of file.
 */

