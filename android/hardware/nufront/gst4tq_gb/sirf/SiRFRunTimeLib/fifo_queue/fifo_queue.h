/**
 * @addtogroup queue
 * @{
 */

/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2007-2009 by SiRF Technology, Inc.                          *   
 *    All rights reserved.                                                 *
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.’s confidential and      *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.’s  express written           *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 ***************************************************************************
 *
 * MODULE:   Linked Queue
 *
 * FILENAME:   fifo_queue.h
 *
 * DESCRIPTION:   Header file for the linked queue module
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/SiRFRunTimeLib/fifo_queue/fifo_queue.h $
 *
 *  $DateTime: 2011/10/17 14:33:40 $
 *
 *  $Revision: #2 $
 *
 ***************************************************************************/

#ifndef __FIFO_QUEUE_H__
#define __FIFO_QUEUE_H__

#include "sirf_types.h"

/***************************************************************************
   Types, Enums, and Structs
***************************************************************************/

typedef struct _fifo_link_t
{
   struct _fifo_link_t *prev;
   struct _fifo_link_t *next;
   void*               *data; /* Variable length data of any type */
} fifo_link_t;

typedef struct _fifo_queue_t
{
   fifo_link_t        *head;
   fifo_link_t        *tail;
   tSIRF_UINT32        count;
} fifo_queue_t;

/***************************************************************************
   Function Prototypes
***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void  fifo_queue_init(      
   fifo_queue_t       * const list );

void  fifo_queue_add_tail(  
   fifo_queue_t       * const list, 
   fifo_link_t        * const link) ;

fifo_link_t *fifo_queue_remove_head( 
   fifo_queue_t       * const list );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIFO_QUEUE_H__ */

/**
* @}
* End of file.
*/
