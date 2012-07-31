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

#include "mali_system.h"
#include "base/pp/mali_pp_job.h"
#include "base/gp/mali_gp_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Setting the array that tells which counters to read for a GP job.
 *
 * The perf_counters array tells which counters on mali we will readout after
 * the job has run, and other things that can be profiled on the job.
 *
 * @param job_handle GP job handle
 * @param perf_counters an array with the counter indices to read
 * @param perf_counters_count the number of counters in the perf_counters array
 */
MALI_IMPORT
void _mali_instrumented_l2_job_set_counters_gp(mali_gp_job_handle job_handle_gp,
                                               u32 *perf_counters,
                                               u32 perf_counters_count);

/**
 * Setting the array that tells which counters to read for a PP job.
 *
 * The perf_counters array tells which counters on mali we will readout after
 * the job has run, and other things that can be profiled on the job.
 *
 * @param job_handle PP job handle
 * @param perf_counters an array with the counter indices to read
 * @param perf_counters_count the number of counters in the perf_counters array
 */
MALI_IMPORT
void _mali_instrumented_l2_job_set_counters_pp(mali_pp_job_handle job_handle_pp,
                                               u32 *perf_counters,
                                               u32 perf_counters_count);


/**
 * Get the array that tells which counters to read / has been read for a GP job.
 *
 * The perf_counters array tells which counters on mali we will readout after
 * the job has run.
 *
 * \param job_handle              GP job handle
 * \param perf_counters returns   the array with the counter indices to read
 * \param perf_counters_count     returns the number of counters in the perf_counters array
 * \param res                     the array with the results
 */
MALI_IMPORT
void _mali_instrumented_l2_job_get_data_gp(mali_gp_job_handle job_handle,
                                           u32 **perf_counters,
                                           u32 *perf_counters_count,
                                           u32 **res);

/**
 * Get the array that tells which counters to read / has been read for a PP job.
 *
 * The perf_counters array tells which counters on mali we will readout after
 * the job has run.
 *
 * \param job_handle              PP job handle
 * \param perf_counters returns   the array with the counter indices to read
 * \param perf_counters_count     returns the number of counters in the perf_counters array
 * \param res                     the array with the results
 * \param raw_res                 the array with the raw results
 * \param frame_split_nr          which part of the frame the specified PP job rendered
 */
MALI_IMPORT
void _mali_instrumented_l2_job_get_data_pp(mali_pp_job_handle job_handle,
                                           u32 **perf_counters,
                                           u32 *perf_counters_count,
                                           u32 **res,
                                           u32 **raw_res,
                                           u32 *frame_split_nr);

#ifdef __cplusplus
}
#endif

#endif /* _MALI_L2_INSTRUMENTED_H_  */
