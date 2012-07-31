/**
 * @addtogroup 
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2005-2008 by SiRF Technology, Inc. All rights reserved.*
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
 * FILE: lsm_io_error.h
 *
 ***************************************************************************/

#ifndef __LSM_IO_ERROR_H__
#define __LSM_IO_ERROR_H__


#include "sirf_types.h"





/* LSM_Input() return values -------------------------------------------*/


#define LSM_IO_ERROR_INVALID_MSG_LENGTH   0x3101
#define LSM_IO_ERROR_QUEUE_ERROR          0x3102
#define LSM_IO_ERROR_NULL_POINTER         0x3103
#define LSM_IO_ERROR_NOT_STARTED          0x3104

/* Prototypes --------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __LSM_IO_ERROR_H */

/**
 * @}
 */

