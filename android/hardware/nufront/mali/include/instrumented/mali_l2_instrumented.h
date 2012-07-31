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

/**
 * Register Mali L2 cache performance counters.
 *
 * This function registers Mali level 2 cache performance counters in the mali_counters system. It
 * is run by _mali_instrumented_create_context().
 *
 * \param ctx      the instrumented context.
 */
MALI_IMPORT
mali_err_code MALI_CHECK_RESULT _mali_l2_instrumented_init(mali_instrumented_context *ctx);

/**
 * Prepare the instrumented GP and PP job
 *
 * \param job_handle_gp      The GP job to prepare for instrumenation
 * \param job_handle_pp      The PP job(s) to prepare for instrumenation
 * \param pp_job_count       The number of PP jobs pointed to by job_handle_pp
 */
MALI_IMPORT
void _instrumented_l2_setup(mali_instrumented_context *context, mali_gp_job_handle job_handle_gp, mali_pp_job_handle *job_handle_pp, int pp_job_count);

/**
 * Collect performance values from the Mali L2 cache after a GP job is completed.
 *
 * \param frame              instrumented frame that we write values to
 * \param job_handle         handle to the GP job that we read values from
 */
MALI_IMPORT
void _instrumented_l2_job_done_gp(mali_instrumented_frame *frame, mali_gp_job_handle job_handle);

/**
 * Collect performance values from the Mali L2 cache after a PP job is completed.
 *
 * \param frame              instrumented frame that we write values to
 * \param job_handle         handle to the PP job that we read values from
 */
MALI_IMPORT
void _instrumented_l2_job_done_pp(mali_instrumented_frame *frame, mali_pp_job_handle job_handle);

/**
 * Collect performance values from the Mali L2 cache after a frame has completed.
 *
 * \param frame              instrumented frame that we write values to
 */
MALI_IMPORT
void _instrumented_l2_frame_done(mali_instrumented_frame *frame);

#endif /* _MALI_L2_INSTRUMENTED_H_ */

