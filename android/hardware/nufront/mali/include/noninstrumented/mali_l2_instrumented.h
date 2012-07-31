/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef _MALI_L2_INSTRUMENTED_H_
#define _MALI_L2_INSTRUMENTED_H_

#include "mali_instrumented_context_types.h"

MALI_STATIC_INLINE mali_err_code MALI_CHECK_RESULT _mali_l2_instrumented_init(mali_instrumented_context *ctx)
{
	MALI_IGNORE(ctx);
	MALI_SUCCESS;
}

MALI_STATIC_INLINE void _instrumented_l2_setup(mali_instrumented_context *context, mali_gp_job_handle job_handle_gp, mali_pp_job_handle *job_handle_pp, int pp_job_count)
{
	MALI_IGNORE(context);
	MALI_IGNORE(job_handle_gp);
	MALI_IGNORE(job_handle_pp);
	MALI_IGNORE(pp_job_count);
}

MALI_STATIC_INLINE void _instrumented_l2_job_done_gp(mali_instrumented_frame *frame, mali_gp_job_handle job_handle)
{
	MALI_IGNORE(frame);
	MALI_IGNORE(job_handle);
}

MALI_STATIC_INLINE void _instrumented_l2_job_done_pp(mali_instrumented_frame *frame, mali_pp_job_handle job_handle)
{
	MALI_IGNORE(frame);
	MALI_IGNORE(job_handle);
}

MALI_STATIC_INLINE void _instrumented_l2_frame_done(mali_instrumented_frame *frame)
{
	MALI_IGNORE(frame);
}

#endif /* _MALI_L2_INSTRUMENTED_H_ */

