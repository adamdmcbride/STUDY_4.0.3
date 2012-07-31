/**
 * @addtogroup platform_src_sirf_sirfnaviii
 * @{
 */

 /**************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2005-2010 by SiRF Technology, a CSR plc Company.       *
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
 * FILE: sirfnav_ui_io.h
 *
 ***************************************************************************/

#ifndef __SIRFNAV_UI_IO_H__
#define __SIRFNAV_UI_IO_H__

#include "sirf_errors.h"
#include "sirf_types.h"



/* Prototypes --------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

tSIRF_RESULT SiRFNav_Input( tSIRF_UINT32 message_id,
                            tSIRF_VOID  *message_structure,
                            tSIRF_UINT32 message_length );

/***************************************************************************
* DESCRIPTION: Output function for messages from the SiRFNav library to
*              the customer.
*
* PARAMETERS:  message_id        - message identification number
*              message_structure - pointer to the message data
*              message_length    - length of the message in bytes
*
* RETURN:      None.
*
* NOTES:       SiRFNav_Output() is a customer-implemented function.
*              SiRFNav_Output() should only be called from the
*              SiRF NAV thread.
***************************************************************************/
tSIRF_VOID SiRFNav_Output( tSIRF_UINT32 message_id,
                           tSIRF_VOID  *message_structure,
                           tSIRF_UINT32 message_length );

/* This is an input function used to feed data coming from tracker
 * to SiRFNav for processing */
tSIRF_RESULT SiRFTrack_Input  (tSIRF_UINT8 *data, tSIRF_UINT32 length);
/* This is an output function that needs to be implemented by the application layer.
 * This function will be called by SiRFNav whenever it has data that needs to be
 * sent to tracker */
tSIRF_RESULT SiRFTrack_Output (const tSIRF_UINT8 *data, tSIRF_UINT32 length);
/* This an input function that is used by the application to get and set
 * configuration parameters e.g., UART baud rate, flow control etc., */
tSIRF_RESULT SiRFTrack_Control (tSIRF_UINT32 opcode, tSIRF_VOID *param);
/* This is an output function that needs to be implemented by the application layer.
 * The purpose of this function is to let the application know of any events
 * and also to let application excute configuration changes on tracker port as
 * required by SiRFNav. e.g., if the host needs to change the baud rate after
 * exchanging the initial messages, it can notify the application using this function */
tSIRF_RESULT SiRFTrack_Notify (tSIRF_UINT32 opcode, tSIRF_VOID *param);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SIRFNAV_UI_IO_H__ */

/**
 * @}
 */

