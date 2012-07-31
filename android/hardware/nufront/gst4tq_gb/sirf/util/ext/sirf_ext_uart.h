/**
 * @addtogroup platform_src_sirf_util_ext
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
 * FILENAME:  sirf_ext_uart.h
 *
 * DESCRIPTION: Auxiliary serial port module
 *
 ***************************************************************************/

#ifndef __SIRF_EXT_UART_H__
#define __SIRF_EXT_UART_H__

/***************************************************************************
   Include files
***************************************************************************/

#include "sirf_types.h"
#include "sirf_ext.h"
#include "sirf_proto.h"
#include "sirf_msg_ssb.h"

/***************************************************************************
  Defines
***************************************************************************/

/* SIRF_EXT_UART must be defined on the command line */
#ifdef SIRF_EXT_UART
   #if (SIRF_EXT_UART >= 1) && (SIRF_EXT_UART <= 3)
      #define MAX_UART_PORTS   SIRF_EXT_UART  /* Maximum is 3 */
   #else
      #error Number of UART ports can be 1, 2 or 3 only.
   #endif
#else
   #define MAX_UART_PORTS      0
#endif


/* Thread stack sizes */
#if ( MAX_UART_PORTS >= 1 )
   #define SIRF_EXT_UART_THREAD_1_STACK_SIZE  ( 32 * 1024 )
#else
   #define SIRF_EXT_UART_THREAD_1_STACK_SIZE  0
#endif

#if ( MAX_UART_PORTS >= 2 )
   #define SIRF_EXT_UART_THREAD_2_STACK_SIZE  ( 32 * 1024 )
#else
   #define SIRF_EXT_UART_THREAD_2_STACK_SIZE  0
#endif

#if ( MAX_UART_PORTS >= 3 )
   #define SIRF_EXT_UART_THREAD_3_STACK_SIZE  ( 32 * 1024 )
#else
   #define SIRF_EXT_UART_THREAD_3_STACK_SIZE  0
#endif

#define SIRF_EXT_UART_THREAD_SIZE (SIRF_EXT_UART_THREAD_1_STACK_SIZE + \
                                   SIRF_EXT_UART_THREAD_2_STACK_SIZE + \
                                   SIRF_EXT_UART_THREAD_3_STACK_SIZE)

/***************************************************************************
   Prototype Definitions
***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/* Global initialization and uninitialization.  Called once per application */
tSIRF_RESULT SIRF_EXT_UART_Create( tSIRF_VOID );
tSIRF_RESULT SIRF_EXT_UART_Delete( tSIRF_VOID );

/* Request creation of one of the avaiable uart handlers and open it*/
tSIRF_RESULT SIRF_EXT_UART_Open(
   tSIRF_UINT32                           * const uartno,
   tSIRF_MSG_SSB_EXT_UART_OPEN_PORT const * const port_settings,
   tSIRF_EXT_PACKET_CALLBACK                      callback,
   tSIRF_PROTO_Parser                             proto_parser);

tSIRF_RESULT SIRF_EXT_UART_Close( tSIRF_UINT32 uartno );

/* Set the protocol and codec callback functions for the port.  These can
 * be changed at runtime */
tSIRF_RESULT SIRF_EXT_UART_SetCallbacks( 
   tSIRF_UINT32 uartno, 
   tSIRF_EXT_PACKET_CALLBACK callback,
   tSIRF_PROTO_Parser proto_parser);

/* This function is used to change the protocol in the middle of a data
 * transmission.  It is designed to be called ONLY from within a callback
 * previously set by SIRF_EXT_UART_SetCallbacks, and both function pointers
 * must be non-null.  The major difference is that this function does not
 * lock the callback mutex before changing */
tSIRF_RESULT SIRF_EXT_UART_SetCallbacksFromCallback( 
   tSIRF_UINT32 uartno, 
   tSIRF_EXT_PACKET_CALLBACK callback,
   tSIRF_PROTO_Parser proto_parser);

/* Send a payload over the uart, first wrap it in the protocol */
tSIRF_RESULT SIRF_EXT_UART_Send( 
   tSIRF_UINT32               uartno,
   tSIRF_VOID   const * const packet,
   tSIRF_UINT32               packet_length );

#ifdef __cplusplus
}
#endif


#endif /* __SIRF_EXT_UART_H__ */

/**
 * @}
 */

