/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2012, Nufront Co. LTD
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
*****************************************************
* Filename: NuCameraV4L2.cpp
* Author:   zhuyuxin, majiping
* Purpose:  This file interacts with the Camera  driver and VPU.
*****************************************************
*/
#define LOG_NDEBUG 0
#define LOG_TAG "NuCameraV4L2"
#include <utils/Log.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/poll.h>
#include "Effect.h"
#include "Converters.h"
#include "NuCameraV4L2.h"
#include "cutils/properties.h"

using namespace android;

#define CHECK(return_value)                                          \
    if (return_value < 0) {                                          \
        LOGE("%s::%d fail. errno: %s, m_camera_id = %d\n",           \
             __func__, __LINE__, strerror(errno), m_camera_id);      \
        return -1;                                                   \
    }


#define CHECK_PTR(return_value)                                      \
    if (return_value < 0) {                                          \
        LOGE("%s::%d fail, errno: %s, m_camera_id = %d\n",           \
             __func__,__LINE__, strerror(errno), m_camera_id);       \
        return NULL;                                                 \
    }

#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)

namespace android {

// ======================================================================
// Camera controls

    //add for ec power control
    int gEC_PW_FD = 0;
    extern "C" {
        struct PWPARM
        {
          unsigned char cmd;
          unsigned char data;
        };

        #define ENEEC_IOC_MAGIC      ('e' + 0x80)
        #define ENEEC_IOC_PW         _IOWR (ENEEC_IOC_MAGIC, 16, struct PWPARM)
        #define CAMERA_POWER_ON      12
        #define CAMERA_POWER_OFF     13

        static int power_on_camera(const int fd){
            int ret;
            struct PWPARM parm;
            parm.cmd = CAMERA_POWER_ON;
            ret = ioctl(fd, ENEEC_IOC_PW, &parm);
            if(ret < 0){
              return errno;
            }
            return 0;
        }

        static int power_off_camera(const int fd){
            int ret;
            struct PWPARM parm;
            parm.cmd = CAMERA_POWER_OFF;
            ret = ioctl(fd, ENEEC_IOC_PW, &parm);
            if(ret < 0){
              return errno;
            }
            return 0;
        }

    }

static struct timeval time_start;
static struct timeval time_stop;

unsigned long measure_time(struct timeval *start, struct timeval *stop)
{
    unsigned long sec, usec, time;

    sec = stop->tv_sec - start->tv_sec;

    if (stop->tv_usec >= start->tv_usec) {
        usec = stop->tv_usec - start->tv_usec;
    } else {
        usec = stop->tv_usec + 1000000 - start->tv_usec;
        sec--;
    }

    time = (sec * 1000000) + usec;

    return time;
}

static int get_pixel_depth(unsigned int fmt)
{
    int depth = 0;

    switch (fmt) {
    case V4L2_PIX_FMT_NV12:
        depth = 12;
        break;
    case V4L2_PIX_FMT_NV12T:
        depth = 12;
        break;
    case V4L2_PIX_FMT_NV21:
        depth = 12;
        break;
    case V4L2_PIX_FMT_YUV420:
        depth = 12;
        break;

    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_YUV422P:
        depth = 16;
        break;

    case V4L2_PIX_FMT_RGB32:
        depth = 32;
        break;
    }

    return depth;
}

#define ALIGN_W(x)      (((x) + 0x7F) & (~0x7F))    // Set as multiple of 128
#define ALIGN_H(x)      (((x) + 0x1F) & (~0x1F))    // Set as multiple of 32
#define ALIGN_BUF(x)    (((x) + 0x1FFF)& (~0x1FFF)) // Set as multiple of 8K

#if NUSMART_GC300_SCALING
static void nusmart_scalingDown420SP(void*src, int srcWidth, int srcHeight,
                                     void*dst, int dstWidth, int dstHeight)
{
    //To be implemented
    return;
}

#else
static void nusmart_scalingDown420SP(void*src, int srcWidth, int srcHeight,
                                     void*dst, int dstWidth, int dstHeight)
{
    int step_x, step_y;
    int x_off, y_off;
    unsigned char *pSrc = (unsigned char*)src;
    unsigned char *pDst = (unsigned char*)dst;
    //for Y planar
    step_x = srcWidth/dstWidth;
    step_y = srcHeight/dstHeight;
    x_off = (srcWidth%dstWidth)>>1;
    y_off = (srcHeight%dstHeight)>>1;

    //adjust pSrc to the start point of crop
    pSrc += (x_off + y_off*srcWidth);
    for (int y=0; y<dstHeight; y++)         // y sampling
    {
        for (int x=0; x<dstWidth; x++)  // x sampling
        { 
            *pDst++ = *(pSrc + x*step_x);
        }
        pSrc += step_y*srcWidth;
    }
    //adjust pSrc to start of UV planar
    pSrc = ((unsigned char*)src) + srcWidth*srcHeight;
    //adjust pSrc to the start point of crop
    pSrc += (x_off + (y_off>>1)*srcWidth);
    //for UV planar
    unsigned char *p;
    for (int y=0; y<dstHeight; y+=2)    // y sampling
    {
        for (int x=0; x<dstWidth; x+=2) // x sampling
        {
            p = pSrc + x*step_x;
            *pDst++ = *p;     
            *pDst++ = *(p + 1);
        }
        
        pSrc += step_y*srcWidth;
    }
}

#endif

static inline int nusmart_poll(struct pollfd *events, int timeout)
{
    int ret;

    ret = poll(events, 1, timeout);
    if (ret < 0) {
        LOGE("ERR(%s):poll error\n", __func__);
        return ret;
    }

    return ret;
}

static int nusmart_v4l2_querycap(int fp)
{
	  LOGV("%s:", __func__);
    struct v4l2_capability cap;
    int ret = 0;

    ret = ioctl(fp, VIDIOC_QUERYCAP, &cap);

    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QUERYCAP failed\n", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("ERR(%s):no capture devices\n", __func__);
        return -1;
    }

    return ret;
}

static const __u8* nusmart_v4l2_enuminput(int fp, int index)
{
    LOGV("%s:", __func__);
    static struct v4l2_input input;

    input.index = index;
    if (ioctl(fp, VIDIOC_ENUMINPUT, &input) != 0) {
        LOGE("ERR(%s):No matching index found\n", __func__);
        return NULL;
    }
    LOGI("Name of input channel[%d] is %s\n", input.index, input.name);

    return input.name;
}


static int nusmart_v4l2_s_input(int fp, int index)
{
	  LOGV("%s:", __func__);
    struct v4l2_input input;
    int ret;

    input.index = index;

    ret = ioctl(fp, VIDIOC_S_INPUT, &input);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_INPUT failed\n", __func__);
        return ret;
    }

    return ret;
}

static int nusmart_v4l2_set_crop(int fp,int left, int top, int width, int height){
    int ret;
    struct v4l2_crop crop;
    struct v4l2_rect rect;
    //zhuyx--add--[
    //NOTE:Our ISP crop function must set 8-times integer.
    int target_w,target_h;
    int remainder_w, remainder_h;
    remainder_w = width%8;
    if (remainder_w != 0){
        target_w = width - remainder_w;
    }else{
        target_w = width;
    }
    remainder_h = height%8;
    if (remainder_h != 0){
        target_h = height - remainder_h;
    }else{
        target_h = height;
    }
    //zhuyx--add--]
    rect.left = left;
    rect.top = top;
    rect.width = target_w;
    rect.height = target_h;
    LOGI("INFO(%s), The Actually Resolution Of Set Crop is:%dx%d .", __func__, rect.width, rect.height);
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = rect;

    ret = ioctl(fp, VIDIOC_S_CROP, &crop);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_CROP failed\n", __func__);
        return ret;
    }

    return 0;
}
static int nusmart_v4l2_set_isp_output_size(int fp,int width, int height){
    LOGV("%s,Set ISP Output resolution:%dx%d", __func__, width, height);
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = V4L2_CID_CAMERA_ISP_OUTPUT_SIZE;
    ctrl.value = ISP_SET_OUTPUT_SIZE(width, height);

    LOGV("Set ISP Output Size:0x%8x", ctrl.value);

    ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_CTRL(V4L2_CID_CAMERA_ISP_OUTPUT_SIZE) failed!,ERROR FOR:%s(%d)\n",
             __func__, strerror(errno), errno);
        return ret;
    }

    return 0;
}

/*
 * This function means was changed!
 * It's mean set ISP input pix fmt type
 */
int NuCameraV4L2::nusmart_v4l2_s_fmt(int fp, unsigned int width, unsigned int height, unsigned int fmt, int flag_capture)
{
    LOGV("%s: fp: %d ,wxh:%dx%d", __func__, fp, width, height);
    struct v4l2_format v4l2_fmt;
    memset(&v4l2_fmt, 0, sizeof(v4l2_fmt));
    int ret = 0;
    int max_width,max_height;
    unsigned int base_width,base_height;
    int base_ratio;
    int ratio_precision;
    ratio_precision = 100;

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fp, VIDIOC_G_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_G_FMT failed. %s(%d)\n", __func__, strerror(errno), errno);
        return -1;
    }
    LOGV("%s, call VIDIOC_TRY_FMT(%dx%d)", __func__, width, height);
    v4l2_fmt.fmt.pix.width = width;
    v4l2_fmt.fmt.pix.height = height;
    ret = ioctl(fp, VIDIOC_TRY_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_TRY_FMT failed. %s(%d)\n", __func__, strerror(errno), errno);
        return -1;
    }
    if(v4l2_fmt.fmt.pix.width < width || v4l2_fmt.fmt.pix.height < height){
        LOGE("ERR(%s):VIDIOC_TRY_FMT Return Wrong Resolution(%dx%d)!,it less than we set (%dx%d)", __func__,
             v4l2_fmt.fmt.pix.width, v4l2_fmt.fmt.pix.height, width, height);
        return -1;
    }
    base_width = v4l2_fmt.fmt.pix.width;
    base_height = v4l2_fmt.fmt.pix.height;
    LOGV("%s, TRY_FMT return support closely resolution:%dx%d", __func__, base_width, base_height);
    base_ratio = ((float)base_width/(float)base_height)*ratio_precision;
    if(base_width != width || base_height != height){
        LOGW("WARNING(%s):Sensor NOT Support Resolution: %dx%d", __func__, width, height);
        //We need set fmt to Sensor-Support-Closely-Resolution then crop and scaling by ISP.
        // Set up for capture
        LOGV("We need set fmt to Sensor-Support-Closely-Resolution(%dx%d) then crop and scaling by ISP",
                base_width, base_height);
        v4l2_fmt.fmt.pix.width = base_width;
        v4l2_fmt.fmt.pix.height = base_height;
        ret = ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt);
        if (ret < 0) {
            LOGE("ERR(%s):VIDIOC_S_FMT failed. %s(%d)\n", __func__, strerror(errno), errno);
            return -1;
        }
        //Caculate Crop
        int left,top;
        int target_ratio;
        int crop_w,crop_h;
        target_ratio = ((float)width/(float)height)*ratio_precision;
        LOGI("INFO(%s), base_ratio:%.2f, target_ratio:%.2f", __func__, ((float)base_ratio/ratio_precision),
                       ((float)target_ratio/ratio_precision));
        if (base_ratio == target_ratio){
            //Set target size as is output size and down scaling directly
            crop_w = width;
            crop_h = height;
            left = 0;
            top = 0;
            //crop to target ratio
            ret = nusmart_v4l2_set_crop(fp, left, top, base_width, base_height);
            if (ret < 0) {
                LOGE("ERR(%s):call nusmart_v4l2_set_crop failed.", __func__);
                return -1;
            }
        } else if(base_ratio > target_ratio) {
            crop_w = (base_height * target_ratio)/ratio_precision;
            crop_h = base_height;
            left = (base_width - crop_w) >> 1;
            top = 0;
            //crop to target ratio
            ret = nusmart_v4l2_set_crop(fp, left, top, crop_w, crop_h);
            if (ret < 0) {
                LOGE("ERR(%s):call nusmart_v4l2_set_crop failed.", __func__);
                return -1;
            }
        } else if(base_ratio < target_ratio) {
            crop_w = base_width;
            crop_h = (base_width/target_ratio)*ratio_precision;
            left = 0;
            top = (base_height - crop_h) >> 1;
            ret = nusmart_v4l2_set_crop(fp, left, top, crop_w, crop_h);
            if (ret < 0) {
                LOGE("ERR(%s):call nusmart_v4l2_set_crop failed.", __func__);
                return -1;
            }
        } else {
            LOGE("ERR(%s): Never Run Here.", __func__);
            return -1;
        }
        //set closely resolution flag
        m_sensor_support_resolution = CAMERA_SENSOR_NOT_SUPPORT_RESOLUTION;
        m_closely_width = crop_w;
        m_closely_height = crop_h;
    }else{
        LOGV("%s,VIDIOC_S_FMT: we set wxh: %dx%d", __func__, width, height);
        v4l2_fmt.fmt.pix.width = width;
        v4l2_fmt.fmt.pix.height = height;
        /* Set up for capture */
        ret = ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt);
        if (ret < 0) {
            LOGE("ERR(%s):VIDIOC_S_FMT failed. %s(%d)\n", __func__, strerror(errno), errno);
            return -1;
        }
        m_sensor_support_resolution = CAMERA_SENSOR_SUPPORT_RESOLUTION;
    }
    //Set ISP Output Size, down scaling
    LOGI("INFO(%s), Set Output Resolution:%dx%d", __func__, width, height);
    ret = nusmart_v4l2_set_isp_output_size(fp, width, height);
        if (ret < 0) {
            LOGE("ERR(%s):call nusmart_v4l2_set_crop failed.", __func__);
            return -1;
        }
    return 0;
}

// may be delete
static int nusmart_v4l2_s_fmt_cap(int fp, int width, int height, unsigned int fmt)
{
    LOGV("%s:", __func__);
    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;
    int ret;

    memset(&pixfmt, 0, sizeof(pixfmt));
    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    pixfmt.width = width;
    pixfmt.height = height;

    //Our ISP support yuv422 only as input stream type
    pixfmt.pixelformat = V4L2_PIX_FMT_YUYV;
    if (fmt == V4L2_PIX_FMT_JPEG) {
        pixfmt.colorspace = V4L2_COLORSPACE_JPEG;
    }
    pixfmt.sizeimage = (width * height * get_pixel_depth(V4L2_PIX_FMT_YUYV)) / 8;
    v4l2_fmt.fmt.pix = pixfmt;

    /* Set up for capture */
    ret = ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
        return ret;
    }

    return ret;
}

static int nusmart_v4l2_enum_fmt(int fp, unsigned int fmt)
{
    LOGV("%s:", __func__);
    struct v4l2_fmtdesc fmtdesc;
    int found = 0;

    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;

    while (ioctl(fp, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        if (fmtdesc.pixelformat == fmt) {
            LOGV("passed fmt = %#x found pixel format[%d]: %s\n", fmt, fmtdesc.index, fmtdesc.description);
            found = 1;
            break;
        }

        fmtdesc.index++;
    }

    if (!found) {
        LOGE("unsupported pixel format\n");
        return -1;
    }

    return 0;
}

static int nusmart_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs)
{
    LOGV("%s:", __func__);
    struct v4l2_requestbuffers req;
    int ret;

    req.count = nr_bufs;
    req.type = type;
    req.memory = V4L2_MEMORY_USERPTR;

    ret = ioctl(fp, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_REQBUFS failed\n", __func__);
        return -1;
    }

    return req.count;
}

static int nusmart_v4l2_querybuf(int fp, struct fimc_buffer *buffer, enum v4l2_buf_type type, int index)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = type;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    ret = ioctl(fp , VIDIOC_QUERYBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QUERYBUF failed\n", __func__);
        return -1;
    }

    buffer->length = v4l2_buf.length;
    if ((buffer->start = (char *)mmap(0, v4l2_buf.length,
                                         PROT_READ | PROT_WRITE, MAP_SHARED,
                                         fp, v4l2_buf.m.offset)) < 0) {
         LOGE("%s %d] mmap() failed\n",__func__, __LINE__);
         return -1;
    }

    LOGD("%s: buffer->start = %p v4l2_buf.length = %d",
         __func__, buffer->start, v4l2_buf.length);

    return 0;
}

static int nusmart_v4l2_streamon(int fp)
{
	  LOGV("%s:", __func__);
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(fp, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_STREAMON failed\n", __func__);
        return ret;
    }

    return ret;
}

static int nusmart_v4l2_streamoff(int fp)
{
	  LOGV("%s:", __func__);
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    LOGV("%s :", __func__);
    ret = ioctl(fp, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_STREAMOFF failed\n", __func__);
        return ret;
    }

    return ret;
}

static int nusmart_v4l2_qbuf(int fp, int index, unsigned long addr, unsigned int size)
{
     struct v4l2_buffer v4l2_buf;

     v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     v4l2_buf.memory = V4L2_MEMORY_USERPTR;
     v4l2_buf.index = index;
     v4l2_buf.m.userptr = addr;
     v4l2_buf.length = size;

     int ret = ioctl(fp, VIDIOC_QBUF, &v4l2_buf);
     if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QBUF failed. error for: %s(%d)\n", __func__, strerror(errno), errno);
        return ret;
     }

    return 0;
}

static int nusmart_v4l2_dqbuf(int fp, int *buffer_done)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_USERPTR;

    ret = ioctl(fp, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_DQBUF failed, dropped frame. error for: %s(%d)", __func__, strerror(errno), errno);
        return ret;
    }

    return v4l2_buf.index;
}


static int nusmart_v4l2_g_ctrl(int fp, unsigned int id)
{
    LOGE("%s:", __func__);
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;

    ret = ioctl(fp, VIDIOC_G_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s): VIDIOC_G_CTRL(id = 0x%x (%d)) failed, ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, ret);
        return ret;
    }

    return ctrl.value;
}

static int nusmart_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
    LOGV("%s:", __func__);
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.value = value;

    ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, value, ret);

        return ret;
    }

    return 0;
}

static int nusmart_v4l2_s_ext_ctrl(int fp, unsigned int id, void *value)
{
	  LOGV("%s:", __func__);
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.reserved = value;

    ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ctrls.count = 1;
    ctrls.controls = &ctrl;

    ret = ioctl(fp, VIDIOC_S_EXT_CTRLS, &ctrls);
    if (ret < 0)
        LOGE("ERR(%s):VIDIOC_S_EXT_CTRLS failed\n", __func__);

    return ret;
}

static int nusmart_v4l2_g_parm(int fp, struct v4l2_streamparm *streamparm)
{
	  LOGV("%s:", __func__);
    int ret;

    streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fp, VIDIOC_G_PARM, streamparm);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_G_PARM failed\n", __func__);
        return -1;
    }

    LOGV("%s : timeperframe: numerator %d, denominator %d\n", __func__,
            streamparm->parm.capture.timeperframe.numerator,
            streamparm->parm.capture.timeperframe.denominator);

    return 0;
}

static int nusmart_v4l2_s_parm(int fp, struct v4l2_streamparm *streamparm)
{
	  LOGV("%s:", __func__);
    int ret;
    streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fp, VIDIOC_S_PARM, streamparm);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_PARM failed\n", __func__);
        return ret;
    }

    return 0;
}

// ======================================================================
// Constructor & Destructor

NuCameraV4L2::NuCameraV4L2() :
            m_flag_init(0),
            m_camera_id(CAMERA_ID_BACK),
            m_cam_fd(-1),
            m_cam_fd2(-1),
            m_preview_v4lformat(V4L2_PIX_FMT_YUYV),
            m_preview_width      (0),
            m_preview_height     (0),
            m_preview_max_width  (MAX_BACK_CAMERA_PREVIEW_WIDTH),
            m_preview_max_height (MAX_BACK_CAMERA_PREVIEW_HEIGHT),
            m_snapshot_v4lformat(-1),
            m_snapshot_width      (0),
            m_snapshot_height     (0),
            m_snapshot_max_width  (MAX_BACK_CAMERA_SNAPSHOT_WIDTH),
            m_snapshot_max_height (MAX_BACK_CAMERA_SNAPSHOT_HEIGHT),
            m_angle(-1),
            m_anti_banding(-1),
            m_wdr(-1),
            m_anti_shake(-1),
            m_zoom_level(ZOOM_LEVEL_0),
            m_object_tracking(-1),
            m_smart_auto(-1),
            m_beauty_shot(-1),
            m_vintage_mode(-1),
            m_face_detect(-1),
            m_gps_enabled(false),
            m_gps_latitude(-1),
            m_gps_longitude(-1),
            m_gps_altitude(-1),
            m_gps_timestamp(-1),
            m_sensor_mode(-1),
            m_shot_mode(-1),
            m_exif_orientation(-1),
            m_blur_level(-1),
            m_video_gamma(-1),
            m_slow_ae(-1),
            m_camera_af_flag(-1),
            m_flag_camera_start(0),
            m_jpeg_thumbnail_width (0),
            m_jpeg_thumbnail_height(0),
            m_jpeg_quality(100),
            m_sensor_support_resolution(CAMERA_SENSOR_SUPPORT_RESOLUTION),
            m_closely_width(0),
            m_closely_height(0),
            m_ewl_instance(NULL),
            m_jpeg_ewl_instance(NULL),
            m_jpeg_encoder(NULL),
            m_jpeg_compressor(NULL),
            m_current_aec_value(0),
            m_current(0)
{
    m_params = (struct nufront_cam_parm*)&m_streamparm.parm.raw_data;
    m_params->capture.timeperframe.numerator = 1;
    m_params->capture.timeperframe.denominator = 0;
    m_params->contrast = -1;
    m_params->effects = -1;
    m_params->brightness = -1;
    m_params->flash_mode = -1;
    m_params->focus_mode = -1;
    m_params->iso = -1;
    m_params->metering = -1;
    m_params->saturation = -1;
    m_params->scene_mode = -1;
    m_params->sharpness = -1;
    m_params->white_balance = -1;




    //init ON2 instance
    EWLInitParam_t param;
    param.clientType = EWL_CLIENT_TYPE_H264_ENC; //as files default encorder
    if ((m_ewl_instance = EWLInit(&param)) == NULL){
        LOGE("%s : call EWLInit failed. %s(%d)", __func__, strerror(errno), errno);
    }
    memset(&m_ewl_mem_info, 0, sizeof(m_ewl_mem_info));

    LOGV("%s :", __func__);
}

NuCameraV4L2::~NuCameraV4L2()
{
    LOGV("%s :", __func__);
    //release ewl instance
    if (NULL != m_ewl_instance) {
        EWLRelease(m_ewl_instance);
        m_ewl_instance = NULL;
    }
}

int NuCameraV4L2::initCamera(int index)
{
    LOGV("NuCameraV4L2::initCamera,cameraID:%d", index);
    int ret = 0;

    if (!m_flag_init) {
        //Reset the lense position only during camera starts; don't do
        // reset between shot to shot
        m_camera_af_flag = -1;
        //change DFVS gover
        //there's no need to check read/write return value because it's not affects our logic.
        ret = readGovernor();
        if(ret < 0){
            LOGW("WARN(%s), read DVFS Governor Failed.It not affects our logic, do nitCamera continuous", __func__);
        }else{
            //call property_set when readGovernor return success
            property_set(DVFS_GOVERNOR_KEY, DVFS_GOVERNOR_PERFORMANCE);
        }
        //open camera device
        switch (index) {
        case CAMERA_ID_FRONT:
            m_cam_fd = openCameraDevice(CAMERA_ID_FRONT);
            if (m_cam_fd < 0) {
                LOGE("ERR(%s):Cannot open facing-front camera sensor.", __func__);
                return -1;
            }
            break;
        default:
        case CAMERA_ID_BACK:
            m_cam_fd = openCameraDevice(CAMERA_ID_BACK);
            if (m_cam_fd < 0) {
                LOGE("ERR(%s):Cannot open facing-back camera sensor.", __func__);
                return -1;
            }
        }

        ret = nusmart_v4l2_querycap(m_cam_fd);
        CHECK(ret);
         /*
         if (!nusmart_v4l2_enuminput(m_cam_fd, index))
            return -1;
        ret = nusmart_v4l2_s_input(m_cam_fd, index);
        CHECK(ret);*/
        m_camera_id = index;

        switch (m_camera_id) {
        case CAMERA_ID_FRONT:
            m_preview_max_width   = MAX_FRONT_CAMERA_PREVIEW_WIDTH;
            m_preview_max_height  = MAX_FRONT_CAMERA_PREVIEW_HEIGHT;
            m_snapshot_max_width  = MAX_FRONT_CAMERA_SNAPSHOT_WIDTH;
            m_snapshot_max_height = MAX_FRONT_CAMERA_SNAPSHOT_HEIGHT;
            break;

        case CAMERA_ID_BACK:
            m_preview_max_width   = MAX_BACK_CAMERA_PREVIEW_WIDTH;
            m_preview_max_height  = MAX_BACK_CAMERA_PREVIEW_HEIGHT;
            m_snapshot_max_width  = MAX_BACK_CAMERA_SNAPSHOT_WIDTH;
            m_snapshot_max_height = MAX_BACK_CAMERA_SNAPSHOT_HEIGHT;
            break;
        }

        setExifFixedAttribute();
        m_flag_init = 1;
    }
    m_zoom_level = ZOOM_LEVEL_0;
    return 0;
}

/*
 * camera_facing:   back or front
 * return: fd
 */
int NuCameraV4L2::openCameraDevice(int camera_facing)
{
    LOGV("%s :", __func__);
    int fd = -1;
    int ret = -1;
    int camera_back_dev = -1;
    if (camera_facing < 0 || camera_facing > 1){
        LOGE("ERR(%s): ERROR input params", __func__);
        return -1;
    }
    fd = open(CAMERA_DEV_VIDEO_0, O_RDWR);
    if (fd < 0) {
        LOGE("ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_VIDEO_0, strerror(errno));
        return -1;
    }
    ret = nusmart_v4l2_g_ctrl(fd, V4L2_CID_CAMERA_FACING);
    if (ret == NUSMART_CAMERA_FACING_BACK){
        m_camera_back_dev = 0;
    } else if (ret == NUSMART_CAMERA_FACING_FRONT){
        m_camera_back_dev = 1;
    } else {
        LOGE("ERR(%s): nusmart_v4l2_g_ctrl return error value", __func__);
        return -1;
    }
    if (camera_facing == CAMERA_ID_BACK){
        if (m_camera_back_dev == 0){
            LOGV("%s:open %s (FACING_BACK) success! fd: %d\n", __func__, CAMERA_DEV_VIDEO_0, fd);
            return fd;
        } else if (m_camera_back_dev == 1){
            close(fd);
            fd = -1;
            fd = open(CAMERA_DEV_VIDEO_1, O_RDWR);
            if (fd < 0) {
                LOGE("ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_VIDEO_1, strerror(errno));
                return -1;
            }
            LOGV("%s:open %s (FACING_BACK) success! fd: %d\n", __func__, CAMERA_DEV_VIDEO_1, fd);
            return fd;
        } else {
            LOGE("ERR(%s): ERROR input params", __func__);
            return -1;
        }
    }
    if (camera_facing == CAMERA_ID_FRONT){
        if (m_camera_back_dev == 0){
            close(fd);
            fd = -1;
            fd = open(CAMERA_DEV_VIDEO_1, O_RDWR);
            if (fd < 0) {
                LOGE("ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_VIDEO_1, strerror(errno));
                return -1;
            }
            LOGV("%s:open %s (FACING_FRONT) success! fd: %d\n", __func__, CAMERA_DEV_VIDEO_1, fd);
            return fd;
        } else if (m_camera_back_dev == 1){
            LOGV("%s:open %s (FACING_FRONT) success! fd: %d\n", __func__, CAMERA_DEV_VIDEO_0, fd);
            return fd;
        } else {
            LOGE("ERR(%s): ERROR input params", __func__);
            return -1;
        }
    }
    return -1;
}

void NuCameraV4L2::resetCamera()
{
    LOGV("%s :", __func__);
    DeinitCamera();
    initCamera(m_camera_id);
}

void NuCameraV4L2::DeinitCamera()
{
    if (m_flag_init) {

        stopRecord();

       /* close m_cam_fd after stopRecord() because stopRecord()
         * uses m_cam_fd to change frame rate
         */
        LOGV("DeinitCamera: m_cam_fd(%d)", m_cam_fd);
        if (m_cam_fd > -1) {
            close(m_cam_fd);
            m_cam_fd = -1;
        }
        //change DVFS governor
        property_set(DVFS_GOVERNOR_KEY, m_DVFS_governor);
        m_flag_init = 0;
    }
    else
    {
        LOGI("%s : already deinitialized", __FUNCTION__);
    }
}


int NuCameraV4L2::getCameraFd(void)
{
    return m_cam_fd;
}

// ======================================================================
// Preview
int NuCameraV4L2::startPreview(void)
{
    v4l2_streamparm streamparm;
    struct nufront_cam_parm *parms;
    parms = (struct nufront_cam_parm*)&streamparm.parm.raw_data;
    LOGV("%s :", __func__);

    // aleady started
    if (m_flag_camera_start > 0) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

    memset(&m_ewl_mem_info, 0, sizeof(m_ewl_mem_info));
    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }
    //zhuyx--add--[
    memset(&m_events_c, 0, sizeof(m_events_c));
    m_events_c.fd = m_cam_fd;
    m_events_c.events = POLLIN | POLLERR;
    //zhuyx--add--]
    // enum_fmt
    int ret = 0;
    //ret = nusmart_v4l2_enum_fmt(m_cam_fd,m_preview_v4lformat);
    //CHECK(ret);

    // set format sampling
    //CAMERA ISP support YUV422 only, output can support multi pix type
    ret = nusmart_v4l2_s_fmt(m_cam_fd, m_preview_width, m_preview_height, m_preview_v4lformat, 0);
    CHECK(ret);

    //zhuyx--add--[
    setZoom(m_preview_width, m_preview_height, m_zoom_level);
    //zhuyx--add--]

    ret = nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_NUSMART_STREAM_OUTPUT_PATH, STREAM_CAPTURE);
    CHECK(ret);

    // register number of buffers
    ret = nusmart_v4l2_reqbufs(m_cam_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, NUM_PREVIEW_BUFFERS);
    CHECK(ret);

    // alloc buffer, NOTE: buf_size != previewframe size
    unsigned int buf_size = MAX_BACK_CAMERA_PREVIEW_WIDTH * MAX_BACK_CAMERA_PREVIEW_HEIGHT << 1;
    EWLLinearMem_t ewl_buf;
    memset(&ewl_buf, 0, sizeof(ewl_buf));
    ret = EWLMallocLinear(m_ewl_instance, buf_size*NUM_PREVIEW_BUFFERS, &ewl_buf);
    CHECK(ret);

    // put the buffer info into table, and enqueue
    mBufferSize = ewl_buf.size;
    m_ewl_mem_info[0].size = buf_size;
    m_ewl_mem_info[0].busAddress = ewl_buf.busAddress;
    m_ewl_mem_info[0].virtualAddress = ewl_buf.virtualAddress;
    ret = nusmart_v4l2_qbuf(m_cam_fd, 0, (unsigned long)m_ewl_mem_info[0].virtualAddress, buf_size);
    if(ret < 0){
        LOGE("ERR(%s):nusmart_v4l2_qbuf Failed, call EWLFreeLinear!\n", __func__);
        if (ewl_buf.virtualAddress != NULL ){
            EWLFreeLinear(m_ewl_instance, &ewl_buf);
        }
        return -1;
    }
    for (int i = 1; i < NUM_PREVIEW_BUFFERS; i++) {
        m_ewl_mem_info[i].size = buf_size;
        m_ewl_mem_info[i].busAddress = m_ewl_mem_info[i-1].busAddress + buf_size;
        m_ewl_mem_info[i].virtualAddress = (unsigned int*)((unsigned int)(m_ewl_mem_info[i-1].virtualAddress) + buf_size);
        // queue buffer
        ret = nusmart_v4l2_qbuf(m_cam_fd, i, (unsigned long)m_ewl_mem_info[i].virtualAddress, buf_size);
        if(ret < 0){
            LOGE("ERR(%s):nusmart_v4l2_qbuf Failed, call EWLFreeLinear!\n", __func__);
            if (ewl_buf.virtualAddress != NULL ){
                EWLFreeLinear(m_ewl_instance, &ewl_buf);
            }
            return -1;
        }
    }

    // stream on
    ret = nusmart_v4l2_streamon(m_cam_fd);
    CHECK(ret);

    m_flag_camera_start = 1;

    return 0;
}

int NuCameraV4L2::stopPreview(void)
{
    LOGV("%s", __func__);
    int ret;

    if (m_flag_camera_start == 0) {
        LOGW("%s: doing nothing because m_flag_camera_start is zero", __func__);
        return 0;
    }

    if (m_cam_fd < 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }

    // switch off output
    ret = nusmart_v4l2_streamoff(m_cam_fd);
    CHECK(ret);

    // release memory
    if (m_ewl_mem_info[0].virtualAddress != NULL){
        LOGV("%s, call EWLFreeLinear(m_ewl_instance)", __func__);
        m_ewl_mem_info[0].size = mBufferSize;
        EWLFreeLinear(m_ewl_instance, &m_ewl_mem_info[0]);
    }
    m_flag_camera_start = 0;

    return ret;
}

//Recording
int NuCameraV4L2::startRecord(void)
{


    return 0;
}

int NuCameraV4L2::stopRecord(void)
{


    return 0;
}

void NuCameraV4L2::pausePreview()
{
    nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_STREAM_PAUSE, 0);
}

int NuCameraV4L2::previewPoll()
{
    int ret;
    memset(&m_events_c, 0, sizeof(m_events_c));
    m_events_c.fd = m_cam_fd;
    m_events_c.events = POLLIN | POLLERR;
    ret = poll(&m_events_c, 1, 1000);
    if (ret < 0) {
        LOGE("ERR(%s):poll error\n", __func__);
        return ret;
    }
    if (ret == 0) {
        LOGE("ERR(%s):No data in 1 secs.. Camera Device Reset \n", __func__);
        return ret;
    }
    return ret;
}


int NuCameraV4L2::getPreviewFrame()
{
    int index;
    int ret;
    index = -1;
    if ((m_flag_camera_start == 0) || previewPoll() == 0) {
        LOGE("ERR(%s):Start Camera Device Reset \n", __func__);
       /*
         * When there is no data for more than 1 second from the camera we inform
         * the FIMC driver by calling nusmart_v4l2_s_input() with a special value = 1000
         * FIMC driver identify that there is something wrong with the camera
         * and it restarts the sensor.
         */
        stopPreview();
        /* Reset Only Camera Device */

        ret = nusmart_v4l2_querycap(m_cam_fd);
        CHECK(ret);

        ret = startPreview();
        if (ret < 0) {
            LOGE("ERR(%s): startPreview() return %d\n", __func__, ret);
            return 0;
        }
    }

    int buffer_done;
    index = nusmart_v4l2_dqbuf(m_cam_fd, &buffer_done);
    if ((0 > index) || (index >= NUM_PREVIEW_BUFFERS)) {
        LOGE("ERR(%s):wrong index = %d\n", __func__, index);
        return -1;
    }

    //since sensor's frame rate larger than that expected, maybe dropping frame is necessary 
    memset(&m_events_c, 0, sizeof(m_events_c));
    m_events_c.fd = m_cam_fd;
    m_events_c.events = POLLIN | POLLRDNORM;
    ret = nusmart_poll(&m_events_c, 0);
    if (ret > 0){
        int tmp_index = nusmart_v4l2_dqbuf(m_cam_fd, &buffer_done);
        if ((0 > tmp_index) || (tmp_index >= NUM_PREVIEW_BUFFERS)) {
            LOGE("ERR(%s):wrong tmp_index = %d\n", __func__, tmp_index);
            return -1;
        } else {
            //drop redundant frame, to keep buffer in order, drop the frame before
            ret = nusmart_v4l2_qbuf(m_cam_fd, index, (unsigned long)m_ewl_mem_info[index].virtualAddress, m_ewl_mem_info[index].size);
            CHECK(ret);
            index = tmp_index;
        }
    }

    m_current = index;
    ret = nusmart_v4l2_qbuf(m_cam_fd, index, (unsigned long)m_ewl_mem_info[index].virtualAddress, m_ewl_mem_info[index].size);
    CHECK(ret);

    return index;
}


int NuCameraV4L2::getRecordFrame()
{

    return m_current;
}

int NuCameraV4L2::setPreviewSize(int width, int height, int pixel_format)
{
    LOGV("%s(width(%d), height(%d), format(%d))", __func__, width, height, pixel_format);

    int v4lpixelformat = pixel_format;

#if LOG_DEBUG
    if (v4lpixelformat == V4L2_PIX_FMT_YUV420)
        LOGV("PreviewFormat:V4L2_PIX_FMT_YUV420");
    else if (v4lpixelformat == V4L2_PIX_FMT_NV12)
        LOGV("PreviewFormat:V4L2_PIX_FMT_NV12");
    else if (v4lpixelformat == V4L2_PIX_FMT_NV12T)
        LOGV("PreviewFormat:V4L2_PIX_FMT_NV12T");
    else if (v4lpixelformat == V4L2_PIX_FMT_NV21)
        LOGV("PreviewFormat:V4L2_PIX_FMT_NV21");
    else if (v4lpixelformat == V4L2_PIX_FMT_YUV422P)
        LOGV("PreviewFormat:V4L2_PIX_FMT_YUV422P");
    else if (v4lpixelformat == V4L2_PIX_FMT_YUYV)
        LOGV("PreviewFormat:V4L2_PIX_FMT_YUYV");
    else if (v4lpixelformat == V4L2_PIX_FMT_RGB565)
        LOGV("PreviewFormat:V4L2_PIX_FMT_RGB565");
    else
        LOGV("PreviewFormat:UnknownFormat");
#endif

    m_preview_width  = width;
    m_preview_height = height;
    m_preview_v4lformat = v4lpixelformat;

    return 0;
}

int NuCameraV4L2::getPreviewSize(int *width, int *height, int *frame_size)
{
    *width  = m_preview_width;
    *height = m_preview_height;
    *frame_size = m_frameSize(m_preview_v4lformat, m_preview_width, m_preview_height);

    return 0;
}

int NuCameraV4L2::getPreviewMaxSize(int *width, int *height)
{
    LOGV("NuCameraV4L2::getPreviewMaxSize");
    *width  = m_preview_max_width;
    *height = m_preview_max_height;
    return 0;
}

int NuCameraV4L2::getPreviewPixelFormat(void)
{
    return m_preview_v4lformat;
}


// ======================================================================
// Snapshot
/*
 * Devide getJpeg() as two funcs, setSnapshotCmd() & getJpeg() because of the shutter sound timing.
 * Here, just send the capture cmd to camera ISP to start JPEG capture.
 */
int NuCameraV4L2::setSnapshotCmd(void)
{


    int ret = 0;

    LOG_TIME_DEFINE(0)
    LOG_TIME_DEFINE(1)

    if (m_flag_camera_start > 0) {
        LOG_TIME_START(0)
        LOGW("WARN(%s):Camera was in preview, should have been stopped\n", __func__);
        stopPreview();
        LOG_TIME_END(0)
    }

    if (m_cam_fd < 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return 0;
    }

    memset(&m_ewl_capture_mem_info, 0 ,sizeof(m_ewl_capture_mem_info));

    LOGV("%s :", __func__);
    LOG_TIME_START(1) // prepare

    //ret = nusmart_v4l2_enum_fmt(m_cam_fd,m_preview_v4lformat);
    //CHECK(ret);
    ret = nusmart_v4l2_s_fmt(m_cam_fd, m_snapshot_width, m_snapshot_height, m_preview_v4lformat, 0);
    CHECK(ret);

    //zhuyx--add--[
    setZoom(m_snapshot_width, m_snapshot_height, m_zoom_level);
    //zhuyx--add--]

    ret = nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_NUSMART_STREAM_OUTPUT_PATH, STREAM_CAPTURE);
    CHECK(ret);

    ret = nusmart_v4l2_reqbufs(m_cam_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, NUM_CAPTURE_BUFFERS);
    CHECK(ret);
    //request buffers
    EWLLinearMem_t ewl_buf;
    unsigned int buf_size = MAX_BACK_CAMERA_SNAPSHOT_WIDTH*MAX_BACK_CAMERA_SNAPSHOT_HEIGHT << 1;
    ret = EWLMallocLinear(m_jpeg_ewl_instance, buf_size*NUM_CAPTURE_BUFFERS, &ewl_buf);
    if (ret < 0){
         LOGE("ERR(%s):EWLMallocLinear Failed! size:%d\n", __func__, buf_size*NUM_CAPTURE_BUFFERS);
         return -1;
    }

    m_ewl_capture_mem_info[0].size = buf_size;
    m_ewl_capture_mem_info[0].busAddress = ewl_buf.busAddress;
    m_ewl_capture_mem_info[0].virtualAddress = ewl_buf.virtualAddress;
    mBufferSize = ewl_buf.size;
    ret = nusmart_v4l2_qbuf(m_cam_fd, 0, (unsigned long)m_ewl_capture_mem_info[0].virtualAddress, buf_size);
    if(ret < 0){
        LOGE("ERR(%s):nusmart_v4l2_qbuf Failed, call EWLFreeLinear!\n", __func__);
        if (ewl_buf.virtualAddress != NULL ){
            EWLFreeLinear(m_jpeg_ewl_instance, &ewl_buf);
        }
        return -1;
    }

    for (int i = 1; i < NUM_CAPTURE_BUFFERS; i++) {
        m_ewl_capture_mem_info[i].size = buf_size;
        m_ewl_capture_mem_info[i].busAddress = m_ewl_capture_mem_info[i-1].busAddress + buf_size;
        m_ewl_capture_mem_info[i].virtualAddress = (unsigned int*)((unsigned int)(m_ewl_capture_mem_info[i-1].virtualAddress) + buf_size);
        LOGV("%s, busAddress[%d]:0x%x, virtualAddress[%d]:0x%x ", __func__, i, (unsigned int)m_ewl_capture_mem_info[i].busAddress,
                  i, (unsigned int)m_ewl_capture_mem_info[i].virtualAddress);
        ret = nusmart_v4l2_qbuf(m_cam_fd, i, (unsigned long)m_ewl_capture_mem_info[i].virtualAddress, buf_size);
        if(ret < 0){
            LOGE("ERR(%s):nusmart_v4l2_qbuf Failed, call EWLFreeLinear!\n", __func__);
            if (ewl_buf.virtualAddress != NULL ){
                EWLFreeLinear(m_jpeg_ewl_instance, &ewl_buf);
            }
            return -1;
        }
    }
    ret = nusmart_v4l2_streamon(m_cam_fd);
    CHECK(ret);
    LOG_TIME_END(1)

    return 0;
}

int NuCameraV4L2::endSnapshot(void)
{
    int ret;
    LOGI("%s : call v4l2_streamoff", __func__);
    ret = nusmart_v4l2_streamoff(m_cam_fd);
    CHECK(ret);

    //release frame buffer
    if (m_ewl_capture_mem_info[0].virtualAddress != NULL){
        m_ewl_capture_mem_info[0].size = mBufferSize;
        EWLFreeLinear(m_jpeg_ewl_instance, &m_ewl_capture_mem_info[0]);
    }

    //release jpeg buffer
    if(m_jpeg_outbuf.virtualAddress != NULL){
        EWLFreeLinear(m_jpeg_ewl_instance, &m_jpeg_outbuf);
    }

    //release thumbnail buffer
    if(m_thumbnail_outbuf.virtualAddress != NULL){
        EWLFreeLinear(m_jpeg_ewl_instance, &m_thumbnail_outbuf);
    }

    //release encoder
    if(NULL != m_jpeg_encoder)
    {
        JpegEncRelease(m_jpeg_encoder);
        m_jpeg_encoder = NULL;
        m_jpeg_ewl_instance = NULL;
    }
    //release compressor
    if (NULL != m_jpeg_compressor){
       delete m_jpeg_compressor;
       m_jpeg_compressor = NULL;
    }
    return ret;
}

/*
 * Set Jpeg quality & exif info and get JPEG data from camera ISP
 */
int NuCameraV4L2::getSnapshotFrame()
{
    LOG_TIME_DEFINE(2)
    LOGV("%s", __func__);
    //zhuyx--add--[
    int ret = -1;
    int index = -1;
    memset(&m_events_c, 0, sizeof(m_events_c));
    m_events_c.fd = m_cam_fd;
    m_events_c.events = POLLIN | POLLRDNORM;
    ret = nusmart_poll(&m_events_c, 3000);
    int buffer_done;
    if(ret > 0){
        index = nusmart_v4l2_dqbuf(m_cam_fd, &buffer_done);
        if (index < 0) {
            LOGE("ERR(%s):wrong index = %d\n", __func__, index);
            return index;
        }

        int ret = nusmart_v4l2_qbuf(m_cam_fd, index, (unsigned long)m_ewl_capture_mem_info[index].virtualAddress, m_ewl_capture_mem_info[index].size);
        if(ret < 0){
            LOGE("ERR(%s):nusmart_v4l2_qbuf Failed!", __func__);
            return -1;
        }
    }
    LOG_TIME_END(2)

    return index;
}

int NuCameraV4L2::getExif(unsigned char *pExifDst)
{
    ExifEncoder exifEnc;
    unsigned int exifSize = 0;

    setExifChangedAttribute();

    LOGV("%s: calling exifEnc.makeExif, mExifInfo.width set to %d, height to %d\n",
         __func__, mExifInfo.width, mExifInfo.height);

    exifEnc.makeExif(pExifDst, &mExifInfo, &exifSize, true);

    return exifSize;
}


int NuCameraV4L2::setExifOrientationInfo(int orientationInfo)
{
     LOGV("%s(orientationInfo(%d))", __func__, orientationInfo);

     if (orientationInfo < 0) {
         LOGE("ERR(%s):Invalid orientationInfo (%d)", __func__, orientationInfo);
         return -1;
     }
     m_exif_orientation = orientationInfo;

     return 0;
}

void NuCameraV4L2::setExifChangedAttribute()
{
    //0th IFD TIFF Tags
    // Image size
    mExifInfo.width = m_snapshot_width;
    mExifInfo.height = m_snapshot_height;

    //Orientation
    switch (m_exif_orientation) {
    case 0:
        mExifInfo.orientation = EXIF_ORIENTATION_UP;
        break;
    case 90:
        mExifInfo.orientation = EXIF_ORIENTATION_270;//EXIF_ORIENTATION_90;
        break;
    case 180:
        mExifInfo.orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        mExifInfo.orientation = EXIF_ORIENTATION_90;//EXIF_ORIENTATION_270;
        break;
    default:
        mExifInfo.orientation = EXIF_ORIENTATION_UP;
        break;
    }
    // Date time
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime((char *)mExifInfo.date_time, 20, "%Y:%m:%d %H:%M:%S", timeinfo);

    // 0th IFD Exif Private Tags
    // Exposure Time
    int shutterSpeed = nusmart_v4l2_g_ctrl(m_cam_fd,
                                            V4L2_CID_CAMERA_SHT_TIME);
    /* TBD - front camera needs to be fixed to support this g_ctrl,
       it current returns a negative err value, so avoid putting
       odd value into exif for now */
    if (shutterSpeed < 0) {
        LOGW("%s: %d getting shutterSpeed, camera_id = %d, using 100",
             __func__, shutterSpeed, m_camera_id);
        shutterSpeed = 10000;
    }
    mExifInfo.exposure_time.num = 1;
    // x us -> 1/x s */
    mExifInfo.exposure_time.den = (uint32_t)(1000000 / shutterSpeed);

    // ISO Speed Rating
    int iso = nusmart_v4l2_g_ctrl(m_cam_fd, V4L2_CID_CAMERA_ISO);
    /* TBD - front camera needs to be fixed to support this g_ctrl,
       it current returns a negative err value, so avoid putting
       odd value into exif for now */
    if (iso < 0) {
        LOGW("%s:  %d getting iso, camera_id = %d, using 100",
             __func__, iso, m_camera_id);
        iso = ISO_100;
    }
    switch(iso) {
        case ISO_50:
            mExifInfo.iso_speed_rating = 50;
            break;
        case ISO_100:
            mExifInfo.iso_speed_rating = 100;
            break;
        case ISO_200:
            mExifInfo.iso_speed_rating = 200;
            break;
        case ISO_400:
            mExifInfo.iso_speed_rating = 400;
            break;
        case ISO_800:
            mExifInfo.iso_speed_rating = 800;
            break;
        case ISO_1600:
            mExifInfo.iso_speed_rating = 1600;
            break;
        default:
            mExifInfo.iso_speed_rating = 100;
            break;
    }

    uint32_t av, tv, bv, sv, ev;
    av = APEX_FNUM_TO_APERTURE((double)mExifInfo.fnumber.num / mExifInfo.fnumber.den);
    tv = APEX_EXPOSURE_TO_SHUTTER((double)mExifInfo.exposure_time.num / mExifInfo.exposure_time.den);
    sv = APEX_ISO_TO_FILMSENSITIVITY(mExifInfo.iso_speed_rating);
    bv = av + tv - sv;
    ev = av + tv;
    LOGD("Shutter speed=%d us, iso=%d\n", shutterSpeed, mExifInfo.iso_speed_rating);
    LOGD("AV=%d, TV=%d, SV=%d\n", av, tv, sv);

    // Shutter Speed
    mExifInfo.shutter_speed.num = tv*EXIF_DEF_APEX_DEN;
    mExifInfo.shutter_speed.den = EXIF_DEF_APEX_DEN;
    // Brightness
    mExifInfo.brightness.num = bv*EXIF_DEF_APEX_DEN;
    mExifInfo.brightness.den = EXIF_DEF_APEX_DEN;
    // Exposure Bias
    mExifInfo.exposure_bias.num = m_params->brightness;
    mExifInfo.exposure_bias.den = 1;

    //metering
    mExifInfo.metering_mode = EXIF_METERING_CENTER;

    // Flash
    if (m_camera_id == CAMERA_ID_BACK){

        switch (m_params->flash_mode){
            case FLASH_MODE_OFF:
                mExifInfo.flash = EXIF_FLASH_OFF;
                break;

            case FLASH_MODE_ON:
                mExifInfo.flash = EXIF_FLASH_ON;
                break;

            case FLASH_MODE_AUTO:
                if (mFlashFired){
                    mExifInfo.flash = EXIF_FLASH_ON_AUTO;
                } else {
                    mExifInfo.flash = EXIF_FLASH_OFF_AUTO;
                }
                break;

            default:
                LOGE("wrong flash mode");
                break;
            }
    }else {
        mExifInfo.flash = EXIF_FLASH_NONE;
    }
    // White Balance
    if (m_params->white_balance == WHITE_BALANCE_AUTO)
        mExifInfo.white_balance = EXIF_WB_AUTO;
    else
        mExifInfo.white_balance = EXIF_WB_MANUAL;
    // 0th IFD GPS Info Tags
    if (m_gps_enabled) {
        if (m_gps_latitude >= 0)
            strcpy((char *)mExifInfo.gps_latitude_ref, "N");
        else
            strcpy((char *)mExifInfo.gps_latitude_ref, "S");

        if (m_gps_longitude >= 0)
            strcpy((char *)mExifInfo.gps_longitude_ref, "E");
        else
            strcpy((char *)mExifInfo.gps_longitude_ref, "W");

        if (m_gps_altitude >= 0)
            mExifInfo.gps_altitude_ref = 0;
        else
            mExifInfo.gps_altitude_ref = 1;

        mExifInfo.gps_latitude[0].num = (uint32_t)labs(m_gps_latitude);
        mExifInfo.gps_latitude[0].den = 10000000;
        mExifInfo.gps_latitude[1].num = 0;
        mExifInfo.gps_latitude[1].den = 1;
        mExifInfo.gps_latitude[2].num = 0;
        mExifInfo.gps_latitude[2].den = 1;

        mExifInfo.gps_longitude[0].num = (uint32_t)labs(m_gps_longitude);
        mExifInfo.gps_longitude[0].den = 10000000;
        mExifInfo.gps_longitude[1].num = 0;
        mExifInfo.gps_longitude[1].den = 1;
        mExifInfo.gps_longitude[2].num = 0;
        mExifInfo.gps_longitude[2].den = 1;

        mExifInfo.gps_altitude.num = (uint32_t)labs(m_gps_altitude);
        mExifInfo.gps_altitude.den = 100;

        struct tm tm_data;
        gmtime_r(&m_gps_timestamp, &tm_data);
        mExifInfo.gps_timestamp[0].num = tm_data.tm_hour;
        mExifInfo.gps_timestamp[0].den = 1;
        mExifInfo.gps_timestamp[1].num = tm_data.tm_min;
        mExifInfo.gps_timestamp[1].den = 1;
        mExifInfo.gps_timestamp[2].num = tm_data.tm_sec;
        mExifInfo.gps_timestamp[2].den = 1;
        snprintf((char*)mExifInfo.gps_datestamp, sizeof(mExifInfo.gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        mExifInfo.enableGps = true;
    } else {
        mExifInfo.enableGps = false;
    }

    // 1th IFD TIFF Tags
    if ((m_jpeg_thumbnail_width > 0) && (m_jpeg_thumbnail_height > 0)) {
        LOGD("setting exif thumbnail info");
        mExifInfo.heightThumb = m_jpeg_thumbnail_height;
        mExifInfo.widthThumb = m_jpeg_thumbnail_width;
        mExifInfo.thumbBuf = m_thumbnail_outbuf.virtualAddress;
        mExifInfo.thumbSize = m_thumbnail_size;
        mExifInfo.enableThumb = true;
        LOGD("thumb buffer address: %p", mExifInfo.thumbBuf);
    } else {
        mExifInfo.enableThumb = false;
    }
}

void NuCameraV4L2::setExifFixedAttribute()
{
    char property[PROPERTY_VALUE_MAX];

    //  0th IFD TIFF Tags
    //Maker
    property_get("ro.product.brand", property, EXIF_DEF_MAKER);
    strncpy((char *)mExifInfo.maker, property,
                sizeof(mExifInfo.maker) - 1);
    mExifInfo.maker[sizeof(mExifInfo.maker) - 1] = '\0';
    //Model
    property_get("ro.product.model", property, EXIF_DEF_MODEL);
    strncpy((char *)mExifInfo.model, property,
                sizeof(mExifInfo.model) - 1);
    mExifInfo.model[sizeof(mExifInfo.model) - 1] = '\0';
    //Software
    property_get("ro.build.id", property, EXIF_DEF_SOFTWARE);
    strncpy((char *)mExifInfo.software, property,
                sizeof(mExifInfo.software) - 1);
    mExifInfo.software[sizeof(mExifInfo.software) - 1] = '\0';

    // YCbCr Positioning
    mExifInfo.ycbcr_positioning = EXIF_DEF_YCBCR_POSITIONING;

    // 0th IFD Exif Private Tags
    mExifInfo.fnumber.num = EXIF_DEF_FNUMBER_NUM;
    mExifInfo.fnumber.den = EXIF_DEF_FNUMBER_DEN;

    //Exposure Program
    mExifInfo.exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;
    // Exif Version
    memcpy(mExifInfo.exif_version, EXIF_DEF_EXIF_VERSION, sizeof(mExifInfo.exif_version));
    //Aperture
    uint32_t av = APEX_FNUM_TO_APERTURE((double)mExifInfo.fnumber.num/mExifInfo.fnumber.den);
    mExifInfo.aperture.num = av*EXIF_DEF_APEX_DEN;
    mExifInfo.aperture.den = EXIF_DEF_APEX_DEN;
    //Maximum lens aperture
    mExifInfo.max_aperture.num = mExifInfo.aperture.num;
    mExifInfo.max_aperture.den = mExifInfo.aperture.den;
    //Lens Focal Length
    if (m_camera_id == CAMERA_ID_BACK)
        mExifInfo.focal_length.num = BACK_CAMERA_FOCAL_LENGTH;
    else
        mExifInfo.focal_length.num = FRONT_CAMERA_FOCAL_LENGTH;

    mExifInfo.focal_length.den = EXIF_DEF_FOCAL_LEN_DEN;
    //User Comments
    strcpy((char *)mExifInfo.user_comment, EXIF_DEF_USERCOMMENTS);
    // Color Space information
    mExifInfo.color_space = EXIF_DEF_COLOR_SPACE;
    //Exposure Mode
    mExifInfo.exposure_mode = EXIF_DEF_EXPOSURE_MODE;

    //0th IFD GPS Info Tags
    unsigned char gps_version[4] = { 0x02, 0x02, 0x00, 0x00 };
    memcpy(mExifInfo.gps_version_id, gps_version, sizeof(gps_version));
    // 1th IFD TIFF Tags
    mExifInfo.compression_scheme = EXIF_DEF_COMPRESSION;
    mExifInfo.x_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.x_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.y_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.y_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.resolution_unit = EXIF_DEF_RESOLUTION_UNIT;
}

int NuCameraV4L2::setGPSLatitude(const char *gps_latitude)
{
    LOGV("%s(gps_latitude(%s))", __func__, gps_latitude);
    if (gps_latitude == NULL)
        m_gps_enabled = false;
    else {
        m_gps_enabled = true;
        m_gps_latitude = lround(strtod(gps_latitude, NULL) * 10000000);
    }

    LOGV("%s(m_gps_latitude(%ld))", __func__, m_gps_latitude);
    return 0;
}

int NuCameraV4L2::setGPSLongitude(const char *gps_longitude)
{
    LOGV("%s(gps_longitude(%s))", __func__, gps_longitude);
    if (gps_longitude == NULL)
        m_gps_enabled = false;
    else {
        m_gps_enabled = true;
        m_gps_longitude = lround(strtod(gps_longitude, NULL) * 10000000);
    }

    LOGV("%s(m_gps_longitude(%ld))", __func__, m_gps_longitude);
    return 0;
}

int NuCameraV4L2::setGPSAltitude(const char *gps_altitude)
{
    LOGV("%s(gps_altitude(%s))", __func__, gps_altitude);
    if (gps_altitude == NULL)
        m_gps_altitude = 0;
    else {
        m_gps_altitude = lround(strtod(gps_altitude, NULL) * 100);
    }

    LOGV("%s(m_gps_altitude(%ld))", __func__, m_gps_altitude);
    return 0;
}

int NuCameraV4L2::setGPSTimeStamp(const char *gps_timestamp)
{
    LOGV("%s(gps_timestamp(%s))", __func__, gps_timestamp);
    if (gps_timestamp == NULL)
        m_gps_timestamp = 0;
    else
        m_gps_timestamp = atol(gps_timestamp);

    LOGV("%s(m_gps_timestamp(%ld))", __func__, m_gps_timestamp);
    return 0;
}

int NuCameraV4L2::setGPSProcessingMethod(const char *gps_processing_method)
{
    LOGV("%s(gps_processing_method(%s))", __func__, gps_processing_method);
    memset(mExifInfo.gps_processing_method, 0, sizeof(mExifInfo.gps_processing_method));
    if (gps_processing_method != NULL) {
        size_t len = strlen(gps_processing_method);
        if (len > sizeof(mExifInfo.gps_processing_method)) {
            len = sizeof(mExifInfo.gps_processing_method);
        }
        memcpy(mExifInfo.gps_processing_method, gps_processing_method, len);
    }
    return 0;
}

//======================================================================

void NuCameraV4L2::getPostViewConfig(int *width, int *height, int *size)
{
    if (m_preview_width == 1024) {
        *width = BACK_CAMERA_POSTVIEW_WIDE_WIDTH;
        *height = BACK_CAMERA_POSTVIEW_HEIGHT;
        *size = BACK_CAMERA_POSTVIEW_WIDE_WIDTH * BACK_CAMERA_POSTVIEW_HEIGHT * BACK_CAMERA_POSTVIEW_BPP / 8;
    } else {
        *width = BACK_CAMERA_POSTVIEW_WIDTH;
        *height = BACK_CAMERA_POSTVIEW_HEIGHT;
        *size = BACK_CAMERA_POSTVIEW_WIDTH * BACK_CAMERA_POSTVIEW_HEIGHT * BACK_CAMERA_POSTVIEW_BPP / 8;
    }
    LOGV("[5B] m_preview_width : %d, mPostViewWidth = %d mPostViewHeight = %d mPostViewSize = %d",
            m_preview_width, *width, *height, *size);
}

void NuCameraV4L2::getThumbnailConfig(int *width, int *height, int *size)
{
    if (m_camera_id == CAMERA_ID_BACK) {
        *width  = BACK_CAMERA_THUMBNAIL_WIDTH;
        *height = BACK_CAMERA_THUMBNAIL_HEIGHT;
        *size   = BACK_CAMERA_THUMBNAIL_WIDTH * BACK_CAMERA_THUMBNAIL_HEIGHT
                    * BACK_CAMERA_THUMBNAIL_BPP / 8;
    } else {
        *width  = FRONT_CAMERA_THUMBNAIL_WIDTH;
        *height = FRONT_CAMERA_THUMBNAIL_HEIGHT;
        *size   = FRONT_CAMERA_THUMBNAIL_WIDTH * FRONT_CAMERA_THUMBNAIL_HEIGHT
                    * FRONT_CAMERA_THUMBNAIL_BPP / 8;
    }
}

int NuCameraV4L2::setSnapshotSize(int width, int height)
{
    LOGV("%s(width(%d), height(%d))", __func__, width, height);

    m_snapshot_width  = width;
    m_snapshot_height = height;

    return 0;
}

int NuCameraV4L2::getSnapshotSize(int *width, int *height, int *frame_size)
{
    *width  = m_snapshot_width;
    *height = m_snapshot_height;

    int frame = 0;

    frame = m_frameSize(m_snapshot_v4lformat, m_snapshot_width, m_snapshot_height);

    // set it big.
    if (frame == 0)
        frame = m_snapshot_width * m_snapshot_height * BPP;

    *frame_size = frame;

    return 0;
}

int NuCameraV4L2::getSnapshotMaxSize(int *width, int *height)
{
    switch (m_camera_id) {
    case CAMERA_ID_FRONT:
        m_snapshot_max_width  = MAX_FRONT_CAMERA_SNAPSHOT_WIDTH;
        m_snapshot_max_height = MAX_FRONT_CAMERA_SNAPSHOT_HEIGHT;
        break;

    default:
    case CAMERA_ID_BACK:
        m_snapshot_max_width  = MAX_BACK_CAMERA_SNAPSHOT_WIDTH;
        m_snapshot_max_height = MAX_BACK_CAMERA_SNAPSHOT_HEIGHT;
        break;
    }

    *width  = m_snapshot_max_width;
    *height = m_snapshot_max_height;

    return 0;
}

int NuCameraV4L2::setSnapshotPixelFormat(int pixel_format)
{
    int v4lpixelformat= pixel_format;

    if (m_snapshot_v4lformat != v4lpixelformat) {
        m_snapshot_v4lformat = v4lpixelformat;
    }

#if defined(LOG_NDEBUG) && LOG_NDEBUG == 0
    if (m_snapshot_v4lformat == V4L2_PIX_FMT_YUV420)
        LOGE("%s : SnapshotFormat:V4L2_PIX_FMT_YUV420", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_NV12)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_NV12", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_NV12T)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_NV12T", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_NV21)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_NV21", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_YUV422P)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_YUV422P", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_YUYV)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_YUYV", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_UYVY)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_UYVY", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_RGB565)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_RGB565", __func__);
    else
        LOGD("SnapshotFormat:UnknownFormat");
#endif
    return 0;
}

int NuCameraV4L2::getSnapshotPixelFormat(void)
{
    return m_snapshot_v4lformat;
}

// ======================================================================
// Settings

int NuCameraV4L2::getCameraId(void)
{
    return m_camera_id;
}

// -----------------------------------

int NuCameraV4L2::setAutofocus(void)
{
    LOGV("%s :", __func__);
    int cmd = 0;
    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }
    if(m_camera_id == CAMERA_ID_FRONT){
        return 0;
    }

    if (FOCUS_MODE_CONT_VIDEO == m_params->focus_mode){
        AFC_SET_CMD(cmd, AFC_CMD_CONT);
    } else {
        AFC_SET_CMD(cmd, AFC_CMD_SING);
    }

    //zhuyx--add--[
    //TODO:Better add focus region to driver, now we set (50,50)
    //NOTE:region is from APK By user point
    AFC_SET_X(cmd, 50);
    AFC_SET_Y(cmd, 50);
    LOGI("INFO(%s):Set IOCTL(V4L2_CID_CAMERA_AUTO_FOCUS, AFC_CMD_SING) ", __func__);
    if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_AUTO_FOCUS, cmd) < 0) {
        LOGE("ERR(%s):IOCTL(V4L2_CID_CAMERA_AUTO_FOCUS, AFC_CMD_SING) Failed!", __func__);
        return -1;
    }
    //zhuyx--add--]

    return 0;
}

int NuCameraV4L2::getAutoFocusResult(void)
{
    LOGV("%s :", __func__);
    int af_result, count, ret;
    int cmd = 0;
    af_result = AF_FAILED;
    for (count = 0; count < AF_SEARCH_COUNT; count++) {
        ret = nusmart_v4l2_g_ctrl(m_cam_fd, V4L2_CID_CAMERA_AUTO_FOCUS);
        if (ret == AF_SUCCESS){
            af_result = AF_SUCCESS;
            LOGV("%s : AF was successful, Counts:%d, Elapse Time:%d ms", __func__, count, count*AF_DELAY/1000);
            AFC_SET_CMD(cmd, AFC_CMD_PAUS);
            AFC_SET_X(cmd, 50);
            AFC_SET_Y(cmd, 50);
            LOGI("INFO(%s):Set IOCTL(V4L2_CID_CAMERA_AUTO_FOCUS, AFC_CMD_PAUS) ", __func__);
            if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_AUTO_FOCUS, cmd) < 0) {
                LOGE("ERR(%s):IOCTL(V4L2_CID_CAMERA_AUTO_FOCUS, AFC_CMD_PAUS) Failed!", __func__);
                return -1;
            }
            return af_result;
        }else if(ret == AF_FAILED){
            LOGV("%s : AF was failed, Counts:%d, Elapse Time:%d ms", __func__, count, count*AF_DELAY/1000);
            af_result = AF_FAILED;
            return af_result;
        }
        usleep(AF_DELAY);
    }
    //normally, we get result from sensor less than 800ms.
    if ((count >= AF_SEARCH_COUNT) || (ret != AF_SUCCESS)) {
        LOGV("%s : AF Timed Out(%d ms), failed, or was canceled.", __func__, AF_SEARCH_COUNT*AF_DELAY/1000);
        af_result = AF_FAILED;
        goto finish_auto_focus;
    }
finish_auto_focus:
    //TODO:if AF failed ,we must set flash mode off.
    return af_result;
}

int NuCameraV4L2::cancelAutofocus(void)
{
    LOGV("%s :", __func__);
    int cmd = 0;
    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }
    //zhuyx--add--[
    AFC_SET_CMD(cmd, AFC_CMD_RELE);
    LOGI("INFO(%s):Set IOCTL(V4L2_CID_CAMERA_AUTO_FOCUS, AFC_CMD_RELE) ", __func__);
    if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_AUTO_FOCUS, cmd) < 0) {
        LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_AUTO_FOCUS", __func__);
        return -1;
    }
    //zhuyx--add--]

    return 0;
}

// -----------------------------------

int NuCameraV4L2::zoomIn(void)
{
    LOGV("%s :", __func__);
    return 0;
}

int NuCameraV4L2::zoomOut(void)
{
    LOGV("%s :", __func__);
    return 0;
}

// -----------------------------------

int NuCameraV4L2::setRotate(int angle)
{
    LOGV("%s(angle(%d))", __func__, angle);

    if (m_angle != angle) {
        switch (angle) {
        case -360:
        case    0:
        case  360:
            m_angle = 0;
            break;

        case -270:
        case   90:
            m_angle = 90;
            break;

        case -180:
        case  180:
            m_angle = 180;
            break;

        case  -90:
        case  270:
            m_angle = 270;
            break;

        default:
            LOGE("ERR(%s):Invalid angle(%d)", __func__, angle);
            return -1;
        }

    }

    return 0;
}

int NuCameraV4L2::getRotate(void)
{
    LOGV("%s : angle(%d)", __func__, m_angle);
    return m_angle;
}

int NuCameraV4L2::setFrameRate(int frame_rate)
{
    LOGV("%s(FrameRate(%d))", __func__, frame_rate);

    if (frame_rate < FRAME_RATE_AUTO || FRAME_RATE_MAX < frame_rate )
        LOGE("ERR(%s):Invalid frame_rate(%d)", __func__, frame_rate);

    if (m_params->capture.timeperframe.denominator != (unsigned)frame_rate) {
        m_params->capture.timeperframe.denominator = frame_rate;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FRAME_RATE, frame_rate) < 0) {
            LOGW("WARN(%s):Fail on V4L2_CID_CAMERA_FRAME_RATE", __func__);
            return -1;
        }
    }

    return 0;
}

// -----------------------------------

int NuCameraV4L2::setVerticalMirror(void)
{
    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }
    LOGV("%s :", __func__);
    if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_VFLIP, 0) < 0) {
        LOGE("ERR(%s):Fail on V4L2_CID_VFLIP", __func__);
        return -1;
    }

    return 0;
}

int NuCameraV4L2::setHorizontalMirror(void)
{
    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }
    LOGV("%s :", __func__);
    if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_HFLIP, 0) < 0) {
        LOGE("ERR(%s):Fail on V4L2_CID_HFLIP", __func__);
        return -1;
    }

    return 0;
}

// -----------------------------------

int NuCameraV4L2::setWhiteBalance(int white_balance)
{
    LOGV("%s(white_balance(%d))", __func__, white_balance);

    if (white_balance < 0 || WHITE_BALANCE_MAX <= white_balance) {
        LOGE("ERR(%s):Invalid white_balance(%d)", __func__, white_balance);
        return -1;
    }

    if (m_params->white_balance != white_balance) {
        m_params->white_balance = white_balance;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_WHITE_BALANCE, white_balance) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_WHITE_BALANCE", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getWhiteBalance(void)
{
    LOGV("%s : white_balance(%d)", __func__, m_params->white_balance);
    return m_params->white_balance;
}

// -----------------------------------

int NuCameraV4L2::setBrightness(int brightness)
{
    LOGD("%s(brightness(%d))", __func__, brightness);

    int tmp = brightness + EV_DEFAULT;

    if (tmp < EV_MINUS_4 || EV_PLUS_4 < tmp) {
        LOGE("ERR(%s):Invalid brightness(%d)", __func__, brightness);
        return -1;
    }

    if (m_params->brightness != brightness) {
        m_params->brightness = brightness;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_BRIGHTNESS, brightness) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_BRIGHTNESS", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getBrightness(void)
{
    LOGV("%s : brightness(%d)", __func__, m_params->brightness);
    return m_params->brightness;
}

#if NUSMART_CAMERA_ISP_EFFECT
int NuCameraV4L2::setImageEffect(int image_effect){
    LOGD("%s(image_effect(%d))", __func__, image_effect);

    if (image_effect <IMAGE_EFFECT_NONE || IMAGE_EFFECT_MAX <= image_effect) {
        LOGE("ERR(%s):Invalid image_effect(%d)", __func__, image_effect);
        return -1;
    }

    if (m_params->effects != image_effect) {
        m_params->effects = image_effect;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SPECIAL_EFFECT, image_effect) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SPECIAL_EFFECT", __func__);
            return -1;
        }
    }

    return 0;
}

#else
int NuCameraV4L2::setImageEffect(int image_effect)
{
    LOGD("%s(image_effect(%d))", __func__, image_effect);

    if (image_effect <Effect::CAMERA_EFFECT_NONE || Effect::CAMERA_EFFECT_AQUA < image_effect) {
        LOGE("ERR(%s):Invalid image_effect(%d)", __func__, image_effect);
        return -1;
    }

    if (m_params->effects != image_effect) {
        m_params->effects = image_effect;
    }

    return 0;
}
#endif

int NuCameraV4L2::getImageEffect(void)
{
    LOGV("%s : image_effect(%d)", __func__, m_params->effects);
    return m_params->effects;
}

// ======================================================================
int NuCameraV4L2::setAntiBanding(int anti_banding)
{
    LOGV("%s(anti_banding(%d))", __func__, anti_banding);

    if (anti_banding < ANTI_BANDING_AUTO || ANTI_BANDING_OFF < anti_banding) {
        LOGE("ERR(%s):Invalid anti_banding (%d)", __func__, anti_banding);
        return -1;
    }

    if (m_anti_banding != anti_banding) {
        m_anti_banding = anti_banding;
        if (m_flag_camera_start) {
            if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_ANTI_BANDING, anti_banding) < 0) {
                 LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_ANTI_BANDING", __func__);
                 return -1;
            }
        }
    }

    return 0;
}

//======================================================================
int NuCameraV4L2::setSceneMode(int scene_mode)
{
    LOGV("%s(scene_mode(%d))", __func__, scene_mode);

    if (scene_mode <= SCENE_MODE_BASE || SCENE_MODE_MAX <= scene_mode) {
        LOGE("ERR(%s):Invalid scene_mode (%d)", __func__, scene_mode);
        return -1;
    }

    if (m_params->scene_mode != scene_mode) {
        m_params->scene_mode = scene_mode;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SCENE_MODE, scene_mode) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SCENE_MODE", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getSceneMode(void)
{
    return m_params->scene_mode;
}

//======================================================================
extern "C" {
static int write_file(const char *path, const char *value)
{
    int fd, ret, len;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        LOGE("write_file: can't open file:%s\n", path);
        return -errno;
    }

    len = strlen(value);
    do {
         ret = write(fd, value, len);
    } while (ret < 0 && errno == EINTR);

    close(fd);
    if (ret < 0) {
          LOGE("write file failed:%s,%s\n",path, value);
          return -errno;
    }

    return 0;
}

}// end extern "C"

int NuCameraV4L2::setFlashMode(int flash_mode)
{
    LOGV("%s(flash_mode(%d))", __func__, flash_mode);

    if (flash_mode < FLASH_MODE_OFF || FLASH_MODE_MAX <= flash_mode) {
        LOGE("ERR(%s):Invalid flash_mode (%d)", __func__, flash_mode);
        return -1;
    }
    if (m_params->flash_mode != flash_mode) {
        m_params->flash_mode = flash_mode;
    }

    return 0;
}

// get exposure control value
void NuCameraV4L2::setAECValue()
{
    m_current_aec_value = nusmart_v4l2_g_ctrl(m_cam_fd, V4L2_CID_CAMERA_GET_AEC);
    LOGV("%s, set current AEC value:0x%x", __func__, m_current_aec_value);
}

/*
 * return = 0:success to set gpio vaule
 * return < 0:failed to set gpio vaule
 */
int NuCameraV4L2::flashing(int is_on)
{
    int ret = 0;
    if (is_on){
         LOGD("flash_mode=%d", m_params->flash_mode);
         switch (m_params->flash_mode)
         {
             case FLASH_MODE_AUTO:
                 mFlashFired = false;
                 if(m_current_aec_value > AUTO_FLASH_THRESHOLD) {
                     ret = property_set(PROP_KEY_FLASH, "1");
                     mFlashFired = true;
                 }
                 break;
             case FLASH_MODE_ON:
                 ret = property_set(PROP_KEY_FLASH, "1");
                 break;
         }
    } else {
        ret = property_set(PROP_KEY_FLASH, "0");
    }
    
    return ret;
}

int NuCameraV4L2::getFlashMode(void)
{
    return m_params->flash_mode;
}

//======================================================================

int NuCameraV4L2::setISO(int iso_value)
{
    LOGV("%s(iso_value(%d))", __func__, iso_value);
    if (iso_value < ISO_AUTO || ISO_MAX <= iso_value) {
        LOGE("ERR(%s):Invalid iso_value (%d)", __func__, iso_value);
        return -1;
    }

    if (m_params->iso != iso_value) {
        m_params->iso = iso_value;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_ISO, iso_value) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_ISO", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getISO(void)
{
    return m_params->iso;
}

//======================================================================

int NuCameraV4L2::setContrast(int contrast_value)
{
    LOGV("%s(contrast_value(%d))", __func__, contrast_value);
    int contrast =  contrast_value + CONTRAST_DEFAULT;
    if (contrast < CONTRAST_MINUS_3 || CONTRAST_MAX <= contrast) {
        LOGE("ERR(%s):Invalid contrast_value (%d)", __func__, contrast_value);
        return -1;
    }

    if (m_params->contrast != contrast_value) {
        m_params->contrast = contrast_value;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_CONTRAST, contrast_value) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_CONTRAST", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getContrast(void)
{
    return m_params->contrast;
}

//======================================================================

int NuCameraV4L2::setSaturation(int saturation_value)
{
    LOGV("%s(saturation_value(%d))", __func__, saturation_value);
    int saturation = saturation_value + SATURATION_DEFAULT;
    if (saturation <SATURATION_MINUS_3 || SATURATION_MAX<= saturation) {
        LOGE("ERR(%s):Invalid saturation_value (%d)", __func__, saturation_value);
        return -1;
    }

    if (m_params->saturation != saturation_value) {
        m_params->saturation = saturation_value;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SATURATION, saturation_value) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SATURATION", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getSaturation(void)
{
    return m_params->saturation;
}

//======================================================================

int NuCameraV4L2::setSharpness(int sharpness_value)
{
    LOGV("%s(sharpness_value(%d))", __func__, sharpness_value);

    if (sharpness_value < SHARPNESS_MINUS_2 || SHARPNESS_MAX <= sharpness_value) {
        LOGE("ERR(%s):Invalid sharpness_value (%d)", __func__, sharpness_value);
        return -1;
    }

    if (m_params->sharpness != sharpness_value) {
        m_params->sharpness = sharpness_value;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SHARPNESS, sharpness_value) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SHARPNESS", __func__);
                return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getSharpness(void)
{
    return m_params->sharpness;
}

//======================================================================

int NuCameraV4L2::setWDR(int wdr_value)
{
    LOGV("%s(wdr_value(%d))", __func__, wdr_value);

    if (wdr_value < WDR_OFF || WDR_MAX <= wdr_value) {
        LOGE("ERR(%s):Invalid wdr_value (%d)", __func__, wdr_value);
        return -1;
    }

    if (m_wdr != wdr_value) {
        m_wdr = wdr_value;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_WDR, wdr_value) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_WDR", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getWDR(void)
{
    return m_wdr;
}

//======================================================================

int NuCameraV4L2::setAntiShake(int anti_shake)
{
    LOGV("%s(anti_shake(%d))", __func__, anti_shake);

    if (anti_shake < ANTI_SHAKE_OFF || ANTI_SHAKE_MAX <= anti_shake) {
        LOGE("ERR(%s):Invalid anti_shake (%d)", __func__, anti_shake);
        return -1;
    }

    if (m_anti_shake != anti_shake) {
        m_anti_shake = anti_shake;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_ANTI_SHAKE, anti_shake) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_ANTI_SHAKE", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getAntiShake(void)
{
    return m_anti_shake;
}

//======================================================================


int NuCameraV4L2::setMetering(int metering_value)
{
    LOGV("%s(metering (%d))", __func__, metering_value);

    if (metering_value <= METERING_BASE || METERING_MAX <= metering_value) {
        LOGE("ERR(%s):Invalid metering_value (%d)", __func__, metering_value);
        return -1;
    }

    if (m_params->metering != metering_value) {
        m_params->metering = metering_value;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_METERING, metering_value) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_METERING", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getMetering(void)
{
    return m_params->metering;
}

//======================================================================

int NuCameraV4L2::setJpegQuality(int jpeg_quality)
{
    LOGV("%s(jpeg_quality (%d))", __func__, jpeg_quality);

    if (jpeg_quality < JPEG_QUALITY_ECONOMY || JPEG_QUALITY_MAX <= jpeg_quality) {
        LOGE("ERR(%s):Invalid jpeg_quality (%d)", __func__, jpeg_quality);
        return -1;
    }

    if (m_jpeg_quality != jpeg_quality) {
        m_jpeg_quality = jpeg_quality;
    }

    return 0;
}

int NuCameraV4L2::getJpegQuality(void)
{
    return m_jpeg_quality;
}

//======================================================================

int NuCameraV4L2::setZoom(int base_w, int base_h, int zoom_level)
{
    LOGV("%s, base resolution:%dx%d , zoom_level:%d", __func__, base_w, base_h, zoom_level);
    int left, top, crop_w, crop_h;
    int ret;
    int zoom_times, base_ratio, target_ratio, ratio_precision;
    ratio_precision = 100;
    if (zoom_level == ZOOM_LEVEL_0){
        LOGI("INFO(%s): ZOOM level is 0, return directly.", __func__);
        m_zoom_level = ZOOM_LEVEL_0;
        return 0;
    }
    if (zoom_level < ZOOM_LEVEL_0 || ZOOM_LEVEL_MAX <= zoom_level) {
        LOGE("ERR(%s):Invalid zoom_level (%d)", __func__, zoom_level);
        return -1;
    }
    if ((m_sensor_support_resolution == CAMERA_SENSOR_NOT_SUPPORT_RESOLUTION) &&
        (m_closely_width != 0) && (m_closely_height != 0)){
        //getCloselyResolution(&base_w, &base_h);
        base_w = m_closely_width;
        base_h = m_closely_height;
        LOGI("INFO(%s), Sensor was not support base resolution, get closely resolution: %dx%d for ZOOM",
                        __func__, base_w, base_h);
    }
    //the zoom_level here must begin from 1 to max
    m_zoom_level = zoom_level;
    //resolution ratio is same
    zoom_times = ZOOM_INC_CHANGE_PRECISION + (zoom_level * ZOOM_INC_CHANGE_PRECISION)/ZOOM_INC_CHANGE_DENOMINATOR;
    crop_w = (base_w * ZOOM_INC_CHANGE_PRECISION)/zoom_times;
    crop_h = (base_h * ZOOM_INC_CHANGE_PRECISION)/zoom_times;
    left = (base_w - crop_w) >> 1;
    top = (base_h - crop_h) >> 1;
    LOGI("INFO(%s), Set Crop: left-top(%d,%d), crop resolution(%dx%d)", __func__, left, top, crop_w, crop_h);
    ret = nusmart_v4l2_set_crop(m_cam_fd, left, top, crop_w, crop_h);
    if (ret < 0) {
        LOGE("ERR(%s):call nusmart_v4l2_set_crop failed.", __func__);
                return -1;
            }
    return 0;
}

int NuCameraV4L2::getZoom(void)
{
    return m_zoom_level;
}


///////////////////////// EWL
void   NuCameraV4L2::getPreviewFrameAddr(int index, EWLLinearMem_t *paddr) const
{
       paddr->busAddress     = m_ewl_mem_info[index].busAddress;
       paddr->virtualAddress = m_ewl_mem_info[index].virtualAddress;
}

void   NuCameraV4L2::getCaptureFrameAddr(int index, EWLLinearMem_t *paddr) const
{
       paddr->busAddress     = m_ewl_capture_mem_info[index].busAddress;
       paddr->virtualAddress = m_ewl_capture_mem_info[index].virtualAddress;
}

inline void NuCameraV4L2::jpegInitEncConfigs(){
     m_jpeg_enc_cfg.rotation = JPEGENC_ROTATE_0;

     m_jpeg_enc_cfg.inputWidth    = (m_snapshot_width + 15) & (~15);
     if (m_jpeg_enc_cfg.inputWidth != (unsigned)m_snapshot_width){
        LOGW("%s: Input width must be multiple of 16!", __func__);
     }
     m_jpeg_enc_cfg.inputHeight   = m_snapshot_height;
     m_jpeg_enc_cfg.codingWidth  = m_snapshot_width;
     m_jpeg_enc_cfg.codingHeight = m_snapshot_height;

    //top-left corner of input image
     m_jpeg_enc_cfg.xOffset = 0;
     m_jpeg_enc_cfg.yOffset = 0;

    //Xdensity to APP0 header
     m_jpeg_enc_cfg.xDensity = 1;
    //Ydensity to APP0 header
     m_jpeg_enc_cfg.yDensity = 1;

    //Restart interval in MCU rows
    m_jpeg_enc_cfg.restartInterval = 0;
    //quantization scale: 0..10
    //Pic Resolution:2592x1944,2048x1536,1600x1200,1280x960,1024x768,640x480,320x240
     m_jpeg_enc_cfg.qLevel = 9;
    /*
      JPEGENC_WHOLE_FRAME:The whole frame is stored in linear memory
      JPEGENC_SLICED_FRAME: The frame is sliced into restart intervals
    */
     m_jpeg_enc_cfg.codingType = JPEGENC_WHOLE_FRAME;
     //Quantization/Huffman table markers: 0 = single marker, 1 = multi marker
     m_jpeg_enc_cfg.markerType = JPEGENC_SINGLE_MARKER;
     //Units type of x- and y-density: 0 = pixel aspect ratio, 1 = dots/inch, 2 = dots/cm
     m_jpeg_enc_cfg.unitsType = JPEGENC_NO_UNITS;
    m_jpeg_enc_cfg.frameType = JPEGENC_YUV420_SEMIPLANAR;

     m_jpeg_enc_cfg.colorConversion.type = JPEGENC_RGBTOYUV_BT601;
     m_jpeg_enc_cfg.codingMode = JPEGENC_420_MODE;


}

inline int NuCameraV4L2::jpegInitInputOutput(int index)
{
    memset(&m_jpeg_outbuf, 0 , sizeof(m_jpeg_outbuf));
    memset(&m_encIn, 0 , sizeof(m_encIn));
    memset(&m_encOut, 0 , sizeof(m_encOut));
    int buf_size = m_jpeg_enc_cfg.codingWidth * m_jpeg_enc_cfg.codingHeight;
    int ret = EWLMallocLinear(m_jpeg_ewl_instance, buf_size, &m_jpeg_outbuf);
    if (ret < 0){
       LOGE(" %s: Failed to alloc memory ", __func__);
       return -1;
    }
    m_encIn.pOutBuf = (u8*)m_jpeg_outbuf.virtualAddress;
    m_encIn.busOutBuf = m_jpeg_outbuf.busAddress;
    m_encIn.outBufSize = m_jpeg_outbuf.size;
    //Enable/disable creation of frame headers
    m_encIn.frameHeader = 1;

    unsigned int sliceRows = 0;
    unsigned int widthSrc, heightSrc;

    //input config
    if(m_jpeg_enc_cfg.codingType == JPEGENC_WHOLE_FRAME){
        sliceRows = m_jpeg_enc_cfg.inputHeight;
    }else{
        sliceRows = m_jpeg_enc_cfg.restartInterval * 16;
    }

    widthSrc  = m_jpeg_enc_cfg.inputWidth;
    heightSrc = m_jpeg_enc_cfg.inputHeight;
    if(m_jpeg_enc_cfg.frameType <= JPEGENC_YUV420_SEMIPLANAR)
    {
        // Bus addresses of input picture, used by hardware encoder
        m_encIn.busLum = m_ewl_capture_mem_info[index].busAddress;
        m_encIn.busCb = m_encIn.busLum + (widthSrc * sliceRows);
        m_encIn.busCr = m_encIn.busCb +
            (((widthSrc + 1) / 2) * ((sliceRows + 1) / 2));

        // Virtual addresses of input picture, used by software encoder
        m_encIn.pLum = (unsigned char *)(m_ewl_capture_mem_info[index].virtualAddress);
        m_encIn.pCb = m_encIn.pLum + (widthSrc * sliceRows);
        m_encIn.pCr = m_encIn.pCb +
            (((widthSrc + 1) / 2) * ((sliceRows + 1) / 2));
    }
    else
    {
        // Bus addresses of input picture, used by hardware encoder
        m_encIn.busLum = m_ewl_capture_mem_info[index].busAddress;
        m_encIn.busCb = m_encIn.busLum;
        m_encIn.busCr = m_encIn.busCb;

        // Virtual addresses of input picture, used by software encoder
        m_encIn.pLum = (unsigned char *)(m_ewl_capture_mem_info[index].virtualAddress);
        m_encIn.pCb = m_encIn.pLum;
        m_encIn.pCr = m_encIn.pCb;
    }

    //output config
    m_encOut.jfifSize = 0;
    return 0;
}

 int NuCameraV4L2::thumbnailInitInOut(int index)
{
    memset(&m_thumbnail_outbuf, 0 , sizeof(m_thumbnail_outbuf));
    memset(&m_encIn, 0 , sizeof(m_encIn));
    memset(&m_encOut, 0 , sizeof(m_encOut));
    int buf_size = m_jpeg_enc_cfg.codingWidth * m_jpeg_enc_cfg.codingHeight<<1;
    int ret = EWLMallocLinear(m_jpeg_ewl_instance, buf_size, &m_thumbnail_outbuf);
    if (ret < 0){
       LOGE(" %s: Failed to alloc memory ", __func__);
       return -1;
    }
    m_encIn.pOutBuf = (u8*)m_thumbnail_outbuf.virtualAddress;
    m_encIn.busOutBuf = m_thumbnail_outbuf.busAddress;
    m_encIn.outBufSize = m_thumbnail_outbuf.size;
    //Enable/disable creation of frame headers
    m_encIn.frameHeader = 1;

    unsigned int sliceRows = 0;
    unsigned int widthSrc, heightSrc;

    //input config
    if(m_jpeg_enc_cfg.codingType == JPEGENC_WHOLE_FRAME){
        sliceRows = m_jpeg_enc_cfg.inputHeight;
    }else{
        sliceRows = m_jpeg_enc_cfg.restartInterval * 16;
    }

    widthSrc  = m_jpeg_enc_cfg.inputWidth;
    heightSrc = m_jpeg_enc_cfg.inputHeight;
    if(m_jpeg_enc_cfg.frameType <= JPEGENC_YUV420_SEMIPLANAR)
    {
        // Bus addresses of input picture, used by hardware encoder
        m_encIn.busLum = m_ewl_capture_mem_info[index].busAddress;
        m_encIn.busCb = m_encIn.busLum + (widthSrc * sliceRows);
        m_encIn.busCr = m_encIn.busCb +
            (((widthSrc + 1) / 2) * ((sliceRows + 1) / 2));

        // Virtual addresses of input picture, used by software encoder
        m_encIn.pLum = (unsigned char *)(m_ewl_capture_mem_info[index].virtualAddress);
        m_encIn.pCb = m_encIn.pLum + (widthSrc * sliceRows);
        m_encIn.pCr = m_encIn.pCb +
            (((widthSrc + 1) / 2) * ((sliceRows + 1) / 2));
    }
    else
    {
        // Bus addresses of input picture, used by hardware encoder
        m_encIn.busLum = m_ewl_capture_mem_info[index].busAddress;
        m_encIn.busCb = m_encIn.busLum;
        m_encIn.busCr = m_encIn.busCb;

        // Virtual addresses of input picture, used by software encoder
        m_encIn.pLum = (unsigned char *)(m_ewl_capture_mem_info[index].virtualAddress);
        m_encIn.pCb = m_encIn.pLum;
        m_encIn.pCr = m_encIn.pCb;
    }

    //output config
    m_encOut.jfifSize = 0;
    return 0;
}
//CODES: jpeg encoding in VPU
int  NuCameraV4L2::jpegInit()
{
    int ret = 0;
    if (NULL == m_jpeg_encoder){
        //init fixed configs
        jpegInitEncConfigs();

        // init jpeg encoder
        if((ret = JpegEncInit(&m_jpeg_enc_cfg, &m_jpeg_encoder)) != JPEGENC_OK)
        {
            LOGE("%s: Failed to call JpegEncInit . Error code: %8i\n", __func__, ret);
            return -1;
        }

        // succeed, can get ewl instance of Jpeg Encoder
        JpegEncEWL(m_jpeg_encoder, &m_jpeg_ewl_instance);
    }

    if (NULL == m_jpeg_compressor){
        m_jpeg_compressor = new YUV420spJpegCompressor((int)JPG_INPUT_FMT_NV12);
    }
    if (NULL == m_jpeg_compressor){
       LOGE("Failed to create jpeg encoder");
       return -1;
    }

    return 0;
}

#if NUSMART_CAMERA_RUN_VPU_HARDWARE
//CODES: jpeg encoding in VPU
int  NuCameraV4L2::jpegEncoding(int index, int* jpegSize)
{
      JpegEncRet ret;
      //set input & output parameters
      if (jpegInitInputOutput(index) < 0){
         return -1;
      }

      if((ret = JpegEncSetPictureSize(m_jpeg_encoder, &m_jpeg_enc_cfg)) != JPEGENC_OK)
      {
           LOGE("%s: Failed to call JpegEncSetPictureSize . Error code: %8i\n", __func__, ret);
           return -1;
      }
       //encoding
      ret = JpegEncEncode(m_jpeg_encoder, &m_encIn, &m_encOut);
      if (ret != JPEGENC_FRAME_READY)
      {
            LOGE("%s: Failed to call JpegEncEncode . Error code: %8i\n", __func__, ret);
            return -1;
      }
      m_jpeg_image_size = *jpegSize = m_encOut.jfifSize;

      //thumbnail encoding
      return thumbnailEncoding(index);
}

int NuCameraV4L2::thumbnailEncoding(int index)
{
    if (m_jpeg_thumbnail_width == 0 || m_jpeg_thumbnail_height == 0){
        return NO_ERROR;
    }
    JpegEncRet ret;
    m_jpeg_enc_cfg.inputWidth = m_jpeg_thumbnail_width;
    m_jpeg_enc_cfg.inputHeight = m_jpeg_thumbnail_height;
    m_jpeg_enc_cfg.codingWidth = m_jpeg_thumbnail_width;
    m_jpeg_enc_cfg.codingHeight = m_jpeg_thumbnail_height;

    //set input & output parameters
    if (thumbnailInitInOut(index) < 0){
       return -1;
    }

    if((ret = JpegEncSetPictureSize(m_jpeg_encoder, &m_jpeg_enc_cfg)) != JPEGENC_OK)
    {
         LOGE("%s: Failed to call JpegEncSetPictureSize . Error code: %8i\n", __func__, ret);
         return -1;
    }
    //scaling down
    unsigned char *pYuvIn = (unsigned char *)(m_ewl_capture_mem_info[index].virtualAddress);
    nusmart_scalingDown420SP(pYuvIn, m_snapshot_width, m_snapshot_height,
                             pYuvIn, m_jpeg_thumbnail_width, m_jpeg_thumbnail_height);
     
    //encoding
    ret = JpegEncEncode(m_jpeg_encoder, &m_encIn, &m_encOut);
    if (ret != JPEGENC_FRAME_READY)
    {
        LOGE("%s: Failed to call JpegEncEncode . Error code: %8i\n", __func__, ret);
        return -1;
    }
    m_thumbnail_size =  m_encOut.jfifSize;
    LOGD("thumbnail encoding ok");
    return NO_ERROR;
}

void NuCameraV4L2::jpegGetImage(void *pbuf)
{
      memcpy(pbuf, m_jpeg_outbuf.virtualAddress, m_jpeg_image_size);
}

#else
//CODES: jpeg encoding in CPU
int  NuCameraV4L2::jpegEncoding(int index, int* jpegSize)
{
      int ret = m_jpeg_compressor->compressRawImage(m_ewl_capture_mem_info[0].virtualAddress,
                                                              m_snapshot_width,
                                                              m_snapshot_height,
                                                              m_jpeg_quality,
                                                              m_params->effects);
      if (NO_ERROR == ret) {
           *jpegSize = m_jpeg_compressor->getCompressedSize();
      }

      return ret;
}

void NuCameraV4L2::jpegGetImage(void *pbuf)
{
      m_jpeg_compressor->getCompressedImage(pbuf);
}

#endif

//======================================================================

int NuCameraV4L2::setObjectTracking(int object_tracking)
{
    LOGV("%s(object_tracking (%d))", __func__, object_tracking);

    if (object_tracking < OBJECT_TRACKING_OFF || OBJECT_TRACKING_MAX <= object_tracking) {
        LOGE("ERR(%s):Invalid object_tracking (%d)", __func__, object_tracking);
        return -1;
    }

    if (m_object_tracking != object_tracking) {
        m_object_tracking = object_tracking;
    }

    return 0;
}

int NuCameraV4L2::getObjectTracking(void)
{
    return m_object_tracking;
}

int NuCameraV4L2::getObjectTrackingStatus(void)
{
    int obj_status = 0;
    obj_status = nusmart_v4l2_g_ctrl(m_cam_fd, V4L2_CID_CAMERA_OBJ_TRACKING_STATUS);
    return obj_status;
}

int NuCameraV4L2::setObjectTrackingStartStop(int start_stop)
{
    LOGV("%s(object_tracking_start_stop (%d))", __func__, start_stop);

    if (m_object_tracking_start_stop != start_stop) {
        m_object_tracking_start_stop = start_stop;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_OBJ_TRACKING_START_STOP, start_stop) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_OBJ_TRACKING_START_STOP", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::setTouchAFStartStop(int start_stop)
{
    LOGV("%s(touch_af_start_stop (%d))", __func__, start_stop);

    if (m_touch_af_start_stop != start_stop) {
        m_touch_af_start_stop = start_stop;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_TOUCH_AF_START_STOP, start_stop) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_TOUCH_AF_START_STOP", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::setTakepicureFlag(int flag)
{
    LOGV("%s", __func__);
    //Tell sensor that user will take picture.
    if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_PRE_CAP, flag) < 0) {
        LOGE("ERR(%s):Fail on setTakepicureFlag", __func__);
        return -1;
    }

    return 0;
}
//======================================================================

int NuCameraV4L2::setSmartAuto(int smart_auto)
{
    LOGV("%s(smart_auto (%d))", __func__, smart_auto);

    if (smart_auto < SMART_AUTO_OFF || SMART_AUTO_MAX <= smart_auto) {
        LOGE("ERR(%s):Invalid smart_auto (%d)", __func__, smart_auto);
        return -1;
    }

    if (m_smart_auto != smart_auto) {
        m_smart_auto = smart_auto;
        if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SMART_AUTO, smart_auto) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SMART_AUTO", __func__);
            return -1;
        }
    }

    return 0;
}

int NuCameraV4L2::getSmartAuto(void)
{
    return m_smart_auto;
}

int NuCameraV4L2::getAutosceneStatus(void)
{
    int autoscene_status = -1;

    if (getSmartAuto() == SMART_AUTO_ON) {
        autoscene_status = nusmart_v4l2_g_ctrl(m_cam_fd, V4L2_CID_CAMERA_SMART_AUTO_STATUS);

        if ((autoscene_status < SMART_AUTO_STATUS_AUTO) || (autoscene_status > SMART_AUTO_STATUS_MAX)) {
            LOGE("ERR(%s):Invalid getAutosceneStatus (%d)", __func__, autoscene_status);
            return -1;
        }
    }
    //LOGV("%s :    autoscene_status (%d)", __func__, autoscene_status);
    return autoscene_status;
}
//======================================================================

int NuCameraV4L2::setBeautyShot(int beauty_shot)
{
    LOGV("%s(beauty_shot (%d))", __func__, beauty_shot);

    if (beauty_shot < BEAUTY_SHOT_OFF || BEAUTY_SHOT_MAX <= beauty_shot) {
        LOGE("ERR(%s):Invalid beauty_shot (%d)", __func__, beauty_shot);
        return -1;
    }

    if (m_beauty_shot != beauty_shot) {
        m_beauty_shot = beauty_shot;
        if (m_flag_camera_start) {
            if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_BEAUTY_SHOT, beauty_shot) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_BEAUTY_SHOT", __func__);
                return -1;
            }
        }

        setFaceDetect(FACE_DETECTION_ON_BEAUTY);
    }

    return 0;
}

int NuCameraV4L2::getBeautyShot(void)
{
    return m_beauty_shot;
}

//======================================================================

int NuCameraV4L2::setVintageMode(int vintage_mode)
{
    LOGV("%s(vintage_mode(%d))", __func__, vintage_mode);

    if (vintage_mode <= VINTAGE_MODE_BASE || VINTAGE_MODE_MAX <= vintage_mode) {
        LOGE("ERR(%s):Invalid vintage_mode (%d)", __func__, vintage_mode);
        return -1;
    }

    if (m_vintage_mode != vintage_mode) {
        m_vintage_mode = vintage_mode;
        if (m_flag_camera_start) {
            if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_VINTAGE_MODE, vintage_mode) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_VINTAGE_MODE", __func__);
                return -1;
            }
        }
    }

    return 0;
}

int NuCameraV4L2::getVintageMode(void)
{
    return m_vintage_mode;
}

//======================================================================

int NuCameraV4L2::setFocusMode(int focus_mode)
{
    LOGV("%s(focus_mode(%d))", __func__, focus_mode);

    if (FOCUS_MODE_MAX <= focus_mode) {
        LOGE("ERR(%s):Invalid focus_mode (%d)", __func__, focus_mode);
        return -1;
    }

    if (m_params->focus_mode != focus_mode) {
        m_params->focus_mode = focus_mode;
    }

    return 0;
}

int NuCameraV4L2::getFocusMode(void)
{
    return m_params->focus_mode;
}

//======================================================================

int NuCameraV4L2::setFaceDetect(int face_detect)
{
    LOGV("%s(face_detect(%d))", __func__, face_detect);

    if (m_face_detect != face_detect) {
        m_face_detect = face_detect;
        if (m_flag_camera_start) {
            if (m_face_detect != FACE_DETECTION_OFF) {
                if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FOCUS_MODE, FOCUS_MODE_AUTO) < 0) {
                    LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_FOCUS_MODin face detecion", __func__);
                    return -1;
                }
            }
            if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FACE_DETECTION, face_detect) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_FACE_DETECTION", __func__);
                return -1;
            }
        }
    }

    return 0;
}

int NuCameraV4L2::getFaceDetect(void)
{
    return m_face_detect;
}

int NuCameraV4L2::setFaceDetectLockUnlock(int facedetect_lockunlock)
{
    LOGV("%s(facedetect_lockunlock(%d))", __func__, facedetect_lockunlock);

    if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FACEDETECT_LOCKUNLOCK, facedetect_lockunlock) < 0) {
        LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_FACEDETECT_LOCKUNLOCK", __func__);
        return -1;
    }

    return 0;
}

//======================================================================

int NuCameraV4L2::setGamma(int gamma)
{
     LOGV("%s(gamma(%d))", __func__, gamma);

     if (gamma < GAMMA_OFF || GAMMA_MAX <= gamma) {
         LOGE("ERR(%s):Invalid gamma (%d)", __func__, gamma);
         return -1;
     }

     if (m_video_gamma != gamma) {
         m_video_gamma = gamma;
         if (m_flag_camera_start) {
             if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SET_GAMMA, gamma) < 0) {
                 LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SET_GAMMA", __func__);
                 return -1;
             }
         }
     }

     return 0;
}

//======================================================================

int NuCameraV4L2::setSlowAE(int slow_ae)
{
     LOGV("%s(slow_ae(%d))", __func__, slow_ae);

     if (slow_ae < GAMMA_OFF || GAMMA_MAX <= slow_ae) {
         LOGE("ERR(%s):Invalid slow_ae (%d)", __func__, slow_ae);
         return -1;
     }

     if (m_slow_ae!= slow_ae) {
         m_slow_ae = slow_ae;
         if (m_flag_camera_start) {
             if (nusmart_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SET_SLOW_AE, slow_ae) < 0) {
                 LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SET_SLOW_AE", __func__);
                 return -1;
             }
         }
     }

     return 0;
}
//=======================================================

void NuCameraV4L2::getRecordingSize(int *width, int*height, int*size)
{
     *width = m_recording_width;
     *height = m_recording_height;
      *size = (m_recording_width * m_recording_height>>1)*3;
}

//======================================================================
int NuCameraV4L2::setRecordingSize(int width, int height)
{
     LOGV("%s(width(%d), height(%d))", __func__, width, height);

     m_recording_width  = width;
     m_recording_height = height;

     return 0;
}

//======================================================================

/* Camcorder fix fps */
int NuCameraV4L2::setSensorMode(int sensor_mode)
{
    LOGV("%s(sensor_mode (%d))", __func__, sensor_mode);

    if (sensor_mode < SENSOR_MODE_CAMERA || SENSOR_MODE_MOVIE < sensor_mode) {
        LOGE("ERR(%s):Invalid sensor mode (%d)", __func__, sensor_mode);
        return -1;
    }

    if (m_sensor_mode != sensor_mode) {
        m_sensor_mode = sensor_mode;
    }

    return 0;
}

/*  Shot mode   */
/*  SINGLE = 0
*   CONTINUOUS = 1
*   PANORAMA = 2
*   SMILE = 3
*   SELF = 6
*/
int NuCameraV4L2::setShotMode(int shot_mode)
{
    LOGV("%s(shot_mode (%d))", __func__, shot_mode);
    if (shot_mode < SHOT_MODE_SINGLE || SHOT_MODE_SELF < shot_mode) {
        LOGE("ERR(%s):Invalid shot_mode (%d)", __func__, shot_mode);
        return -1;
    }
    m_shot_mode = shot_mode;

    return 0;
}

const __u8* NuCameraV4L2::getCameraSensorName(void)
{
    LOGV("%s", __func__);

    return nusmart_v4l2_enuminput(m_cam_fd, getCameraId());
}

// ======================================================================
// Jpeg

int NuCameraV4L2::setJpegThumbnailSize(int width, int height)
{
    LOGV("%s(width(%d), height(%d))", __func__, width, height);
    if (width <= m_snapshot_width){
       m_jpeg_thumbnail_width  = width;
       m_jpeg_thumbnail_height = height;
    } else {
       m_jpeg_thumbnail_width  = m_snapshot_width;
       m_jpeg_thumbnail_height = m_snapshot_height;
    }

    return 0;
}

int NuCameraV4L2::getJpegThumbnailSize(int *width, int  *height)
{
    if (width)
        *width   = m_jpeg_thumbnail_width;
    if (height)
        *height  = m_jpeg_thumbnail_height;

    return 0;
}

// ======================================================================
// CPU Change governors base on DVFS
#define DVFS_GOVERNOR_FILE "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
inline int NuCameraV4L2::readGovernor(){
    FILE *fp = NULL;
    int i = 0;
    //clear global variables
    memset(m_DVFS_governor, '\0', DVFS_GOVERNOR_SIZE);
    fp = fopen(DVFS_GOVERNOR_FILE,"r");
    if( fp == NULL ){
        LOGE("ERR(%s), Open %s Failed.ERR FOR:%s(%d)", __func__, DVFS_GOVERNOR_FILE, strerror(errno),errno);
        return -1;
    }
    //read a line from governor file
    fgets(m_DVFS_governor, DVFS_GOVERNOR_SIZE, fp);
    //let 'LF' to '\0'
    m_DVFS_governor[strlen(m_DVFS_governor)-1] = '\0';
    LOGI("INFO(%s), The current camera HAL governor is :%s", __func__, m_DVFS_governor);
    fclose(fp);
    return 0;
}

/*
 * NOTE:The available governor:
 *      userspace powersave ondemand performance
 *      The governor won't be changed when we set an invalid keywords!
 *
 */
inline int NuCameraV4L2::writeGovernor(const char *governor){
    FILE *fp;
    LOGI("INFO: writeGovernor(governor:%s)",governor );
    fp = fopen(DVFS_GOVERNOR_FILE,"r+");
    if( !fp  ){
        LOGE("ERR(%s), Open %s Failed.ERR FOR:%s(%d)", __func__, DVFS_GOVERNOR_FILE, strerror(errno),errno);
        return -1;
    }
    //write to governor file
    fseek(fp, 0L, SEEK_SET);
    fwrite(governor, strlen(governor), 1, fp);
    LOGI("INFO(%s), The current DVFS governor is :%s", __func__, governor);
    fclose(fp);
    return 0;
}


// ======================================================================
// Conversions

inline int NuCameraV4L2::m_frameSize(int format, int width, int height)
{
    int size = 0;

    switch (format) {
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
        size = (width * height * 3 / 2);
        break;

    case V4L2_PIX_FMT_NV12T:
        size = ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height)) +
                            ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height / 2));
        break;

    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_UYVY:
        size = (width * height * 2);
        break;

    default :
        LOGE("ERR(%s):Invalid V4L2 pixel format(%d)\n", __func__, format);
    case V4L2_PIX_FMT_RGB565:
        size = (width * height * BPP);
        break;
    }

    return size;
}

status_t NuCameraV4L2::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, 255, "dump(%d)\n", fd);
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return NO_ERROR;
}

}; // namespace android
