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
#ifndef HW_NUCAMERA_UTILS_H
#define HW_NUCAMERA_UTILS_H

#define WAIT_POWER_ON_TRY_TIMES 10
//200 ms
#define WAIT_POWER_ON_TIME_TRY 200000
// 1.5 s
#define WAIT_POWER_ON_TIME 1500000

namespace android {

int power_on_camera();

int power_off_camera();

void yuyv422_to_yuv420sp(unsigned char *bufsrc, unsigned char *bufdest, unsigned int width, unsigned int height);

//void dumpYUYV(void *frame, char * name, int width, int height, int index);
void dumpYUYV(void *frame, int width, int height, int index);

}; /* namespace android */

#endif
