/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Locking semaphore for hardware sharing
--
------------------------------------------------------------------------------*/

#ifndef __EWL_LINUX_LOCK_H__
#define __EWL_LINUX_LOCK_H__

/*m@nufront*/
#ifdef ANDROID_POSIX

#include <semaphore.h>
typedef struct on2sem_
{
    sem_t sem_enc;
    sem_t sem_pp;
} on2sem;

int on2_sem_init(on2sem *on2_sem);
int on2_sem_wait(sem_t *sem);
int on2_sem_post(sem_t *sem);
int on2_sem_destroy(on2sem *on2_sem);

#else /*SYSTEM V*/

#include <sys/types.h>
#include <sys/ipc.h>

int binary_semaphore_allocation(key_t key, int nsem, int sem_flags);
int binary_semaphore_deallocate(int semid);
int binary_semaphore_wait(int semid, int sem_num);
int binary_semaphore_post(int semid, int sem_num);
int binary_semaphore_initialize(int semid, int sem_num);
#endif

#endif /* __EWL_LINUX_LOCK_H__ */
