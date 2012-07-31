/*
 * Copyright (C) 2011 The Android Open Source Project
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

/*
 * Contains implementation of a class NV21JpegCompressor that encapsulates a
 * converter between NV21, and JPEG formats.
 * Contains implementation of a class YuYvJpegCompressor that encapsulates a
 * converter between YUV422i, and JPEG formats.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "EmulatedCamera_JPEG"
#include <cutils/log.h>
#include "JpegCompressor.h"
#include "Effect.h"
#include "Converters.h"

extern "C" {
#include <jpeglib.h>
}

namespace android {

/*
 *  NV21JpegCompressor
 */
NV21JpegCompressor::NV21JpegCompressor()
    : Yuv420SpToJpegEncoder(mStrides)
{
}

NV21JpegCompressor::~NV21JpegCompressor()
{
}

/*****************************************************************************
 * Public API of NV21JpegCompressor
 ***************************************************************************/

status_t NV21JpegCompressor::compressRawImage(const void* image,
                                              int width,
                                              int height,
                                              int quality,
                                              int useEffect
                                              )
{
    LOGV("%s: %p[%dx%d]", __FUNCTION__, image, width, height);
    void* pY = const_cast<void*>(image);
    int offsets[2];
    offsets[0] = 0;
    offsets[1] = width * height;
    mStrides[0] = width;
    mStrides[1] = width;
    if (encode(&mStream, pY, width, height, offsets, quality)) {
        LOGV("%s: Compressed JPEG: %d[%dx%d] -> %d bytes",
             __FUNCTION__, (width * height * 12) / 8, width, height, mStream.getOffset());
        return NO_ERROR;
    } else {
        LOGE("%s: JPEG compression failed", __FUNCTION__);
        return errno ? errno : EINVAL;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
/*
 * NV21EffectJpegCompressor
 */

NV21EffectJpegCompressor::NV21EffectJpegCompressor(): mJpegBuffer(NULL), mJpegSize(0)
{
}

NV21EffectJpegCompressor::~NV21EffectJpegCompressor()
{
    if (NULL != mJpegBuffer)
    {
        delete [] mJpegBuffer;
        mJpegBuffer = NULL;
    }
}

/****************************************************************************
 * Public API of NV21EffectJpegCompressor
 ***************************************************************************/
status_t NV21EffectJpegCompressor::compressRawImage(const void* image,
                                         int width,
                                         int height,
                                         int quality,
                                         int useEffect
                                         )
{
    void * src = const_cast<void *>(image);
    uint8_t *yp = static_cast<uint8_t *>(src);
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    uint8_t *line_buffer;
    int bufferlen = width*height<<1;

    //LOGE("@TEI compress_NV21 with effect_to_jpeg: enter!" );
    line_buffer = (uint8_t*)calloc(width*3,1);
    mJpegBuffer = new uint8_t[bufferlen];
    mJpegSize = 0;

    if(!line_buffer){
        LOGE("@TEI compress_NV21_to_jpeg: line_buffer calloc failed!" );
        return -1;
    }
    if (!mJpegBuffer)
    {
        free(line_buffer);
        return -1;
    }

    cinfo.err = jpeg_std_error (&jerr);
    //LOGE("@TEI compress_NV21_to_jpeg: jpeg creat compress!" );
    jpeg_create_compress (&cinfo);
    jpeg_nf_stdio_dest(&cinfo,(char*)mJpegBuffer,&bufferlen );
    //LOGE("@TEI compress_NV21_to_jpeg: jpeg creat compress after!" );

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults (&cinfo);
    jpeg_set_quality (&cinfo, quality, TRUE);

    //LOGE("@TEI compress_NV21_to_jpeg: jpeg start compress before!" );
    jpeg_start_compress (&cinfo, TRUE);
    //LOGE("@TEI compress_NV21_to_jpeg: jpeg start compress after!" );

    uint8_t *vp = yp + (width*height);
    uint8_t *up = vp + 1;
    uint8_t *vpos=vp;
    uint8_t *upos=up;
    uint32_t rgb = 0xFFFFFFFF;
    int z = 0;
    while (cinfo.next_scanline < cinfo.image_height) {
        unsigned char *ptr = line_buffer;

        for (int x = 0; x < width; x += 2) {
           rgb = YUVToRGB32WithEffect(*yp, *up, *vp, useEffect);
           *ptr++ = R32(rgb);
           *ptr++ = G32(rgb);
           *ptr++ = B32(rgb);

           //
           yp++;
           rgb = YUVToRGB32WithEffect(*yp, *up, *vp, useEffect);
           *ptr++ = R32(rgb);
           *ptr++ = G32(rgb);
           *ptr++ = B32(rgb);
           //
           yp++;
           up += 2;
           vp += 2;
        }
        if (z == 0){
          vp = vpos;
          up = upos;
          z++;
        } else {
          vpos = vp;
          upos = up;
          z--;
        }

        row_pointer[0] = line_buffer;
        jpeg_write_scanlines (&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress (&cinfo);
    jpeg_destroy_compress (&cinfo);

    free (line_buffer);
    mJpegSize = bufferlen;
    LOGD("=====Jpeg size: %d ======", mJpegSize);
    return 0;
}

}; /* namespace android */
