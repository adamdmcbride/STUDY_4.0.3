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

#ifndef HARDWARE_RENDERER_H_

#define HARDWARE_RENDERER_H_

#include <media/stagefright/ColorConverter.h>
#include <media/stagefright/nufront/VPU.h>
#include <utils/RefBase.h>
#include <ui/android_native_buffer.h>

namespace android {

struct MetaData;

class HardwareRenderer {
public:
    HardwareRenderer(
            const sp<ANativeWindow> &nativeWindow, const sp<MetaData> &meta);

    ~HardwareRenderer();

    void render(
            const void *data, size_t size, void *platformPrivate);

    void ACodecRender(
            const void *data, size_t size, void *platformPrivate);

private:
    enum YUVMode {
        None,
    };

    OMX_COLOR_FORMATTYPE mColorFormat;
    ColorConverter *mConverter;
    YUVMode mYUVMode;
    sp<ANativeWindow> mNativeWindow;
    int32_t mWidth, mHeight;
    int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;
    int32_t mCropWidth, mCropHeight;
    int32_t mDisplayWidth, mDisplayHeight;
	float   mBytesPerPixel;
	bool    mVideoMemMapped;
    mutable Mutex mMutex;
    uint32_t mHALFormat;

    HardwareRenderer(const HardwareRenderer &);
    HardwareRenderer &operator=(const HardwareRenderer &);
};

}  // namespace android

#endif  // HARDWARE_RENDERER_H_
