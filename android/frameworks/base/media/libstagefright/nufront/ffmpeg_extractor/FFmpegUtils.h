//
//  Copyright (C) 2012 Nufront Microsystems
//
//  File: FFmpegUtils.h
//  $Id: FFmpegUtils.h,v 1.1 2012/4/30 00:47:56 liuchao Exp $

/// @file FFmpegUtils.h
/// provide FFMPEG API to OMXCodec because ffmpeg decoder need some internal info

#ifndef __ANDROID_FFMPEG_UTILS__
#define __ANDROID_FFMPEG_UTILS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/threads.h>
#include <utils/nufrontlog.h>
#include <OMX_Audio.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include "libavformat/url.h"
}

#define USE_NUAUDIO_DECODER

namespace android
{

struct MimeCodecTable
{
    const char* mime;
    enum CodecID codec;
};

class FFmpegUtils
{
public:
    static MimeCodecTable* GetMimeCodecTable();

    static int GetCodecTableSize();

    static void SetAudioProfile(OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE* profile, const char* mime);

    static void DumpAudioProfile(OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE* profile);

    static void RegisterCodec(char* mime = NULL);

private:

    static const MimeCodecTable mime_codec_table[];
};
};

#endif  //__ANDROID_FFMPEG_UTILS__

