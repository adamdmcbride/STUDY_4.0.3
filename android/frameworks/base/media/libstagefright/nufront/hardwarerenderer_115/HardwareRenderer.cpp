/*
 * Copyright (C) 2009 The Android Open Source Project
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

#define LOG_TAG "HardwareRenderer"
#include <utils/Log.h>

#include <binder/MemoryHeapBase.h>
#include <binder/MemoryHeapPmem.h>
#include <HardwareRenderer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MetaData.h>
#include <surfaceflinger/Surface.h>
#include <ui/android_native_buffer.h>
#include <ui/GraphicBufferMapper.h>
#include <gui/ISurfaceTexture.h>

#define DEFAULT_RENDER_BYTES_PERPIXEL  2.0

namespace android {

HardwareRenderer::HardwareRenderer(
        const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta)
    : mConverter(NULL),
      mYUVMode(None),
      mNativeWindow(nativeWindow),
      mBytesPerPixel(DEFAULT_RENDER_BYTES_PERPIXEL),
      mVideoMemMapped(false) {
    int32_t tmp;
    CHECK(meta->findInt32(kKeyColorFormat, &tmp));
    mColorFormat = (OMX_COLOR_FORMATTYPE)tmp;

    CHECK(meta->findInt32(kKeyWidth, &mWidth));
    CHECK(meta->findInt32(kKeyHeight, &mHeight));
    if (!meta->findInt32(kKeyDisplayWidth, &mDisplayWidth) ||
            !meta->findInt32(kKeyDisplayHeight, &mDisplayHeight)) {
        mDisplayWidth  = 0;
        mDisplayHeight = 0;
    }
    LOGV("mDisplayWidth:%d mDisplayHeight:%d", mDisplayWidth, mDisplayHeight);

    if (!meta->findRect(
                kKeyCropRect,
                &mCropLeft, &mCropTop, &mCropRight, &mCropBottom)) {
        mCropLeft = mCropTop = 0;
        mCropRight = mWidth - 1;
        mCropBottom = mHeight - 1;
    }

    mCropWidth = mCropRight - mCropLeft + 1;
    mCropHeight = mCropBottom - mCropTop + 1;

    int32_t rotationDegrees;
    if (!meta->findInt32(kKeyRotation, &rotationDegrees)) {
        rotationDegrees = 0;
    }

    int32_t flipHV;
    if (!meta->findInt32(kKeyFlip, &flipHV)) {
        flipHV = 0;
    }

    switch(mColorFormat) {
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_TI_COLOR_FormatYUV420PackedSemiPlanar:
            mHALFormat = HAL_PIXEL_FORMAT_YV12;
            break;
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_NUFRONT_COLOR_FormatYUV420SemiPlanar:
            mHALFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
            break;
        case OMX_COLOR_Format32bitARGB8888:
            mHALFormat = HAL_PIXEL_FORMAT_RGBA_8888;
            break;
        default:
            mHALFormat = HAL_PIXEL_FORMAT_RGB_565;
            break;
	}
	if (mColorFormat != OMX_COLOR_FormatYUV420Planar
        && mColorFormat != OMX_TI_COLOR_FormatYUV420PackedSemiPlanar
		&& mColorFormat != OMX_COLOR_FormatYUV420SemiPlanar
        && mColorFormat != OMX_NUFRONT_COLOR_FormatYUV420SemiPlanar) {
			mConverter = new ColorConverter(
						mColorFormat, OMX_COLOR_Format16bitRGB565);
			CHECK(mConverter->isValid());
	}

    CHECK(mNativeWindow != NULL);
    CHECK(mCropWidth > 0);
    CHECK(mCropHeight > 0);
    CHECK(mConverter == NULL || mConverter->isValid());

    CHECK_EQ(0,
            native_window_set_usage(
            mNativeWindow.get(),
            GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));

    CHECK_EQ(0,
            native_window_set_scaling_mode(
            mNativeWindow.get(),
            NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW));

    uint32_t transform;
    switch (rotationDegrees) {
        case 0: transform = 0; break;
        case 90: transform = HAL_TRANSFORM_ROT_90; break;
        case 180: transform = HAL_TRANSFORM_ROT_180; break;
        case 270: transform = HAL_TRANSFORM_ROT_270; break;
        default: transform = 0; break;
    }
    switch (flipHV) {
        case 0: break;
        case FLIP_H: transform = HAL_TRANSFORM_FLIP_H; break;
        case FLIP_V: transform = HAL_TRANSFORM_FLIP_V; break;
        default: break;
    }

    if (transform) {
        CHECK_EQ(0, native_window_set_buffers_transform(
                    mNativeWindow.get(), transform));
    }
}

HardwareRenderer::~HardwareRenderer() {
    if (mConverter) {
        delete mConverter;
        mConverter = NULL;
    }
}

static int ALIGN(int x, int y) {
    // y must be a power of 2.
    return (x + y - 1) & ~(y - 1);
}

void HardwareRenderer::render(
        const void *data, size_t size, void *platformPrivate) {
    Mutex::Autolock lock(mMutex);

    ANativeWindowBuffer *buf;
    int err;

    /* To avoid memcpy to surfaceflinger's gralloc(ump) memory area,
     * mapping video memory into frambuffer area.
     */
    mVideoMemMapped = false;

#ifdef USE_GC_CONVERTER
    TransformExternalMemInfo param;
    param.srcPA = (unsigned int)(*(unsigned int*)data);
    param.srcW  = mCropWidth;
    param.srcH  = mCropHeight;
    param.cropW = mDisplayWidth;
    param.cropH = mDisplayHeight;
    param.srcFmt = mHALFormat;
    if ( (err = mNativeWindow->perform(mNativeWindow.get(),
                NATIVE_WINDOW_API_TRANSFORM_EXTERNALMEM,
                &param)) == 0) {
        if (param.outPA != 0
            && param.outW > 0
            && param.outH > 0) {
            mVideoMemMapped = true;
	        CHECK_EQ(0, native_window_set_buffers_geometry(
                            mNativeWindow.get(),
                            (param.outW + 1) & ~1,
                            (param.outH + 1) & ~1,
                            param.outFmt));
        } else {
            return;
        }
    } else {
        LOGE("Surface::mapExternalMem returned error %d", err);
	    CHECK_EQ(0, native_window_set_buffers_geometry(
                        mNativeWindow.get(),
                        (mCropWidth + 1) & ~1,
                        (mCropHeight + 1) & ~1,
                        mHALFormat));
    }
#endif

    if ((err = mNativeWindow->dequeueBuffer(mNativeWindow.get(), &buf)) != 0) {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }

    if(!mVideoMemMapped) {
        CHECK_EQ(0, mNativeWindow->lockBuffer(mNativeWindow.get(), buf));

        GraphicBufferMapper &mapper = GraphicBufferMapper::get();

        Rect bounds(mCropWidth, mCropHeight);

        void *dst;
        CHECK_EQ(0, mapper.lock(
                    buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));

        /* If video memory failed to mapped into framebuffer area,
         * convert data to graphic memory by CPU
         */
        if (mConverter) {
            mConverter->convert(
                    data,
                    mWidth, mHeight,
                    mCropLeft, mCropTop, mCropRight, mCropBottom,
                    dst,
                    buf->stride, buf->height,
                    0, 0, mCropWidth - 1, mCropHeight - 1);
        } else if ( //mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar ||
                    mColorFormat == OMX_NUFRONT_COLOR_FormatYUV420SemiPlanar) {    /* m@nufront */
            if (dst != NULL && data != NULL) {
                 const uint8_t *src = (const uint8_t *)(*((const uint32_t *)data + 1));
                 int size = mCropWidth * mCropHeight * 1.5;
                 memcpy(dst, src, size);
             }
        } else if (mColorFormat == OMX_COLOR_FormatYUV420Planar) {
            const uint8_t *src_y = (const uint8_t *)data;
            const uint8_t *src_u = (const uint8_t *)data + mWidth * mHeight;
            const uint8_t *src_v = src_u + (mWidth / 2 * mHeight / 2);

            uint8_t *dst_y = (uint8_t *)dst;
            size_t dst_y_size = buf->stride * buf->height;
            size_t dst_c_stride = ALIGN(buf->stride / 2, 16);
            size_t dst_c_size = dst_c_stride * buf->height / 2;
            uint8_t *dst_v = dst_y + dst_y_size;
            uint8_t *dst_u = dst_v + dst_c_size;

            for (int y = 0; y < mCropHeight; ++y) {
                memcpy(dst_y, src_y, mCropWidth);

                src_y += mWidth;
                dst_y += buf->stride;
            }

            for (int y = 0; y < (mCropHeight + 1) / 2; ++y) {
                memcpy(dst_u, src_u, (mCropWidth + 1) / 2);
                memcpy(dst_v, src_v, (mCropWidth + 1) / 2);

                src_u += mWidth / 2;
                src_v += mWidth / 2;
                dst_u += dst_c_stride;
                dst_v += dst_c_stride;
            }
        } else {
            CHECK_EQ(mColorFormat, OMX_TI_COLOR_FormatYUV420PackedSemiPlanar);

            const uint8_t *src_y =
                (const uint8_t *)data;

            const uint8_t *src_uv =
                (const uint8_t *)data + mWidth * (mHeight - mCropTop / 2);

            uint8_t *dst_y = (uint8_t *)dst;

            size_t dst_y_size = buf->stride * buf->height;
            size_t dst_c_stride = ALIGN(buf->stride / 2, 16);
            size_t dst_c_size = dst_c_stride * buf->height / 2;
            uint8_t *dst_v = dst_y + dst_y_size;
            uint8_t *dst_u = dst_v + dst_c_size;

            for (int y = 0; y < mCropHeight; ++y) {
                memcpy(dst_y, src_y, mCropWidth);

                src_y += mWidth;
                dst_y += buf->stride;
            }

            for (int y = 0; y < (mCropHeight + 1) / 2; ++y) {
                size_t tmp = (mCropWidth + 1) / 2;
                for (size_t x = 0; x < tmp; ++x) {
                    dst_u[x] = src_uv[2 * x];
                    dst_v[x] = src_uv[2 * x + 1];
                }

                src_uv += mWidth;
                dst_u += dst_c_stride;
                dst_v += dst_c_stride;
            }
        }

        CHECK_EQ(0, mapper.unlock(buf->handle));
    }

    if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
}

void HardwareRenderer::ACodecRender(
        const void *data, size_t size, void *platformPrivate) {
    ANativeWindowBuffer *buf;
    int err;
    unsigned int * vAddr;
    /* To avoid memcpy to surfaceflinger's gralloc(ump) memory area,
     * mapping video memory into frambuffer area.
     */
    mVideoMemMapped = false;
    GraphicExternalMemInfo param;
    param.physAddr = (unsigned int)(*(unsigned int*)data);
    param.size = mCropWidth * mCropHeight * mBytesPerPixel;
/*  if ( (err = mNativeWindow->perform(mNativeWindow.get(), NATIVE_WINDOW_API_MAP_EXTERNALMEM, &param)) != 0) {
        LOGE("Surface::mapExternalMem returned error %d", err);
    } else {
        mVideoMemMapped = true;
    }
*/
    if ((err = mNativeWindow->dequeueBuffer(mNativeWindow.get(), &buf)) != 0) {
        LOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }

    if(!mVideoMemMapped) {
        CHECK_EQ(0, mNativeWindow->lockBuffer(mNativeWindow.get(), buf));

        GraphicBufferMapper &mapper = GraphicBufferMapper::get();

        Rect bounds(mCropWidth, mCropHeight);

        void *dst;
        CHECK_EQ(0, mapper.lock(
                    buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));

                vAddr=(unsigned int *)(*((unsigned int*)(data+4)));

                memcpy(dst,vAddr,param.size);

    }

    if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf)) != 0) {
        LOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
}


}  // namespace android
