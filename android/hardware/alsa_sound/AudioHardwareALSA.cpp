/* AudioHardwareALSA.cpp
 **
 ** Copyright 2008-2010 Wind River Systems
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

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define LOG_TAG "AudioHardwareALSA"
#include <utils/Log.h>
#include <utils/String8.h>
/*a@nufront start: add for nufront log*/
#include <utils/nufrontlog.h>
/*a@nufront end*/

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"
/*m@nufront start: audio hal for alsa merge*/
#include <hardware_legacy/AudioHardwareInterface.h>
extern "C"
{
    //
    // Function for dlsym() to look up for creating a new AudioHardwareInterface.
    //
    /*
    android::AudioHardwareInterface *createAudioHardware(void) {
        return android::AudioHardwareALSA::create();
    }
    */
    android_audio_legacy::AudioHardwareInterface * createAudioHardware(void) {
        return android_audio_legacy::AudioHardwareALSA::create();
    }
    /*m@nufront end*/

}         // extern "C"

/*m@nufront start: audio hal for alsa merge*/
/*namespace android*/
namespace android_audio_legacy
/*m@nufront end*/
{

// ----------------------------------------------------------------------------

static void ALSAErrorHandler(const char *file,
                             int line,
                             const char *function,
                             int err,
                             const char *fmt,
                             ...)
{
    char buf[BUFSIZ];
    va_list arg;
    int l;

    va_start(arg, fmt);
    l = snprintf(buf, BUFSIZ, "%s:%i:(%s) ", file, line, function);
    vsnprintf(buf + l, BUFSIZ - l, fmt, arg);
    buf[BUFSIZ-1] = '\0';
    LOG(LOG_ERROR, "ALSALib", "%s", buf);
    va_end(arg);
}

AudioHardwareInterface *AudioHardwareALSA::create() {
    return new AudioHardwareALSA();
}

AudioHardwareALSA::AudioHardwareALSA() :
    mALSADevice(0),
    mAcousticDevice(0),
    mSpeakerSwitch(1),
    mHPSwitch(0),
    mHDMIAudioSwitch(0)
{
    snd_lib_error_set_handler(&ALSAErrorHandler);
    mMixer = new ALSAMixer;

    mControl = new ALSAControl("default");

    hw_module_t *module;
    int err = hw_get_module(ALSA_HARDWARE_MODULE_ID,
            (hw_module_t const**)&module);

    if (err == 0) {
        hw_device_t* device;
        err = module->methods->open(module, ALSA_HARDWARE_NAME, &device);
        if (err == 0) {
            mALSADevice = (alsa_device_t *)device;
            mALSADevice->init(mALSADevice, mDeviceList);
        } else
            LOGE("ALSA Module could not be opened!!!");
    } else
        LOGE("ALSA Module not found!!!");

    err = hw_get_module(ACOUSTICS_HARDWARE_MODULE_ID,
            (hw_module_t const**)&module);

    if (err == 0) {
        hw_device_t* device;
        err = module->methods->open(module, ACOUSTICS_HARDWARE_NAME, &device);
        if (err == 0)
            mAcousticDevice = (acoustic_device_t *)device;
        else
            LOGE("Acoustics Module not found.");
    }
}

AudioHardwareALSA::~AudioHardwareALSA()
{
    if (mControl) delete mControl;
    if (mMixer) delete mMixer;
    if (mALSADevice)
        mALSADevice->common.close(&mALSADevice->common);
    if (mAcousticDevice)
        mAcousticDevice->common.close(&mAcousticDevice->common);
}

status_t AudioHardwareALSA::initCheck()
{
    if (!mALSADevice)
        return NO_INIT;

    if (!mMixer || !mMixer->isValid())
        LOGW("ALSA Mixer is not valid. AudioFlinger will do software volume control.");

    if (!mControl || (initControl() != NO_ERROR)) {
        ZJFLOGE("ALSA Ctrl is not valid. AudioFlinger will do software volume control.");
    }
#if 0
    if (mControl) {
        unsigned int tmpVal, &val = tmpVal ;
        setSpeakerSwitch(0);
        setHpSwitch(0);
        getHpSwitch(val);
        ZJFLOGD("val is %u", val);
    }
#endif

    return NO_ERROR;
}

status_t AudioHardwareALSA::setVoiceVolume(float volume)
{
    // The voice volume is used by the VOICE_CALL audio stream.
    /*modify for audio codec*/
    //if (mOutStream) {
        // the mOutStream will set the volume of current device
      //  return mOutStream->setVolume(volume, volume);
    if (mMixer) {
        return mMixer->setVolume(AudioSystem::DEVICE_OUT_EARPIECE, volume, volume);
     /*end modify*/
    } else {
        // return error
        return INVALID_OPERATION;
    }
}

status_t AudioHardwareALSA::setMasterVolume(float volume)
{
    if (mMixer)
        return mMixer->setMasterVolume(volume);
    else
        return INVALID_OPERATION;
}
/********************************************/
status_t AudioHardwareALSA::setHpSwitch(unsigned int val)
{
    ZJFLOGD("function in, value is %d.", val);
    if (mControl) {
        return mControl->set("HP Playback Switch", val % 2, -1);
    } else {
        return INVALID_OPERATION;
    }
}

status_t AudioHardwareALSA::getHpSwitch(unsigned int &val)
{
    ZJFLOGD("function in");
    if (mControl)
        return mControl->get("HP Playback Switch", val, -1);
    else
        return INVALID_OPERATION;
}

status_t AudioHardwareALSA::setSpeakerSwitch(unsigned int val)
{
    ZJFLOGD("function in, val is %d", val);
    if (mControl) {
        return mControl->set("Speaker Playback Switch", val % 2, -1);
    } else {
        return INVALID_OPERATION;
    }
}

status_t AudioHardwareALSA::getSpeakerSwitch(unsigned int &val)
{
    if (mControl)
        return mControl->get("Speaker Playback Switch", val, -1);
    else
        return INVALID_OPERATION;
}

status_t AudioHardwareALSA::setHDMISwitch(unsigned int val)
{
    ZJFLOGD("function in, val is %d", val);
    if (mControl) {
        mHDMIAudioSwitch = val % 2;
        return mControl->set("Speaker Playback Switch", val % 2, -1);
    } else {
        return INVALID_OPERATION;
    }
}

status_t AudioHardwareALSA::getHDMISwitch(unsigned int &val)
{
    if (mControl)
        return mControl->get("Speaker Playback Switch", val, -1);
    else
        return INVALID_OPERATION;
}

status_t AudioHardwareALSA::setParameters(const String8& keyValuePairs) {
    ZJFLOGD("function in.");
    unsigned int tmpval, &val=tmpval;
    getHpSwitch(val);
    ZJFLOGD("after getHpSwitch, val is %u", val);
    getSpeakerSwitch(val);
    ZJFLOGD("after getSpeakerSwitch, val is %u", val);
    AudioParameter param = AudioParameter(keyValuePairs);
    String8 keyHP = String8(AudioParameter::keyHeadphonePlugin);
    String8 keyHDMI = String8(AudioParameter::keyHDMIAudioPlugin);
    status_t status = NO_ERROR;
    int headphoneAction;
    int hdmiAudioAction;
    LOGV("setParameters() %s", keyValuePairs.string());

    if (param.getInt(keyHP, headphoneAction) == NO_ERROR) {
        ZJFLOGD("get headphonePlugin parameters");
        Mutex::Autolock autoLock(mLock);
        if (1 == headphoneAction)
        {
            if (mControl) {
                mSpeakerSwitch = 0;
                mHPSwitch = 1;
                if (!mHDMIAudioSwitch) {
                    setSpeakerSwitch(mSpeakerSwitch);
                    setHpSwitch(mHPSwitch);
                }
            }
        } else if(0 == headphoneAction){
            if (mControl) {
                mSpeakerSwitch = 1;
                mHPSwitch = 0;
                if (!mHDMIAudioSwitch) {
                    setSpeakerSwitch(mSpeakerSwitch);
                    setHpSwitch(mHPSwitch);
                }
            }

        }
        if (param.size()) {
            status = BAD_VALUE;
        }
        ZJFLOGD("function out.");
        return status;
    } else if (param.getInt(keyHDMI, hdmiAudioAction) == NO_ERROR) {
        ZJFLOGD("get hdmiaudio parameters");
        Mutex::Autolock autoLock(mLock);
        if (1 == hdmiAudioAction)
        {
            mHDMIAudioSwitch = 1;
            if (mControl) {
                setSpeakerSwitch(0);
                setHpSwitch(0);
            }
        } else if(0 == hdmiAudioAction){
            mHDMIAudioSwitch = 0;
            if (mControl) {
                setHpSwitch(mHPSwitch);
                setSpeakerSwitch(mSpeakerSwitch);
            }

        }
        if (param.size()) {
            status = BAD_VALUE;
        }
        ZJFLOGD("function out.");
        return status;
    }else {
        status = BAD_INDEX;
        return status;
    }
}
/*a@nufront start*/
status_t AudioHardwareALSA::initControl() {
    ZJFLOGD("function in.");
    status_t status = NO_ERROR;
    FILE * fh, *fh1;
    fh = fopen("/sys/class/switch/h2w/state", "r");
    fh1 = fopen("/sys/class/switch/hmdi/state", "r");
    {
        Mutex::Autolock autoLock(mLock);
        if (fh != NULL) {
            fscanf(fh, "%u", &mHPSwitch);
            fclose(fh);
            mSpeakerSwitch = (mHPSwitch + 1) % 2;
            ZJFLOGD("mHPSwitch is %u, mSpeakerSwitch is %u", mHPSwitch, mSpeakerSwitch);
        } else {
            ZJFLOGE("open /sys/class/switch/h2w/state error");
            return INVALID_OPERATION;
        }

        if (fh1 != NULL) {
            fscanf(fh1, "%u", &mHDMIAudioSwitch);
            fclose(fh1);
            ZJFLOGD("mHDMIAudioSwitch is %u", mHDMIAudioSwitch);
        } else {
            ZJFLOGE("open /sys/class/switch/hdmi/state error");
            return INVALID_OPERATION;
        }

        if (mHDMIAudioSwitch) {
            status = setHDMISwitch(mHDMIAudioSwitch);
            if (NO_ERROR != status) {
                ZJFLOGE("function out");
                return status;
            }
            status = setHpSwitch(0);
            if (NO_ERROR != status) {
                ZJFLOGE("function out");
                return status;
            }
            status = setSpeakerSwitch(0);
            if (NO_ERROR != status) {
                ZJFLOGE("function out");
                return status;
            }
        } else {
            status = setHpSwitch(mHPSwitch);
            if (NO_ERROR != status) {
                ZJFLOGE("function out");
                return status;
            }
            status = setSpeakerSwitch(mSpeakerSwitch);
            if (NO_ERROR != status) {
                ZJFLOGE("function out");
                return status;
            }
        }
    }
    ZJFLOGD("function out.");
    return status;
}
/*a@nufront end*/
/********************************************/
status_t AudioHardwareALSA::setMode(int mode)
{
    status_t status = NO_ERROR;

    if (mode != mMode) {
        status = AudioHardwareBase::setMode(mode);

        if (status == NO_ERROR) {
            // take care of mode change.
            for(ALSAHandleList::iterator it = mDeviceList.begin();
                it != mDeviceList.end(); ++it)
                if (it->curDev) {
                    status = mALSADevice->route(&(*it), it->curDev, mode);
                    if (status != NO_ERROR)
                        break;
                }
        }
    }

    return status;
}

AudioStreamOut *
AudioHardwareALSA::openOutputStream(uint32_t devices,
                                    int *format,
                                    uint32_t *channels,
                                    uint32_t *sampleRate,
                                    status_t *status)
{
    LOGD("openOutputStream called for devices: 0x%08x", devices);

    status_t err = BAD_VALUE;
    AudioStreamOutALSA *out = 0;

    if (devices & (devices - 1)) {
        if (status) *status = err;
        LOGD("openOutputStream called with bad devices");
        return out;
    }

    // Find the appropriate alsa device
    for(ALSAHandleList::iterator it = mDeviceList.begin();
        it != mDeviceList.end(); ++it)
        if (it->devices & devices) {
            err = mALSADevice->open(&(*it), devices, mode());
            if (err) break;
            out = new AudioStreamOutALSA(this, &(*it));
            err = out->set(format, channels, sampleRate);
            break;
        }
    if (status) *status = err;
    return out;
}

void
AudioHardwareALSA::closeOutputStream(AudioStreamOut* out)
{
    delete out;
}

AudioStreamIn *
AudioHardwareALSA::openInputStream(uint32_t devices,
                                   int *format,
                                   uint32_t *channels,
                                   uint32_t *sampleRate,
                                   status_t *status,
                                   AudioSystem::audio_in_acoustics acoustics)
{
    ZJFLOGD("function in.");
    ZJFLOGD("channels is %u, sampleRate is %u.", *channels, *sampleRate);
    status_t err = BAD_VALUE;
    AudioStreamInALSA *in = 0;

    if (devices & (devices - 1)) {
        if (status) *status = err;
        return in;
    }

    // Find the appropriate alsa device
    for(ALSAHandleList::iterator it = mDeviceList.begin();
        it != mDeviceList.end(); ++it)
        if (it->devices & devices) {
        /*a@nufront start*/
            it->sampleRate = *sampleRate;
            it->bufferSize = it->sampleRate / 5; /*set desire sample num according _defauleOut parameters*/
        /*a@nufront end*/
            err = mALSADevice->open(&(*it), devices, mode());
            if (err) break;
            in = new AudioStreamInALSA(this, &(*it), acoustics);
            err = in->set(format, channels, sampleRate);
            break;
        }

    if (status) *status = err;
    return in;
}

void
AudioHardwareALSA::closeInputStream(AudioStreamIn* in)
{
    delete in;
}

status_t AudioHardwareALSA::setMicMute(bool state)
{
    if (mMixer)
        return mMixer->setCaptureMuteState(AudioSystem::DEVICE_OUT_EARPIECE, state);

    return NO_INIT;
}

status_t AudioHardwareALSA::getMicMute(bool *state)
{
    if (mMixer)
        return mMixer->getCaptureMuteState(AudioSystem::DEVICE_OUT_EARPIECE, state);

    return NO_ERROR;
}

status_t AudioHardwareALSA::dump(int fd, const Vector<String16>& args)
{
    return NO_ERROR;
}
/*a@nufront start: add to imply interface of getInputBUfferSize*/
#define AUDIO_HW_IN_PERIOD_SZ 2048
size_t AudioHardwareALSA::getBufferSize(uint32_t sampleRate, int channelCount)
{
    ZJFLOGD("function in.");
    size_t ratio;

    switch (sampleRate) {
    case 8000:
    case 11025:
        ratio = 4;
        break;
    case 16000:
    case 22050:
        ratio = 2;
        break;
    case 44100:
    case 48000:
    default:
        ratio = 1;
        break;
    }

    ZJFLOGD("function out.");
    return (AUDIO_HW_IN_PERIOD_SZ*channelCount*sizeof(int16_t)) / ratio ;
}
/*a@nufront end*/

/*a@nufront start: imply interface for alsa record*/
size_t AudioHardwareALSA::getInputBufferSize(uint32_t sampleRate, int format, int channelCount)
{
    ZJFLOGD("function in, sampleRate is %u.", sampleRate);
    if (format != AudioSystem::PCM_16_BIT) {
        LOGW("getInputBufferSize bad format: %d", format);
        return 0;
    }
    if (channelCount < 1 || channelCount > 2) {
        LOGW("getInputBufferSize bad channel count: %d", channelCount);
        return 0;
    }
    if (sampleRate != 8000 && sampleRate != 11025 && sampleRate != 16000 &&
            sampleRate != 22050 && sampleRate != 44100 && sampleRate != 48000) {
        LOGW("getInputBufferSize bad sample rate: %d", sampleRate);
        return 0;
    }

    ZJFLOGD("function out.");
    return getBufferSize(sampleRate, channelCount);
}
/*a@nufront end*/

}       // namespace android
