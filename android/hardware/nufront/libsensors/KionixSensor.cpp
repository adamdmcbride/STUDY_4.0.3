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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "KionixSensor.h"

/*****************************************************************************/

KionixSensor::KionixSensor()
: SensorBase(DIR_DEV, INPUT_NAME_ACC),
      mEnabled(0),
      mInputReader(8),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    mDelays = 200000000; // 200 ms by default

    // read the actual value of all sensors if they're enabled already
    int flags = 0;

    open_device();

    if (!ioctl(dev_fd, KIONIX_IOCTL_GET_ENABLE, &flags)) {
        mEnabled = 1;
        if (flags) {
            setInitialState();
        }
    }
    if (!mEnabled) {
        close_device();
    }
}

KionixSensor::~KionixSensor() {
}

int KionixSensor::setInitialState() {
    struct input_absinfo absinfo;

	if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
		mPendingEvent.acceleration.x = absinfo.value * CONVERT_A_X;
	}
	if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
		mPendingEvent.acceleration.y = absinfo.value * CONVERT_A_Y;
	}
	if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
		mPendingEvent.acceleration.z = absinfo.value * CONVERT_A_Z;
	}
    return 0;
}

int KionixSensor::enable(int32_t handle, int en)
{
    int cmd = en ? KIONIX_IOCTL_ENABLE_OUTPUT : KIONIX_IOCTL_DISABLE_OUTPUT;
    int newState = en ? 1 : 0;
    int err = 0;
    if (newState != mEnabled) {
        if (!mEnabled) {
            open_device();
        }
        int flags = newState;
    	err = ioctl(dev_fd, cmd);
        err = err<0 ? -errno : 0;
        LOGE_IF(err, "CAPELLA_CM3602_IOCTL_ENABLE failed (%s)", strerror(-err));
        if (!err) {
            mEnabled = newState;
            if (en) {
                setInitialState();
            }
        }
        if (!mEnabled) {
            close_device();
        }
    }
    return err;
}

int KionixSensor::setDelay(int32_t handle, int64_t ns)
{
    if (ns < 0)
        return -EINVAL;

    mDelays = ns;
    return update_delay();
}

int KionixSensor::update_delay()
{
    if (mEnabled) {
        uint64_t wanted = -1LLU;
		uint64_t ns = mDelays;
		wanted = wanted < ns ? wanted : ns;
        int delay = int64_t(wanted) / 1000000;
        if (ioctl(dev_fd, KIONIX_IOCTL_UPDATE_ODR, &delay)) {
            return -errno;
        }
    }
    return 0;
}

bool KionixSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int KionixSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            switch (event->code) {
                case EVENT_TYPE_ACCEL_X:
                    mPendingEvent.acceleration.x = event->value * CONVERT_A_X;
                    break;
                case EVENT_TYPE_ACCEL_Y:
                    mPendingEvent.acceleration.y = event->value * CONVERT_A_Y;
                    break;
                case EVENT_TYPE_ACCEL_Z:
                    mPendingEvent.acceleration.z = event->value * CONVERT_A_Z;
                    break;
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            LOGE("KionixSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}
