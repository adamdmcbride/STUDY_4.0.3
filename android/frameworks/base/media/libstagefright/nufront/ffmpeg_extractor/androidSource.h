//
//  Copyright (C) 2012 Nufront Microsystems
//
//  File: androidSource.h
//  $Id: androidSource.h,v 1.1 2012/4/30 00:47:56 liuchao Exp $

/// @file androidSource.h
/// live stream implement

#ifndef __ANDROID_SOURCE_H__
#define __ANDROID_SOURCE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/threads.h>
#include <utils/nufrontlog.h>
#include <media/stagefright/DataSource.h>

extern "C" {
#include "libavformat/url.h"
}

#define CONFIG_ANDROIDSOURCE_PROTOCOL   1

namespace android 
{
class androidSource
{
public:
    static bool RegisterAndroidSource();

    static bool IsFFMPEG();

    static const char* GetUrl(void* source);

private:

    struct androidSourceContext
    {
        DataSource* source;
        char* url;
        off64_t offset;
        off64_t length;
        off64_t pos;
    };

    static const char SOURCENAME[32];

    static Mutex mLock;
    
    static int android_source_open(URLContext *h, const char *filename, int flags);

    static int android_source_close(URLContext *h);

    static int android_source_get_handle(URLContext *h);

    static int64_t android_source_seek(URLContext *h, int64_t pos, int whence);

    static int android_source_read(URLContext *h, unsigned char *buf, int size);
};
};

#endif  //__ANDROID_SOURCE_H__

