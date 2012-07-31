/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_CAMERA_EFFECT_H
#define ANDROID_HARDWARE_CAMERA_EFFECT_H

#include <sys/types.h>
#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


namespace android {

class Effect {

public:
  Effect();
  ~Effect();
    
  //static inline void processEffect(unsigned char *effect_rgb888,unsigned char r,unsigned char g,unsigned char b,int effectType);
  //the effect we support  
  static const int CAMERA_EFFECT_NONE = 0;
  static const int CAMERA_EFFECT_MONO = 1;
  static const int CAMERA_EFFECT_NEGATIVE = 2;
  static const int CAMERA_EFFECT_SOLARIZE = 3;
  static const int CAMERA_EFFECT_SEPIA = 4;
  static const int CAMERA_EFFECT_AQUA = 5;
  
  static inline void processEffectPerPixel(unsigned char *r,unsigned char *g,unsigned char *b,int effectType)
  {
    int gray;
    //make sure r,g,b is between 0 and 255
    /*
    (*r) = ((*r) > 255) ? 255 : (((*r) < 0) ? 0 : (*r));
    (*g) = ((*g) > 255) ? 255 : (((*g) < 0) ? 0 : (*g));
    (*b) = ((*b) > 255) ? 255 : (((*b) < 0) ? 0 : (*b));  
    */
    if(effectType != Effect::CAMERA_EFFECT_NONE){
    //effect process
        switch(effectType){
            case Effect::CAMERA_EFFECT_NEGATIVE:
                //negative
                (*r) = ~(*r);
                (*g) = ~(*g);
                (*b) = ~(*b);  
                break;
            case Effect::CAMERA_EFFECT_MONO:
                //mono
                gray = 0;
                //get max of r,g,b and then set the max value to gray
                gray = (*r) > (*g) ? (*r) : (*g);
                gray = gray < (*b) ? (*b) : gray;
                (*r) = gray;
                (*g) = gray;
                (*b) = gray;  
                break;
            case Effect::CAMERA_EFFECT_SOLARIZE:
                (*r) = (*r) < 128 ? ~(*r) : (*r);
                (*g) = (*g) < 128 ? ~(*g) : (*g);
                (*b) = (*b) < 128 ? ~(*b) : (*b); 
                break;
            case Effect::CAMERA_EFFECT_AQUA:
                gray = 0.3 * (*r) + 0.59 * (*g) + 0.11 * (*b);
                gray = (gray > 255) ? 255 : ((gray < 0) ? 0 : gray);
                (*r) = 0.50 * gray;//red:127
                (*g) = gray;
                (*b) = 0.83 * gray;//blue:212
                break;
            case Effect::CAMERA_EFFECT_SEPIA:
                gray = 0.3 * (*r) + 0.59 * (*g) + 0.11 * (*b);
                gray = (gray > 255) ? 255 : ((gray < 0) ? 0 : gray);
                (*r) = 0.37 * gray;//red:94
                (*g) = 0.15 * gray;//38
                (*b) = 0.07 * gray;//blue:18
            default:
            //other effect, do nothing now.
                break;
        }//end switch
    }//end if effectType
  }

}; // Class Effect 
}; // namespace android

#endif // ANDROID_HARDWARE_FAKECAMERA_H
