/*
 *
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2010 by SiRF Technology, a CSR plc Company.
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
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/app/AndroidSharedLib/source/gps_sirf_cpa.c $
 *
 *  $DateTime: 2011/08/30 16:01:28 $
 *
 *  $Revision: #3 $
 */
#include "gps_sirf_cpa.h"
#include "gps_logging.h"

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#include "LSM_APIs.h"

extern LSM_BOOL controlPowerWakeLock(int enable);
//extern void DBGPRINTF( const char * format, ... );

static int g_request_id = 0;

LSM_BOOL cpa_CP_SendMeasurementTerminate(eLSM_OTA_TYPE otaType)
{
    LSM_BOOL ret;
    
    ret = CP_SendMeasurementTerminate(otaType);
    controlPowerWakeLock(GPS_WAKE_LOCK_DISABLE);
    return ret;
}

LSM_BOOL cpa_CP_SendNetMessage(eLSM_OTA_TYPE ota_type,
                           eLSM_RRC_STATE rrc_state,
                           eLSM_SESSION_PRIORITY sessionPriority,
                           LSM_UINT8* p_msg_data,
                           LSM_UINT32 msg_size)
{
    controlPowerWakeLock(GPS_WAKE_LOCK_ENABLE);
    return CP_SendNetMessage(ota_type, rrc_state, sessionPriority, p_msg_data, msg_size);
}

/* Response from framework for frequency info */
int sirf_inject_hw_frequency(double frequency,  double accuracy)
{
   LSM_HW_FREQ_UPDATE freqInfo;
   memset(&freqInfo, 0x00, sizeof(LSM_HW_FREQ_UPDATE));
   SIRF_LOGD("sirf_inject_hw_frequency: IN Params: frq = %f Hz, acc= %f ppm", frequency, accuracy );
   freqInfo.request_id = g_request_id;
   freqInfo.frequency_data_type  = LSM_FREQUENCY_UPDATE_CENTER; /* Need to be verified*/
   freqInfo.accuracy_data_type   = LSM_FREQUENCY_ACCURACY_PPM; /* Need to be verified*/
   freqInfo.frequency = frequency;
   freqInfo.accuracy = accuracy;
   LSM_UpdateFrequency(&freqInfo);
   SIRF_LOGD("sirf_inject_hw_frequency: OUT" );
   return 1;
}




