/**
 * @addtogroup sif_interface
 * @ingroup SiRFInstantFix
 * @{
 */

/**
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006-2011 by SiRF Technology, a CSR plc Company.
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
 *  Keywords for Perforce.  Do not modify.
 *
 *  Archive:$File: //customs/customer/Marvell-U1/sirf/Software/sirf/SiRFIF/include/sif_sirfnav_interface.h $
 *
 *  Last modified by: $Author: kt01 $
 *
 *  $DateTime: 2011/10/17 14:33:40 $
 *
 *  Revision:$Revision: #2 $
 */

/**
 * @file: sif_sirfnav_interface.h
 * @brief: External User Interface to SIF2.2 interface functions.
 */

#ifndef __SIF_SIRFNAV_INTERFACE_H__
#define __SIF_SIRFNAV_INTERFACE_H__

#if ( ( SIRF_INSTANT_FIX_GSW ) || ( SIRF_INSTANT_FIX_LPL ) )

#include "sirf_types.h"
#include "clm_structs.h"
#include "clm_ui.h"
#include "clm_debug_ui.h"
#include "convert_ephem.h"

#include "sirf_msg.h"
#include "sirf_msg_ssb.h"

/**
 * Receive buffer size MAX_SIZE_OF_RECV_BUFFER should be always 
 * greater than or equal to MAX_HTTP_HEADER_SIZE
 **/
#define MAX_SIZE_OF_RECV_BUFFER (SSB_DLD_MAX_PKT_LEN)

/**
 * MAX_HTTP_HEADER_SIZE should be greater than or equal to 1000 bytes
 **/
#define MAX_HTTP_HEADER_SIZE    (1000)

#define RESCHEDULE_INTERVAL_NEW_FILE         (24*3600)
#define RESCHEDULE_INTERVAL_NO_NEW_FILE      (4*3600)
#define RESCHEDULE_INTERVAL_CORRUPT_FILE     (1*3600)
#define RESCHEDULE_INTERVAL_INPUT_SGEE_ERROR (30)

/* defines for time */
#define SECS_IN_ONE_HOUR            (3600)
#define MILLISECS_IN_ONE_SEC        (1000)

/* Enter C naming convention */
#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/

/**
 * Used to represent the status of an operation.
 */
/**
 * @enum tCLM_CGEE_PREDICTION_STATUS
 * @brief tCLM_CGEE_PREDICTION_STATUS
 *
 * @description Used to represent the prediction status of an operation.
 */
typedef enum {
    CLM_CGEE_PREDICTION_ENABLE = 0,
    CLM_CGEE_PREDICTION_DISABLE,
    CLM_CGEE_PREDICTION_PRGMD_TO_STOP,
}tCLM_CGEE_PREDICTION_STATUS;

    typedef struct{
    tSIRF_MSG_SSB_EE_CLK_BIAS_ADJ satClkBiasAdj[12];
} tSEA_CLK_BIAS_ADJUSTMENT;

/**
 * @fn tCLM_STATUS SIF_Init(tSIRF_MSG_SSB_SIF_SET_CONFIG *pSIFSetConfigMsg)
 * @brief CLM Initialisation function.
 *
 * @description To initialize the SIF and set CLM Configuration parameters 
 *              and EE downloader server parameters.
 *
 * @param[in] sifCfg pointer to tSIRF_MSG_SSB_SIF_SET_CONFIG,
 *                   It provides Input SIF/EE Downloader configuration and will be
 *                   Valid ONLY IF "pSIFSetConfigMsg->operationMode" has some valid Value
 *                   as CLM_CONFIG_MODE_CGEE/CLM_CONFIG_MODE_SGEE/CLM_CONFIG_MODE_MIXED.
 *                   If this is Passed as NULL then default SIF configuration will be taken.
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code
 *
 */

tCLM_STATUS SIF_Init(
    tSIRF_MSG_SSB_SIF_SET_CONFIG *pSIFSetConfigMsg
    );

/**
 * @fn tCLM_STATUS SIF_PostSIFStatus(tSIRF_VOID)
 * @brief Post SIF status for every 5 seconds.
 *
 * @description To post SIF status every 5 sec.
 *
 * @param[in] void
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code
 *
 */

tCLM_STATUS SIF_PostSIFStatus(tSIRF_VOID);

/**
 * @fn tCLM_STATUS SIF_Start(tSIRF_VOID)
 * @brief To Start the Sirf Instant Fix
 *
 * @description To start SIF Engine.
 *
 * @param
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code
 *
 */
tCLM_STATUS SIF_Start(tSIRF_VOID);

/**
 * @fn tCLM_STATUS SIF_Stop()
 * @brief To Stop the Sirf Instant Fix
 *
 * @description To stop SIF Engine.
 *
 * @param
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code
 *
 */
tCLM_STATUS SIF_Stop(tSIRF_VOID);

/**
 * @fn tCLM_STATUS SIF_Cleanup(tSIRF_VOID )
 * @brief SIF Interface file cleanup function.
 *
 * @description To Clean up the var,structs etc used for SIF Interface.
 *
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code
 *
 */
tCLM_STATUS SIF_Cleanup(
    tSIRF_VOID
);


/**
 * @fn tCLM_STATUS SIF_OutputMessage(tSIRF_UINT32 messageId,
 *                                     tSIRF_VOID *messageStructure,
 *                                     tSIRF_UINT32 messageLength)
 * @brief To give Onput messages from CLM to NAV
 *
 * @description To send the CLM message (parsed, complete and single
 *              in configured SSB protocol) to receiver.
 *
 * @param[in] messageId Message ID of SSB message
 * @param[in] messageStructure Pointer to message structure
 * @param[in] messageLength Message Length
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code
 *
 */
tCLM_STATUS SIF_OutputMessage(
    tSIRF_UINT32 messageId,
    tSIRF_VOID *messageStructure,
    tSIRF_UINT32 messageLength);

/**
 * @fn tCLM_STATUS SIF_InputMessage( tSIRF_UINT32 messageId,
 *                                     tSIRF_VOID *messageStructure,
 *                                     tSIRF_UINT32 messageLength)
 * @brief To give input messages to CLM using SIF
 *
 * @description To send the receiver message (parsed, complete and single
 *              in configured SSB protocol) .
 *
 * @param[in] messageId Message ID of SSB message
 * @param[in] messageStructure Pointer to message structure
 * @param[in] messageLength Message Length
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code
 *
 */
tCLM_STATUS SIF_InputMessage(
    tSIRF_UINT32 messageId,
    tSIRF_VOID *messageStructure,
    tSIRF_UINT32 messageLength
);

/**
 * @fn tCLM_STATUS SIF_InputConfigMessage(  tSIRF_MSG_SSB_SIF_SET_CONFIG *sif_config_message )
 * @brief To check input messages to CLM using SIF
 *
 * @description To check input messages to CLM using SIF
 *
 * @param[in] SIF config
 *
 * @return tSIRF_RESULT with CLM_SUCCESS or error code
 *
 */

tSIRF_RESULT SIF_InputConfigMessage( tSIRF_MSG_SSB_SIF_SET_CONFIG *sif_config_message );


/* Leave C naming convention */
#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* SIRF_INSTANT_FIX_GSW || SIRF_INSTANT_FIX_LPL*/

#endif //__SIF_SIRFNAV_INTERFACE_H__

/**
 * @}
 * End of file.
 */

