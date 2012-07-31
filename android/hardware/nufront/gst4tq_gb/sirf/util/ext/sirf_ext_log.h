/**
 * @addtogroup platform_src_sirf_util_ext
 * @{
 */
 
 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2005-2010 by SiRF Technology, a CSR plc Company.
 *     All rights reserved.*
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
 * FILENAME:  sirf_ext_log.h
 *
 ***************************************************************************/

#ifndef __SIRF_EXT_LOG_H__
#define __SIRF_EXT_LOG_H__

/***************************************************************************
 *  Include files
 ***************************************************************************/
#include "sirf_errors.h"
#include "sirf_types.h"

/***************************************************************************
 *  Defines
 ***************************************************************************/

typedef enum {
   SIRF_EXT_LOG_SIRF_ASCII_TEXT,
   SIRF_EXT_LOG_SIRF_BINARY_STREAM
} tSIRF_EXT_LOG_TYPE;

#define SIRF_EXT_LOG_MAX_MESSAGE_LEN 3072


#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
 *  Prototype Definitions
 ***************************************************************************/
tSIRF_RESULT SIRF_EXT_LOG_Open( tSIRF_EXT_LOG_TYPE type, tSIRF_CHAR *filename );
tSIRF_RESULT SIRF_EXT_LOG_Close( tSIRF_VOID );

tSIRF_RESULT SIRF_EXT_LOG_Send( tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length );
tSIRF_RESULT SIRF_EXT_LOG_Send_Passthrough( tSIRF_VOID *PktBuffer, tSIRF_UINT32 PktLen );

#ifdef __cplusplus
}
#endif

#endif /* __SIRF_EXT_LOG_H__ */

/**
 * @}
 * End of file.
 */
