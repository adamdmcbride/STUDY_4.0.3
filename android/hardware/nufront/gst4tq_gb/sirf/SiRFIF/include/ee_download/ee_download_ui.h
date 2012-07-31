/**
 * @addtogroup ee_download
 * @ingroup SiRFInstantFix
 * @{
 */

/**
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006-2010 by SiRF Technology, a CSR plc Company.
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
 * @file: ee_download_ui.h
 * @brief: Definition of the EE_DOWNLOAD interface to user.
 */

#ifndef __EE_DOWNLOAD_UI_H__
#define __EE_DOWNLOAD_UI_H__

#include "sirf_types.h"
#include "sirfclm_config.h"

/* EE Downloader status codes */
/**
 * @enum tEE_DOWNLOAD_STATUS
 * @brief tEE_DOWNLOAD_STATUS
 *
 * @description EE Downloader status codes
 *
 */
typedef enum {
    EE_DOWNLOAD_SUCCESS = 0,                    /* Operation completed successfully */
    EE_DOWNLOAD_GENERIC_FAILURE = -1,           /* Operation encountered generic failure */
    EE_DOWNLOAD_STATUS_BASE = 0x08000000,       /* Base for EE Downloader*/
    EE_DOWNLOAD_INVALID_PARAM,                  /* Operation can not be allowed with passed input parameters.*/
    EE_DOWNLOAD_ALREADY_INITIALISED,            /* EE Downloader has already been initialized.*/
    EE_DOWNLOAD_ALREADY_STARTED,                /* EE Downloader is already started and can not be configured now.*/
    EE_DOWNLOAD_PAL_RESOURCE_ERROR,             /* Error returned from PAL due to some resource problem.*/
    EE_DOWNLOAD_SGEE_NONEWFILE,                 /* Supplied file is not newer than the ephemeris stored in storage.*/
    EE_DOWNLOAD_NET_TIMEOUT,                    /* Network request is time out for the requested operation.*/
    EE_DOWNLOAD_NET_ERROR,                      /* Network request is giving Error for the requested operation.*/
    EE_DOWNLOAD_NET_CONNECTION_ERROR,           /* Network connection Error.*/
    EE_DOWNLOAD_NET_CONNECTION_CLOSED,          /* Network connection is already closed and requested operation can not be performed.*/
    EE_DOWNLOAD_INTERNET_CONNECT_ERROR,         /* Error while trying internet connection.*/
    EE_DOWNLOAD_HTTPOPENREQUEST_ERROR,          /* Error while trying for HTTP open request.*/
    EE_DOWNLOAD_HTTPSENDREQUEST_ERROR,          /* Error while trying for HTTP send request.*/
    EE_DOWNLOAD_INTERNETQUERYDATAAVAILABLE_ERROR,/* Error while querying for data available from Internet */
    EE_DOWNLOAD_INTERNETREADFILE_ERROR,         /* Errors in reading file using Internet APIs.*/
    EE_DOWNLOAD_INTERNETOPEN_ERROR,             /* Error in opening Internet connection APIs.*/
    EE_DOWNLOAD_INPUT_ERROR,                    /* Input error in downloading*/
    EE_DOWNLOAD_INCOMPLETEFILE_ERROR,           /* Downloaded file is incomplete.*/
    EE_DOWNLOAD_CORRUPTFILE_ERROR,              /* Downloaded file is corrupt.*/
    EE_DOWNLOAD_CREATEWINDOW_ERROR,             /* Failing in Window creation.*/
    EE_DOWNLOAD_INSUFFICIENT_BUFFER_ERROR,      /* Failing in Window creation.*/
    EE_DOWNLOAD_WAIT_FILE_PART,                 /* Wait for SGEE file part */
    EE_DOWNLOAD_INBUILT_DOWNLODER_RUNNING,      /* Wait for SGEE file part */
    EE_DOWNLOAD_NOT_RUNNNING,                   /* EE Downloader has not been started */
    EE_DOWNLOAD_THREAD_EXITING,                 /* EE Downloader thread is exiting */
    EE_DOWNLOAD_STOPPED,                        /* EE Downloader Stopped successfully */

    EE_DOWNLOAD_IDS_STATUS_BASE = 0x08100000,
    EE_DOWNLOAD_IDS_OK,                         /* Download file is successful.*/
    EE_DOWNLOAD_IDS_ERROR_OVERFLOW,             /* IDS encountered buffer overflow*/
    EE_DOWNLOAD_IDS_EXCEPTION_CAUGHT,           /* IDS encountered exception error.*/
    EE_DOWNLOAD_IDS_ERROR_DOWNLOAD_VERIFY,      /* Verification of downloaded data failed.*/
    EE_DOWNLOAD_IDS_ERROR_REG_DENY,             /* Registration with server is denied.*/
    EE_DOWNLOAD_IDS_ERROR_DOWNLOAD_DENY         /* Download request from the server is denied,
                                                   may be because of invalid user id and authorization.*/

}tEE_DOWNLOAD_STATUS;

/* Sleep counts in seconds : must ne less than or equal to 10Secs*/
#define SLEEP_COUNT_IN_SEC              (5)

/*File formats*/
#define EE_DOWNLOAD_OTA_FORMAT_UNKNOWN           0x0
#define EE_DOWNLOAD_OTA_FORMAT_FF1           0x1
#define EE_DOWNLOAD_OTA_FORMAT_FF2           0x2

/* Maximum length of EE DOWNLOADER Module version string */
#define MAX_LEN_EE_DNLDR_VER_STRING     64

typedef enum
{
    EE_DOWNLOAD_DATA_CALLBACK = 0,
    EE_DOWNLOAD_STATUS_CALLBACK
}tEE_DOWNLOAD_CALLBACK_TYPE;

/* Enter C naming convention */
#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Init(tSIRF_CHAR   pServerAddress[2][80],
 *                                          tSIRF_UINT16 serverPort,
 *                                          tSIRF_CHAR   *pServerFile,
 *                                          tSIRF_CHAR   *pServerAuth,
 *                                          tSIRF_UINT8 serverFileFormat )
 * @brief EE_Download_Init
 *
 * @description To Init the EE Download.
 * For default settings: Pass any of (pServerAddress, serverPort, pServerFile,
 * pServerAuth ) as NULL/0 with valid OTA format Else pass all valid required
 * paramaters.
 *
 * @param[in] pServerAddress[2][80] pServerAddress[0][80] : Server address 1
 *                                  pServerAddress[1][80] : Server address 2,
 *                                                          failover server
 *                                  Use "127.0.0.1" for local EE file, local file
 *                                  must be stored as "NVM4" name.
 * @param[in] serverPort server port number
 * @param[in] pServerFile  pointer to server file name
 * @param[in] pServerAuth server authentication code
 * @param[in] serverFileFormat file format downloaded from server
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code defined in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Init(
    tSIRF_CHAR   pServerAddress[2][80],
    tSIRF_UINT16 serverPort,
    tSIRF_CHAR   *pServerFile,
    tSIRF_CHAR   *pServerAuth,
    tSIRF_UINT8 serverFileFormat );

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Start(tSIRF_VOID *callBackFunction )
 * @brief EE_Download_Start
 *
 *
 * @description To start the EE Download.
 * &param[in] callBackFunction, Callback function provided by Application
 *            tSIRF_VOID (*EE_Download_CallBackFnPtr)(tEE_DOWNLOAD_CALLBACK_TYPE, tSIRF_UINT8 *pData, tSIRF_UINT32 dataLength, tEE_DOWNLOAD_STATUS);
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code defined
 *         in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Start( tSIRF_VOID * callBackFunction);

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Stop(tSIRF_VOID)
 * @brief EE_Download_Stop
 *
 * @description To stop the EE Download.
 *
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code defined
 *         in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Stop(
    tSIRF_VOID
    );



/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_GetLibVersion(
 *                         tSIRF_CHAR  version[MAX_LEN_EE_DNLDR_VER_STRING])
 * @brief EE_Download_GetLibVersion
 *
 * @description To log the debug messages in log file or console
 *               depending upon the selected CLM configuration.
 *
 * @param[out] version [MAX_LEN_EE_DNLDR_VER_STRING] version of EE_Download LIbrary.
 *
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code
 *         defined in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_GetLibVersion(
    tSIRF_CHAR  version[MAX_LEN_EE_DNLDR_VER_STRING]
    );

/* Callback functions to be called by EE downloader to send the downloaded SGEE data to SIF,
   Application needs to implement this function IF EE Downloader Library is used with standalone
   application.
*/
/**
tEE_DOWNLOAD_STATUS (*EE_Download_CallBackFnPtr)(tEE_DOWNLOAD_CALLBACK_TYPE, tSIRF_UINT8 *pData, tSIRF_UINT32 dataLength, tEE_DOWNLOAD_STATUS) = NULL;
*/


/* This function will be required for the EE downloader  module to print on console
   if EE downloader is NOT built with USE_CLM_DEBUG_LOG flag */
/* Extern functions to be called by EE downloader to print on console,
   Application needs to implement this function IF EE Downloader Library is used with standalone
   application and not being used with SiRFNAV library.
   If EE Downloader library is used with SiRFNAV library, this function will come from there */
/**
 * @fn extern tSIRF_INT32 CLM_ConsolePrintf( tSIRF_CHAR *pFormat, ... )
 * @brief CLM_ConsolePrintf
 *
 * @description To be implemented by the User application to output
 *              formatted string on console.
 *              Can be implemented as "Empty function" if debug information
 *              is not requested by user application
 *
 * @param[in] pFormat Formatted string

 * @return SUCCESS =0, FAILURE = Non-Zero
 *
 */
extern tSIRF_INT32 CLM_ConsolePrintf( tSIRF_CHAR *pFormat, ... );

/**
 * @fn tSIRF_VOID EE_Download_SiRFNavResponse( tSIRF_UINT32 message_id, 
 *                                tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
 * @brief Handle response for send EE file part
 *
 * @description Handle response for send EE file part, this is call back function from application.
 *
 * @param[in] message_id message ID.
 * @param[in] *message_structure pointer to message structure.
 * @param[in] message_length length of message structure.
 *
 * @return tSIRF_VOID
 *
 */
tSIRF_VOID EE_Download_SiRFNavResponse( tSIRF_UINT32 message_id, 
                        tSIRF_VOID *message_structure, tSIRF_UINT32 message_length );

/* Leave C naming convention */
#ifdef __cplusplus
}
#endif /*__cplusplus*/


#endif //__EE_DOWNLOAD_UI_H__

/**
 * @}
 * End of file.
 */

