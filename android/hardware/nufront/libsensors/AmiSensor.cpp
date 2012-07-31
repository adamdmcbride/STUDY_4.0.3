/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include "ami30x.h" 
#include "AmiSensor.h"

#define  ACCELEROMETER_SENSOR_OPEN       AMIT_BIT_ACCELEROMETER
#define  MAGNETIC_FIELD_SENSOR_OPEN      AMIT_BIT_MAGNETIC_FIELD
#define  ORIENTATION_SENSOR_OPEN         AMIT_BIT_ORIENTATION
#define  GYROSCOPE_SENSOR_OPEN           AMIT_BIT_GYROSCOPE

/*****************************************************************************/

AmiSensor::AmiSensor()
: SensorBase(AMI_DEVICE_NAME, "ami30x_compass"),
      mEnabled(0),
      mPendingMask(0),
      mInputReader(32)
{
    int    err = 0; 
    int    control[AMI304_CB_LENGTH];
    char   buf[128];
    int    iYaw, iPitch, iRoll, iStatus;
    int    imx_raw, imy_raw, imz_raw, iax_raw, iay_raw, iaz_raw, istatus;
    
    memset(mPendingEvents, 0, sizeof(mPendingEvents));
	memset(control, 0, sizeof(control));
	memset(buf, 0, sizeof(buf));

    mPendingEvents[Accelerometer].version = sizeof(sensors_event_t);
    mPendingEvents[Accelerometer].sensor = ID_A;
    mPendingEvents[Accelerometer].type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvents[Accelerometer].acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[MagneticField].sensor = ID_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Orientation  ].version = sizeof(sensors_event_t);
    mPendingEvents[Orientation  ].sensor = ID_ORIENTATION;
    mPendingEvents[Orientation  ].type = SENSOR_TYPE_ORIENTATION;
    mPendingEvents[Orientation  ].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;

    ACCELEROMETER_Activate  =   0;
    MAGNETIC_FIELD_Activate =   0;    
    ORIENTATION_Activate    =   0;
    current_delay_time      =  AMI30x_DEFAULT_POLLING_TIME;
    iYaw = iPitch = iRoll = iStatus = 0;
    imx_raw = imy_raw = imz_raw = iax_raw = iay_raw = iaz_raw = 0;

	//AMI30x_DEFAULT_POLLING_TIME ms by default, but mDelays[i] are saved by unit -> ns
	for (int i=0 ; i<numSensors ; i++)
		mDelays[i] = current_delay_time * 1000000; 

    open_device();

    err = ioctl(dev_fd, AMI304HAL_IOCTL_GET_CONTROL, control);    
    if(err == -1)
    {
      LOGE("Read Control from ami-Sensor is error at %s , read cnt(%d), error no=(%s)\n", __FUNCTION__, err, strerror(errno));
    }
    else
	{
		if(control[AMI304_CB_ACTIVESENSORS] & ACCELEROMETER_SENSOR_OPEN)
		{       
			mEnabled |= 1<<Accelerometer;
			ACCELEROMETER_Activate++;    
			if (!ioctl(dev_fd, AMI304HAL_IOCTL_GET_CALIDATA, buf)) {
				sscanf(buf, "%d %d %d %d %d %d %d", &imx_raw, &imy_raw, &imz_raw, &iax_raw, &iay_raw, &iaz_raw, &iStatus);       
				mPendingEvents[Accelerometer].acceleration.x = iax_raw;
				mPendingEvents[Accelerometer].acceleration.y = iay_raw;
				mPendingEvents[Accelerometer].acceleration.z = iaz_raw;
			}
		}
		
		if(control[AMI304_CB_ACTIVESENSORS] & MAGNETIC_FIELD_SENSOR_OPEN)
		{    
			mEnabled |= 1<<MagneticField;
			MAGNETIC_FIELD_Activate++;
			if (!ioctl(dev_fd, AMI304HAL_IOCTL_GET_CALIDATA, buf)) {
				sscanf(buf, "%d %d %d %d %d %d %d", &imx_raw, &imy_raw, &imz_raw, &iax_raw, &iay_raw, &iaz_raw, &iStatus);                  
				mPendingEvents[MagneticField].magnetic.x = imx_raw;
				mPendingEvents[MagneticField].magnetic.y = imy_raw;
				mPendingEvents[MagneticField].magnetic.z = imz_raw;           
			}
		}
		
		if(control[AMI304_CB_ACTIVESENSORS] & ORIENTATION_SENSOR_OPEN)
		{
			mEnabled |= 1<<Orientation;
			ORIENTATION_Activate++;    
			if (!ioctl(dev_fd, AMI304HAL_IOCTL_GET_POSTURE, buf)) {
				sscanf(buf, "%d %d %d %d", &iYaw, &iPitch, &iRoll, &iStatus);
				mPendingEvents[Orientation].orientation.azimuth = iYaw;
				mPendingEvents[Orientation].orientation.pitch = iPitch;
				mPendingEvents[Orientation].orientation.roll = iRoll;
				mPendingEvents[Orientation].orientation.status = SetAccuracyStatus(Orientation, (int)(iStatus));
			}
		}     
	}//ioctl(dev_fd, AMI304HAL_IOCTL_GET_CONTROL, control)
 
    LOGI("AmiSensor::AmiSensor, mEnabled = %d\n", mEnabled);
    if (!mEnabled) {
        close_device();
    }
}

AmiSensor::~AmiSensor() {
}

int AmiSensor::enable(int32_t handle, int en)
{
    int what = -1;
    int control[AMI304_CB_LENGTH];
    
    LOGV("AmiSensor::enable()");
	
	memset(control, 0, sizeof(control));
	
    switch (handle) {
        case ID_A: 
            what = Accelerometer; 
            break;
        case ID_MAGNETIC_FIELD: 
            what = MagneticField; 
            break;
        case ID_ORIENTATION: 
            what = Orientation;   
            break;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    int newState  = en ? 1 : 0;
    int err = 0;

    LOGV("what = %d, mEnabled, en = %d , %d\n", what, mEnabled, en);
    if ((uint32_t(newState)<<what) != (mEnabled & (1<<what))) {
        LOGE("mEnabled = %d \n", mEnabled);
        if (!mEnabled) {
            open_device();
        }

        if(en)
        {               
            if(what == Orientation)
                ORIENTATION_Activate++;
            else if(what == MagneticField)
                MAGNETIC_FIELD_Activate++;
            else if(what == Accelerometer)
                ACCELEROMETER_Activate++;
        }
        else
        {       
            if(what == Orientation)
                ORIENTATION_Activate = 0;
            else if(what == MagneticField)
                MAGNETIC_FIELD_Activate = 0;
            else if(what == Accelerometer)
                ACCELEROMETER_Activate = 0;
        }          

        err = ioctl(dev_fd, AMI304HAL_IOCTL_GET_CONTROL, control);    
        if(err == -1)
        {
            LOGE("Read Control from ami-Sensor is error at %s , read cnt(%d), error no=(%s)\n", __FUNCTION__, err, strerror(errno));
        }
        else
        {
            if(ORIENTATION_Activate == 0)      
                control[AMI304_CB_ACTIVESENSORS] &= ~ORIENTATION_SENSOR_OPEN;
            else   
                control[AMI304_CB_ACTIVESENSORS] |= ORIENTATION_SENSOR_OPEN;    
            
            if(MAGNETIC_FIELD_Activate == 0)
                control[AMI304_CB_ACTIVESENSORS] &= ~MAGNETIC_FIELD_SENSOR_OPEN;
            else
                control[AMI304_CB_ACTIVESENSORS] |= MAGNETIC_FIELD_SENSOR_OPEN;
            
            if(ACCELEROMETER_Activate == 0)
                control[AMI304_CB_ACTIVESENSORS] &= ~ACCELEROMETER_SENSOR_OPEN;
            else
                control[AMI304_CB_ACTIVESENSORS] |= ACCELEROMETER_SENSOR_OPEN;
                                                   
            // if no sensor, set default delay-time
            if( control[AMI304_CB_ACTIVESENSORS] == 0 )
			{
				 //LOGE("AkmSensor::set delay to default !! \n");
				for (int i=0 ; i<numSensors ; i++)
			        mDelays[i] = AMI30x_DEFAULT_POLLING_TIME * 1000000;
                control[AMI304_CB_LOOPDELAY] = AMI30x_DEFAULT_POLLING_TIME;  //unit => ms
            }
			//LOGE("Now control[AMI304_CB_ACTIVESENSORS] = %d \n", control[AMI304_CB_ACTIVESENSORS]);
            //set active sensor to driver
            err = ioctl(dev_fd, AMI304HAL_IOCTL_SET_CONTROL, control);    
            if(err == -1)
            {
                LOGE("Set control to ami-Sensor is error at %s , read cnt(%d), error no=(%s)\n", __FUNCTION__, err, strerror(errno));
            }       
        }  //err = ioctl, AMI304HAL_IOCTL_GET_CONTROL
        
        if (!err) {
            mEnabled = control[AMI304_CB_ACTIVESENSORS];
            update_delay();
        }
        
        if (!mEnabled) {
            close_device();
        }
    }
    
    LOGV("AmiSensor::enable()-leave");
    return err;
}

int AmiSensor::setDelay(int32_t handle, int64_t ns)
{
    int what = -1;
    
    LOGV("AmiSensor::setDelay()");
    switch (handle) {
        case ID_A: 
            what = Accelerometer; 
            break;
        case ID_MAGNETIC_FIELD: 
            what = MagneticField; 
            break;
        case ID_ORIENTATION: 
            what = Orientation;   
            break;
    }

    if (uint32_t(what) >= numSensors)
        return -EINVAL;

    if (ns < 0)
        return -EINVAL;

    mDelays[what] = ns;    
    return update_delay();
}

int AmiSensor::update_delay()
{
    int current_DelayTime = 0, set_ms = 0;
    int res = 0;
    int control[AMI304_CB_LENGTH];

    LOGV("AmiSensor::update_delay() => mEnabled =%d\n", mEnabled);
    
    if (mEnabled) {
	
		memset(control, 0, sizeof(control));
        uint64_t wanted = -1LLU;//ns
        for (int i=0 ; i<numSensors ; i++) {
            if (mEnabled & (1<<i)) {
                uint64_t ns = mDelays[i];
                wanted = wanted < ns ? wanted : ns;
            }
        }
		
		if(-1LLU==wanted)
			return res;
		
        res = ioctl(dev_fd, AMI304HAL_IOCTL_GET_CONTROL, control);    
        if(res == -1)
        {
            LOGE("Read Control from ami-Sensor is error at %s , read cnt(%d), error no=(%s)\n", __FUNCTION__, res, strerror(errno));
        }
		else
		{
			set_ms = int64_t(wanted) / 1000000; //unit -> ms
			current_DelayTime = control[AMI304_CB_LOOPDELAY];

			if(current_DelayTime == 0)           //init set delay time
				current_DelayTime = set_ms;
			else if(current_DelayTime > set_ms)  //use the faster sampling rate
				current_DelayTime = set_ms;         
			if(current_DelayTime < 5)            //set max sampling rate = 200Hz
				current_DelayTime = 5;
			current_delay_time = control[AMI304_CB_LOOPDELAY] = current_DelayTime;  //unit => ms
			
			//set delay to driver
			res = ioctl(dev_fd, AMI304HAL_IOCTL_SET_CONTROL, control);    
			if(res == -1)
			   LOGE("Set delay to ami-Sensor is error, read cnt(%d), error no=(%s)\n", res, strerror(errno));
        }//ioctl(dev_fd, AMI304HAL_IOCTL_GET_CONTROL, control); 
        LOGV("AmiSensor::update_delay() = %d -leave", res);   
        return res;   
    }//if (mEnabled)
    
    LOGV("AmiSensor::update_delay()-leave");
    return 0;
}

void AmiSensor::Wait_Delay(int ms_delay)
{
    if(ms_delay <= 5)
        usleep(1000);
    else if(ms_delay <= 20)
        usleep(2000);
    else if(ms_delay <= 60)
        usleep(6000);
    else  //delay == 200
        usleep(10000);
}

int AmiSensor::readEvents(sensors_event_t* data, int count)
{   
    if (count < 1)
        return -EINVAL;

    LOGV("AmiSensor::readEvents()");

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            processEvent(event->code, event->value);
            mInputReader.next();
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {
                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvents[j].timestamp = time; /* time is in nanosecond */
                    if (mEnabled & (1<<j)) {
                        *data++ = mPendingEvents[j];
                        count--;
                        numEventReceived++;
                    }
                }
            }
            if (!mPendingMask) {
                mInputReader.next();
            }
            //Wait_Delay(current_delay_time);
        } else {
            LOGW("AmiSensor: unknown event (type=%d, code=%d)", type, event->code);
            mInputReader.next();
        }
    }

    LOGV("AmiSensor::readEvents()-leave");
    return numEventReceived;
}

void AmiSensor::processEvent(int code, int value)
{
    int status = 0;
    
    switch (code) {
		#if 1
        case EVENT_TYPE_ACCEL_X:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.x = ((float)value/(float)AMI304_POSTURE_ACCURACY_RATE) * CONVERT_A;
			//LOGE("EVENT_TYPE_ACCEL_X = %f\n", mPendingEvents[Accelerometer].acceleration.x); 
			break;
        case EVENT_TYPE_ACCEL_Y:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.y = ((float)value/(float)AMI304_POSTURE_ACCURACY_RATE) * CONVERT_A;
            //LOGE("EVENT_TYPE_ACCEL_Y = %f\n", mPendingEvents[Accelerometer].acceleration.y); 
            break;
        case EVENT_TYPE_ACCEL_Z:
            mPendingMask |= 1<<Accelerometer;
            mPendingEvents[Accelerometer].acceleration.z = ((float)value/(float)AMI304_POSTURE_ACCURACY_RATE) * CONVERT_A;
            //LOGE("EVENT_TYPE_ACCEL_Z = %f\n", mPendingEvents[Accelerometer].acceleration.z); 
            break;
		#endif
        case EVENT_TYPE_MAGV_X:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.x = ((float)value/(float)AMI304_POSTURE_ACCURACY_RATE) * CONVERT_M;
			//LOGE("EVENT_TYPE_MAGV_X = %f\n", mPendingEvents[MagneticField].magnetic.x);  
            break;
        case EVENT_TYPE_MAGV_Y:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.y = ((float)value/(float)AMI304_POSTURE_ACCURACY_RATE) * CONVERT_M;
			//LOGE("EVENT_TYPE_MAGV_Y = %f\n", mPendingEvents[MagneticField].magnetic.y); 
            break;
        case EVENT_TYPE_MAGV_Z:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.z = ((float)value/(float)AMI304_POSTURE_ACCURACY_RATE) * CONVERT_M;
			//LOGE("EVENT_TYPE_MAGV_Z = %f\n", mPendingEvents[MagneticField].magnetic.z);
            break;
        case EVENT_TYPE_MAGV_STATUS:
            mPendingMask |= 1<<MagneticField;
            status = SetAccuracyStatus(MagneticField, (int)(value));
            mPendingEvents[MagneticField].magnetic.status = status ; 
			//LOGE("EVENT_TYPE_MAGV_STATUS = %d\n", mPendingEvents[MagneticField].magnetic.status);
            break;            

        case EVENT_TYPE_YAW:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.azimuth = (float)value/(float)AMI304_POSTURE_ACCURACY_RATE;
			//LOGE("EVENT_TYPE_YAW = %f\n", mPendingEvents[Orientation].orientation.azimuth);
            break;
        case EVENT_TYPE_PITCH:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.pitch = (float)value/(float)AMI304_POSTURE_ACCURACY_RATE;
			//LOGE("EVENT_TYPE_PITCH = %f\n", mPendingEvents[Orientation].orientation.pitch);
            break;
        case EVENT_TYPE_ROLL:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.roll = (float)value/(float)AMI304_POSTURE_ACCURACY_RATE;
			//LOGE("EVENT_TYPE_ROLL = %f\n", mPendingEvents[Orientation].orientation.roll);
            break;
        case EVENT_TYPE_ORIENT_STATUS:
            mPendingMask |= 1<<Orientation;
            status = SetAccuracyStatus(Orientation, (int)(value));
            mPendingEvents[Orientation].orientation.status = status;
			//LOGE("EVENT_TYPE_ORIENT_STATUS = %d\n", mPendingEvents[Orientation].orientation.status);
            break;
    }
    
    LOGV("AmiSensor::processEvent()-leave");
}

int8_t AmiSensor::SetAccuracyStatus(int SensorType, int SensorStatus)
{
    int8_t android_status = SENSOR_STATUS_ACCURACY_HIGH;
    
    if ( (SensorType == Orientation) || (SensorType == MagneticField) )
    {
        switch (SensorStatus)
        {
            case 1: case 2:
                    android_status = SENSOR_STATUS_ACCURACY_HIGH;
                    break;
            case 3:
                    android_status = SENSOR_STATUS_ACCURACY_MEDIUM;
                    break;
            case 4:
                    android_status = SENSOR_STATUS_ACCURACY_LOW;
                    break;
            case 5:     
                    android_status = SENSOR_STATUS_UNRELIABLE;
                    break;  
        }
    }
        
    return android_status;
}
