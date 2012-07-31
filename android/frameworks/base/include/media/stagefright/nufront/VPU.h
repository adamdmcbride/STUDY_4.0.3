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

#ifndef VPU_H_

#define VPU_H_

namespace android {

#define VPU_TRUE  1
#define VPU_FALSE 0

#define VPU_SUPPORT VPU_TRUE /*video player will use hardware decoder.*/
//#define VPU_SUPPORT VPU_FALSE /*video player will use software decoder.*/

/*camera use vpu reserved mem*/
#define USE_VPU_MEM  VPU_TRUE

}  // namespace android

#endif  // VPU_H_
