/**
 * @addtogroup app_TestAppConfigMgr
 * @{
 */

/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, a CSR plc Company                    *
 *                           GPS Software                                  *
 *                                                                         *
 *    Copyright (c) 2007-2011 by SiRF Technology, a CSR plc Company        *
 *                        Inc. All rights reserved.                        *
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.’s confidential and       *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.’s  express written            *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 ***************************************************************************
 */
 
#ifndef __TAGMANAGER_H__
#define __TAGMANAGER_H__

#include "sirf_types.h"

#define MAX_STR_KEY_VAL_LEN      (128)

/* Data Types of various config entity */
#define TYPE_STR 0x00   /* String  */ 
#define TYPE_INT 0x01   /* Integer */ 
#define TYPE_BIN 0x02   /* Binary  */  

#define NO              (0)
#define YES             (1)

#define ABSENT          (0)
#define PRESENT         (1)

#define PREF_MSB        (0)
#define PREF_MSA        (1)
#define PREF_NONE       (2)

#define SINGLE          (1)
#define CONTINUOUS      (2)

#define NO_LOGGING      (0)
#define SINGLE_SESSION  (1)
#define MULTISESSION    (2)

#define NO_AID          (0)
#define LOCAL_AID       (1)
#define NET_AID         (2)

#define FACTORY        (0)
#define COLD           (1)  
#define HOT            (2)
#define WARM           (3)
#define AUTO           (4)

#define AP_SUPL_RRLP   (0)
#define AP_SUPL_RRC    (1)
#define AP_CP_RRLP     (2)
#define AP_CP_RRC      (3)

#define DISABLE        (0)
#define ENABLE         (1)

/* TAG NAME defines */
#define TAG_GPS_PORT         "GPSPort"
#define TAG_RESET_PORT       "ResetPort"
#define TAG_AUX_PORT         "AUXPort"
#define TAG_SERVER_PORT      "serverPort"
#define TAG_MODEM_PORT       "GPRSModemPort"
#define TAG_BEARER           "Bearer"
#define TAG_APN              "Apn"    
#define TAG_LOGIN            "Login"
#define TAG_PASSWORD         "Password"
#define TAG_MODEM            "Modem"
#define TAG_PHONE_NUMBER     "PhoneNumber"
#define TAG_SESSIONS         "NumberOfSessions"
#define TAG_AGPS_ADDRESS     "ServerAddress"
#define TAG_AGPS_PORT        "Port"
#define TAG_AGPS_SECURITY    "Secure"
#define TAG_RESET_TYPE       "ResetType"
#define TAG_AIDING_TYPE      "AidingType"
#define TAG_POS_TECH_MSB     "PositionTechnologyMSB"
#define TAG_POS_TECH_MSA     "PositionTechnologyMSA"
#define TAG_POS_TECH_AUTO    "PositionTechnologyAutonomous"
#define TAG_POS_TECH_ECID    "PositionTechnologyECID"
#define TAG_PREF_METHOD      "Preference"
#define TAG_FIX_TYPE         "FixType"
#define TAG_FIXES            "NumberOfFixes"
#define TAG_INTERVAL         "TimeBetweenFixes"
#define TAG_ASSIST_PROTO     "AssistProtocol"
#define TAG_APX_LOC          "ApproximateLocation"
#define TAG_APX_LAT          "ApproximateLatitude"
#define TAG_APX_LON          "ApproximateLongitude"
#define TAG_APX_ALT          "ApproximateAltitude"
#define TAG_APX_HERR         "ApproximateHorErr"
#define TAG_APX_VERR         "ApproximateVerErr"
#define TAG_QOP              "QoPinSUPLSTART" 
#define TAG_HOR_ACCURACY     "QoShorizontalAccuracy"
#define TAG_VER_ACCURACY     "QoSverticalAccuracy"
#define TAG_MAX_RESP_TIME    "QoSmaxResponseTime"
#define TAG_MAX_LOC_AGE      "QoSmaxLocAge"
#define TAG_CGEE             "CGEE"
#define TAG_SGEE             "SGEE"
#define TAG_EEFILE           "EEFileUpdate"
#define TAG_LOG_TYPE         "LoggingType"
#define TAG_LOG_BRIEF        "BriefLogFilePath"
#define TAG_LOG_DETAIL       "DetailedLogFilePath"
#define TAG_LOG_AGPS         "AGPSLogFilePath"
#define TAG_LOG_SLC          "SLCLogFilePath"
#define TAG_LOG_SN           "SNLogFilePath"
#define TAG_SIRF_AWARE       "SiRFAware"
#define TAG_NAV_BIT_AID      "NAVBitAiding"
#define TAG_SGEE_SERVER_ADR1  "SGEEServerAddress1"
#define TAG_SGEE_SERVER_ADR2  "SGEEServerAddress2"
#define TAG_SGEE_SERVER_PORT "SGEEServerPort"
#define TAG_SGEE_SERVER_FILE "SGEEServerFile"
#define TAG_SGEE_SERVER_AUTH "SGEEServerAuth"
#define TAG_SGEE_URID_FLAG   "SGEEUrId"
#define TAG_SGEE_URID_DEVICEID "SGEEDeviceId"
#define TAG_SGEE_URID_OEMSUBID  "SGEEOemSubId"
#define TAG_NETWORK_STATUS   "NetworkConnection"
#define TAG_LISTENER_NMEA    "NMEAListener"
#define TAG_LISTENER_SSB     "SSBListener"
#define TAG_LISTENER_SERIAL  "SerialListener"
#define TAG_LISTENER_DEBUG   "DebugListener"
  /** 
   * @struct    config_info_t
   * @brief     Used to define a complete configuration entity   
   */
   typedef struct _config_info_t
   {
      const tSIRF_CHAR* name; /* Name */
      tSIRF_UINT8       type; /* Data Type */ 
      union 
      {
         /* Entity Value: if Data type is String.  */
         tSIRF_CHAR      str_value[MAX_STR_KEY_VAL_LEN]; 
         /* Entity Value: if Data type is Integer. */
         tSIRF_INT32     int_value;                      
      } value;
   } config_info_t;

  /**
   * @fn           tSIRF_RESULT   config_table_init(tSIRF_VOID)
   * @brief        Initialises the resources of global configuration data 
   *               structure "theConfigTable".
   * @param        None.
   * @return       SIRF_SUCCESS: success; SIRF_FAILURE: failure.
   */
   tSIRF_RESULT   config_table_init(tSIRF_VOID);

  /**
   * @fn           tSIRF_RESULT   config_table_deinit(tSIRF_VOID)
   * @brief        Releases the resources of global configuration data  
   *               structure "theConfigTable".
   * @param        None.
   * @return       SIRF_SUCCESS: success; SIRF_FAILURE: failure.
   */
   tSIRF_RESULT   config_table_deinit(tSIRF_VOID);

  /**
   * @fn           config_table_update(eConfigSourceType aType)
   * @brief        Updates the global config structure by retrieving the 
   *               latest values from the "Config Source".
   * @param        aType : "Config Source" type e.g. Registry or XML.
   * @return       SIRF_SUCCESS: success; SIRF_FAILURE: failure.
   */
   tSIRF_RESULT   config_table_update(eConfigSourceType aType);

  /**
   * @fn           config_info_t* GetEntryValue(const tSIRF_CHAR* aName)
   * @brief        This function is used to retrieve the config entity
   *               associated with the provided tag name.
   * @param        aName : A null-terminated string specifies the tag name.
   * @return       on succes : Pointer to the associated config entity.
   *               on failure: NULL (0).
   */
   config_info_t* GetEntryValue(const tSIRF_CHAR* aName);

#endif /* __TAGMANAGER_H__ */

/**
 * @}
 * End of file.
 */
