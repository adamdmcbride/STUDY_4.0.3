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
#ifndef HW_NUCAMERA_V4L2_H
#define HW_NUCAMERA_V4L2_H

#include <linux/videodev2.h>

#define ENABLE_ESD_PREVIEW_CHECK

#define CAMERA_DEV_NAME   "/dev/video0"

#define MAX_BUFFERS     4

#define CHECK(return_value)                                          \
    if (return_value < 0) {                                          \
        LOGE("%s::%d fail. errno: %s\n",                             \
             __func__, __LINE__, strerror(errno));                   \
        return -1;                                                   \
    }

namespace android {

struct fimc_buffer {
    void    *start;
    size_t  length;
};

int get_pixel_depth(unsigned int fmt);

int fimc_v4l2_querycap(int fp);

const __u8* fimc_v4l2_enuminput(int fp, int index);

int fimc_v4l2_s_input(int fp, int index);

int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, int flag_capture);

int fimc_v4l2_s_fmt_cap(int fp, int width, int height, unsigned int fmt);

int fimc_v4l2_enum_fmt(int fp, unsigned int fmt);

int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs);

int fimc_v4l2_querybuf(int fp, struct fimc_buffer *buffer, enum v4l2_buf_type type, int index);

int fimc_v4l2_streamon(int fp);

int fimc_v4l2_streamoff(int fp);

int fimc_v4l2_qbuf(int fp, int index);

int fimc_v4l2_dqbuf(int fp);

}; /* namespace android */

#endif
