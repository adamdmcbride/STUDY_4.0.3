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
 * Contains implementation of a class CallbackNotifier that manages callbacks set
 * via set_callbacks, enable_msg_type, and disable_msg_type camera HAL API.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "EmulatedCamera_CallbackNotifier"
#include <cutils/log.h>
#include <media/stagefright/MetadataBufferType.h>
#include "EmulatedCameraDevice.h"
#include "CallbackNotifier.h"
#include "JpegCompressor.h"
#include "Converters.h"
#include "Utils.h"

namespace android {

#define EXIF_BUF_SIZE 1024

/* String representation of camera messages. */
static const char* lCameraMessages[] =
{
    "CAMERA_MSG_ERROR",
    "CAMERA_MSG_SHUTTER",
    "CAMERA_MSG_FOCUS",
    "CAMERA_MSG_ZOOM",
    "CAMERA_MSG_PREVIEW_FRAME",
    "CAMERA_MSG_VIDEO_FRAME",
    "CAMERA_MSG_POSTVIEW_FRAME",
    "CAMERA_MSG_RAW_IMAGE",
    "CAMERA_MSG_COMPRESSED_IMAGE",
    "CAMERA_MSG_RAW_IMAGE_NOTIFY",
    "CAMERA_MSG_PREVIEW_METADATA"
};
static const int lCameraMessagesNum = sizeof(lCameraMessages) / sizeof(char*);

/* Builds an array of strings for the given set of messages.
 * Param:
 *  msg - Messages to get strings for,
 *  strings - Array where to save strings
 *  max - Maximum number of entries in the array.
 * Return:
 *  Number of strings saved into the 'strings' array.
 */
static int GetMessageStrings(uint32_t msg, const char** strings, int max)
{
    int index = 0;
    int out = 0;
    while (msg != 0 && out < max && index < lCameraMessagesNum) {
        while ((msg & 0x1) == 0 && index < lCameraMessagesNum) {
            msg >>= 1;
            index++;
        }
        if ((msg & 0x1) != 0 && index < lCameraMessagesNum) {
            strings[out] = lCameraMessages[index];
            out++;
            msg >>= 1;
            index++;
        }
    }

    return out;
}

/* Logs messages, enabled by the mask. */
static void PrintMessages(uint32_t msg)
{
    const char* strs[lCameraMessagesNum];
    const int translated = GetMessageStrings(msg, strs, lCameraMessagesNum);
    for (int n = 0; n < translated; n++) {
        LOGV("    %s", strs[n]);
    }
}

CallbackNotifier::CallbackNotifier()
    : mNotifyCB(NULL),
      mDataCB(NULL),
      mDataCBTimestamp(NULL),
      mGetMemoryCB(NULL),
      mCBOpaque(NULL),
      mLastFrameTimestamp(0),
      mFrameRefreshFreq(0),
      mMessageEnabler(0),
      mJpegQuality(90),
      mVideoRecEnabled(false),
      mTakingPicture(false),
      mRecordBuffer(NULL)
      //mMirrorBuffer(NULL)
{
}

CallbackNotifier::~CallbackNotifier()
{
}

/****************************************************************************
 * Camera API
 ***************************************************************************/

void CallbackNotifier::setCallbacks(camera_notify_callback notify_cb,
                                    camera_data_callback data_cb,
                                    camera_data_timestamp_callback data_cb_timestamp,
                                    camera_request_memory get_memory,
                                    void* user)
{
    LOGV("%s: %p, %p, %p, %p (%p)",
         __FUNCTION__, notify_cb, data_cb, data_cb_timestamp, get_memory, user);

    Mutex::Autolock locker(&mObjectLock);
    mNotifyCB = notify_cb;
    mDataCB = data_cb;
    mDataCBTimestamp = data_cb_timestamp;
    mGetMemoryCB = get_memory;
    mCBOpaque = user;
}

void CallbackNotifier::enableMessage(uint msg_type)
{
    LOGV("%s: msg_type = 0x%x", __FUNCTION__, msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler |= msg_type;
    LOGV("**** Currently enabled messages:");
    PrintMessages(mMessageEnabler);
}

void CallbackNotifier::disableMessage(uint msg_type)
{
    LOGV("%s: msg_type = 0x%x", __FUNCTION__, msg_type);
    PrintMessages(msg_type);

    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler &= ~msg_type;
    LOGV("**** Currently enabled messages:");
    PrintMessages(mMessageEnabler);
}

//status_t CallbackNotifier::enableVideoRecording(int fps)
status_t CallbackNotifier::enableVideoRecording(int fps, EmulatedCameraDevice* camera_dev)
{
    LOGV("%s", __FUNCTION__);

    Mutex::Autolock locker(&mObjectLock);
    mVideoRecEnabled = true;
    mLastFrameTimestamp = 0;
    mFrameRefreshFreq = 1000000000LL / fps;
    //zhuyx--add--[
    if (mRecordBuffer) {
        LOGV("%s, case mRecordBuffer != NULL ,release it!", __FUNCTION__);
        mRecordBuffer->release(mRecordBuffer);
        mRecordBuffer = 0;
    }
    LOGV("%s:recording fps->%d, frame buffer size->%d", __FUNCTION__, fps, camera_dev->getFrameBufferSize());
    mRecordBuffer = mGetMemoryCB(-1, camera_dev->getFrameBufferSize(), 1, NULL);
    if (!mRecordBuffer) {
        LOGE("ERR(%s): Record heap creation fail", __func__);
        return UNKNOWN_ERROR;
    }
    /*
    mMirrorBuffer = malloc(camera_dev->getFrameWidth() * camera_dev->getFrameHeight() * 3 / 2);
    if (mMirrorBuffer == NULL) {
        LOGE("ERR(%s): malloc mMirrorBuffer fail", __func__);
        return UNKNOWN_ERROR;
    }
    */
    //zhuyx--add--]

    return NO_ERROR;
}

void CallbackNotifier::disableVideoRecording()
{
    LOGV("%s:", __FUNCTION__);

    Mutex::Autolock locker(&mObjectLock);
    mVideoRecEnabled = false;
    mLastFrameTimestamp = 0;
    mFrameRefreshFreq = 0;
    if (mRecordBuffer) {
        mRecordBuffer->release(mRecordBuffer);
        mRecordBuffer = 0;
    }
    /*
    if(mMirrorBuffer != NULL){
        free(mMirrorBuffer);
        mMirrorBuffer == NULL;
    }
    */
}

void CallbackNotifier::releaseRecordingFrame(const void* opaque)
{
    /* We don't really have anything to release here, since we report video
     * frames by copying them directly to the camera memory. */
}

status_t CallbackNotifier::storeMetaDataInBuffers(bool enable)
{
    /* Return INVALID_OPERATION means HAL does not support metadata. So HAL will
     * return actual frame data with CAMERA_MSG_VIDEO_FRRAME. Return
     * INVALID_OPERATION to mean metadata is not supported. */
    return INVALID_OPERATION;
}

/****************************************************************************
 * Public API
 ***************************************************************************/

void CallbackNotifier::cleanupCBNotifier()
{
    Mutex::Autolock locker(&mObjectLock);
    mMessageEnabler = 0;
    mNotifyCB = NULL;
    mDataCB = NULL;
    mDataCBTimestamp = NULL;
    mGetMemoryCB = NULL;
    mCBOpaque = NULL;
    mLastFrameTimestamp = 0;
    mFrameRefreshFreq = 0;
    mJpegQuality = 90;
    mVideoRecEnabled = false;
    mTakingPicture = false;
}

void mirrorNV21(const void* from, void* to, int width, int height)
{
    unsigned char* f = (unsigned char *) from;
    unsigned char* t = (unsigned char *) to;
    for (int j = 0; j < height * 3 / 2; j++) {
        t += width;
        for (int i = 0; i < width; i++) {
            *t-- = *f++;
        }
        t += width;
    }
}

static void mirrorYUYV(const void* from, void* to, int width, int height)
{
    unsigned char* f = (unsigned char *) from;
    unsigned char* t = (unsigned char *) to;
    int w = width << 1;
    for (int j = 0; j < height; j++) {
        t += w;
        for (int i = 0; i < width; i += 2) {
            *(t-2) = *f++;
            *(t-3) = *f++;
            *(t-4) = *f++;
            *(t-1) = *f++;
            t -= 4;
        }
        t += w ;
    }
}

//zhuyx--add--just for test--[
int writeToTmpFile(void *frame,ssize_t length,int name_number){
     FILE *fp;
     char test_file_path[30];
     //name_number = 0;
     sprintf(test_file_path, "%stest.yuv","/data/zhuyuxin/");
     if( (fp = fopen(test_file_path,"w+") ) < 0 ){
        LOGE("@TEI call fopen(test_file_path) failed:%s",strerror(errno));
        fclose(fp);
        return -1;
     }
     if (fwrite(frame, length, 1, fp) != 1) {
        LOGE("@TEI call fputs(frame,fp) failed:%s",strerror(errno));
        fclose(fp);
        return -1;
     }
     fflush(fp);
     fclose(fp);
     return 0;
}
//zhuyx--add--just for test--]

void CallbackNotifier::onNextFrameAvailable(const void* frame,
                                            nsecs_t timestamp,
                                            EmulatedCameraDevice* camera_dev)
{
    //LOGV("CallbackNotifier::onNextFrameAvailable");
    if (isMessageEnabled(CAMERA_MSG_VIDEO_FRAME) && isVideoRecordingEnabled() &&
            isNewVideoFrameTime(timestamp)) {
        //LOGV("CallbackNotifier::onNextFrameAvailable, call mGetMemoryCB and mDataCBTimestamp");
        camera_memory_t* cam_buff = mRecordBuffer;
        //zhuyx--add--[
        //LOGV("CallbackNotifier::onNextFrameAvailable, reslotion:%dx%d",camera_dev->getFrameWidth(), camera_dev->getFrameHeight());
        //mirrorNV21(frame, mMirrorBuffer, camera_dev->getFrameWidth(), camera_dev->getFrameHeight());
        //zhuyx--test--[
        //LOGV("CallbackNotifier::onNextFrameAvailable, call writeToTmpFile");
        //writeToTmpFile((char *)mMirrorBuffer,320*240*1.5,0);
        //zhuyx--test--]
        if (NULL != cam_buff && NULL != cam_buff->data) {
            memcpy(cam_buff->data, frame, camera_dev->getFrameBufferSize());
            //memcpy(cam_buff->data, mMirrorBuffer, camera_dev->getFrameBufferSize());
            //zhuyx--add--]
            mDataCBTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME,
                               cam_buff, 0, mCBOpaque);
        } else {
            LOGE("%s: Memory failure in CAMERA_MSG_VIDEO_FRAME", __FUNCTION__);
        }
    }

    if (isMessageEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
        camera_memory_t* cam_buff =
            mGetMemoryCB(-1, camera_dev->getFrameBufferSize(), 1, NULL);
        if (NULL != cam_buff && NULL != cam_buff->data) {
            memcpy(cam_buff->data, frame, camera_dev->getFrameBufferSize());
            mDataCB(CAMERA_MSG_PREVIEW_FRAME, cam_buff, 0, NULL, mCBOpaque);
            cam_buff->release(cam_buff);
        } else {
            LOGE("%s: Memory failure in CAMERA_MSG_PREVIEW_FRAME", __FUNCTION__);
        }
    }

    if (mTakingPicture) {
        LOGV("CallbackNotifier::onNextFrameAvailable, case mTakingPicture,pic_width:%d, pic_height:%d",
                               camera_dev->getFrameWidth(), camera_dev->getFrameHeight());
        /* This happens just once. */
        mTakingPicture = false;
        /* The sequence of callbacks during picture taking is:
         *  - CAMERA_MSG_SHUTTER
         *  - CAMERA_MSG_RAW_IMAGE_NOTIFY
         *  - CAMERA_MSG_COMPRESSED_IMAGE
         */
        if (isMessageEnabled(CAMERA_MSG_SHUTTER)) {
            mNotifyCB(CAMERA_MSG_SHUTTER, 0, 0, mCBOpaque);
        }
        if (isMessageEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY)) {
            mNotifyCB(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCBOpaque);
        }
        if (isMessageEnabled(CAMERA_MSG_COMPRESSED_IMAGE)) {
            /* Compress the frame to JPEG. Note that when taking pictures, we
             * have requested camera device to provide us with NV21 frames. */
            //zhuyx --add --[
            /*
            LOGV("CallbackNotifier::onNextFrameAvailable, case mTakingPicture");
            int pic_width,pic_height;
            pic_width = camera_dev->getFrameWidth();
            pic_height = camera_dev->getFrameHeight();
            status_t res;
            void *nv21frame = NULL;
            if(camera_dev->getColorEffect() != Effect::CAMERA_EFFECT_NONE){
                LOGV("CallbackNotifier::onNextFrameAvailable, mTakingPicture, case != Effect::CAMERA_EFFECT_NONE, calloc");
                nv21frame = calloc(1,(size_t)(pic_width * pic_height * 3/2));
                if(nv21frame == NULL){
                   LOGE("CallbackNotifier::onNextFrameAvailable, calloc failed!");
                }
                memcpy(nv21frame,frame,(pic_width * pic_height * 3/2));
                LOGV("CallbackNotifier::onNextFrameAvailable, mTakingPicture, case != Effect::CAMERA_EFFECT_NONE, call getNV21EffectFrame");
                camera_dev->getNV21EffectFrame(nv21frame,pic_width,pic_height);
                //dumpYUYV(nv21frame, pic_width, pic_width, 0);
            }
            LOGV("CallbackNotifier::onNextFrameAvailable, NV21JpegCompressor compressor");
            //void *nv21frame = calloc(1,(size_t)(camera_dev->getFrameWidth() * camera_dev->getFrameHeight());
            //camera_dev->getCurrentPreviewFrame(frame);
            NV21JpegCompressor compressor;
            if(camera_dev->getColorEffect() == Effect::CAMERA_EFFECT_NONE){
                LOGV("CallbackNotifier::onNextFrameAvailable, case CAMERA_EFFECT_NONE");
                res = compressor.compressRawImage(frame, camera_dev->getFrameWidth(),
                                            camera_dev->getFrameHeight(),
                                            mJpegQuality);
            }else{
                LOGV("CallbackNotifier::onNextFrameAvailable, case not CAMERA_EFFECT_NONE");
                res = compressor.compressRawImage(nv21frame, pic_width, pic_height, mJpegQuality);
            }
            */
            //zhuyx --add --]
            /*
            void *tmp = malloc(camera_dev->getFrameBufferSize());
            if (NULL == tmp)
            {
                return ;
            }
            */
            //mirrorYUYV(frame, tmp, camera_dev->getFrameWidth(), camera_dev->getFrameHeight());

            NV21EffectJpegCompressor compressor;
            status_t res = compressor.compressRawImage(frame, camera_dev->getFrameWidth(),
                            camera_dev->getFrameHeight(),
                            mJpegQuality, camera_dev->getColorEffect());
            //free(tmp);
            if (res == NO_ERROR) {
                camera_memory_t *exifHeap = mGetMemoryCB(-1, EXIF_BUF_SIZE, 1, 0);
                int exifSize = 0;
                if (NULL != exifHeap){
                   exifSize = camera_dev->getExif((unsigned char *)exifHeap->data,NULL);
                }

                if (exifSize < 0)
                {
                   LOGE("ERR(%s): exif size: %d", __func__, exifSize);
                   exifHeap->release(exifHeap);
                   exifSize = 0;
                }
                camera_memory_t* jpeg_buff =
                    mGetMemoryCB(-1, compressor.getCompressedSize()+exifSize, 1, NULL);
                if (NULL != jpeg_buff && NULL != jpeg_buff->data) {
                    unsigned char * ptr = (unsigned char *)(jpeg_buff->data);
                    compressor.getCompressedImage(ptr + exifSize);
                    if (exifHeap != NULL){
                        memset(ptr, 0xFF, 1);
                        memset(ptr+1, 0xD8,1);
                        memcpy(ptr+2, exifHeap->data, exifSize);
                        exifHeap->release(exifHeap);
                    }
                    mDataCB(CAMERA_MSG_COMPRESSED_IMAGE, jpeg_buff, 0, NULL, mCBOpaque);
                    jpeg_buff->release(jpeg_buff);
                } else {
                    LOGE("%s: Memory failure in CAMERA_MSG_VIDEO_FRAME", __FUNCTION__);
                }
            } else {
                LOGE("%s: Compression failure in CAMERA_MSG_VIDEO_FRAME", __FUNCTION__);
            }
            /*
            if(nv21frame != NULL){
                LOGV("CallbackNotifier::onNextFrameAvailable, case nv21frame != NULL, free it");
                free(nv21frame);
                nv21frame = NULL;
            }
            */
        }
    }
}

void CallbackNotifier::onCameraDeviceError(int err)
{
    if (isMessageEnabled(CAMERA_MSG_ERROR) && mNotifyCB != NULL) {
        mNotifyCB(CAMERA_MSG_ERROR, err, 0, mCBOpaque);
    }
}

/****************************************************************************
 * Private API
 ***************************************************************************/

bool CallbackNotifier::isNewVideoFrameTime(nsecs_t timestamp)
{
    Mutex::Autolock locker(&mObjectLock);
    if ((timestamp - mLastFrameTimestamp) >= mFrameRefreshFreq) {
        mLastFrameTimestamp = timestamp;
        return true;
    }
    return false;
}

}; /* namespace android */
