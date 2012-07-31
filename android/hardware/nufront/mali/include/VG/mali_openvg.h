/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2007-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _MALI_OPENVG_H_
#define _MALI_OPENVG_H_


/*
 * This is the Mali OpenVG wrapper, for use in driver development only
 *  all applications should be built with the stock openvg.h and egl.h
 */

/* current khronos distributed openvg.h, must be on include path */
#include <VG/openvg.h>

/* Defines some things required for Khronos vgext.h, not included in Symbian's openvg.h */


#ifdef __SYMBIAN32__
	#define MALI_VG_API_CALL VG_API_CALL __SOFTFP
#else
	#ifdef MALI_MONOLITHIC
		#define MALI_VG_API_CALL MALI_VISIBLE VG_API_CALL
	#else
		#define MALI_VG_API_CALL VG_API_CALL
	#endif
#endif

#endif /* _MALI_OPENVG_H_ */

