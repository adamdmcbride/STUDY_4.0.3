/**
 * @addtogroup platform_src_sirf_util_demo
 * @{
 */

/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2005 - 2009 by SiRF Technology, Inc.  All rights       *
 *    reserved.                                                            *
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
 * FILENAME:  sirf_demo.c
 *
 * DESCRIPTION: This file include the data structures and the functions to
 *              implement the registration of protocols with UI mdoule
 *
 ***************************************************************************/

/***************************************************************************
 * Include Files
 ***************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "SiRFNav.h"
#include "sirf_demo.h"
#include "sirf_pal.h"

#ifdef SIRFNAV_DEMO
#include "sirfnav_demo_config.h"
#endif

#include "sirf_ext_log.h"
#include "sirf_ext_aux.h"
#include "sirf_ext_tcpip.h"
#include "sirf_codec.h"
#include "sirf_codec_f.h"
#include "sirf_codec_ssb.h"
#include "sirf_proto_ai3.h"
#include "sirf_proto_parse.h"
#include "string_sif.h"

#ifdef SIRF_STRESS_TEST
#include "sirf_stress_test.h"
#endif

/***************************************************************************
 * External Data Declarations
 ***************************************************************************/

/***************************************************************************
 * Global Data Declarations
 ***************************************************************************/

/***************************************************************************
 * Local Declarations
 ***************************************************************************/

static tSIRF_BOOL c_demo_running = SIRF_FALSE;   /* TRUE means demo running */

/* start/stop testing */
tSIRF_UINT32 c_last_start_mode;
tSIRF_UINT32 c_last_clock_offset;
tSIRF_UINT32 c_last_port_num;
tSIRF_UINT32 c_last_baud_rate;
#if 0 /* disable for now (3tw leftover) */
static tSIRF_BOOL   c_last_available = SIRF_FALSE;
static tSIRF_UINT8  c_restart_mode =  SIRF_MSG_SSB_RESTART_MODE_START_STOP;
#endif

static tSIRF_RESULT SIRFNAV_DEMO_Parse_Callback( tSIRF_UINT8 *PktBuf, 
                                                 tSIRF_UINT32 PktLen, 
                                                 tSIRF_ParserType Parser );


/***************************************************************************
 * Description: 
 * Parameters:  
 * Returns:     
 ***************************************************************************/

static tSIRF_RESULT SIRFNAV_DEMO_Data_Callback(tSIRF_UINT8 *buf, 
                                               tSIRF_UINT32 len )
{
   tSIRF_RESULT result = SIRF_SUCCESS;
#ifdef SIRF_LOC
   /* SL parser handles SSB, AI3, F, Debug, and NMEA */
   SIRF_PROTO_SLParse( buf, len );
#else
   /* Combines the SSB and NMEA parsers */
   SIRF_PROTO_Parse( buf, len );
#endif /* SIRF_LOC */
   return result;
}

/***************************************************************************
 * Description: Function to query if the demo is running or not
 * Parameters:  none
 * Returns:     TRUE : FALSE
 ***************************************************************************/

tSIRF_BOOL SIRFNAV_DEMO_Running()
{
   return c_demo_running;
}

/***************************************************************************
 * Description: prepare the demo for operation - call BEFORE SiRFNav_Start
 * Parameters:  none
 * Returns:     SIRF_SUCCESS : SIRF_FAILURE
 ***************************************************************************/

tSIRF_UINT32 SIRFNAV_DEMO_Create( tSIRF_UINT8* port_str,tSIRF_UINT32 baud, 
                                  tSIRF_UINT16 tcpip_port,
                                  char *logname, tSIRF_UINT8 input_flags )
{
   tSIRF_UINT32 tRet = SIRF_FAILURE;

   /* Unused parameters */
#if !defined(SIRF_EXT_TCPIP) && !defined(SIRF_EXT_LOG) && !defined(SIRF_EXT_AUX)
   (tSIRF_VOID)input_flags;
#endif
#if !defined(SIRF_EXT_TCPIP)
   (tSIRF_VOID)tcpip_port;
#endif

   if (!c_demo_running)
   {

   /* initial values */
#ifdef SIRF_LOC
      SIRF_PROTO_AI3_Init( SIRFNAV_DEMO_Send_Passthrough,
                           SiRFNav_Input ); /* Register callback for the send and input */
#endif /* SIRF_LOC */

      c_demo_running = SIRF_TRUE;
      tRet = SIRF_SUCCESS;

      /* Optional Log section. If it fails, don't stop the app */
#if defined(SIRF_EXT_LOG)
      if ( (input_flags & SIRF_LOG_ENABLED) && (SIRF_SUCCESS == tRet) )
      {
         (tSIRF_VOID)SIRF_EXT_LOG_Open(SIRF_EXT_LOG_SIRF_ASCII_TEXT,(char *)logname);
      }
#endif /* SIRF_EXT_LOG */

      /* Optional Net section. If it fails, don't stop the app */
#if defined(SIRF_EXT_TCPIP)
      if ( (input_flags & SIRF_TCPIP_ENABLED) && (SIRF_SUCCESS == tRet) ) 
      {
         (tSIRF_VOID)SIRF_EXT_TCPIP_Create(NULL, tcpip_port); /* NULL = create listener */

         /* Register the callback for incoming data to be parsed */
         SIRF_EXT_TCPIP_Callback_Register( SIRFNAV_DEMO_Data_Callback );
      }
#endif /* SIRF_EXT_TCPIP */

      /* Optional Aux port section. If it fails, don't stop the app unless requested */
#ifdef SIRF_EXT_AUX
      if ( (input_flags & SIRF_AUX_ENABLED) && (SIRF_SUCCESS == tRet) )
      {
         /* Special case to check and pass back the failure */
         if ( input_flags & SIRF_AUX_OPEN_FAIL )
         {
            tRet = SIRF_EXT_AUX_Create(&port_str[0], baud);
         }
         else
         {
            (tSIRF_VOID)SIRF_EXT_AUX_Create(&port_str[0], baud);
         }

         /* Register the callback for incoming data to be parsed */
         SIRF_EXT_AUX_Callback_Register( SIRFNAV_DEMO_Data_Callback );
      }
#endif /* SIRF_EXT_AUX */

      /* Register the callback function for the parser to call */
      SIRF_PROTO_Parse_Register( SIRFNAV_DEMO_Parse_Callback );

      if (tRet != SIRF_SUCCESS)
      {
         c_demo_running = SIRF_FALSE;
      }
   }
   return tRet;
}

/***************************************************************************
 * Description: clean up after a demo run - call AFTER SiRFNav_Stop
 * Parameters:  none
 * Returns:     SIRF_SUCCESS : SIRF_FAILURE
 ***************************************************************************/

tSIRF_UINT32 SIRFNAV_DEMO_Delete( tSIRF_VOID )
{
   tSIRF_UINT32 tRet = SIRF_FAILURE;
   if (c_demo_running)
   {
      c_demo_running = SIRF_FALSE;
      tRet = SIRF_SUCCESS;
      /* Close and delete in reverse order 
         Close the aux serial and delete the COM channel so the thread will wake up and die 
       */
#if defined(SIRF_EXT_LOG)
      (tSIRF_VOID)SIRF_EXT_LOG_Close();
#endif

#if defined(SIRF_EXT_AUX)
      (tSIRF_VOID)SIRF_EXT_AUX_Delete();
#endif

#if defined(SIRF_EXT_TCPIP)
      (tSIRF_VOID)SIRF_EXT_TCPIP_Delete();
#endif
         
   }
   return tRet;
}


/***************************************************************************
 * Description: Used for callback from the parsers
 * Parameters:  
 * Returns:     
 ***************************************************************************/
/* Define maximum size for the message buffer */
#define MAX_BUFFER_SIZE 6000

static tSIRF_RESULT SIRFNAV_DEMO_Parse_Callback( tSIRF_UINT8 *PktBuf, 
                                                 tSIRF_UINT32 PktLen, 
                                                 tSIRF_ParserType Parser )
{
   tSIRF_UINT32 mid = 0;              /* message id */
   tSIRF_UINT8 msg[MAX_BUFFER_SIZE];  /* message buffer */
   tSIRF_UINT32 len = 0;              /* message length */
   tSIRF_RESULT tRet = SIRF_SUCCESS;
#if defined(SIRF_LOC)
   tSIRF_UINT8 Logical_Channel; /* F, AI3, SSB, NMEA, et. al. */
#endif

   /* Validate arguments */
   if ( !PktBuf || (0 == PktLen) )
   {
      return SIRF_FAILURE;
   }

#ifdef SIRF_LOC

   if ( PARSER_SIRFLOC == Parser )
   {
      /* Get the logical channel information */
      Logical_Channel = PktBuf[0];

      if (Logical_Channel == SIRF_LC_AI3)
      {   
         /* Over the serial interface AI3 messages can be 
          * packeted.  Call the packet handler, when a fully
          * formed packet is finally received have it call
          * SiRFNav_Input */
         if (SIRF_SUCCESS != SIRF_PROTO_AI3_PacketInput(PktBuf,PktLen))
         {
            tRet = SIRF_FAILURE;
         }
      }
      else if (Logical_Channel == SIRF_LC_F)
      {
         /* Convert the byte stream to a structure */
         if (SIRF_CODEC_F_DecodeCp(PktBuf, PktLen, &mid, msg, &len) == SIRF_SUCCESS)
         {   
            SiRFNav_Input(mid, msg, len);
         }
      }
      else if (Logical_Channel == SIRF_LC_NMEA)
      {
         SiRFNav_Input(SIRF_MAKE_MSG_ID(Logical_Channel, 0, 0), &PktBuf[1], PktLen-1);
      }
      else if (Logical_Channel == SIRF_LC_SSB)
      {
         tSIRF_UINT32 option = SIRF_CODEC_OPTIONS_GET_FIRST_MSG;
         if (SIRF_CODEC_SSB_Decode(&PktBuf[1], PktLen-1, &mid, msg, &len, &option) == SIRF_SUCCESS)
         {
            /* Further parse specific SSB commands. Will call SiRFNav_Input if necessary
             * to pass messages to the SiRFNav library */
            SIRFNAV_DEMO_Receive(mid, msg, len);
         }
      }
      else if (Logical_Channel == SIRF_LC_DEBUG)
      {
         SiRFNav_Input(SIRF_MAKE_MSG_ID(Logical_Channel, 0, 0), &PktBuf[1], PktLen-1);
      }
   }
   else
   {
      tRet = SIRF_FAILURE;
   }

#else
   /* Non SL. Check for SSB or NMEA */
   if ( PARSER_SSB == Parser ) 
   {
      tSIRF_UINT32 option = 1;
      if (SIRF_CODEC_SSB_Decode(PktBuf, PktLen, &mid, msg, &len,&option) == SIRF_SUCCESS)
      {
         SIRFNAV_DEMO_Receive(mid, msg, len);
      }
   }
   else if ( PARSER_NMEA == Parser )
   {
      SiRFNav_Input(SIRF_MAKE_MSG_ID(SIRF_LC_NMEA, 0, 0), PktBuf, PktLen);
   }
   else 
   {
      tRet = SIRF_FAILURE;
   }

#endif /* SIRF_LOC */

   return tRet;
}

/***************************************************************************
 * DESCRIPTION: 
 * PARAMETERS:  
 * RETURN:      
 ***************************************************************************/

tSIRF_RESULT SIRFNAV_DEMO_Send_Passthrough( tSIRF_VOID *buffer, tSIRF_UINT32 PktLen )
{
   /* Pipe output to external components (AUX port, GUI, TCP/IP, ...): */
#if defined( SIRF_EXT_AUX )
   tSIRF_INT8 auxno;

   for ( auxno = 0; auxno < MAX_AUX_PORTS; auxno++ )
   {
      SIRF_EXT_AUX_Send_Passthrough( auxno, buffer, PktLen );
   }
#endif

#if defined( SIRF_EXT_GUI )
   /* Placeholder - not currently implemented */
   SIRF_EXT_GUI_Send_Passthrough( buffer, PktLen );
#endif

#if defined( SIRF_EXT_TCPIP ) && !defined ( SIRF_COMPACT_DEMO )
   SIRF_EXT_TCPIP_Send_Passthrough( buffer, PktLen );
#endif

#if defined( SIRF_EXT_LOG )
   SIRF_EXT_LOG_Send_Passthrough( buffer, PktLen );
#endif

#if defined( SIRF_EXT_LED )
   /* Placeholder - not currently implemented */
   SIRF_EXT_LED_Send_Passthrough( buffer, PktLen );
#endif

   return SIRF_SUCCESS;

} /* SIRFNAV_DEMO_Send_Passthrough */

/***************************************************************************
 * DESCRIPTION: 
 * PARAMETERS:  
 * RETURN:      
 ***************************************************************************/

tSIRF_VOID SIRFNAV_DEMO_Send( tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
{
#ifdef SIRF_EXT_AUX
   tSIRF_INT8 auxno;
#endif
   tSIRF_UINT8 LogicalChannel = SIRF_GET_LC(message_id);

   /* Special handling for some SSB messages */
   if ( SIRF_LC_SSB == LogicalChannel )
   {
   switch ( message_id )
      {
         case SIRF_MSG_SSB_SIRFNAV_START:
         {
            tSIRF_MSG_SSB_SIRFNAV_START *msg = (tSIRF_MSG_SSB_SIRFNAV_START*)message_structure;
            c_last_start_mode   = msg->start_mode;
            c_last_clock_offset = msg->clock_offset;
            c_last_port_num     = msg->port_num;
            c_last_baud_rate    = msg->baud_rate;
#if 0 /* disable for now (3tw leftover) */
            c_last_available    = SIRF_TRUE;
#endif
            break;
         }

#if 0
#ifndef SIRF_COMPACT_DEMO
         /* Append Demo version string to Navserver version */
         case SIRF_MSG_SSB_SW_VERSION:
         {
            tSIRF_CHAR demo_version_str[] = " Demo-" SIRFNAV_DEMO_VERSION;
            tSIRF_MSG_SSB_SW_VERSION *msg = (tSIRF_MSG_SSB_SW_VERSION*)message_structure;

            /* Append only if there is enough room in the message */
            if( (strnlen(demo_version_str,sizeof(demo_version_str)) + 
                 strnlen((char *)msg->sw_version,sizeof(msg->sw_version))) 
                < sizeof(msg->sw_version) )
            {
               strlcat((char *)msg->sw_version, 
                       demo_version_str,
                       sizeof(msg->sw_version));
               message_length = strnlen((char *)msg->sw_version,
                                        sizeof(msg->sw_version));
            }
            break;
         }
#endif /* SIRF_COMPACT_DEMO */
#endif

         default:
         {
            /* If message_id is not SIRF_MSG_SSB_SIRFNAV_START, do nothing here */
            break;
         }
      } /* switch */
   } /* if */
   
#ifndef SIRF_LOC
   /* Use the passthrough version for NMEA for GSW */
   if ( SIRF_LC_NMEA == LogicalChannel )
   {
      SIRFNAV_DEMO_Send_Passthrough( message_structure, message_length );
   } 
   else
#endif
   {
   /* Pipe output to external components (AUX port, GUI, TCP/IP, ...): */
#ifdef SIRF_EXT_AUX
      for ( auxno = 0; auxno < MAX_AUX_PORTS; auxno++ )
      {
         SIRF_EXT_AUX_Send( auxno, message_id, message_structure, message_length );
      }
#endif

#if defined( SIRF_EXT_GUI )
   /* Placeholder - not currently implemented */
      SIRF_EXT_GUI_Send( message_id, message_structure, message_length );
#endif

#if defined( SIRF_EXT_TCPIP ) && !defined ( SIRF_COMPACT_DEMO )
      SIRF_EXT_TCPIP_Send( message_id, message_structure, message_length );
#endif

#if defined( SIRF_EXT_LOG )
      SIRF_EXT_LOG_Send( message_id, message_structure, message_length );
#endif

#if defined( SIRF_EXT_LED )
      /* Placeholder - not currently implemented */
      SIRF_EXT_LED_Send( message_id, message_structure, message_length );
#endif
   }


} /* SIRFNAV_DEMO_Send */

/***************************************************************************
 * DESCRIPTION: Example routine showing how to get data from SLD et. al.
 * PARAMETERS:  message_id, structure, length
 * RETURN:      SIRF_FAILURE : SIRF_SUCCESS
 ***************************************************************************/

tSIRF_RESULT SIRFNAV_DEMO_Receive(tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length)
{
   tSIRF_RESULT tRet = SIRF_FAILURE;

   if ((message_structure != NULL) && c_demo_running )
   {
#ifdef SIRF_COMPACT_DEMO
      /* Pass all data to the target system */
      tRet = SiRFNav_Input( message_id, message_structure, message_length );
#else
#ifdef SIRF_AGPS
      tSIRF_MSG_SSB_ACKNACK_ERROR acknakmsg;
#else
      tSIRF_MSG_SSB_ACK acknakmsg;
#endif /* SIRF_AGPS */
      /* prepare the ack or nak message with the incoming message id */
      acknakmsg.msg_id = (tSIRF_CHAR)(message_id & 0xff);
      acknakmsg.sub_id = (tSIRF_CHAR)(message_id >> 8);

      switch (message_id)
      {
         /* re-init (0) or start-stop(1) */
         case SIRF_MSG_SSB_DEMO_SET_RESTART_MODE:
#if 0 /* disable for now */
            c_restart_mode = ((tSIRF_MSG_SSB_DEMO_SET_RESTART_MODE *)message_structure)->control;
#endif
#ifdef SIRF_AGPS
            acknakmsg.session_error_reason = SIRF_MSG_SSB_ACK_VAL;
            SIRFNAV_DEMO_Send( SIRF_MSG_SSB_ACK_NACK_ERROR_OUT, &acknakmsg, sizeof(acknakmsg) );         
#else
            SIRFNAV_DEMO_Send(SIRF_MAKE_MSG_ID(SIRF_LC_SSB,SIRF_MSG_SSB_ACK,0), &acknakmsg, sizeof(acknakmsg));
#endif /* SIRF_AGPS */
            break;

         case SIRF_MSG_SSB_INITIALIZE:
         case SIRF_MSG_SSB_DR_SET_NAV_INIT:
#if 0 /* disable for now */
            if ( c_restart_mode == SIRF_MSG_SSB_RESTART_MODE_START_STOP )
            {  /* full restart of GPS Engine: */
               tSIRF_UINT32 gps_start_mode;
               
               DEBUGMSG(0, (DEBUGTXT("Stopping GPS...\n")));
               tRet = SiRFNav_Stop( 0 );
               if ( tRet != SIRF_SUCCESS )
               {
                  DEBUGMSG(0, (DEBUGTXT("ERROR: Could not stop GPS engine, result=0x%4lX\n"), tRet));
               }
               else
               {
                  DEBUGMSG(0, (DEBUGTXT("GPS stopped.\n")));
               }
               
               if ( message_id == SIRF_MSG_SSB_INITIALIZE )
               {
                  tSIRF_MSG_SSB_INITIALIZE *msg = (tSIRF_MSG_SSB_INITIALIZE*) message_structure;
                  switch ( (msg->restart_flags) & 0x3F )
                  {
                     case '0': gps_start_mode = SIRFNAV_UI_CTRL_MODE_AUTO;    break;
                     case '2': gps_start_mode = SIRFNAV_UI_CTRL_MODE_WARM;    break;
                     case '4': gps_start_mode = SIRFNAV_UI_CTRL_MODE_COLD;    break;
                     case '8': gps_start_mode = SIRFNAV_UI_CTRL_MODE_FACTORY; break;
                     default:  gps_start_mode = SIRFNAV_UI_CTRL_MODE_AUTO;    break;
                  }
               }
               
               DEBUGMSG(0, (DEBUGTXT("Starting GPS...\n")));
               tRet |= SiRFNav_Start( c_last_start_mode | gps_start_mode, c_last_clock_offset, c_last_port_num, c_last_baud_rate);
               
               if ( SIRF_SUCCESS == tRet )
               {
                  DEBUGMSG(0, (DEBUGTXT("GPS started.\n")));
               }
               /* Causes compiler warning
               else
               {
                  DEBUGMSG(0, (DEBUGTXT("ERROR: Could not restart GPS engine, result=0x%4lX\n"), tRet));
               }
               */
            }
            else
#endif
            { /* reinitialize GPS Engine:  (ack/nak comes asynchronously) */
               tRet = SiRFNav_Input( message_id, message_structure, message_length );
            }
            break;

         case SIRF_MSG_SSB_DEMO_TEST_CPU_STRESS:
#if defined (SIRF_STRESS_TEST)
            /* SiRFNav Demo: stress test is enabled */
            tRet = SIRF_STRESS_Input( message_id, message_structure, message_length );
#ifdef SIRF_AGPS
            if ( tRet == SIRF_SUCCESS )
            {
               acknakmsg.session_error_reason = SIRF_MSG_SSB_ACK_VAL;
               SIRFNAV_DEMO_Send(SIRF_MSG_SSB_ACK_NACK_ERROR_OUT, &acknakmsg, sizeof(acknakmsg) );
            }
            else
            {
               acknakmsg.session_error_reason = SIRF_MSG_SSB_NACK_VAL;
               SIRFNAV_DEMO_Send(SIRF_MSG_SSB_ACK_NACK_ERROR_OUT, &acknakmsg, sizeof(acknakmsg) );
            }
#else /* SIRF_AGPS */
            if ( tRet == SIRF_SUCCESS )
            {
               SIRFNAV_DEMO_Send( SIRF_MAKE_MSG_ID(SIRF_LC_SSB,SIRF_MSG_SSB_ACK,0), &acknakmsg, sizeof(acknakmsg) );
            }
            else
            {
               SIRFNAV_DEMO_Send( SIRF_MAKE_MSG_ID(SIRF_LC_SSB,SIRF_MSG_SSB_NAK,0), &acknakmsg, sizeof(acknakmsg) );
            }
#endif /* SIRF_AGPS */
#else /* SIRF_STRESS_TEST */
#ifdef SIRF_AGPS
            acknakmsg.session_error_reason = SIRF_MSG_SSB_NACK_VAL;
            SIRFNAV_DEMO_Send( SIRF_MSG_SSB_ACK_NACK_ERROR_OUT, &acknakmsg, sizeof(acknakmsg) );
#else
            /* SiRFNav Demo: stress test is disabled */
            SIRFNAV_DEMO_Send( SIRF_MAKE_MSG_ID(SIRF_LC_SSB,SIRF_MSG_SSB_NAK,0), &acknakmsg, sizeof(acknakmsg) );
#endif /* SIRF_AGPS */
#endif /*SIRF_STRESS_TEST */
            break;

            /* This case is only valid for 3tw */
         case SIRF_MSG_SSB_DEMO_START_GPS_ENGINE:
#if 0
            {
               tSIRF_MSG_SSB_DEMO_START_GPS_ENGINE *msg = (tSIRF_MSG_SSB_DEMO_START_GPS_ENGINE*)message_structure;
#if defined(SIRF_EXT_AUX)
               /* reopen AUX port in case system have been suspended */
               SIRF_EXT_AUX_Reopen(0);
#endif
               if ( ( msg->start_mode == 0 ) &&
                    ( msg->clock_offset == 0) &&
                    ( msg->port_num == 0 ) &&
                    ( msg->baud_rate == 0 ) &&
                    ( c_last_available ) ) /* determines if the values have been set previously */
               {
                  /* Use the same start parameters as were used in first start except GPS start mode */
                  tRet = SiRFNav_Start( c_last_start_mode & ~SIRFNAV_UI_CTRL_MODE_MASK, c_last_clock_offset, c_last_port_num, c_last_baud_rate);
                  DEBUGMSG(1,(DEBUGTXT("INFO: SiRFNav_Start returned 0x%4lX\n"), tRet));
               } 
               else
               {
                  tRet = SiRFNav_Start(msg->start_mode, msg->clock_offset, msg->port_num, msg->baud_rate);
                  DEBUGMSG(1,(DEBUGTXT("INFO: SiRFNav_Start returned 0x%4lX\n"), tRet));
               }
            }
#endif
            break;

            /* This case is only valid for 4t/4e */
         case SIRF_MSG_SSB_DEMO_START_NAV_ENGINE:
            {
               tSIRF_MSG_SSB_DEMO_START_NAV_ENGINE *msg = (tSIRF_MSG_SSB_DEMO_START_NAV_ENGINE *)message_structure;
#if defined(SIRF_EXT_AUX)
               /* reopen AUX port in case system have been suspended */
               /* We don't have enough information to reopen the aux port */
               //SIRF_EXT_AUX_Reopen(0);
#endif
               tRet = SiRFNav_Start( &msg->config );
               DEBUGMSG(1,(DEBUGTXT("INFO: SiRFNav_Start returned 0x%4lX\n"), tRet));
            }
            break;

         case SIRF_MSG_SSB_DEMO_STOP_GPS_ENGINE:
            {
               tSIRF_MSG_SSB_DEMO_STOP_GPS_ENGINE *msg = (tSIRF_MSG_SSB_DEMO_STOP_GPS_ENGINE*)message_structure;
               tRet = SiRFNav_Stop(msg->stop_mode);
               if ( tRet != SIRF_SUCCESS )
               {
                  DEBUGMSG(1, (DEBUGTXT("ERROR: Could not stop GPS engine, result=0x%4lX\n"), tRet));
               }
               else
               {
                  DEBUGMSG(1, (DEBUGTXT("GPS stopped.\n")));
               }
            }
            break;

         default:
            {
               tRet = SiRFNav_Input(message_id, message_structure, message_length);
            }
            break;
      }
#endif /* SIRF_COMPACT_DEMO */
   }
   return tRet;
}

/**
 * @}
 */
