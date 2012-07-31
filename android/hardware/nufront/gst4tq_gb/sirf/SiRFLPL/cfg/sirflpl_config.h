/**
 * @addtogroup LPL_cfg_sirflpl30
 * @{
 */

/****************************************************************************
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2005-2009 by SiRF Technology, Inc.  All rights reserved.
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
 ****************************************************************************
 *
 *  $File: 
 *
 *  $DateTime: 
 *
 *  $Revision: 
 *
 ****************************************************************************
 */

/**
 * @file   sirflpl_config.h
 *
 * @brief  SiRFLPL Configuration.
 */

#ifndef LPL_CONFIG_H_INCLUDED
#define LPL_CONFIG_H_INCLUDED

#include "sirflsm_config.h"

/* ----------------------------------------------------------------------------
 *    Preprocessor Definitions
 * ------------------------------------------------------------------------- */

/* Thread IDs */
#define SIRFLPL_THREAD_SLC               (0x0040)
#define SIRFLPL_THREAD_NET               (0x0041)
#define SIRFLPL_THREAD_CP                (0x0042)
#define SIRFLPL_THREAD_TIMER             (0x0043)
#define SIRFLPL_THREAD_GPRS              (0x0044)


/* Thread stack sizes */
#define SIRFLPL_THREAD_SLC_STACK_SIZE               8192
#define SIRFLPL_THREAD_NET_STACK_SIZE               8192
#define SIRFLPL_THREAD_CP_STACK_SIZE                16384
#define SIRFLPL_THREAD_TIMER_STACK_SIZE             2048
#define SIRFLPL_THREAD_GPRS_STACK_SIZE              4096

/* Aggregate stack size for LPL */
#define LPL_STACK_MAX  (SIRFLPL_THREAD_SLC_STACK_SIZE + \
                        SIRFLPL_THREAD_NET_STACK_SIZE + \
                        SIRFLPL_THREAD_CP_STACK_SIZE +  \
                        SIRFLPL_THREAD_TIMER_STACK_SIZE + \
                        SIRFLPL_THREAD_GPRS_STACK_SIZE)

/* Number of mutexes */
#ifdef SIRF_HOST
#define LPL_MUTEX_MAX           12 /* CLM(1) + Heap(2) + Timer(1) + LPL Core */ + SIRFLSM_MUTEX_MAX 
#else
#define LPL_MUTEX_MAX           12 /* CLM(1) + Heap(2) + Timer(1) + LPL Core */ + SIRFLSM_MUTEX_MAX 
#endif

/* Number of semaphores */
#define LPL_SEM_MAX             14 + SIRFLSM_SEM_MAX 



#endif /* !LPL_CONFIG_H_INCLUDED */

/**
 * @}
 * End of file.
 */

