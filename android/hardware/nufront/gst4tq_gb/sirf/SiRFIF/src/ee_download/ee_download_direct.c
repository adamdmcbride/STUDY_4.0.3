/**
 * @addtogroup ee_download
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
 *    $File: //customs/customer/Marvell-U1/sirf/Software/sirf/SiRFIF/src/ee_download/ee_download_direct.c $
 *
 *    $DateTime: 2011/10/17 14:33:40 $
 *
 *    $Revision: #2 $
 *
 */

/**
* @file: ee_download_direct.c
* @brief: Implementation of the EE_DOWNLOAD direct functions.
*/
#include <stdio.h>
#include <string.h>

#if (SIRF_INSTANT_FIX_GSW >= 24) || (SIRF_INSTANT_FIX_LPL >= 24)
#include "string_sif.h"
#endif
#include "sirf_pal_tcpip.h"
#include "ee_download_direct.h"
#include "ee_download.h"
#include "sirf_msg.h"
#include "sirf_msg_ssb.h"
#include "sirfnav_ui_io.h"
/******************************************************************************
 *
 * Precompiled headers
 *
 ******************************************************************************/

#define     HTTPNEWLINE  "\x0D\x0A"


/******************************************************************************
 *
 * Global Variables declarations and initialisation
 *
 ******************************************************************************/
#ifdef USE_URID 
        tSIRF_BOOL uridInfoValidFlagGlobal = SIRF_FALSE;
        tSIRF_UINT8  uridOemSubIdGlobal[OEM_SUB_ID_SIZE];
        tSIRF_UINT8 uridDeviceIdGlobal[DEVICE_ID_SIZE];
#endif
static stateDirectDwnldMod stateDirectDwnldEnum = STATE_EE_DOWNLOAD_DIRECT_STOPPED;
static const tSIRF_CHAR   *lenstr                 = "Content-Length:";
static tSIRF_UINT8 runningDownaloadDirect = 0;
extern tEE_DOWNLOAD_STATUS gSIFReponse;
extern tEE_DOWNLOAD_STATUS (*EE_Download_CallBackFnPtr)(tEE_DOWNLOAD_CALLBACK_TYPE, tSIRF_UINT8 *pData, tSIRF_UINT32 dataLength, tEE_DOWNLOAD_STATUS);
/**
 * @fn tEE_DOWNLOAD_STATUS Direct_Download_Start(tSIRF_UINT8 * pEEFilebuffer,
 *                                          tSIRF_CHAR   * pServerAddress,
 *                                          tSIRF_UINT16 serverPort,
 *                                          tSIRF_CHAR   *pServerFile,
 *                                          tSIRF_CHAR   *pServerAuth,
 *                                          tSIRF_UINT8 serverFileFormat )
 * @brief Direct_Download_Start
 *
 * @description To start the EE Download using sockets.
 *              pass all valid required paramaters.
 *
 * @param[in] pServerAddress Server address
 * @param[in] serverPort server port number
 * @param[in] pServerFile  pointer to server file name
 * @param[in] pServerAuth server authentication code
 * @param[in] serverFileFormat file format downloaded from server
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code
 *         defined in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS Direct_Download_Start( tSIRF_UINT8 * pEEFilebuffer,
        tSIRF_CHAR   *pServerAddress,
        tSIRF_UINT16 serverPort,
        tSIRF_CHAR   *pServerFile,
        tSIRF_CHAR   *pServerAuth,
        tSIRF_UINT8 serverFileFormat )
{

    tSIRF_UINT32 actualLen     = 0;
    tSIRF_INT32 file_length    = 0;
    tSIRF_INT32 file_start     = 0;
    tEE_DOWNLOAD_STATUS returnVal = EE_DOWNLOAD_SUCCESS;
    tSIRF_UINT32 fileHeaderSize = 0;
    tSIRF_UINT8 headerProcessed = 0;
    tSIRF_UINT32 downloadSize = 0;
    tSIRF_UINT32 bytesRcvd = 0;
    tSIRF_UINT32 reqSize = 0;
#ifdef USE_URID
   tSIRF_CHAR urid_string[ CSR_URID_STRING_SIZE ];
   tSIRF_UINT32 urid_result;
#endif /* USE_URID */

#if SIRF_INSTANT_FIX_GSW
    tSIRF_RESULT result = SIRF_SUCCESS;
    tSIRF_MSG_SSB_EE_FILE_PART msg;
#endif
    (void)serverFileFormat; 

    if (STATE_EE_DOWNLOAD_DIRECT_STARTED == stateDirectDwnldEnum)
    {
        returnVal = EE_DOWNLOAD_ALREADY_STARTED;
    }
    else
    {
        runningDownaloadDirect = 1;
        stateDirectDwnldEnum = STATE_EE_DOWNLOAD_DIRECT_STARTED;
#ifdef USE_URID
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
            "EE_DOWNLOAD: uridInfoValidFlagGlobal %d \n", uridInfoValidFlagGlobal);            
    if(SIRF_TRUE == uridInfoValidFlagGlobal)
    {
        urid_result = CSR_URID_Get_URID_String( urid_string, sizeof(urid_string) );
        if ( URID_SUCCESS != urid_result )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                            "EE_DOWNLOAD: Syn_Direct_Download_Start failed,CSR_URID_Get_URID_String failed %d!!!\n", urid_result );            
            returnVal = EE_DOWNLOAD_GET_URID_FAILED;
        }
        else
        {
            snprintf( (tSIRF_CHAR *)pEEFilebuffer, MAX_SIZE_OF_RECV_BUFFER, "GET %s HTTP/1.1\nHost: %s:%d\nAuthorization: Basic %s\nURID: %s\n\n",
                     pServerFile, pServerAddress, serverPort, pServerAuth, urid_string );
        }
    }
    else
    {
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                        "EE_DOWNLOAD: Syn_Direct_Download_Start failed, no valid OED ID and SubID!!!\n" );
        returnVal = EE_DOWNLOAD_NO_URID_INFO;
    }
#else
        snprintf( (tSIRF_CHAR *)pEEFilebuffer, MAX_SIZE_OF_RECV_BUFFER, "GET %s HTTP/1.1\nHost: %s:%d\nAuthorization: Basic %s\n\n",
                 pServerFile, pServerAddress, serverPort, pServerAuth );
#endif /* USE_URID */
        /* write request to the download socket (???  writeset[0] = tcp_sock;???? */
        returnVal = EE_Download_Send( pEEFilebuffer, (tSIRF_UINT32)strlen( (tSIRF_CHAR *)pEEFilebuffer ) );
        if (EE_DOWNLOAD_SUCCESS != returnVal)
        {
            return returnVal;
        }

        memset( pEEFilebuffer, 0, MAX_SIZE_OF_RECV_BUFFER );

        downloadSize = fileHeaderSize = MAX_HTTP_HEADER_SIZE; /* Max Valid HTTP header size */
        bytesRcvd = 0;
        /* read response and EE file data */
        do
        {

            reqSize = (downloadSize < MAX_SIZE_OF_RECV_BUFFER)? downloadSize : MAX_SIZE_OF_RECV_BUFFER;
            returnVal = EE_Download_Receive( pEEFilebuffer + bytesRcvd,reqSize, &actualLen) ;
            if (EE_DOWNLOAD_SUCCESS == returnVal)
            {
                bytesRcvd += actualLen;
                downloadSize -= actualLen;
            }
            else
            {
                /* Reset SGEE Module states by indicating '0' size in receive buffer */
#if SIRF_INSTANT_FIX_GSW
                msg.buffSize = 0;
                result = SiRFNav_Input(SIRF_MSG_SSB_EE_FILE_PART, &msg, sizeof(msg));
                if( SIRF_SUCCESS == result )
                {
                    returnVal = EE_DOWNLOAD_INPUT_ERROR;
                    result = EE_Download_CheckForError(EE_DOWNLOAD_WAIT_FILE_PART);
                    if(SIRF_SUCCESS == result)
                    {
                        returnVal = gSIFReponse;/* response */
                    }
                    else if(SIRF_PAL_OS_SEMAPHORE_WAIT_TIMEOUT == result)
                    {
                        /* In case of timeout, we need to reset the
                         * SIF Download state machine to make that ready for
                         * next download 
                         */
                        memset(&msg, 0x0, sizeof(msg));
                        (void)SiRFNav_Input(SIRF_MSG_SSB_EE_FILE_PART, &msg, sizeof(msg));
                    }
                }
                else if( SIRFNAV_UI_ERROR_NOT_STARTED == result )
                {
                    returnVal = EE_DOWNLOAD_INPUT_ERROR;
                }
                else
                {
                    returnVal = EE_DOWNLOAD_CORRUPTFILE_ERROR;
                }
#else
                (void)EE_Download_CallBackFnPtr(EE_DOWNLOAD_DATA_CALLBACK,pEEFilebuffer, 0, 0 );
#endif
                return returnVal;
            }

            /* if we've got header and beginning of file, parse header now: */
            if ( bytesRcvd == fileHeaderSize && (!headerProcessed))
            {
                if ( !strstr( (tSIRF_CHAR*)pEEFilebuffer, lenstr) )
                {
                    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                     "EE_DOWNLOAD: Direct_Download_Start HTTP header parsing error; header dump follows:\n" );
                    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                     "%s\n", pEEFilebuffer );

                    return EE_DOWNLOAD_CORRUPTFILE_ERROR;
                }

                /* coverity[secure_coding] */
                if(1 != sscanf( strstr( (tSIRF_CHAR*)pEEFilebuffer, lenstr ) + strlen( lenstr ), "%ld", &file_length ))
                {
                    return EE_DOWNLOAD_CORRUPTFILE_ERROR;
                }

                if ( strstr( (tSIRF_CHAR*)pEEFilebuffer, HTTPNEWLINE HTTPNEWLINE) )
                {
                    file_start = (tSIRF_INT32)( strstr( (tSIRF_CHAR*)pEEFilebuffer, HTTPNEWLINE HTTPNEWLINE ) + strlen( HTTPNEWLINE HTTPNEWLINE ) - (tSIRF_CHAR*)pEEFilebuffer );
                }
                else
                {
                    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                     "EE_DOWNLOAD: Direct_Download_Start HTTP header parsing error; header dump follows:\n" );
                    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                     "%s\n", pEEFilebuffer );
                    return EE_DOWNLOAD_CORRUPTFILE_ERROR;
                }
                downloadSize  = file_length;
                downloadSize -= (fileHeaderSize - file_start); /* Already downloaded */
                if ( downloadSize <= fileHeaderSize )
                {
                    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                    "EE_DOWNLOAD: Direct_Download_Start HTTP header parsing error; header dump follows:\n" );
                    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                    "%s\n", pEEFilebuffer );
                    return EE_DOWNLOAD_CORRUPTFILE_ERROR;
                }
                headerProcessed = 1;
            }
            if(headerProcessed)
            {
                /* Provide the EE file to CLM: bytesRcvd */
#if SIRF_INSTANT_FIX_GSW
                memcpy( &(msg.buff[0]), (tSIRF_UINT8 *)(pEEFilebuffer+file_start), (bytesRcvd-file_start));
                msg.buffSize = bytesRcvd-file_start;
                result = SiRFNav_Input(SIRF_MSG_SSB_EE_FILE_PART, &msg, sizeof(msg));
                if( SIRF_SUCCESS == result)
                {
                    returnVal = EE_DOWNLOAD_INPUT_ERROR;
                    result = EE_Download_CheckForError(EE_DOWNLOAD_WAIT_FILE_PART);
                    if(SIRF_SUCCESS == result)
                    {
                        returnVal = gSIFReponse;/* response */
                    }
                    else if(SIRF_PAL_OS_SEMAPHORE_WAIT_TIMEOUT == result)
                    {
                        /* In case of timeout, we need to reset the
                         * SIF Download state machine to make that ready for
                         * next download 
                         */
                        memset(&msg, 0x0, sizeof(msg));
                        (void)SiRFNav_Input(SIRF_MSG_SSB_EE_FILE_PART, &msg, sizeof(msg));
                    }
                }
                else if( SIRFNAV_UI_ERROR_NOT_STARTED == result )
                {
                    returnVal = EE_DOWNLOAD_INPUT_ERROR;
                }
                else
                {
                    returnVal = EE_DOWNLOAD_CORRUPTFILE_ERROR;
                }
#else
                returnVal = EE_Download_CallBackFnPtr(EE_DOWNLOAD_DATA_CALLBACK, pEEFilebuffer+file_start, bytesRcvd-file_start, 0 );
#endif
                if(returnVal != EE_DOWNLOAD_SUCCESS)
                {
                    returnVal = EE_DOWNLOAD_INPUT_ERROR;
                    /* Reset connection and download based on time scheme */
                    break;
                }
                file_start = 0;
                bytesRcvd = 0;
            }

        } while ( downloadSize && runningDownaloadDirect);

   }

   return returnVal;
}

/**
 * @fn tEE_DOWNLOAD_STATUS Direct_Download_Stop(tSIRF_VOID)
 * @brief Direct_Download_Stop
 *
 * @description To Stop the download using direct connection.
 *
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code
 *         defined in tEE_DOWNLOAD_STATUS
 */
tEE_DOWNLOAD_STATUS Direct_Download_Stop( tSIRF_VOID )
{


    if (STATE_EE_DOWNLOAD_DIRECT_STARTED != stateDirectDwnldEnum)
    {
        return EE_DOWNLOAD_GENERIC_FAILURE;
    }
    else
    {
        stateDirectDwnldEnum = STATE_EE_DOWNLOAD_DIRECT_STOPPED;
        runningDownaloadDirect = 0;
    }
    return EE_DOWNLOAD_SUCCESS;
}

/**
 * @}
 * End of file.
 */

