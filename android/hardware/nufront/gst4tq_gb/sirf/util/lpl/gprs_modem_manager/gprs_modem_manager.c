/**
 * @addtogroup app_util_gprs_modem_manager
 * @{
 */
 
 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *     Copyright (c) 2005-2010 by SiRF Technology, a CSR plc Company.      *
 *     All rights reserved.                                                *
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
 * FILENAME:  gprs_modem_manager.c
 *
 * DESCRIPTION: Module that initializes the GPRS Modem for use in the LPL
 *              and handles control plain aiding and SMS NI sessions starting
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/util/lpl/gprs_modem_manager/gprs_modem_manager.c $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************/

#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include "sirf_types.h"
#include "sirf_pal.h"

#include "sirf_msg.h"
#include "sirf_msg_gprs_at_command.h"

#include "sirf_proto.h"
#include "sirf_proto_mas.h"
#include "sirf_proto_gprs_at_command.h"

#include "sirf_codec.h"
#include "sirf_codec_csv.h"

#include "sirf_ext.h"
#include "gprs_modem_manager.h"
#include "gprs_at_command_server.h"

#include "string_sif.h"

#if 0 /* @todo when util_if.h or equivilent functionality is added to the
       * SiRFRunTimeLib the else clause can be removed. */
#include "util_if.h"
#else
#ifndef UTIL_Assert
   #define UTIL_Assert(_condition)
#endif

#ifndef UTIL_AssertAlways
   #define UTIL_AssertAlways()
#endif
#endif

#include "LSM_APIs.h"
#include "LSM_Types.h"

/***************************************************************************
 * Local Data Declarations
 ***************************************************************************/
#define BCDS_PER_BYTE (2)

/** Global data structure for the modem containing information retrieved from
 * the modem and other state information. */
typedef struct
{
   tSIRF_GPRS_HANDLE handle;
   tSIRF_UINT32 pcced_requested_dump;
   
   LSM_netCellID cell_id;
   tSIRF_BOOL    waiting_for_imsi;
   SETID_Info    imsi;
} gprs_modem_manager_t;
/*----------------------------------------------------------------------------*/
/* Global Data */
/*----------------------------------------------------------------------------*/
gprs_modem_manager_t gprs_modem_manager;
/*----------------------------------------------------------------------------*/
/* Static function prototypes */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Function implemenation */
/*----------------------------------------------------------------------------*/
/** 
 * Send the +CCED message to get the cell ID information.  The request is to
 * have the CID sent continously so that it can be updated once per second
 * while in dynamic situations
 * 
 * @param handle         handle to the GPRS Modem
 * @param requested_dump What information to request. This is stored so that
 *                       the response can be properly parsed.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT gprs_send_pcced(tSIRF_GPRS_HANDLE handle,
                                    tSIRF_UINT32 requested_dump)
{
   tSIRF_RESULT result;

   gprs_msg_t *msg;
   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PCCED));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PCCED *msg_structure = (tSIRF_MSG_GPRS_AT_COMMAND_PCCED*)msg->message_structure;
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PCCED;
      /* Message length is set by MsgMalloc */

      msg_structure->mode            = SIRF_MSG_GPRS_AT_COMMAND_PCCED_MODE_AUTO;
      msg_structure->requested_dump  = requested_dump;
      msg_structure->csq_step[0]     = '\0';

      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}

/** 
 * Send the +CIMI to the the SET Idendification
 * 
 * @param handle 
 * 
 * @return 
 */
static tSIRF_RESULT gprs_send_pcimi(tSIRF_GPRS_HANDLE handle)
{
   tSIRF_RESULT result;

   gprs_msg_t *msg;
   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
      &msg,
      sizeof(tSIRF_MSG_GPRS_AT_COMMAND_PCIMI));
   if (SIRF_SUCCESS == result && NULL != msg)
   {
      msg->handle = handle;
      msg->message_id = SIRF_MSG_GPRS_AT_COMMAND_PCIMI;
      /* Message length is set by MsgMalloc */

      result = SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);
   }
   return result;
}

/** 
 * Helper function to set the LPL format data from the Message formatted data
 *
 * This function also calls CP_SendCellInfo to periodically update the LPL.
 * 
 * @param main_cell Message format data
 * 
 * @return  SIRF_SUCCESS or non-zero error code.
 */
static tSIRF_RESULT set_lsm_cell_id (tSIRF_MSG_GPRS_AT_COMMAND_PCCED_RESPONSE_MAIN_CELL* main_cell)
{
   LSM_BOOL success;

   gprs_modem_manager.cell_id.eNetworkType = LSM_GSM;
   gprs_modem_manager.cell_id.m.gsm_cellid.mcc = (tSIRF_UINT16)main_cell->mcc;
   gprs_modem_manager.cell_id.m.gsm_cellid.mnc = (tSIRF_UINT16)main_cell->mnc;
   gprs_modem_manager.cell_id.m.gsm_cellid.lac = (tSIRF_UINT16)main_cell->lac;
   gprs_modem_manager.cell_id.m.gsm_cellid.cid = (tSIRF_UINT16)main_cell->ci;

   success = CP_SendCellInfo(&gprs_modem_manager.cell_id,SIRF_TRUE);
   
   if (success)
   {
      return SIRF_SUCCESS;
   }
   else
   {
      return SIRF_FAILURE;
   }
}

/** 
 * The modem manager is looking for IMSI message at certain times which are
 * returned a string message since they contain no identifying information.
 * 
 * @param message_id         message id
 * @param message_structure  STRING message
 * @param message_length     size of the STRING message
 * 
 * @return SIRF_SUCCESS if the message was successfully processed or non-zero
 * error code otherwise.
 *
 */
static tSIRF_RESULT handle_string_response(tSIRF_UINT32 message_id, 
                                           tSIRF_VOID *message_structure, 
                                           tSIRF_UINT32 message_length)
{
   (void)message_id;
   if (gprs_modem_manager.waiting_for_imsi)
   {
      tSIRF_UINT32 ii, kk;
      tSIRF_CHAR *imsi_string = (tSIRF_CHAR*)message_structure;
      tSIRF_CHAR digit[2];
      tSIRF_UINT8 value;

      gprs_modem_manager.waiting_for_imsi = SIRF_FALSE;
      gprs_modem_manager.imsi.SETidType = LSM_IMSI;
      /* The string for the IMSI needs to be stored in BCD format:
      -- msisdn, mdn and imsi are a BCD (Binary Coded Decimal) string
      -- represent digits from 0 through 9,
      -- two digits per octet, each digit encoded 0000 to 1001 (0 to 9)
      -- bits 8765 of octet n encoding digit 2n
      -- bits 4321 of octet n encoding digit 2(n-1) +1
      -- not used digits in the string shall be filled with 1111
      */
      memset(gprs_modem_manager.imsi.SETidValue,0,sizeof(gprs_modem_manager.imsi.SETidValue));
      for (ii = 0; ii < SET_ID_LENGTH; ii++)
      {
         for (kk = 0; kk < BCDS_PER_BYTE; kk++)
         {
            /* unused digits are encoded with 1111 or 0xF */
            if ((ii*BCDS_PER_BYTE + kk) >= (message_length - 1)) 
            {
               value = 0xF;
            }
            else
            {
               digit[0] = imsi_string[ii*BCDS_PER_BYTE + kk];
               digit[1] = '\0';
               value = atoi(digit);
            }
            /* Apparently the LPL expects the digits reversed in each byte so use 1-kk
            * instead of kk */
            gprs_modem_manager.imsi.SETidValue[ii] |= value << ((1-kk) * 4);
         }
      }
   }
   else
   {
      /* We could print the string, but most likely just not interested */
   }
   return SIRF_SUCCESS;
}

/** 
 * Handle the +CCED response message.  This takes special parsing and requires
 * special knowledge of the request made which is stored in the 
 * gprs_modem_manager
 * 
 * @param message_id        Should be SIRF_MSG_GPRS_AT_COMMAND_PCCED
 * @param message_structure pointer to the message structure.
 * @param message_length    Size of the message
 * 
 * @return 
 */
static tSIRF_RESULT handle_pcced_response(tSIRF_UINT32 message_id, 
                                          tSIRF_VOID *message_structure, 
                                          tSIRF_UINT32 message_length)
{
   (void)message_id;
   if (gprs_modem_manager.pcced_requested_dump & SIRF_MSG_GPRS_AT_COMMAND_PCCED_DUMP_MAIN)
   {
      tSIRF_UINT32 bytes_read = 0;
      tSIRF_UINT32 total_bytes = 0;
      tSIRF_UINT8* msg = message_structure;
      tSIRF_MSG_GPRS_AT_COMMAND_PCCED_RESPONSE_MAIN_CELL main_cell;

      memset(&main_cell,0,sizeof(main_cell));
      bytes_read = CSV_ImportUINT32(&main_cell.mcc,&msg);
      total_bytes += bytes_read;
      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.mnc,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32Hex(&main_cell.lac,&msg); /* Hex format */
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32Hex(&main_cell.ci,&msg);  /* Hex format */
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.bsic,&msg);
      total_bytes += bytes_read;
      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.bcch_freq,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.rx_lev,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.rx_lev_full,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.rx_lev_sub,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.rx_qual,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.rx_qual_full,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.rx_qual_sub,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }
      bytes_read = CSV_ImportUINT32(&main_cell.idle_ts ,&msg);
      total_bytes += bytes_read;

      if (total_bytes > message_length)
      {
         return SIRF_CODEC_ERROR_INVALID_MSG_LENGTH;
      } 
      else if (0 == bytes_read)
      {
         total_bytes += AdvancePastNextComma(&msg,message_length-bytes_read);
      }

      set_lsm_cell_id(&main_cell);
   }

   return SIRF_SUCCESS;
}

/** 
 * Call back function to receive messages from the server
 * 
 * @param message_id        Id of the message received
 * @param message_structure Data of the message received
 * @param message_length    Size of the data received
 * 
 * @return Result of processing the message.  Usually ignored.
 */
static tSIRF_RESULT gprs_modem_manager_callback(tSIRF_UINT32 message_id,
                                                tSIRF_VOID *message_structure, 
                                                tSIRF_UINT32 message_length)
{
   tSIRF_RESULT result;
   switch (message_id)
   {
   case SIRF_MSG_GPRS_AT_COMMAND_PCCED_RESPONSE:
      result = handle_pcced_response(message_id,
                                     message_structure,
                                     message_length);
      break;
   case SIRF_MSG_GPRS_AT_COMMAND_STRING:
      result = handle_string_response(message_id,
                                      message_structure,
                                      message_length);
      break;
   default:
      result = SIRF_FAILURE;
      break;
   }

   return result;
}

/** 
 * Get the latest stored Cell ID
 * 
 * @param cell_id Cell ID information
 * 
 * @return SIRF_SUCCESS if the cell ID is valid, LSM_INVALID_NT_TYPE otherwise
 */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_GetCellId(LSM_netCellID * cell_id)
{
   *cell_id = gprs_modem_manager.cell_id;
   if (LSM_INVALID_NT_TYPE == gprs_modem_manager.cell_id.eNetworkType)
   {
      return (tSIRF_RESULT)LSM_INVALID_NT_TYPE;
   }
   else
   {
      return SIRF_SUCCESS;
   }
}

/** 
 * Get the stored SET ID
 * 
 * @param set_id SET ID information
 * 
 * @return SIRF_SUCCESS if the cell ID is valid, 
 *         LSM_INVALID_SETID_TYPE otherwise
 */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_GetSETId(SETID_Info * set_id)
{
   if (LSM_INVALID_SETID_TYPE == gprs_modem_manager.imsi.SETidType)
   {
      return (tSIRF_RESULT) LSM_INVALID_SETID_TYPE;
   }

   *set_id = gprs_modem_manager.imsi;

   return SIRF_SUCCESS;
}

/** 
 * Create the AT Command server instance
 * 
 * @param port_name Name of the uart port the Modem is connected
 * @param apn       APN name
 * 
 * @return Success if the port was opened successfully
 */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_Create(
   tSIRF_CHAR const * const port_name,
   tSIRF_CHAR const * const apn) 
{
   tSIRF_RESULT result;
   tSIRF_UINT32 timeout;

   memset(&gprs_modem_manager,0,sizeof(gprs_modem_manager));
   gprs_modem_manager.cell_id.eNetworkType = LSM_INVALID_NT_TYPE;

   /* Open the GPRS Modem if it is connected */
   result = SIRF_GPRS_AT_COMMAND_SERVER_Create(port_name,
                                               apn);
   if ( SIRF_SUCCESS != result )
   {
      return (result);
   }

   /* Open our own handle for receiving messages */
   result = SIRF_GPRS_AT_COMMAND_SERVER_Open(&gprs_modem_manager.handle,
                                              gprs_modem_manager_callback);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   gprs_modem_manager.waiting_for_imsi = SIRF_TRUE;
   result = gprs_send_pcimi(gprs_modem_manager.handle);
   if (SIRF_SUCCESS != result)
   {
      gprs_modem_manager.waiting_for_imsi = SIRF_FALSE;
      return result;
   }

   gprs_modem_manager.pcced_requested_dump = SIRF_MSG_GPRS_AT_COMMAND_PCCED_DUMP_MAIN;
   result = gprs_send_pcced(gprs_modem_manager.handle,
                            gprs_modem_manager.pcced_requested_dump);

   /* Delay for up to 5 seconds for the IMSI and CELL ID messages to come in */
   timeout = 0;
   while ((LSM_INVALID_SETID_TYPE == gprs_modem_manager.imsi.SETidType || 
          LSM_INVALID_NT_TYPE == gprs_modem_manager.cell_id.eNetworkType) &&
          timeout < 50)
   {
      timeout++;
      SIRF_PAL_OS_THREAD_Sleep(100);
   }

   return result;
}


/** 
 * Delete the AT command server instance and release all allocated resources
 * 
 * @return Any non-zero error codes that occur during deletion, 
 *         SIRF_SUCCESS otherwise
 */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_Delete( void )
{
   tSIRF_RESULT result;

   result = SIRF_GPRS_AT_COMMAND_SERVER_Delete();

   return result;
} /* SIRF_GPRS_AT_COMMAND_Delete()*/


/**
 * @}
 * End of file.
 */

