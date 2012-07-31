/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2007-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _EGL_INSTRUMENTED_H_
#define _EGL_INSTRUMENTED_H_

#include "mali_instrumented_context.h"
#include "mali_log.h"
#include "mali_counters.h"
#include "mali_egl_instrumented_types.h"
#include "sw_profiling.h"

MALI_IMPORT
mali_err_code _egl_instrumentation_init(mali_instrumented_context* ctx);

#endif /* _EGL_INSTRUMENTED_H_ */

