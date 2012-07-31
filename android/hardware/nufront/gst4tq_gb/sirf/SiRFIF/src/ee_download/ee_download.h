/**
 * @addtogroup ee_download
 * @ingroup SiRFInstantFix
 * @{
 */

/**
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
 */

/**
 * @file: ee_download.h
 * @brief: Definition of the EE_DOWNLOAD interface functions.
 */

#ifndef __EE_DOWNLOAD_H__
#define __EE_DOWNLOAD_H__

#include "sif_errors.h"
#include "sirf_types.h"
#include "ee_download_ui.h"

#define EE_DOWNLOAD_VERSION_GENERAL ("2.4-ALPHA-0000" )

/**
 * MAX_HTTP_HEADER_SIZE should be greater than or equal to 400 bytes
 **/
#define MAX_HTTP_HEADER_SIZE    (500)

/**
 * Receive buffer size MAX_SIZE_OF_RECV_BUFFER should be always 
 * greater than or equal to MAX_HTTP_HEADER_SIZE
 **/
#define MAX_SIZE_OF_RECV_BUFFER (SSB_DLD_MAX_PKT_LEN)

/* Reschedule time period in Seconds for EE Download when SGEE data is new and successfully downloaded */
#define RESCHEDULE_INTERVAL_NEW_FILE         (24*3600)
/* Reschedule time period in Seconds for EE Download when SGEE data is not new*/
#define RESCHEDULE_INTERVAL_NO_NEW_FILE      (4*3600)
/* Reschedule time period in Seconds for EE Download when SGEE data found corrupted after one retry*/
#define RESCHEDULE_INTERVAL_CORRUPT_FILE     (1*3600)
/* Reschedule time period in Seconds for EE Download when SGEE data found corrupted in first try*/
#define RESCHEDULE_INTERVAL_CORRUPT_RETRY    (120)
/* Reschedule time period in Seconds for EE Download when SiRFNAV is not ready or gives error*/
#define RESCHEDULE_INTERVAL_INPUT_ERROR      (30)
/* Reschedule time period in Seconds for EE Download when there is some network error*/
#define RESCHEDULE_INTERVAL_NET_ERROR        (10)
/* Wait time period in Seconds for response from SiRFNAV */
#define SGEE_FILE_PART_WAIT_TIMEOUT          (90)
/* Reschedule time period in Seconds for EE Download when retry count for Net Error or Input Error Exhausts*/
#define RESCHEDULE_INTERVAL_RETRY_EXHAUSTED     (2*3600)
/* defines for time */
#define SECS_IN_ONE_HOUR            (3600)
#define MILLISECS_IN_ONE_SEC        (1000)
/**
 * Used to represent the state of an EE Downloader MODULE.
 */
typedef enum {
    STATE_EE_DOWNLOAD_UNINITIALISED   = 0,            /*  EE Downloader module uninitialised successfully      */
    STATE_EE_DOWNLOAD_INITIALISED,                    /*  EE Downloader module initialised successfully          */
    STATE_EE_DOWNLOAD_STARTED,
    STATE_EE_DOWNLOAD_STOPPED

}stateEEDownldrMod;
/* Enter C naming convention */
#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/


/* To be implemented by EE downloader */
/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Send( tSIRF_UINT8 *bytes, tSIRF_UINT32 len )
 * @brief EE_Download_Send
 *
 * @description To Send the packet to network using network connection.
 *
 * @param[in] bytes pointer to the packet
 * @param[in] len length of the packet to be sent
 * @return tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Send( tSIRF_UINT8 *bytes, tSIRF_UINT32 len );

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Receive( tSIRF_UINT8 *bytes,
 *                                              tSIRF_UINT32 requestedLen,
 *                                              tSIRF_UINT32 *pActualLen )
 * @brief EE_Download_Receive
 *
 * @description To receive the packet from network using network connection.
 *
 * @param[in] bytes pointer to the received packet
 * @param[in] requestedLen expected length of packet to be received
 * @param[out] pActualLen Actual length of the packet received
 * @return tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Receive( tSIRF_UINT8 *bytes, tSIRF_UINT32 requestedLen,
                                       tSIRF_UINT32 *pActualLen );

/**
 * @fn tSIRF_RESULT EE_Download_CheckForError(tEE_DOWNLOAD_STATUS returnVal)
 * @brief EE_Download_Receive
 *
 * @description To receive the packet from network using network connection.
 *
 * @param[in] returnVal input wait type 
 * @return tSIRF_RESULT SIRF_SUCCESS/ corresponding error message
 *
 */
tSIRF_RESULT EE_Download_CheckForError(tEE_DOWNLOAD_STATUS returnVal);

#if defined (USE_CLM_DEBUG_LOG)
#include "clm_debug_ui.h"
#define EEDownloadDebug_Log CLMDebug_Log
#else
/* Module Id which can be used in set config options */
#define CLM_DLOG_MOD_ID_EEDOWNLOADER        0

#define DEBUG_OPTION(x,y)            x,y
tSIRF_INT32 EEDownloadDebug_Log(tSIRF_UINT32 moduleID, tSIRF_UINT32 level,tSIRF_VOID * pFormattedString,...);
#endif

/* Leave C naming convention */
#ifdef __cplusplus
}
#endif /*__cplusplus*/


#endif //__EE_DOWNLOAD_H__

/**
 * @}
 * End of file.
 */

