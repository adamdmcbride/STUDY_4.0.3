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


#define LOG_NDEBUG 0
#define LOG_TAG "NuCameraUtils"
#include <cutils/log.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>

#include "Utils.h"

namespace android {

struct PWPARM
{
  unsigned char cmd;
  unsigned char data;
};

#define ENEEC_IOC_MAGIC      ('e' + 0x80) 
#define ENEEC_IOC_PW         _IOWR (ENEEC_IOC_MAGIC, 16, struct PWPARM)
#define CAMERA_POWER_ON      12
#define CAMERA_POWER_OFF     13

#define NUSMART_POWER_EC     "/dev/io373x_pw"

int power_on_camera() {
    int ret;
    int fd;
    struct PWPARM parm;
    fd = open(NUSMART_POWER_EC, O_RDWR);
    if (fd < 0) {
        LOGE("Open EC Power device failed.%s", strerror(errno));
        return -1;
    }
    LOGV("@TEI Open EC Power device success!");

    parm.cmd = CAMERA_POWER_ON;
    ret = ioctl(fd, ENEEC_IOC_PW, &parm);
    if (ret < 0) {
        LOGE("power_on_camera failed.%s", strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}       

int power_off_camera() {
    int ret;
    int fd;
    struct PWPARM parm;
    fd = open(NUSMART_POWER_EC, O_RDWR);
    if (fd < 0) {
        LOGE("Open EC Power device failed.%s", strerror(errno));
        return -1;
    }
    LOGV("@TEI Open EC Power device success!");

    parm.cmd = CAMERA_POWER_OFF;
    ret = ioctl(fd, ENEEC_IOC_PW, &parm);
    if (ret < 0) {
        LOGE("power_off_camera failed.%s", strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

void yuyv422_to_yuv420sp(unsigned char *bufsrc, unsigned char *bufdest, unsigned int width, unsigned int height)
{
    int32_t dst_pos, src_pos;
    unsigned int x, y, src_y_start_pos, dst_cbcr_pos, srcWidth, srcHeight;
    unsigned char *srcBufPointer = (unsigned char *)bufsrc;
    unsigned char *dstBufPointer = (unsigned char *)bufdest;
    
    srcWidth = width;
    srcHeight = height;
    
    dst_pos = 0; 
    dst_cbcr_pos = srcWidth*srcHeight;
    for (uint32_t y = 0; y < srcHeight; y++) {
        src_y_start_pos = (y * (srcWidth * 2)); 
        for (uint32_t x = 0; x < (srcWidth * 2); x += 2) { 
            src_pos = src_y_start_pos + x; 
            dstBufPointer[dst_pos++] = srcBufPointer[src_pos];
        }    
    }    
    for (uint32_t y = 0; y < srcHeight; y += 2) { 
        src_y_start_pos = (y * (srcWidth * 2)); 
        for (uint32_t x = 0; x < (srcWidth * 2); x += 4) { 
            src_pos = src_y_start_pos + x; 
            dstBufPointer[dst_cbcr_pos++] = srcBufPointer[src_pos + 3];
            dstBufPointer[dst_cbcr_pos++] = srcBufPointer[src_pos + 1];
        }    
    }    
}

//void dumpYUYV(void *frame, char *name, int width, int height, int index) {
void dumpYUYV(void *frame, int width, int height, int index) {
    int fd;
    char test_file_path[30];
    // please mkdir /tmp and chmod 777 for testing
    sprintf(test_file_path, "/tmp/nv21_%dx%d_%d", width, height, index);
    if ( (fd = open(test_file_path, O_CREAT | O_RDWR) ) < 0 ) {
        LOGE("@TEI call open(test_file_path=%s) failed:%s",test_file_path, strerror(errno));
        close(fd);
        return;
    }
    if (write(fd, frame, width * height * 2) < 0) {
        LOGE("@TEI call write() failed:%s",strerror(errno));
        close(fd);
        return;
    }
    close(fd);
    LOGV("@TEI dump file (test_file_path=%s) success.",test_file_path);
}

}; /* namespace android */
