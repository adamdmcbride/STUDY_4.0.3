/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef AAC_ENCODER_H
#define AAC_ENCODER_H

#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
/*a@nufront start*/
#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/stagefright/MediaBuffer.h>
#include <utils/List.h>
/*a@nufront end*/
struct VO_AUDIO_CODECAPI;
struct VO_MEM_OPERATOR;

namespace android {

struct MediaBufferGroup;

/*m@nufront start*/
class AACEncoder : public MediaSource, public MediaBufferObserver {
/*m@nufront end*/
    public:
        AACEncoder(const sp<MediaSource> &source, const sp<MetaData> &meta);

        virtual status_t start(MetaData *params);
        virtual status_t stop();
        virtual sp<MetaData> getFormat();
        virtual status_t read(
                MediaBuffer **buffer, const ReadOptions *options);
        /*a@nufront start*/
        virtual void signalBufferReturned(MediaBuffer *buffer);
        /*a@nufront end*/

    protected:
        virtual ~AACEncoder();

    private:
        sp<MediaSource>   mSource;
        sp<MetaData>      mMeta;
        bool              mStarted;
        MediaBuffer      *mInputBuffer;
        /*m@nufront start*/
        //MediaBufferGroup *mBufferGroup;
        /*m@nufront end*/
        status_t          mInitCheck;
        int32_t           mSampleRate;
        int32_t           mChannels;
        int32_t           mBitRate;
        int32_t           mFrameCount;

        int64_t           mAnchorTimeUs;
        int64_t           mNumInputSamples;

        enum {
            kNumSamplesPerFrame = 1024,
        };

        int16_t           *mInputFrame;

        uint8_t           mAudioSpecificConfigData[2]; // auido specific data
        void             *mEncoderHandle;
        VO_AUDIO_CODECAPI *mApiHandle;
        VO_MEM_OPERATOR  *mMemOperator;

        status_t setAudioSpecificConfigData();
        status_t initCheck();

        AACEncoder& operator=(const AACEncoder &rhs);
        AACEncoder(const AACEncoder& copy);
        /*a@nufront start*/
        int64_t mNumClientOwnedBuffers;
        Mutex mLock;
        Condition mFrameEncodingCompletionCondition;
        void waitOutstandingEncodingFrames_l();
        /*a@nufront end*/

};

}

#endif  //#ifndef AAC_ENCODER_H

