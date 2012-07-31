/**
 * @addtogroup diff_to_ee_converter
 * @ingroup SiRFInstantFix
 * @{
 */

/*
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006-2011 by SiRF Technology, a CSR plc Company.
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
 * @file: convert_ephem.h
 * @brief: Definition of the convertor function to generate the predicted ephemeris file.
 */
#ifndef __CONVERT_EPHEM_H__
#define __CONVERT_EPHEM_H__

#include "clm_structs.h"
#include "sirf_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    CONVERT_DONE = 0,
    CONVERT_PROGRESS = 1,
    CONVERT_ERROR = -1
}tCLM_PREDICT_STATUS;


/**
 * @fn tCLM_PREDICT_STATUS Differential2Predicted(tSIRF_UINT8 *sourceFile,
 *                                        tSIRF_UINT32 sourceFileLen,
 *                                        tSIRF_UINT8 *destinationFile,
 *                                        tSIRF_UINT32 *destinationFileLen,
 *                                        tSIRF_UINT8 format)
 * @brief Differential2Predicted
 *
 * @description   To convert the differential ephemeris format file to
 *                predicted ephemenris file
 *
 * @param[in]     sourceFile source file pointer
 * @param[in]     sourceFileLen input source file length
 * @param[out]    destinationFile destination file pointer,it is assumed
 *                that it will have memory equal to source file length
 * @param[in/out] destinationFileLen: [in]input value is consdered as destination buffer maximum size
 *                                    [out]Output value is converted destination file length  
 * @param[in]     format format of input and output file 0:Invalid format,
 *                   1:OTA_FORMAT_FF1,2:OTA_FORMAT_FF2
 * @return        tCLM_PREDICT_STATUS with CLM_SUCCESS on success
 *                else error code as CLM_GENERIC_FAILURE\CLM_SGEE_INVALIDFILE\
 *                CLM_SGEE_CORRUPTFILE
 *
 *  NOTE:         destinationFileLen value should be atleast MIN_CHUNK_CONVERT_SIZE 
 */
tCLM_PREDICT_STATUS Differential2Predicted(tSIRF_UINT8 *sourceFile,
                                           tSIRF_UINT32 sourceFileLen,
                                           tSIRF_UINT8 *destinationFile,
                                           tSIRF_UINT32 *destinationFileLen,
                                           tSIRF_UINT8 format);

#ifdef EMB_SIF
/**
 * FUNCTION: InitDiff2EEGlobals
 *
 * DESCRIPTION: To initialize global states.
 *
 * PARAMETERS: tSIRF_VOID
 *
 * RETURNS: tSIRF_VOID
 *
 *
 * HISTORY:
 *
 */
tSIRF_VOID InitDiff2EEGlobals(tSIRF_VOID);
#endif /* EMB_SIF */

#ifdef __cplusplus
}
#endif

#endif /* !__CONVERT_EPHEM_H__ */

/**
 * @}
 * End of file.
 */

