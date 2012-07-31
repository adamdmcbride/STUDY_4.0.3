/**
 * @addtogroup sif_interface
 * @ingroup SiRFInstantFix
 * @{
 */

/**
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006-2010 by SiRF Technology, a CSR plc Company.
 *    All rights reserved.
 *
 *    This Software is protected by United States copyright laws and
 *    international treaties.  You may not reverse engineer, decompile
 *    or disassemble this Software.
 *
 *    WARNING:
 *    This Software contains SiRF Technology Inc.’s confidential and
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this
 *    Software without SiRF Technology, Inc.’s  express written
 *    permission.   Use of any portion of the contents of this Software
 *    is subject to and restricted by your signed written agreement with
 *    SiRF Technology, Inc.
 *
 */

/**
 * @file: sif_errors.h
 * @brief: SiRF Instant Fix error codes.
 */

#ifndef __SIF_ERRORS_H__
#define __SIF_ERRORS_H__

#if 1 /* This is for 4T as sirf_errors.h already has all the enums defined */

#include "sirf_errors.h"

#else /* These enums are required in case of 3tw as NO sirf_errors.h */

enum
{

   /* PAL Network Errors                     = 0x27xx */
   SIRF_PAL_NET_CONNECT_INPROGRESS           = 0x2709,
   SIRF_PAL_NET_CONNECT_WOULD_BLOCK          = 0x270A,
   SIRF_PAL_NET_CONNECT_ALREADY              = 0x270B,

   /* CLM Control Errors                     = 0x10xx */
   CLM_CTRL_ALREADY_STARTED                  = 0x1000,
   CLM_CTRL_ALREADY_STOPPED                  = 0x1001,
   CLM_CTRL_INVALID_MODE                     = 0x1002,
   CLM_CTRL_ERROR_OPENING_PORT               = 0x1003,
   CLM_CTRL_RTOS_ERROR                       = 0x1004,
   CLM_CTRL_MODE_NOT_SPECIFIED               = 0x1005,

   /* SEA File Errors                        = 0x11xx */
   SEA_FILE_OK                               = 0x1100,
   SEA_FILE_ERROR_DECODING                   = 0x1101,
   SEA_FILE_ERROR_ZERO_FILE                  = 0x1102,
   SEA_FILE_ERROR_OVERFLOW                   = 0x1103,
   SEA_FILE_ERROR_IDS_EXCEPTION_CAUGHT       = 0x1104,
   SEA_FILE_ERROR_DOWNLOAD_VERIFY            = 0x1105,
   SEA_FILE_ERROR_REG_DENY                   = 0x1106,
   SEA_FILE_ERROR_DOWNLOAD_DENY              = 0x1107,

   /* EXT UART Errors                        = 0x45xx */
   SIRF_EXT_UART_ALREADY_CREATED             = 0x4500,
   SIRF_EXT_UART_NOT_CREATED                 = 0x4501,
   SIRF_EXT_UART_ALREADY_RUNNING             = 0x4502,
   SIRF_EXT_UART_NOT_RUNNING                 = 0x4503,
   SIRF_EXT_UART_UNKNOWN_PROTOCOL            = 0x4504,
   SIRF_EXT_UART_NO_MORE_PORTS_AVAILABLE     = 0x4505,
   SIRF_EXT_UART_PROTOCOL_NOT_SET            = 0x4506,
   SIRF_EXT_UART_SEND_ERROR                  = 0x4507

};
#endif
#endif //__SIF_ERRORS_H__

/**
 * @}
 * End of file.
 */

