/*
 * Copyright Nufront Co.,LTD.
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * JPEG DRIVER MODULE (JpegEncoder.h)
 * Author  : majiping
 * Purpose : This file implements the Exif encoder APIs as needed by Camera HAL
 */
#ifndef __EXIF_API_H__
#define __EXIF_API_H__

#include <stdint.h>
#include <sys/ioctl.h>

#include "Exif.h"

namespace android {

#define EXIF_BUF_SIZE    131072   //128K

typedef enum {
    EXIF_FAIL,
    EXIF_SUCCESS,
    OK_HD_PARSING,
    ERR_HD_PARSING,
    OK_ENC_OR_DEC,
    ERR_ENC_OR_DEC,
    ERR_UNKNOWN
} exif_return_status;

class ExifEncoder {
public:
    ExifEncoder();
    virtual ~ExifEncoder();

    exif_return_status makeExif(unsigned char *exifOut,
                               exif_attribute_t *exifIn,
                               unsigned int *size,
                               bool useMainbufForThumb = false);

private:
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 uint32_t value);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 unsigned char *pValue);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 rational_t *pValue,
                                 unsigned int *offset,
                                 unsigned char *start);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 unsigned char *pValue,
                                 unsigned int *offset,
                                 unsigned char *start);
};
};
#endif /* __EXIF_API_H__ */
