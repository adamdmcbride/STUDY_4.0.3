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
 * Contains implementation of a class YUV422IJpegCompressor that encapsulates a
 * converter between YUV422i, and JPEG formats.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "JPEG_COMPRESSOR"
#include <cutils/log.h>
#include "Converters.h"
#include "JpegCompressor.h"
#include "Effect.h"

extern "C" {
#include <jpeglib.h>
}

namespace android {
int JpegCompressor::compressRawImage(const void * image,
                                                                                          int width,
                                                                                          int height,
                                                                                          int quality,
                                                                                          int useEffect = 0)
{
    void * src = const_cast<void *>(image);
    uint8_t *yp = static_cast<uint8_t *>(src);
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    uint8_t *line_buffer;
    int bufferlen = width*height<<1;

    line_buffer = (uint8_t*)calloc(width*3,1);
    mJpegBuffer = new uint8_t[bufferlen];
    mJpegSize = 0;

    if(!line_buffer){
        LOGE("@TEI compress_yuv_to_jpeg: line_buffer calloc failed!" );
        return -1;
    }
    if (!mJpegBuffer)
    {
        free(line_buffer);
        return -1;
    }

    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_compress (&cinfo);
    jpeg_nf_stdio_dest(&cinfo,(char*)mJpegBuffer,&bufferlen );

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults (&cinfo);
    jpeg_set_quality (&cinfo, quality, TRUE);

    jpeg_start_compress (&cinfo, TRUE);
    
    uint8_t *vp, *up,*vpos, *upos;
    
    getYuvOffsets(width, height, yp, &up, &vp);
 
    vpos=vp;
    upos=up;
    
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
           yp += mStrides[0];
           rgb = YUVToRGB32WithEffect(*yp, *up, *vp, useEffect);
           *ptr++ = R32(rgb);
           *ptr++ = G32(rgb);
           *ptr++ = B32(rgb);
           //
           yp += mStrides[0];
           up += mStrides[1];
           vp += mStrides[2];
        }
        
        if (mIs420){
            if (z == 0){
                vp = vpos;
                up = upos;
                z++;
            } else {
                vpos = vp;
                upos = up;
                z--;
            }
        }

        row_pointer[0] = line_buffer;
        jpeg_write_scanlines (&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress (&cinfo);
    jpeg_destroy_compress (&cinfo);

    free (line_buffer);
    mJpegSize = bufferlen;
    LOGD("====== Jpeg size: %d ======", mJpegSize);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//YUV420spJpegCompressor
//
//////////////////////////////////////////////////////////////////////////////////

YUV420spJpegCompressor::YUV420spJpegCompressor(int inputFmt)
{
       mInputFmt = inputFmt;
       mJpegBuffer = NULL;
       mJpegSize = 0;
       mPlaneNum = 2;
       mStrides[0] = 1;   //Y stride
       mStrides[1] = 2;   //Cb stride
       mStrides[2] = 2;   //Cr stride
       mIs420 = true;
}

void YUV420spJpegCompressor::getYuvOffsets(int width, int height, 
                                                             uint8_t * pimg, uint8_t ** pu, uint8_t ** pv)
{
     if (JPG_INPUT_FMT_NV12 == mInputFmt)
    {
        *pu = pimg + (width*height);
        *pv =  *pu + 1;   
    } else if (JPG_INPUT_FMT_NV21 == mInputFmt){
        *pv = pimg + (width*height);
        *pu = *pv + 1;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
//
// YUV422IJpegCompressor
//
/////////////////////////////////////////////////////////////////////////////////

YUV422IJpegCompressor::YUV422IJpegCompressor(int inputFmt)
{    
        mInputFmt = inputFmt; 
        mJpegBuffer = NULL; 
        mJpegSize = 0; 
        mPlaneNum = 1;
        mStrides[0] = 2;   //Y stride
        mStrides[1] = 4;   //Cb stride
        mStrides[2] = 4;   //Cr stride
        mIs420 = false;
}

void YUV422IJpegCompressor::getYuvOffsets(int width, int height, 
                                                             uint8_t * pimg, uint8_t ** pu, uint8_t ** pv)
{
    if (JPG_INPUT_FMT_YUYV == mInputFmt)
    {
        *pu = pimg + 1;
        *pv = *pu + 2;   
    } else if (JPG_INPUT_FMT_YVYU == mInputFmt){
        *pv = pimg + 1;
        *pu = *pv + 2; 
    }
}

}; /* namespace android */
