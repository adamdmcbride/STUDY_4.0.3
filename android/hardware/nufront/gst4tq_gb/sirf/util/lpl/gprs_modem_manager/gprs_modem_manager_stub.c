/**
 * @addtogroup platform_src_sirf_util_ext
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
 ***************************************************************************/

#include <string.h>
#include <limits.h>

#include "sirf_types.h"
#include "sirf_pal.h"

#include "sirf_msg.h"
#include "sirf_msg_gprs_at_command.h"

#include "sirf_proto.h"
#include "sirf_proto_mas.h"
#include "sirf_proto_gprs_at_command.h"

#include "sirf_codec.h"

#include "sirf_ext.h"
#include "gprs_modem_manager.h"
#include "gprs_at_command_server.h"

#include "string_sif.h"
#if 0 /* @todo when util_if.h or equivilent functionality is added to the
       * SiRFRunTimeLib the else clause can be removed.*/
#include "util_if.h"
#else
#ifndef UTIL_Assert
   #define UTIL_Assert(_condition)
#endif

#ifndef UTIL_AssertAlways
   #define UTIL_AssertAlways()
#endif
#endif

/***************************************************************************
 * Local Data Declarations
 ***************************************************************************/

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
 * Stub version
 * 
 * @param cell_id cell id to fill out.
 * 
 * @return LSM_INVALID_NT_TYPE always
 */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_GetCellId(LSM_netCellID * cell_id)
{
   (void)cell_id;
   return (tSIRF_RESULT)LSM_INVALID_NT_TYPE;
}

/** 
 * Stub version
 * 
 * @param set_id  SET id to fill out
 * 
 * @return LSM_INVALID_SETID_TYPE
 */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_GetSETId(SETID_Info * set_id)
{
   (void)set_id;
   return (tSIRF_RESULT)LSM_INVALID_SETID_TYPE;
}

/** 
 * Create the AT Command server instance
 * 
 * @param port_name Name of the uart port the Modem is connected
 * 
 * @return Success if the port was opened successfully
 */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_Create(
   tSIRF_CHAR const * const port_name,
   tSIRF_CHAR const * const apn) 
{
   (void)port_name;
   (void)apn;
   return SIRF_SUCCESS;
}


/** 
 * Delete the AT command server instance and release all allocated resources
 * 
 * @return Any error codes that occur during deletion, SIRF_SUCCESS otherwise
 */
tSIRF_RESULT SIRF_GPRS_MODEM_MANAGER_Delete( void )
{
   return SIRF_SUCCESS;
} /* SIRF_GPRS_AT_COMMAND_Delete()*/


/**
 * @}
 * End of file.
 */

