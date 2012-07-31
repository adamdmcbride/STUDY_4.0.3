/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2005-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_system.h
 *
 * This file is intended for simple inclusion of all the common defines
 * and types required for most code.
 */

#ifndef _MALI_SYSTEM_H_
#define _MALI_SYSTEM_H_

#ifdef __SYMBIAN32__
#ifndef _DEBUG
/* Mali sets these defines for release (rather than setting defines for debug)
 *		CPPFLAGS += -DNDEBUG
 *		CPPFLAGS += -DMALI_DEBUG_SKIP_ASSERT -DMALI_DEBUG_SKIP_CODE -DMALI_DEBUG_SKIP_TRACE -DMALI_DEBUG_SKIP_TPRINT -DMALI_DEBUG_SKIP_PRINT -DMALI_DEBUG_SKIP_ERROR
 */
#define NDEBUG
#define MALI_DEBUG_SKIP_ASSERT
#define MALI_DEBUG_SKIP_CODE
#define MALI_DEBUG_SKIP_TRACE
#define MALI_DEBUG_SKIP_TPRINT
#define MALI_DEBUG_SKIP_PRINT
#define MALI_DEBUG_SKIP_ERROR

#else /* _DEBUG */

/* Code relies on DEBUG (not _DEBUG) being set */
#define DEBUG
#endif /* !_DEBUG */
#endif /* __SYMBIAN32__ */

#include <mali_config.h>
#include <base/mali_types.h>
#include <base/mali_debug.h>
#include <base/mali_macros.h>
#include <base/mali_runtime.h>
#include <hwconfig.h>


#endif /*_MALI_SYSTEM_H_ */
