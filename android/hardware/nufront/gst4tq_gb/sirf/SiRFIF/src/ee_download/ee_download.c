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
 *    $File: //customs/customer/Marvell-U1/sirf/Software/sirf/SiRFIF/src/ee_download/ee_download.c $
 *
 *    $DateTime: 2011/10/17 14:33:40 $
 *
 *    $Revision: #2 $
 *
 */

/**
* @file: ee_download.c
* @brief: Implementation of the EE_DOWNLOAD interface functions.
*/

#include <stdio.h>
#include <string.h>

#include "ee_download.h"
#include "ee_download_ui.h"

#include "sirf_pal_os_semaphore.h"
#include "sirf_pal_tcpip.h"
#include "sirf_pal_os_thread.h"
/* used by EE_Download_Local */
#include "sirf_pal_storage.h"
#include "sif_errors.h"
#include "sirfnav_ui_io.h"
#include "sirf_msg.h"
#include "sirf_msg_ssb.h"

#define VERSION_POSIX_METHOD " : POSIX"
/******************************************************************************
 *
 * Enum Types
 *
 ******************************************************************************/
#define MAX_RETRY_NET_ERROR 5
#define MAX_RETRY_INPUT_ERROR 3
/******************************************************************************
 *
 * Global Variables declarations and initialisation
 *
 ******************************************************************************/
static stateEEDownldrMod stateEEDownldrEnum = STATE_EE_DOWNLOAD_UNINITIALISED;
static tSIRF_SEMAPHORE    eeDownloaderSemaphore;
static tSIRF_THREAD       downloaderThreadHandle;
/* This is used to check if Download retry is required in case of corrupt file
   due to some error while downloading */
static tSIRF_UINT8        gDnldrRetryCntOnCorruptFile;
static tSIRF_UINT8        gDnldrRetryCntOnNetError;
static tSIRF_UINT8        gDnldrRetryCntOnInputError;
tEE_DOWNLOAD_STATUS       gSIFReponse;
static tSIRF_SOCKET       downloadSocketGlobal  = 0;
static tSIRF_UINT8        downloaderRunningGlobal = 0;

static tSIRF_UINT8        serverFileFormatGlobal = EE_DOWNLOAD_OTA_FORMAT_FF1;

static tSIRF_CHAR      serverAuthGlobal[256]        = "GET_NEW_CODE"; /* Get New authentication code */

/* Receive Buffer required to be allocated and passed to below IDS/DirectDownload*/
static tSIRF_UINT8     downloaderRecvBufferGlobal[MAX_SIZE_OF_RECV_BUFFER];     /* buffer for receiving data*/

/* SiRF's demo server address*/
/* due to always failure in connecting to eedemo2.sirf.com both are made eedemo1.sirf.com */
static tSIRF_CHAR      serverAddressGlobal[2][80]   = { "192.168.54.214", "192.168.54.214" };
static tSIRF_UINT16    serverPortGlobal             = 80;
static tSIRF_CHAR      serverFileGlobal[256]        = "/diff/packedDifference.f1p7.ee";      /* most current file*/

/* extern functions implemented by direct downloader*/
extern tEE_DOWNLOAD_STATUS Direct_Download_Start( tSIRF_UINT8 * pEEFilebuffer,
            tSIRF_CHAR   *pServerAddress,
            tSIRF_UINT16 serverPort,
            tSIRF_CHAR   *pServerFile,
            tSIRF_CHAR   *pServerAuth,
            tSIRF_UINT8 serverFileFormat );
extern tEE_DOWNLOAD_STATUS Direct_Download_Stop(tSIRF_VOID);

/* Callback function provided by Application */
tEE_DOWNLOAD_STATUS (*EE_Download_CallBackFnPtr)(tEE_DOWNLOAD_CALLBACK_TYPE, tSIRF_UINT8 *pData, tSIRF_UINT32 dataLength, tEE_DOWNLOAD_STATUS) = NULL;

static tEE_DOWNLOAD_STATUS EE_Download_SelfCleanup(  tSIRF_VOID );

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Send( tSIRF_UINT8 *bytes, tSIRF_UINT32 len )
 * @brief EE_Download_Send
 *
 * @description Send packet for the EE Download.
 *
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code
 *         defined in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Send( tSIRF_UINT8 *bytes, tSIRF_UINT32 len )
{
    tSIRF_SOCKET writeset[2] = { 0, 0 };
    tSIRF_RESULT result  = SIRF_SUCCESS;
    tSIRF_UINT32 byte_count  = 0;

    if ( ( len > 0 ) && ( bytes == NULL ) )
    {
        return EE_DOWNLOAD_INVALID_PARAM;
    }

    while ( len > 0 )
    {
        memset(writeset,(int)SIRF_PAL_NET_INVALID_SOCKET,sizeof(writeset));
        writeset[0] = downloadSocketGlobal;
        result  = SIRF_PAL_NET_Select( NULL, writeset, NULL, 30000 );
        if (SIRF_SUCCESS == result)
        {
            result = SIRF_PAL_NET_Write( downloadSocketGlobal, bytes, len, &byte_count );
        }
        if ( result ) break;

        bytes    += byte_count;
        len    -= byte_count;
    }
    if ( SIRF_SUCCESS == result )
    {
        return EE_DOWNLOAD_SUCCESS;
    }
    else if ( SIRF_PAL_NET_TIMEOUT == result )
    {
        return EE_DOWNLOAD_NET_TIMEOUT;
    }
    else
    {
        return EE_DOWNLOAD_NET_ERROR;
    }
}

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Receive( tSIRF_UINT8 *bytes,
 *                                              tSIRF_UINT32 requestedLen,
 *                                              tSIRF_UINT32 *pActualLen )
 * @brief EE_Download_Receive
 *
 * @description Receive packet for the EE Download.
 *
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code
 *         defined in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Receive( tSIRF_UINT8 *bytes, tSIRF_UINT32 requestedLen, tSIRF_UINT32 *pActualLen )
{
    tSIRF_RESULT result      = SIRF_SUCCESS;
    tSIRF_SOCKET readset[2]  = {0,0};

    if ( ( ( requestedLen > 0 ) && ( NULL == bytes ) )||
         ( NULL == pActualLen ) )
    {
        return EE_DOWNLOAD_INVALID_PARAM;
    }
    *pActualLen  = 0;
    memset(readset,(int)SIRF_PAL_NET_INVALID_SOCKET,sizeof(readset));
    readset[0]  = downloadSocketGlobal;
    result      = SIRF_PAL_NET_Select( readset, NULL, NULL, 30000 );
    if ( SIRF_SUCCESS == result )
    {
        result = SIRF_PAL_NET_Read( downloadSocketGlobal, bytes, requestedLen, pActualLen );
    }
    if ( SIRF_SUCCESS == result )
    {
        return EE_DOWNLOAD_SUCCESS;
    }
    else if ( SIRF_PAL_NET_CONNECTION_CLOSED == result )
    {
        return EE_DOWNLOAD_NET_CONNECTION_CLOSED;
    }
    else
    {
        return EE_DOWNLOAD_NET_ERROR;
    }
}

/**
 * @fn tSIRF_RESULT EE_Download_Wait( tSIRF_INT32 waitInSeconds )
 * @brief EE_Download_Wait
 *
 * @description Sleep execution for SGEE download.
 *
 * @return tSIRF_RESULT
 *
 */
tSIRF_RESULT EE_Download_Wait( tSIRF_INT32 waitInSeconds )
{
    tSIRF_RESULT result = SIRF_FAILURE;
    if ( waitInSeconds && downloaderRunningGlobal )
    {
       result = SIRF_PAL_OS_SEMAPHORE_Wait( eeDownloaderSemaphore, waitInSeconds * MILLISECS_IN_ONE_SEC );

       if ( SIRF_SUCCESS != result )
       {
           /* Check if there is timeout for wait or there is some error */
           if ( SIRF_PAL_OS_SEMAPHORE_WAIT_TIMEOUT == result )
           {
                EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                    "EE_Download_Wait timedout time = %d sec!!!\n",waitInSeconds );
           }
           else
           {
                EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                    "EE_Download_Wait failed.0x%x\n",result );
           }
       }
    }
    return result;
}

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

tSIRF_RESULT EE_Download_CheckForError(tEE_DOWNLOAD_STATUS returnVal)
{
    tSIRF_INT32 waitInSeconds = 0;
    tSIRF_RESULT result = SIRF_SUCCESS;
    switch( returnVal )
    {
        case EE_DOWNLOAD_SUCCESS:
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: Successful download!!! ");
            waitInSeconds = RESCHEDULE_INTERVAL_NEW_FILE;
            gDnldrRetryCntOnCorruptFile = 0;
            gDnldrRetryCntOnNetError = 0;
            gDnldrRetryCntOnInputError = 0;  
        }
        break;
        case EE_DOWNLOAD_SGEE_NONEWFILE:
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD:No New File" );
            waitInSeconds = RESCHEDULE_INTERVAL_NO_NEW_FILE;
            gDnldrRetryCntOnCorruptFile = 0;
            gDnldrRetryCntOnNetError = 0;
            gDnldrRetryCntOnInputError = 0;
        }
        break;
        case EE_DOWNLOAD_INCOMPLETEFILE_ERROR:
        case EE_DOWNLOAD_CORRUPTFILE_ERROR:
        {
            gDnldrRetryCntOnNetError = 0;
            gDnldrRetryCntOnInputError = 0;
            /* Check if retry is required in case of corrupt file */
            if( gDnldrRetryCntOnCorruptFile )
            {
                waitInSeconds = RESCHEDULE_INTERVAL_CORRUPT_FILE;
                gDnldrRetryCntOnCorruptFile = 0;
            }
            else
            {
                /* Retry once in case of corrupt file after RESCHEDULE_INTERVAL_CORRUPT_RETRY */
                waitInSeconds = RESCHEDULE_INTERVAL_CORRUPT_RETRY;
                gDnldrRetryCntOnCorruptFile++;
            }
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                    "EE_DOWNLOAD: Failed! Corrupt/incomplete file, ret 0x%x", returnVal);
        }
        break;
        case EE_DOWNLOAD_INPUT_ERROR:
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: Failed! SGEE Input Error." );
            if(MAX_RETRY_INPUT_ERROR > gDnldrRetryCntOnInputError)
            {
                waitInSeconds = RESCHEDULE_INTERVAL_INPUT_ERROR;
                gDnldrRetryCntOnInputError++;
            }
            else
            {
                gDnldrRetryCntOnInputError = 0;
                waitInSeconds = RESCHEDULE_INTERVAL_RETRY_EXHAUSTED;          
            }
        }
        break;
        case EE_DOWNLOAD_NET_ERROR:
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: Failed! Net Error ret 0x%x", returnVal );
            if(MAX_RETRY_NET_ERROR > gDnldrRetryCntOnNetError)
            {
                waitInSeconds = RESCHEDULE_INTERVAL_NET_ERROR;
                gDnldrRetryCntOnNetError++;
            }
            else
            {
                gDnldrRetryCntOnNetError = 0;
                waitInSeconds = RESCHEDULE_INTERVAL_RETRY_EXHAUSTED;
            }
        }
        break;
        case EE_DOWNLOAD_WAIT_FILE_PART:
        {
            waitInSeconds = SGEE_FILE_PART_WAIT_TIMEOUT;
        }
        break;
        default:
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                "EE_DOWNLOAD:EE_Download_CheckForError: Failed!!!" );
            waitInSeconds = RESCHEDULE_INTERVAL_CORRUPT_FILE;
        }
        break;
    }

    EE_Download_CallBackFnPtr(EE_DOWNLOAD_STATUS_CALLBACK, NULL, 0, returnVal);


    /* make the downlaoder thread to exit */
    downloaderRunningGlobal = 0;

    return result;
}


/**
 * @fn static tSIRF_VOID EE_Download_Scheduler( tSIRF_VOID )
 * @brief EE_Download_Scheduler
 *
 * @description To schedule the downloading of EE file
 *
 * @return tSIRF_VOID
 *
 */
static tSIRF_VOID EE_Download_Scheduler( tSIRF_VOID )
{
    tSIRF_SOCKET tcp_sock;
    tSIRF_RESULT result       = SIRF_SUCCESS;
    tSIRF_SOCKET writeset[2]  = {0, 0};
    tSIRF_SOCKET exceptset[2] = {0, 0};
    tSIRF_UINT16 failover     = 0;
    tEE_DOWNLOAD_STATUS returnVal = EE_DOWNLOAD_SUCCESS;

    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MIN),
                        "EE_DOWNLOAD:EE_Download_Scheduler started; server_address=%s, serverPort=%d, pServerFile=%s\n", serverAddressGlobal[0], serverPortGlobal, serverFileGlobal );

    do
    {
        /* Check if Shutdown/Stop is requested */
        if( !downloaderRunningGlobal )
        {
            return ;
        }
        writeset[1] = writeset[0] = 0;

        /* create socket and initiate connection: */
        result = SIRF_PAL_NET_CreateSocket( &tcp_sock, SIRF_PAL_NET_SOCKET_TYPE_DEFAULT );
        if ( SIRF_SUCCESS != result )
        {
            /* do not return else try after some time */
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_2),
                                "EE_DOWNLOAD: EE_Download_Scheduler unable to create socket.\n" );
            returnVal = EE_DOWNLOAD_NET_ERROR;
            EE_Download_CheckForError(returnVal);
            continue;
        }
        /* DEBUGCHK( result == 0 ); */
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_1),
                            "EE_DOWNLOAD: EE_Download_Scheduler trying connecting...\n" );

        result = SIRF_PAL_NET_Connect( tcp_sock, serverAddressGlobal[failover], serverPortGlobal , SIRF_PAL_NET_SECURITY_NONE);

        if ( (SIRF_SUCCESS != result) &&
             (SIRF_PAL_NET_CONNECT_INPROGRESS != result) &&
             (SIRF_PAL_NET_CONNECT_WOULD_BLOCK != result) &&
             (SIRF_PAL_NET_CONNECT_ALREADY != result))
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_1),
                                "EE_DOWNLOAD: EE_Download_Scheduler Failed to connect\n" );
            /* close failed socket and release memory: */
            SIRF_PAL_NET_CloseSocket( tcp_sock );
            returnVal = EE_DOWNLOAD_NET_ERROR;
            if(failover > 0)
            {
               EE_Download_CheckForError(returnVal);
            }
            /* retry for next */
            failover = (failover + 1);
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_1),
                    "EE_DOWNLOAD: EE_Download_Scheduler Try for faliover socket %d.\n",failover );

            continue;
        }

        memset(writeset,(int)SIRF_PAL_NET_INVALID_SOCKET,sizeof(writeset));
        memset(exceptset,(int)SIRF_PAL_NET_INVALID_SOCKET,sizeof(exceptset));
        writeset[0]  = tcp_sock;
        exceptset[0] = tcp_sock;
        /* store socket handle in global variable*/
        downloadSocketGlobal = tcp_sock;

        result       = SIRF_PAL_NET_Select( NULL, writeset, exceptset, 60000 );

        /* if connection established, do read/write: */
        if ( (SIRF_SUCCESS == result) && writeset[0] )
        {

            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_1),
                                "EE_DOWNLOAD: EE_Download_Scheduler connection established\n" );
            returnVal = Direct_Download_Start( downloaderRecvBufferGlobal,
                                               (tSIRF_CHAR *)serverAddressGlobal[failover],
                                               serverPortGlobal,
                                               serverFileGlobal,
                                               serverAuthGlobal,
                                               serverFileFormatGlobal );

            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                        "EE_DOWNLOAD: EE_Download_Scheduler File Download done...\n");

            /* Check for Return val and Sleep accordingly */
            if( ( EE_DOWNLOAD_SUCCESS != returnVal ) &&
                ( EE_DOWNLOAD_SGEE_NONEWFILE != returnVal ) &&
                ( EE_DOWNLOAD_INCOMPLETEFILE_ERROR != returnVal ) &&
                ( EE_DOWNLOAD_CORRUPTFILE_ERROR != returnVal ) &&
                ( EE_DOWNLOAD_INPUT_ERROR != returnVal ))
            {
                returnVal = EE_DOWNLOAD_NET_ERROR;
                failover = (failover + 1) % 2;
            }
            (tSIRF_VOID)Direct_Download_Stop();
        }
        else
        {
            returnVal = EE_DOWNLOAD_NET_ERROR;
            failover = (failover + 1) % 2;
        }
        /* close socket and release memory: */
        SIRF_PAL_NET_CloseSocket( tcp_sock );
        EE_Download_CheckForError(returnVal);
    }
    while ( downloaderRunningGlobal );
    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER, CLM_DLOG_SEVERITY_LEVEL_MAX),
            "EE_DOWNLOAD: EE_Download_Scheduler Exit.\n");

}

/**
 * @brief Return the Storage File Size.
 * @return Return the size of the file computed.
   Note:This is a temporary function to handle the file size in case
   of UCOS OS until implemented in PAL for UCOS as the current 
   SIRF_PAL_STORAGE_Size() implementation for UCOS in PAL always return -1 */
static tSIRF_INT32 EE_Download_Get_File_Size(tSIRF_UINT32 storage_id)
{
   tSIRF_RESULT result    = SIRF_SUCCESS;
   tSIRF_INT32 fileLength = 0;

   /* Read MAX_SIZE_OF_RECV_BUFFER bytes block*/
   while ( SIRF_SUCCESS == result )
   {
      result = SIRF_PAL_STORAGE_Read(storage_id, fileLength, &downloaderRecvBufferGlobal[0] , MAX_SIZE_OF_RECV_BUFFER);
      if( SIRF_SUCCESS == result )
      {
         fileLength += MAX_SIZE_OF_RECV_BUFFER;
      }
      else
      {
         break;
      }
   }
   result = SIRF_SUCCESS;
   while ( SIRF_SUCCESS == result )
   {
      result = SIRF_PAL_STORAGE_Read(SIRF_PAL_STORAGE_LOCAL_EE_FILE, fileLength, &downloaderRecvBufferGlobal[0], 1);
      if(SIRF_SUCCESS == result)
      {
         fileLength ++;
      }
   }
   return fileLength;
}


/**
 * @fn static tEE_DOWNLOAD_STATUS EE_Download_Local( tSIRF_VOID )
 * @brief EE_Download_Local
 *
 * @description To download the Local SGEE file
 *
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code
 *         defined in tEE_DOWNLOAD_STATUS
 *
 */
static tEE_DOWNLOAD_STATUS EE_Download_Local( tSIRF_VOID )
{

    tSIRF_RESULT result      = SIRF_SUCCESS;
    tEE_DOWNLOAD_STATUS returnVal = EE_DOWNLOAD_SUCCESS;
    tSIRF_INT32 fileLength = 0, remainingFile = 0, offsetFile = 0;
    tSIRF_UINT32  size = 0;
#if SIRF_INSTANT_FIX_GSW
    tSIRF_MSG_SSB_EE_FILE_PART msg;
#endif
    do
    {
        /* Check if Shutdown/Stop is requested */
        if( !downloaderRunningGlobal )
        {
            return EE_DOWNLOAD_SUCCESS;
        }
        /* Initialize local variables */
        fileLength      = 0;
        size            = 0;
        remainingFile   = 0;
        offsetFile      = 0;

        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_2),
                            "EE_DOWNLOAD: EE_Download_Local started; local_file=NVM%d\n",SIRF_PAL_STORAGE_LOCAL_EE_FILE );

        result = SIRF_PAL_STORAGE_Open( SIRF_PAL_STORAGE_LOCAL_EE_FILE );

        if ( SIRF_SUCCESS != result )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: EE_Download_Local could not open storage NVM%d.\n",
                                SIRF_PAL_STORAGE_LOCAL_EE_FILE );
            returnVal = EE_DOWNLOAD_CORRUPTFILE_ERROR;
            EE_Download_CheckForError(returnVal);
            continue;
        }

        fileLength = EE_Download_Get_File_Size(SIRF_PAL_STORAGE_LOCAL_EE_FILE);

        /* Now We know actual length of file process it */
        if ( fileLength < 1024 )
        {
            /* Close storage */
            if( SIRF_SUCCESS != SIRF_PAL_STORAGE_Close( SIRF_PAL_STORAGE_LOCAL_EE_FILE ) )
            {
                EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                    "EE_DOWNLOAD: PAL_STORAGE_CLOSE Failure\n" );
            }
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: EE_Download_Local corrupted storage NVM%d.\n",
                                SIRF_PAL_STORAGE_LOCAL_EE_FILE );
            returnVal = EE_DOWNLOAD_CORRUPTFILE_ERROR;
            EE_Download_CheckForError(returnVal);
            continue;
        }
        result = SIRF_SUCCESS;
        remainingFile = fileLength;

        while ( SIRF_SUCCESS == result && remainingFile > 0 )
        {
            if( remainingFile > MAX_SIZE_OF_RECV_BUFFER )
            {
                size = MAX_SIZE_OF_RECV_BUFFER;
                remainingFile -= MAX_SIZE_OF_RECV_BUFFER;
            }
            else
            {
                size = remainingFile;
                remainingFile = 0;
            }
            result = SIRF_PAL_STORAGE_Read(SIRF_PAL_STORAGE_LOCAL_EE_FILE, offsetFile, &downloaderRecvBufferGlobal[0], size);
            offsetFile += size;

            if( SIRF_SUCCESS != result )
            {
                /* Force Converter and SGEE modules to reset states */
                size =0;
            }
    #if SIRF_INSTANT_FIX_GSW
            memcpy( &(msg.buff[0]), (tSIRF_UINT8 *)(&downloaderRecvBufferGlobal[0]), size);
            msg.buffSize = size;
            if( SIRF_SUCCESS == SiRFNav_Input(SIRF_MSG_SSB_EE_FILE_PART, &msg, sizeof(msg)))
            {
                returnVal = EE_DOWNLOAD_INPUT_ERROR; /* Initialize error */
                if(SIRF_SUCCESS == EE_Download_CheckForError(EE_DOWNLOAD_WAIT_FILE_PART))
                {
                    returnVal = gSIFReponse;/* response */
                }
            }
            else
            {
                returnVal = EE_DOWNLOAD_CORRUPTFILE_ERROR;
            }
    #else
            returnVal = EE_Download_CallBackFnPtr( EE_DOWNLOAD_DATA_CALLBACK, downloaderRecvBufferGlobal, size , 0);
    #endif
            if(EE_DOWNLOAD_SUCCESS != returnVal || SIRF_SUCCESS != result)
            {
                break;
            }
        }

        /* Close storage */
        if( SIRF_SUCCESS != SIRF_PAL_STORAGE_Close( SIRF_PAL_STORAGE_LOCAL_EE_FILE ) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: PAL_STORAGE_CLOSE Failure\n" );
        }

        EE_Download_CheckForError(returnVal);
    } while(downloaderRunningGlobal);
    EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER, CLM_DLOG_SEVERITY_LEVEL_MAX),
            "EE_DOWNLOAD: EE_Download_Local Exit.\n");


    return returnVal;
}


/******************************************************************************
 * FUNCTION: EE_Download_Thread
 *
 * DESCRIPTION: To create thread and process the EE Download.
 *
 * PARAMETERS:
 * Name                         Mode       Use
 * SIRF_PAL_OS_THREAD_PARAMS     in
 *
 * RETURNS: SIRF_THREAD_DECL
 *
 * HISTORY:
 *
 ******************************************************************************/
SIRF_PAL_OS_THREAD_RETURNTYPE EE_Download_Thread( SIRF_PAL_OS_THREAD_PARAMS )
{
    SIRF_PAL_OS_THREAD_UNUSED
    SIRF_PAL_OS_THREAD_START();

    if ( 0 == strcmp(serverAddressGlobal[0], "127.0.0.1") )
        EE_Download_Local();
    else
        EE_Download_Scheduler();

    EE_Download_CallBackFnPtr(EE_DOWNLOAD_STATUS_CALLBACK, NULL, 0, EE_DOWNLOAD_THREAD_EXITING);
    EE_Download_SelfCleanup();
    SIRF_PAL_OS_THREAD_RETURN()
}

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Init(tEE_DOWNLOAD_CONFIG *pEEDownloadConfig)
 * @brief EE_Download_Init
 *
 * @description To Init the EE Download.
 * For default settings: Pass any of (pServerAddress, serverPort, pServerFile,
 * pServerAuth ) as NULL/0 with valid OTA format Else pass all valid required
 * paramaters.
 *
 * @param[in] pEEDownloadConfig pointer to the tEE_DOWNLOAD_CONFIG
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code defined in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Init( tEE_DOWNLOAD_CONFIG *pEEDownloadConfig )
{

    tSIRF_UINT8 i = 0;

    if ( STATE_EE_DOWNLOAD_INITIALISED == stateEEDownldrEnum )
    {
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                            "EE_DOWNLOAD: EE_Download_Init failed, already started.\n");
        return EE_DOWNLOAD_ALREADY_INITIALISED;
    }
    else
    {
        if(NULL == pEEDownloadConfig)
        {
            return EE_DOWNLOAD_INVALID_PARAM;
        }

        if ( ( EE_DOWNLOAD_OTA_FORMAT_FF1 != pEEDownloadConfig->OTAFormat ) && ( EE_DOWNLOAD_OTA_FORMAT_FF2 != pEEDownloadConfig->OTAFormat ) )
        {
            return EE_DOWNLOAD_INVALID_PARAM;
        }
        /* store the server OTA file format*/
        serverFileFormatGlobal = pEEDownloadConfig->OTAFormat;

#ifdef USE_URID
{
    extern tSIRF_BOOL uridInfoValidFlagGlobal;
    extern tSIRF_UINT8 uridDeviceIdGlobal[];
    extern tSIRF_UINT8 uridOemSubIdGlobal[];

    /* store the URID Information */
    uridInfoValidFlagGlobal = pEEDownloadConfig->uridInfoValidFlag;

    /* store the oem sub id */
    strncpy(uridOemSubIdGlobal, pEEDownloadConfig->uridOemSubId, OEM_SUB_ID_SIZE);

    /* store the device id */
    strncpy(uridDeviceIdGlobal, pEEDownloadConfig->uridDeviceId, DEVICE_ID_SIZE);

    if ( URID_SUCCESS != CSR_URID_Set_OEM_Info( uridOemSubIdGlobal, OEM_SUB_ID_SIZE, uridDeviceIdGlobal, DEVICE_ID_SIZE ) )
    {
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: EE_Download_Init : CSR_URID_Set_OEM_Info() return failed!!!\n" );
    }       
}
#endif /* USE_URID */
        if ( ( NULL != (tSIRF_CHAR*)pEEDownloadConfig->pServerAddress ) &&
             ( (ADDRESS_STRING_SIZE-1) > strlen((tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[0]) ) &&
             ( 0 < strlen((tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[0]) ) )
        {
            if (0 == strcmp((tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[0],"127.0.0.1"))
            {
                /* store the server address*/
                strncpy(serverAddressGlobal[0], (tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[0],(ADDRESS_STRING_SIZE-1));
                stateEEDownldrEnum = STATE_EE_DOWNLOAD_INITIALISED;
                EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                    "EE_DOWNLOAD: EE_Download_Init done.\n");
                return EE_DOWNLOAD_SUCCESS;
            }
        }
        if ( ( NULL != pEEDownloadConfig->pServerAuth ) && ( NULL != (tSIRF_CHAR*)pEEDownloadConfig->pServerAddress ) &&
             ( NULL != pEEDownloadConfig->pServerFile ) && ( 0 != pEEDownloadConfig->serverPort ) )
        {
            if ((256 <= strlen(pEEDownloadConfig->pServerFile)) || (256 <= strlen(pEEDownloadConfig->pServerAuth)) ||
                    ((ADDRESS_STRING_SIZE-1) <= strlen((tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[0])) || ((ADDRESS_STRING_SIZE-1) <= strlen((tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[1]))||
                    (0 == strlen(pEEDownloadConfig->pServerFile)) || (0 == strlen(pEEDownloadConfig->pServerAuth)) ||
                    (0 == strlen((tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[0])) || (0 == strlen((tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[1])))
            {
                return EE_DOWNLOAD_INVALID_PARAM;
            }
            /* store the server address*/
            for (i = 0; i < 2; i++)
            {
                if (strlen((tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[i]) > 0)
                {
                    strncpy(serverAddressGlobal[i], (tSIRF_CHAR*)pEEDownloadConfig->pServerAddress[i],(ADDRESS_STRING_SIZE-1));
                }
            }
            /* store the port number*/
            serverPortGlobal = pEEDownloadConfig->serverPort;

            /* store the server file name*/
            strncpy(serverFileGlobal, pEEDownloadConfig->pServerFile, 255);

            /* store the server authorization code*/
            strncpy(serverAuthGlobal, pEEDownloadConfig->pServerAuth, 255);

        }
        else
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: EE_Download_Init server parameters are invalid, default FF%d server params assumed.\n",pEEDownloadConfig->OTAFormat);
            /* Load only server file as per OTA other values are deafult set */
            if (EE_DOWNLOAD_OTA_FORMAT_FF2 == pEEDownloadConfig->OTAFormat)
            {
                strncpy(serverFileGlobal, "/diff/packedDifference.f2p7enc.ee", 255);
            }
            else
            {
                strncpy(serverFileGlobal, "/diff/packedDifference.f1p7.ee", 255);
            }
        }
        stateEEDownldrEnum = STATE_EE_DOWNLOAD_INITIALISED;
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                            "EE_DOWNLOAD: EE_Download_Init done.\n");
    }
    return EE_DOWNLOAD_SUCCESS;
}

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Start(tSIRF_VOID *callBackFunction )
 * @brief EE_Download_Start
 *
 *
 * @description To start the EE Download.
 * &param[in] callBackFunction, Callback function provided by Application
 *            tEE_DOWNLOAD_STATUS (*EE_Download_CallBackFnPtr)(tEE_DOWNLOAD_CALLBACK_TYPE, tSIRF_UINT8 *pData, tSIRF_UINT32 dataLength, tEE_DOWNLOAD_STATUS);
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code defined
 *         in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Start( tSIRF_VOID * callBackFunction)
{

    tSIRF_RESULT result = SIRF_SUCCESS;
    tEE_DOWNLOAD_STATUS returnVal = EE_DOWNLOAD_SUCCESS;

    if ( (STATE_EE_DOWNLOAD_STARTED == stateEEDownldrEnum) || (STATE_EE_DOWNLOAD_INITIALISED != stateEEDownldrEnum))
    {
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                            "EE_DOWNLOAD: EE_Download_Start failed, already started.\n");
        EE_Download_CallBackFnPtr(EE_DOWNLOAD_STATUS_CALLBACK, NULL, 0, EE_DOWNLOAD_ALREADY_STARTED);
        return EE_DOWNLOAD_ALREADY_STARTED;
    }
    else
    {
        if(NULL != callBackFunction)
        {
            EE_Download_CallBackFnPtr = (tEE_DOWNLOAD_STATUS (*)(tEE_DOWNLOAD_CALLBACK_TYPE, tSIRF_UINT8 *, tSIRF_UINT32 , tEE_DOWNLOAD_STATUS))callBackFunction;
        }
        else
        {
            return EE_DOWNLOAD_GENERIC_FAILURE;
        }
        downloadSocketGlobal  = 0;
        downloaderRunningGlobal = 1;
        gDnldrRetryCntOnCorruptFile = 0;
        gDnldrRetryCntOnNetError = 0;
        gDnldrRetryCntOnInputError = 0;
        if ( SIRF_SUCCESS != SIRF_PAL_OS_SEMAPHORE_Create(&eeDownloaderSemaphore, 0 ) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                         "EE_DOWNLOAD: EE_Download_Start failed semaphore creation\n");
            returnVal = EE_DOWNLOAD_PAL_RESOURCE_ERROR;
        }
        else
        {

            result = SIRF_PAL_OS_THREAD_Create( SIRFINSTANTFIX_THREAD_SGEE, (tSIRF_HANDLE)EE_Download_Thread, &downloaderThreadHandle );
            if (SIRF_SUCCESS == result)
            {
                stateEEDownldrEnum = STATE_EE_DOWNLOAD_STARTED;
                EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: EE_Download_Start successful.\n");
            }
            else
            {
                EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                    "EE_DOWNLOAD: EE_Download_Start failed. thread creation\n");
                if ( SIRF_SUCCESS != SIRF_PAL_OS_SEMAPHORE_Delete( eeDownloaderSemaphore ) )
                {
                   EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                    "EE_DOWNLOAD: EE_Download_Start sem delete failed.\n" );
                }
                returnVal = EE_DOWNLOAD_PAL_RESOURCE_ERROR;
            }
        }
    }
    return returnVal;
}

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_Stop( tSIRF_VOID )
 * @brief EE_Download_Stop
 *
 * @description To stop the EE Download.
 * 
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code defined
 *         in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS EE_Download_Stop(  tSIRF_VOID )
{

    tEE_DOWNLOAD_STATUS returnVal = EE_DOWNLOAD_SUCCESS;
    if ( STATE_EE_DOWNLOAD_STARTED != stateEEDownldrEnum )
    {
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                            "EE_DOWNLOAD: EE_Download_Stop failed, EE_Download not started.\n");
        EE_Download_CallBackFnPtr(EE_DOWNLOAD_STATUS_CALLBACK, NULL, 0, EE_DOWNLOAD_NOT_RUNNNING);
        return EE_DOWNLOAD_GENERIC_FAILURE;
    }
    else
    {
        downloadSocketGlobal  = 0;
        downloaderRunningGlobal = 0;
        /* Release the semaphore to unblock the process thread, if its blocked */
        if ( SIRF_SUCCESS != SIRF_PAL_OS_SEMAPHORE_Release(eeDownloaderSemaphore) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: Stop, sem release failed.\n" );
            returnVal = EE_DOWNLOAD_PAL_RESOURCE_ERROR;
        }
        if ( SIRF_SUCCESS != SIRF_PAL_OS_THREAD_Delete( downloaderThreadHandle) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: EE_Download_Stop thread delete failed.\n" );
            returnVal = EE_DOWNLOAD_PAL_RESOURCE_ERROR;
        }
        if ( SIRF_SUCCESS != SIRF_PAL_OS_SEMAPHORE_Delete( eeDownloaderSemaphore ) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                "EE_DOWNLOAD: Stop, sem delete failed.\n" );
            returnVal = EE_DOWNLOAD_PAL_RESOURCE_ERROR;
        }
        stateEEDownldrEnum = STATE_EE_DOWNLOAD_STOPPED;
        EE_Download_CallBackFnPtr(EE_DOWNLOAD_STATUS_CALLBACK, NULL, 0, EE_DOWNLOAD_STOPPED);
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                            "EE_DOWNLOAD: EE_Download_Stop done.\n");
    }
    return returnVal;
}

/**
 * @fn tEE_DOWNLOAD_STATUS EE_Download_SelfCleanup( tSIRF_VOID )
 * @brief EE_Download_SelfCleanup
 *
 * @description To cleanup the EE Download resources.
 * 
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code defined
 *         in tEE_DOWNLOAD_STATUS
 *
 */
static tEE_DOWNLOAD_STATUS EE_Download_SelfCleanup(  tSIRF_VOID )
{

    tEE_DOWNLOAD_STATUS returnVal = EE_DOWNLOAD_SUCCESS;
    if ( STATE_EE_DOWNLOAD_STARTED != stateEEDownldrEnum )
    {
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                            "EE_DOWNLOAD: EE_Download_SelfCleanup failed, EE_Download not started.\n");
        return EE_DOWNLOAD_GENERIC_FAILURE;
    }
    else
    {
        downloadSocketGlobal  = 0;
        downloaderRunningGlobal = 0;
        /* Release the semaphore to unblock the process thread, if its blocked */
        if ( SIRF_SUCCESS != SIRF_PAL_OS_SEMAPHORE_Release(eeDownloaderSemaphore) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: EE_Download_SelfCleanup, sem release failed.\n" );
            returnVal = EE_DOWNLOAD_PAL_RESOURCE_ERROR;
        }

        if ( SIRF_SUCCESS != SIRF_PAL_OS_SEMAPHORE_Delete( eeDownloaderSemaphore ) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                "EE_DOWNLOAD: EE_Download_SelfCleanup, sem delete failed.\n" );
            returnVal = EE_DOWNLOAD_PAL_RESOURCE_ERROR;
        }
        stateEEDownldrEnum = STATE_EE_DOWNLOAD_STOPPED;
        if ( SIRF_SUCCESS != SIRF_PAL_OS_THREAD_Delete( downloaderThreadHandle) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: EE_Download_SelfCleanup thread delete failed.\n" );
            returnVal = EE_DOWNLOAD_PAL_RESOURCE_ERROR;
        }
        EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                            "EE_DOWNLOAD: EE_Download_SelfCleanup done.\n");
    }
    return returnVal;
}

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
)
{

    tSIRF_CHAR  * pVersion;
    tSIRF_CHAR  * pVer = EE_DOWNLOAD_VERSION_GENERAL;

    if (NULL == version)
    {
        return EE_DOWNLOAD_INVALID_PARAM;
    }

    pVersion = (tSIRF_CHAR  *)&version[0];
    if ( MAX_LEN_EE_DNLDR_VER_STRING > strlen(pVer) )
    {
        strncpy(pVersion, pVer,MAX_LEN_EE_DNLDR_VER_STRING-9);
    }
    strncat(pVersion, (tSIRF_CHAR *)VERSION_POSIX_METHOD,8);
    return EE_DOWNLOAD_SUCCESS;
}

static FILE *pFile = NULL; /* for logging */

#define SLL_MAX_LOG_FILE_PATH 256
static tSIRF_UINT32 eeDebugLevel = CLM_DLOG_SEVERITY_LEVEL_MIN;
static tSIRF_CHAR mEELogPath[SLL_MAX_LOG_FILE_PATH] = "/data/";

tSIRF_VOID EEDownloadDebugLogLevelSet(tSIRF_CHAR * logPath, tSIRF_UINT32 level)
{
    eeDebugLevel = level;
    if(logPath != NULL)
    {
        strncpy(mEELogPath, logPath, SLL_MAX_LOG_FILE_PATH);
    }
}


#if !defined (USE_CLM_DEBUG_LOG)
#include <stdarg.h>
tSIRF_INT32 EEDownloadDebug_Log(
    tSIRF_UINT32 moduleID,
    tSIRF_UINT32 level,
    tSIRF_VOID * pFormattedString,
    ...
)
{
    va_list args;
    tSIRF_CHAR buf[256] = {0};
    tSIRF_CHAR fileName[50] = {0};

    if(moduleID != CLM_DLOG_MOD_ID_EEDOWNLOADER || level < eeDebugLevel)
    {
        return 0;
    }
        
    va_start( args, pFormattedString );
#if defined(OS_WINCE)
    vsprintf( buf, (const tSIRF_CHAR *)pFormattedString, args );
#else
    vsnprintf( buf, (sizeof(buf) - 1), (const tSIRF_CHAR *)pFormattedString, args );
#endif
    /* print on console */
   if (pFile == NULL)
   {
#ifdef OS_ANDROID
      strcpy( fileName, mEELogPath );
#endif
      strcat( fileName, "ee_download_debug.txt" );
      pFile = fopen( fileName, "a" );
      if (pFile == NULL)
      {
         printf("No File Opened");
         return 0 ;
      }
   }
   fwrite( buf, 1, strlen(buf), (FILE *)pFile );
   fflush(pFile);
   return 1;
}
#endif

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
                        tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
{
    (tSIRF_VOID)message_length;
    if( SIRF_MSG_SSB_EE_FILE_PART_RESP  == message_id)
    {
        tSIRF_MSG_SSB_EE_FILE_PART_RESP *msg = (tSIRF_MSG_SSB_EE_FILE_PART_RESP*)message_structure;
        gSIFReponse = (tEE_DOWNLOAD_STATUS)(msg->response);
        if ( SIRF_SUCCESS != SIRF_PAL_OS_SEMAPHORE_Release(eeDownloaderSemaphore) )
        {
            EEDownloadDebug_Log(DEBUG_OPTION(CLM_DLOG_MOD_ID_EEDOWNLOADER,CLM_DLOG_SEVERITY_LEVEL_MAX),
                                "EE_DOWNLOAD: SiRF Nav response, sem release failed.\n" );
        }
    }
}

/**
 * @}
 * End of file.
 */

