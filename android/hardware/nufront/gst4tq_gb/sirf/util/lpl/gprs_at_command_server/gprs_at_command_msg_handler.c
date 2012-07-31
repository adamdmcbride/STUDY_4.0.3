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
 * FILENAME:  sirf_gprs_at_command.c
 *
 * DESCRIPTION: Gprs_At_Commandiliary serial port module
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/util/lpl/gprs_at_command_server/gprs_at_command_msg_handler.c $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************/

#include <string.h>

#include "sirf_types.h"

#include "sirf_msg.h"
#include "sirf_msg_gprs_at_command.h"

#include "sirf_pal.h"

#include "gprs_at_command_server.h"

#include "sirf_codec.h"

#include "sirf_proto.h"
#include "sirf_proto_gprs_at_command.h"
#include "string_sif.h"
/***************************************************************************
 * Local Data Declarations
 ***************************************************************************/

/**
 * Structure for internal AT Command server management 
 */
/*----------------------------------------------------------------------------*/
/* Global Data */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Static function prototypes */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Function implemenation */
/*----------------------------------------------------------------------------*/
/** 
 * Set the state based on expecting an OK messages only 
 * 
 * @param msg             Contents of the response message
 * @param response_number How many responses have been received so far
 * @param state           What the state should change to
 * 
 * @return SIRF_SUCCESS or SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED
 */
tSIRF_RESULT handle_response_single_SIRF_MSG_GPRS_AT_COMMAND_OK_expected(
   gprs_msg_t           const * const msg,
   tSIRF_UINT32                       response_number,
   gprs_server_state_t        * const state)
{
   /* Only one message is expected, thus we can always process the next input
      message after this */
   *state = GPRS_SERVER_STATE_AT_COMMAND;

   /* Expect that a single OK message has come in since the command was sent */
   if (SIRF_MSG_GPRS_AT_COMMAND_OK              == msg->message_id && 
       1                                        == response_number)
   {
      return SIRF_SUCCESS;
   }
   else
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED;
   }
}

/** 
 * Set the state based on receiving responses to sending the +WIPDATA
 * 
 * @param msg             Contents of the response message
 * @param response_number How many responses have been received so far
 * @param state           What the state should change to
 * 
 * @return SIRF_SUCCESS or SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED
 */
tSIRF_RESULT handle_response_SIRF_MSG_GPRS_AT_COMMAND_PWIPDATA(
   gprs_msg_t           const * const msg,
   tSIRF_UINT32                       response_number,
   gprs_server_state_t        * const state)
{
   /* Only one message is expected, thus we can always process the next input
      message after this */

   /* Expect that a single OK message has come in since the command was sent */
   if (SIRF_MSG_GPRS_AT_COMMAND_CONNECT         == msg->message_id && 
       1                                        == response_number)
   {
      *state = GPRS_SERVER_STATE_TCP_IP_DATA;
      return SIRF_SUCCESS;
   }
   else
   {
      *state = GPRS_SERVER_STATE_AT_COMMAND;
      return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED;
   }
}

/** 
 * Set the state based on receiving responses to sending the +CGMI
 * 
 * @param msg             Contents of the response message
 * @param response_number How many responses have been received so far
 * @param state           What the state should change to
 * 
 * @return SIRF_SUCCESS or SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED
 */
tSIRF_RESULT handle_response_SIRF_MSG_GPRS_AT_COMMAND_PCGMI(
   gprs_msg_t           const * const msg,
   tSIRF_UINT32                       response_number,
   gprs_server_state_t        * const state)
{
   /* Forward all string responses until we get an OK message */
   if (SIRF_MSG_GPRS_AT_COMMAND_STRING         == msg->message_id && 
       SIRF_MSG_GPRS_AT_COMMAND_PCGMI_RESPONSES > response_number)
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_FORWARD;
   }

   if (SIRF_MSG_GPRS_AT_COMMAND_OK              == msg->message_id && 
       SIRF_MSG_GPRS_AT_COMMAND_PCGMI_RESPONSES == response_number)
   {
      *state = GPRS_SERVER_STATE_AT_COMMAND;
      return SIRF_SUCCESS;
   }

   return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED;
}

/** 
 * Set the state based on receiving responses to sending the +CCED
 * 
 * @param msg             Contents of the response message
 * @param response_number How many responses have been received so far
 * @param request_type    The type of +CCED request that was made
 * @param state           What the state should change to
 * 
 * @return SIRF_SUCCESS or SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED
 */
tSIRF_RESULT handle_response_SIRF_MSG_GPRS_AT_COMMAND_PCCED(
   gprs_msg_t           const * const msg,
   tSIRF_UINT32                       response_number,
   tSIRF_UINT32                       request_type,
   gprs_server_state_t        * const state)
{

   /* Start with 1 for the OK */
   tSIRF_BOOL csq_rsp_exp  = SIRF_FALSE;
   tSIRF_BOOL cced_rsp_exp = SIRF_FALSE;

   /* Three possible types of responses expected for a single shot:
    * - +CSQ
    * - +CCED
    * - +CSQ and +CCED
    */
   if ((SIRF_MSG_GPRS_AT_COMMAND_PCCED_DUMP_RSSI & request_type) ||
       (SIRF_MSG_GPRS_AT_COMMAND_PCCED_DUMP_ALL == request_type))
   {
      csq_rsp_exp = SIRF_TRUE;
   }

   if((~SIRF_MSG_GPRS_AT_COMMAND_PCCED_DUMP_RSSI) & request_type)
   {
      cced_rsp_exp = SIRF_TRUE;
   }

   /* +CSQ */
   if(SIRF_MSG_GPRS_AT_COMMAND_PCSQ == msg->message_id)
   {
      /* Check to see if this message was expected */
      if ((1 == response_number) && csq_rsp_exp)
      {
         return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_FORWARD;
      }
      else
      {
         *state = GPRS_SERVER_STATE_AT_COMMAND;
         return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED;
      }
   }
   
   /* +CCED */
   if (SIRF_MSG_GPRS_AT_COMMAND_PCCED_RESPONSE == msg->message_id)
   {
      /* This message is supposed to be message 1 or 2 depending on if a +CSQ
       * message is expected */
      if (!cced_rsp_exp ||
         ( 3 >= response_number) ||
         ((2 == response_number) && !csq_rsp_exp) ||
         ((1 == response_number) &&  csq_rsp_exp))
      {
         *state = GPRS_SERVER_STATE_AT_COMMAND;
         return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED;
      }
      else
      {
         return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_FORWARD;
      }
   }

   /* OK message */
   if (SIRF_MSG_GPRS_AT_COMMAND_OK == msg->message_id)
   {
      *state = GPRS_SERVER_STATE_AT_COMMAND;
      if ((1 == response_number && !csq_rsp_exp && !cced_rsp_exp) ||
          (2 == response_number && (csq_rsp_exp ^   cced_rsp_exp)) ||
          (3 == response_number &&  csq_rsp_exp &&  cced_rsp_exp))
      {
         return SIRF_SUCCESS;
      }
      else
      {
         return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED;
      }
   }
   
   return SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_UNEXPECTED;
}

/**
 * @}
 * End of file.
 */

