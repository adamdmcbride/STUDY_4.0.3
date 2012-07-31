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
 * FILENAME:  sirf_gprs_at_command_msg_handler.h
 *
 * DESCRIPTION: Auxiliary serial port module
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/util/lpl/gprs_at_command_server/gprs_at_command_msg_handler.h $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************/

#ifndef __SIRF_GPRS_AT_COMMAND_MSG_HANDLER_H__
#define __SIRF_GPRS_AT_COMMAND_MSG_HANDLER_H__

/***************************************************************************
   Include files
***************************************************************************/

#include "sirf_types.h"
#include "gprs_at_command_server.h"

/***************************************************************************
  Defines
***************************************************************************/

/***************************************************************************
   Prototype Definitions
***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

tSIRF_RESULT handle_response_SIRF_MSG_GPRS_AT_COMMAND_PWIPDATA(
   gprs_msg_t           const * const msg,
   tSIRF_UINT32                       response_number,
   gprs_server_state_t        * const state);

tSIRF_RESULT handle_response_SIRF_MSG_GPRS_AT_COMMAND_PCGMI(
   gprs_msg_t           const * const msg,
   tSIRF_UINT32                       response_number,
   gprs_server_state_t        * const state);

tSIRF_RESULT handle_response_SIRF_MSG_GPRS_AT_COMMAND_PCCED(
   gprs_msg_t           const * const msg,
   tSIRF_UINT32                       response_number,
   tSIRF_UINT32                       request_type,
   gprs_server_state_t        * const state);

tSIRF_RESULT handle_response_single_SIRF_MSG_GPRS_AT_COMMAND_OK_expected(
   gprs_msg_t           const * const msg,
   tSIRF_UINT32                       response_number,
   gprs_server_state_t        * const state);

#ifdef __cplusplus
}
#endif


#endif /* __SIRF_GPRS_AT_COMMAND_MSG_HANDLER_H__ */

/**
 * @}
 */

