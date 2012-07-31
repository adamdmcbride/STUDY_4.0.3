/**
 * @addtogroup platform_src_sirf_util_ext
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2005-2010 by SiRF Technology, a CSR plc Company.       *
 *    All rights reserved.                                                 *
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
 *
 * MODULE:  HOST
 *
 * FILENAME:  sirf_ext_log.c
 *
 * DESCRIPTION: Routines to log to local disk the I/O messages passed ro
 *              and from this program, for example when to connected to
 *              SiRFLoc Demo or its equivalent.
 *
 ***************************************************************************/

#ifdef SIRF_EXT_LOG

/***************************************************************************
 * Include Files
 ***************************************************************************/

#include <string.h>
#include <stdio.h>

#include "sirf_types.h"
#include "sirf_pal.h"
#include "sirf_ext_log.h"
#include "sirf_codec.h"
#include "sirf_proto_common.h"

#include "sirf_codec_ascii.h"
#include "sirf_pal_log.h"
#include "sirf_msg_ssb.h"

/***************************************************************************
 * Global variables
 ***************************************************************************/

tSIRF_EXT_LOG_TYPE log_type = SIRF_EXT_LOG_SIRF_ASCII_TEXT;

/* temp */
#define SIRFNAV_DEMO_VERSION_NAME "Eng"
#define SIRFNAV_DEMO_VERSION "0.1"

/* ----------------------------------------------------------------------------
 *    Local Variables
 * ------------------------------------------------------------------------- */

static tSIRF_LOG_HANDLE log_file = NULL;

/***************************************************************************
 * Description:
 * Parameters:
 * Returns:
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_LOG_Open( tSIRF_EXT_LOG_TYPE type, tSIRF_CHAR *filename )
{
   tSIRF_RESULT        result;
   tSIRF_DATE_TIME     date_time;
   tSIRF_MSG_SSB_COMM_MESSAGE_TEXT header_message;

   log_type = type;

   if ( log_file != NULL ) {
      return SIRF_EXT_LOG_ALREADY_OPEN;
   }

   result = SIRF_PAL_LOG_Open(filename, &log_file, SIRF_PAL_LOG_MODE_OVERWRITE);
   if ( SIRF_SUCCESS != result ) {
      return SIRF_PAL_LOG_OPEN_ERROR;
   }

   /* Write header (demo version number and time when opened): */
   result = SIRF_PAL_OS_TIME_DateTime( &date_time );

   if ( result == SIRF_SUCCESS )
   {
      sprintf( header_message.msg_text, "%s Version %s log file opened %02d/%02d/%02d %02d:%02d:%02d\n",
         SIRFNAV_DEMO_VERSION_NAME,
         SIRFNAV_DEMO_VERSION,
         date_time.month,
         date_time.day,
         date_time.year%100,
         date_time.hour,
         date_time.minute,
         date_time.second );

      SIRF_PAL_LOG_Write( log_file, (char *)&header_message, strlen(header_message.msg_text) );
   }

   return SIRF_SUCCESS;

} /* SIRF_EXT_LOG_Open */


/***************************************************************************
 * Description:
 * Parameters:
 * Returns:
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_LOG_Close( tSIRF_VOID )
{
   tSIRF_DATE_TIME date_time;
   tSIRF_MSG_SSB_COMM_MESSAGE_TEXT message_structure;

   if ( NULL == log_file )
   {
      return SIRF_EXT_LOG_ALREADY_CLOSED;
   }

   if ( SIRF_SUCCESS == SIRF_PAL_OS_TIME_DateTime( &date_time ) )
   {
      sprintf( message_structure.msg_text, "Log file closed %02d/%02d/%02d %02d:%02d:%02d",
         date_time.month,
         date_time.day,
         date_time.year%100,
         date_time.hour,
         date_time.minute,
         date_time.second );

      /* Ignore return value */
      (void)SIRF_PAL_LOG_Write( log_file, (tSIRF_CHAR *)&message_structure, strlen(message_structure.msg_text) );
   }
   (void)SIRF_PAL_LOG_Close(log_file);
   log_file = NULL;
   return SIRF_SUCCESS;

} /* SIRF_EXT_LOG_Close */


/***************************************************************************
 * Description:
 * Parameters:
 * Returns:
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_LOG_Send_Passthrough( tSIRF_VOID *PktBuffer, tSIRF_UINT32 PktLen )
{
   return SIRF_PAL_LOG_Write( log_file, (char *)PktBuffer, PktLen );

}  /* SIRF_EXT_LOG_Send_Passthrough */


/**
 * Send a messsage to the log file.  Encodes and adds the protocol wrapper
 * then calls SIRF_PAL_LOG_Write to store the data.
 *
 * @param message_id         Message id
 * @param message_structure  pointer to the message structure
 * @param message_length     length of data pointed to by message_structure
 *
 * @return SIRF_SUCCESS if successful or appropriate error code
 */
tSIRF_RESULT SIRF_EXT_LOG_Send( tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
{
   tSIRF_RESULT tRet = SIRF_SUCCESS;

   /* write log entry if log file is open: */
   if ( log_file != NULL )
   {
      tSIRF_UINT8  payload[SIRF_EXT_LOG_MAX_MESSAGE_LEN];
      tSIRF_UINT8  packet[SIRF_EXT_LOG_MAX_MESSAGE_LEN+10];
      tSIRF_UINT32 payload_length = sizeof(payload);
      tSIRF_UINT32 packet_length = sizeof(packet);

      if ( SIRF_EXT_LOG_SIRF_ASCII_TEXT == log_type )
      {
         tSIRF_UINT32 options = SIRF_CODEC_OPTIONS_GET_FIRST_MSG;
         tRet = SIRF_CODEC_ASCII_Encode( message_id, message_structure, message_length,
                                         (tSIRF_UINT8 *)packet, &packet_length,&options );
      }
      else /* SIRF_EXT_LOG_SIRF_BINARY_STREAM */
      {
         /* Call the generic encode function, but it does not support AI3 */
         tRet = SIRF_CODEC_Encode( message_id, message_structure, message_length,
                                   payload, &payload_length );
         if ( tRet != SIRF_SUCCESS )
         {
            return tRet;
         }

         /* Apply the protocol wrapper */
         tRet = SIRF_PROTO_Wrapper( payload, payload_length, packet, &packet_length );
      }

      if ( tRet != SIRF_SUCCESS )
      {
         return tRet;
      }

      /* Send the data */
      tRet = SIRF_PAL_LOG_Write( log_file, (char *)packet, packet_length );
   }
   return tRet;

} /* SIRF_EXT_LOG_Send */

#endif /* SIRF_EXT_LOG */

/**
 * @}
 */

