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
 * FILENAME:  sirf_ext_uart.c
 *
 * DESCRIPTION: Uartiliary serial port module
 *
 ***************************************************************************/

#include <string.h>

#include "sirf_types.h"

#include "sirf_msg.h"
#include "sirf_msg_ssb.h"

#include "sirf_pal.h"

#include "sirf_ext.h"
#include "sirf_ext_uart.h"

#include "sirf_proto.h"

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
#include "sirfnav_demo_config.h"
/***************************************************************************
 * Local Data Declarations
 ***************************************************************************/

/** Maximum number of bytes to read at a time from the UART to find frame */
#define SIRF_EXT_UART_MAX_BUFFER_SIZE (4096)
/** Maximum string length for each character */
#define PORT_NUM_STRING_LENGTH (32)

/** Global mutex used to protect the allocation of each individual port */
static tSIRF_MUTEX uartmx = (tSIRF_MUTEX)NULL;

/** Uart control block */
typedef struct
{
   /* Details of this port */
   tSIRF_CHAR            port_name[PORT_NUM_STRING_LENGTH];
   tSIRF_UINT32          port_baud_rate;
   tSIRF_UINT32          port_flow_control;

   /* OS Objects */
   tSIRF_SEMAPHORE       semaphore;
   tSIRF_MUTEX           mutex;
   tSIRF_THREAD          thread_handle;
   tSIRF_HANDLE          port_handle;

   /* State information */
   tSIRF_BOOL            thread_running;
   tSIRF_BOOL            port_open;

   /* Results of opening the UART from within the thread */
   tSIRF_RESULT          result_open;
   tSIRF_RESULT          result_close;

   /* callback functions */
   tSIRF_PROTO_Parser        proto_parser;
   tSIRF_EXT_PACKET_CALLBACK packet_callback;

} tSIRF_UART_CONTROL_BLOCK;

/** Set of uart control blocks a unique one returned each time Open is called */
static tSIRF_UART_CONTROL_BLOCK uartcb[ MAX_UART_PORTS ];
/** Used with synchrionization to initialize a thread with the appropriate
 * control block when initially starting */
static tSIRF_UINT32 uartno_set;

static tSIRF_RESULT UART_GetNextAvailable(tSIRF_UINT32 *uartno);
static SIRF_PAL_OS_THREAD_DECL SIRF_EXT_UART_ReadThread( SIRF_PAL_OS_THREAD_PARAMS );

/** 
 * Iniatialization of the UART module.  Call this once and only once before 
 * calling @see SIRF_EXT_UART_Open
 * 
 * @return SIRF_SUCCESS if successful, or an error code returned from one
 * of the unsuccessful initialization routines.
 */
tSIRF_RESULT SIRF_EXT_UART_Create( tSIRF_VOID )
{
   tSIRF_RESULT result;

   if (NULL != uartmx)
   {
      return SIRF_EXT_UART_ALREADY_CREATED;
   }

   memset( &uartcb, 0, sizeof(uartcb) );

   result = SIRF_PAL_OS_MUTEX_Create( &uartmx );
   if ( SIRF_SUCCESS !=  result )
   {
      return result;
   }

   return result;

} /* SIRF_EXT_UART_Create()*/


/** 
 * Delete and close all open uarts.
 * 
 * @return SIRF_SUCCESS if successful, or an error code returned from one
 * of the unsuccessful Shutdown routines.  May also return 
 * SIRF_EXT_UART_NOT_CREATED if @see SIRF_EXT_UART_Create has not yet been 
 * called
 */
tSIRF_RESULT SIRF_EXT_UART_Delete( tSIRF_VOID )
{
   tSIRF_RESULT result;
   tSIRF_UINT32 uartno;

   if (NULL == uartmx)
   {
      return SIRF_EXT_UART_NOT_CREATED;
   }

   /* Close port in case it is still open*/
   for ( uartno=0; uartno < MAX_UART_PORTS; uartno++ )
   {
      SIRF_EXT_UART_Close( uartno );
   }

   result = SIRF_PAL_OS_MUTEX_Delete( uartmx );

   uartmx = NULL;

   return result;

} /* SIRF_EXT_UART_Delete()*/

/** 
 * Get the next avaiable uart port from the uart port array
 * 
 * @param uartno [out]On success this is set to the uartno allocated
 * 
 * @return SIRF_SUCCESS if a port num is allocated, and 
 *         SIRF_EXT_UART_NO_MORE_PORTS_AVAILABLE otherwise
 */
static tSIRF_RESULT UART_GetNextAvailable(tSIRF_UINT32 *uartno)
{
   tSIRF_UINT32 ii;

   SIRF_PAL_OS_MUTEX_Enter(uartmx);

   for ( ii=0; ii<MAX_UART_PORTS; ii++ )
   {
      if (!uartcb[ii].port_open)
      {
         uartcb[ii].port_open = SIRF_TRUE;
         *uartno = ii;
         break;
      }
   }

   SIRF_PAL_OS_MUTEX_Exit(uartmx);
   if ( MAX_UART_PORTS > ii )
   {
      return SIRF_SUCCESS;
   }
   else 
   {
      return SIRF_EXT_UART_NO_MORE_PORTS_AVAILABLE;
   }
}

/** 
 * Change the proto and packet callbacks from without a packet or proto parse 
 * function.
 *
 * This function locks the mutex so is safe, but if called from within the
 * callback function, the mutext will be re-entered which may cause a lockup
 * 
 * @param uartno       Uart to change
 * @param callback     New callback function pointer
 * @param proto_parser New Protocol Parser function pointer.
 * 
 * @return SIRF_SUCCESS or specific error code otherwise
 */
tSIRF_RESULT SIRF_EXT_UART_SetCallbacks( tSIRF_UINT32 uartno, 
                                         tSIRF_EXT_PACKET_CALLBACK callback,
                                         tSIRF_PROTO_Parser proto_parser)
{
   tSIRF_RESULT result;
   if (NULL == uartmx)
   {
      return SIRF_EXT_UART_NOT_CREATED;
   }

   if ( (uartno >= MAX_UART_PORTS )  ||
        (!uartcb[uartno].port_open ))
   {
      return SIRF_EXT_UART_NOT_RUNNING;
   }

   result = SIRF_PAL_OS_MUTEX_Enter(uartcb[uartno].mutex);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   uartcb[uartno].packet_callback = callback;
   uartcb[uartno].proto_parser = proto_parser;

   result = SIRF_PAL_OS_MUTEX_Exit( uartcb[uartno].mutex);

   return result;

} /* SIRF_EXT_UART_Set_Callback()*/

/** 
 * Change the proto and packet callbacks from within a packet or proto parse 
 * function.
 *
 * This function does not lock mutex's, but validates that the function pointers
 * are not NULL;
 * 
 * @param uartno       Uart to change
 * @param callback     New callback function pointer
 * @param proto_parser New Protocol Parser function pointer.
 * 
 * @return SIRF_SUCCESS or specific error code otherwise
 */
tSIRF_RESULT SIRF_EXT_UART_SetCallbacksFromCallback(  tSIRF_UINT32 uartno, 
                                                      tSIRF_EXT_PACKET_CALLBACK callback,
                                                      tSIRF_PROTO_Parser proto_parser)
{
   UTIL_Assert(NULL != callback);
   UTIL_Assert(NULL != proto_parser);

   if (NULL == uartmx)
   {
      return SIRF_EXT_UART_NOT_CREATED;
   }

   if ( (uartno >= MAX_UART_PORTS )  ||
        (!uartcb[uartno].port_open ))
   {
      return SIRF_EXT_UART_NOT_RUNNING;
   }

   uartcb[uartno].packet_callback = callback;
   uartcb[uartno].proto_parser = proto_parser;

   return SIRF_SUCCESS;
}

/** 
 * Send data to the UART.
 * 
 * @param uartno        The uart on which to write
 * @param packet        Pointer to the data to write
 * @param packet_length Amount of data to write
 * 
 * @return SIRF_SUCCESS or error code returned from a sub function.
 */
tSIRF_RESULT SIRF_EXT_UART_Send( 
   tSIRF_UINT32               uartno,
   tSIRF_VOID   const * const packet,
   tSIRF_UINT32               packet_length )
{
   tSIRF_RESULT result;
   tSIRF_UINT32 bytes_written;

   if (NULL == uartmx)
   {
      return SIRF_EXT_UART_NOT_CREATED;
   }

   if ( (uartno >= MAX_UART_PORTS )  ||
        (!uartcb[uartno].port_open ))
   {
      return SIRF_EXT_UART_NOT_RUNNING;
   }

   /* Send the data*/
   result = SIRF_PAL_COM_UART_Write( uartcb[uartno].port_handle, 
                                     packet, 
                                     packet_length, 
                                     &bytes_written);

   if ( SIRF_SUCCESS == result && bytes_written < packet_length)
   {
      return SIRF_EXT_UART_SEND_ERROR;
   }

   return result;

} /* SIRF_EXT_UART_Send() */





/** 
 * Open a uart.
 * 
 * @param uartno        Set to the uart opened on success. 
 * @param port_settings Settings for the port to open
 * @param callback      Callback to receive packets.
 * @param proto_parser  Protocol parsing function.
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
tSIRF_RESULT SIRF_EXT_UART_Open(
   tSIRF_UINT32                           * const uartno,
   tSIRF_MSG_SSB_EXT_UART_OPEN_PORT const * const port_settings,
   tSIRF_EXT_PACKET_CALLBACK                      callback,
   tSIRF_PROTO_Parser                             proto_parser)
{
   tSIRF_RESULT result;

   if (NULL == uartmx)
   {
      return SIRF_EXT_UART_NOT_CREATED;
   }

   result = UART_GetNextAvailable(uartno);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }
   
   strncpy(uartcb[*uartno].port_name,            
           port_settings->port_name,
           sizeof(uartcb[*uartno].port_name));

   uartcb[*uartno].port_baud_rate    = port_settings->baud_rate;
   uartcb[*uartno].port_flow_control = port_settings->flow_control;

   result = SIRF_PAL_OS_SEMAPHORE_Create( &(uartcb[*uartno].semaphore), 0 );
   if ( SIRF_SUCCESS != result )
   {
      goto SIRF_EXT_UART_Open_Error0;
   }

   result = SIRF_PAL_OS_MUTEX_Create( &(uartcb[*uartno].mutex));
   if ( SIRF_SUCCESS != result )
   {
      goto SIRF_EXT_UART_Open_Error1;
   }

   result = SIRF_PAL_COM_UART_Create(&uartcb[*uartno].port_handle);
   if ( SIRF_SUCCESS != result )
   {
      goto SIRF_EXT_UART_Open_Error2;
   }

   /* now that the resources are available we can just call SetCallbacks */
   result = SIRF_EXT_UART_SetCallbacks(*uartno,callback,proto_parser);
   if ( SIRF_SUCCESS != result )
   {
      goto SIRF_EXT_UART_Open_Error3;
   }
   
   /* This protects the global variable uartno_set from getting set by anthing
    * else until the local stack variable of the thread is set */
   result = SIRF_PAL_OS_MUTEX_Enter( uartmx );
   if ( SIRF_SUCCESS != result )
   {
      /* This is the same error case as an error from SetCallbacks since
       * set callbacks does not require any de-initialization */
      goto SIRF_EXT_UART_Open_Error3;
   }

   uartno_set = *uartno;
   result = SIRF_PAL_OS_THREAD_Create( SIRFNAV_DEMO_THREAD_EXT_UART1+*uartno, 
                                       (tSIRF_HANDLE)SIRF_EXT_UART_ReadThread, 
                                       &uartcb[*uartno].thread_handle );
   if ( SIRF_SUCCESS != result )
   {
      goto SIRF_EXT_UART_Open_Error4;
   }

   /* Wait for thread to start set the local stack variable to *uartno, 
    * and open the port, setting result_open */
   result = SIRF_PAL_OS_SEMAPHORE_Wait( uartcb[*uartno].semaphore, 
                                        SIRF_TIMEOUT_INFINITE );
   if ( SIRF_SUCCESS != result )
   {
      /* Semaphore wait failed, who knows if the result_open is successful
       * so to successfully shut down the thread we must goto exit condition
       * 6 which sets the thread running to FALSE and closes the UART allowing
       * the thread that was successfully created to exit
       */
      goto SIRF_EXT_UART_Open_Error6;
   }

   if ( uartcb[*uartno].result_open != SIRF_SUCCESS )
   {
      result = uartcb[*uartno].result_open;
      goto SIRF_EXT_UART_Open_Error5;
   }

   result = SIRF_PAL_OS_MUTEX_Exit( uartmx );
   if ( SIRF_SUCCESS != result )
   {
      goto SIRF_EXT_UART_Open_Error6;
   }

   return SIRF_SUCCESS;

/* Error handling. Release resources in reverse order than created. Report 
 * first error occured.*/
SIRF_EXT_UART_Open_Error6:
   /* Thread launched successfully but there was an error wih mutex exit
    * shut down the thread */
   uartcb[*uartno].thread_running = SIRF_FALSE;
   (void)SIRF_PAL_COM_UART_Close( uartcb[*uartno].port_handle );

SIRF_EXT_UART_Open_Error5:
   /* If the thread returns a failure code in result_open, it will exit 
    * immediately and release the semaphore.  Wait for that to happen */
   (void)SIRF_PAL_OS_SEMAPHORE_Wait( uartcb[*uartno].semaphore, SIRF_TIMEOUT_INFINITE );

   (void)SIRF_PAL_OS_THREAD_Delete( uartcb[*uartno].thread_handle );

SIRF_EXT_UART_Open_Error4:
   (void)SIRF_PAL_OS_MUTEX_Exit( uartmx );

SIRF_EXT_UART_Open_Error3:
   (void)SIRF_PAL_COM_UART_Delete( &uartcb[*uartno].port_handle );

SIRF_EXT_UART_Open_Error2:
   (void)SIRF_PAL_OS_MUTEX_Delete( uartcb[*uartno].mutex );

SIRF_EXT_UART_Open_Error1:
   (void)SIRF_PAL_OS_SEMAPHORE_Delete( uartcb[*uartno].semaphore );

SIRF_EXT_UART_Open_Error0:
   uartcb[*uartno].port_open = SIRF_FALSE;

   return result;

} /* UART_Open()*/


/** 
 * Close this UART
 * 
 * @param uartno The UART that is ready to be closed
 * 
 * @return SIRF_SUCCESS or non-zero error code.
 */
tSIRF_RESULT SIRF_EXT_UART_Close( tSIRF_UINT32 uartno )
{
   tSIRF_RESULT result, return_value = SIRF_SUCCESS;

   if (NULL == uartmx)
   {
      return SIRF_EXT_UART_NOT_CREATED;
   }

   if ( (uartno >= MAX_UART_PORTS )  ||
        (!uartcb[uartno].port_open ))
   {
      return SIRF_EXT_UART_NOT_RUNNING;
   }

   uartcb[uartno].port_open       = SIRF_FALSE;
   uartcb[uartno].thread_running  = SIRF_FALSE;
   SIRF_EXT_UART_SetCallbacks(uartno,NULL,NULL);

   /* Release resources in reverse order than created. Report first error 
    * occured.*/

   result = SIRF_PAL_COM_UART_Close( uartcb[uartno].port_handle );
   if ( SIRF_SUCCESS == return_value )
   {
      return_value = result;
   }

   result = SIRF_PAL_OS_SEMAPHORE_Wait( uartcb[uartno].semaphore, 
                                        SIRF_TIMEOUT_INFINITE );
   if ( SIRF_SUCCESS == return_value )
   {
      return_value = result;
   }

   result = uartcb[uartno].result_close;
   if ( SIRF_SUCCESS == return_value )
   {
      return_value = result;
   }

   result = SIRF_PAL_OS_THREAD_Delete( uartcb[uartno].thread_handle );
   if ( SIRF_SUCCESS == return_value )
   {
      return_value = result;
   }

   result = SIRF_PAL_COM_UART_Delete( &uartcb[uartno].port_handle );
   if ( SIRF_SUCCESS == return_value )
   {
      return_value = result;
   }

   result = SIRF_PAL_OS_MUTEX_Delete( uartcb[uartno].mutex );
   if ( SIRF_SUCCESS == return_value )
   {
      return_value = result;
   }

   result = SIRF_PAL_OS_SEMAPHORE_Delete( uartcb[uartno].semaphore );
   if ( SIRF_SUCCESS == return_value )
   {
      return_value = result;
   }

   return return_value;

} /* UART_Close()*/


/** 
 * Read thread responsible for reading all data from This UART and passing it to
 * the callback functions.
 */
static SIRF_PAL_OS_THREAD_DECL SIRF_EXT_UART_ReadThread( SIRF_PAL_OS_THREAD_PARAMS )
{
   tSIRF_RESULT result;
   tSIRF_UINT32 uartno;

   tSIRF_UINT32 bytes_read     = 0;
   tSIRF_UINT32 read_so_far    = 0;
   tSIRF_UINT32 data_length    = 0;

   tSIRF_UINT8  read_buffer[ SIRF_EXT_UART_MAX_BUFFER_SIZE ];

   SIRF_PAL_OS_THREAD_UNUSED;
   SIRF_PAL_OS_THREAD_START();

   uartno = uartno_set;

   /* Baud rate and flow control need to be set before opening the port */
   result = SIRF_PAL_COM_UART_Control(uartcb[uartno].port_handle,
                                      SIRF_COM_CTRL_UART_BAUD_RATE,
                                      (void*)uartcb[uartno].port_baud_rate);
   if (SIRF_SUCCESS == result)
   {
      
      result = SIRF_PAL_COM_UART_Control(uartcb[uartno].port_handle,
                                         SIRF_COM_CTRL_UART_FLOW_CONTROL,
                                         (void *)uartcb[uartno].port_flow_control);
   }

   /* Open the Port */
   if (SIRF_SUCCESS == result)
   {
      result = SIRF_PAL_COM_UART_Open(uartcb[uartno].port_handle,
                                      (const char*)uartcb[uartno].port_name);
   }

   /* If the device uses flow control then we need to set the lines stating
    * that we are ready to communicate and that it can send a response.  If
    * these lines are not hooked up then they may return failure, so ignore
    * the return codes */
   if (SIRF_SUCCESS == result)
   {
   /* Signal Data Terminal Ready */
      (void)SIRF_PAL_COM_UART_Control(uartcb[uartno].port_handle,
                                      SIRF_COM_CTRL_UART_SET_DTR,
                                      (void*)SIRF_TRUE);

      /* Signal Ready to Send, only if flow control is not used.  If flow
       * control is enabled, then this will be set or cleared by the driver */
      if (!uartcb[uartno].port_flow_control)
      {
         (void)SIRF_PAL_COM_UART_Control(uartcb[uartno].port_handle,
                                         SIRF_COM_CTRL_UART_SET_RTS,
                                         (void*)SIRF_TRUE);
      }
   }

   /* set the result and release the semaphore */
   uartcb[uartno].result_open = result;

   if ( SIRF_SUCCESS == uartcb[uartno].result_open )
   {
      uartcb[uartno].thread_running = SIRF_TRUE;
   }

   SIRF_PAL_OS_SEMAPHORE_Release( uartcb[uartno].semaphore );

   /*-------------------------------------------------------------------------*/
   /* Main loop */
   /*-------------------------------------------------------------------------*/
   while ( uartcb[uartno].thread_running )
   {
      /* Blocking read call */
      result = SIRF_PAL_COM_UART_Read( uartcb[uartno].port_handle, 
                                       read_buffer+read_so_far, 
                                       sizeof(read_buffer)-read_so_far, 
                                       &bytes_read );

      /* We just woke up.  Check to see if we've been asked to exit */
      if ( !uartcb[uartno].thread_running )
      {
         break;
      }

      /* Make sure the read was successful */
      if (SIRF_SUCCESS != result || 0 == bytes_read)
      {
         /* If the read returns immediately with no data then this sleep
          * avoids taking 100% of available cycles */
         SIRF_PAL_OS_THREAD_Sleep( 10 );
         continue;
      }

      /* Protect against dynamic changing of the callback and proto parser */
      result = SIRF_PAL_OS_MUTEX_Enter(uartcb[uartno].mutex);
      if (SIRF_SUCCESS != result)
      {
         /* Short sleep. Avoid potential lockup, waiting for being told to
          * exit */
         SIRF_PAL_OS_THREAD_Sleep( 10 );
         continue;
      }

      /* If either functio is null, we can't do anything */
      if ((NULL == uartcb[uartno].proto_parser   ) ||
          (NULL == uartcb[uartno].packet_callback) )
      {
         result = SIRF_PAL_OS_MUTEX_Exit(uartcb[uartno].mutex);
         SIRF_PAL_OS_THREAD_Sleep( 10 );/* Short sleep. Avoid potential lockup*/
         continue;
      }

      /* At this point we are under mutex protection, there are new bytes read
       * and the proto_parser and packet_callback are not null */
      read_so_far += bytes_read;

      do
      {
         result = uartcb[uartno].proto_parser( read_buffer, 
                                               read_so_far, 
                                               &data_length );

         if ( SIRF_SUCCESS == result )
         {
            result = uartcb[uartno].packet_callback(read_buffer,data_length);
         }
         else if ( result != SIRF_PROTO_NOT_ENOUGH_DATA_YET )
         {
            /* @todo report the error condition somehow, or fail silently */
         }
         /* Remove data used (good or bad) from a buffer*/
         read_so_far -= data_length;
         memmove( read_buffer, 
                  read_buffer+data_length, 
                  read_so_far );

      }
      while ( 0 < read_so_far && result != SIRF_PROTO_NOT_ENOUGH_DATA_YET );

      result = SIRF_PAL_OS_MUTEX_Exit(uartcb[uartno].mutex);
      if (SIRF_SUCCESS != result)
      {
         UTIL_Assert(SIRF_SUCCESS == result);
      }

   } /* while()*/

   /* @todo, this need to really be close, and the other close needs to be some type of stop */
   uartcb[uartno].result_close = SIRF_SUCCESS; /* SIRF_PAL_COM_UART_Close( uartcb[uartno].port_handle ); */

   (void)SIRF_PAL_OS_SEMAPHORE_Release( uartcb[uartno].semaphore );

   SIRF_PAL_OS_THREAD_RETURN();

} /* SIRF_EXT_UART_ReadThread()*/

/**
 * @}
 * End of file.
 */

