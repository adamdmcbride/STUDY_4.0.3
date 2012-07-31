/**
 * @addtogroup platform_src_sirf_sirfclm
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
 * @file   sirfclm_config.h
 *
 * @brief  SiRFCLM Configuration.
 */

#ifndef CLM_CONFIG_H_INCLUDED
#define CLM_CONFIG_H_INCLUDED



/* ----------------------------------------------------------------------------
 *    Preprocessor Definitions
 * ------------------------------------------------------------------------- */

/* Thread IDs */

/* CLM CGEE Process THREAD */
#define SIRFINSTANTFIX_THREAD_CGEE            0x0050
/* SGEE THREAD */
#define SIRFINSTANTFIX_THREAD_SGEE            0x0051
/* External SGEE Downloader THREAD */
#define SIRFINSTANTFIX_THREAD_SGEE_DNLDR      0x0052

/* Test App: */
/* Thread IDs */
#define SIRFINSTANTFIX_THREAD_TRK_COM        0x0053
#define SIRFINSTANTFIX_DEMO_THREAD_TRANSMIT  0x0054
#define SIRFINSTANTFIX_DEMO_THREAD_MAIN      0x0055
#define SIRFINSTANTFIX_REQMSG_THREAD_MAIN    0x0056

/* Thread stack sizes */
#define SIRFINSTANTFIX_THREAD_CGEE_STACK_SIZE          (24*1024)
#define SIRFINSTANTFIX_THREAD_SGEE_STACK_SIZE          (8*1024)
#define SIRFINSTANTFIX_THREAD_SGEE_DNLDR_STACK_SIZE    (8*1024)

/* Aggregate stack size for CLM */
#define SIRFINSTANTFIX_STACK_MAX  ( SIRFINSTANTFIX_THREAD_CGEE_STACK_SIZE + SIRFINSTANTFIX_THREAD_SGEE_STACK_SIZE )

#ifdef SIRFINSTANTFIX_MUTEX_MAX
#undef SIRFINSTANTFIX_MUTEX_MAX
#endif
/* Number of mutexes */
#define SIRFINSTANTFIX_MUTEX_MAX    4

#ifdef SIRFINSTANTFIX_SEM_MAX
#undef SIRFINSTANTFIX_SEM_MAX
#endif
/* Number of semaphores */
#define SIRFINSTANTFIX_SEM_MAX      6

#endif /* !CLM_CONFIG_H_INCLUDED */

/**
 * @}
 */


