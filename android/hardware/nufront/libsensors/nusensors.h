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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

#include "kxud9.h"
#define KIONIX_IOCTL_ENABLE_OUTPUT	KXUD9_IOCTL_ENABLE_OUTPUT
#define KIONIX_IOCTL_DISABLE_OUTPUT	KXUD9_IOCTL_DISABLE_OUTPUT
#define KIONIX_IOCTL_GET_ENABLE		KXUD9_IOCTL_GET_ENABLE
#define KIONIX_IOCTL_UPDATE_ODR		KXUD9_IOCTL_UPDATE_ODR

__BEGIN_DECLS

/*****************************************************************************/

int init_nusensors(hw_module_t const* module, hw_device_t** device);

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0)
#define ID_L  (1)
#define  ID_MAGNETIC_FIELD 	(2)
#define  ID_ORIENTATION    	(3)

/*****************************************************************************/

/*
 * The SENSORS Module
 */


/* the CM3602 is a binary proximity sensor that triggers around 9 cm on
 * this hardware */
#define PROXIMITY_THRESHOLD_CM  9.0f

/*****************************************************************************/

#define LS_DEVICE_NAME      "/dev/lightsensor"
#define AMI_DEVICE_NAME     "/dev/ami304hal"

#define EVENT_TYPE_ACCEL_X          ABS_Y
#define EVENT_TYPE_ACCEL_Y          ABS_X
#define EVENT_TYPE_ACCEL_Z          ABS_Z
#define EVENT_TYPE_ACCEL_STATUS     ABS_WHEEL

#define EVENT_TYPE_YAW              ABS_RX
#define EVENT_TYPE_PITCH            ABS_RY
#define EVENT_TYPE_ROLL             ABS_RZ
#define EVENT_TYPE_ORIENT_STATUS    ABS_RUDDER

#define EVENT_TYPE_MAGV_X           ABS_HAT0X	 /* x-axis of raw magnetic vector */
#define EVENT_TYPE_MAGV_Y           ABS_HAT0Y	 /* y-axis of raw magnetic vector */
#define EVENT_TYPE_MAGV_Z           ABS_BRAKE	 /* z-axis of raw magnetic vector */
#define EVENT_TYPE_MAGV_STATUS      ABS_WHEEL	/* status of magnetic sensor */

#define EVENT_TYPE_TEMPERATURE      ABS_THROTTLE
#define EVENT_TYPE_STEP_COUNT       ABS_GAS
#define EVENT_TYPE_PROXIMITY        ABS_DISTANCE
#define EVENT_TYPE_LIGHT            ABS_MISC

// 720 LSG = 1G
#define LSG                          (32.0f/1.5f)

// conversion of acceleration data to SI units (m/s^2)
#define CONVERT_A                   (GRAVITY_EARTH / MAX_SENS_DEV)
#define CONVERT_A_X                 (-CONVERT_A)
#define CONVERT_A_Y                 (-CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

// conversion of magnetic data to uT units
#define CONVERT_M                   (0.1f)

#define CONVERT_O                   (1.0f)
#define CONVERT_O_Y                 (CONVERT_O)
#define CONVERT_O_P                 (CONVERT_O)
#define CONVERT_O_R                 (-CONVERT_O)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
