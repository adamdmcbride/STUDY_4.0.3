/**
 * @addtogroup platform_src_sirf_util_config_parser
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2010 by SiRF Technology, a CSR plc Company.            *
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.'s confidential and       *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.'s  express written            *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 ***************************************************************************
 *
 * MODULE:      HOST
 *
 * FILENAME:    sirf_sensor_config_parser.c
 *
 * DESCRIPTION: Parses the SiRFNav sensor configuration file.
 *
 ***************************************************************************/

/***************************************************************************
 * Include Files
 ***************************************************************************/
#include <stdio.h>

#include "SiRFNav.h"
#include "sirf_pal.h"
#include "sirf_types.h"
#include "user_interface.h"

/***************************************************************************
 * Defines
 ***************************************************************************/

/***************************************************************************
 * Typedefs
 ***************************************************************************/

/***************************************************************************
 * Variables
 ***************************************************************************/

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/
static tSIRF_BOOL SIRF_SensConfigReadLine(FILE* hFile, tSIRF_CHAR* line, tSIRF_UINT32 max);
 
/***************************************************************************
 * Functions
 ***************************************************************************/

/**
 * @brief Reads one line from the Sensors Configuration File
 *        This function will read 1 line from the file (pointed to by 'hFile'),
 *        up to the maximumlength specified by 'max'. A line end is indicated in the
 *        file by the new line character. Lines that are empty or start with the comment
 *        indicator "//" are ignored. The read line is stored in the buffer indicated by 'line'.
 */
static tSIRF_BOOL SIRF_SensConfigReadLine(FILE* hFile, tSIRF_CHAR* line, tSIRF_UINT32 max)
{
   /* local definitions */
   #define NULL_CHAR                '\0'
   #define LINE_FEED_CHAR           '\n'
   #define CARRIAGE_RETURN_CHAR     '\r'
   #define SPACE_CHAR               ' '
   #define COMMENT1_CHAR            '/'
   #define COMMENT2_CHAR            '/'
   
   #define BYTES_TO_READ_PER_OP     (1)
   #define ITEMS_TO_READ_PER_OP     (1)
   #define EXPECTED_BYTES_READ      (BYTES_TO_READ_PER_OP * ITEMS_TO_READ_PER_OP)

   tSIRF_UINT32 read;

   do
   {
      /* read 1 full line of the file */
      read = 0;
      if (EXPECTED_BYTES_READ != fread(
                                    &(line[read]),
                                    BYTES_TO_READ_PER_OP,
                                    ITEMS_TO_READ_PER_OP,
                                    hFile))
      {
         return SIRF_FALSE;
      }

      while((LINE_FEED_CHAR != line[read]) && (read < max))
      {
         if((read > 0) || (SPACE_CHAR != line[read]))
         {
            read++;
         }

         if (EXPECTED_BYTES_READ != fread(
                                    &(line[read]),
                                    BYTES_TO_READ_PER_OP,
                                    ITEMS_TO_READ_PER_OP,
                                    hFile))
         {
            return SIRF_FALSE;
         }
      }
   }
   while (((line[0] == COMMENT1_CHAR) && (line[1] == COMMENT2_CHAR)) ||
          ( line[0] == CARRIAGE_RETURN_CHAR)                         ||
          ( line[0] == LINE_FEED_CHAR));

   return SIRF_TRUE;
}



/**
 * @brief Loads the Sensor Configuration Information from a text config file.
 *        This function will read the complete sensor configuration for the system from
 *        the file indicated by file_handle. This information is the dsame as the contents of the
 *        tSIRF_MSG_SSB_SENSOR_CONFIG message, and should be provided in the file in the same
 *        order and format. The read configuration is stored in the buffer indicated by 'sensCfg'.
 */
tSIRF_RESULT SIRF_LoadSensorsConfiguration(FILE* file_handle, tSIRF_MSG_SSB_SENSOR_CONFIG *sensCfg)
{
   tSIRF_CHAR line[300];
   tSIRF_RESULT res = SIRF_SUCCESS;
   tSIRF_UINT8 x, y;
   tSIRF_UINT16 value1, value2;

   if(NULL == sensCfg)
   {
      res = SIRF_FAILURE;
   }

   /* numSensors */
   if(SIRF_SUCCESS == res)
   {
      if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
         (sscanf(line, "0x%hX", &value1) != 1))
      {
         GUI_Print("Error while reading field: [numSensors]\n");
         res = SIRF_FAILURE;
      }
      else
      {
         sensCfg->numSensors = (tSIRF_UINT8)value1;
      }
   }

   /* i2cSpeed */
   if(SIRF_SUCCESS == res)
   {
      if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
         (sscanf(line, "0x%hX", &value1) != 1))
      {
         GUI_Print("Error while reading field: [i2cSpeed]\n");
         res = SIRF_FAILURE;
      }
      else
      {
         sensCfg->i2cSpeed = (tSIRF_UINT8)value1;
      }
   }

   for (x = 0; (SIRF_SUCCESS == res) && (x < sensCfg->numSensors); x++)
   {
      /* i2cAddress */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [i2cAddress(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].i2cAddress = (tSIRF_UINT16)value1;
         }
      }

      /* sensorType */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [sensorType(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].sensorType = (tSIRF_UINT8)value1;
         }
      }

      /* initTime */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [initTime(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].initTime = (tSIRF_UINT8)value1;
         }
      }

      /* nBytesResol */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [nBytesResol(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].nBytesResol = (tSIRF_UINT8)value1;
         }
      }

      /* sampRate */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [sampRate(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].sampRate = (tSIRF_UINT8)value1;
         }
      }

      /* sendRate */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [sendRate(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].sendRate = (tSIRF_UINT8)value1;
         }
      }

      /* decmMethod */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [decmMethod(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].decmMethod = (tSIRF_UINT8)value1;
         }
      }

      /* acqTime */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [acqTime(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].acqTime = (tSIRF_UINT8)value1;
         }
      }

      /* numReadReg */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [numReadReg(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].numReadReg = (tSIRF_UINT8)value1;
         }
      }

      /* measState */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [measState(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
             sensCfg->Sensors[x].measState = (tSIRF_UINT8)value1;
         }
      }

      for (y = 0; (SIRF_SUCCESS == res) && (y < sensCfg->Sensors[x].numReadReg); y++)
      {
         /* readOprMethod, dataReadReg */
         if(SIRF_SUCCESS == res)
         {
            if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
               (sscanf(line, "0x%hX, 0x%hX", &value1, &value2) != 2))
            {
               GUI_Print("Error while reading fields: [READ_REG: readOprMethod, dataReadReg (%x)]\n", x);
               res = SIRF_FAILURE;
            }
            else
            {
               sensCfg->Sensors[x].sensorReadReg[y].readOprMethod = (tSIRF_UINT8)value1;
               sensCfg->Sensors[x].sensorReadReg[y].dataReadReg   = (tSIRF_UINT8)value2;
            }
         }
      }

      /* pwrCtrlReg */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [pwrCtrlReg(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].pwrCtrlReg = (tSIRF_UINT8)value1;
         }
      }

      /* pwrOffSetting */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [pwrOffSetting(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].pwrOffSetting = (tSIRF_UINT8)value1;
         }
      }

      /* pwrOnSetting */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [pwrOnSetting(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].pwrOnSetting = (tSIRF_UINT8)value1;
         }
      }

      /* numInitReadReg */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [numInitReadReg(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].numInitReadReg = (tSIRF_UINT8)value1;
         }
      }

      for (y = 0; (SIRF_SUCCESS == res) && (y < sensCfg->Sensors[x].numInitReadReg); y++)
      {
         /* address, nBytes */
         if(SIRF_SUCCESS == res)
         {
            if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
               (sscanf(line, "0x%hX, 0x%hX", &value1, &value2) != 2))
            {
               GUI_Print("Error while reading fields: [INIT_REG: address, nBytes (%x)]\n", x);
               res = SIRF_FAILURE;
            }
            else
            {
               sensCfg->Sensors[x].sensorInitReg[y].address = (tSIRF_UINT8)value1;
               sensCfg->Sensors[x].sensorInitReg[y].nBytes  = (tSIRF_UINT8)value2;
            }
         }
      }

      /* numCtrlReg */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [numCtrlReg(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].numCtrlReg = (tSIRF_UINT8)value1;
         }
      }

      /* ctrlRegWriteDelay */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [ctrlRegWriteDelay(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->Sensors[x].ctrlRegWriteDelay = (tSIRF_UINT8)value1;
         }
      }

      for (y = 0; (SIRF_SUCCESS == res) && (y < sensCfg->Sensors[x].numCtrlReg); y++)
      {
         /* address, value */
         if(SIRF_SUCCESS == res)
         {
            if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
               (sscanf(line, "0x%hX, 0x%hX", &value1, &value2) != 2))
            {
               GUI_Print("Error while reading fields: [CTRL_REG: address, value (%x)]\n", x);
               res = SIRF_FAILURE;
            }
            else
            {
               sensCfg->Sensors[x].sensorCtrlReg[y].address = (tSIRF_UINT8)value1;
               sensCfg->Sensors[x].sensorCtrlReg[y].value   = (tSIRF_UINT8)value2;
            }
         }
      }

   }

   /* processingRate */
   if(SIRF_SUCCESS == res)
   {
      if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
         (sscanf(line, "0x%hX", &value1) != 1))
      {
         GUI_Print("Error while reading field: [processingRate]\n");
         res = SIRF_FAILURE;
      }
      else
      {
         sensCfg->processingRate = (tSIRF_UINT8)value1;
      }
   }

   for (x = 0; (SIRF_SUCCESS == res) && (x < sensCfg->numSensors); x++)
   {
      /* zeroPointVal */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [zeroPointVal(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->sensorScaleZeroPointVal[x].zeroPointVal = (tSIRF_UINT16)value1;
         }
      }

      /* scaleFactor */
      if(SIRF_SUCCESS == res)
      {
         if((SIRF_TRUE != SIRF_SensConfigReadLine(file_handle, line, sizeof(line))) ||
            (sscanf(line, "0x%hX", &value1) != 1))
         {
            GUI_Print("Error while reading field: [scaleFactor(%x)]\n", x);
            res = SIRF_FAILURE;
         }
         else
         {
            sensCfg->sensorScaleZeroPointVal[x].scaleFactor = (tSIRF_UINT16)value1;
         }
      }
   }

   return res;
}

/**
 * @brief Sends the Sensor Configuration and Sensor Enable messages to the Host.
 *        This function sends the Sensor Config and Sensor enable messages to the Host based on
 *        the configuration information present in the sensCfg buffer. This function must only be
 *        invoked if a Sensor Configuration file with a valid configuration was provided at the
 *        time the application was launched.
 */
void SIRF_SendSensorEnableMsgs(tSIRF_MSG_SSB_SENSOR_CONFIG *sensCfg)
{
   tSIRF_MSG_SSB_SENSOR_SWITCH enable;

   /* TODO: this has to be moved to the host application */
   SIRF_PAL_OS_MEM_Init();


   SiRFNav_Input((SIRF_LC_SSB << 16) | SIRF_MSG_SSB_SENSOR_CONFIG,
                 sensCfg,
                 sizeof(tSIRF_MSG_SSB_SENSOR_CONFIG));

   /* Enable Sensors and Receiver STate Notification meessages */
   enable.sensorSetState = SENS_STATE_ENABLE | SENS_STATE_NOTIFY;


   SiRFNav_Input((SIRF_LC_SSB << 16) | SIRF_MSG_SSB_SENSOR_SWITCH,
                 &enable,
                 sizeof(enable));

}

/**
 * @}
 */
