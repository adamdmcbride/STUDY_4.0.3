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
 * Contains implementation of a class EmulatedCameraFactory that manages cameras
 * available for emulation.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "EmulatedCamera_Factory"
#include <cutils/log.h>
#include <cutils/properties.h>
#include "EmulatedQemuCamera.h"
#include "EmulatedFakeCamera.h"
#include "EmulatedCameraFactory.h"

extern camera_module_t HAL_MODULE_INFO_SYM;

/* A global instance of EmulatedCameraFactory is statically instantiated and
 * initialized when camera emulation HAL is loaded.
 */
android::EmulatedCameraFactory  gEmulatedCameraFactory;

namespace android {

EmulatedCameraFactory::EmulatedCameraFactory()
        : mQemuClient(),
          mEmulatedCameras(NULL),
          mEmulatedCameraNum(0),
          mFakeCameraID(-1),
          mConstructedOK(false)

{
    if (isFakeCameraEmulationOn()) {
        /* ID fake camera with the number of created 'qemud' cameras. */
        mFakeCameraID = mEmulatedCameraNum;
        mEmulatedCameraNum++;

        /* Make sure that array is allocated (in case there were no 'qemu'
         * cameras created. */
        if (mEmulatedCameras == NULL) {
            mEmulatedCameras = new EmulatedCamera*[mEmulatedCameraNum + 1];
            if (mEmulatedCameras == NULL) {
                LOGE("%s: Unable to allocate emulated camera array for %d entries",
                     __FUNCTION__, mEmulatedCameraNum);
                return;
            }
            memset(mEmulatedCameras, 0, (mEmulatedCameraNum + 1) * sizeof(EmulatedCamera*));
        }

        /* Create, and initialize the fake camera */
       mEmulatedCameras[mFakeCameraID] =
            new EmulatedFakeCamera(mFakeCameraID, &HAL_MODULE_INFO_SYM.common);
        if (mEmulatedCameras[mFakeCameraID] != NULL) {
            if (mEmulatedCameras[mFakeCameraID]->Initialize() != NO_ERROR) {
                delete mEmulatedCameras[mFakeCameraID];
                mEmulatedCameras--;
                mFakeCameraID = -1;
            }
        } else {
            mEmulatedCameras--;
            mFakeCameraID = -1;
            LOGE("%s: Unable to instantiate fake camera class", __FUNCTION__);
        }
    } else {
        LOGD("Fake camera emulation is disabled.");
    }

    LOGV("%d cameras are being emulated. Fake camera ID is %d",
         mEmulatedCameraNum, mFakeCameraID);

    //we have not supported real double camera yet,change condition to false
    //if (true) {
    if (false) {
        /* ID fake camera with the number of created 'qemud' cameras. */
        mFakeCameraID = mEmulatedCameraNum;
        mEmulatedCameraNum++;

        /* Make sure that array is allocated (in case there were no 'qemu'
         * cameras created. */
        if (mEmulatedCameras == NULL) {
            mEmulatedCameras = new EmulatedCamera*[mEmulatedCameraNum];
            if (mEmulatedCameras == NULL) {
                LOGE("%s: Unable to allocate emulated camera array for %d entries",
                     __FUNCTION__, mEmulatedCameraNum);
                return;
            }
            memset(mEmulatedCameras, 0, mEmulatedCameraNum * sizeof(EmulatedCamera*));
        }

        /* Create, and initialize the fake camera */
        mEmulatedCameras[mFakeCameraID] =
            new EmulatedQemuCamera(mFakeCameraID, &HAL_MODULE_INFO_SYM.common);
        if (mEmulatedCameras[mFakeCameraID] != NULL) {
            if (mEmulatedCameras[mFakeCameraID]->Initialize() != NO_ERROR) {
                delete mEmulatedCameras[mFakeCameraID];
                mEmulatedCameras--;
                mFakeCameraID = -1;
            }
        } else {
            mEmulatedCameras--;
            mFakeCameraID = -1;
            LOGE("%s: Unable to instantiate qemu camera class", __FUNCTION__);
        }
        
        LOGV("%d cameras are being emulated. Qemu camera ID is %d",
            mEmulatedCameraNum, mFakeCameraID);
    } else {
        LOGD("Qemu camera emulation is disabled.");
    }

    mConstructedOK = true;
}

EmulatedCameraFactory::~EmulatedCameraFactory()
{
    if (mEmulatedCameras != NULL) {
        for (int n = 0; n < mEmulatedCameraNum; n++) {
            if (mEmulatedCameras[n] != NULL) {
                delete mEmulatedCameras[n];
            }
        }
        delete[] mEmulatedCameras;
    }
}

/****************************************************************************
 * Camera HAL API handlers.
 *
 * Each handler simply verifies existence of an appropriate EmulatedCamera
 * instance, and dispatches the call to that instance.
 *
 ***************************************************************************/

int EmulatedCameraFactory::cameraDeviceOpen(int camera_id, hw_device_t** device)
{
    LOGV("%s: id = %d", __FUNCTION__, camera_id);

    *device = NULL;

    if (!isConstructedOK()) {
        LOGE("%s: EmulatedCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= getEmulatedCameraNum()) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getEmulatedCameraNum());
        return -EINVAL;
    }

    return mEmulatedCameras[camera_id]->connectCamera(device);
}

int EmulatedCameraFactory::getCameraInfo(int camera_id, struct camera_info* info)
{
    LOGV("%s: id = %d", __FUNCTION__, camera_id);

    if (!isConstructedOK()) {
        LOGE("%s: EmulatedCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= getEmulatedCameraNum()) {
        LOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getEmulatedCameraNum());
        return -EINVAL;
    }

    return mEmulatedCameras[camera_id]->getCameraInfo(info);
}

/****************************************************************************
 * Camera HAL API callbacks.
 ***************************************************************************/

int EmulatedCameraFactory::device_open(const hw_module_t* module,
                                       const char* name,
                                       hw_device_t** device)
{
    /*
     * Simply verify the parameters, and dispatch the call inside the
     * EmulatedCameraFactory instance.
     */

    if (module != &HAL_MODULE_INFO_SYM.common) {
        LOGE("%s: Invalid module %p expected %p",
             __FUNCTION__, module, &HAL_MODULE_INFO_SYM.common);
        return -EINVAL;
    }
    if (name == NULL) {
        LOGE("%s: NULL name is not expected here", __FUNCTION__);
        return -EINVAL;
    }

    return gEmulatedCameraFactory.cameraDeviceOpen(atoi(name), device);
}

int EmulatedCameraFactory::get_number_of_cameras(void)
{
    return gEmulatedCameraFactory.getEmulatedCameraNum();
}

int EmulatedCameraFactory::get_camera_info(int camera_id,
                                           struct camera_info* info)
{
    return gEmulatedCameraFactory.getCameraInfo(camera_id, info);
}

/********************************************************************************
 * Internal API
 *******************************************************************************/

/*
 * Camera information tokens passed in response to the "list" factory query.
 */

/* Device name token. */
static const char lListNameToken[]    = "name=";
/* Frame dimensions token. */
static const char lListDimsToken[]    = "framedims=";
/* Facing direction token. */
static const char lListDirToken[]     = "dir=";

bool EmulatedCameraFactory::isFakeCameraEmulationOn()
{
    /* Defined by 'qemu.sf.fake_camera' boot property: If property is there
     * and contains 'off', fake camera emulation is disabled. */
    char prop[PROPERTY_VALUE_MAX];
    if (property_get("qemu.sf.fake_camera", prop, NULL) <= 0 ||
        strcmp(prop, "off")) {
        return true;
    } else {
        return false;
    }
}

/********************************************************************************
 * Initializer for the static member structure.
 *******************************************************************************/

/* Entry point for camera HAL API. */
struct hw_module_methods_t EmulatedCameraFactory::mCameraModuleMethods = {
    open: EmulatedCameraFactory::device_open
};

}; /* namespace android */
