//
//  Copyright (C) 2012 Nufront Microsystems
//
//  File: androidSource.c
//  $Id: androidSource.c,v 1.1 2012/4/30 00:47:56 liuchao Exp $

/// @file androidSource.c
/// live stream implement

extern "C" {
#include "libavutil/opt.h"
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavformat/httpauth.h"
#include "libavformat/url.h"
}

#include <unistd.h>
#include <media/stagefright/DataSource.h>
#include "androidSource.h"


using namespace android;

#if CONFIG_ANDROIDSOURCE_PROTOCOL

const char androidSource::SOURCENAME[32] = "androidsource";

Mutex androidSource::mLock;

int androidSource::android_source_open(URLContext *h, const char* filename, int flags)
{
    Mutex::Autolock lock(mLock);
    LCHLOGD1("filename=%s", filename);
    
    if(strncmp(filename,"androidsource://", 16))
    {
        LCHLOGD1("unsupported format, url=%s", filename);
    }

    DataSource* source = (DataSource*) atoi(&filename[16]);

    if(source)
    {
        androidSourceContext* s = (androidSourceContext* ) av_mallocz(sizeof(androidSourceContext));

        if(s)
        {
            off64_t lengh = 0x0;
            source->getSize(&lengh);
            
            h->priv_data = (void* ) s;
            s->source = source;
            s->length = lengh;
            s->url = (char* ) source->getUri().string();

            return 0x0;
        }            
    }

    return -1;
}

int androidSource::android_source_close(URLContext *h)
{
    Mutex::Autolock lock(mLock);
    LCHLOGD1("");
    
    av_free(h->priv_data);

    return 0;
}

int androidSource::android_source_get_handle(URLContext *h)
{
    Mutex::Autolock lock(mLock);
    LCHLOGD1("");
    androidSourceContext* s = (androidSourceContext* ) h->priv_data;
    return ((int ) s->source);
}

int64_t androidSource::android_source_seek(URLContext *h, int64_t pos, int whence)
{
    Mutex::Autolock lock(mLock);
    androidSourceContext* s = (androidSourceContext* ) h->priv_data;
    LCHLOGV1("pos=%lld, whence=%d", pos, whence);

    if(SEEK_SET == whence)
    {
        s->offset = pos;
    }
    else if(SEEK_CUR == whence)
    {
        s->offset += pos;
    }
    else if(SEEK_END == whence)
    {
        if(!s->length)
        {
            LCHLOGE("unsupported seek, pos=%lld", pos);
            return -1;
        }
        
        s->offset -= pos;
    }
    
    return 0x0;
}

int androidSource::android_source_read(URLContext *h, unsigned char *buf, int size)
{
    Mutex::Autolock lock(mLock);
    androidSourceContext* s = (androidSourceContext* ) h->priv_data;

    int nBytes = (int ) s->source->readAt(s->offset, (void*) buf, (size_t) size);
    s->offset += nBytes;
    LCHLOGV1("read %d bytes, get %d bytes", size, nBytes);

    if(nBytes != size)
    {
        LCHLOGD1("read %d bytes, get %d bytes", size, nBytes);
    }

    return nBytes;
}

bool androidSource::RegisterAndroidSource()
{
    LCHLOGD1("");
    Mutex::Autolock lock(mLock);
    
    static bool bInitialized = false;

    if(!bInitialized)
    {
        static URLProtocol protocol;

        memset((void*) &protocol, 0x0, sizeof(URLProtocol));

        protocol.name = SOURCENAME;
        protocol.url_open = android_source_open;
        protocol.url_read = android_source_read;
        protocol.url_write = NULL;
        protocol.url_seek = android_source_seek;
        protocol.url_close = android_source_close;
        protocol.url_get_file_handle = android_source_get_handle;
        protocol.priv_data_size = sizeof(androidSourceContext);
        protocol.flags = URL_PROTOCOL_FLAG_NETWORK;

        if(0x0 == ffurl_register_protocol(&protocol, sizeof(URLProtocol)))
        {
            bInitialized = true;
        }
    }

    return true;
}

bool androidSource::IsFFMPEG()
{
   // return true;
   return false;
}

const char* androidSource::GetUrl(void* source)
{
    static char url[256];

    if(androidSource::IsFFMPEG())
    {
        char temp[33];

        snprintf(temp, 32, "%d", (unsigned int) source);
        sprintf(url, "androidsource://%s", temp);
        LCHLOGV1("url = %s", url);
        
        return ((const char* ) url);
    }

    return NULL;
}

#endif //CONFIG_ANDROIDSOURCE_PROTOCOL

