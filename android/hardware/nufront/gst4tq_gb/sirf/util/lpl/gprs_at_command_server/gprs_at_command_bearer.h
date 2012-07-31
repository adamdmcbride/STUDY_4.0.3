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
 * FILENAME:  sirf_gprs_at_command_bearer.h
 *
 * DESCRIPTION: Auxiliary serial port module
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/util/lpl/gprs_at_command_server/gprs_at_command_bearer.h $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************/

#ifndef __GPRS_AT_COMMAND_BEARER_H__
#define __GPRS_AT_COMMAND_BEARER_H__

/***************************************************************************
   Include files
***************************************************************************/

#include "sirf_types.h"

/***************************************************************************
  Defines
***************************************************************************/

/***************************************************************************
   Prototype Definitions
***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

tSIRF_RESULT gprs_stop(tSIRF_UINT32 uartno);
tSIRF_RESULT gprs_start(tSIRF_UINT32 uartno,
                        tSIRF_BOOL flow_control,
                        tSIRF_CHAR const * const apn);
tSIRF_RESULT gprs_send_tcpip_wipdata(tSIRF_UINT32 index);
tSIRF_RESULT gprs_send_close_data(tSIRF_UINT32 uartno);

#endif /* __GPRS_AT_COMMAND_BEARER_H__ */

/**
 * @}
 */

