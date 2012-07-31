/**
 * @addtogroup gprs_at_command_server
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
 * FILENAME:  gprs_at_command_server.c
 *
 * DESCRIPTION: Gprs_At_Commandiliary serial port module
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/util/lpl/gprs_at_command_server/gprs_at_command_server.c $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  $Revision: #1 $
 *
 ***************************************************************************/

#include <string.h>
#include <limits.h>

#include "sirf_types.h"

#include "sirf_msg.h"
#include "sirf_msg_gprs_at_command.h"

#include "sirf_pal.h"

#include "sirflpl_config.h"
#include "sirf_ext.h"
#include "sirf_ext_uart.h"
#include "sirf_codec.h"
#include "sirf_proto.h"
#include "sirf_proto_mas.h"
#include "sirf_proto_gprs_at_command.h"

#include "string_sif.h"
#include "sirf_heap_sif.h"
#include "fifo_queue/fifo_queue.h"
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

#include "gprs_at_command_server.h"
#include "gprs_at_command_bearer.h"
#include "gprs_at_command_msg_handler.h"

/***************************************************************************
 * Local Data Declarations
 ***************************************************************************/

/** Fixed baud rate of the modem */
#define SIRF_GPRS_AT_COMMAND_BAUD_RATE       (115200)
/** Fixed flow control settings of the Modem */
#define SIRF_GPRS_AT_COMMAND_FLOW_CONTROL    (SIRF_FALSE)
/** Maximum number of handles that can be listening for messages */
#define SIRF_GPRS_AT_COMMAND_MAX_HANDLES     (4)
/** Size of the message queue heap */
#define SIRF_GPRS_MSG_QUEUE_HEAP_SIZE (15 * 1024)
/** Maximum number of MS to wait for a response from the GPRS modem */
#define DEFAULT_GPRS_RESPONSE_TIMEOUT (5000)
/** How long to wait before before closing the data stream.
 * This was discovered by trial and error.  1 Second wasn't enough, 5 was about 
 * right */
#define GPRS_DATA_TIMEOUT (5000)

/** structure for each individual listener */
typedef struct
{
   tSIRF_EXT_MSG_CALLBACK msg_callback; /**< Callback function for this 
                                         * listener */
   
   tSIRF_UINT32           data_index; /**< Modem index for the data channel */
   
   fifo_queue_t           msg_queue_mas;      /**< TCP Data messages */
   fifo_queue_t           msg_queue_mas_input;/**< TCP Input data messages */
} gprs_listener_t;

 
/**
 * Structure for internal AT Command server management 
 */
typedef struct
{
   /* OS resources */
   tSIRF_UINT32        uartno;         /**< Uart Number for sirf_ext_uart */
   tSIRF_SEMAPHORE     semaphore;      /**< Signals arrival of a messages */
   tSIRF_THREAD        server_thread;  /**< Thread thand handle the processing
                                            of messages */
   /* Server state variables */
   gprs_server_state_t state;
   tSIRF_GPRS_HANDLE   data_handle; /* Handle of currently active TCP/IP 
                                      * connection when the state is TCP Data */
   volatile tSIRF_BOOL thread_running; /**< Loop variable */

   /* Message queues and heap */
   tSIRF_MUTEX         queue_mx;           /**< Mutual exclusion handle for
                                             * the listeners */
   tHeap               msg_heap;           /**< Heap for messages */
   fifo_queue_t        msg_queue_gprs;     /**< Messages from the GPRS modem */
   fifo_queue_t        msg_queue_gprs_input;/**< Messages from the listeners */
   tSIRF_UINT32        semaphore_timeout;  /**< Amount of time to wait for the
                                            * next message to arrive */
   tSIRF_UINT32        command_timeout;    /**< Total amount of time to wait for
                                            * each message in a command */

   /** The listener queue */
   tSIRF_MUTEX         listener_mx;         /**< Mutual exclusion handle for
                                             * the listeners */
   gprs_listener_t     listener[ SIRF_GPRS_AT_COMMAND_MAX_HANDLES ];

   /* Values to help handle command responses */
   tSIRF_GPRS_HANDLE   command_handle;/**< Handle to the listern that issued 
                                       * this command */
   tSIRF_UINT32        command_timeout_left; /**< Time util giving up waiting
                                              * for responses */
   tSIRF_UINT32        command_msg; /**< Subject of the message */
   tSIRF_UINT32        command_data; /**< Used to store misc important info */
   tSIRF_UINT32        command_responses; /**< Count of the number of responses 
                                           * so far */

} gprs_at_command_t;

/** Function pointer for local message handler functions */
typedef tSIRF_RESULT (*tGPRS_MSG_HANDLER)(
   gprs_msg_t   const * const msg, 
   tSIRF_UINT32       * const timeout);

/*----------------------------------------------------------------------------*/
/* Global Data */
/*----------------------------------------------------------------------------*/
/** One and only one global connection to the GPRS modem */
static gprs_at_command_t gprs_at_command; /* Put in ZI space */
/** Memory for the messages, with 64 bit alignment */
static tSIRF_UINT64 gprs_heap_space[SIRF_GPRS_MSG_QUEUE_HEAP_SIZE 
                                    / sizeof(tSIRF_UINT64)];
/*----------------------------------------------------------------------------*/
/* Static function prototypes */
/*----------------------------------------------------------------------------*/
static tSIRF_VOID handle_unexpected_result(tSIRF_UINT32 result);
static tSIRF_RESULT gprs_switch_from_data_mode(tSIRF_VOID);

/* While these functions could take const packet, the API that calls them
 * doesn't care if the underlying function modifies the contents */
static tSIRF_RESULT SIRF_GPRS_AT_COMMAND_UartPacketCallbackATCommand( 
   tSIRF_UINT8  * packet, 
   tSIRF_UINT32   packet_length );
static tSIRF_RESULT SIRF_GPRS_AT_COMMAND_UartPacketCallbackMAS( 
   tSIRF_UINT8  * packet, 
   tSIRF_UINT32   packet_length );

static SIRF_PAL_OS_THREAD_DECL SIRF_GPRS_AT_COMMAND_ReadThread( SIRF_PAL_OS_THREAD_PARAMS );


/*----------------------------------------------------------------------------*/
/* Function implemenation */
/*----------------------------------------------------------------------------*/
/**
 * @brief  Registered callback function for failures
 *
 * When these errors occur, it indicates some type of critical system failure.
 *
 * @param  heap   The heap object
 * @param  err    Error found
 * @param  extra  Extra data
 *
 */
static tSIRF_VOID
GprsMsgQueueHeap_notify(tHeap heap, tHeapError err, unsigned long extra)
{
   (void)extra;
#ifndef DEBUG
   (void) heap;
   (void) err;
#endif
   DEBUGMSG(1,(DEBUGTXT("gprs msg queue heap error heap %x err %d "),
               heap, err));
   UTIL_Assert(FALSE);
}

/** 
 * Create the AT Command server instance
 * 
 * @param port_name Name of the uart port the Modem is connected
 * 
 * @return Success if the port was opened successfully
 */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Create( 
   tSIRF_CHAR const * const port_name,
   tSIRF_CHAR const * const apn )
{
   tSIRF_RESULT result;
   tSIRF_MSG_SSB_EXT_UART_OPEN_PORT port_settings;

   /* This only works initially if gprs_at_command is in ZI data */
   if (NULL != gprs_at_command.listener_mx)
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_ALREADY_CREATED;
   }

   /* This is necessar if Create is ever called a second time */
   memset( &gprs_at_command, 0, sizeof(gprs_at_command) );

   /* Initialize the queues.  These fucntion cannot fail */
   fifo_queue_init( &gprs_at_command.msg_queue_gprs );
   fifo_queue_init( &gprs_at_command.msg_queue_gprs_input );
   gprs_at_command.command_timeout = DEFAULT_GPRS_RESPONSE_TIMEOUT;

   /* Open a uart port to the modem */
   strlcpy(port_settings.port_name,port_name,sizeof(port_settings.port_name));
   port_settings.baud_rate    = SIRF_GPRS_AT_COMMAND_BAUD_RATE;
   port_settings.flow_control = SIRF_GPRS_AT_COMMAND_FLOW_CONTROL;

   /* Create the semaphore */
   result = SIRF_PAL_OS_SEMAPHORE_Create(&gprs_at_command.semaphore,0);
   if ( SIRF_SUCCESS != result )
   {
      return result;
   }

   result = SIRF_PAL_OS_MUTEX_Create( &gprs_at_command.listener_mx );
   if ( SIRF_SUCCESS != result )
   {
      goto GPRS_ERROR_1;
   }

   result = SIRF_PAL_OS_MUTEX_Create( &gprs_at_command.queue_mx );
   if ( SIRF_SUCCESS != result )
   {
      goto GPRS_ERROR_2;
   }

   gprs_at_command.msg_heap = Heap_Create( GprsMsgQueueHeap_notify,
                                           gprs_heap_space,
                                           sizeof(gprs_heap_space) );
   if (NULL == gprs_at_command.msg_heap)
   {
      goto GPRS_ERROR_3;
   }

   /* Must open the UART last since it might call the callback fuctnion which
    * uses the heap and queues mutex's and semaphores, but before the 
    * queue reading thread since the queue reading thread calls into the
    * uart */
   result = SIRF_EXT_UART_Open(&gprs_at_command.uartno,
                               &port_settings,
                               SIRF_GPRS_AT_COMMAND_UartPacketCallbackATCommand,
                               SIRF_PROTO_GPRS_AT_COMMAND_Parser);

   if ( SIRF_SUCCESS != result )
   {
      goto GPRS_ERROR_4;
   }

   gprs_at_command.thread_running = SIRF_TRUE;
   result = SIRF_PAL_OS_THREAD_Create( 
      SIRFLPL_THREAD_GPRS, 
      (tSIRF_HANDLE)SIRF_GPRS_AT_COMMAND_ReadThread, 
      &gprs_at_command.server_thread );

   if (SIRF_SUCCESS != result)
   {
      goto GPRS_ERROR_5;
   }

   result = gprs_start(gprs_at_command.uartno,
                       SIRF_GPRS_AT_COMMAND_FLOW_CONTROL,
                       apn);
   if (SIRF_SUCCESS != result)
   {
      goto GPRS_ERROR_5;
   }

   return SIRF_SUCCESS;

   

GPRS_ERROR_5:
   /* Must kill our own thread and delete it */
   gprs_at_command.thread_running = SIRF_FALSE;
   (void)SIRF_PAL_OS_SEMAPHORE_Release(gprs_at_command.semaphore);
   (void)SIRF_PAL_OS_THREAD_Delete(gprs_at_command.server_thread);

GPRS_ERROR_4:
   /* Heap_Create doesn't have a Heap Destroy */

GPRS_ERROR_3:
   (void)SIRF_PAL_OS_MUTEX_Delete(gprs_at_command.queue_mx);
   gprs_at_command.queue_mx = NULL;

GPRS_ERROR_2:
   (void)SIRF_PAL_OS_MUTEX_Delete(gprs_at_command.listener_mx);
   gprs_at_command.listener_mx = NULL;

GPRS_ERROR_1:
   (void)SIRF_PAL_OS_SEMAPHORE_Delete(gprs_at_command.semaphore);
   gprs_at_command.semaphore = NULL;
   return result;

} /* SIRF_GPRS_AT_COMMAND_Create()*/


/** 
 * Delete the AT command server instance and release all allocated resources
 * 
 * @return Any error codes that occur during deletion, SIRF_SUCCESS otherwise
 */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Delete( tSIRF_VOID )
{
   tSIRF_RESULT result;
   tSIRF_RESULT ret_val = SIRF_SUCCESS;
   tSIRF_UINT32 handle;

   if (NULL == gprs_at_command.listener_mx)
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_NOT_CREATED;
   }

   /* Just in case we were in data mode, which out.  */
   gprs_switch_from_data_mode();
   /* Now shutdown the modem */
   result = gprs_stop(gprs_at_command.uartno);
   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }

   /* Close port in case it is still open*/
   for ( handle=0; handle < SIRF_GPRS_AT_COMMAND_MAX_HANDLES; handle++ )
   {
      /* ignore the return code and close all handles */
      (void)SIRF_GPRS_AT_COMMAND_SERVER_Close( (tSIRF_GPRS_HANDLE)handle );
   }

   /* Close the thread */
   gprs_at_command.thread_running = SIRF_FALSE;
   result = SIRF_PAL_OS_SEMAPHORE_Release(gprs_at_command.semaphore);
   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }

   result = SIRF_PAL_OS_THREAD_Delete(gprs_at_command.server_thread);
   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }

   /* Close the UART */
   result = SIRF_EXT_UART_Close(gprs_at_command.uartno);
   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }

   result = SIRF_PAL_OS_MUTEX_Delete(gprs_at_command.queue_mx);
   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }
      /* Close the Mutex */
   result = SIRF_PAL_OS_MUTEX_Delete( gprs_at_command.listener_mx );
   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }
   /* Close the semaphore */
   result = SIRF_PAL_OS_SEMAPHORE_Delete(gprs_at_command.semaphore);
   if (SIRF_SUCCESS == ret_val)
   {
      ret_val = result;
   }

   /* Set the initialization flag to NULL */
   gprs_at_command.listener_mx = NULL;
   gprs_at_command.queue_mx = NULL;

   return result;

} /* SIRF_GPRS_AT_COMMAND_Delete()*/

/** 
 * Get and initialize the next avaiable listern object.
 * 
 * @param handle   [out] Handle value of a found listener
 * @param callback [in] Function pointer to initialize the listener object 
 * 
 * @return SIRF_SUCCESS if a listener is allocated, and 
 *         SIRF_GPRS_AT_COMMAND_NO_MORE_HANDLES_AVAILABLE otherwise
 */
static tSIRF_RESULT GPRS_AT_COMMAND_SetNextAvailableListener(
   tSIRF_UINT32           * const handle,
   tSIRF_EXT_MSG_CALLBACK         callback)
{
   /* Assign default callback*/
   tSIRF_UINT32 ii;
   tSIRF_RESULT result;

   result = SIRF_PAL_OS_MUTEX_Enter(gprs_at_command.listener_mx);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }


   for ( ii=0; ii<SIRF_GPRS_AT_COMMAND_MAX_HANDLES; ii++ )
   {
      if (NULL == gprs_at_command.listener[ii].msg_callback)
      {
         gprs_at_command.listener[ii].msg_callback = callback;
         fifo_queue_init( &gprs_at_command.listener[ii].msg_queue_mas );
         fifo_queue_init( &gprs_at_command.listener[ii].msg_queue_mas_input );
         gprs_at_command.listener[ii].data_index = 0xFFFFFFFF;
         *handle = ii;
         break;
      }
   }

   result = SIRF_PAL_OS_MUTEX_Exit(gprs_at_command.listener_mx);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   if ( SIRF_GPRS_AT_COMMAND_MAX_HANDLES > ii )
   {
      return SIRF_SUCCESS;
   }
   else 
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_NO_MORE_HANDLES_AVAILABLE;
   }
}

/** 
 * Open a handle to the AT command server
 * 
 * @param handle   [out] Handle for reference further activity on the modem
 * @param callback [in]  Call back function pointer to call when data is 
 *                       received
 * 
 * @return SIRF_SUCCESS if there are avaiable handles.
 */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Open(tSIRF_GPRS_HANDLE *handle,
                                              tSIRF_EXT_MSG_CALLBACK callback)
{
   tSIRF_RESULT result;

   if (NULL == gprs_at_command.listener_mx)
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_NOT_CREATED;
   }

   result = GPRS_AT_COMMAND_SetNextAvailableListener(
      (tSIRF_UINT32*)handle, 
      callback);
   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   return SIRF_SUCCESS;

} /* GPRS_AT_COMMAND_Open()*/


/** 
 * Close a previously opened handle
 * 
 * @param handle The handle that was previously opened
 * 
 * @return Any error code that occurs when freeing the handle
 */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Close( tSIRF_GPRS_HANDLE handle )
{
   tSIRF_RESULT result = SIRF_SUCCESS;

   if (NULL == gprs_at_command.listener_mx)
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_NOT_CREATED;
   }

   if ( ((tSIRF_UINT32)handle >= SIRF_GPRS_AT_COMMAND_MAX_HANDLES )  ||
        (NULL == gprs_at_command.listener[(tSIRF_UINT32)handle].msg_callback ))
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_INVALID_HANDLE;
   }

   /* While this may only take one instruction to complete it needs protection
    * because it is checked for NULL before being called. */
   result = SIRF_PAL_OS_MUTEX_Enter(gprs_at_command.listener_mx);
   if (SIRF_SUCCESS != result) 
   {
      return result;
   }
   gprs_at_command.listener[(tSIRF_UINT32)handle].msg_callback = NULL;
   result = SIRF_PAL_OS_MUTEX_Exit(gprs_at_command.listener_mx);

   return result;

} /* GPRS_AT_COMMAND_Close()*/

/** 
 * Send a message to the AT Command server by placing the message on a Queue
 * 
 * A more efficient set of methods should be used in place of this function
 * @todo yet to be implemetned.  Alloc message for sending, and send alloced 
 * message.
 * 
 * @param handle             Handle previously opened
 * @param message_id         Subject of the message
 * @param message_structure  Contents of the message
 * @param message_length     Size of the message
 * 
 * @return Error code if one occured or SIRF_SUCCESS.
 */

tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_Send(   
   tSIRF_GPRS_HANDLE               handle,
   tSIRF_UINT32                    message_id,
   tSIRF_VOID        const * const message_structure,
   tSIRF_UINT32                    message_length )
{
   tSIRF_RESULT result;
   gprs_msg_t * msg;

   if (NULL == gprs_at_command.listener_mx)
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_NOT_CREATED;
   }

   if ( ((tSIRF_UINT32)handle >= SIRF_GPRS_AT_COMMAND_MAX_HANDLES )  ||
        (NULL == gprs_at_command.listener[(tSIRF_UINT32)handle].msg_callback ))
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_INVALID_HANDLE;
   }

   result = SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(&msg, message_length );

   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   msg->handle            = handle;
   msg->message_id        = message_id;
   msg->message_length    = message_length;

   /* To avoid this memcpy use MsgMalloc and MsgSend directly */
   memcpy(msg->message_structure,message_structure,message_length);

   return SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(msg);

} /* SIRF_GPRS_AT_COMMAND_Send() */


/** 
 * Malloc off of the Server Heap memory for sending a message
 * 
 * @param msg            Pointer to Malloced message
 * @param message_length Length of memory to Malloc
 * 
 * @return SIRF_SUCCESS or SIRF_GPRS_AT_COMMAND_SERVER_MEMORY_ERROR
 */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_MsgMalloc(
   gprs_msg_t               **     msg,
   tSIRF_UINT32                    message_length )
{
   fifo_link_t     * link;

   if (NULL == gprs_at_command.listener_mx)
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_NOT_CREATED;
   }

   /* Post the data on the message queue to be handled by the GPRS thread */
   link = (fifo_link_t*)Heap_Malloc(gprs_at_command.msg_heap,
                               sizeof(fifo_link_t) + 
                               sizeof(gprs_msg_t) + 
                               message_length);
   
   if (NULL == link)
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_MEMORY_ERROR;
   }

   *msg                      = (gprs_msg_t*)((tSIRF_UINT32)link + sizeof(fifo_link_t));
   link->data                = (void*)*msg;
   (*msg)->message_structure = (void*)((tSIRF_UINT32)*msg + sizeof(gprs_msg_t));
   (*msg)->message_length = message_length;
   return SIRF_SUCCESS;
}

/** 
 * Send a message previously allocated
 * 
 * @param msg The message to send.  Must have been previously allocated by 
 *            MsgMalloc
 * 
 * @return SIRF_SUCCESS if the message or specific error code.
 */
tSIRF_RESULT SIRF_GPRS_AT_COMMAND_SERVER_MsgSend(
   gprs_msg_t           const * const msg)
{
   tSIRF_RESULT result;
   fifo_link_t * link;
   
   UTIL_Assert(NULL != msg);

   link = (fifo_link_t*)((tSIRF_UINT32)msg - sizeof(fifo_link_t));

   if ( ((tSIRF_UINT32)msg->handle >= SIRF_GPRS_AT_COMMAND_MAX_HANDLES )  ||
        (NULL == gprs_at_command.listener[(tSIRF_UINT32)msg->handle].msg_callback ))
   {
      Heap_Free(gprs_at_command.msg_heap,link);
      return SIRF_GPRS_AT_COMMAND_SERVER_INVALID_HANDLE;
   }

   /* Check to see if we need to send a wipdata message,
    * Only do this if this is a MAS message and there isn't a data connection
    * already open for this handle */
   if ((SIRF_LC_MAS == SIRF_GET_LC(msg->message_id)) &&
       /* MAS Message.  If we aren't in data mode send a message */
       ((GPRS_SERVER_STATE_TCP_IP_DATA != gprs_at_command.state) ||
        /* If we are in data mode, but opened via a differen handle, send wipdata */
       ((GPRS_SERVER_STATE_TCP_IP_DATA == gprs_at_command.state) &&
        (msg->handle != gprs_at_command.data_handle)))
      )
   {
      gprs_at_command.data_handle = msg->handle;
      result = gprs_send_tcpip_wipdata(gprs_at_command.listener[(tSIRF_UINT32)msg->handle].data_index);
      if (SIRF_SUCCESS != result)
      {
         Heap_Free(gprs_at_command.msg_heap,link);
         return result;
      }
   }

   result = SIRF_PAL_OS_MUTEX_Enter( gprs_at_command.queue_mx );
   if (SIRF_SUCCESS != result) 
   {
      Heap_Free(gprs_at_command.msg_heap,link);
      return result;
   }

   /* Add it to the appripriate message queue */
   if (SIRF_LC_MAS == SIRF_GET_LC(msg->message_id))
   {
      fifo_queue_add_tail(&gprs_at_command.listener[(tSIRF_UINT32)msg->handle].msg_queue_mas_input, link);
   }
   else
   {
      fifo_queue_add_tail(&gprs_at_command.msg_queue_gprs_input, link);
   }

   result = SIRF_PAL_OS_MUTEX_Exit( gprs_at_command.queue_mx );
   if (SIRF_SUCCESS != result)
   {
      return result;
   }
   
   /* Signal that a new message has arrived */
   result = SIRF_PAL_OS_SEMAPHORE_Release( gprs_at_command.semaphore );

   return result;

} /* SIRF_GPRS_AT_COMMAND_Send() */


static tSIRF_RESULT gprs_switch_from_data_mode(tSIRF_VOID)
{
   tSIRF_RESULT result;

   result = gprs_send_close_data(gprs_at_command.uartno);
   if (SIRF_SUCCESS == result)
   {
      result = SIRF_EXT_UART_SetCallbacks(gprs_at_command.uartno,
                                          SIRF_GPRS_AT_COMMAND_UartPacketCallbackATCommand,
                                          SIRF_PROTO_GPRS_AT_COMMAND_Parser);

      gprs_at_command.state = GPRS_SERVER_STATE_AT_COMMAND;
   }
   return result;
}

static tSIRF_VOID handle_unexpected_result(tSIRF_UINT32 result)
{
   tSIRF_UINT32 handle;
   /* Send up a message to the listener that sent the command a timeout
    * error message */
   tSIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR err;
   memset(&err,0,sizeof(err));
   err.result = result;
   /* Ignore the return code when handling a different error case */
   (void) SIRF_PAL_OS_MUTEX_Enter(gprs_at_command.listener_mx);
   for (handle = 0; handle < SIRF_GPRS_AT_COMMAND_MAX_HANDLES; handle++)
   {
      if (gprs_at_command.listener[handle].msg_callback)
      {
         gprs_at_command.listener[handle].msg_callback(
            SIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR,
            &err,
            sizeof(err));
      }
   }
   (void) SIRF_PAL_OS_MUTEX_Exit(gprs_at_command.listener_mx);
}

static tSIRF_VOID handle_response_timeout(tSIRF_VOID)
{
   /* Send up a message to the listener that sent the command a timeout
    * error message */
   tSIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR err;
   memset(&err,0,sizeof(err));
   err.result = SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_TIMEOUT;
   err.data[0] = gprs_at_command.command_msg;
   err.data[1] = gprs_at_command.command_timeout;
   err.data[2] = gprs_at_command.command_responses;
   /* Ignore the return code when handling a different error case */
   (void) SIRF_PAL_OS_MUTEX_Enter(gprs_at_command.listener_mx);
   /* Make sure the handle didn't get closed while waiting for the response */
   if (gprs_at_command.listener[(tSIRF_UINT32)gprs_at_command.command_handle].msg_callback)
   {
      gprs_at_command.listener[(tSIRF_UINT32)gprs_at_command.command_handle].msg_callback(
         SIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR,
         &err,
         sizeof(err));
   }
   /* Change the state to handle the next command */
   gprs_at_command.state = GPRS_SERVER_STATE_AT_COMMAND;
   (void) SIRF_PAL_OS_MUTEX_Exit(gprs_at_command.listener_mx);

}

/** 
 * Callback to process MAS data packets
 * 
 * @param packet Packet to process
 * @param packet_length Size of the packet
 * 
 * @return SIRF_SUCCESS if successfully processed
 */
static tSIRF_RESULT SIRF_GPRS_AT_COMMAND_UartPacketCallbackMAS(
   tSIRF_UINT8* packet, tSIRF_UINT32 packet_length )
{
   tSIRF_UINT32 ii;
   tSIRF_UINT32 message_length;
   tSIRF_UINT32 message_id;
   fifo_link_t *link;
   gprs_msg_t *msg;
   tSIRF_RESULT result;
   tSIRF_UINT32 options;

   /* Search for the ETX character, and if found switch protocols immediately
    * so that no data is lost */
   for (ii = 0; ii < packet_length; ii++)
   {
      if (SIRF_PROTO_MAS_ETX == packet[ii])
      {
         /* make sure it wasn't escaped */
         if (ii == 0 || SIRF_PROTO_MAS_DLE != packet[ii-1])
         {
            SIRF_EXT_UART_SetCallbacksFromCallback(gprs_at_command.uartno,
                                                SIRF_GPRS_AT_COMMAND_UartPacketCallbackATCommand,
                                                SIRF_PROTO_GPRS_AT_COMMAND_Parser);
            /* Potential yet unlikely race condition here where we could be in the
            * middle of processing a MAS message (input or output) and ETX has come in.
            * This is OK and normal.  Sending <pause> +++ <pause> while in AT command 
            * mode will cause no harm since the modem is looking for AT<command><CR>.
            * We set the state here to try to reduce the likely hood of sending unnecessary 
            * characeters.  Also release the semaphore so the thread wakes up and
            * sets the timeout appropriately */
            gprs_at_command.state = GPRS_SERVER_STATE_AT_COMMAND;
            SIRF_PAL_OS_SEMAPHORE_Release(gprs_at_command.semaphore);            
            break;
         }
      }
   }

   /* Put the rest of the data on the queue.  If there is none, just return */
   if (0 == ii)
   {
      return SIRF_SUCCESS;
   }

   /* Find out how much memeory we need */
   result = SIRF_PROTO_MAS_Decode(packet,
                                  ii,
                                  &message_id,
                                  NULL,
                                  &message_length,
                                  &options);


   link = (fifo_link_t*) Heap_Malloc(gprs_at_command.msg_heap,
                                     sizeof(fifo_link_t) +
                                     sizeof(gprs_msg_t) + 
                                     message_length);

   if (NULL == link) 
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_MEMORY_ERROR;
   }

   msg = (gprs_msg_t*)((tSIRF_UINT32)link + sizeof(fifo_link_t));
   link->data = (void*)msg;
   msg->message_structure = (void*)((tSIRF_UINT32)msg + sizeof(gprs_msg_t));

   options = SIRF_CODEC_OPTIONS_GET_FIRST_MSG; /* First message in the packet */

   /* actually decode the message */
   result = SIRF_PROTO_MAS_Decode(packet,
                                  ii,
                                  &msg->message_id,
                                  msg->message_structure,
                                  &msg->message_length,
                                  &options);

   if (SIRF_SUCCESS != result) 
   {
      return result;
   }

   msg->handle = gprs_at_command.data_handle;

   result = SIRF_PAL_OS_MUTEX_Enter( gprs_at_command.queue_mx );
   if (SIRF_SUCCESS != result) 
   {
      return result;
   }

   fifo_queue_add_tail(&gprs_at_command.listener[(tSIRF_UINT32)gprs_at_command.data_handle].msg_queue_mas,
                         link);

   result = SIRF_PAL_OS_MUTEX_Exit( gprs_at_command.queue_mx );
   if (SIRF_SUCCESS != result) 
   {
      return result;
   }

   /* Signal a new message has arrived */
   result = SIRF_PAL_OS_SEMAPHORE_Release( gprs_at_command.semaphore );
   
   return result;

}

/** 
 * Packet callback called by the sirf_ext_uart when a full packet has arrived
 * 
 * @param packet        pointer to the packet
 * @param packet_length The length of the packet
 * 
 * @return SIRF_SUCCESS if the packet was successfully processed, other error
 *         code otherwise.
 */
static tSIRF_RESULT 
SIRF_GPRS_AT_COMMAND_UartPacketCallbackATCommand( 
   tSIRF_UINT8  *packet, 
   tSIRF_UINT32  packet_length )
{
   tSIRF_UINT32 message_id;
   tSIRF_UINT32 message_length;
   tSIRF_UINT32 options;
   tSIRF_RESULT result;
   fifo_link_t *link;
   gprs_msg_t  *msg;

   options = SIRF_CODEC_OPTIONS_GET_FIRST_MSG; /* First message in the packet */
   result = SIRF_PROTO_GPRS_AT_COMMAND_Decode(packet,
                                              packet_length,
                                              &message_id,
                                              NULL, /* return just the length */
                                              &message_length,
                                              &options);

   if (SIRF_SUCCESS != result)
   {
      return result;
   }

   UTIL_Assert(0 != message_length);

   link = (fifo_link_t*) Heap_Malloc(gprs_at_command.msg_heap,
                                     sizeof(fifo_link_t) +
                                     sizeof(gprs_msg_t) + 
                                     message_length);

   if (NULL == link) 
   {
      return SIRF_GPRS_AT_COMMAND_SERVER_MEMORY_ERROR;
   }

   msg = (gprs_msg_t*)((tSIRF_UINT32)link + sizeof(fifo_link_t));
   link->data = (void*)msg;
   msg->message_structure = (void*)((tSIRF_UINT32)msg + sizeof(gprs_msg_t));

   /* actually decode the message */
   result = SIRF_PROTO_GPRS_AT_COMMAND_Decode(packet,
                                              packet_length,
                                              &msg->message_id,
                                              msg->message_structure,
                                              &msg->message_length,
                                              &options);

   if (SIRF_SUCCESS != result) 
   {
      return result;
   }

   /* Special case here that we have to switch protocols if the CONNECT message
    * comes in */
   if (SIRF_MSG_GPRS_AT_COMMAND_CONNECT == msg->message_id)
   {
      SIRF_EXT_UART_SetCallbacksFromCallback(gprs_at_command.uartno,
                                             SIRF_GPRS_AT_COMMAND_UartPacketCallbackMAS,
                                             SIRF_PROTO_MAS_Parser);
   }

   result = SIRF_PAL_OS_MUTEX_Enter( gprs_at_command.queue_mx );
   if (SIRF_SUCCESS != result) 
   {
      return result;
   }
   fifo_queue_add_tail(&gprs_at_command.msg_queue_gprs,
                       link);

   result = SIRF_PAL_OS_MUTEX_Exit( gprs_at_command.queue_mx );
   if (SIRF_SUCCESS != result) 
   {
      return result;
   }

   /* Signal a new message has arrived */
   result = SIRF_PAL_OS_SEMAPHORE_Release( gprs_at_command.semaphore );
   
   return result;
}

/** 
 * Handle a message to the GPRS modem
 * 
 * @param msg The message to the GPRS modem
 * 
 * @return SIRF_SUCCESS if successfully handled.
 */
tSIRF_RESULT handle_mas_msg(gprs_msg_t   const * const msg,
                            tSIRF_UINT32       * const timeout)
{
   tSIRF_RESULT result = SIRF_FAILURE;
   *timeout = 0;
   if (gprs_at_command.listener[(tSIRF_UINT32)msg->handle].msg_callback)
   {
      result = gprs_at_command.listener[(tSIRF_UINT32)msg->handle].msg_callback(
         msg->message_id,
         msg->message_structure,
         msg->message_length);
   }
   return result;
}

/** 
 * Handle a message to the GPRS modem
 * 
 * @param msg The message to the GPRS modem
 * 
 * @return SIRF_SUCCESS if successfully handled.
 */
tSIRF_RESULT handle_mas_input_msg(gprs_msg_t   const * const msg,
                                  tSIRF_UINT32       * const timeout)
{
   tSIRF_RESULT result;
   tSIRF_UINT8 packet[SIRF_PROTO_GPRS_AT_COMMAND_MAX_MSG_LENGTH];
   tSIRF_UINT32 packet_length = sizeof(packet);
   tSIRF_UINT32 options = SIRF_CODEC_OPTIONS_GET_FIRST_MSG;

   /* handled a message */
   *timeout = 0;

   /* we need to be in data mode.  If the GPRS bearer is not open and the tcp connection is
    * not already established send back an error message */
   if (GPRS_SERVER_STATE_TCP_IP_DATA == gprs_at_command.state)
   {
      result = SIRF_PROTO_MAS_Encode(msg->message_id,
                                     msg->message_structure,
                                     msg->message_length,
                                     packet,
                                     &packet_length,
                                     &options);
      if (SIRF_SUCCESS == result)
      {
         result = SIRF_EXT_UART_Send(gprs_at_command.uartno,
                                     packet,
                                     packet_length);
      }
   }
   else
   {
      /* +PWIPDATA message is going to fail, send error to client */
      tSIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR err;
      tSIRF_UINT32 size;

      memset(&err,0,sizeof(err));
      err.result = SIRF_GPRS_AT_COMMAND_SERVER_SEND_ERROR;
      err.data[0] = msg->message_id;
      err.data[1] = msg->message_length;
      size = SIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR_MAX_DATA_SIZE > msg->message_length 
         ? msg->message_length 
         : SIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR_MAX_DATA_SIZE;
      memcpy(&err.data[2],msg->message_structure,size);
      /* Ignore the return code when handling a different error case */
      (void) SIRF_PAL_OS_MUTEX_Enter(gprs_at_command.listener_mx);
         
      if (gprs_at_command.listener[(tSIRF_UINT32)msg->handle].msg_callback)
      {
         gprs_at_command.listener[(tSIRF_UINT32)msg->handle].msg_callback(
            SIRF_MSG_GPRS_AT_COMMAND_SERVER_ERROR,
            &err,
            sizeof(err));
      }

      (void) SIRF_PAL_OS_MUTEX_Exit(gprs_at_command.listener_mx);
      /* The error has been handled.  Setting success here causes us to
       * not do an unexpected response handler */
      result = SIRF_SUCCESS;
   }
   return result;
}

/** 
 * Handle a message from the GPRS modem
 * 
 * @param msg The message from the GPRS modem
 * 
 * @return SIRF_SUCCESS if successfully handled.
 */
tSIRF_RESULT handle_gprs_msg(gprs_msg_t   const * const msg,
                             tSIRF_UINT32       * const timeout)
{
   tSIRF_RESULT result;
   tSIRF_UINT32 ii;

   /* Timeout is 0 unless we handle a message that requires another response */
   *timeout = 0;

   result = SIRF_PAL_OS_MUTEX_Enter( gprs_at_command.listener_mx );
   if (SIRF_SUCCESS != result) 
   {
      return result;
   }

   /* First handle unsolicited messages  */
   if (0 == gprs_at_command.command_msg){
      for (ii = 0; ii < SIRF_GPRS_AT_COMMAND_MAX_HANDLES; ii++)
      {
         if (NULL != gprs_at_command.listener[ii].msg_callback)
         {
            (void)gprs_at_command.listener[ii].msg_callback(
               msg->message_id,
               msg->message_structure,
               msg->message_length);
         }
      }
   } 
   else
   {
      gprs_server_state_t state;
      gprs_at_command.command_responses++;
      /* this was a solicited message: handle each message individually */
      switch (gprs_at_command.command_msg)
      {
      case SIRF_MSG_GPRS_AT_COMMAND_PCGMI:
      {
         result = handle_response_SIRF_MSG_GPRS_AT_COMMAND_PCGMI(
            msg,
            gprs_at_command.command_responses,
            &state);
      }
      break;
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPDATA:
      {
         result = handle_response_SIRF_MSG_GPRS_AT_COMMAND_PWIPDATA(
            msg,
            gprs_at_command.command_responses,
            &state);
      }
      break;
      case SIRF_MSG_GPRS_AT_COMMAND_PCCED:
         result = handle_response_SIRF_MSG_GPRS_AT_COMMAND_PCCED(
            msg,
            gprs_at_command.command_responses,
            gprs_at_command.command_data,
            &state);
         break;
      /* Single OK message expected for these ones */
      case SIRF_MSG_GPRS_AT_COMMAND_PCMEE:
      case SIRF_MSG_GPRS_AT_COMMAND_PIFC:
      case SIRF_MSG_GPRS_AT_COMMAND_PWOPEN:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCFG_STOP_TCPIP:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCFG_START_TCPIP:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCFG_CONFIGURE_TCPIP:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_CLOSE:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_OPEN:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_SET:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_START:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_STOP:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_CFG_MANAGEMENT:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_UDP:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_TCP_CLIENT:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_TCP_SERVER:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_FTP:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_HTTP_CLIENT:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_SMTP_CLIENT:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_POP3_CLIENT:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCLOSE:
         result = handle_response_single_SIRF_MSG_GPRS_AT_COMMAND_OK_expected(
            msg,
            gprs_at_command.command_responses,
            &state);
         break;
      case SIRF_MSG_GPRS_AT_COMMAND_PCGMM:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCFG_TCPIP_VERSION:
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPCFG_STORE_TCPIP_CFG: 
      case SIRF_MSG_GPRS_AT_COMMAND_PWIPBR_GET:
      default:
         state = GPRS_SERVER_STATE_AT_COMMAND;
         result = SIRF_GPRS_AT_COMMAND_SERVER_COMMAND_NOT_SUPPORTED;
      };

      if (NULL != gprs_at_command.listener[(tSIRF_UINT32)gprs_at_command.command_handle].msg_callback)
      {
         /* Pass the message up to the registered listener */
         (void)gprs_at_command.listener[(tSIRF_UINT32)gprs_at_command.command_handle].msg_callback(
            msg->message_id,
            msg->message_structure,
            msg->message_length);
      }

      /* If the message is successfully handled then reset the state back 
       * to ready to hanlde another command */
      if (SIRF_GPRS_AT_COMMAND_SERVER_RESPONSE_FORWARD == result)
      {
         /* more responses expected for this message */
         *timeout = gprs_at_command.command_timeout;
      }
      else
      {
         /* Reset the states in preperation for the next message */
         gprs_at_command.state = state;
         gprs_at_command.command_handle     = 0;
         gprs_at_command.command_msg        = 0;
         gprs_at_command.command_responses  = 0;
      }
      
      /* Special handling to transition to data mode if the state changes */
      if (GPRS_SERVER_STATE_TCP_IP_DATA == gprs_at_command.state)
      {
         result = SIRF_EXT_UART_SetCallbacks(
            gprs_at_command.uartno,
            SIRF_GPRS_AT_COMMAND_UartPacketCallbackMAS,
            SIRF_PROTO_MAS_Parser);
         if (SIRF_SUCCESS != result)
         {
            gprs_at_command.state = GPRS_SERVER_STATE_AT_COMMAND;
            handle_unexpected_result(result);
         }
      }
   }
   result = SIRF_PAL_OS_MUTEX_Exit( gprs_at_command.listener_mx );
   
   return result;
}

/** 
 * Handle a message to the GPRS modem
 * 
 * @param msg The message to the GPRS modem
 * 
 * @return SIRF_SUCCESS if successfully handled.
 */
tSIRF_RESULT handle_input_msg(gprs_msg_t   const * const msg,
                              tSIRF_UINT32       * const timeout)
{
   tSIRF_UINT8 packet[SIRF_PROTO_GPRS_AT_COMMAND_MAX_MSG_LENGTH];
   tSIRF_UINT32 packet_length = sizeof(packet);
   tSIRF_UINT32 options;
   tSIRF_RESULT result;

   /* If the message isn't processed correctly set the timeout to infinite */
   *timeout = SIRF_TIMEOUT_INFINITE;

   /* Record the sender in case of error or specific response */
   gprs_at_command.command_handle    = msg->handle;
   gprs_at_command.command_msg       = msg->message_id;
   gprs_at_command.command_responses = 0;
      
   /* The TCP/IP connection message requires us to remember to index that is
    * being used with this handle so we can send the right +WIPDATA message*/
   if (SIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_TCP_CLIENT == msg->message_id)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_TCP_CLIENT *client = (tSIRF_MSG_GPRS_AT_COMMAND_PWIPCREATE_TCP_CLIENT*)msg->message_structure;
      gprs_at_command.listener[(tSIRF_UINT32)msg->handle].data_index = client->communication_index;
   }

   /* Special handling for PCCED since it can have 1,2,or 3 responses */
   if (SIRF_MSG_GPRS_AT_COMMAND_PCCED == msg->message_id)
   {
      tSIRF_MSG_GPRS_AT_COMMAND_PCCED *pcced = (tSIRF_MSG_GPRS_AT_COMMAND_PCCED*)msg->message_structure;
      if (SIRF_MSG_GPRS_AT_COMMAND_PCCED_MODE_ONCE == pcced->mode)
      {
         gprs_at_command.command_data = pcced->requested_dump;
      }
      else
      {
         gprs_at_command.command_data = 0;
      }
   }

   /* Send the message */
   options = SIRF_CODEC_OPTIONS_GET_FIRST_MSG;
   packet_length = sizeof(packet);
   result = SIRF_PROTO_GPRS_AT_COMMAND_Encode(msg->message_id,
                                              msg->message_structure,
                                              msg->message_length,
                                              packet,
                                              &packet_length,
                                              &options);
   if (SIRF_SUCCESS == result)
   {
      result = SIRF_EXT_UART_Send(gprs_at_command.uartno,
                                  packet,
                                  packet_length);
      if (SIRF_SUCCESS == result)
      {
         gprs_at_command.state = GPRS_SERVER_STATE_AT_RESPONSE;
         *timeout = gprs_at_command.command_timeout;
      }
   }

   return result;
}

/** 
 * Process all the messages on one of the GPRS queues
 * 
 * @param msg_queue Which message queue to process
 * @param handler   Which handler function to use
 * @param timeout   Pointer to amount of time to wait for the next message.  
 *                  This is set to SIRF_TIMEOUT_INFINITE if there are no 
 *                  messages left on the queue.
 * 
 * @return result of the handler or SIRF_SUCCESS if there are no messages
 * process.  
 */
static tSIRF_RESULT process_next_msg( 
   fifo_queue_t            * const msg_queue,
   tGPRS_MSG_HANDLER               handler,
   tSIRF_UINT32            * const timeout)
{
   gprs_msg_t *msg;
   fifo_link_t *link;
   tSIRF_RESULT result;

   UTIL_Assert(NULL != msg_queue);
   UTIL_Assert(NULL != handler);

   result = SIRF_PAL_OS_MUTEX_Enter(gprs_at_command.queue_mx);

   if(SIRF_SUCCESS != result)
   {
      return result;
   }
    
   /* Get the next message */
   link = fifo_queue_remove_head( msg_queue );

   result = SIRF_PAL_OS_MUTEX_Exit(gprs_at_command.queue_mx);
   if(SIRF_SUCCESS != result)
   {
      return result;
   }

   if (NULL != link)
   {
      msg = (gprs_msg_t*) link->data;
      result = handler(msg,
                       timeout);
      Heap_Free(gprs_at_command.msg_heap,link);
   }
   else
   {
      /* No messages to process, we need to wait until one arrives */
      *timeout = SIRF_TIMEOUT_INFINITE;
   }

   return result;
}

/** 
 * Read thread to process messages from or to the GPRS modem
 */
static SIRF_PAL_OS_THREAD_DECL SIRF_GPRS_AT_COMMAND_ReadThread( SIRF_PAL_OS_THREAD_PARAMS )
{
   tSIRF_UINT32 wait_start;
   tSIRF_UINT32 wait_end;
   tSIRF_UINT32 wait_elapsed;
   tSIRF_UINT32 timeout;
   tSIRF_UINT32 result;
   tSIRF_UINT32 semaphore_result;

   SIRF_PAL_OS_THREAD_UNUSED;
   SIRF_PAL_OS_THREAD_START();
   gprs_at_command.semaphore_timeout = SIRF_TIMEOUT_INFINITE;
   /*-------------------------------------------------------------------------*/
   /* Main loop */
   /*-------------------------------------------------------------------------*/
   while ( gprs_at_command.thread_running )
   {
      wait_start = SIRF_PAL_OS_TIME_SystemTime();
      semaphore_result = SIRF_PAL_OS_SEMAPHORE_Wait(gprs_at_command.semaphore,
                                                    gprs_at_command.semaphore_timeout);
      /* We just woke up.  Check to see if we've been asked to exit */
      if ( !gprs_at_command.thread_running )
      {
         break;
      }
      
      /* Record how long we were waiting, just in case what we were looking
       * for did not arrive */
      wait_end = SIRF_PAL_OS_TIME_SystemTime();

      /* 32 bits of timer will roll over every ~50 days, but we need to be
       * safe */
      if (wait_end >= wait_start)
      {
         wait_elapsed = wait_end - wait_start;
      }
      else
      {
         /* Roll over case */
         tSIRF_UINT64 temp;
         temp = wait_end + ULONG_MAX + 1 - wait_start;
         /* This should now be less than 32 bits */
         wait_elapsed = (tSIRF_UINT32) temp;
      }

      /* If we are in TCP Data mode process all of these messages */
      if (GPRS_SERVER_STATE_TCP_IP_DATA == gprs_at_command.state)
      {
         result = process_next_msg( &gprs_at_command.listener[(tSIRF_UINT32)gprs_at_command.data_handle].msg_queue_mas,
                                    handle_mas_msg,
                                    &timeout);
         if (SIRF_SUCCESS != result)
         {
            /* really bad error occured here, send it to the listeners */
            handle_unexpected_result(result);
            SIRF_PAL_OS_THREAD_Sleep(10);
         }
         /* For TCP Data the timeout values are either 0 or GPRS_DATA_TIMEOUT
          * as the data connection should be shut down if idle for the 
          * GPRS_DATA_TIMEOUT amount.  If a message was processed the timeout
          * value is set to 0 so that the next one can be processed */
         else if (SIRF_TIMEOUT_INFINITE == timeout)
         {
            result = process_next_msg( &gprs_at_command.listener[(tSIRF_UINT32)gprs_at_command.data_handle].msg_queue_mas_input,
                                       handle_mas_input_msg,
                                       &timeout);
            if (SIRF_SUCCESS != result)
            {
               /* really bad error occured here, send it to the listeners */
               handle_unexpected_result(result);
               SIRF_PAL_OS_THREAD_Sleep(10);
            }
            /* At this point check to see if the timeout has elapsed and we need
             * to transition from data to at command mode */
            if (SIRF_SUCCESS != semaphore_result && wait_elapsed >= GPRS_DATA_TIMEOUT)
            {
               gprs_switch_from_data_mode();
               gprs_at_command.semaphore_timeout = SIRF_TIMEOUT_INFINITE;
            }
            else
            {
               gprs_at_command.semaphore_timeout = GPRS_DATA_TIMEOUT;
            }
         }
         
         continue;
      }

      /* If we make it to this line of code the state is either
       * GPRS_SERVER_STATE_AT_COMMAND 
       * GPRS_SERVER_STATE_AT_RESPONSE
       * 
       * Handle responses and unsolicited messages next
       */
      result = process_next_msg( &gprs_at_command.msg_queue_gprs,
                                 handle_gprs_msg,
                                 &timeout);

      if (SIRF_SUCCESS != result)
      {
         /* really bad error occured here, send it to the listeners */
         handle_unexpected_result(result);
         SIRF_PAL_OS_THREAD_Sleep(10);
         continue;
      }

      /* SIRF_TIMEOUT_INFINITE means nothing was processed.  
       * Other values indicate that a message was processed  */
      if (SIRF_TIMEOUT_INFINITE != timeout)
      {
         /* Loop around, don't wait and look for another message */
         gprs_at_command.semaphore_timeout = 0;
         gprs_at_command.command_timeout_left = timeout;
         continue;
      }

      /* Now check the state to see if we are still waiting for a response.  If
       * we are then we can't process any input messages yet and must wait until
       * the timout before proceding.  Check to see if the timeout has occured
       * and if not update the timeout values */
      else if (GPRS_SERVER_STATE_AT_RESPONSE == gprs_at_command.state )
      {
         if ((SIRF_SUCCESS != semaphore_result) && 
             (wait_elapsed >= gprs_at_command.command_timeout_left))
         {
            /* This is an error case.  We timed out waiting for the expected
             * response, Send up an error message and continue */
            handle_response_timeout();
            gprs_at_command.state = GPRS_SERVER_STATE_AT_COMMAND;
         }
         else
         {
            /* Adjust the timeout for the time elapsed waiting */
            gprs_at_command.command_timeout_left -= wait_elapsed;
            gprs_at_command.semaphore_timeout = 
               gprs_at_command.command_timeout_left; 
         }
         /* In either case, continue */
         continue;
      }

      result = process_next_msg( &gprs_at_command.msg_queue_gprs_input,
                                 handle_input_msg,
                                 &gprs_at_command.semaphore_timeout);
      
      if (SIRF_SUCCESS != result)
      {
         /* really bad error occured here, send it to the listeners */
         handle_unexpected_result(result);
         SIRF_PAL_OS_THREAD_Sleep(10);
      }
      else 
      {
         gprs_at_command.command_timeout_left = gprs_at_command.semaphore_timeout;
      }

   } /* while()*/

   SIRF_PAL_OS_THREAD_RETURN();

} /* SIRF_EXT_UART_ReadThread()*/

/**
 * @}
 * End of file.
 */

