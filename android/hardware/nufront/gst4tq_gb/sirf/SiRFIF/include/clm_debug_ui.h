/**
 * @addtogroup clm_main
 * @ingroup SiRFInstantFix
 * @{
 */

/*
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006-2010 by SiRF Technology, a CSR plc Company.
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
 * @file: clm_debug_ui.h
 * @brief: Definition of the CLM_Debug interface functions.
 */

#ifndef __CLM_DEBUG_UI_H__
#define __CLM_DEBUG_UI_H__

#include "sirf_types.h"

/* These two macros can only be used for setting the CLM config options
 * during CLM Configure to either enable all or disable all the module debug messages */
#define CLM_DLOG_MOD_ID_ALL                 (1<<30)
#define CLM_DLOG_MOD_ID_NONE                (0x80000000)/*(1<<31)*/
/*************************************************************************************/


/* Module Id which can be used in set config options */
#define CLM_DLOG_MOD_ID_COMMON              0x1
#define CLM_DLOG_MOD_ID_STORAGE             0x2
#define CLM_DLOG_MOD_ID_SGEE                0x3
#define CLM_DLOG_MOD_ID_CGEE                0x4
#define CLM_DLOG_MOD_ID_MTL                 0x5
#define CLM_DLOG_MOD_ID_EEDOWNLOADER        0x6
#define CLM_DLOG_MOD_ID_EXTERN              0x7
#define CLM_DLOG_MOD_MAX                   (10)
/* Severity Levels which can be used in set config options */
#define CLM_DLOG_SEVERITY_LEVEL_MAX       0x3
#define CLM_DLOG_SEVERITY_LEVEL_2         0x2
#define CLM_DLOG_SEVERITY_LEVEL_1         0x1
#define CLM_DLOG_SEVERITY_LEVEL_MIN       0x0

#define CLM_DLOG_MOD_ID_MASK                 0x3FF
#define CLM_DLOG_MOD_ID_BITS                 10
#define CLM_DLOG_SEVERITY_MASK               0x3
#define CLM_DLOG_SEVERITY_BITS               2



/* Enter C naming convention */
#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/

#define DEBUG_OPTION(id, level)    id , level

/*
   Bit10-29: Corresponding 2 bit tells severity for the sub module
   Bit0-9:  Corresponding bit for sub Module log (1 : enabled) (0: Disabled )
   Bit30: 1->logging is enable for all modules with common severity level (bit:10-11)
         0->logging is enabled as per module ID(bit:0-9) with severity level (bit:10-29)
   Bit31: 1->logging is disabled for all modules
         0->logging is enabled as per module ID(bit:0-9) with severity level (bit:10-29 or 30)
   Default: 0x0
*/
#define SET_DEBUG_OPTION(id, level)   ((CLM_DLOG_MOD_ID_NONE == (CLM_DLOG_MOD_ID_NONE & (id)))? 0 :\
                                       ((CLM_DLOG_MOD_ID_ALL == (CLM_DLOG_MOD_ID_ALL & (id)))? \
                                       ((id) | ((level) << CLM_DLOG_MOD_ID_BITS)):\
                                       (( (1 << ((id) -1 )) | ( (level) << ((((id) -1) * CLM_DLOG_SEVERITY_BITS) + CLM_DLOG_MOD_ID_BITS)) ))))

/**
 * @fn void CLMDebug_Log(tSIRF_UINT32 moduleID,tSIRF_UINT32 level,
 *                                 tSIRF_VOID * pFormattedString,...)
 * @brief Message Logging function.
 *
 * @description To log the debug messages in log file or console
 *               depending upon the selected CLM configuration.
 *
 * @param[in] moduleID Module ID
 * @param[in] level severity level of message
 * @param[in] pFormattedString  Log message
 * @return    none
 *
 */

#ifdef SIF_ENABLE_LOGGING
void CLMDebug_Log(
    tSIRF_UINT32 moduleID,
    tSIRF_UINT32 level,
    tSIRF_VOID * pFormattedString,
    ...
    );

#else
   #define CLMDebug_Log(moduleID, ...)

#endif /* #ifdef SIF_ENABLE_LOGGING */


/* Leave C naming convention */
#ifdef __cplusplus
}
#endif /*__cplusplus*/


#endif //__CLM_DEBUG_UI_H__

/**
 * @}
 * End of file.
 */

