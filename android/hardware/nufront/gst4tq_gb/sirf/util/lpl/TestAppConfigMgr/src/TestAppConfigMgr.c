/**
 * @addtogroup app_ConfigMgr
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
 *                                                                         *
 *  MODULE:         app_ConfigMgr                                          *
 *                                                                         *
 *  FILENAME:       TestAppConfigMgr.c                                     *
 *                                                                         *
 *  DESCRIPTION:                                                           *      
 *                                                                         *
 *  NOTES:                                                                 *
 *                                                                         *
 ***************************************************************************/

#ifndef OS_UCOSII 
#include <memory.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef SIRF_AGPS
#include "sirf_host_sdk_version.h"
#include "sirf_types.h"
#if (SIRF_HOST_SDK_VERSION >= (400) )
#include "sirf_errors.h"
#endif/* SIRF_HOST_SDK_VERSION */
#endif/* SIRF_AGPS */

#include "string_sif.h"

#include "TestAppConfigMgr.h"

#include "XMLReader.h"
#include "RegistryReader.h"

static eConfigSourceType   gConfigSourceType;

/* Local functions */
static tSIRF_VOID UpdateAGPSConfig(AGPSConfig* pAgpsConfig);

#ifdef __cplusplus 
extern "C"
{
#endif /* __cplusplus  */

tSIRF_RESULT TestAppConfigMgr_Init(eConfigSourceType aType, tSIRF_CHAR* aParam)
{   
   tSIRF_RESULT res = SIRF_SUCCESS;
   gConfigSourceType = aType;

   switch(gConfigSourceType)
   {
      case eCONFIG_DEFAULT:
         break;

      case eCONFIG_XML:
      case eCONFIG_REGISTRY:
         if( eCONFIG_XML == gConfigSourceType)
         {
            if(SIRF_SUCCESS != OpenXMLSource(aParam))
            {
               res = SIRF_FAILURE;
               break;
            }
         }
         else
         {
            if(SIRF_SUCCESS != OpenRegistrySource(aParam))
            {
               res = SIRF_FAILURE;
               break;
            }
         }
         break;
      default:
         res = SIRF_FAILURE;
         break;
   }/* switch */

   if (SIRF_SUCCESS == res)
   {
      if(SIRF_SUCCESS != config_table_init())
      {
         if( eCONFIG_XML == gConfigSourceType)
         {
            CloseXMLSource();
         }
         else if( eCONFIG_REGISTRY == gConfigSourceType)
         {
            CloseRegistrySource();
         }
         res = SIRF_FAILURE;
      }
   }
   return res;
}

tSIRF_VOID TestAppConfigMgr_DeInit(tSIRF_VOID)
{
   config_table_deinit();
   if( eCONFIG_XML == gConfigSourceType)
   {
      CloseXMLSource();
   }
   else if( eCONFIG_REGISTRY == gConfigSourceType)
   {
      CloseRegistrySource();
   }
}

AGPSConfig *getCfgPointer(tSIRF_VOID)
{
   static AGPSConfig agpsConfig;
   return(&agpsConfig);
}


tSIRF_RESULT getConfig(AGPSConfig *pAgpsConfig) 
{
   if (SIRF_SUCCESS != config_table_update(gConfigSourceType))
   {
      return SIRF_FAILURE;
   }
   UpdateAGPSConfig(pAgpsConfig);
   return SIRF_SUCCESS;
}

tSIRF_VOID UpdateAGPSConfig(AGPSConfig* pAgpsConfig)
{   
   config_info_t* entry;
   int i=0;

   /* GPS Port */ 
   entry = GetEntryValue(TAG_GPS_PORT);
   strlcpy(pAgpsConfig->GPSPort, (entry->value.str_value), MAX_PORT_STRING_LENGTH);

   /* Reset Port */
   entry = GetEntryValue(TAG_RESET_PORT);
   strlcpy(pAgpsConfig->ResetPort, entry->value.str_value, MAX_PORT_STRING_LENGTH);

   /* AUX Port */
   entry = GetEntryValue(TAG_AUX_PORT);
   strlcpy(pAgpsConfig->AUXPort, entry->value.str_value, MAX_PORT_STRING_LENGTH);

   /* GPRS Modem Port */
   entry = GetEntryValue(TAG_MODEM_PORT);
   strlcpy(pAgpsConfig->GPRSModemPort, entry->value.str_value, MAX_PORT_STRING_LENGTH);

   /* Bearer */
   entry = GetEntryValue(TAG_BEARER);
   strlcpy(pAgpsConfig->Bearer, entry->value.str_value, MAX_LOGIN_INFO_LEN);

   /* APN */
   entry = GetEntryValue(TAG_APN);
   strlcpy(pAgpsConfig->Apn, entry->value.str_value, MAX_LOGIN_INFO_LEN);

   /* Login */
   entry = GetEntryValue(TAG_LOGIN);
   strlcpy(pAgpsConfig->Login, entry->value.str_value, MAX_LOGIN_INFO_LEN);

   /* Password */
   entry = GetEntryValue(TAG_PASSWORD);
   strlcpy(pAgpsConfig->Password, entry->value.str_value, MAX_LOGIN_INFO_LEN);
   
   /* Modem */
   entry = GetEntryValue(TAG_MODEM);
   strlcpy(pAgpsConfig->Modem, entry->value.str_value, MAX_LOGIN_INFO_LEN);

   /* PhoneNumber */
   entry = GetEntryValue(TAG_PHONE_NUMBER);
   strlcpy(pAgpsConfig->PhoneNumber, entry->value.str_value, MAX_LOGIN_INFO_LEN);

   /* NumberOfSessions */
   entry = GetEntryValue(TAG_SESSIONS);
   pAgpsConfig->numOfSessions = (tSIRF_UINT16) entry->value.int_value;  

   /* ServerAddress */
   entry = GetEntryValue(TAG_AGPS_ADDRESS);
   strlcpy(pAgpsConfig->serverAddress, entry->value.str_value, MAX_SERVER_ADDRESS_LEN);
 
   /* serverPort */
   entry = GetEntryValue(TAG_AGPS_PORT);
   pAgpsConfig->serverPort = (tSIRF_UINT16) entry->value.int_value;  
   
   /* isSecure */
   entry = GetEntryValue(TAG_AGPS_SECURITY);
   pAgpsConfig->isSecure = (tSIRF_UINT8) entry->value.int_value;  

   /* Reset Type */   
   entry = GetEntryValue(TAG_RESET_TYPE);
   pAgpsConfig->resetType = (tSIRF_UINT8) entry->value.int_value;  

   /* Aiding Type */
   entry = GetEntryValue(TAG_AIDING_TYPE);
   pAgpsConfig->aidingType = (tSIRF_UINT8) entry->value.int_value;  

   /* PositionTechnologyMSB */
   entry = GetEntryValue(TAG_POS_TECH_MSB);
   pAgpsConfig->isPosTechMSB = (tSIRF_UINT8) entry->value.int_value;  

   /* PositionTechnologyMSA */
   entry = GetEntryValue(TAG_POS_TECH_MSA);
   pAgpsConfig->isPosTechMSA = (tSIRF_UINT8) entry->value.int_value;  

   /* PositionTechnologyAutonomous */
   entry = GetEntryValue(TAG_POS_TECH_AUTO);
   pAgpsConfig->isPosTechAuto = (tSIRF_UINT8) entry->value.int_value;  

   /* PositionTechnologyECID */
   entry = GetEntryValue(TAG_POS_TECH_ECID);
   pAgpsConfig->isPosTechECID = (tSIRF_UINT8) entry->value.int_value;  

   /* PreferredLocMethod */
   entry = GetEntryValue(TAG_PREF_METHOD);
   pAgpsConfig->prefLocMethod = (tSIRF_UINT8) entry->value.int_value;  
 
   /* FixType */
   entry = GetEntryValue(TAG_FIX_TYPE);

   /* NumberOfFixes */
   entry = GetEntryValue(TAG_FIXES);
   pAgpsConfig->numberOfFixes = (tSIRF_UINT8) entry->value.int_value;  

   /* TimeBetweenFixes */
   entry = GetEntryValue(TAG_INTERVAL);
   pAgpsConfig->timeBetweenFixes = (tSIRF_UINT8) entry->value.int_value;  

   /* AssistProtocol */
   entry = GetEntryValue(TAG_ASSIST_PROTO);
   pAgpsConfig->AssistProtocol = (tSIRF_UINT8) entry->value.int_value;  

    /* ApproximateLocation */
   entry = GetEntryValue(TAG_APX_LOC);
   pAgpsConfig->isApproximateLocationPresent = (tSIRF_BOOL) entry->value.int_value;  

   if(pAgpsConfig->isApproximateLocationPresent)
   {
      /* ApproximateLatitude */
      entry = GetEntryValue(TAG_APX_LAT);
      pAgpsConfig->approximateLatitude = (tSIRF_DOUBLE) atof(entry->value.str_value);  

      /* ApproximateLongitude */
      entry = GetEntryValue(TAG_APX_LON);
      pAgpsConfig->approximateLongitude = (tSIRF_DOUBLE) atof(entry->value.str_value);  

      /* ApproximateAltitude */
      entry = GetEntryValue(TAG_APX_ALT);
      pAgpsConfig->approximateAltitude = (tSIRF_UINT16) entry->value.int_value;  

      /* ApproximateHorErr*/
      entry = GetEntryValue(TAG_APX_HERR);
      pAgpsConfig->approximateHorErr = (tSIRF_UINT16) entry->value.int_value;  

      /* ApproximateVerErr */
      entry = GetEntryValue(TAG_APX_VERR);
      pAgpsConfig->approximateVerErr = (tSIRF_UINT16) entry->value.int_value;  
   }
   else
   {
      pAgpsConfig->approximateLatitude = (tSIRF_DOUBLE) ~0;  
      pAgpsConfig->approximateLatitude = (tSIRF_DOUBLE) ~0;  
      pAgpsConfig->approximateLatitude = (tSIRF_UINT16) ~0;  
   }
   
   /* IsQoSinSUPLSTART */
   entry = GetEntryValue(TAG_QOP);
   pAgpsConfig->QoSinSUPLSTART = (tSIRF_UINT16) entry->value.int_value;  

   /* QoShorizontalAccuracy */
   entry = GetEntryValue(TAG_HOR_ACCURACY);
   pAgpsConfig->QoSHorizontalAccuracy = (tSIRF_UINT8) entry->value.int_value;  

   /* QoSverticalAccuracy */
   entry = GetEntryValue(TAG_VER_ACCURACY);
   pAgpsConfig->QoSVerticalAccuracy = (tSIRF_UINT8) entry->value.int_value;  

   /* QoSMaxResponseTime */
   entry = GetEntryValue(TAG_MAX_RESP_TIME);
   pAgpsConfig->QoSMaxResponseTime = (tSIRF_UINT8) entry->value.int_value;  

   /* QoSMaxLocAge */
   entry = GetEntryValue(TAG_MAX_LOC_AGE);
   pAgpsConfig->maxLocAge = (tSIRF_UINT8) entry->value.int_value;  


   /* LPL Capabilities */

   /* NAVBitAiding */
   entry = GetEntryValue(TAG_NAV_BIT_AID);
   pAgpsConfig->isCapNavBitAiding = (tSIRF_BOOL) entry->value.int_value;  

   /* CGEE */
   entry = GetEntryValue(TAG_CGEE);
   pAgpsConfig->isCapCGEE = (tSIRF_BOOL) entry->value.int_value;  

   /* SGEE */
   entry = GetEntryValue(TAG_SGEE);
   pAgpsConfig->isCapSGEE = (tSIRF_BOOL) entry->value.int_value;  

   /* EE FileUpdate */
   entry = GetEntryValue(TAG_EEFILE);
   pAgpsConfig->eeFileUpdate = (tSIRF_BOOL) entry->value.int_value;  
  
   /* SGEE Server address1 */
   entry = GetEntryValue(TAG_SGEE_SERVER_ADR1);
   strlcpy(pAgpsConfig->SGEEserverAddress[0], entry->value.str_value, MAX_SGEE_ADDRESS_LEN);

   /* SGEE Server address2 */
   entry = GetEntryValue(TAG_SGEE_SERVER_ADR2);
   strlcpy(pAgpsConfig->SGEEserverAddress[1], entry->value.str_value, MAX_SGEE_ADDRESS_LEN);

   /* SGEE serverPort */
   entry = GetEntryValue(TAG_SGEE_SERVER_PORT);
   pAgpsConfig->SGEEserverPort = (tSIRF_UINT16)entry->value.int_value;  

   /* SGEE Server File */
   entry = GetEntryValue(TAG_SGEE_SERVER_FILE);
   strlcpy(pAgpsConfig->SGEEserverFile, entry->value.str_value, MAX_PATH_LEN);
   
   /* SGEE Server Auth */
   entry = GetEntryValue(TAG_SGEE_SERVER_AUTH);
   strlcpy(pAgpsConfig->SGEEserverAuth, entry->value.str_value, MAX_LOGIN_INFO_LEN);

   /* SGEE URID Flag */
   entry = GetEntryValue(TAG_SGEE_URID_FLAG);
   pAgpsConfig->SGEEuridFlag = (tSIRF_BOOL) entry->value.int_value;  
   
   /* SGEE URID Device Id */ /* Do not use strcpy using bytes */
   entry = GetEntryValue(TAG_SGEE_URID_DEVICEID);
   for (i =0; i<MAX_DEVICEID_LEN; i++)
   {
      int temp =0; 
      sscanf((entry->value.str_value)+(2*i), "%2hhx", &temp);
      pAgpsConfig->SGEEuridDeviceId[MAX_DEVICEID_LEN -1 - i] = 0xFF & temp;
   }

   /* SGEE URID OEM Id */ /* Do not use strcpy using bytes */
   entry = GetEntryValue(TAG_SGEE_URID_OEMSUBID);
   for (i =0; i<MAX_OEMSUBID_LEN; i++)
   {
      int temp =0; 
      sscanf((entry->value.str_value)+(2*i), "%2hhx", &temp);
      pAgpsConfig->SGEEuridOemSubId[MAX_OEMSUBID_LEN -1 - i] = 0xFF & temp;
   }

   /* Network Status */
   entry = GetEntryValue(TAG_NETWORK_STATUS);
   pAgpsConfig->networkStatus = (tSIRF_BOOL) entry->value.int_value;  

   /* LOGGING INFO */

   /* LoggingType */
   entry = GetEntryValue(TAG_LOG_TYPE);
   pAgpsConfig->loggingType = (tSIRF_UINT8) entry->value.int_value;  

   /* BriefLog */
   entry = GetEntryValue(TAG_LOG_BRIEF);
   strlcpy(pAgpsConfig->briefLogFileName, entry->value.str_value, MAX_PATH_LEN);

   /* DetailedLog */
   entry = GetEntryValue(TAG_LOG_DETAIL);
   strlcpy(pAgpsConfig->detailedLogFileName, entry->value.str_value, MAX_PATH_LEN);

   /* AGPSLog */
   entry = GetEntryValue(TAG_LOG_AGPS);
   strlcpy(pAgpsConfig->agpsLogFileName, entry->value.str_value, MAX_PATH_LEN);

   /* SLCLog */
   entry = GetEntryValue(TAG_LOG_SLC);
   strlcpy(pAgpsConfig->slcLogFileName, entry->value.str_value, MAX_PATH_LEN);

   /* SNLog */
   entry = GetEntryValue(TAG_LOG_SN);
   strlcpy(pAgpsConfig->snLogFileName, entry->value.str_value, MAX_PATH_LEN);

   /* SiRF Aware */
   entry = GetEntryValue(TAG_SIRF_AWARE);
   pAgpsConfig->isSiRFAware = (tSIRF_BOOL) entry->value.int_value;  
   
   /* NMEA Listener */
   entry = GetEntryValue(TAG_LISTENER_NMEA);
   pAgpsConfig->enableNMEAListener = (tSIRF_BOOL) entry->value.int_value;  
   
   /* SSB Listener */
   entry = GetEntryValue(TAG_LISTENER_SSB);
   pAgpsConfig->enableSSBListener = (tSIRF_BOOL) entry->value.int_value;  

   /* Debug Listener */
   entry = GetEntryValue(TAG_LISTENER_DEBUG);
   pAgpsConfig->enableDebugListener = (tSIRF_BOOL) entry->value.int_value;  
   
   /* Serial Listener */
   entry = GetEntryValue(TAG_LISTENER_SERIAL);
   pAgpsConfig->enableSerialListener = (tSIRF_BOOL) entry->value.int_value;  
}

#ifdef __cplusplus 
}
#endif /* __cplusplus  */

/**
 * @}
 * End of file.
 */
