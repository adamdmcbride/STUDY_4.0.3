/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2007, 2009-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_context.h
 * All module usage in the base driver is scoped using a context object.
 * All objects created in base are local to the context they're created in.
 */

#ifndef _MALI_CONTEXT_H_
#define _MALI_CONTEXT_H_

#include <base/mali_types.h>
#include <base/mali_macros.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defintion of the base context type
 */
typedef struct mali_base_ctx_type * mali_base_ctx_handle;

/**
 * Create a base driver context
 * Creates a new context in the base driver and returns a handle to it
 * @return A base context handle or MALI_NO_HANDLE on error
 */
MALI_IMPORT mali_base_ctx_handle _mali_base_context_create(void);

/**
 * Destroy a base driver context
 * Destroys the given context. If any modules are still open they will be closed.
 * Any open modules when a context is destroyed will be logged in debug mode
 * @param ctx Handle to the base context to destroy
 */
MALI_IMPORT void _mali_base_context_destroy(mali_base_ctx_handle ctx);


/**
 * Get a frame_id from context.
 * The frame id is a counter that is incremented for each call.
 * It does not need to be released.
 * @return A new frame_id, a new unique number.
 */
MALI_IMPORT mali_base_frame_id _mali_base_frame_id_get_new(mali_base_ctx_handle ctx);

/*
 * Get next unique frame builder id
 * The id counter is incremented (atomically) for each call ensuring the uniqueness of the id. 
 *
 * @return Unique frame_builder_id
 */
mali_base_frame_builder_id _mali_base_frame_builder_id_get_new(mali_base_ctx_handle ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MALI_RUNTIME_H_ */
