/**
 * @addtogroup gprs_at_command_server
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *  Copyright (c) 2005-2009 by SiRF Technology, Inc.  All rights reserved. *
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
 * FILENAME:  gprs_at_command_server.h
 *
 * DESCRIPTION: Auxiliary serial port module
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/util/lpl/gprs_at_command_server/gprs_at_command_server.h $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************/

#ifndef __SIRF_GPRS_AT_COMMAND_SERVER_H__
#define __SIRF_GPRS_AT_COMMAND_SERVER_H__

/***************************************************************************
   Include files
***************************************************************************/

#include "sirf_types.h"
#include "sirf_ext.h"
#include "sirf_proto.h"
#include "sirf_msg_ssb.h"

/***************************************************************************
  Defines
***************************************************************************/
#define SIRF_GPRS_AT_COMMAND_SERVER_ERROR_START (0xC000)

/* Return values */
typedef enum {
   SIRF_GPRS_AT_COMMAND_SERVER_SUCCESS          = SIRF_SUCCESS,
   SIRF_GPRS_AT_COMMAND_SERVER_ALREADY_CREATED  = SIRF_GPRS_AT_COMMAND_SERVER_ERROR_START,
   SIRF_GPRS_AT_COMMAND_SERVER_NOT_CREATED,
   SIRF_GPRS_AT_COMMAND_SERVER_INVALID_HANDLE,
   SIRF_GPRS_AT_COMMAND_SERVER_UNKNOWN_PROTOCOL,
   SIRF_GPRS_AT_COMMAND_SERVER_NO_MORE_HANDLES_AVAILABLE,
   SIRF_GPRS_AT_COMMAND_SERVER_SEND_ERROR,
   SIRF_GPRS_AT_COMMAND_SERVER_MEMORY_ERROR,
   SIRF_GPRS_AT_COMMAND_SERVER_COMMAND_NOT_SUPPORTED,
   SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED,
   SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_TIMEOUT,
   SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_FORWARD,
   SIRF_GPRS_AT_COMMAND_INVALID_APN,
} tSIRF_GPRS_AT_COMMAND_SERVER_RESULT;

/* Handle to an open AT Command server connection */
typedef tSIRF_VOID* tSIRF_GPRS_HANDLE;

/** Decoded message that are placed on the queue.  */
typedef struct
{
   tSIRF_GPRS_HANDLE    handle;           /**< Handle of the sender. Not used
                                             for gprs messages */
   tSIRF_UINT32         message_id;       /**< Id of the message */
   tSIRF_VOID*          message_structure;/**< structure pointer */
   tSIRF_UINT32         message_length;   /**< Length of the message */
} gprs_msg_t;

typedef enum
{
   GPRS_SERVER_STATE_AT_COMMAND,
   GPRS_SERVER_STATE_AT_RESPONSE,
   GPRS_SERVER_STATE_TCP_IP_DATA,
} gprs_server_state_t;

/***************************************************************************
   Prototype Definitions
***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/* Global initialization and uninitialization.  Called once per application */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Create( 
   tSIRF_CHAR const * const port_name,
   tSIRF_CHAR const * const apn );
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Delete( tSIRF_VOID );

/* Request creation of an avaiable gprs_at_command handle and open it*/
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Open(
   tSIRF_GPRS_HANDLE         * const handle,
   tSIRF_EXT_MSG_CALLBACK            callback);

tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Close( 
   tSIRF_GPRS_HANDLE handle );

/* Send a payload over the gprs_at_command, first wrap it in the protocol */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Send(
   tSIRF_GPRS_HANDLE                  handle,
   tSIRF_UINT32                       message_id,
   tSIRF_VOID           const * const message_structure,
   tSIRF_UINT32                       message_length );

/* These two function work as a pair.  First call MsgMalloc, fill out the 
 * pertinant information, and then call MsgSend */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
   gprs_msg_t                 **      msg,
   tSIRF_UINT32                       mesage_length);

tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(
   gprs_msg_t           const * const msg);

#ifdef __cplusplus
}
#endif


#endif /* __SIRF_GPRS_AT_COMMAND_SERVER_H__ */

/**
 * @}
 */

