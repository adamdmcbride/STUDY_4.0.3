/**
 * @addtogroup ee_download
 * @ingroup SiRFInstantFix
 * @{
 */

/**
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006- 2010 by SiRF Technology, a CSR plc Company.
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
 * @file: ee_download_direct.h
 * @brief: Definition of the interfaces using direct download function.
 */

#ifndef __EE_DOWNLOAD_DIRECT_H__
#define __EE_DOWNLOAD_DIRECT_H__
#include "sirf_types.h"
#include "ee_download_ui.h"
/**
 * Used to represent the state of an DIRECT DWNLD MODULE.
 */
typedef enum {
    STATE_EE_DOWNLOAD_DIRECT_STOPPED  = 0,            /*  DIRECT DWNLD module stopped successfully      */
    STATE_EE_DOWNLOAD_DIRECT_STARTED                  /*  DIRECT DWNLD module started successfully          */
}stateDirectDwnldMod;

/* Enter C naming convention */
#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/
/**
 * @fn tEE_DOWNLOAD_STATUS Direct_Download_Start(tSIRF_UINT8 * pEEFilebuffer,
 *                                          tSIRF_CHAR   * pServerAddress,
 *                                          tSIRF_UINT16 serverPort,
 *                                          tSIRF_CHAR   *pServerFile,
 *                                          tSIRF_CHAR   *pServerAuth,
 *                                          tSIRF_UINT8 serverFileFormat )
 * @brief Direct_Download_Start
 *
 * @description To start the EE Download using sockets.
 *              pass all valid required paramaters.
 *
 * @param[in] pServerAddress Server address
 * @param[in] serverPort server port number
 * @param[in] pServerFile  pointer to server file name
 * @param[in] pServerAuth server authentication code
 * @param[in] serverFileFormat file format downloaded from server
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code
 *         defined in tEE_DOWNLOAD_STATUS
 *
 */
tEE_DOWNLOAD_STATUS Direct_Download_Start( tSIRF_UINT8 * pEEFilebuffer,
    tSIRF_CHAR   * pServerAddress,
    tSIRF_UINT16 serverPort,
    tSIRF_CHAR   * pServerFile,
    tSIRF_CHAR   * pServerAuth,
    tSIRF_UINT8 serverFileFormat );

/**
 * @fn tEE_DOWNLOAD_STATUS Direct_Download_Stop(tSIRF_VOID)
 * @brief Direct_Download_Stop
 *
 * @description To Stop the download using direct connection.
 *
 * @return tEE_DOWNLOAD_STATUS with EE_DOWNLOAD_SUCCESS or error code
 *         defined in tEE_DOWNLOAD_STATUS
 */
tEE_DOWNLOAD_STATUS Direct_Download_Stop(tSIRF_VOID);
/* Leave C naming convention */
#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif //__EE_DOWNLOAD_DIRECT_H__

/**
 * @}
 * End of file.
 */

