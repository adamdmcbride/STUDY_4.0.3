/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2007, 2009-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _MALI_VGU_H_
#define _MALI_VGU_H_


/*
 * This is the Mali VGU wrapper, for use in driver development only
 *  all applications should be built with the stock vgu.h.
 */

/* current khronos distributed openvg.h, must be on include path */
#include <VG/vgu.h>

#ifdef __SYMBIAN32__
#define MALI_VGU_API_CALL VGU_API_CALL __SOFTFP
#else
#define MALI_VGU_API_CALL VGU_API_CALL
#endif

#endif /* _MALI_VGU_H_ */
