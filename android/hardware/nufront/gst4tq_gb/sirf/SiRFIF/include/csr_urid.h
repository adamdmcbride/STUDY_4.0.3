/*
 *                   CSR Plc Software
 *
 *    Copyright (c) 2011 by CSR plc. All rights reserved.
 *
 *    This Software is protected by United States copyright laws and
 *    international treaties.  You may not reverse engineer, decompile
 *    or disassemble this Software.
 *
 *    WARNING:
 *    This Software contains CSR plc’s confidential and
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this
 *    Software without CSR plc’s  express written permission.
 *    Use of any portion of the contents of this Software is subject to
 *    and restricted by your signed written agreement with CSR plc.
 */

/**
 * @file   csr_urid.h
 *
 * @brief  CSR URID API
 */

#ifndef __CSR_URID_H__
#define __CSR_URID_H__

#include "sirf_types.h"

#define OEM_SUB_ID_SIZE  4
#define DEVICE_ID_SIZE   8

#define CSR_URID_SIZE          25
#define CSR_URID_STRING_SIZE  (2 * CSR_URID_SIZE + 1)

#define URID_SUCCESS 0
#define URID_FAILURE 1

#ifdef __cplusplus
extern "C" {
#endif

tSIRF_UINT32 CSR_URID_Set_OEM_Info( tSIRF_UINT8 *oem_sub_id, tSIRF_UINT32 oem_sub_id_size, tSIRF_UINT8 *device_id, tSIRF_UINT32 device_id_size );
tSIRF_UINT32 CSR_URID_Get_URID_String( char *urid_string, tSIRF_UINT32 size );

#ifdef __cplusplus
}
#endif

#endif /* __CSR_URID_H__ */
