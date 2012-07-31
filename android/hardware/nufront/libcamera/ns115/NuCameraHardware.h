/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Nufront Co. LTD
**
*/

#ifndef ANDROID_HARDWARE_NUFRONT_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_NUFRONT_CAMERA_HARDWARE_H

#include "NuCameraV4L2.h"
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <camera/CameraParameters.h>
#include <system/graphics.h>

namespace android {

///////////////////////////////////////////////////////////////////////
//
//   make magic number
//
///////////////////////////////////////////////////////////////////////
#define NUSMART_CAMERA_SOURCE_KEY   0x12345678

typedef struct
{
    unsigned int  magic_key;   // = NUSMART_CAMERA_SOURCE_KEY
    unsigned int  phy_addr;    // physical address of image frame
    unsigned int *vir_addr;    // virtual address of image frame
    unsigned int  size;        // size of image frame
}nusmart_nativewindow_buf;


///////////////////////////////////////////////////////////////////////
//
//   implement Camera HAL API
//
///////////////////////////////////////////////////////////////////////
class NuCameraHardware : public virtual RefBase {
public:
    //interface for hardware/camera.h's camera_device_ops
    virtual void        setCallbacks(camera_notify_callback notify_cb,
                                     camera_data_callback data_cb,
                                     camera_data_timestamp_callback data_cb_timestamp,
                                     camera_request_memory get_memory,
                                     void *user);
    //message functions
    virtual void        enableMsgType(int32_t msgType);
    virtual void        disableMsgType(int32_t msgType);
    virtual bool        msgTypeEnabled(int32_t msgType);

    //preview functions
    virtual status_t    startPreview();
    virtual void        stopPreview();
    virtual bool        previewEnabled();
    virtual status_t    setPreviewWindow(preview_stream_ops *w);

    //recording functions
    virtual status_t    startRecording();
    virtual void        stopRecording();
    virtual bool        recordingEnabled();
    virtual void        releaseRecordingFrame(const void *opaque);

    //focus functions
    virtual status_t    autoFocus();
    virtual status_t    cancelAutoFocus();

    //picture functions
    virtual status_t    takePicture();
    virtual status_t    cancelPicture();

    //parameters
    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters  getParameters() const;


    virtual status_t    storeMetaDataInBuffers(bool enable);

    //device
    virtual void        release();
    inline  int         getCameraId() const;

    //debug
    virtual status_t    dump(int fd) const;
    virtual status_t    sendCommand(int32_t command, int32_t arg1, int32_t arg2);

    NuCameraHardware(int cameraId, camera_device_t *dev);
    virtual             ~NuCameraHardware();
private:

/////////////////////////////////////////////////////////////////
//
//     Init and parameters
//
////////////////////////////////////////////////////////////////

    static  const int   mBufferCount = NUM_PREVIEW_BUFFERS;

            void        initDefaultParameters(int cameraId);
inline     void        initFrontParameters();
inline     void        initBackParameters();
            bool        isSupportedPreviewSize(const int width,
                                               const int height) const;
            bool        isSupportedParameter(const char * const parm,
                            const char * const supported_parm) const;
    // fields
    CameraParameters    mParameters;
    CameraParameters    mInternalParameters;
           Vector<Size> mSupportedPreviewSizes;
            int32_t     mMsgEnabled;

/////////////////////////////////////////////////////////////////
//
//     Preview Thread
//
////////////////////////////////////////////////////////////////
    class PreviewThread : public Thread {
        NuCameraHardware *mHardware;
    public:
        PreviewThread(NuCameraHardware *hw):
        Thread(true),
        mHardware(hw){ }
        virtual void onFirstRef() {
            run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
        }
        virtual bool threadLoop() {
            mHardware->previewThreadWrapper();
            return false;
        }
    };

    // functions
    inline int         previewThread();
           int         previewThreadWrapper();
           int         previewThreadInternal();
           status_t    startPreviewInternal();
           void        stopPreviewInternal();
           void        setSkipFrame(int frame);
           bool        isPreviewTime();
    // fields
    sp<PreviewThread>   mPreviewThread;
    preview_stream_ops *mPreviewWindow;
    /* used by preview thread to block until it's told to run */
    mutable Mutex       mPreviewLock;
    mutable Condition   mPreviewCondition;
    mutable Condition   mPreviewStoppedCondition;
    mutable Mutex       mSkipFrameLock;
            int         mSkipFrame;
            bool        mPreviewRunning;
            bool        mPreviewStartDeferred;
            bool        mExitPreviewThread;


            uint64_t    mLastPreviewed;
            uint32_t    mPreviewAfter;

/////////////////////////////////////////////////////////////////
//
//     AutoFocus Thread
//
////////////////////////////////////////////////////////////////

class AutoFocusThread : public Thread {
        NuCameraHardware *mHardware;
    public:
        AutoFocusThread(NuCameraHardware *hw): Thread(false), mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraAutoFocusThread", PRIORITY_DEFAULT);
        }
        virtual bool threadLoop() {
            mHardware->autoFocusThread();
            return true;
        }
};

    /* used by auto focus thread to block until it's told to run */
    mutable Mutex       mFocusLock;
    mutable Condition   mFocusCondition;
            bool        mExitAutoFocusThread;
    sp<AutoFocusThread> mAutoFocusThread;
            int         autoFocusThread();


/////////////////////////////////////////////////////////////////
//
//     Capture Thread
//
////////////////////////////////////////////////////////////////
    class CaptureThread : public Thread {
        NuCameraHardware  *mHardware;
    public:
         CaptureThread(NuCameraHardware *hw):
         Thread(true),
         mHardware(hw),
         mOneBurst(false){ }

         virtual bool threadLoop() {
            if (mHardware->captureThread()){
                return !mOneBurst;
            } else {
                return false;
            }
         }

        inline status_t startThread(bool one_burst)
        {
            mOneBurst = one_burst;
            return run("CameraCaptureThread", ANDROID_PRIORITY_URGENT_DISPLAY, 0);
        }

        inline void stopThread(void){
            mOneBurst = true;
        }

    private:
        bool mOneBurst;
    };

    // functions
   inline     bool         captureThread();
              int          pictureThreadInternal();
              int          recordingThreadInternal();
            status_t    waitCaptureCompletion();

    // fields
            bool        mCaptureInProgress;
    sp<CaptureThread>   mCaptureThread;
    // used to guard mCaptureInProgress
    mutable Mutex       mCaptureLock;
    mutable Condition   mCaptureCondition;
            int         mPostViewWidth;
            int         mPostViewHeight;
            int         mPostViewSize;

/////////////////////////////////////////////////////////////////
//
//     Recording Thread
//
////////////////////////////////////////////////////////////////

            bool        mRecordRunning;
    mutable Mutex       mRecordLock;
            bool        mArrived;

/////////////////////////////////////////////////////////////////
//
//     User and User callbacks
//
////////////////////////////////////////////////////////////////

    void                      *mCallbackCookie;
    camera_notify_callback     mNotifyCb;
    camera_data_callback       mDataCb;
    camera_data_timestamp_callback mDataCbTimestamp;
    camera_request_memory      mGetMemoryCb;

    camera_memory_t           *mPreviewHeap;  //size=w*h*4,  for RGBA
    camera_memory_t           *mRawHeap;
    camera_memory_t           *mRecordHeap;

/////////////////////////////////////////////////////////////////
//
//     Device
//
////////////////////////////////////////////////////////////////
    camera_device_t               *mHalDevice;
    NuCameraV4L2                  *mNuCameraV4L2;
    const __u8                    *mCameraSensorName;
    static gralloc_module_t const *mGrallocHal;

}; // end class NuCameraHardware

}; // end namespace android

#endif
