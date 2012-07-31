/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <pthread.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>
#include <ump/ump_ref_drv.h>

#include "gralloc_priv.h"
#include "alloc_device.h"
#include "framebuffer_device.h"

#ifdef USE_GC_CONVERTER
#include "gc_converter.h"
#endif

static pthread_mutex_t s_map_lock = PTHREAD_MUTEX_INITIALIZER;
static int s_ump_is_open = 0;

#ifdef GRALLOC_VIDEO_MAP_UMP
extern uint32_t    g_vphysaddr;
extern ump_handle  g_vhandle;
extern int         g_vdev;
extern void       *g_vbase;
extern uint32_t    g_vump_id;
#else
extern int g_fbdev;
extern uint32_t g_physaddr;
extern void *g_fbbase;
#endif

static int gralloc_device_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
	int status = -EINVAL;

	if (!strcmp(name, GRALLOC_HARDWARE_GPU0))
	{
		status = alloc_device_open(module, name, device);
	}
	else if (!strcmp(name, GRALLOC_HARDWARE_FB0))
	{
		status = framebuffer_device_open(module, name, device);
	}

	return status;
}

static int gralloc_register_buffer(gralloc_module_t const* module, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		LOGE("Registering invalid buffer, returning error");
		return -EINVAL;
	}

	// if this handle was created in this process, then we keep it as is.
	private_handle_t* hnd = (private_handle_t*)handle;
	if (hnd->pid == getpid())
	{
		return 0;
	}

	int retval = -EINVAL;

	pthread_mutex_lock(&s_map_lock);

	if (!s_ump_is_open)
	{
		ump_result res = ump_open(); // TODO: Fix a ump_close() somewhere???
		if (res != UMP_OK)
		{
			pthread_mutex_unlock(&s_map_lock);
			LOGE("Failed to open UMP library");
			return retval;
		}
		s_ump_is_open = 1;
	}

	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
	{
		hnd->ump_mem_handle = (int)ump_handle_create_from_secure_id(hnd->ump_id);
		if (UMP_INVALID_MEMORY_HANDLE != (ump_handle)hnd->ump_mem_handle)
		{
			hnd->base = (int)ump_mapped_pointer_get((ump_handle)hnd->ump_mem_handle);
			if (0 != hnd->base)
			{
				hnd->lockState = private_handle_t::LOCK_STATE_MAPPED;
				hnd->writeOwner = 0;
				hnd->lockState = 0;

				pthread_mutex_unlock(&s_map_lock);
				return 0;
			}
			else
			{
				LOGE("Failed to map UMP handle");
			}

			ump_reference_release((ump_handle)hnd->ump_mem_handle);
		}
		else
		{
			LOGE("Failed to create UMP handle");
		}
	}
	else
	{
		LOGE("registering non-UMP buffer not supported");
	}

	pthread_mutex_unlock(&s_map_lock);
	return retval;
}

static int gralloc_unregister_buffer(gralloc_module_t const* module, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		LOGE("unregistering invalid buffer, returning error");
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)handle;
    
	LOGE_IF(hnd->lockState & private_handle_t::LOCK_STATE_READ_MASK, "[unregister] handle %p still locked (state=%08x)", hnd, hnd->lockState);

	// never unmap buffers that were created in this process
	if (hnd->pid != getpid())
	{
		pthread_mutex_lock(&s_map_lock);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
		{
			ump_mapped_pointer_release((ump_handle)hnd->ump_mem_handle);
			hnd->base = 0;
			ump_reference_release((ump_handle)hnd->ump_mem_handle);
			hnd->ump_mem_handle = (int)UMP_INVALID_MEMORY_HANDLE;
		}
		else
		{
			LOGE("unregistering non-UMP buffer not supported");
		}

		hnd->base = 0;
		hnd->lockState  = 0;
		hnd->writeOwner = 0;

		pthread_mutex_unlock(&s_map_lock);
	}

	return 0;
}

static int gralloc_lock(gralloc_module_t const* module, buffer_handle_t handle, int usage, int l, int t, int w, int h, void** vaddr)
{
	if (private_handle_t::validate(handle) < 0)
	{
		LOGE("Locking invalid buffer, returning error");
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)handle;

	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP)
	{
		hnd->writeOwner = usage & GRALLOC_USAGE_SW_WRITE_MASK;
	}

	if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
	{
		*vaddr = (void*)hnd->base;
	}
	return 0;
}

static int gralloc_unlock(gralloc_module_t const* module, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		LOGE("Unlocking invalid buffer, returning error");
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)handle;
	int32_t current_value;
	int32_t new_value;
	int retry;

	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_UMP && hnd->writeOwner)
	{
		ump_cpu_msync_now((ump_handle)hnd->ump_mem_handle, UMP_MSYNC_CLEAN_AND_INVALIDATE, (void*)hnd->base, hnd->size);
	}
	return 0;
}

static int gralloc_perform(gralloc_module_t const* module, int operation, ...)
{
	int res = -EINVAL;
	va_list args;
	va_start(args, operation);

	switch (operation) {
		case GRALLOC_MODULE_PERFORM_CREATE_HANDLE_FROM_BUFFER:
			{
				uint32_t physAddr = va_arg(args, uint32_t);
				size_t size = va_arg(args, size_t);
				int flags = va_arg(args, int);
				native_handle_t** handle = va_arg(args, native_handle_t**);
				if (flags) {
					private_handle_t* hnd = (private_handle_t*)native_handle_create(private_handle_t::sNumFds, private_handle_t::sNumInts);
					hnd->magic = private_handle_t::sMagic;
#ifdef GRALLOC_VIDEO_MAP_UMP
					hnd->flags = private_handle_t::PRIV_FLAGS_USES_UMP;
					hnd->size = size;
					hnd->offset = physAddr - g_vphysaddr;
					hnd->base = intptr_t(g_vbase) + hnd->offset;
					hnd->fd = 0;
					hnd->writeOwner = 0;
					hnd->pid = getpid();
					hnd->ump_id = (int)g_vump_id;
					hnd->ump_mem_handle = (int)g_vhandle;
/*
                                        LOGD("%s fd=0x%x, size=0x%x, offset=0x%x - 0x%x=0x%x, base=0x%x+offset=0x%x, handle=0x%x", __FUNCTION__, \
                                              hnd->fd, hnd->size, physAddr, g_vphysaddr, hnd->offset, g_vbase, hnd->base, hnd);
*/
#else
					hnd->fd = g_fbdev;
					hnd->flags = private_handle_t::PRIV_FLAGS_FRAMEBUFFER;
					hnd->size = size;
					hnd->offset = physAddr - g_physaddr;
					hnd->base = intptr_t(g_fbbase) + hnd->offset;
#endif
					hnd->lockState = private_handle_t::LOCK_STATE_MAPPED;
					*handle = (native_handle_t *)hnd;
					res = 0;
				}
			}
			break;
#ifdef USE_GC_CONVERTER
		case GRALLOC_MODULE_PERFORM_CONVERT_BUFFER:
			{
				uint32_t srcPhysAddr = va_arg(args, uint32_t);
				uint32_t srcWidth    = va_arg(args, uint32_t);
				uint32_t srcHeight   = va_arg(args, uint32_t);
				uint32_t srcFormat   = va_arg(args, uint32_t);
				int srcCropLeft   = va_arg(args, int);
				int srcCropTop    = va_arg(args, int);
				int srcCropRight  = va_arg(args, int);
				int srcCropBottom = va_arg(args, int);

				int flags = va_arg(args, int);
				uint32_t *pdstPA  = va_arg(args, uint32_t*);
				uint32_t *pdstW   = va_arg(args, uint32_t*);
				uint32_t *pdstH   = va_arg(args, uint32_t*);
				uint32_t *pdstFmt = va_arg(args, uint32_t*);
				native_handle_t** handle = va_arg(args, native_handle_t**);

				uint32_t dstPA;
				uint32_t dstW;
				uint32_t dstH;
				uint32_t dstFmt;
				if (flags) {
					if (0 == gcconverter_toggle_video_mode(flags)) {
						if (0 == gcconverter_start_blit_video(
									srcPhysAddr, srcWidth, srcHeight, srcFormat, \
									srcCropLeft, srcCropTop, srcCropRight, srcCropBottom,  \
									&dstPA, &dstW, &dstH, &dstFmt)) {
							if (dstPA > 0 && dstW > 0 && dstH > 0) {
								*pdstPA = dstPA;
								*pdstW  = dstW;
								*pdstH  = dstH;
								*pdstFmt = dstFmt;
								private_handle_t* hnd = (private_handle_t*)native_handle_create(
										private_handle_t::sNumFds, private_handle_t::sNumInts);
								hnd->magic = private_handle_t::sMagic;
								hnd->fd = g_fbdev;
								hnd->flags = private_handle_t::PRIV_FLAGS_FRAMEBUFFER;
								hnd->size = dstW * dstH * ((dstFmt == HAL_PIXEL_FORMAT_RGB_565) ? 2 : 4);
								hnd->offset = dstPA - g_physaddr;
								hnd->base = intptr_t(g_fbbase) + hnd->offset;
								hnd->lockState = private_handle_t::LOCK_STATE_MAPPED;
								*handle = (native_handle_t *)hnd;
								LOGV("%s src: PhysAddr=0x%x, W=0x%x, H=0x%x, fmt=0x%x, l=0x%x, t=0x%x, r=0x%x, b=0x%x", \
										__FUNCTION__, srcPhysAddr, srcWidth, srcHeight, srcFormat, \
										srcCropLeft, srcCropTop, srcCropRight, srcCropBottom);
								LOGV("%s dst: PhysAddr=0x%x, W=0x%x, H=0x%x, fmt=0x%x, handle=0x%x", \
										__FUNCTION__, dstPA, dstW, dstH, dstFmt, hnd);
							} else {
								*pdstPA = 0;
								*pdstW  = 0;
								*pdstH  = 0;
								*pdstFmt = 0;
								*handle = 0;
							}
							res = 0;
						}
					}
				}
			}
			break;

		case GRALLOC_MODULE_PERFORM_EXIT_CONVERTION_MODE:
			{
				if (0 == gcconverter_toggle_video_mode(false))
					res = 0;
			}
			break;
#endif
		default:
			break;
	}

	va_end(args);
	return res;
}

// There is one global instance of the module

static struct hw_module_methods_t gralloc_module_methods =
{
	open: gralloc_device_open
};

private_module_t::private_module_t()
{
#define INIT_ZERO(obj) (memset(&(obj),0,sizeof((obj))))

	base.common.tag = HARDWARE_MODULE_TAG;
	base.common.version_major = 1;
	base.common.version_minor = 0;
	base.common.id = GRALLOC_HARDWARE_MODULE_ID;
	base.common.name = "Graphics Memory Allocator Module";
	base.common.author = "ARM Ltd.";
	base.common.methods = &gralloc_module_methods;
	base.common.dso = NULL;
	INIT_ZERO(base.common.reserved);

	base.registerBuffer = gralloc_register_buffer;
	base.unregisterBuffer = gralloc_unregister_buffer;
	base.lock = gralloc_lock;
	base.unlock = gralloc_unlock;
	base.perform = gralloc_perform;
	INIT_ZERO(base.reserved_proc);

	framebuffer = NULL;
	flags = 0;
	numBuffers = 0;
	bufferMask = 0;
	pthread_mutex_init(&(lock), NULL);
	currentBuffer = NULL;
	INIT_ZERO(info);
	INIT_ZERO(finfo);
	xdpi = 0.0f; 
	ydpi = 0.0f; 
	fps = 0.0f; 

#undef INIT_ZERO
};

/*
 * HAL_MODULE_INFO_SYM will be initialized using the default constructor
 * implemented above
 */ 
struct private_module_t HAL_MODULE_INFO_SYM;

