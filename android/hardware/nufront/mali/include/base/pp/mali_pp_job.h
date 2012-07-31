/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2006-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_pp_job.h
 * PP job system.
 * All access to the mali hardware goes through a job system. Jobs are created by the user and queued for execution.
 *
 */

#ifndef _MALI_PP_JOB_H_
#define _MALI_PP_JOB_H_

#include <base/mali_types.h>
#include <base/mali_context.h>
#include <base/mali_memory.h>
#include <base/mali_sync_handle.h>
#include <mali_render_regs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Definition of the handle type used to represent a pp job
 */
typedef struct mali_pp_job_type * mali_pp_job_handle;

/**
 * Definition of the pp job template handle type
 */
typedef struct mali_pp_job_template_type * mali_pp_job_template_handle;

/**
 * Definition of the pp callback function
 * Called when a job has completed
 * @param ctx A mali base context handle
 * @param cb_param User defined callback parameter
 * @param completion_status Job completion status
 * @param job_handle Handle for job that is completed when this callback is called
 * @return void
 */
typedef void (*mali_cb_pp)(mali_base_ctx_handle ctx, void * cb_param, mali_job_completion_status completion_status, mali_pp_job_handle job_handle);

/**
 * Flags for pp jobs. See description of each flag for more information.
 */
typedef enum mali_pp_job_flags
{
	MALI_PP_JOB_FLAG_DEFAULT = 0,          /**< Default behaviour; Flush L2 caches before start, no following jobs */
	MALI_PP_JOB_FLAG_NO_FLUSH = 1,         /**< No need to flush L2 caches before start */
	MALI_PP_JOB_FLAG_MORE_JOBS_FOLLOW = 2, /**< More related jobs follows, try to schedule them as soon as possible after this job */
} mali_pp_job_flags;

/**
 * Initialize the PP job system. Returns a base context handle.
 * Each _mali_pp_open call must be matched with a call to _mali_pp_close.
 * It's safe to call this function multiple times.
 * @see _mali_pp_close()
 * @param ctx The base context to scope the PP usage to
 * @return A standard Mali error code
 */
MALI_IMPORT mali_err_code _mali_pp_open(mali_base_ctx_handle ctx) MALI_CHECK_RESULT;

/**
 * Close a reference to the PP job system. Match each call to _mali_pp_open with a call to this function.
 * @see _mali_pp_open()
 * @param ctx The mali base context handle used in the open call
 */
MALI_IMPORT void _mali_pp_close(mali_base_ctx_handle ctx);

/**
 * Create a pp job template
 * All render registers are by default initialized to zero.
 * @param ctx The mali base context handle obtained in a open call earlier
 * @return A mali_pp_job_template_handle.
 */
MALI_IMPORT mali_pp_job_template_handle _mali_pp_job_template_new(mali_base_ctx_handle ctx);

/**
 * Free resources used by a template
 * @param handle The pp template to free
 * @return void
 */
MALI_IMPORT void _mali_pp_job_template_free(mali_pp_job_template_handle handle);

/**
 * Set render register value in a pp job template
 * @param handle The pp template to set a register in
 * @param regid ID of register to set
 * @param value value to assign to register
 * @return void
 */
MALI_IMPORT void _mali_pp_job_template_set_render_reg(mali_pp_job_template_handle handle, mali_reg_id regid, mali_reg_value value);

/**
 * Get a new PP job struct.
 * Returns a pointer to a new mali_pp_job.
 * When you are finished with the job it needs to be released.
 * This can either be done with setting the auto_free member variable or
 * calling the _mali_pp_job_free function.
 * @param ctx The mali base context handle obtained in a open call earlier
 * @param ptemplate Handle to a template for this job. Use MALI_NO_HANDLE if no template is to be used
 * @param flags Special flags for this PP job, see mali_pp_job_flags for more information
 */
MALI_IMPORT mali_pp_job_handle _mali_pp_job_new(mali_base_ctx_handle ctx, mali_pp_job_template_handle template_handle_pointer, mali_pp_job_flags flags);

/**
 * Release a PP job struct.
 * Returns the PP job struct to the base system and frees up any resources used by it.
 * Any attached resources will also be freed like upon a normal job completion.
 * @param job The job to release
 */
MALI_IMPORT void _mali_pp_job_free(mali_pp_job_handle job);

/**
 * Reset a PP job
 * Resets the PP job to same the state as when newly allocated.
 * Any attached resources will be freed like upon a normal job completion.
 * @param job_handle Handle for a PP job
 */
MALI_IMPORT void _mali_pp_job_reset(mali_pp_job_handle job_handle);

/**
 * Add a mali_mem_handle to a job's free-on-termination list
 * @param job Handle to the job to update
 * @param mem Handle to add to list
 * @return void
 */
MALI_IMPORT void _mali_pp_job_add_mem_to_free_list(mali_pp_job_handle job, mali_mem_handle mem);

/**
 * Set callback for a pp job
 * @param job Handle to the job to update
 * @param func Function to set as callback func
 * @param cp_param Argument passed to callback
 * @return void
 */
MALI_IMPORT void _mali_pp_job_set_callback(mali_pp_job_handle job, mali_cb_pp func, void * cb_param);

/**
 * Get the address of a render attachment from the write back unit of a pp job
 * @param job Handle to the job
 * @param wb_unit the Write Back unit
 * @return the memory address of the render attachment
 */
MALI_IMPORT u32 _mali_pp_job_get_render_attachment_address(mali_pp_job_handle job, u32 wb_unit);

/**
 * Set a render register value in a job definition
 * @param job Handle to the job to update
 * @param regid ID of register to set
 * @param value value to assign to register
 */
MALI_IMPORT void _mali_pp_job_set_render_reg(mali_pp_job_handle job, mali_reg_id regid, mali_reg_value value);

/**
 * Return true if it is possible to update a register of the pp job
 * @param job Handle to the job to check
 * @return true if it is possible to update a job register, false otherwise
 */
MALI_IMPORT mali_bool _mali_pp_job_is_updateable(mali_pp_job_handle job);

/**
 * Get a wait handle which is trigged when the pp job has finished processing
 * Returns a handle to a wait object usable for waiting on this pp job to finish processing
 * @note This must be called before @see _mali_pp_job_start if you want to be able wait on this pp job
 * @param handle Handle to a pp job
 * @return Handle which can be used with @see _mali_wait_on_handle
 */
MALI_IMPORT mali_base_wait_handle _mali_pp_job_get_wait_handle(mali_pp_job_handle job);

/**
 * Queue a PP job for execution by the system.
 * Puts the job onto the queue of jobs to be run.
 * The job's priority will decide where in the queue it will be put.
 * @param job Pointer to the job to put on the execution queue.
 * @param priority Priority of the job
 */
MALI_IMPORT void _mali_pp_job_start(mali_pp_job_handle job, mali_job_priority priority);

/**
 * Attach a PP job to a sync object
 * Attaches the job to the list of jobs the sync object should wait for before firing the callback
 * @note The two objects must be from the same context
 * @param sync Handle to the sync object to attach this job to
 * @param job Pointer to the job to put on the sync list
 */
MALI_IMPORT void _mali_pp_job_add_to_sync_handle(mali_sync_handle sync, mali_pp_job_handle job);

/**
 * Retrieve interrupt status on job completion
 * @param job Pointer to the job to query interrupt status from
 * @return The interrupt status when the job ran.
 */
MALI_IMPORT u32 _mali_pp_job_intstat_get(mali_pp_job_handle job_handle);

/**
 * Function to return number of MaliPP cores available
 * @return The number of PP cores on the system
 */
MALI_IMPORT u32 _mali_pp_get_core_nr(void);

/**
 * Function to return version number of MaliPP. To be used if we have several different versions
 * of MaliPP, and some may have bugfixes.
 * @return The core version
 */
MALI_IMPORT u32 _mali_pp_get_core_product_id(void);

/**
 * Function to return the Major and Minor fields (version) reported for the cores on the system.
 * @return The version number read from the cores
 */
MALI_IMPORT u32 _mali_pp_get_core_version(void);

/**
 * Set a frame_nr on the job. It can be used for aborting jobs. The frame_nr
 * can be given to the job_abort function, and it will abort all jobs in the
 * context with this frame_id.
 * @param job_handle Set frame_id on this job
 * @param frame_id   The frame_id we want to set.
 * @see _mali_base_frame_id_get_new()
 */
MALI_IMPORT void _mali_pp_job_set_frame_id(mali_pp_job_handle job_handle, mali_base_frame_id frame_id);

/**
 *	Set job identity, which is defined by unique frame builder id and flush id. Called from Frame Builder's
 *  @c _pp_jobs_create.
 *
 *  @param job_handle	Job handle
 *  @param fb_id		Frame Builder ID
 *  @param flush_id		Flush ID
 */
void _mali_pp_job_set_identity(mali_pp_job_handle job_handle, mali_base_frame_id fb_id, mali_base_flush_id flush_id);

/**
 * Aborting all the jobs that has the frame_nr equal to the input parameter
 * in the given context.
 * @param ctx A mali base context handle
 * @param frame_id   The frame_id we want to set.
 */
MALI_IMPORT void _mali_pp_job_abort(mali_base_ctx_handle ctx, mali_base_frame_id frame_id);


/**
 * Get the duration it took for the mali core to execute this job.
 * This function can only be called successfully from within the callback function
 * of when the current job is finished.
 * The time is given in microseconds
 * @param job_handle
 * @return micro seconds (10^-6 s)
 */
u32 _mali_pp_job_get_render_time(mali_pp_job_handle job_handle);

#ifdef __cplusplus
}
#endif

#endif /* _MALI_PP_JOB_H_ */
