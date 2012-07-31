/**
 * @addtogroup platform_src_sirf_util_ext
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2005 - 2011 by SiRF Technology, a CSR plc Company.     *
 *    All rights reserved.                                                 *
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
 * FILENAME:  sirf_ext_tcpip.c
 *
 * DESCRIPTION: This file include the data structures and the functions to
 *              implement the registration of protocols with UI module
 *
 ***************************************************************************/

#ifdef SIRF_EXT_TCPIP

#include <string.h>
#include <stdio.h>
#include "sirf_types.h"
#include "sirf_pal.h"

#include "string_sif.h"

#include "sirf_proto_common.h"
#include "sirf_codec_ssb.h"
#include "sirf_ext_tcpip.h"
#include "sirf_codec.h"

#include "sirf_msg.h"
#include "sirf_msg_ssb.h"

#include "sirfnav_demo_config.h"  /* @TODO: review making a global define instead with Stefan */ 

#define RECEIVE_BUFFER_SIZE  6000

#define DEFAULT_TIMEOUT      1000
#define CONNECT_TIMEOUT      5000

/* This needs to be redefined and renamed */
#define GPS_MAX_PACKET_LEN 512

static tSIRF_BOOL      c_running;
static tSIRF_THREAD    c_tcp_connect_handle;
static tSIRF_SOCKET    c_tcp_sock;
static tSIRF_BOOL      c_anything_received;
static tSIRF_UINT16    c_tcpip_port;
static tSIRF_CHAR      c_tcpip_addr[256];

static tSIRF_UINT32    c_nsent;        /* count of successful sends */
static tSIRF_UINT32    c_lasterr;      /* c_nsent value at last error */

static tSIRF_EXT_PACKET_CALLBACK f_callback = NULL;

/** 
 * Encode a message with the appropriate protocol and send on the aux port
 * 
 * @param message_id    Identifier of message to send 
 * @param message_structure   ID-dependent data to encode 
 * @param  message_length     size of message structure in bytes
 * 
 * @return SIRF_SUCCESS if sent successfully.  TCP error code otherwise.
 */
tSIRF_RESULT SIRF_EXT_TCPIP_Send( tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
{
   tSIRF_UINT8  payload[SIRF_EXT_TCPIP_MAX_MESSAGE_LEN];
   tSIRF_UINT8  packet[SIRF_EXT_TCPIP_MAX_MESSAGE_LEN+10];
   tSIRF_UINT32 payload_length = sizeof(payload);
   tSIRF_UINT32 packet_length = sizeof(packet);
   tSIRF_RESULT tRet = SIRF_SUCCESS;

   if ( !c_running || (0 == message_length) || !message_structure ||
        SIRF_PAL_NET_INVALID_SOCKET == c_tcp_sock )
   {
      return SIRF_FAILURE;
   }

#if defined(OS_ANDROID) && defined(SENS_SSB_DATA_INPUT_MODE)
    /* In case of Android we use TCP/IP to pass sensor data into the location manager and
       to report status updates from location manager to the sensors service. 
       This conditional compilation is intented to reduce amount of data sent over TCP/IP to the sensor service
    */
    if( message_id != SIRF_MSG_SSB_RCVR_STATE && message_id != SIRF_MSG_SSB_POINT_N_TELL_OUTPUT )
        return SIRF_SUCCESS;
#endif

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

   tRet = SIRF_EXT_TCPIP_Send_Passthrough(packet, packet_length);

   return tRet;
}

/** 
 * Send a message that requires no Codec
 * 
 * @param message Message to send
 * @param length  Length of message
 * 
 * @return SIRF_SUCCESS if sent successfully.  TCP error code otherwise.
 */
tSIRF_RESULT SIRF_EXT_TCPIP_Send_Passthrough( tSIRF_VOID *message, tSIRF_UINT32 length )
{
   tSIRF_UINT32 nwritten;
   tSIRF_RESULT tRet;

   if ( !c_running || (0 == length) || !message ||
        SIRF_PAL_NET_INVALID_SOCKET == c_tcp_sock )
   {
      return SIRF_FAILURE;
   }

   /* Only print on the first of consecutive errors. */
   tRet = SIRF_PAL_NET_Write(c_tcp_sock, message, length, &nwritten);
   if (tRet == SIRF_SUCCESS && nwritten == length)
   {
      c_nsent++;
   }
   else if (c_lasterr != c_nsent)
   {
      DEBUGMSG(1, (DEBUGTXT("tcp aux: send error packet %lu\n"), c_nsent));
      c_lasterr = c_nsent;
   }

   return tRet;
}

/** 
 * Set the callback function.
 * 
 * @param callback_func pointer to the callback function.
 * 
 */
tSIRF_VOID SIRF_EXT_TCPIP_Callback_Register( tSIRF_EXT_PACKET_CALLBACK callback_func )
{
   f_callback = callback_func;
}

/** 
 * Guts of the thread for reading and writing to the socket.
 *
 * Upon data it calls the fucntion set by SIRF_EXT_TCPIP_Callback_Register
 */
static tSIRF_VOID TCPReaderWriter(tSIRF_VOID)
{
   static tSIRF_UINT32 byte_count;
   static tSIRF_UINT16 buf_len;
   static tSIRF_UINT8  buf[RECEIVE_BUFFER_SIZE];
   static tSIRF_INT32 result;
   static tSIRF_UINT32 payload_length;

   SIRF_PAL_OS_THREAD_START();

   /* runs a simplified SiRFBinary import/export parser while connection is open: */
   buf_len = 0;
   memset( buf, 0, sizeof(buf) );
   while ( c_running )
   {
      tSIRF_SOCKET readset[2] = {SIRF_PAL_NET_INVALID_SOCKET, SIRF_PAL_NET_INVALID_SOCKET};
      tSIRF_SOCKET excptset[2] = {SIRF_PAL_NET_INVALID_SOCKET, SIRF_PAL_NET_INVALID_SOCKET};

      /* reset input buffer if full: */
      if (buf_len==sizeof(buf))
         buf_len = 0;

      /* wait for socket: */
      readset[0] = c_tcp_sock;
      excptset[0] = c_tcp_sock;

      result = SIRF_PAL_NET_Select( readset, 0, excptset, DEFAULT_TIMEOUT );
      if ( result!=SIRF_SUCCESS || !c_running )
      {
         continue; /* timed out or error */
      }

      /* socket reads: */
      if( excptset[0] != SIRF_PAL_NET_INVALID_SOCKET )
      {
         printf("%s: exception on socket\n", __FUNCTION__);
         break;
      }

      if ( readset[0] != SIRF_PAL_NET_INVALID_SOCKET )
      {
         result = SIRF_PAL_NET_Read( c_tcp_sock, buf+buf_len, sizeof(buf)-buf_len, &byte_count );
         if ( result==SIRF_PAL_NET_CONNECTION_CLOSED )
         {
            break; /* while c_running */
         }
         if ( result!=SIRF_SUCCESS )
         {
            DEBUGMSG(1, (DEBUGTXT("tcpip: read error, result=%d"), result));
            continue; /* while c_running */
         }

         buf_len = buf_len + (tSIRF_UINT16)byte_count;

         /* parse all messages received in whole: */
         while ( buf_len >= 8 )
         {
            payload_length = ((tSIRF_UINT32)buf[2]<<8) | buf[3];
            if ( payload_length+8 > buf_len )
               break; /* insufficient bytes in buffer */

            /* check for errors: */
            if (  buf[0]!=SIRF_MSG_HEAD0 || buf[1]!=SIRF_MSG_HEAD1
               || buf[payload_length+6]!=SIRF_MSG_TAIL0 || buf[payload_length+7]!=SIRF_MSG_TAIL1 )
            {
               buf_len = 0;
               break;
            }

            /* Call the data callback if it is available to parse the data */
            if ( f_callback )
            {
               f_callback( buf, payload_length + 8);
            }

            /* Not currently used, but should be? */
            c_anything_received = SIRF_TRUE;

            /* shift remaining bytes: */
            memmove( buf, buf+payload_length+8, buf_len-(payload_length+8) );
            buf_len = buf_len - (tSIRF_UINT16)(payload_length+8);
            if (buf_len>sizeof(buf))
               buf_len = 0;
         }
      }
   } /* repeat until connection closed or module exiting */
}


/** 
 * Thread that creates a connection and runs the reader writer.
 * 
 * @param SIRF_PAL_OS_THREAD_PARAMS OS specific parameters.
 * 
 * @return OS specific.
 */
static SIRF_PAL_OS_THREAD_DECL TCPConnect( SIRF_PAL_OS_THREAD_PARAMS )
{
   tSIRF_INT16 reconnect = 1;
   tSIRF_RESULT result    = 0;

   /* Unused Parameters. */
   SIRF_PAL_OS_THREAD_UNUSED
   
   SIRF_PAL_OS_THREAD_START();

   do
   {
      tSIRF_SOCKET writeset[2] = {SIRF_PAL_NET_INVALID_SOCKET, SIRF_PAL_NET_INVALID_SOCKET};
      /* create socket and initiate connection: */
      result = SIRF_PAL_NET_CreateSocket(&c_tcp_sock, SIRF_PAL_NET_SOCKET_TYPE_DEFAULT);
      DEBUGCHK(result==0);

      DEBUGMSG(1, (DEBUGTXT("tcpip: connecting...\n")));
      result = SIRF_PAL_NET_Connect( c_tcp_sock, c_tcpip_addr, c_tcpip_port, SIRF_PAL_NET_SECURITY_NONE );

      writeset[0] = c_tcp_sock;
      result |= SIRF_PAL_NET_Select( NULL, writeset, NULL, CONNECT_TIMEOUT );

      /* if connection established, do read/write: */
      if ( result==SIRF_SUCCESS )
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: connection established\n")));

         TCPReaderWriter();

         #ifdef EPH_AIDING_DEMO
            reconnect = 0;
         #endif
      }
      else
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: unable to connect; result=%d; retrying in 1 second\n"), result));
         SIRF_PAL_OS_THREAD_Sleep(DEFAULT_TIMEOUT);
      }

      /* close socket and release memory: */
      SIRF_PAL_NET_CloseSocket(c_tcp_sock);
      c_tcp_sock = SIRF_PAL_NET_INVALID_SOCKET;

   } while ( c_running && reconnect );

   SIRF_PAL_OS_THREAD_RETURN();
}

/** 
 * TCP Listener thread
 * 
 * @param SIRF_PAL_OS_THREAD_PARAMS OS Specific
 * 
 * @return OS specific
 */
static SIRF_PAL_OS_THREAD_DECL TCPListener( SIRF_PAL_OS_THREAD_PARAMS )
{
   tSIRF_SOCKET    listener;
   tSIRF_UINT32    result;
   tSIRF_SOCKADDR  name, incoming;

   /* Unused Parameters. */
   SIRF_PAL_OS_THREAD_UNUSED
   
   SIRF_PAL_OS_THREAD_START();

   result = SIRF_PAL_NET_CreateSocket(&listener, SIRF_PAL_NET_SOCKET_TYPE_DEFAULT);
   DEBUGCHK( result==SIRF_SUCCESS && listener );

   memset(&name, 0, sizeof(name));
   #ifdef _ENDIAN_BIG
      name.sin_port = c_tcpip_port;
   #else
      name.sin_port = c_tcpip_port>>8 | c_tcpip_port<<8;
   #endif

   do
   {
      result = SIRF_PAL_NET_Bind(listener, &name);
      if ( result==SIRF_SUCCESS )
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: bound to port %d...\n"), c_tcpip_port));
      }
      else
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: error binding to port %d, result=0x%04ld...\n"), c_tcpip_port, result));
         SIRF_PAL_OS_THREAD_Sleep(DEFAULT_TIMEOUT);
      }
   } while ( c_running && result!=SIRF_SUCCESS );

   result = SIRF_PAL_NET_Listen(listener);
   if (result!=0)
   {
      DEBUGMSG(1, (DEBUGTXT("tcpip: error listening\n")));
      SIRF_PAL_OS_THREAD_Sleep(DEFAULT_TIMEOUT);
      SIRF_PAL_OS_THREAD_RETURN();
   }

   while (c_running)
   {
      memset(&incoming, 0, sizeof(incoming));
      result = SIRF_PAL_NET_Accept(listener, &c_tcp_sock, &incoming, SIRF_PAL_NET_SECURITY_NONE);
      if (result!=0)
      {
         DEBUGMSG(1, (DEBUGTXT("tcpip: unable to accept; result=%d; continuing listening\n"), result));
         SIRF_PAL_OS_THREAD_Sleep(DEFAULT_TIMEOUT);
         continue;
      }

      DEBUGMSG(1, (DEBUGTXT("tcpip: connection established\n")));

      TCPReaderWriter();

      SIRF_PAL_NET_CloseSocket(c_tcp_sock);
      c_tcp_sock = SIRF_PAL_NET_INVALID_SOCKET;
   }

   /* shutdown and close listener sockets, preventing any further connections: */
   SIRF_PAL_NET_CloseSocket(listener);
   DEBUGMSG(1, (DEBUGTXT("tcpip: listener closed\n")));

   /* close socket and release memory: */
   if (SIRF_PAL_NET_INVALID_SOCKET != c_tcp_sock)
      SIRF_PAL_NET_CloseSocket(c_tcp_sock);

   SIRF_PAL_OS_THREAD_RETURN();
}


/** 
 * Creates a thread to connect to a tcpip address or to listen to a connection
 * 
 * @param tcpip_addr tcpip address, If null creates a listening thread
 * @param tcpip_port tcpip port
 * 
 * @return SIRF_SUCCESS if the threads to do the request action is successful
 *         thread error code otherwise.
 */
tSIRF_RESULT SIRF_EXT_TCPIP_Create( tSIRF_CHAR *tcpip_addr, tSIRF_UINT16 tcpip_port )
{
   tSIRF_RESULT ret = 0;

   if (c_running)
      return SIRF_FAILURE;

   c_tcp_sock = SIRF_PAL_NET_INVALID_SOCKET;
   c_anything_received = SIRF_FALSE;
   c_nsent = c_lasterr = 0;

   c_tcpip_port = tcpip_port;
   c_running = SIRF_TRUE;

   if (tcpip_addr)
   {
      strlcpy(c_tcpip_addr, tcpip_addr, sizeof(c_tcpip_addr));
      ret |= SIRF_PAL_OS_THREAD_Create(SIRFNAV_DEMO_THREAD_EXT1, (tSIRF_HANDLE)TCPConnect, &c_tcp_connect_handle);
   }
   else
   {
      ret |= SIRF_PAL_OS_THREAD_Create(SIRFNAV_DEMO_THREAD_EXT1, (tSIRF_HANDLE)TCPListener, &c_tcp_connect_handle);
   }

   return ret;
}

/** 
 * Shuts down the EXT tcp threads and releases all resources allocated.
 * 
 * @return SIRF_SUCCESS if successful
 */
tSIRF_RESULT SIRF_EXT_TCPIP_Delete(tSIRF_VOID)
{
   tSIRF_RESULT ret = SIRF_SUCCESS;

   if (!c_running)
      return SIRF_FAILURE;

   /* signal shutdown: */
   c_running = SIRF_FALSE;

   /* stop threads: */
   if(c_tcp_connect_handle != NULL)
   {
      ret = SIRF_PAL_OS_THREAD_Delete(c_tcp_connect_handle);
   }
   DEBUGMSG(1, (DEBUGTXT("tcpip: connection closed\n")));

   return ret;
}


/** 
 * Validates that anything has been received on the connection.
 * 
 * @return SIRF_SUCCESS if any data has ever been received.
 */
tSIRF_RESULT SIRF_EXT_TCPIP_VerifyConnection(tSIRF_VOID)
{
   return c_anything_received ? SIRF_SUCCESS : SIRF_FAILURE;
}


#endif /* SIRF_EXT_TCPIP */

/**
 * @}
 */
