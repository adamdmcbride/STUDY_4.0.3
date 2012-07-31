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
 * FILENAME:  gprs_at_command_bearer.c
 *
 * DESCRIPTION: Gprs_At_Command bearer initialization
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/util/lpl/gprs_at_command_server/gprs_at_command_bearer.c $
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
#include "sirf_ext_uart.h"
/***************************************************************************
 * Local Data Declarations
 ***************************************************************************/
#define GPRS_RETRY_SLEEP (5000)
/* Maximum number of times trying to send a successful message to the modem before giving up*/
#define GPRS_AT_COMMAND_MAX_BEARER_RETRIES (3)

/** State machine states for opening the GPRS Bearer */
typedef enum
{
   GPRS_NETWORK_STATE_IDLE,
   GPRS_NETWORK_STATE_RESET,
   GPRS_NETWORK_STATE_PIFC,
   GPRS_NETWORK_STATE_ATE,
   GPRS_NETWORK_STATE_PCMEE,
   GPRS_NETWORK_STATE_TCPIP_STACK_OPENING,
   GPRS_NETWORK_STATE_TCPIP_STACK_STARTING,
   GPRS_NETWORK_STATE_BEARER_OPENING,
   GPRS_NETWORK_STATE_BEARER_CONFIGURING,
   GPRS_NETWORK_STATE_BEARER_STARTING,
} gprs_network_state_t;

#define PWOPEN_OK_RESPONSES_EXPECTED (2)

/**
 * Structure for internal AT Command server management 
 */
typedef struct
{
   /* Variables for establishing a GPRS connection */
   tSIRF_GPRS_HANDLE   handle; /**< Handle for internal responses etc */
   tSIRF_BOOL          flow_control;/**< When going through the state machine
                                     * send an +IFC message to set the modem
                                     * to these settings */
   tSIRF_UINT32        pwopen_oks_needed; /**< Number of OK respones needed to 
                                           * move to the next state */
   tSIRF_UINT32        pwopen_oks_received;/**< Number of OK respones needed to 
                                            * move to the next state */
   tSIRF_UINT32        bearer_retries;/**< Number of retries in this state */
   tSIRF_UINT32        bearer_state;  /**< Current state machine state */
   tSIRF_UINT32        bearer_result; /**< Result of going through the state 
                                       * machine */
   tSIRF_CHAR          apn[SIRF_MSG_GPRS_AT_COMMAND_MAX_STRING];/**< APN Name */
} gprs_bearer_t;

static gprs_bearer_t gprs_bearer;
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
 * Allocates, fills out and sends the ATE0 to turn echo off
 * 
 * @param handle Handle to the GPRS server.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_ate(tSIRF_GPRS_HANDLE handle)
{
   tSIRF_RESULT result;

   gprs_msg_t *msg;
   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_E));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_E *msg_structure = (tSIRF_MSG_GPRS_AT_COMMAND_E*)msg->message_structure;
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_E;
      /* Message length is set by MsgMalloc */
      msg_structure->echo = 0; /* Do not echo */
      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}

/** 
 * Allocates, fills out and sends the +IFC to set flow control
 * 
 * @param handle Handle to the GPRS server.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_pifc(tSIRF_GPRS_HANDLE handle,
                                   tSIRF_BOOL flow_control)
{
   tSIRF_RESULT result;

   gprs_msg_t *msg;
   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PIFC));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PIFC *msg_structure = (tSIRF_MSG_GPRS_AT_COMMAND_PIFC*)msg->message_structure;
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PIFC;
      /* Message length is set by MsgMalloc */
      if (flow_control)
      {
         msg_structure->dce_by_dte = SIRF_MSG_GPRS_AT_COMMAND_PIFC_DCE_BY_DTE_RTS;
         msg_structure->dte_by_dce = SIRF_MSG_GPRS_AT_COMMAND_PIFC_DTE_BY_DCE_CTS;
      }
      else
      {
         msg_structure->dce_by_dte = SIRF_MSG_GPRS_AT_COMMAND_PIFC_DCE_BY_DTE_NONE;
         msg_structure->dte_by_dce = SIRF_MSG_GPRS_AT_COMMAND_PIFC_DTE_BY_DCE_NONE;
      }
      
      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}
/** 
 * Allocates, fills out and sends the +PCMEE to get extended error information
 * 
 * @param handle Handle to the GPRS server.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_pcmee(tSIRF_GPRS_HANDLE handle)
{
   tSIRF_RESULT result;

   gprs_msg_t *msg;
   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PCMEE));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PCMEE *msg_structure = (tSIRF_MSG_GPRS_AT_COMMAND_PCMEE*)msg->message_structure;
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PCMEE;
      /* Message length is set by MsgMalloc */
      msg_structure->mode = SIRF_MSG_GPRS_AT_COMMAND_PCMEE_MODE_ON;
      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}

/** 
 * Allocates, fills out and sends the +WOPEN to start or stop the TCP/IP stack
 * 
 * @param handle Handle to the GPRS server.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_pwopen(tSIRF_GPRS_HANDLE handle,
                                     tSIRF_UINT32 state )
{
   tSIRF_RESULT result;

   gprs_msg_t *msg;
   /* Reset number of respones expected */
   gprs_bearer.pwopen_oks_needed = 0;
   gprs_bearer.pwopen_oks_received = 0;
   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PWOPEN));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PWOPEN *msg_structure = (tSIRF_MSG_GPRS_AT_COMMAND_PWOPEN*)msg->message_structure;
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PWOPEN;
      /* Message length is set by MsgMalloc */
      msg_structure->state = state;
      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);

      /* Flow control settings get reset after each change to wopen */
      if (SIRF_SUCCESS == result)
      {
         gprs_bearer.pwopen_oks_needed++;
         result = gprs_send_pifc(gprs_bearer.handle,
                                 gprs_bearer.flow_control);
         if (SIRF_SUCCESS == result)
         {
            gprs_bearer.pwopen_oks_needed++;
         }
      }
   }
   return result;
}

/** 
 * Allocates, fills out and sends the +WIPCFG to start the IP stack
 * 
 * @param handle Handle to the GPRS server.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_pwipcfg_start_tcpip(tSIRF_GPRS_HANDLE handle)
{
   tSIRF_RESULT result;

   gprs_msg_t *msg;
   /* Send the next message to open the tcpip stack */
   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PWIPCFG_START_TCPIP));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PWIPCFG_START_TCPIP;
      /* Message length is set by MsgMalloc */
      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}

/** 
 * Allocates, fills out and sends the +WIPBR message to open the bearer module
 * 
 * @param handle Handle to the GPRS server.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_bearer_open(tSIRF_GPRS_HANDLE handle)
{
   tSIRF_RESULT result;
   gprs_msg_t *msg;

   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_OPEN));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_OPEN *msg_structure = (tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_OPEN*)msg->message_structure;
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_OPEN;
      /* Message length is set by MsgMalloc */

      msg_structure->bid = SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_BID_GPRS;
      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}

/** 
 * Allocates, fills out and sends the +WIPBR message to set the APN
 * 
 * @param handle Handle to the GPRS server.
 * @param apn    APN string.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_bearer_set_apn(tSIRF_GPRS_HANDLE handle,
                                             tSIRF_CHAR const * const apn)
{
   tSIRF_RESULT result;
   gprs_msg_t *msg;
   tSIRF_UINT32 apn_length;

   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_SET));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_SET *msg_structure = (tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_SET*)msg->message_structure;
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_SET;
      /* Message length is set by MsgMalloc */

      msg_structure->bid     = SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_BID_GPRS;
      msg_structure->opt_num = WIP_BOPT_GPRS_APN;
      msg_structure->value[0] = '\"';
      apn_length = strlcpy(&msg_structure->value[1],
                           apn,
                           sizeof(msg_structure->value)-3);
      msg_structure->value[apn_length + 1] = '\"';
      msg_structure->value[apn_length + 2] = '\0';

      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}

/** 
 * Allocates, fills out and sends the +WIPBR message to start the bearer
 * 
 * @param handle Handle to the GPRS server.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_bearer_start(tSIRF_GPRS_HANDLE handle)
{
   tSIRF_RESULT result;
   gprs_msg_t *msg;

   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_START));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_START *msg_structure = (tSIRF_MSG_GPRS_AT_COMMAND_PWIPBR_START*)msg->message_structure;
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_START;
      /* Message length is set by MsgMalloc */

      msg_structure->bid  = SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_BID_GPRS;
      msg_structure->mode = SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_START_MODE_CLIENT;
      /* Null out the strings */
      msg_structure->login[0]              = '\0';
      msg_structure->password[0]           = '\0';
      msg_structure->caller_identity[0]    = '\0';

      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}

/*==============================================================================
 * Callback function 
 *============================================================================*/

/** 
 * Call back function to receive messages from the server.
 * 
 * Is a request response state machine sending in sequence the messags 
 * necessary to setup and connect to the GPRS bearer.
 * 
 * @param message_id        Id of the message received
 * @param message_structure Data of the message received
 * @param message_length    Size of the data received
 * 
 * @return Result of processing the message.  Usually ignored.
 */
static tSIRF_RESULT gprs_start_callback(tSIRF_UINT32 message_id, 
                                        tSIRF_VOID *message_structure, 
                                        tSIRF_UINT32 message_length)
{
   tSIRF_RESULT result = SIRF_SUCCESS;

   (void)message_length;
   if ((GPRS_NETWORK_STATE_IDLE != gprs_bearer.bearer_state) && 
       (SIRF_MSG_GPRS_AT_COMMAND_OK == message_id))
   {
      /* we expected this message and we can mark the particular state as 
       * completed */
      switch (gprs_bearer.bearer_state)
      {
      case GPRS_NETWORK_STATE_RESET:
         /* Two OK responses required while in this state.  One from the +WOPEN
          * and one from the +IFC.  When both are received we can continue */
         gprs_bearer.pwopen_oks_received++;
         if (--gprs_bearer.pwopen_oks_needed)
         {
            break;
         }
         if (PWOPEN_OK_RESPONSES_EXPECTED != gprs_bearer.pwopen_oks_received)
         {
            result = gprs_send_pwopen(gprs_bearer.handle, 
                                      SIRF_MSG_GPRS_AT_COMMAND_PWOPEN_STATE_CLOSE);            
            break;
         }
         result = gprs_send_pwopen(gprs_bearer.handle, 
                                   SIRF_MSG_GPRS_AT_COMMAND_PWOPEN_STATE_OPEN);
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_TCPIP_STACK_OPENING;
         break;
      case GPRS_NETWORK_STATE_TCPIP_STACK_OPENING:
         gprs_bearer.pwopen_oks_received++;
         if (--gprs_bearer.pwopen_oks_needed)
         {
            break;
         }
         if (PWOPEN_OK_RESPONSES_EXPECTED != gprs_bearer.pwopen_oks_received)
         {
            result = gprs_send_pwopen(gprs_bearer.handle, 
                                      SIRF_MSG_GPRS_AT_COMMAND_PWOPEN_STATE_OPEN);            
            break;
         }

         /* Immediately after +WOPEN send the ATE0 to turn the echo off.
          * While not absolutely necessary, it makes logging and dealing with
          * the protocol faster.  +WOPEN resets the echo state to echo on */
         result = gprs_send_ate(gprs_bearer.handle );
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_ATE;
         break;
      case GPRS_NETWORK_STATE_ATE:
         /* After the response from setting the Echo mode, we need to turn
          * ERROR codes on.  Without error code every error is just ERROR
          * and we cannot report useful information. */
         result = gprs_send_pcmee(gprs_bearer.handle );
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_PCMEE;
         break;
      case GPRS_NETWORK_STATE_PCMEE:
         result = gprs_send_pwipcfg_start_tcpip(gprs_bearer.handle );
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_TCPIP_STACK_STARTING;
         break;
      case GPRS_NETWORK_STATE_TCPIP_STACK_STARTING:
         result = gprs_send_bearer_open(gprs_bearer.handle);
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_BEARER_OPENING;
         break;
      case GPRS_NETWORK_STATE_BEARER_OPENING:
         result = gprs_send_bearer_set_apn(gprs_bearer.handle,
                                           gprs_bearer.apn);
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_BEARER_CONFIGURING;
         break;
      case GPRS_NETWORK_STATE_BEARER_CONFIGURING:
         result = gprs_send_bearer_start(gprs_bearer.handle);
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_BEARER_STARTING;
         break;
      case GPRS_NETWORK_STATE_BEARER_STARTING:
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_IDLE;
         result = SIRF_SUCCESS;
         break;
      default:
         /* unsolicited message, determine if this is a message we should
          * handle */
         break;
      };

      if (SIRF_SUCCESS != result)
      {
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_IDLE;               
      }
      else
      {
         gprs_bearer.bearer_retries = 0;
      }
   }
   /* If someone left the TCIPIP stack open last time then this error code
    * will be generated.  Since the stack is already open, continue to the next
    * command */
   else if ((GPRS_NETWORK_STATE_TCPIP_STACK_STARTING == gprs_bearer.bearer_state) 
            && (SIRF_MSG_GPRS_AT_COMMAND_PCME_ERROR == message_id) 
            && (PCME_ERROR_STACK_ALREADY_IN_USE == ((tSIRF_MSG_GPRS_AT_COMMAND_PCME_ERROR*)message_structure)->error))
   {
      result = gprs_send_bearer_open(gprs_bearer.handle);
      gprs_bearer.bearer_state = GPRS_NETWORK_STATE_BEARER_OPENING;
   }
   /* If someone left the BEARER open last time then this error code
    * will be generated.  Since it is already, continue to the next
    * command */
   else if ((GPRS_NETWORK_STATE_BEARER_OPENING == gprs_bearer.bearer_state) 
            && (SIRF_MSG_GPRS_AT_COMMAND_PCME_ERROR == message_id) 
            && (PCME_ERROR_DEVICE_ALREADY_OPEN == ((tSIRF_MSG_GPRS_AT_COMMAND_PCME_ERROR*)message_structure)->error))
   {
      result = gprs_send_bearer_set_apn(gprs_bearer.handle,
                                        gprs_bearer.apn);
      gprs_bearer.bearer_state = GPRS_NETWORK_STATE_BEARER_STARTING;
   }
   /* This is the bearer already started response. */
   else if ((GPRS_NETWORK_STATE_BEARER_STARTING == gprs_bearer.bearer_state)
            && (SIRF_MSG_GPRS_AT_COMMAND_PCME_ERROR == message_id) 
            && (PCME_ERROR_WIP_BAD_STATE == ((tSIRF_MSG_GPRS_AT_COMMAND_PCME_ERROR*)message_structure)->error))
   {
      gprs_bearer.bearer_state = GPRS_NETWORK_STATE_IDLE; 
      result = SIRF_SUCCESS;              
   }
   /* Handle errors, specifically timeouts here.  Some timeouts are somtimes
    * expected, the GPRS modem is very... very... flaky */
   else if (((SIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR == message_id) && ((SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_TIMEOUT == ((tSIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR*)message_structure)->result)) ||
             (GPRS_NETWORK_STATE_IDLE != gprs_bearer.bearer_state && ((SIRF_MSG_GPRS_AT_COMMAND_ERROR == message_id) || (SIRF_MSG_GPRS_AT_COMMAND_PCME_ERROR == message_id)))))
   {
      /* If we've exceeded the maximum number of retries, we're done */
      if (GPRS_AT_COMMAND_MAX_BEARER_RETRIES < ++gprs_bearer.bearer_retries)
      {
         gprs_bearer.bearer_state = GPRS_NETWORK_STATE_IDLE;
      }
      else
      {
         /* Sleep for a little bit.  Issuing commands to quickly often results 
          * in errors that don't recover */
         if ((SIRF_MSG_GPRS_AT_COMMAND_ERROR == message_id) || 
             (SIRF_MSG_GPRS_AT_COMMAND_PCME_ERROR == message_id))
         {
            SIRF_PAL_OS_THREAD_Sleep(GPRS_RETRY_SLEEP);
         }

         switch (gprs_bearer.bearer_state)
         {
         case GPRS_NETWORK_STATE_RESET:
            /* Wait until all OK's have been received or have timed out */
            if (--gprs_bearer.pwopen_oks_needed)
            {
               break;
            }
            result = gprs_send_pwopen(gprs_bearer.handle, SIRF_MSG_GPRS_AT_COMMAND_PWOPEN_STATE_CLOSE);
            break;
         case GPRS_NETWORK_STATE_PIFC:
            gprs_send_pifc(gprs_bearer.handle,
                           gprs_bearer.flow_control);
            break;
         case GPRS_NETWORK_STATE_TCPIP_STACK_OPENING:
            if (--gprs_bearer.pwopen_oks_needed)
            {
               break;
            }
            result = gprs_send_pwopen(gprs_bearer.handle, SIRF_MSG_GPRS_AT_COMMAND_PWOPEN_STATE_OPEN);
            break;
         case GPRS_NETWORK_STATE_ATE:
            result = gprs_send_ate(gprs_bearer.handle);
            break;
         case GPRS_NETWORK_STATE_PCMEE:
            result = gprs_send_pcmee(gprs_bearer.handle);
            break;
         case GPRS_NETWORK_STATE_TCPIP_STACK_STARTING:
            result = gprs_send_pwipcfg_start_tcpip(gprs_bearer.handle);
            break;
         case GPRS_NETWORK_STATE_BEARER_OPENING:
            result = gprs_send_bearer_open(gprs_bearer.handle);
            break;
         case GPRS_NETWORK_STATE_BEARER_CONFIGURING:
            result = gprs_send_bearer_set_apn(gprs_bearer.handle,
                                              gprs_bearer.apn);
            break;
         case GPRS_NETWORK_STATE_BEARER_STARTING:
            result = gprs_send_bearer_start(gprs_bearer.handle);
            break;
         
         default:
            /* Unhandled timeout, time to give up */
            gprs_bearer.bearer_state = GPRS_NETWORK_STATE_IDLE;
            result = SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_TIMEOUT;
            break;
         };
      }
   }
   else if (SIRF_MSG_GPRS_AT_COMMAND_ERROR == message_id)
   {
      /* There was an error with the last message but we don't know what it 
       * is */
      result = message_id;
   }
   else
   {
      /* Unsolicited messages, currently none we are interested in handling */
   }

   if (SIRF_SUCCESS != result)
   {
      gprs_bearer.bearer_state = GPRS_NETWORK_STATE_IDLE;               
   }

   gprs_bearer.bearer_result = result;
   return result;
}

/*==============================================================================
 * Public API's
 *============================================================================*/

/** This results in closing an active data connection or nothing if in command
 * mode
 * 
 * @return Result of the sending of +++.  @see SIRF_EXT_UART_Send
 */
tSIRF_RESULT gprs_send_close_data(tSIRF_UINT32 uartno)
{
   tSIRF_RESULT result;
   tSIRF_CHAR packet[] = "+++";
   tSIRF_UINT32 packet_length = sizeof(packet) - 1;
   result = SIRF_EXT_UART_Send(uartno,
                               packet,
                               packet_length);
   /* This sleep is necessary.  If anything else is sent then it will
    * not be registered as a <pause>+++<pause>.  To work, before calling this
    * function the prot must have been idle from all sending for 1 second as
    * well */
   SIRF_PAL_OS_THREAD_Sleep(1000);
   return result;
}

/** 
 * Send the +WIPDATA message to the modem to switch to data mode
 * 
 * @param index Index of the connection to open
 * 
 * @return result of trying to send the message.  SIRF_SUCCESS indicates success
 */
tSIRF_RESULT gprs_send_tcpip_wipdata(tSIRF_UINT32 index)
{
   tSIRF_RESULT result;

   gprs_msg_t *msg;
   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PWIPDATA));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PWIPDATA *wipdata = (tSIRF_MSG_GPRS_AT_COMMAND_PWIPDATA*) msg->message_structure;
      msg->handle = gprs_bearer.handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PWIPDATA;
      wipdata->protocol = SIRF_MSG_GPRS_AT_COMMAND_PWIPDATA_PROTOCOL_TCP_CLIENT;
      wipdata->idx = index;
      wipdata->mode = SIRF_MSG_GPRS_AT_COMMAND_PWIPDATA_MODE_CONTINOUS;
      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}


/** 
 * Closes and resets the GPRS Device
 * 
 * @param uartno Which UART the GPRS modem is connected to
 * 
 * @return SIRF_SUCCESS or first non-zero error code.
 */
tSIRF_RESULT gprs_stop(tSIRF_UINT32 uartno)
{
   tSIRF_RESULT result;
   tSIRF_RESULT ret_val = SIRF_SUCCESS;
   tSIRF_UINT8 packet[SIRF_PROTO_GPRS_AT_COMMAND_MAX_MSG_LENGTH];
   tSIRF_UINT32 packet_length = sizeof(packet);
   tSIRF_UINT32 options;
   tSIRF_MSG_GPRS_AT_COMMAND_PWOPEN pwopen;
   /* reset it, and send it immediately */
   pwopen.state = SIRF_MSG_GPRS_AT_COMMAND_PWOPEN_STATE_CLOSE;
   result = SIRF_PROTO_GPRS_AT_COMMAND_Encode(SIRF_MSG_GPRS_AT_COMMAND_PWOPEN,
                                              &pwopen,
                                              sizeof(pwopen),
                                              packet,
                                              &packet_length,
                                              &options);
   if (SIRF_SUCCESS == result)
   {
      result = SIRF_EXT_UART_Send(uartno,
                                  packet,
                                  packet_length);
   }

   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }
   /* Wait Don't wait around for a response */
   
   result = SIRF_GPRS_AT_COMMAND_SERVER_Close(gprs_bearer.handle);
   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }
   return ret_val;
}

/** 
 *  Kicks off the opening of the GPRS bearer
 * 
 * @param uartno The uart on which the GPRS Modem is connected
 * @param apn    The APN name of the bearer
 * 
 * @return SIRF_SUCCESS if the bearer is successfully opened
 */
tSIRF_RESULT gprs_start(tSIRF_UINT32 uartno,
                        tSIRF_BOOL   flow_control,
                        tSIRF_CHAR const * const apn)
{
   tSIRF_RESULT result;

   /* Open up a handle to receive the callbacks */
   result = SIRF_GPRS_AT_COMMAND_SERVER_Open(&gprs_bearer.handle,
                                              gprs_start_callback);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   /* Save for later use */
   gprs_bearer.flow_control = flow_control;

   strlcpy(gprs_bearer.apn,apn,sizeof(gprs_bearer.apn));

   /* Just in case the modem has a data connection, it won't see the +wopen */
   result = gprs_send_close_data(uartno);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   /* +WOPEN */
   gprs_bearer.bearer_state = GPRS_NETWORK_STATE_RESET;
   gprs_bearer.bearer_retries = 0;
   result = gprs_send_pwopen(gprs_bearer.handle, SIRF_MSG_GPRS_AT_COMMAND_PWOPEN_STATE_CLOSE);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   /* Now that +WOPEN has been sent, wait for the bearer code to go back idle
    * before returning.  If there is an error it will eventually timeout and
    * set the bearer_state back to idle */
   while (gprs_bearer.bearer_state != GPRS_NETWORK_STATE_IDLE)
   {
      SIRF_PAL_OS_THREAD_Sleep(10);
   }

   return gprs_bearer.bearer_result;
}

/**
 * @}
 * End of file.
 */

