/*
 * Copyright (C) 2009 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaExtractor"
#include <utils/nufrontlog.h>

#include "include/AMRExtractor.h"
#include "include/MP3Extractor.h"
#include "include/MPEG4Extractor.h"
#include "include/WAVExtractor.h"
#include "include/OggExtractor.h"
#include "include/MPEG2PSExtractor.h"
#include "include/MPEG2TSExtractor.h"
#include "include/DRMExtractor.h"
#include "include/WVMExtractor.h"
#include "include/FLACExtractor.h"
#include "include/AACExtractor.h"
/* a@nufront begin */
#if defined(USE_FFMPEG_EXTRACTOR)
#include "nufront/ffmpeg_extractor/androidSource.h"
#include "nufront/ffmpeg_extractor/FFmpegExtractor.h"
#warning   "defined USE_FFMPEG_EXTRACTOR"
#endif
/* a@nufront end */

#include "matroska/MatroskaExtractor.h"

#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>

namespace android {

sp<MetaData> MediaExtractor::getMetaData() {
    return new MetaData;
}

uint32_t MediaExtractor::flags() const {
    return CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_PAUSE | CAN_SEEK;
}

/* a@nufront begin */
#if defined(USE_FFMPEG_EXTRACTOR)
#warning "defined USE_FFMPEG_EXTRACTOR"
//static
bool MediaExtractor::isSystemMedia(const char* path) {
   using namespace FFMPEG;
   Regex reg("^/system/media/audio/");
   return reg.exec(path);
}
#endif
/* a@nufront end */

// static
sp<MediaExtractor> MediaExtractor::Create(
        const sp<DataSource> &source, const char *mime) {
    sp<AMessage> meta;
    LCHLOGD1("mime=%s, MIME=%s", mime, source->getMIMEType().string());
    String8 tmp;
    if (mime == NULL) {
        float confidence;
        if (!source->sniff(&tmp, &confidence, &meta)) {
            LOGV("FAILED to autodetect media content.");

            return NULL;
        }

        mime = tmp.string();
        LOGV("Autodetected media content as '%s' with confidence %.2f",
             mime, confidence);
    }

    bool isDrm = false;
    // DRM MIME type syntax is "drm+type+original" where
    // type is "es_based" or "container_based" and
    // original is the content's cleartext MIME type
    if (!strncmp(mime, "drm+", 4)) {
        const char *originalMime = strchr(mime+4, '+');
        if (originalMime == NULL) {
            // second + not found
            return NULL;
        }
        ++originalMime;
        if (!strncmp(mime, "drm+es_based+", 13)) {
            // DRMExtractor sets container metadata kKeyIsDRM to 1
            return new DRMExtractor(source, originalMime);
        } else if (!strncmp(mime, "drm+container_based+", 20)) {
            mime = originalMime;
            isDrm = true;
        } else {
            return NULL;
        }
    }

    MediaExtractor *ret = NULL;

/* a@nufront begin */
/*
    if (source->getUri() &&
       strlen(source->getUri()) &&
       !isSystemMedia(source->getUri())) {

        using namespace FFMPEG;
        ret = new FFmpegExtractor(source);
        LOGD("zx, create FFmpegExtractor.");
    }
    else {
*/
/* a@nufront end */
#if !defined(USE_FFMPEG_EXTRACTOR)
    ZXLOGD("not use FFmpegExtractor.");
#warning "not defined USE_FFMPEG_EXTRACTOR"
#endif
    LCHLOGD1("mime=%s, file=%s", mime, source->getUri().string());
    if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
            || !strcasecmp(mime, "audio/mp4")) {
        ret = new MPEG4Extractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)) {
        ret = new MP3Extractor(source, meta);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
            || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)) {
        ret = new AMRExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
        ret = new FLACExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
        ret = new WAVExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
        ret = new OggExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MATROSKA)) {
        ret = new MatroskaExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) {
        ret = new MPEG2TSExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WVM)) {
        ret = new WVMExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS)) {
        ret = new AACExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2PS)) {
        ret = new MPEG2PSExtractor(source);
    }
#if defined(USE_FFMPEG_EXTRACTOR)
#warning "defined USE_FFMPEG_EXTRACTOR"
    else {
        using namespace FFMPEG;
        ret = new FFmpegExtractor(source);
        ZXLOGD("use FFmpegExtractor.");
        /*a@nufront start: add open status check*/
        if (((FFmpegExtractor*)ret)->IsOpenFailed()) {
            delete ret;
            ret = NULL;
        }
        /*a@nufront end*/
    }
#endif
/* a@nufront begin */
/*
    else {
        using namespace FFMPEG;
        ret = new FFmpegExtractor(source);
    }
    }
*/
/* a@nufront end */

    if (ret != NULL) {
       if (isDrm) {
           ret->setDrmFlag(true);
       } else {
           ret->setDrmFlag(false);
       }
    }

    return ret;
}

}  // namespace android
