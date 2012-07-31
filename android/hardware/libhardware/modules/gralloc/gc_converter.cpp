#ifdef USE_GC_CONVERTER

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "gc_converter.h"

#include <gc_hal.h>
#include <gc_hal_driver.h>
#include <gc_hal_raster.h>

#ifdef BOARD_USES_HDMI
#include "hdmi_device.h"
#endif

#define BUFFER_BASEADDR_FILTER 0x80000000

enum {
	GC_MODE_NONE,
	GC_MODE_UI_VIDEO,
	GC_MODE_HDMI_VIDEO,
	GC_MODE_HDMI_UI,
};

struct gcconv_rect {
	int left;
	int top;
	int right;
	int bottom;
};

struct gcconv_pane {
	uint32_t physAddr;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t stride;
	struct gcconv_rect rect;
};

struct gcconv_param{
	struct gcconv_pane src;
	struct gcconv_pane dst;
	int    gc_mode;
	int    vindex;
	int    video_state;

	gcoOS   gco_os;
	gcoHAL  gco_hal;
	gco2D   gco_2d;

	pthread_mutex_t lock;
	struct private_module_t *gc_priv_mod;
};

struct gcconv_param gc_param;

gceSURF_FORMAT gcconverter_get_gc_format(uint32_t format, int *strideBpp)
{
	gceSURF_FORMAT gcfmt = gcvSURF_UNKNOWN;
	int bpp = 0;
	switch(format)
	{
		case HAL_PIXEL_FORMAT_RGB_565:
			gcfmt = gcvSURF_R5G6B5;
			bpp = 2;
			break;
		case HAL_PIXEL_FORMAT_BGRA_8888:
			gcfmt = gcvSURF_A8R8G8B8;
			bpp = 4;
			break;
		case HAL_PIXEL_FORMAT_YCbCr_420_SP:
			gcfmt = gcvSURF_NV12;
			bpp = 1;
			break;
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
			gcfmt = gcvSURF_NV21;
			bpp = 1;
			break;
		case HAL_PIXEL_FORMAT_YCbCr_422_SP:
			gcfmt = gcvSURF_NV16;
			bpp = 1;
			break;
		case HAL_PIXEL_FORMAT_YV12:
		default:
			gcfmt = gcvSURF_YV12;
			bpp = 1;
			break;
	}
	*strideBpp = bpp;
	return gcfmt;
}

int gcconverter_blit(gcconv_param *param)
{
	if (param == NULL)
		return -1;

	gceSTATUS gcostat;
	gcsRECT srcRect;
	gcsRECT dstRect;
	gcsRECT dstSubRect;
	gceSURF_FORMAT srcFormat = (gceSURF_FORMAT)param->src.format;
	gceSURF_FORMAT dstFormat = (gceSURF_FORMAT)param->dst.format;

	gctUINT uPhysical;
	gctUINT vPhysical;
	gctUINT uStride;
	gctUINT vStride;

	gco2D  gco_2d = param->gco_2d;

	srcRect.left   = param->src.rect.left;
	srcRect.top    = param->src.rect.top;
	srcRect.right  = param->src.rect.right;
	srcRect.bottom = param->src.rect.bottom;

	dstRect.left   = param->dst.rect.left;
	dstRect.top    = param->dst.rect.top;
	dstRect.right  = param->dst.rect.right;
	dstRect.bottom = param->dst.rect.bottom;

	dstSubRect.left   = 0;
	dstSubRect.top    = 0;
	dstSubRect.right  = param->dst.rect.right - param->dst.rect.left;
	dstSubRect.bottom = param->dst.rect.bottom - param->dst.rect.top;

	uStride = vStride = param->src.stride;
	uPhysical = vPhysical = param->src.physAddr + param->src.stride * param->src.height;

	LOGV("%s src: PhysAddr=0x%x, Width=0x%x, Height=0x%x, Format=0x%x, crop: l=0x%x, t=0x%x, r=0x%x, b=0x%x\n", \
			__FUNCTION__, param->src.physAddr, param->src.width, param->src.height, param->src.format, 
			param->src.rect.left, param->src.rect.top, param->src.rect.right, param->src.rect.bottom);
	LOGV("%s dst: PhysAddr=0x%x, Width=0x%x, Height=0x%x, Format=0x%x, region: l=0x%x, t=0x%x, r=0x%x, b=0x%x\n", \
			__FUNCTION__, param->dst.physAddr, param->dst.width, param->dst.height, param->dst.format,
			param->dst.rect.left, param->dst.rect.top, param->dst.rect.right, param->dst.rect.bottom);

	gcmVERIFY_OK(gco2D_SetGenericSource(gco_2d,
				&param->src.physAddr, 1, &param->src.stride, 1,  
				gcvLINEAR/*srcTiling*/,
				srcFormat, 
				gcvSURF_0_DEGREE/*srcRotation*/, 
				param->src.width, param->src.height));

	gcmVERIFY_OK(gco2D_SetBitBlitMirror(gco_2d,
				gcvFALSE/*horzMirror*/,
				gcvFALSE/*vertMirror*/));

	gcmVERIFY_OK(gco2D_SetGenericTarget(gco_2d,
				&param->dst.physAddr, 1, &param->dst.stride, 1,
				gcvLINEAR/*dstTiling*/,
				dstFormat,
				gcvSURF_0_DEGREE/*dstRotation*/,
				param->dst.width, param->dst.height));

	gcmVERIFY_OK(gco2D_DisableAlphaBlend(gco_2d));

	gco2D_SetPixelMultiplyModeAdvanced(gco_2d,
			gcv2D_COLOR_MULTIPLY_DISABLE, gcv2D_COLOR_MULTIPLY_DISABLE,
			gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE, gcv2D_COLOR_MULTIPLY_DISABLE);

	gcmVERIFY_OK(gco2D_SetClipping(gco_2d, gcvNULL));

	// set kernel size
	gcostat = gco2D_SetKernelSize(gco_2d, 9, 9);
	if (gcostat != gcvSTATUS_OK) {
		LOGE("2D set kernel size failed (status = %d)", gcostat);
	}

	gcmVERIFY_OK(gco2D_SetFilterType(gco_2d, gcvFILTER_SYNC));
	gcmVERIFY_OK(gco2D_EnableDither(gco_2d, gcvTRUE));

	gcostat = gco2D_FilterBlit(gco_2d,
			param->src.physAddr, param->src.stride,
			uPhysical, uStride, vPhysical, vStride,
			srcFormat, gcvSURF_0_DEGREE/*srcRotation*/,
			param->src.width, &srcRect,
			param->dst.physAddr, param->dst.stride,
			dstFormat, gcvSURF_0_DEGREE/*dstRotation*/,
			param->dst.width, &dstRect, &dstSubRect);
	if (gcostat != gcvSTATUS_OK) {
		LOGE("2D FilterBlit failed (status = %d)", gcostat);
	}
	gcmVERIFY_OK(gco2D_EnableDither(gco_2d, gcvFALSE));

	gcmVERIFY_OK(gco2D_Flush(gco_2d));

	/* do synchronized commit to wait till finished */
	gcmVERIFY_OK(gcoHAL_Commit(param->gco_hal, gcvTRUE));

	return 0;
}

int gcconverter_set_param(uint32_t srcPhysAddr, uint32_t srcWidth, uint32_t srcHeight, uint32_t srcFormat,
							int srcCropLeft, int srcCropTop, int srcCropRight, int srcCropBottom,
							uint32_t dstPhysAddr, uint32_t dstWidth, uint32_t dstHeight, uint32_t dstFormat,
							int dstRgnLeft, int dstRgnTop, int dstRgnRight, int dstRgnBottom)
{
	if (srcPhysAddr < BUFFER_BASEADDR_FILTER || srcWidth == 0 || srcHeight == 0
		|| dstPhysAddr < BUFFER_BASEADDR_FILTER || dstWidth == 0 || srcHeight == 0)
		return -1;

	int bytesPerPixel;
	gc_param.src.physAddr = srcPhysAddr - BUFFER_BASEADDR_FILTER;
	gc_param.src.width    = srcWidth;
	gc_param.src.height   = srcHeight;
	gc_param.src.format   = (uint32_t)gcconverter_get_gc_format(srcFormat, &bytesPerPixel);
	gc_param.src.stride   = srcWidth * bytesPerPixel;
	gc_param.src.rect.left   = srcCropLeft;
	gc_param.src.rect.top    = srcCropTop;
	gc_param.src.rect.right  = srcCropRight;
	gc_param.src.rect.bottom = srcCropBottom;
	if (srcCropRight <= 0 || srcCropBottom <= 0) {
		gc_param.src.rect.left   = 0;
		gc_param.src.rect.top    = 0;
		gc_param.src.rect.right  = srcWidth;
		gc_param.src.rect.bottom = srcHeight;
	}

	gc_param.dst.physAddr = dstPhysAddr - BUFFER_BASEADDR_FILTER;
	gc_param.dst.width    = dstWidth;
	gc_param.dst.height   = dstHeight;
	gc_param.dst.format   = (uint32_t)gcconverter_get_gc_format(dstFormat, &bytesPerPixel);
	gc_param.dst.stride   = dstWidth * bytesPerPixel;
	gc_param.dst.rect.left   = dstRgnLeft;
	gc_param.dst.rect.top    = dstRgnTop;
	gc_param.dst.rect.right  = dstRgnRight;
	gc_param.dst.rect.bottom = dstRgnBottom;
	if (dstRgnRight <= 0 || dstRgnBottom <= 0) {
		gc_param.dst.rect.left   = 0;
		gc_param.dst.rect.top    = 0;
		gc_param.dst.rect.right  = dstWidth;
		gc_param.dst.rect.bottom = dstHeight;
	}

	return 0;
}

int gcconverter_init(struct private_module_t* module)
{
	gc_param.gc_priv_mod = module;

	gctBOOL separated;
	gceSTATUS gcostat;
	gcostat = gcoOS_Construct(gcvNULL, &gc_param.gco_os);
	if (gcostat < 0)
		LOGE("*ERROR* Failed to construct OS object (status = %d)\n", gcostat);

	gcostat = gcoHAL_Construct(gcvNULL, gc_param.gco_os, &gc_param.gco_hal);
	if (gcostat < 0) {
		LOGE("*ERROR* Failed to construct GAL object (status = %d)\n", gcostat);
		return -1;
	}

	/* Switch to 2D core if has separated 2D/3D cores. */
	separated = gcoHAL_QuerySeparated3D2D(gc_param.gco_hal) == gcvSTATUS_TRUE;
	if (separated) {
		gcostat = gcoHAL_SetHardwareType(gc_param.gco_hal, gcvHARDWARE_2D);
		if (gcostat < 0) {
			LOGE("*ERROR* Failed to set hard ware type object (status = %d)\n", gcostat);
			return -1;
		}
	}

	/* Get gco2D object pointer. */
	gcostat = gcoHAL_Get2DEngine(gc_param.gco_hal, &gc_param.gco_2d);
	if (gcostat < 0) {
		LOGE("*ERROR* Failed to get 2D Engine (status = %d)\n", gcostat);
		return -1;
	}
	LOGV("%s complete: gco_os=0x%x, gco_hal=0x%x, gco_2d=0x%x", __FUNCTION__, gc_param.gco_os, gc_param.gco_hal, gc_param.gco_2d);

	gc_param.gc_mode = GC_MODE_NONE;
	gc_param.vindex = 0;
	gc_param.video_state = 0;
	memset(&gc_param.src, 0, sizeof(struct gcconv_pane));
	memset(&gc_param.dst, 0, sizeof(struct gcconv_pane));

	return 0;
}

int gcconverter_deinit()
{
	int status = -EINVAL;

	if (gc_param.gco_2d != gcvNULL)
		gcmVERIFY_OK(gco2D_FreeFilterBuffer(gc_param.gco_2d));

	if (gc_param.gco_hal != gcvNULL) {
		gcoHAL_Commit(gc_param.gco_hal, gcvTRUE);
		gcoHAL_Destroy(gc_param.gco_hal);
	}

	if (gc_param.gco_os != gcvNULL)
		gcoOS_Destroy(gc_param.gco_os);
	
	gc_param.gc_priv_mod = NULL;
	gc_param.gc_mode = GC_MODE_NONE;
	gc_param.video_state = 0;

	return 0;
}

#ifdef BOARD_USES_HDMI
int gcconverter_blit_ui(struct private_module_t *module, int index)
{
	if (!module || module->hdmi_state == 0)
		return -1;

	if (index < 0 || index > 1)
		index = 0;

	LOGV("gcconvert_blit_ui(index=0x%x)", index);

	if (gc_param.gc_mode == GC_MODE_NONE) {
		gc_param.gc_mode = GC_MODE_HDMI_UI;
		gc_param.video_state = 0;
	}
	else if (gc_param.gc_mode == GC_MODE_HDMI_VIDEO 
			|| gc_param.gc_mode == GC_MODE_UI_VIDEO) {
		return -1;
	}

	LOGV("%s gc_mode = %d.", __FUNCTION__, gc_param.gc_mode);

	int bytesPerPixel;
	uint32_t srcPhysAddr = (unsigned int)module->finfo.smem_start + \
							index * module->info.xres * module->info.yres * module->info.bits_per_pixel / 8;
	uint32_t srcWidth    = module->info.xres;
	uint32_t srcHeight   = module->info.yres;
	uint32_t srcFormat   = module->info.bits_per_pixel == 16 ? HAL_PIXEL_FORMAT_RGB_565 : HAL_PIXEL_FORMAT_BGRA_8888; // BGRA8888 is a display format for ns115
	
	uint32_t dstPhysAddr = (unsigned int)module->hdmi_finfo.smem_start + \
							index * module->hdmi_info.xres * module->hdmi_info.yres * module->hdmi_info.bits_per_pixel / 8;
	uint32_t dstWidth    = module->hdmi_info.xres;
	uint32_t dstHeight   = module->hdmi_info.yres;
	uint32_t dstFormat   = module->hdmi_info.bits_per_pixel == 16 ? HAL_PIXEL_FORMAT_RGB_565 : HAL_PIXEL_FORMAT_BGRA_8888; // BGRA8888 is a display format for ns115

	pthread_mutex_lock(&gc_param.lock);

	int status = gcconverter_set_param(srcPhysAddr, srcWidth, srcHeight, srcFormat, 0, 0, srcWidth, srcHeight, \
									dstPhysAddr, dstWidth, dstHeight, dstFormat, 0, 0, dstWidth, dstHeight);
	if (status < 0) {
		LOGE("ERROR: failed to set param.");
		pthread_mutex_unlock(&gc_param.lock);
		return status;
	}

	status = gcconverter_blit(&gc_param);

	pthread_mutex_unlock(&gc_param.lock);

	return status;
}
#endif

int gcconverter_start_blit_video(uint32_t srcPhysAddr, uint32_t srcWidth, uint32_t srcHeight, uint32_t srcFormat,
								int srcCropLeft, int srcCropTop, int srcCropRight, int srcCropBottom,
								uint32_t *pdstPhysAddr, uint32_t *pdstWidth, uint32_t *pdstHeight, uint32_t *pdstFormat)
{
	if (gc_param.gc_priv_mod == NULL) {
		LOGE("%s gc_priv_mod = NULL", __FUNCTION__);
		return -1;
	}
#ifdef BOARD_USES_HDMI	
	if (gc_param.gc_priv_mod->hdmi_state == 1) {
		if (gc_param.gc_mode != GC_MODE_HDMI_VIDEO) {
			gc_param.gc_mode = GC_MODE_HDMI_VIDEO;
		}
	} else
#endif
	{
		if (gc_param.gc_mode != GC_MODE_UI_VIDEO) {
			gc_param.gc_mode = GC_MODE_UI_VIDEO;
		}
	}
	LOGV("%s gc_mode = %d", __FUNCTION__, gc_param.gc_mode);

	*pdstWidth = 0;
	*pdstHeight = 0;
	*pdstPhysAddr = 0;
	*pdstFormat = 0;

	uint32_t rgbPhysAddr;
	uint32_t dstPA, dstW, dstH;
	uint32_t dstFmt, bytesPerPixel;
	int dstRgnLeft, dstRgnTop, dstRgnRight, dstRgnBottom;
	if (gc_param.gc_mode == GC_MODE_UI_VIDEO) {
		bytesPerPixel = 2;
		dstFmt = HAL_PIXEL_FORMAT_RGB_565; // it's fixed format, not to change
		rgbPhysAddr = gc_param.gc_priv_mod->finfo.smem_start + gc_param.gc_priv_mod->finfo.smem_len - 1280*720*2*2;
		dstW = srcWidth;
		dstH = srcHeight;
		// when using gc_convert, down-scaling to <=720P automatically
		if (dstW >= 1280 && dstH >= 720) {
			dstH = 720;
			if ((srcCropRight - srcCropLeft) > 0 && (srcCropBottom - srcCropTop) > 0)
				dstW = 720 * (srcCropRight - srcCropLeft) / (srcCropBottom - srcCropTop);
			else
				dstW = 720 * (srcWidth & ~7) / srcHeight;
			dstW = (dstW + 15) & ~15;
		}
		dstRgnLeft   = 0;
		dstRgnTop    = 0;
		dstRgnRight  = dstW;
		dstRgnBottom = dstH;
	}
#ifdef BOARD_USES_HDMI	
	else {
		bytesPerPixel = gc_param.gc_priv_mod->hdmi_info.bits_per_pixel / 8;
		dstFmt = (gc_param.gc_priv_mod->hdmi_info.bits_per_pixel == 16) ? HAL_PIXEL_FORMAT_RGB_565 : HAL_PIXEL_FORMAT_BGRA_8888;
		rgbPhysAddr = gc_param.gc_priv_mod->hdmi_finfo.smem_start;
		dstW = gc_param.gc_priv_mod->hdmi_info.xres;
		dstH = gc_param.gc_priv_mod->hdmi_info.yres;
		uint32_t w = dstW;
		uint32_t h = dstH;
		// when using gc_convert, down-scaling to <=720P automatically
		if (h >= 1280 && h >= 720) {
			h = 720;
			if ((srcCropRight - srcCropLeft) > 0 && (srcCropBottom - srcCropTop) > 0)
				w = 720 * (srcCropRight - srcCropLeft) / (srcCropBottom - srcCropTop);
			else
				w = 720 * (srcWidth & ~7) / srcHeight;
			w = (w + 15) & ~15;
		}
		dstRgnLeft   = (dstW - w) / 2;
		dstRgnTop    = (dstH - h) / 2;
		dstRgnRight  = dstRgnLeft + w;
		dstRgnBottom = dstRgnTop + h;
	}
#endif
	dstPA = rgbPhysAddr + gc_param.vindex * dstW * dstH * bytesPerPixel;

	pthread_mutex_lock(&gc_param.lock);

	int status = gcconverter_set_param(srcPhysAddr, srcWidth, srcHeight, srcFormat, \
										srcCropLeft, srcCropTop, srcCropRight, srcCropBottom, \
										dstPA, dstW, dstH, dstFmt, dstRgnLeft, dstRgnTop, dstRgnRight, dstRgnBottom);
	if (status != 0) {
		LOGE("%s failed to set param.", __FUNCTION__);
		pthread_mutex_unlock(&gc_param.lock);
		return status;
	}

	status = gcconverter_blit(&gc_param);
	if (status != 0) {
		LOGE("%s failed to set param.", __FUNCTION__);
		pthread_mutex_unlock(&gc_param.lock);
		return status;
	}

	pthread_mutex_unlock(&gc_param.lock);

	// you could limit only when GC_MODE_UI_VIDEO 
	// return properties of video upto UI
//	if (gc_param.gc_mode == GC_MODE_UI_VIDEO) {
	*pdstWidth = dstW;
	*pdstHeight = dstH;
	*pdstPhysAddr = dstPA;
	*pdstFormat = dstFmt;
//	}

#ifdef BOARD_USES_HDMI	 
	if (gc_param.gc_mode == GC_MODE_HDMI_VIDEO) {
		// GC_MODE_HDMI_VIDEO render video directly onto HDMI,
		// ignore UI composition.
		if (hdmi_post(gc_param.gc_priv_mod, gc_param.vindex) != 0)
			LOGE("%s after repainted HDMI video, failed to hdmi_post, index=%d", __FUNCTION__, gc_param.vindex);
	}
#endif
	gc_param.vindex = (gc_param.vindex + 1) % 2;

	return status;
}

int gcconverter_toggle_video_mode(int state)
{
	if (gc_param.gc_priv_mod == NULL) {
		LOGE("%s gc_priv_mod = NULL", __FUNCTION__);
		return -1;
	}
	
	if (state == gc_param.video_state)
		return 0;

	gc_param.video_state = (state == 0) ? 0 : 1;
	if (gc_param.video_state == 1) {
#ifdef BOARD_USES_HDMI	
		if (gc_param.gc_priv_mod->hdmi_state == 1) {
			if (gc_param.gc_mode != GC_MODE_HDMI_VIDEO) {
				gc_param.gc_mode = GC_MODE_HDMI_VIDEO;
			}
		} else
#endif
		{
			if (gc_param.gc_mode != GC_MODE_UI_VIDEO) {
				gc_param.gc_mode = GC_MODE_UI_VIDEO;
			}
		}
	} else {
#ifdef BOARD_USES_HDMI	
		if (gc_param.gc_priv_mod->hdmi_state == 1) {
			if (gc_param.gc_mode != GC_MODE_HDMI_UI) {
				gc_param.gc_mode = GC_MODE_HDMI_UI;
			}
		} else
#endif
		{
			if (gc_param.gc_mode != GC_MODE_NONE) {
				gc_param.gc_mode = GC_MODE_NONE;
			}
		}
	}

	return 0;
}

#endif
