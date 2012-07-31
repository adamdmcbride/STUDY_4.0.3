/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2010 by SiRF Technology, a CSR plc Company.            *
 *    All rights reserved.                                                 *
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.s confidential and        *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.s  express written             *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define  LOG_TAG  "gps_sirf_tcp"

#include <cutils/log.h>
# define  D(...)   LOGD(__VA_ARGS__)

/* SiRF Software Interface Header files */
#include "SiRFNav.h"
#include "sirf_ext_tcpip.h"
#include "sirf_proto_ai3.h"
#include "sirf_proto_parse.h"
#include "sirf_ext_aux.h"
#include "sirf_types.h"
#include "sirf_msg_ssb.h"
#include "sirf_codec_ssb.h"


#define MAX_BUFFER_SIZE 6000
static tSIRF_UINT8 msg[MAX_BUFFER_SIZE];  /* message buffer */

static tSIRF_CHAR *m_sensors_cfg_name = "/system/sensors.txt";
static tSIRF_MSG_SSB_SENSOR_CONFIG  m_sensCfg;

static tSIRF_UINT8 m_send_confing_msg;
static tSIRF_UINT8 m_send_data_msg;

extern void SIRF_SendSensorEnableMsgs(tSIRF_MSG_SSB_SENSOR_CONFIG *sensCfg);
extern tSIRF_RESULT SIRF_LoadSensorsConfiguration(FILE* file_handle, tSIRF_MSG_SSB_SENSOR_CONFIG *sensCfg);
extern int HandleRawSensorDataInput(tSIRF_MSG_SSB_SENSOR_READINGS* pMsg);
/*
   Setting file: text file, optional, which contains text lines like the following:
   OPTION_NAME = 1
   It is intended to be used only during initialization.
*/
struct KeyValue {
   char  *key;    // staticly intialized option name
   int   *value;  // staticly intialized pointer (!) to the local variable (so all variables must be delcared before)
};

static int m_log72 = 0;    // Enable logging all 72,* messages (default: disabled)
static int m_disable = 0;  // Disable sensors interface completely (default: do not disable)
//extern int gContextLogEnabled; // enable creating context_log.csv in /data/ folder for fine CD tuning

// local list of supported options
static struct KeyValue m_options[] = {
   { "LOG_MSG72",   & m_log72 },
   { "DISABLE",     & m_disable }
//   { "LOG_CD",      & gContextLogEnabled }
};
#define LOCAL_OPT_SIZE (sizeof(m_options)/sizeof(struct KeyValue)) 


static void load_options(void)
{
   FILE *fin = fopen( "/data/debug_sensors.txt", "r" );
   if( NULL == fin ) 
      return;

   while (fgets((char*)msg, sizeof(msg), fin)) {
      unsigned int i;
      char *ptr = strchr( (char *)msg, '=');
      if( NULL == ptr )
         continue;
      *ptr++ = 0; // put EOL
      // try to find this TAG in the local list of options
      for( i = 0; i < LOCAL_OPT_SIZE; i++ ) {
         if( NULL != strstr( (const char *)msg, m_options[i].key ) ) {
            // option found: set the value now
            int *value = m_options[i].value;
            if( NULL != value ) {
               *value = atoi( ptr );
               break;
            }
         }
      }
   }

   fclose(fin);
}

static void sirf_enable_sensors(void)
{
   D("SIRF_SendSensorEnableMsgs ...");
   SIRF_SendSensorEnableMsgs( & m_sensCfg );
   m_send_confing_msg = 0;
   m_send_data_msg = 1;
}

tSIRF_RESULT sirf_gps_tcp_output(tSIRF_UINT32 message_id)
{
   if( m_disable ||                    // sensors are disabled
       0 == m_sensCfg.numSensors ) {   // configuration file is not loaded
      return SIRF_SUCCESS; // ignore any message 
      }

   if( m_send_confing_msg && ( SIRF_MSG_SSB_GEODETIC_NAVIGATION == message_id ) ) {
      sirf_enable_sensors();
      }

   if( ! m_log72 ) {
      // disable 72,* output because of potential interference with the other thread calling SiRFNav_Output()
      // keep 72,3 because this is the output for the outer world
      if( 72 == SIRF_GET_MID(message_id) && 
           3 != SIRF_GET_SUB_ID(message_id) ) 
         return SIRF_FAILURE;
      }
      
   return SIRF_SUCCESS;
}

static tSIRF_RESULT sirf_configure_sensors(void ) 
{
    D("%s: called", __FUNCTION__);

    m_sensCfg.numSensors = 0;

    FILE *fin = fopen( m_sensors_cfg_name, "r" );
    if( NULL == fin ) {
        D("Sensor file %s not found", m_sensors_cfg_name );
        D("Sensors will be disabled" );
        return SIRF_FAILURE;
        }

    tSIRF_RESULT ret = SIRF_LoadSensorsConfiguration( fin, & m_sensCfg );

    fclose( fin );

    if( SIRF_SUCCESS != ret ) {
        m_sensCfg.numSensors = 0;
        D("Error loading sensor configuration file %s", m_sensors_cfg_name );
        D("Sensors will be disabled" );
        return SIRF_FAILURE;
        }
   return SIRF_SUCCESS;
}

static tSIRF_RESULT externalSensorsParse_Callback( tSIRF_UINT8 *PktBuf, 
                                                 tSIRF_UINT32 PktLen, 
                                                 tSIRF_ParserType Parser )
{
   tSIRF_UINT32 mid = 0;              /* message id */
   tSIRF_UINT32 len = 0;              /* message length */
   tSIRF_RESULT tRet = SIRF_SUCCESS;

   /* Validate arguments */
   if ( !PktBuf || (0 == PktLen) )
   {
      return SIRF_FAILURE;
   }

   /* Non SL. Check for SSB or NMEA */
   if ( PARSER_SSB == Parser ) 
   {
      tSIRF_UINT32 option = 1;
      tSIRF_RESULT res = SIRF_CODEC_SSB_Decode(PktBuf, PktLen, &mid, msg, &len,&option);
      if ( res == SIRF_SUCCESS)
      {
         if( ! m_send_data_msg ) 
            return SIRF_SUCCESS;    // do not process messages before sensors are enabled

         // using SiRFNav_Input is a preffered way but it results in huge amount of data coming through input queue
         // therefore do not use it for data readings
         switch( mid ) {
            case SIRF_MSG_SSB_SENSOR_CONFIG:
            case SIRF_MSG_SSB_SENSOR_SWITCH:
               SiRFNav_Input(mid, msg, len); 
               break;

            case SIRF_MSG_SSB_SENSOR_READINGS:
               (void)HandleRawSensorDataInput((tSIRF_MSG_SSB_SENSOR_READINGS*)msg);
               break; 
         }

      } else {
         D("Error unpacking SSB message(%d, %d): %d\n", SIRF_GET_MID(mid),SIRF_GET_SUB_ID(mid),(int)
         res);
      }
   }

   return tRet;
}

static tSIRF_RESULT SiRF_TCPIP_Callback(tSIRF_UINT8 *buf, tSIRF_UINT32 len )
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

int sirf_gps_tcp_start(void)
{
    char tmp[ 5 ];
    unsigned int i;

    load_options();
    for( i = 0; i < LOCAL_OPT_SIZE; i++ ) {
       D("%s = %d",m_options[i].key, *m_options[i].value );
    }

    if( m_disable ||                                  // option from the file 
        SIRF_SUCCESS != sirf_configure_sensors() ||   // wrong or missing configuration file 
        0 == m_sensCfg.numSensors ) {                 // wrong number of sensors
        return SIRF_FAILURE;
    }

    // create TCP IP listener thread
    tSIRF_RESULT result = SIRF_EXT_TCPIP_Create(NULL, 7555);
    D("Create TCPIP listener. Result: %d\n", (int)result );

    /* Register the callback for incoming data to be parsed */
    SIRF_EXT_TCPIP_Callback_Register( SiRF_TCPIP_Callback );
    
    // register handler for incoming messages
    SIRF_PROTO_Parse_Register( externalSensorsParse_Callback );
   
    m_send_confing_msg = 1;
    m_send_data_msg = 0;

    return SIRF_SUCCESS;
}

int sirf_gps_tcp_stop()
{
    if( ! m_disable && 0 != m_sensCfg.numSensors ) {
       tSIRF_RESULT result = SIRF_EXT_TCPIP_Delete();
       D("Deleted TCPIP listener. Result: %d\n", (int)result );
    }
    m_send_confing_msg = m_send_data_msg = 0;
    return 0;
}


