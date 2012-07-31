/**
 * @addtogroup clm_main
 * @ingroup SiRFInstantFix
 * @{
 */

/*
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006- 2009 by SiRF Technology, Inc.  All rights reserved.
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
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  Archive:$File: //customs/customer/Marvell-U1/sirf/Software/sirf/SiRFIF/include/clm_main/clm_ui.h $
 *
 *  Last modified by: $Author: kt01 $
 *
 *  $DateTime: 2011/07/29 13:26:23 $
 *
 *  Revision:$Revision: #1 $
 */

/**
 * @file: clm_ui.h
 * @brief: Definition of the CLM_Common interface to User.
 */

#ifndef __CLM_UI_H__
#define __CLM_UI_H__

#include "sirf_types.h"
#include "clm_structs.h"

/* Enter C naming convention */
#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/


/**
 * @fn extern tSIRF_INT32 CLM_ConsolePrintf( tSIRF_CHAR *pFormat, ... )
 * @brief CLM_ConsolePrintf
 *
 * @description To be implemented by the User application to output formatted
 *              string on console.
 *              Can be implemented as "Empty function" if debug console is not
 *              requested by user application during CLM_Configure.
 *
 * @param[in] pFormat  Log message
 * @return tSIRF_INT32 with 0 for success or -1 for failure
 *
 */
extern tSIRF_INT32 CLM_ConsolePrintf( tSIRF_CHAR *pFormat, ... );


/**
 * @fn tCLM_STATUS CLM_Configure(tCLM_CONFIG *  pConfigData)
 * @brief CLM_Configure
 *
 * @description To specify the CLM configuration options (SGEE/CGEE/SGEE-CGEE
 *              mode of operation,OTA and storage file format, SGEE and CGEE
 *              age limit and debug log options)
 *
 * @param[in] pConfigData Pointer to configuration structure filled with the required
 *                        CLM configuration options
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_Configure(
    tCLM_CONFIG *  pConfigData
    );

/**
 * @fn tCLM_STATUS CLM_Start()
 * @brief CLM_Start
 *
 * @description To start the CLM operation based on the configurations
 *              specified with last CLM_Configure call.
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_Start(tSIRF_VOID);


/**
 * @fn tCLM_STATUS CLM_Stop()
 * @brief CLM_Stop
 *
 * @description To stop the CLM operation .
 *
 * @return tCLM_STATUS with CLM_SUCCESS\CLM_COMMON_NOT_RUNNING
 *
 */
tCLM_STATUS CLM_Stop(tSIRF_VOID);


/**
 * @fn tCLM_STATUS CLM_GetEEAiding(tSIRF_UINT8  gpsTimeValid,tSIRF_UINT16 *pGpsWeek,
 *                  tSIRF_UINT32 gpsTow,tSIRF_UINT32 satMask,tCLM_EE_DATA *pEEData)
 * @brief CLM_GetEEAiding
 *
 * @description To get EE data for specified satellites, GPS week and TOW.
 *
 * @param[in] gpsTimeValid gpsTimeValid flag( Bit0 Set: GPS week is valid,
 *                                            Bit1 Set: GPS TOW is valid)
 * @param[in/out] pGpsWeek Pointer to GPS week number, may get updated by
 *                         using host RTC offst and valid TOW. Caller must
 *                         use updated GPS week for sending response to receiver.
 * @param[in] gpsTow  GPS TOW
 * @param[out] satMask satellite mask for which aiding is requested
 *                    (set bit 0: sat 1, set bit 31 : sat 32)
 * @param[out] pEEData  Array of EE data structures, must be allocated for EE data
 *                      of MAX_NUM_SV satellites.All zero fields for the EE data
 *                      block for which EE is not available.
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_GetEEAiding(
    tSIRF_UINT8  gpsTimeValid,
    tSIRF_UINT16 *pGpsWeek,
    tSIRF_UINT32 gpsTow,
    tSIRF_UINT32 satMask,
    tCLM_EE_DATA *pEEData
    );

/**
 * @fn tCLM_STATUS CLM_ClearEE()
 * @brief CLM_ClearEE
 *
 * @description To clear EE data for all satellites BE,SGEE and CGEE,
 *              can only be called before CLM is started.
 *
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_ClearEE(tSIRF_VOID);

/**
 * @fn tCLM_STATUS CLM_GetIonoAiding(tSIRF_UINT8  gpsTimeValid,
 *                                  tSIRF_UINT16 *pGpsWeek,tSIRF_UINT32 gpsTow,
 *                                  tCLM_IONO_DATA *pIonoData )
 * @brief CLM_GetIonoAiding
 *
 * @description To get Iono data for specified GPS week and TOW.
 *
 * @param[in] gpsTimeValid GPS time Valid flag( Bit0 Set: GPS week is valid,
 *                                              Bit1 Set: GPS TOW is valid)
 * @param[in/out] pGpsWeek Pointer to GPS week number, may get updated by
 *                         using host RTC offst and valid TOW.Caller must
 *                         use updated GPS week for sending response to receiver.
 * @param[in] gpsTow  GPS TOW
 * @param[in] pIonoData pointer to structure tCLM_IONO_DATA.It must be
 *                      allocated by caller.
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_GetIonoAiding(
    tSIRF_UINT8  gpsTimeValid,
    tSIRF_UINT16 *pGpsWeek,
    tSIRF_UINT32 gpsTow,
    tCLM_IONO_DATA *pIonoData
    );

/**
 * @fn tCLM_STATUS CLM_SendClockBias(tSIRF_UINT8  numSAT,
 *                                   tCLM_CLK_BIAS_ADJ * pClkBiasAdj)
 * @brief CLM_SendClockBias
 *
 * @description To send the clock bias and adjustments for specified satellites.
 *
 * @param[in] numSAT Number of satellites for which clock bias and adjustment
 *                   is passed.Valid values can be from 1 to MAX_NUM_SV
 * @param[in] pClkBiasAdj Array of structures containing clock bias and adjustment values.
 *                        It must be allocated for numSAT tCLM_CLK_BIAS_ADJ structures
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_SendClockBias(
    tSIRF_UINT8  numSAT,
    tCLM_CLK_BIAS_ADJ * pClkBiasAdj
    );

/**
 * @fn tCLM_STATUS CLM_GetSGEEAge(tSIRF_UINT32 *pAge, tSIRF_UINT32 *pPrediction,
 *                                tSIRF_UINT8 satId)
 * @brief CLM_GetSGEEAge
 *
 * @description To get EE age for the requested number of satellites.
 *
 * @param[out] pAge pointer to age.
 * @param[out] pPrediction pointer to prediction.
 * @param[in] satId  Satellite id (1 based),meaningful only in case of FF1.
 *                   Valid values can be from 1 to MAX_NUM_SV
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
 tCLM_STATUS CLM_GetSGEEAge( tSIRF_UINT32 *pAge, tSIRF_UINT32 *pPrediction, tSIRF_UINT8 satId);


/**
 * @fn tCLM_STATUS CLM_GetEEAge(tSIRF_UINT8  numSAT,tCLM_EE_AGE *pEEAge)
 * @brief CLM_GetEEAge
 *
 * @description To get EE age for the requested number of satellites.
 *              PRN no. shall be updated in every tCLM_EE_AGE structure before
 *              calling this function.
 *
 * @param[in] numSAT Number of satellites for which EE age is required.
 *                   Valid values can be from 1 to MAX_NUM_SV
 * @param[in\out] pEEAge Array of EE age structures,to store EE age of requested SAT.
 *                       PRN number must be filled inside each tCLM_EE_AGE structure
 *                       before calling.Type and validity of age is specified
 *                       inside the EE age structure.It must be allocated for
 *                       numSAT times tCLM_EE_AGE structure.
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_GetEEAge(
    tSIRF_UINT8  numSAT,
    tCLM_EE_AGE *pEEAge
    );

/**
 * @fn tCLM_STATUS CLM_SendIntegrityWarning(tCLM_INTEGRITY_WARNING *pIntegWarning)
 * @brief CLM_SendIntegrityWarning
 *
 * @description To send the integrity warning for satellites.
 *
 * @param[in] pIntegWarning Pointer to structure containing position,
 *                          clock and health validity bitmaps.
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_SendIntegrityWarning(
   tCLM_INTEGRITY_WARNING *pIntegWarning
   );


/**
 * @fn tCLM_STATUS CLM_SendSubFrame(tCLM_SF_DATA * pSubFrameData)
 * @brief CLM_SendSubFrame
 *
 * @description To send NAV sub frame data (50bps data) to generate the CGEE.
 *
 * @param[in] pSubFrameData Pointer to Sub frame data.
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_SendSubFrame(
   tCLM_SF_DATA * pSubFrameData
   );


/**
 * @fn tCLM_STATUS CLM_SendIONO(tCLM_IONO_DATA  * pIonoData)
 * @brief CLM_SendIONO
 *
 * @description To send ionosphere correction data.
 *
 * @param[in] pIonoData Pointer to Ionospheric data.
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_SendIONO(
   tCLM_IONO_DATA  * pIonoData
   );


/**
 * @fn tCLM_STATUS CLM_SendBE(tSIRF_UINT8 numSat,tSIRF_UINT8 gpsTimeValid,
 *                            tSIRF_UINT16 gpsWeekExtended,tSIRF_UINT32 gpsTow,
 *                            tCLM_BE_DATA  * pBEData)
 * @brief CLM_SendBE
 *
 * @description To send broadcast ephemeris data.
 *
 * @param[in] numSat Number of satellites .
 *                   Valid values can be from 1 to MAX_NUM_SV
 * @param[in] gpsTimeValid gpsTimeValid for valid week and tow
 *                        ( Bit0 Set: GPS week is valid,Bit1 Set: GPS TOW is valid)
 * @param[in] gpsWeekExtended  extended gpsweek number
 * @param[in] gpsTow gps tow
 * @param[in] pBEData pointer to array of BE data allocated for
 *                    numSat tCLM_BE_DATA structures
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_SendBE(
   tSIRF_UINT8 numSat,
   tSIRF_UINT8 gpsTimeValid,
   tSIRF_UINT16 gpsWeekExtended,
   tSIRF_UINT32 gpsTow,
   tCLM_BE_DATA  * pBEData
   );


/**
 * @fn tCLM_STATUS CLM_SendEEFile(tSIRF_UINT8 *  pSGEEFileData,
 *                                tSIRF_UINT32  fileLengthSGEE)
 * @brief CLM_SendEEFile
 *
 * @description To send SGEE File data.
 *
 * @param[in] pSGEEFileData Pointer to memory containing complete SGEE file’s data bytes.
 * @param[in] fileLengthSGEE Complete SGEE file data length in bytes.
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_SendEEFile(
   tSIRF_UINT8 *  pSGEEFileData,
   tSIRF_UINT32  fileLengthSGEE
   );

/**
 * @fn tCLM_STATUS CLM_GetLibVersion(tSIRF_CHAR  version[MAX_CLM_VERSION_STRING])
 * @brief CLM_GetLibVersion
 *
 * @description To get version of the CLM library.
 *
 * @param[in] version[MAX_CLM_VERSION_STRING] version of CLM LIbrary.
 * @return tCLM_STATUS with CLM_SUCCESS or error code defined in tCLM_STATUS
 *
 */
tCLM_STATUS CLM_GetLibVersion(
    tSIRF_CHAR  version[MAX_CLM_VERSION_STRING]
    );

/**
* @fn tCLM_STATUS CLM_GetStatus( tCLM_CGEE_STATUS *pStatus)
* This function returns the status of the SIF CGEE Engine and the predicted satellite
* mask to the user.
*
* @param[out] status  pointer to a tCLM_CGEE_STATUS structure
*            containing the cgee engine state and prediction mask of all satellites.
* @return CLM_SUCCESS on success, on error CLM_COMMON_NOT_RUNNING
 *                                         CLM_GENERIC_FAILURE
 *                                         CLM_COMMON_INVALID_PARAM
 *                                         CLM_COMMON_INVALID_OPS
*/
tCLM_STATUS CLM_GetStatus(
    tCLM_CGEE_STATUS *pStatus
    );


/* Leave C naming convention */
#ifdef __cplusplus
}
#endif /*__cplusplus*/


#endif //__CLM_UI_H__

/**
 * @}
 * End of file.
 */

