/**
 * @addtogroup app_util_gprs_modem_manager
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
 * FILENAME:  sirf_gprs_modem_manager.h
 *
 * DESCRIPTION: Manager for the GPRS Modem handling initilization, setup
 *              and some important unsolicited messages.
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/util/lpl/gprs_modem_manager/gprs_modem_manager.h $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************/

#ifndef __SIRF_GPRS_MODEM_MANAGER_H__
#define __SIRF_GPRS_MODEM_MANAGER_H__

/***************************************************************************
   Include files
***************************************************************************/

#include "sirf_types.h"
#include "sirf_ext.h"
#include "sirf_proto.h"
#include "sirf_msg_ssb.h"
#include "LSM_Types.h"

/***************************************************************************
  Defines
***************************************************************************/

/***************************************************************************
   Prototype Definitions
***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/* Get the latest stored Cell ID information if available */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_GetCellId(LSM_netCellID * cell_id);
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_GetSETId(SETID_Info * set_id);

/* Global initialization and uninitialization.  Called once per application */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_Create( 
   tSIRF_CHAR const * const port_name,
   tSIRF_CHAR const * const apn );

tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_Delete( tSIRF_VOID );

#ifdef __cplusplus
}
#endif


#endif /* __SIRF_GPRS_MODEM_MANAGER_H__ */

/**
 * @}
 */

