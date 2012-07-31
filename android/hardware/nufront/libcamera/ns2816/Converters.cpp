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
 * Contains implemenation of framebuffer conversion routines.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "EmulatedCamera_Converter"
#include <cutils/log.h>
#include "Converters.h"



namespace android {

static void _YUV420SToRGB565(const uint8_t* Y,
                             const uint8_t* U,
                             const uint8_t* V,
                             int dUV,
                             uint16_t* rgb,
                             int width,
                             int height)
{
    const uint8_t* U_pos = U;
    const uint8_t* V_pos = V;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 2, U += dUV, V += dUV) {
            const uint8_t nU = *U;
            const uint8_t nV = *V;
            *rgb = YUVToRGB565(*Y, nU, nV);
            Y++; rgb++;
            *rgb = YUVToRGB565(*Y, nU, nV);
            Y++; rgb++;
        }
        if (y & 0x1) {
            U_pos = U;
            V_pos = V;
        } else {
            U = U_pos;
            V = V_pos;
        }
    }
}

static void _YUV420SToRGB32(const uint8_t* Y,
                            const uint8_t* U,
                            const uint8_t* V,
                            int dUV,
                            uint32_t* rgb,
                            int width,
                            int height)
{
    const uint8_t* U_pos = U;
    const uint8_t* V_pos = V;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 2, U += dUV, V += dUV) {
            const uint8_t nU = *U;
            const uint8_t nV = *V;
            *rgb = YUVToRGB32(*Y, nU, nV);
            Y++; rgb++;
            *rgb = YUVToRGB32(*Y, nU, nV);
            Y++; rgb++;
        }
        if (y & 0x1) {
            U_pos = U;
            V_pos = V;
        } else {
            U = U_pos;
            V = V_pos;
        }
    }
}

//zhuyx --add--[
static void _YUV420SColorEffectSwitch(uint8_t* Y,
                            uint8_t* U,
                            uint8_t* V,
                            int dUV,
                            //uint32_t* rgb,
                            int width,
                            int height,
                            int effectType)
{
    uint8_t* U_pos = U;
    uint8_t* V_pos = V;
    //LOGV("Converters::_YUV420SColorEffectSwitch");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 2, U += dUV, V += dUV) {
            uint8_t nU = *U;
            uint8_t nV = *V;
            //LOGV("Converters::_YUV420SColorEffectSwitch, call YUVEffectSwitch");
            YUVEffectSwitch(Y, &nU, &nV, effectType);
            Y++;
            YUVEffectSwitch(Y, &nU, &nV, effectType);
            Y++;
        }
        if (y & 0x1) {
            U_pos = U;
            V_pos = V;
        } else {
            U = U_pos;
            V = V_pos;
        }
    }
}

/*
static void _NV21ColorEffectSwitch(uint8_t* Y,
                         uint8_t* U,
                         uint8_t* V,
                         //uint32_t* rgb,
                         int width,
                         int height,
                         int effectType)
{
    _YUV420SColorEffectSwitch(Y, U, V, 2, width, height, effectType);
}
*/

void NV21ColorEffectSwitch(void* nv21,int width, int height, int effectType)
{
    int pix_total = width * height;
    uint8_t* y = reinterpret_cast<uint8_t*>(nv21);
    LOGV("Converters::NV21ColorEffectSwitch,call _YUV420SColorEffectSwitch");
    _YUV420SColorEffectSwitch(y, y + pix_total + 1, y + pix_total, 2, width, height, effectType);
}

//zhuyx --add--]
static void _YUV420SToRGB32WithEffect(const uint8_t* Y,
                            const uint8_t* U,
                            const uint8_t* V,
                            int dUV,
                            uint32_t* rgb,
                            int width,
                            int height,
                            int effectType)
{
    const uint8_t* U_pos = U;
    const uint8_t* V_pos = V;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 2, U += dUV, V += dUV) {
            const uint8_t nU = *U;
            const uint8_t nV = *V;
            *rgb = YUVToRGB32WithEffect(*Y, nU, nV, effectType);
            Y++; rgb++;
            *rgb = YUVToRGB32WithEffect(*Y, nU, nV, effectType);
            Y++; rgb++;
        }
        if (y & 0x1) {
            U_pos = U;
            V_pos = V;
        } else {
            U = U_pos;
            V = V_pos;
        }
    }
}

void YV12ToRGB565(const void* yv12, void* rgb, int width, int height)
{
    const int pix_total = width * height;
    const uint8_t* Y = reinterpret_cast<const uint8_t*>(yv12);
    const uint8_t* U = Y + pix_total;
    const uint8_t* V = U + pix_total / 4;
    _YUV420SToRGB565(Y, U, V, 1, reinterpret_cast<uint16_t*>(rgb), width, height);
}

void YV12ToRGB32(const void* yv12, void* rgb, int width, int height)
{
    const int pix_total = width * height;
    const uint8_t* Y = reinterpret_cast<const uint8_t*>(yv12);
    const uint8_t* V = Y + pix_total;
    const uint8_t* U = V + pix_total / 4;
    _YUV420SToRGB32(Y, U, V, 1, reinterpret_cast<uint32_t*>(rgb), width, height);
}

void YU12ToRGB32(const void* yu12, void* rgb, int width, int height)
{
    const int pix_total = width * height;
    const uint8_t* Y = reinterpret_cast<const uint8_t*>(yu12);
    const uint8_t* U = Y + pix_total;
    const uint8_t* V = U + pix_total / 4;
    _YUV420SToRGB32(Y, U, V, 1, reinterpret_cast<uint32_t*>(rgb), width, height);
}

/* Common converter for YUV 4:2:0 interleaved to RGB565.
 * y, u, and v point to Y,U, and V panes, where U and V values are interleaved.
 */
static void _NVXXToRGB565(const uint8_t* Y,
                          const uint8_t* U,
                          const uint8_t* V,
                          uint16_t* rgb,
                          int width,
                          int height)
{
    _YUV420SToRGB565(Y, U, V, 2, rgb, width, height);
}

/* Common converter for YUV 4:2:0 interleaved to RGB32.
 * y, u, and v point to Y,U, and V panes, where U and V values are interleaved.
 */
static void _NVXXToRGB32(const uint8_t* Y,
                         const uint8_t* U,
                         const uint8_t* V,
                         uint32_t* rgb,
                         int width,
                         int height)
{
    _YUV420SToRGB32(Y, U, V, 2, rgb, width, height);
}

static void _NVXXToRGB32WithEffect(const uint8_t* Y,
                         const uint8_t* U,
                         const uint8_t* V,
                         uint32_t* rgb,
                         int width,
                         int height,
                         int effectType)
{
    _YUV420SToRGB32WithEffect(Y, U, V, 2, rgb, width, height, effectType);
}

void NV12ToRGB565(const void* nv12, void* rgb, int width, int height)
{
    const int pix_total = width * height;
    const uint8_t* y = reinterpret_cast<const uint8_t*>(nv12);
    _NVXXToRGB565(y, y + pix_total, y + pix_total + 1,
                  reinterpret_cast<uint16_t*>(rgb), width, height);
}

void NV12ToRGB32(const void* nv12, void* rgb, int width, int height)
{
    const int pix_total = width * height;
    const uint8_t* y = reinterpret_cast<const uint8_t*>(nv12);
    _NVXXToRGB32(y, y + pix_total, y + pix_total + 1,
                 reinterpret_cast<uint32_t*>(rgb), width, height);
}

void NV21ToRGB565(const void* nv21, void* rgb, int width, int height)
{
    const int pix_total = width * height;
    const uint8_t* y = reinterpret_cast<const uint8_t*>(nv21);
    _NVXXToRGB565(y, y + pix_total + 1, y + pix_total,
                  reinterpret_cast<uint16_t*>(rgb), width, height);
}

void NV21ToRGB32(const void* nv21, void* rgb, int width, int height)
{
    const int pix_total = width * height;
    const uint8_t* y = reinterpret_cast<const uint8_t*>(nv21);
    _NVXXToRGB32(y, y + pix_total + 1, y + pix_total,
                 reinterpret_cast<uint32_t*>(rgb), width, height);
}

void NV21ToRGB32WithEffect(const void* nv21, void* rgb, int width, int height, int effectType)
{
    const int pix_total = width * height;
    const uint8_t* y = reinterpret_cast<const uint8_t*>(nv21);
    _NVXXToRGB32WithEffect(y, y + pix_total + 1, y + pix_total,
                 reinterpret_cast<uint32_t*>(rgb), width, height, effectType);
}

void YUYVToRGB32WithEffect(const void* yuyv, void* rgb, int width, int height, int effectType)
{
     const int pix_total = width * height;
     const uint8_t* src = reinterpret_cast<const uint8_t*>(yuyv);
     int row, col;
     int y, u, v;
     int z = 0;
    
     uint32_t* ptr = reinterpret_cast<uint32_t*>(rgb); 
     for (row = 0; row < height; row++)
    {
               
       for (col = 0; col < width; col++) {    
        if (!z)           
             y = src[0];            
        else              
             y = src[2];      
        u = src[1] ;       
        v = src[3];      
        *ptr = YUVToRGB32WithEffect(y, u, v, effectType);
                ptr++;
                
        if (z++) {        
            z = 0;          
            src += 4;    
        }    
    }
   }
}
}; /* namespace android */
