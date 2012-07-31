/* ALSAControl.cpp
 **
 ** Copyright 2008-2009 Wind River Systems
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

#define LOG_TAG "ALSAControl"
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"
/*m@nufront start: merge audio hal for alsa*/
/*namespace android*/
namespace android_audio_legacy
/*a@nufront end*/
{

ALSAControl::ALSAControl(const char *device)
{
    ZJFLOGD("function in.");
    int err;
    if ((err = snd_ctl_open(&mHandle, device, 0)) < 0) {
        ZJFLOGE("open ctl device error");;
    }
    ZJFLOGD("function out");
}

ALSAControl::~ALSAControl()
{
    ZJFLOGD("function in.");
    if (mHandle) snd_ctl_close(mHandle);
    ZJFLOGD("function out");
}

status_t ALSAControl::get(const char *name, unsigned int &value, int index)
{
    ZJFLOGD("function in.");
    if (!mHandle) {
        LOGE("Control not initialized");
        return NO_INIT;
    }

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *control;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&control);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, name);
    snd_ctl_elem_info_set_id(info, id);

    int ret = snd_ctl_elem_info(mHandle, info);
    if (ret < 0) {
        LOGE("Control '%s' cannot get element info: %d", name, ret);
        return BAD_VALUE;
    }

    int count = snd_ctl_elem_info_get_count(info);
    if (index >= count) {
        LOGE("Control '%s' index is out of range (%d >= %d)", name, index, count);
        return BAD_VALUE;
    }

    snd_ctl_elem_info_get_id(info, id);
    snd_ctl_elem_value_set_id(control, id);

    ret = snd_ctl_elem_read(mHandle, control);
    if (ret < 0) {
        LOGE("Control '%s' cannot read element value: %d", name, ret);
        return BAD_VALUE;
    }

    snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(info);
    switch (type) {
        case SND_CTL_ELEM_TYPE_BOOLEAN:
            value = snd_ctl_elem_value_get_boolean(control, index);
            break;
        case SND_CTL_ELEM_TYPE_INTEGER:
            value = snd_ctl_elem_value_get_integer(control, index);
            break;
        case SND_CTL_ELEM_TYPE_INTEGER64:
            value = snd_ctl_elem_value_get_integer64(control, index);
            break;
        case SND_CTL_ELEM_TYPE_ENUMERATED:
            value = snd_ctl_elem_value_get_enumerated(control, index);
            break;
        case SND_CTL_ELEM_TYPE_BYTES:
            value = snd_ctl_elem_value_get_byte(control, index);
            break;
        default:
            ZJFLOGE("bad value for get");
            return BAD_VALUE;
    }

    ZJFLOGD("function out, val is %u.", value);
    return NO_ERROR;
}

status_t ALSAControl::set(const char *name, unsigned int value, int index)
{
    ZJFLOGD("function in.");
    if (!mHandle) {
        LOGE("Control not initialized");
        return NO_INIT;
    }

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, name);
    snd_ctl_elem_info_set_id(info, id);

    int ret = snd_ctl_elem_info(mHandle, info);
    if (ret < 0) {
        LOGE("Control '%s' cannot get element info: %d", name, ret);
        return BAD_VALUE;
    }

    int count = snd_ctl_elem_info_get_count(info);
    if (index >= count) {
        LOGE("Control '%s' index is out of range (%d >= %d)", name, index, count);
        return BAD_VALUE;
    }

    if (index == -1)
        index = 0; // Range over all of them
    else
        count = index + 1; // Just do the one specified

    snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(info);

    snd_ctl_elem_value_t *control;
    snd_ctl_elem_value_alloca(&control);

    snd_ctl_elem_info_get_id(info, id);
    snd_ctl_elem_value_set_id(control, id);

    for (int i = index; i < count; i++)
        switch (type) {
            case SND_CTL_ELEM_TYPE_BOOLEAN:
                snd_ctl_elem_value_set_boolean(control, i, value);
                break;
            case SND_CTL_ELEM_TYPE_INTEGER:
                snd_ctl_elem_value_set_integer(control, i, value);
                break;
            case SND_CTL_ELEM_TYPE_INTEGER64:
                snd_ctl_elem_value_set_integer64(control, i, value);
                break;
            case SND_CTL_ELEM_TYPE_ENUMERATED:
                snd_ctl_elem_value_set_enumerated(control, i, value);
                break;
            case SND_CTL_ELEM_TYPE_BYTES:
                snd_ctl_elem_value_set_byte(control, i, value);
                break;
            default:
                break;
        }

    ret = snd_ctl_elem_write(mHandle, control);
    ZJFLOGD("function out");
    return (ret < 0) ? BAD_VALUE : NO_ERROR;
}

status_t ALSAControl::set(const char *name, const char *value)
{
    ZJFLOGD("funtion in.");
    if (!mHandle) {
        LOGE("Control not initialized");
        return NO_INIT;
    }

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, name);
    snd_ctl_elem_info_set_id(info, id);

    int ret = snd_ctl_elem_info(mHandle, info);
    if (ret < 0) {
        LOGE("Control '%s' cannot get element info: %d", name, ret);
        return BAD_VALUE;
    }

    int items = snd_ctl_elem_info_get_items(info);
    for (int i = 0; i < items; i++) {
        snd_ctl_elem_info_set_item(info, i);
        ret = snd_ctl_elem_info(mHandle, info);
        if (ret < 0) continue;
        if (strcmp(value, snd_ctl_elem_info_get_item_name(info)) == 0) {
            ZJFLOGD("function for set");
            return set(name, i, -1);
        }
    }

    LOGE("Control '%s' has no enumerated value of '%s'", name, value);

    ZJFLOGE("funtion out for BAD_VALUE.");
    return BAD_VALUE;
}
};        // namespace android
