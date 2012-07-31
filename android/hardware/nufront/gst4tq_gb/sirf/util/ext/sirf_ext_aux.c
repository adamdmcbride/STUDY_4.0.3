/**
 * @addtogroup platform_src_sirf_util_ext
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *  Copyright (c) 2005-2010 by SiRF Technology, a CSR plc Company.         *
 *  All rights reserved.                                                   *
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
 * FILENAME:  sirf_ext_aux.c
 *
 * DESCRIPTION: This file include the data structures and the functions to
 *              implement the registration of protocols with UI mdoule
 *
 ***************************************************************************/

#ifndef UTIL_Assert
   #define UTIL_Assert(_condition)
#endif

#ifndef UTIL_AssertAlways
   #define UTIL_AssertAlways()
#endif

#ifdef SIRF_EXT_AUX

/***************************************************************************
 * Include Files
 ***************************************************************************/
#include <stdio.h>

#include "sirf_types.h"
#include "sirf_pal.h"
#include "sirf_ext.h"

#ifdef SIRFNAV_DEMO
#include "sirfnav_demo_config.h"
#endif
#include "string_sif.h"
#include "sirf_ext_aux.h"
#include "sirf_codec.h"
#include "sirf_proto_common.h"

/***************************************************************************
 * Local Data Declarations
 ***************************************************************************/

static tSIRF_UINT8      s_demo_com_port[MAX_PORT_NUM_STRING_LENGTH];
static tSIRF_SEMAPHORE  c_aux_open_sem;
static tSIRF_HANDLE     c_ExtAuxHandle = SIRF_INVALID_HANDLE;
static tSIRF_UINT8      c_auxno_set = 0;     /* note: thread create mutex */
static tSIRF_THREAD     c_hCommReaderThread;
static tSIRF_BOOL       b_Aux_Thread_Running = SIRF_FALSE;

static SIRF_PAL_OS_THREAD_DECL SIRF_EXT_AUX_CommThread(SIRF_PAL_OS_THREAD_PARAMS);

static tSIRF_EXT_PACKET_CALLBACK f_callback = NULL;

static tSIRF_MUTEX      c_AUX_COM_WRITE_Critical;

/***************************************************************************
 * Global Data Declarations
 ***************************************************************************/

/***************************************************************************
 * Description:
 * Parameters:
 * Returns:
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_AUX_Create(tSIRF_UINT8* port_str, tSIRF_UINT32 baud_rate)
{
   tSIRF_RESULT tRet;

   /* Create write mutex */
   tRet = SIRF_PAL_OS_MUTEX_Create(&c_AUX_COM_WRITE_Critical);
   if (SIRF_SUCCESS == tRet)
   {
      /* programmers note: this allocates a COM channel for each potential input port;
       * right now this is always a COM port, but when we go to LPL, it will become
       * a programmatic interface. Also, fyi, we only support one COM port here for now. */
      tRet = SIRF_PAL_OS_SEMAPHORE_Create(&c_aux_open_sem, 0);
      if (SIRF_SUCCESS == tRet)
      {
         tRet = SIRF_PAL_COM_UART_Create(&c_ExtAuxHandle);
         if (SIRF_SUCCESS == tRet)
         {
            tRet = SIRF_EXT_AUX_Open(&port_str[0], baud_rate);
            if (tRet == SIRF_SUCCESS)
            {
               /* rds programmers note; c_auxno_set is really a counting semaphore
                * that clears when the thread starts running in context */
               c_auxno_set = 1;

#if defined(TOOLCHAIN_VC8)
#pragma warning (disable : 4054) /* cast from function to data pointer */
#endif

               tRet = SIRF_PAL_OS_THREAD_Create(SIRF_EXT_AUX_THREAD_, (tSIRF_HANDLE)SIRF_EXT_AUX_CommThread, &c_hCommReaderThread);

#if defined(TOOLCHAIN_VC8)
#pragma warning (default : 4054) /* cast from function to data pointer */
#endif

               if (SIRF_SUCCESS == tRet)
               {
                  while (c_auxno_set)
                  {
                     SIRF_PAL_OS_THREAD_Sleep(100);
                  }
               }
               else
               {
                  (void)SIRF_PAL_OS_SEMAPHORE_Delete(c_aux_open_sem);
                  (void)SIRF_PAL_COM_UART_Delete(&c_ExtAuxHandle);
                  SIRF_EXT_AUX_Close();
               } /* Thread Create */
            }
            else
            {
               (void)SIRF_PAL_OS_SEMAPHORE_Delete(c_aux_open_sem);
               (void)SIRF_PAL_COM_UART_Delete(&c_ExtAuxHandle);
            } /* OpenAux */
         }
         else
         {
            (void)SIRF_PAL_OS_SEMAPHORE_Delete(c_aux_open_sem);

         } /* COM_Create */

      } /* SEMAPHORE_Create */

   } /* Mutex create */

   return tRet;

} /* SIRF_EXT_AUX_Create */

/***************************************************************************
 * Description:
 * Parameters:
 * Returns:
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_AUX_Delete()
{
   /* Note: chose to use the first non-success return value as this function's
      return value. May need to reevaluate this. */
   tSIRF_RESULT com_delete_ret = SIRF_COM_RET_FAILURE_GENERIC;
   tSIRF_RESULT mutex_delete_ret = SIRF_PAL_OS_ERROR;

   if (c_ExtAuxHandle != SIRF_INVALID_HANDLE)
   {
      (void)SIRF_EXT_AUX_Close();
      com_delete_ret = SIRF_PAL_COM_UART_Delete(&c_ExtAuxHandle);
      UTIL_Assert(SIRF_SUCCESS == com_delete_ret);
   }

   b_Aux_Thread_Running = SIRF_FALSE;
   if(c_hCommReaderThread != NULL)
   {
      (void)SIRF_PAL_OS_THREAD_Delete(c_hCommReaderThread);
   }

   if(c_aux_open_sem != NULL)
   {
      (void)SIRF_PAL_OS_SEMAPHORE_Delete(c_aux_open_sem);
   }

   /* Delete Write Mutex */
   if(c_AUX_COM_WRITE_Critical != NULL)
   {
      mutex_delete_ret = SIRF_PAL_OS_MUTEX_Delete(c_AUX_COM_WRITE_Critical);
   }
   c_AUX_COM_WRITE_Critical = 0;

   if (SIRF_SUCCESS == com_delete_ret)
   {
      return mutex_delete_ret;
   }
   return com_delete_ret;

} /* SIRF_EXT_AUX_Delete */

/***************************************************************************
 * Description: Call here to close one of the AUX ports
 * Parameters:  index of the AUX port to close
 * Returns:     SIRF_SUCCESS : SIRF_PAL_COM_ERROR
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_AUX_Close()
{
   if(SIRF_SUCCESS == SIRF_PAL_COM_UART_Close(c_ExtAuxHandle))
   {
      return SIRF_SUCCESS;
   }

   return SIRF_FAILURE;

} /* SIRF_EXT_AUX_Close */

/***************************************************************************
 * Description: Call here to open one of the AUX ports
 * Returns:     SIRF_PAL_COM_ERROR or SIRF_SUCCESS
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_AUX_Open(tSIRF_UINT8* port_str, tSIRF_UINT32 baud_rate)
{
   tSIRF_RESULT tRet;

   strlcpy((char*)&s_demo_com_port[0], (const char*)port_str, sizeof(s_demo_com_port));

   /* no flow control for aux port */
   do
   {
      tRet = SIRF_PAL_COM_UART_Control(
                  c_ExtAuxHandle,
                  SIRF_COM_CTRL_UART_BAUD_RATE,
                  (void *)baud_rate);
      UTIL_Assert(SIRF_SUCCESS == tRet);
      if (SIRF_SUCCESS != tRet)
      {
         break;
      }

      tRet = SIRF_PAL_COM_UART_Control(
                  c_ExtAuxHandle,
                  SIRF_COM_CTRL_UART_FLOW_CONTROL,
                  (void *)SIRF_FALSE);
      UTIL_Assert(SIRF_SUCCESS == tRet);
      if (SIRF_SUCCESS != tRet)
      {
         break;
      }

      tRet = SIRF_PAL_COM_UART_Control(
                  c_ExtAuxHandle,
                  SIRF_COM_CTRL_READ_BYTE_TIMEOUT,
                  (void *)SIRF_TIMEOUT_INFINITE);
      UTIL_Assert(SIRF_SUCCESS == tRet);
      if (SIRF_SUCCESS != tRet)
      {
         break;
      }

      tRet = SIRF_PAL_COM_UART_Open(
                  c_ExtAuxHandle,
                  (const char*)&port_str[0]);
      UTIL_Assert(SIRF_SUCCESS == tRet);
      if (SIRF_SUCCESS != tRet)
      {
         break;
      }

#ifdef TOOLCHAIN_VC8
#pragma warning (disable : 4127) /* conditional expression is constant */
#endif
   } while(SIRF_FALSE);
#ifdef TOOLCHAIN_VC8
#pragma warning (default : 4127) /* conditional expression is constant */
#endif


   (void)SIRF_PAL_OS_SEMAPHORE_Release(c_aux_open_sem);
   return(tRet);

} /* SIRF_EXT_AUX_Open */

/***************************************************************************
 * Description: Call here to re-open the AUX port
 * Parameters:  none
 * Returns:     SIRF_FAILURE or SIRF_SUCCESS
 ***************************************************************************/

tSIRF_UINT32 SIRF_EXT_AUX_Reopen(tSIRF_UINT8 auxno)
{
   tSIRF_RESULT  com_return_value;
   tSIRF_UINT32  return_value;
   char          port_str[MAX_PORT_NUM_STRING_LENGTH];

   /* Unused parameter */
   return_value = SIRF_FAILURE;

   com_return_value = SIRF_PAL_COM_UART_Close(c_ExtAuxHandle);
   UTIL_Assert(SIRF_SUCCESS == com_return_value);

   sprintf(port_str, "\\\\.\\COM%d", auxno);
   com_return_value = SIRF_PAL_COM_UART_Open(c_ExtAuxHandle, (const char*)&port_str[0]);

   if(SIRF_SUCCESS == com_return_value)
   {
      return_value = SIRF_SUCCESS;
   }

   return(return_value);

} /* SIRF_EXT_AUX_Reopen */

/***************************************************************************
 * DESCRIPTION: Reset the AUX port to new operating parameters
 *
 * programmers note: this routine is really a hook to handle the switch
 *    from nmea to ssb and back when operating in HS mode
 *
 * PARAMETERS:  baud rate, protocol
 * RETURN:      SIRF_SUCCESS : SIRF_FAILURE
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_AUX_PortReset(tSIRF_UINT32 baud_rate, tSIRF_UINT32 protocol)
{
   tSIRF_RESULT tRet = SIRF_SUCCESS;

   /* Unused Parameters. */
   (tSIRF_VOID)protocol;

   SIRF_EXT_AUX_Close();
   /* rds programmers note: pass in the port id as a parameter */
   tRet = SIRF_EXT_AUX_Open(&s_demo_com_port[0], baud_rate);
   if (SIRF_SUCCESS != tRet)
   {
      /* rds programmers note: probably should log a debug message here */
   }
   return tRet;

} /* SIRF_EXT_AUX_PortReset */

/***************************************************************************
 * Description: Send a byte-stream buffer out on the AUX port(s)
 * Parameters:  packed byte-stream, size of same
 * Returns:     nothing
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_AUX_Send( tSIRF_UINT8 auxno, tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
{
   tSIRF_UINT8  payload[SIRF_EXT_AUX_MAX_MESSAGE_LEN];
   tSIRF_UINT8  packet[SIRF_EXT_AUX_MAX_MESSAGE_LEN+10];
   tSIRF_UINT32 payload_length = sizeof(payload);
   tSIRF_UINT32 packet_length = sizeof(packet);
   tSIRF_RESULT tRet = SIRF_SUCCESS;

   /* Unused parameter */
   (tSIRF_VOID)auxno;

   /* Call the generic encode function, but does not support AI3  */
   tRet = SIRF_CODEC_Encode( message_id, message_structure, message_length, payload, &payload_length );
   if ( tRet != SIRF_SUCCESS )
   {
      return tRet;
   }

   /* Apply the protocol wrapper */
   tRet = SIRF_PROTO_Wrapper( payload, payload_length, packet, &packet_length );
   if ( tRet != SIRF_SUCCESS )
   {
      return tRet;
   }

   /* Send the data */
   if (SIRF_SUCCESS == SIRF_PAL_OS_MUTEX_Enter(c_AUX_COM_WRITE_Critical))
   {
      tSIRF_RESULT     write_return_value;
      tSIRF_UINT32     bytes_written;

      write_return_value = SIRF_PAL_COM_UART_Write(
                              c_ExtAuxHandle,
                              packet,
                              packet_length,
                              &bytes_written);
      if(SIRF_SUCCESS == write_return_value &&
         bytes_written == packet_length)
      {
         tRet = SIRF_SUCCESS;
      }

      SIRF_PAL_OS_MUTEX_Exit(c_AUX_COM_WRITE_Critical);
   }

   return tRet;

} /* SIRF_EXT_AUX_Send */



/***************************************************************************
 * Description: Send a byte-stream buffer out on the AUX port(s)
 * Parameters:  packed byte-stream, size of same
 * Returns:     nothing
 ***************************************************************************/

tSIRF_RESULT SIRF_EXT_AUX_Send_Passthrough( tSIRF_UINT8 auxno, tSIRF_VOID *PktBuffer, tSIRF_UINT32 PktLen )
{
   tSIRF_RESULT tRet = SIRF_SUCCESS;

   /* Unused parameter */
   (tSIRF_VOID)auxno;

   if (SIRF_SUCCESS == SIRF_PAL_OS_MUTEX_Enter(c_AUX_COM_WRITE_Critical))
   {
      tSIRF_RESULT     write_return_value;
      tSIRF_UINT32     bytes_written;

      write_return_value = SIRF_PAL_COM_UART_Write(
                              c_ExtAuxHandle,
                              PktBuffer,
                              PktLen,
                              &bytes_written);
      if(SIRF_SUCCESS == write_return_value &&
         bytes_written == PktLen)
      {
         tRet = SIRF_SUCCESS;
      }

      SIRF_PAL_OS_MUTEX_Exit(c_AUX_COM_WRITE_Critical);
   }

   return tRet;
} /* SIRF_EXT_AUX_Send_Passthrough */

/***************************************************************************
 * DESCRIPTION:
 * PARAMETERS:
 * RETURN:      nothing
 ***************************************************************************/

tSIRF_VOID SIRF_EXT_AUX_Callback_Register( tSIRF_EXT_PACKET_CALLBACK callback_func )
{
   f_callback = callback_func;
}

/***************************************************************************
 * Description:   If in SL mode, parse as F, else parse as SSB or NMEA
 * Parameters:   none, really, just a place holder
 * Returns:      doesn't, unless the caller terminates it with c_demo_running
 ***************************************************************************/
#define MAX_BUFFER_SIZE 6000
SIRF_PAL_OS_THREAD_DECL SIRF_EXT_AUX_CommThread(SIRF_PAL_OS_THREAD_PARAMS)
{
   tSIRF_UINT32 BytesRead = 0;     /* byte count in raw message buffer */
   tSIRF_UINT8 Buf[MAX_BUFFER_SIZE];      /* raw message buffer */

   /* Unused Parameters. */
   SIRF_PAL_OS_THREAD_UNUSED

   /* the caller is waiting for this to go to zero before moving on to the next port */
   c_auxno_set = 0;
   b_Aux_Thread_Running = SIRF_TRUE;

   SIRF_PAL_OS_THREAD_START();

   while (SIRF_TRUE == b_Aux_Thread_Running)
   {
      BytesRead = 0;

      /* try to get some work; if it was a false positive, try again */
      if (SIRF_SUCCESS != SIRF_PAL_COM_UART_Read(c_ExtAuxHandle, Buf, sizeof(Buf), &BytesRead))
      {
         SIRF_PAL_OS_THREAD_Sleep(100);
         continue;
      }

      /* we might have waited quite awhile; don't artificially delay the user */
      if (SIRF_FALSE == b_Aux_Thread_Running)
      {   /* note: this guards against a race condition where we run the rest of
             this pass while the rest of the code is halted */
         break;
      }

      if ( BytesRead )
      {
         /* Send the data to the callback function, if it is registered */
         if ( f_callback )
         {
            f_callback( (tSIRF_UINT8 *)&Buf, BytesRead );
         }
      }
   }

   SIRF_PAL_OS_THREAD_RETURN();

} /* SIRF_EXT_AUX_CommThread */

#endif /* SIRF_EXT_AUX */

/**
 * @}
 * End of file.
 */

