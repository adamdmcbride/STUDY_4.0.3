/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2009-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_dependency_system.h
 *
 * The Mali Dependency System is a independent dependency system that the Mali drivers use to
 * keep track on Read and Write dependencies between Surface Buffers (Resources), and the
 * entities (Framebuilders) that want to work on these surfaces (Consumers).
 * To rephrase it: Consumers (Frame builders) have read or write connections to several
 * Resources (surfaces). Other Consumers may be the Display driver that may held a
 * read dependency on a Resource (frame buffer) which is visible on screen.
 *
 * The reason why we introduce this somewhat complex Dependency System is that we want to
 * avoid to having the CPU to wait for a blocking dependency, when there are other things
 * that can be done. For example: we want to be able to start the GP job for a surface
 * even if the things we need for starting the PP job is not available. After that
 * we want to start the PP job when both the GP job is finished, and the surfaces this
 * PP job depends on. By letting the depedency system do these job starts from callbacks
 * when their list of dependencies are met, we do not need to have the functional but
 * slow locking system, that locks the CPU until the dependency is met.
 *
 * The basic entities are thus Resources which is allocated with: \a mali_ds_resource_allocate(),
 * and Consumers that are allocated through \a mali_ds_consumer_allocate().
 * You make a dependency between a Consumer and a Resource by: \a mali_ds_connect() where the
 * user also states wheter the Consumer will Read or Write to the resource.
 * The consumer has an activation callback function that will be called when the Consumer should
 * start its work. The activation callback will normal start all the PP jobs for this consumer.
 *
 * When all the dependencies are added to the Consumer, it can be flushed. Flushing is to call
 * the function \a mali_ds_consumer_flush() on the consumer. This tells the Dependency system
 * that the consumer should get the Activation callback when all its dependencies are met.
 *
 * This is what we do in the activation function to the Frame today:
 * First we increase the Consumer ref count to the number of PP jobs we are about to start:
 * Calling * \a mali_ds_consumer_release_ref_count_change(frame->ds_consumer_pp_render,2)
 * the last number 2 indicates that it increases the ref count by 2.
 * Then it starts the PP jobs (2 in this case).
 *
 * The callback from the PP jobs when they are finished does this:
 * If the job fails it calls: \a mali_ds_consumer_set_error(consumer) And it always release
 * the reference count this job held:
 * \a mali_ds_consumer_release_ref_count_change(consumer,-1).
 *
 * When all (2) PP jobs are finished, the consumer is actually released, and the Dependency
 * System does the Release callback on the Consumer. The release callback will do the
 * _egl_mali_frame_swap() which shows the rendered image on screen. In this callback function
 * we also use the funciton \a mali_ds_connect_give_direct_ownership_and_flush() to transfer the rights
 * of the Frame Surface Resource from the Frame builder Consumer to the EGL Display consumer,
 * it will also release the previous EGL Display consumer so this can be used in the next
 * rendering.
 *
 * To see more (simple) examples of how the Dependency system works, see the test of it, found in
 * file \a dependency01.c in the testbench.
 *
 */

#ifndef _MALI_DEPENDENCY_SYSTEM_H
#define _MALI_DEPENDENCY_SYSTEM_H

#include <mali_system.h>
#include <base/mali_types.h>
#include <base/mali_macros.h>
#include <base/mali_context.h>

#ifdef __cplusplus
extern "C" {
#endif

/* In case DS tracing is needed, enable this
#define DS_TRACE */

#ifdef DS_TRACE
#define DS_TRACE_LOG_PARAM ,const char * func, const char * file, u32 line
#if MALI_TIMELINE_PROFILING_ENABLED
#error You cannot use DS_TRACE in combination with TLP, as function declarations clash
#endif
#else
#define DS_TRACE_LOG_PARAM
#endif

#if MALI_TIMELINE_PROFILING_ENABLED
#define TIMELINE_PROFILING_REASON ,u32 tlp_reason
#else
#define TIMELINE_PROFILING_REASON
#endif

/* Can be used on the ref_count_decrease variable into function mali_ds_consumer_release_ref_count_change() */
#define MALI_DS_REF_COUNT_TRIGGER		(MALI_S32_MAX)
#define MALI_DS_REF_COUNT_GRAB			(1)
#define MALI_DS_REF_COUNT_RELEASE		(-1)

/** Tells if the specified object is keeped in a reseted state,
 * or released and thus not acccessible any more */
typedef enum mali_ds_release
{
	MALI_DS_RELEASE,
 	MALI_DS_KEEP
} mali_ds_release;

/**
 * Enum used as an input parameter to function \a mali_ds_resource_release_connections().
 * It tells which dependency connections it should remove.
 * \a MALI_DS_ABORT_NONE    Tells that it should not abort any connections.
 * \a MALI_DS_ABORT_WAITING Tells that it should only abort connections which has NOT been activated.
 * \a MALI_DS_ABORT_ALL     Tells that it should abort all connections.
 */
typedef enum mali_ds_abort
{
	MALI_DS_ABORT_NONE,
	MALI_DS_ABORT_WAITING,
	MALI_DS_ABORT_ALL
} mali_ds_abort;

/**
 * Used by functions creating a dependency connection between a Consumer and a Resource.
 * This tells if the action of the Consumer should read or write to the Resource.
 * Many Consumers can Read from a Resource at the same time, but only one can write.
 */
typedef enum mali_ds_connection_type
{
	MALI_DS_READ,
 	MALI_DS_WRITE
} mali_ds_connection_type;

/**
 * This is a status enum used as an input to \a mali_ds_consumer_release_ref_count_change()
 * This is used to indicate that something error has occured. The error is propageted
 * to the callbackfunctions to the consumer.
 * If the callback functions to the consumer gets a callback with MALI_DS_ERROR set
 * it could mean one of two things:
 * 1) The dependency on a resource is killed by \a mali_ds_resource_release_connections
 * 2) The consumer object has been told there occured an error through the \a mali_ds_consumer_set_error()
 * function.
 * The callback functions should then know that they can not trust the result of what they are waiting for.
 */
typedef enum mali_ds_error
{
	MALI_DS_OK,
	MALI_DS_ERROR
} mali_ds_error;


/**
 * Parameter for \a mali_ds_consumer_release_set_mode() function.
 * Decides what shall happen when the consumer is told to release all its connections.
 * If MALI_DS_RELEASE_WRITE_GOTO_UNFLUSHED is set the consumer will only release write connections,
 * leaving the read connections still attached. It is then in unflushed state. Normal procedure is
 * then later to add new write dependencies, and flush it which will start a new rendering.
 * This is used for what is called incremented rendering.
 * If this mode is set, it is important to set it back to MALI_DS_RELEASE_ALL before final releas of
 * the consumer, to not keep the Read dependencies forever, which would have led to memory leak.
 * */

typedef enum mali_ds_release_mode
{
	MALI_DS_RELEASE_ALL,   /**< Normal release: 1) Release callback 2) Release connections */
	MALI_DS_RELEASE_WRITE_GOTO_UNFLUSHED  /**< 1) Release callback 2) Release write deps, 3) To UNFLUSHED State */
} mali_ds_release_mode ;


/** A Resource - Normally in a 1-to-1 relation with a Surface */
typedef struct mali_ds_resource_type * mali_ds_resource_handle;

/** A Consumer - Something that will read or write to a number of Resources.
 * Gets an activation callback when the dependencies to all the Resources it depends on are triggered.*/
typedef struct mali_ds_consumer_type * mali_ds_consumer_handle;

/**
 * Callback function from a resource when it is deleted.
 * Makes it possible to synchronize the ref count of the surface with the Resource object,
 * to keep the surface alive until the resource is deleted.
*/
typedef void (*mali_ds_cb_func_resource)(mali_base_ctx_handle base_ctx, void * cb_param);

/**
 * A typedef of a callback function that is called when the consumer is activated, that means:
 * all the connections to the Resources it depends on are triggered.
 * Normally the callback will start the Rendering job, since this is the point when it knows that
 * all buffers it will read and write to are available for it.
 * After the Rendering is finished it MUST release its connections, so other can use these Resources.
 * The error_value variable is MALI_DS_ERROR if there were any errors, in that case it is not safe to
 * start any rendering, and all connections should be removed.
 */
typedef void (*mali_ds_cb_func_consumer_activate)(mali_base_ctx_handle base_ctx, void * owner, mali_ds_error error_value);


/**
 * A typedef of a callback function intended for the Copy On Write procedure.
 * When the user is calling \a mali_ds_connect() and tries to connect a WRITE dependency to a Resource (surface)
 * that already has a Read dependency that is not flushed, or has a mode that will not release its dependency
 * when the rendering is finished the current connection would block until this other connection is released, which
 * could be undeinitely. To avoid waiting undefenitely the consumer can set this callback function, and what it should
 * do is to perform is to allocate a new Surface and a corresponding Resource. The new Resource should be returned
 * from this callback function. Since the Resource is new, it is not blocked by any other unflushed framebuilders,
 * and the rendering could start as soon it gets all its Read connections triggered, and GP is finished.
 * */
typedef mali_ds_resource_handle (*mali_ds_cb_func_consumer_replace_resource)(void * resource_owner, void * consumer_owner);

/**
 * A typedef of a callback function that is called when the consumer is released, that means:
 * All the connections to the Resources is talled to be removed from it, and it goes to Unused state.
 * If the consumer mode was set to MALI_DS_RELEASE_WRITE_GOTO_UNFLUSHED initially this function
 * would not relase its Read dependencies, and instead go back to the unflushed state.
 * The status variable tells if the work was done with success.
 * If the release function finds out that it should not release its connections after all,
 * but set the consumer back to FLUSHED state, still holding all the connections, the function
 * should return MALI_DS_KEEP. This is used for CMU, which split each rendering into several
 * iterations of GP and PP jobs, where each continues where the last iteration stopped.
 * In all other cases it should return MALI_DS_RELEASE.
 * If status==MALI_DS_ERROR the return from this function must be MALI_DS_RELEASE.
 */
typedef mali_ds_release (*mali_ds_cb_func_consumer_release)(mali_base_ctx_handle base_ctx, void * owner, mali_ds_error status);


/**
 * Allocates a Resource - The dependency object for a Buffer that can be Read or Written to.
 * It is assumed that it has a 1-to-1 relationship with the descriptor object for the buffer,
 * and this is the \a cb_param input parameter. The optional callback function is automatically
 * called when the resource is actually released.
 * @param ctx The base context
 * @param cb_param The input to the callback function when it is released, and to the
 *        consumer_replace_resource callback function.
 * @param cb_on_release A callback function that will be called when the Resource is released.
*/
MALI_IMPORT mali_ds_resource_handle  mali_ds_resource_allocate(mali_base_ctx_handle ctx,
                                                               void * cb_param,
                                                               mali_ds_cb_func_resource cb_on_release DS_TRACE_LOG_PARAM ) MALI_CHECK_RESULT;

/**
 * Sets the pointer to the Surface Descriptor this Resource is in 1-to-1 relationship with.
 * This is the same as the \a cb_param in the \a mali_ds_resource_allocate() function.
 */
MALI_IMPORT void mali_ds_resource_set_owner(mali_ds_resource_handle resource, void * owner DS_TRACE_LOG_PARAM);


/**
 * Sets the owner pointer 
 * This is the same as the \a cb_param in the \a mali_ds_consumer_allocate() function.
 */
MALI_IMPORT void mali_ds_consumer_set_owner(mali_ds_consumer_handle resource, void * owner DS_TRACE_LOG_PARAM);

/**
 * Function that can do two different things:
 * 1) Force the release of dependency connections to this resource.
 * The connections that are "killed" in this way will be signalled that they are missing a resource.
 * 2) It may release the actual Resource object itself, with the parameter \a keep_resource
 * If this parameter is set to \A MALI_DS_RELEASE the Resource, and the \a do_abort parameter
 * is set to \a MALI_DS_ABORT_ALL, the resource and its connection are removed before the
 * function returns. If the \a do_abort argument is set to something else, the deletion
 * of this resource may happen later, when all the connections to consumers are released.
 * @param resource The resource we want to kill connections from or delete
 * @param keep_resource Telling if we want to keep or delete the Resource object.
 * If it is set to be deleted, it is not allowed to add more connections to it.
 * @param connections Telling if this function should delete NONE connections,
 * WAITING connections that have not been triggered yet, or ALL which means that it
 * will also kill connections that are Trigged, and therefore currently running.
 *
 * @NOTE At the moment the integration of the drivers do NOT kill any connections from the
 * Resource itself, and have always released all connections before it deletes the Resource.
 */

MALI_IMPORT void mali_ds_resource_release_connections(mali_ds_resource_handle resource,
                                                      mali_ds_release keep_resource,
                                                      mali_ds_abort do_abort DS_TRACE_LOG_PARAM );


/**
 * Allocates a Consumer in the Dependency system.
 * The consumer is an object with an associated action, a callbackfunction that will
 * be called when the consumer is activated.
 * The Consumer object can depend on a list of Resources that it can have Read or Write
 * connections to, and it will be activated when all its connections to Resources are triggered.
 * The consumer must release its connection after it has been activated.
 * When the consumer releases its connection, it will execute the \a cb_func_release before
 * it actually releases its connections. This makes it possible to use the function
 * \a mali_ds_connect_give_direct_ownership_and_flush to transfer the ownership of a Resource from
 * this consumer to another consumer when this consumer is releasing its connection to it.
 * @param ctx Mali base context
 * @param cb_param The callback parameter given to the callback functions.
 * @param cb_func_activate Callback function called when consumer is Activated.
 * @param cb_func_release  Callback function called when an Activated consumer is Releasing its connections.
 */
MALI_IMPORT mali_ds_consumer_handle mali_ds_consumer_allocate(mali_base_ctx_handle ctx,
                                                              void * cb_param,
                                                              mali_ds_cb_func_consumer_activate cb_func_activate,
                                                              mali_ds_cb_func_consumer_release cb_func_release DS_TRACE_LOG_PARAM ) MALI_CHECK_RESULT;


/** Sets the callback function the consumer calls when it is activated (rendering can start) */
MALI_IMPORT void mali_ds_consumer_set_callback_activate(mali_ds_consumer_handle consumer, mali_ds_cb_func_consumer_activate cb_func_activate);

/** Sets the callback function the consumer calls when it is released, typically a callback to swap the buffer */
MALI_IMPORT void mali_ds_consumer_set_callback_release(mali_ds_consumer_handle consumer, mali_ds_cb_func_consumer_release cb_func_release);

/** Sets the callback function not used */
MALI_IMPORT void mali_ds_consumer_set_callback_replace_resource(mali_ds_consumer_handle consumer, mali_ds_cb_func_consumer_replace_resource cb_func);


/**
 * After all connections are added between the consumer and all the Resources it depends on,
 * the consumer should be flushed. If all dependency connections are met before the flush
 * is called, the activation and the activation callback function callback function will
 * be called from inside the flush function call.
 * If the consumer is still waiting for some connections when flush is called, the
 * activation of the consumer will first happen when these dependencies are met.
 * It is not allowed to call this function twice on a consumer, before it has been
 * activated and released again.
 * @param consumer Handle to the Consumer to be flushed
 */
MALI_IMPORT void mali_ds_consumer_flush(mali_ds_consumer_handle consumer DS_TRACE_LOG_PARAM );


/** This function flush this consumer, and do then wait until it is actually
 * activated before returning.
 * @NOTE Can not be called from inside a DS callback function.
 */
MALI_IMPORT mali_err_code mali_ds_consumer_flush_and_wait(mali_ds_consumer_handle consumer TIMELINE_PROFILING_REASON DS_TRACE_LOG_PARAM ) MALI_CHECK_RESULT;

/** Changing the ref count for when the consumer should be activated.
 * This allows us to flush the consumer before the GP job starts.
 * The default value can be seen as zero. Before flushing the consumer use this function to
 * increase this value to 1. Then the consumer is not activated (and start PP job) immidiately
 * after flush. Then the GP job can be started. When the GP job returns as finished this
 * function should be called again, but now with \a ref_count_change set to -1. This will
 * make the ref_count go to zero, and if all dependencies are met, activate the consumer which
 * starts the PP job. */
MALI_IMPORT void mali_ds_consumer_activation_ref_count_change(mali_ds_consumer_handle consumer, signed int ref_count_change );

/** Release all connections a consumer has. Afterwards the consumer will be in Unused state.
 * This function can not be used if \a mali_ds_consumer_release_ref_count_set_initial()
 * has been called on the consumer. */
MALI_IMPORT void mali_ds_consumer_release_all_connections(mali_ds_consumer_handle consumer_h DS_TRACE_LOG_PARAM );

/** Returning if a consumer is active or not. Useful for assertions */
MALI_IMPORT int mali_ds_consumer_active(mali_ds_consumer_handle consumer_h DS_TRACE_LOG_PARAM );

/** Set a ref count on how many times the \a mali_ds_consumer_release_ref_count_dec() must be called
 * before the resources added to this consumer is released.*/
MALI_IMPORT void mali_ds_consumer_release_ref_count_set_initial(mali_ds_consumer_handle consumer_h, u32 ref_count_initial DS_TRACE_LOG_PARAM );

/** Decrement the release reference count. Its initial value is set by the function
 * \a mali_ds_consumer_release_ref_count_set_initial(). Current function must afterwards decrement the
 * consumers release reference count the same number of times before the connections will be released.*/

MALI_IMPORT void mali_ds_consumer_release_ref_count_dec(mali_ds_consumer_handle consumer_h DS_TRACE_LOG_PARAM );

/**
 * Set what happends when the consumer will release its connections after a call to
 * \a mali_ds_consumer_release_ref_count_change().
 * Normally all connections are released and the consumer is set back to the unused state,
 * but if the mode is set to: MALI_DS_RELEASE_WRITE_GOTO_UNFLUSHED, the read dependencies are not set
 * and the consumer goes back to the unflushed preparing state */
MALI_IMPORT void mali_ds_consumer_release_set_mode(mali_ds_consumer_handle consumer_h, mali_ds_release_mode mode DS_TRACE_LOG_PARAM );

/** Calling this function will free the object allocated to the consumer handle.
 * The consumer_handle can not be used after calls to this funciton.
 * This is not true if the consumer is currently activated. If so it is freed after the
 * consumer release callback is called.
 * @NOTE To force free, call this first: mali_ds_consumer_release_ref_count_change(consumer, MALI_DS_REF_COUNT_TRIGGER);
 *
*/
MALI_IMPORT void mali_ds_consumer_free(mali_ds_consumer_handle consumer_h DS_TRACE_LOG_PARAM );


/** Calling this function will set the consumer into error state.
 * The error state is forwarded as a parameter into to activation and release
 * callback functions for this consumer.
 * After all connections are released from the consumer, the error state will be
 * removed.*/
MALI_IMPORT void mali_ds_consumer_set_error(mali_ds_consumer_handle consumer_h DS_TRACE_LOG_PARAM);

/**
 * Make a dependency connection to a Consumer from a Resource.
 * You can add many connections to a Consumer. This can be visualized as there are
 * pointing arrows from a lot of Resources to this Consumer. When all the arrows
 * are connected, the user calls flush on the Consumer. If all the arrows are already
 * triggered, the Consumer will be activated and doing its callback during the flush.
 * If some of the Resources are busy, the Consumer will get into a wait state during
 * flush, and it will first be activated with the callback when all the Resources
 * it depends on are available, so that all the connections are triggered.
 * @param consumer_handle Connection to the specified Consumer
 * @param resource_handle Connection from the specified Resource
 * @param rights Sets whether the connection will Read or Write from the Resource
 */
MALI_IMPORT mali_err_code mali_ds_connect(mali_ds_consumer_handle consumer_handle,
                                          mali_ds_resource_handle resource_handle,
                                          mali_ds_connection_type rights DS_TRACE_LOG_PARAM ) MALI_CHECK_RESULT;

MALI_IMPORT mali_err_code mali_ds_connect_and_activate_without_callback(mali_ds_consumer_handle arrow_to,
                                                                mali_ds_resource_handle arrow_from,
                                                                mali_ds_connection_type rights DS_TRACE_LOG_PARAM )  MALI_CHECK_RESULT;


/* Wrap most important functions with logging capabilities using the preprocessor */
#ifndef MALI_DS_IMPLEMENTOR
#ifdef DS_TRACE
#define mali_ds_resource_allocate(x,y,z) mali_ds_resource_allocate(x,y,z, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_resource_set_owner(x,y) mali_ds_resource_set_owner(x,y, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_set_owner(x,y) mali_ds_consumer_set_owner(x,y, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_resource_release_connections(x,y,z) mali_ds_resource_release_connections(x,y,z, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_connect_and_activate_without_callback(x,y,z) mali_ds_connect_and_activate_without_callback(x,y,z, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_connect(x,y,z) mali_ds_connect(x,y,z, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_release_ref_count_set_initial(o,p) mali_ds_consumer_release_ref_count_set_initial(o, p , __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_release_ref_count_dec(x) mali_ds_consumer_release_ref_count_dec(x, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_release_set_mode(x,y) mali_ds_consumer_release_set_mode(x,y,  __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_free(x) mali_ds_consumer_free(x, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_set_error(x) mali_ds_consumer_set_error(x, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_release_all_connections(x) mali_ds_consumer_release_all_connections(x, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_flush_and_wait(x,r) mali_ds_consumer_flush_and_wait(x, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_allocate(x,y,z,w) mali_ds_consumer_allocate(x,y,z,w, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_flush(x) mali_ds_consumer_flush(x, __FUNCTION__, __FILE__, __LINE__)
#define mali_ds_consumer_active(x) mali_ds_consumer_active(x, __FUNCTION__, __FILE__, __LINE__)
#else
#if !MALI_TIMELINE_PROFILING_ENABLED
#define mali_ds_consumer_flush_and_wait(x,r) mali_ds_consumer_flush_and_wait(x)
#endif
#endif
#endif

#ifndef MALI_DEBUG_SKIP_CODE
/**
 * The function prints a list of all resources this consumer is connected to.
 * For each of these resources it prints all consumers they are connected to again.
 * Don't call this function directly from GDB since it takes the mutex, and if the
 * program is hanging or stopped with a breakpoint in a thread that holds that mutex
 * GDB will block on this mutex as well.
 * Function intended for Debug purposes only.
 * @note From GDB use the equivalent function \a debug_ds_system_print_consumer() instead
 * @param consumer Handle to the Consumer
 */
MALI_IMPORT void mali_dependency_system_debug_print_consumer(mali_ds_consumer_handle consumer);

/**
 * The function prints a list of all consumers that is connected to this resource.
 * For each of these consumers it prints all resources they are connected to again.
 * Don't call this function directly from GDB since it takes the mutex, and if the
 * program is hanging or stopped with a breakpoint in a thread that holds that mutex
 * GDB will block on this mutex as well.
 * Function intended for Debug purposes only.
 * @note From GDB use the equivalent function \a debug_ds_system_print_resource() instead
 * @param consumer Handle to the Consumer
 */
MALI_IMPORT void mali_dependency_system_debug_print_resource(mali_ds_resource_handle resource);

/** GDB Debugging
 * If the program goes into a deadlock state, it is possible to connect GDB into it and find
 * what it blocks on. To connect to a program with gdb: "attatch PID"
 * Then it is possible to call the internals of the print functions:
 * \a mali_dependency_system_debug_print_consumer() \a mali_dependency_system_debug_print_consumer()
 * which should not be called from GDB.
 * The internals that does not take the mutexes and Can be called from GDB are called:
 * \a debug_ds_system_print_consumer(consumer) and \a debug_ds_system_print_resource(resource) .
 * To run this function: "call debug_ds_system_print_consumer(0xADDR_CONSUMER)" */

#endif /* #ifndef MALI_DEBUG_SKIP_CODE */

#ifdef __cplusplus
}
#endif

#endif /*_MALI_DEPENDENCY_SYSTEM_H */
