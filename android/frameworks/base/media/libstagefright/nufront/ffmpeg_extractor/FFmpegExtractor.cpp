#include <dlfcn.h> /* For dynamic loading */
#include "FFmpegExtractor.h"
#define LOG_TAG "FFmpegExtractor"
#include <utils/nufrontlog.h>
#include "androidSource.h"
#include "FFmpegUtils.h"

extern "C" {
    #include "vpudecinfo.h"
}

#define INVALID_TIMESTAMP 0x7fffffffffffffff

/* a@nufront start: The following are just from pratical result not theoretical */
const int MAX_WAV_PACKET_LENGTH   = 32768;
const int MAX_MP3_PACKET_LENGTH   = 32768;     /* It's 8192 in opencore */
const int MAX_AMR_PACKET_LENGTH   = 5120;
const int MAX_DEF_PACKET_LENGTH   = 409600 * 3;    /* It's default for other formats except for WAV,MP3 and AMR */
const int MAX_BUFFER_SIZE_APPEND  = 20;     /** this const defines for max_buffer_size append extra data */
/* a@nufront end */

namespace FFMPEG {
/******************************************/

using namespace android;

/******************************************/
// FFmpegExtractor implement's area;
FFmpegExtractor::FFmpegExtractor(const sp<DataSource>& source) :
    mDataSource(source),
    mFirstTrack(NULL),
    mLastTrack(NULL),
    ffmpeg(NULL),
    ffmpegReader(NULL),
    mHaveMetadata(false),
    mHasVideo(false),
    mFileMetaData(new MetaData),
    mOpenFailed(false)
    {
    ZXLOGD("function in.");

    const char* filename = source->getFilePath();
    ZXLOGD("logd_file: source->getFilePath() ==>%s)", filename);

    if(androidSource::IsFFMPEG() && (!strncmp(filename,"http://", 5)))
    {
        filename = androidSource::GetUrl((void* ) source.get());
    }

    ffmpegReader = FFmpegSafeReader::create(filename);
    ffmpeg = ffmpegReader->getffmpeg();

    if (ffmpegReader->IsOpenFailed()) {
        mOpenFailed = true;
    } else {
        mOpenFailed = false;
    }

    ZXLOGD("function out.");
}

FFmpegExtractor::~FFmpegExtractor() {
    ZXLOGD("function in.");
    Track *track = mFirstTrack;
    while (track) {
        Track *next = track->next;

        delete track;
        track = next;
    }
    mFirstTrack = mLastTrack = NULL;

    if (ffmpegReader) {
        logd_ref("logd_ref, ~FFmpegExtractor(). ffmpegReader->release_ref():%d", __LINE__);
        ffmpegReader->release_ref();
        ffmpegReader = NULL;
    }
    ZXLOGD("funciton out");
}

size_t FFmpegExtractor::countTracks() {
    ZXLOGD("function in.");
    status_t err;
    if ((err = readMetaData()) != OK) {
        return 0;
    }

    size_t n = 0;
    Track *track = mFirstTrack;
    while (track) {
        ++n;
        track = track->next;
    }

    ZXLOGD("funciton out");
    return n;
}

sp<MediaSource> FFmpegExtractor::getTrack(size_t index) {
    ZXLOGD("function in.");
    status_t err;
    if ((err = readMetaData()) != OK) {
        ZXLOGD("funciton out");
        return NULL;
    }

    Track *track = mFirstTrack;
    while (index > 0) {
        if (track == NULL) {
            ZXLOGD("funciton out");
            return NULL;
        }

        track = track->next;
        --index;
    }

    if (track == NULL) {
        ZXLOGD("funciton out");
        return NULL;
    }

    ZXLOGD("getTrack(%d)", index);
    /* a@nufront start */
    ZXLOGD("funciton out");
    return new FFmpegStreamSource(
                           track->meta,
                           mDataSource,
                           track->streamId,
                           true,
                           this->ffmpegReader
                           );
    /* a@nufront end */
}

sp<MetaData>  FFmpegExtractor::getTrackMetaData(size_t index, uint32_t flags) {
    ZXLOGD("function in.");
    status_t err;
    if ((err = readMetaData()) != OK) {
        ZXLOGD("funciton out");
        return NULL;
    }

    Track *track = mFirstTrack;
    while (index > 0) {
        if (track == NULL) {
            ZXLOGD("funciton out");
            return NULL;
        }

        track = track->next;
        --index;
    }

    if (track == NULL) {
        ZXLOGD("funciton out");
        return NULL;
    }
    ZXLOGD("funciton out");
    return track->meta;
}

/* m@nufront start */
sp<MetaData>  FFmpegExtractor::getMetaData() {
    ZXLOGD("function in.");
    status_t err;
    if ((err = readMetaData()) != OK) {
        ZXLOGD("funciton out");
        return new MetaData;
    }
    const char *mime = "notmedia";
    Track *track = mFirstTrack;
    if (mHasVideo) {
        while (track) {
            if (track->streamId == ffmpeg->getVideoStreamIndex()) {
                track->meta->findCString(kKeyMIMEType, &mime);
                break;
            }
            track = track->next;
        }
        /*if (!strncasecmp(mime, "notmedia", 8)) {
            mime = "video/mp4";
        }*/
    }
    else {
        while (track) {
            if (track->streamId == ffmpeg->getAudioStreamIndex()) {
                track->meta->findCString(kKeyMIMEType, &mime);
                break;
            }
            track = track->next;
        }
    }
    mFileMetaData->setCString(kKeyMIMEType, mime);
    ZXLOGD("funciton out");

    return mFileMetaData;
}
/* m@nufront end */

status_t    FFmpegExtractor::readMetaData() {
    ZXLOGD("function in.");
    if (mHaveMetadata) {
        return OK;
    }


    status_t err;
    err = parseChunk();
    ZXLOGD("funciton out");
    return err;
}


status_t FFmpegExtractor::parseChunk() {
    ZXLOGD("function in.");
    mHaveMetadata = true;
    assert(ffmpeg);

    LOGD("parseChunk() begin, ffmpeg->getStreamCount() is %d.", ffmpeg->getStreamCount());

    for (int j=0;j<2;j++){

        const char* mimeType = NULL;
        int         codecID;
        float       timescale;
        int64_t     duration = 0;
        int         ffmpeg_type = 0;
        int         stream_id = -1;

        if(j==0) {
            stream_id = ffmpeg->getVideoStreamIndex();
            /* a@nufront start */
            if (stream_id != -1) {
                mHasVideo = true;
             }
             /* a@nufront end */
        } else if(j==1) {
            stream_id = ffmpeg->getAudioStreamIndex();
        }

        if(stream_id<0)
            continue;

        codecID  = ffmpeg->getCodecID(stream_id);
        mimeType = findMimeFromMap(codecID);

        ZXLOGD("parseChunk,stream_id=%d, mimeType=%s", stream_id, mimeType);

        if (NULL == mimeType) {
            continue;
        }

/**/
        Track* track    = new Track;
        track->next     = NULL;
        track->streamId = stream_id;
        if (mLastTrack) {
            mLastTrack->next  = track;
        }
        else {
            mFirstTrack = track;
        }
        mLastTrack = track;
        track->meta = new MetaData;
/**/
        track->meta->setCString(kKeyMIMEType, mimeType);
        {
            Mutex::Autolock autolock(ffmpeg->mutex());
            ffmpeg_type = ffmpeg->getStreamType(stream_id);
            duration = ffmpeg->getStreamDuration(stream_id);
        }
        if (duration <= 0) {
            Mutex::Autolock autolock(ffmpeg->mutex());
            duration = ffmpeg->getDuration();
        }
        ZXLOGD(" parseChunk,i=%d, duration=%lld",stream_id, duration);
        track->meta->setInt64(kKeyDuration, duration);

        //track->meta->setInt32(kKeyTimeType, track->bUseDts ? 1 : 0);   //timestamp is dts;

        /* extradata, size  */
        uint8_t* extradata;
        int extradata_size;
        {
            Mutex::Autolock autolock(ffmpeg->mutex());
            extradata      = ffmpeg->stream(stream_id)->codec->extradata;
            extradata_size = ffmpeg->stream(stream_id)->codec->extradata_size;
        }
        /* max sample size */
        int max_sample_size = 0;
        {
            Mutex::Autolock autolock(ffmpeg->mutex());
            max_sample_size = ffmpeg->stream(stream_id)->max_sample_size;
        }


        if ( ffmpeg_type == AVMEDIA_TYPE_VIDEO) {

            mHasVideo = true;

            uint16_t width, height;
            {
                Mutex::Autolock autolock(ffmpeg->mutex());
                width  = ffmpeg->stream(stream_id)->codec->width;
                height = ffmpeg->stream(stream_id)->codec->height;
            }
            //width  = width + 7 & ~7;
            //height = height + 7 & ~7;
            ZXLOGD("video,i=%d, width:%d", stream_id, width);
            ZXLOGD("video,i=%d, height:%d", stream_id, height);
            track->meta->setInt32(kKeyWidth, width);
            track->meta->setInt32(kKeyHeight, height);

            LCHLOGD("video,i=%d, width=%d, height=%d", stream_id, width, height);

            if (max_sample_size <= 0) {

                max_sample_size = width * height * 3 >> 1;
                ZXLOGD("video,max_sample_size:%d", max_sample_size);
                assert(max_sample_size > 0);
            }
            max_sample_size += MAX_BUFFER_SIZE_APPEND + extradata_size;
            track->meta->setInt32(kKeyMaxInputSize, max_sample_size);
            if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_AVC)) {
                /*Only when stream is avc, we set specific data.*/
                if (extradata_size > 0
                        && extradata
                        && *(char*)extradata == 1) {
                    track->meta->setData(kKeyAVCC, kTypeAVCC, extradata, extradata_size);
                }
                ZXLOGD("parse avc,max_sample_size=%d", max_sample_size);
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_MPEG4) ||
                     !strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_FLV1)) {

                const char* mpeg4Type = NULL;
                int   codec_tag       = 0;
                int   codec_id        = 0;
                {
                    Mutex::Autolock autolock(ffmpeg->mutex());
                    codec_tag = (const int)ffmpeg->stream(stream_id)->codec->codec_tag;
                    codec_id   = ffmpeg->stream(stream_id)->codec->codec_id;
                }
                if (codec_tag == AV_RL32("FLV1")) {
                    mpeg4Type = MEDIA_MIMETYPE_VIDEO_FLV1; // on2 sorenson spark
                }
                else if (CODEC_ID_MSMPEG4V3 == codec_id) {
                    mpeg4Type = MEDIA_MIMETYPE_VIDEO_MPEG4_DIVX3; // on2 divx
                }
                else {
                    // MP4V || XVID || ...
                    mpeg4Type = MEDIA_MIMETYPE_VIDEO_MPEG4; // on2 mpeg4
                }
                track->meta->setCString(kKeyMPEG4, mpeg4Type);
                {
                    Mutex::Autolock autolock(ffmpeg->mutex());
                    LOGI("[line:%d] max_sample_size=%d", __LINE__, ffmpeg->stream(stream_id)->max_sample_size);
                }
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_H263)) {
                ZXLOGD("parse video H263");
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_RV10)) {
                ZXLOGD("parse video rv10;");
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_RV20)) {
                ZXLOGD("parse video rv20");
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_RV30)) {
                ZXLOGD("parse video rv30");
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_RV40)) {
                ZXLOGD("parse video rv40");
            }
            else if ((!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_MPEG1)) ||
                     (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_MPEG2))) {
                track->meta->setData(kKeyMPEG2, kTypeMPEG2, extradata, extradata_size);
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_VP6)) {
                ZXLOGD("parse vp6 : max_sample_size=%d", max_sample_size);
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_VP6F)) {
                ZXLOGD("parse vp6f : max_sample_size=%d", max_sample_size);
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_VP6A)) {
                ZXLOGD("parse vp6a : max_sample_size=%d", max_sample_size);
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_VIDEO_VC1)) {
                ZXLOGD("parse vc1 : max_sample_size=%d", max_sample_size);

                const char* vc1Type = NULL;
                int codec_id        = 0;
                {
                    Mutex::Autolock autolock(ffmpeg->mutex());
                    codec_id = ffmpeg->stream(stream_id)->codec->codec_id;
                }
                if (codec_id == CODEC_ID_WMV3) {
                    vc1Type = MEDIA_MIMETYPE_VIDEO_VC1_WMV3;
                }
                else if (codec_id == CODEC_ID_VC1) {
                    vc1Type = MEDIA_MIMETYPE_VIDEO_VC1;
                }
                track->meta->setCString(kKeyVC1Type, vc1Type);
            }
            else {
                ZXLOGD("Parse ERROR: other mimetype ??    [at line:%d]", __LINE__);
            }


            // add video rotate infomation;
            if (ffmpeg) {
                track->meta->setInt32(kKeyRotation, ffmpeg->getVideoRotate(stream_id));
                ZXLOGD("video rotation:%d , {path: %s}", ffmpeg->getVideoRotate(stream_id), ffmpeg->getPath());
            }
        }
        else if (ffmpeg_type == AVMEDIA_TYPE_AUDIO) {

            AVCodecContext *codec = NULL;
            {
                Mutex::Autolock autolock(ffmpeg->mutex());
                codec = ffmpeg->stream(stream_id)->codec;
            }

            // transform ffmpeg codec pointer for audio component.
            //track->meta->setInt32(kKeyFFmpegCodecPointer, int(codec));
            //track->meta->setInt32(kKeyFFmpegCodecPointerSize, sizeof(*codec));
            /*a@nufront start: add to set key value for AVCodecContext*/
            track->meta->setInt32(kKeyCodecCtxCodecType, int(codec->codec_type));
            track->meta->setInt32(kKeyCodecCtxCodecID, int(codec->codec_id));
            track->meta->setInt32(kKeyCodecCtxCodecTag, int(codec->codec_tag));
            track->meta->setInt32(kKeyCodecCtxBitRate, int(codec->bit_rate));
            track->meta->setInt32(kKeyCodecCtxFlags, int(codec->flags));
            track->meta->setInt32(kKeyCodecCtxFlags2, int(codec->flags2));
            track->meta->setInt32(kKeyCodecCtxProfile, int(codec->profile));
            track->meta->setInt32(kKeyCodecCtxTimeBaseNum, int(codec->time_base.num));
            track->meta->setInt32(kKeyCodecCtxTimeBaseDen, int(codec->time_base.den));
            if (codec->extradata_size > 0) {
                track->meta->setInt32(kKeyCodecCtxExtraData, int(codec->extradata));
                track->meta->setInt32(kKeyCodecCtxExtraDataSize, int(codec->extradata_size));
            }
            track->meta->setInt32(kKeyCodecCtxSampleFmt, int(codec->sample_fmt));
            track->meta->setInt32(kKeyCodecCtxSampleRate, int(codec->sample_rate));
            track->meta->setInt32(kKeyCodecCtxChannels, int(codec->channels));
            track->meta->setInt32(kKeyCodecCtxFrameSize, int(codec->frame_size));
            track->meta->setInt32(kKeyCodecCtxFrameNumber, int(codec->frame_number));
            track->meta->setInt32(kKeyCodecCtxBlockAlign, int(codec->block_align));
            track->meta->setInt64(kKeyCodecCtxChannelLayout, (int64_t)(codec->channel_layout));
            track->meta->setInt32(kKeyCodecCtxTicksPerFrame, int(codec->ticks_per_frame));
            track->meta->setInt32(kKeyCodecCtxAudioServiceType, int(codec->audio_service_type));
            track->meta->setInt32(kKeyCodecCtxBitsPerCodedSample, int(codec->bits_per_coded_sample));
            track->meta->setInt32(kKeyCodecCtxBitsPerRawSample, int(codec->bits_per_raw_sample));
 //           track->meta->setCString(kKeyCodecCtxCodecNameArry, codec->codec_name);
            /*a@nufront end*/
            uint16_t num_channels = codec->channels;
            uint32_t sample_rate  = codec->sample_rate;

            track->meta->setCString(kKeyMIMEType, mimeType);
            track->meta->setInt32(kKeyChannelCount, num_channels);
            track->meta->setInt32(kKeySampleRate, sample_rate);

            // calc audio max_sample_size
            int max_sample_size = 0;
            {
                Mutex::Autolock autolock(ffmpeg->mutex());
                max_sample_size = ffmpeg->stream(stream_id)->max_sample_size;
            }
            if (max_sample_size <= 0) {
                max_sample_size = MAX_DEF_PACKET_LENGTH;
            }
            max_sample_size += MAX_BUFFER_SIZE_APPEND;
            track->meta->setInt32(kKeyMaxInputSize, max_sample_size);
            ZXLOGD("parse audio: calc audio max_input_size: %d", max_sample_size);

            if(ffmpeg->esds!=NULL)
            {
                track->meta->setData(kKeyESDS,kTypeESDS,
                        ffmpeg->esds,ffmpeg->esds_size);
            }
            if (!strcmp(mimeType, MEDIA_MIMETYPE_AUDIO_NUAUDIO)) {
                ZXLOGD("parse AAC");
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_AUDIO_RAW)) {
                ZXLOGD("parse audio raw.");
                track->meta->setInt32(kKeyMaxInputSize, MAX_WAV_PACKET_LENGTH);
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_AUDIO_MPEG)) {
                ZXLOGD("parse audio mpeg");
                track->meta->setInt32(kKeyMaxInputSize, MAX_MP3_PACKET_LENGTH);
            }
            else if (!strcmp(mimeType, MEDIA_MIMETYPE_AUDIO_AMR_NB) ||
                     !strcmp(mimeType, MEDIA_MIMETYPE_AUDIO_AMR_WB)) {
                ZXLOGD("parse audio AMR_NB or AMR_WB");
                track->meta->setInt32(kKeyMaxInputSize, MAX_AMR_PACKET_LENGTH);
            }
            else {
                ZXLOGD("Parse Waning: not recognize  audio mime format?? ");
            }
        }
    }


#if 1
    char* parser = (char* ) ffmpeg->getName();

    if((parser))
    {
        AVDictionaryEntry *tag = NULL;
        int mp3piclen = 0x0;
        int mp3picoffset = 0x0;
        char* mp3picdata = NULL;
        char mp3picmime[65];
        /* bool album = false; */ /*m@nufront: not used any more.*/

        while(tag = ffmpeg->getDictionary(tag))
        {
            LCHLOGD1("tag --key=%s, value=%s", tag->key, tag->value);

            if((tag->key) && (tag->value))
            {
                if(!strcasecmp(tag->key, "title"))
                {
                    mFileMetaData->setCString(kKeyTitle, tag->value);
                }
                else if(!strcasecmp(tag->key, "artist"))
                {
                    mFileMetaData->setCString(kKeyArtist, tag->value);
                }
                else if(!strcasecmp(tag->key, "album"))
                {
                    mFileMetaData->setCString(kKeyAlbum, tag->value);
                }
                else if(!strcasecmp(tag->key, "date"))
                {
                    mFileMetaData->setCString(kKeyDate, tag->value);
                }
                else if(!strcasecmp(tag->key, "genre"))
                {
                    mFileMetaData->setCString(kKeyGenre, tag->value);
                }
                else if(!strcasecmp(tag->key, "composer"))
                {
                    mFileMetaData->setCString(kKeyComposer, tag->value);
                }
                else if(!strcasecmp(tag->key, "albumart"))
                {
                    mFileMetaData->setCString(kKeyAlbumArt, tag->value);
                }
                else if(!strcasecmp(tag->key, "author"))
                {
                    mFileMetaData->setCString(kKeyAuthor, tag->value);
                }
                else if(!strcasecmp(tag->key, "year"))
                {
                    mFileMetaData->setCString(kKeyYear, tag->value);
                }
                else if(!strcasecmp(tag->key, "writer"))
                {
                    mFileMetaData->setCString(kKeyWriter, tag->value);
                }
                else if(!strcmp(parser, "mp3") && !strcmp(tag->key, "mp3piclen"))
                {
                    mp3piclen = atoi(tag->value);
                }
                else if(!strcmp(parser, "mp3") && !strcmp(tag->key, "mp3picmime"))
                {
                    strcpy(mp3picmime, tag->value);
                }
                else if(!strcmp(parser, "mp3") && !strcmp(tag->key, "mp3picdata"))
                {
                    mp3picdata = (char* ) atoi(tag->value);
                    /*LOGD("%s:%d  tag --key=%s, mp3picdata=%p", __FUNCTION__, __LINE__, tag->key, mp3picdata);
                    for(int i = 0x0; i < 32; i++) LOGD("0x%02x, ", mp3picdata[i] & 0xFF); */
                }
                else if(!strcmp(parser, "ogg") && !strcmp(tag->key, "ANDROID_LOOP"))
                {
                    if (!strcmp(tag->value, "true")) {
                        mFileMetaData->setInt32(kKeyAutoLoop, true);;
                    }
                }
            }
        }

        if(!strcmp(parser, "mp3") && (mp3piclen > 0x0) && (mp3picdata))
        {
            mFileMetaData->setData(kKeyAlbumArt, MetaData::TYPE_NONE, &mp3picdata[mp3picoffset], (mp3piclen - mp3picoffset));
            mFileMetaData->setCString(kKeyAlbumArtMIME, mp3picmime);

            LCHLOGD1("%s:%d  tag --",  __FUNCTION__, __LINE__);
            LCHLOGD1("mp3piclen=%d, mp3picoffset=%d, mp3picdata=%p, mp3picmime=%s", mp3piclen, mp3picoffset, mp3picdata,mp3picmime);
        }
    }
#endif

    /*a@nufront: get keylocation and set in metadata*/
    {
        AVDictionaryEntry *tag = NULL;

        if(tag = ffmpeg->getLocation())
            mFileMetaData->setCString(kKeyLocation, tag->value);
    }
    /*a@nufront: end*/
    ZXLOGD("parseChunk() end.");
    return OK;
}

const char* FFmpegExtractor::findMimeFromMap(int codec) {
    ZXLOGD("function in.");
    const char * result = NULL;
    bool bloop = true;
    int i = 0;

    VPUDecBuild (*pVPUDecGetBuild)(void);
    void *vpuDecLibHandle = NULL;
    VPUDecBuild decBuild;
    memset(&decBuild, 0 , sizeof(VPUDecBuild));

    vpuDecLibHandle = dlopen("libdwlx170.so", RTLD_NOW);
    if (vpuDecLibHandle != NULL) {
        pVPUDecGetBuild = (VPUDecBuild(*)())dlsym(vpuDecLibHandle, "VPUDecGetBuild");
        if (pVPUDecGetBuild != NULL) {
            decBuild = pVPUDecGetBuild();
        }
    }

    MimeCodecTable* codec_table = FFmpegUtils::GetMimeCodecTable();

    while (bloop) {
        if (codec_table[i].codec == codec) {
            result = codec_table[i].mime;
            if (CODEC_ID_VP8 == codec) {
                if (!strcmp(result, MEDIA_MIMETYPE_VIDEO_VP8)) {
                    if (decBuild.hwConfig.vp8Support) {
                        HPLOGD("VPU VP8 Decoder Supported");
                        bloop = false;
                    }
                } else {
                    bloop = false;
                }
            } else {
                bloop = false;
            }
        }
        i++;
        if (codec_table[i].codec == CODEC_ID_NONE) {
            bloop = false;
        }
    }

    if (vpuDecLibHandle != NULL) {
        dlclose(vpuDecLibHandle);
    }

    if (!result) {
        ZXLOGD("findMimeFromMap() not found.");
    }
    return result;
}

/*******************************************/
// hp add
static void alloc_and_copy(uint8_t **poutbuf,          int *poutbuf_size,
                           const uint8_t *sps_pps, uint32_t sps_pps_size,
                           const uint8_t *in,      uint32_t in_size) {
    uint32_t offset = *poutbuf_size;
    uint8_t nal_header_size = offset ? 3 : 4;

    *poutbuf_size += sps_pps_size+in_size+nal_header_size;
    //*poutbuf = (uint8_t*)av_realloc(*poutbuf, *poutbuf_size);
    if (sps_pps)
        memcpy(*poutbuf+offset, sps_pps, sps_pps_size);
    memcpy(*poutbuf+sps_pps_size+nal_header_size+offset, in, in_size);
    if (!offset)
        AV_WB32(*poutbuf+sps_pps_size, 1);
    else {
        (*poutbuf+offset+sps_pps_size)[0] = (*poutbuf+offset+sps_pps_size)[1] = 0;
        (*poutbuf+offset+sps_pps_size)[2] = 1;
    }
    ZXLOGD("funciton out");
}

static int h264_mp4toannexb_filter(H264BSFContext *ctx, //AVCodecContext *avctx,
                                   char* _extradata,
                                   const int _extradata_size,
                                   uint8_t  **poutbuf,  int *poutbuf_size,
                                   const uint8_t *buf,  int      buf_size) {
    uint8_t unit_type;
    int32_t nal_size;
    uint32_t cumul_size = 0;
    const uint8_t *buf_end = buf + buf_size;

    ZXLOGD("function in.");
    /* nothing to filter */
    if (!_extradata || _extradata_size < 6) {
        *poutbuf = (uint8_t*) buf;
        *poutbuf_size = buf_size;
        return 0;
    }

    /* retrieve sps and pps NAL units from extradata */
    if (!ctx->sps_pps_data) {
        uint16_t unit_size;
        uint32_t total_size = 0;
        uint8_t *out = NULL, unit_nb, sps_done = 0;
        const uint8_t *extradata = (const uint8_t*)_extradata+4;
        static const uint8_t nalu_header[4] = {0, 0, 0, 1};

        /* retrieve length coded size */
        ctx->length_size = (*extradata++ & 0x3) + 1;
        //if (ctx->length_size == 3)
        //    return AVERROR(EINVAL);

        /* retrieve sps and pps unit(s) */
        unit_nb = *extradata++ & 0x1f; /* number of sps unit(s) */
        if (!unit_nb) {
            unit_nb = *extradata++; /* number of pps unit(s) */
            sps_done++;
        }
        while (unit_nb--) {
            unit_size = AV_RB16(extradata);
            total_size += unit_size+4;
            if (extradata + 2 + unit_size > (const uint8_t*)(_extradata + _extradata_size)) {
                av_free(out);
                return AVERROR(EINVAL);
            }
            out = (uint8_t*)av_realloc(out, total_size);
            if (!out)
                return AVERROR(ENOMEM);
            memcpy(out+total_size-unit_size-4, nalu_header, 4);
            memcpy(out+total_size-unit_size,   extradata+2, unit_size);
            extradata += 2+unit_size;

            if (!unit_nb && !sps_done++)
                unit_nb = *extradata++; /* number of pps unit(s) */
        }

        ctx->sps_pps_data = out;
        ctx->size = total_size;
        ctx->first_idr = 1;
    }

    *poutbuf_size = 0;
    //*poutbuf = NULL;
    do {
        if (buf + ctx->length_size > buf_end)
            goto fail;

        if (ctx->length_size == 1)
            nal_size = buf[0];
        else if (ctx->length_size == 2)
            nal_size = AV_RB16(buf);
        else {
            for(nal_size = 0, unit_type = 0; unit_type < ctx->length_size; unit_type++) {
                nal_size = (nal_size << 8) | buf[unit_type];
            }
        } //nal_size = AV_RB32(buf);

        buf += ctx->length_size;
        unit_type = *buf & 0x1f;
        if (buf + nal_size > buf_end || nal_size < 0)
            goto fail;

        /* prepend only to the first type 5 NAL unit of an IDR picture */
        if (ctx->first_idr && unit_type == 5) {
            alloc_and_copy(poutbuf, poutbuf_size,
                           ctx->sps_pps_data, ctx->size,
                           buf, nal_size);
            ctx->first_idr = 0;
        } else {
            alloc_and_copy(poutbuf, poutbuf_size,
                           NULL, 0,
                           buf, nal_size);
            if (!ctx->first_idr && unit_type == 1)
                ctx->first_idr = 1;
        }

        buf += nal_size;
        cumul_size += nal_size + ctx->length_size;
    } while (cumul_size < buf_size);

    ZXLOGD("funciton out");
    return 1;

fail:
    //av_freep(poutbuf);     // this buffer is MediaBuffer. donnot free.
    *poutbuf_size = 0;
    ZXLOGD("funciton out");
    return AVERROR(EINVAL);
}


/*******************************************/
FFmpegStreamSource::FFmpegStreamSource(const sp<MetaData> &format,
                                       const sp<DataSource> &dataSource,
                                       int sourceStream,
                                       bool bIsDts,
                                       FFmpegSafeReader* reader
                                       ) :
    mFormat(format),
    mDataSource(dataSource),
    mStarted(false),
    mGroup(NULL),
    mBuffer(NULL),
    mbIsDts(bIsDts),
    mLastDts(-1),
    mSourceStream(sourceStream),
    packet(&mPkt),
    ffmpeg(NULL),
    isFirstPacket(true),
    ffmpegReader(reader),
    streamInfo(new FFmpegStreamInfo),
    mMaxInputSize(0),
    mSrcBuffer(NULL)
    {
    ZXLOGD("FFmpegSreamSource constructor.  streamid:%d", sourceStream);
    memset(&mH264BSFContext, 0, sizeof(H264BSFContext));    //hp add.

    ffmpegReader->add_ref();
    ffmpeg = ffmpegReader->getffmpeg();
    assert(ffmpeg);
    ZXLOGD("get streamInfo.");
    {
        Mutex::Autolock autolock(ffmpeg->mutex());
        ffmpeg->getStreamInfo(streamInfo, mSourceStream);
    }

    mTimescale = streamInfo->getTimescale();
    {
        Mutex::Autolock autolock(ffmpeg->mutex());
        ffmpeg->initPacket(packet);
    }
    ZXLOGD("get streamInfo.");
    ZJFLOGD("streamInfo->getStreamType() is %d, FFmpegStreamInfo::StreamTypeAudio is %d, FFmpegStreamInfo::StreamTypeVideo is %d", streamInfo->getStreamType(), FFmpegStreamInfo::StreamTypeAudio, FFmpegStreamInfo::StreamTypeVideo);
    if (FFmpegStreamInfo::StreamTypeAudio == streamInfo->getStreamType()) {
        ffmpegReader->setStreamType(0);
    } else if (FFmpegStreamInfo::StreamTypeVideo == streamInfo->getStreamType()) {
        ffmpegReader->setStreamType(1);
    }
    ZXLOGD("funciton out");
}

FFmpegStreamSource::~FFmpegStreamSource() {
    ZXLOGD("~FFmpegStreamSource destructor.");
    if (packet) {
        //Mutex::Autolock autolock(ffmpeg->mutex());
        ffmpeg->freePacket(packet);
    }
    delete streamInfo;
    if (ffmpegReader) {
        ffmpegReader->release_ref();
    }
    ZXLOGD("funciton out");
}

sp<MetaData> FFmpegStreamSource::getFormat() {
    ZXLOGD("function in.");
    Mutex::Autolock autoLock(mLock);
    ZXLOGD("funciton out");
    return mFormat;
}

status_t FFmpegStreamSource::start(MetaData* params ) {
    ZXLOGD("function in.");
    ffmpegReader->start();
    Mutex::Autolock autoLock(mLock);

    CHECK(!mStarted);

    mGroup = new MediaBufferGroup;

    int32_t max_size;
    CHECK(mFormat->findInt32(kKeyMaxInputSize, &max_size));
    mMaxInputSize = max_size;
    mGroup->add_buffer(new MediaBuffer(max_size));

    mSrcBuffer = new uint8_t[max_size];

    mStarted = true;
    isFirstPacket = true;

    ZXLOGD("start().");

    ZXLOGD("funciton out");
    return OK;
}

status_t FFmpegStreamSource::stop() {
    ZXLOGD("function in.");
    Mutex::Autolock autoLock(mLock);

    CHECK(mStarted);

    if (mBuffer != NULL) {
        mBuffer->release();
        mBuffer = NULL;
    }

    if (mSrcBuffer) {
        delete[] mSrcBuffer;
        mSrcBuffer = NULL;
    }
    if (mGroup) {
        delete mGroup;
        mGroup = NULL;
    }
    mStarted = false;
    isFirstPacket = true;


    ZXLOGD("stop(). ");

    ZXLOGD("funciton out");
    return OK;
}

status_t FFmpegStreamSource::set_seek_exit_flag(SeekExitFlags flag) {
    if(NULL != ffmpeg) {
        ffmpeg->set_seek_exit_flag(flag);
    }
    return OK;
}

int FFmpegStreamSource::readPacket(AVPacket* pkt, const int sourceType) {
    ZXLOGD("function in.");
    int ret = -2;
    if (sourceType == FFmpegStreamInfo::StreamTypeVideo) {
        // video
        ZXLOGD("read Video Packet.");
        ret = ffmpegReader->readVideoPacket(pkt);
    }
    else if (sourceType == FFmpegStreamInfo::StreamTypeAudio) {
        // audio
        ZXLOGD("read Audio Packet.");
        ret = ffmpegReader->readAudioPacket(pkt);
    }

    if (ret != -2) {
        ffmpegReader->readSignal();
    }
    ZXLOGD("funciton out");
    return ret;
}

status_t FFmpegStreamSource::read(MediaBuffer** out,
                               const ReadOptions* options ) {
    ZXLOGD("function in.");
    Mutex::Autolock autoLock(mLock);

    CHECK(mStarted);
    *out = NULL;

    const int streamType        = streamInfo->getStreamType();
    const char* streamTypeStr   = streamInfo->getStreamTypeStr();
    //AVStream* curStream         = ffmpeg->stream(mSourceStream);
    const int streamId          = mSourceStream;
    const CodecID codec_id      = streamInfo->getCodecId();
    bool bVideoSeeked           = false;

    ZXLOGD("read(), streamType:%d. mSourceStream:%d", streamType, mSourceStream);


    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;

    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        // seekking
        ZXLOGD("seek %s  will call avformat_seek.", streamTypeStr);
        //ffmpeg->seek(seekTimeUs);
        ffmpegReader->seek(seekTimeUs, streamType);       // mutilthread safe.
        ZXLOGD("seek %s  finish call avformat_seek. ", streamTypeStr);
        if (streamType == FFmpegStreamInfo::StreamTypeVideo) {
            bVideoSeeked = true;
        }
    }

    off_t   offset;
    size_t  size;
    int64_t dts;
    bool    newBuffer = false;
    if (mBuffer == NULL) {
        newBuffer = true;
        status_t err        = OK;
        bool isEnd          = false;
        int  ret            = 0;
        const int streamId  = mSourceStream;

        while ((ret = readPacket(packet, streamType)) >= 0) {
            if (streamId == packet->stream_index) {
                //edit
                //LOGD("zx, read a packet.");
                packet->size = packet->size <= mMaxInputSize? packet->size : 0;
                break;
            }
            else {
                {
                    //Mutex::Autolock autolock(ffmpeg->mutex());
                    ffmpeg->freePacket(packet);
                }
                ZXLOGD("read other packet(isn't this stream)");
            }
        }
        ZJFLOGD("after read packet data, ret is %d", ret);
        if (ret != -1) {
        ZXLOGD("read packet data ok. pid:%d, type:%d, packet.size: %d packet.dts: %lld packet.pts: %lld",
             getpid(), streamType, packet->size,packet->dts,packet->pts);

        offset = packet->pos;
        size   = packet->size;
        dts    = packet->dts;

/* a@nufront begin: yangxuming */
        if (streamType == FFmpegStreamInfo::StreamTypeVideo && dts < 0) {
            dts = 0;
        }
/* a@nufront end */
        }
        if (ret < 0) {
            //Mutex::Autolock autolock(ffmpeg->mutex());
            ffmpeg->freePacket(packet);
            ZXLOGD("read packet at end of stream.");
            return ERROR_END_OF_STREAM;
        }

//        if (ret < 0) {
//            if (ffmpeg->isAtEof()) {
//                ffmpeg->freePacket(packet);
//                LOGD("zx, read error, end of stream.");
//                return ERROR_END_OF_STREAM;
//            }
//            /*
//            if (AVERROR_EOF == ret || ffmpeg->isAtEof()) {
//                LOGD("zx, %s read,3 isEnd.", streamTypeStr);
//                isEnd = true;
//                return ERROR_END_OF_STREAM;
//            }
//            else {
//                LOGD("zx: %s read,4 empty packet. ", streamTypeStr);
//                MediaBuffer* tmpBuf = new MediaBuffer(0);
//                tmpBuf->meta_data()->setInt64(kKeyTime, 0);
//                *out = tmpBuf;
//                mBuffer = NULL;
//                return OK;
//            }
//            */
//        }

        if (size == 0) {
            ZXLOGD("%s read,4 empty packet. ", streamTypeStr);
            MediaBuffer* tmpBuf = new MediaBuffer(0);
            tmpBuf->meta_data()->setInt64(kKeyTime, 0);
            *out    = tmpBuf;
            mBuffer = NULL;
            return OK;
        }

        mLastDts = dts;
        ZXLOGD("adjust packet timestamp:%lld",dts);
    }

    int max_buff_size = packet->size + MAX_BUFFER_SIZE_APPEND;
    if (codec_id == CODEC_ID_RV10 ||
        codec_id == CODEC_ID_RV20 ||
        codec_id == CODEC_ID_RV30 ||
        codec_id == CODEC_ID_RV40) {
        max_buff_size += 1;                 // insert rm slice number;
    }

    status_t err = mGroup->acquire_buffer(&mBuffer);
    if (err != OK) {
        CHECK(mBuffer == NULL);
        //Mutex::Autolock autolock(ffmpeg->mutex());
        ffmpeg->freePacket(packet);
        ZXLOGD("function out for error is %d", err);
        return err;
    }

    if (streamType == FFmpegStreamInfo::StreamTypeVideo) {
        if (codec_id == CODEC_ID_MPEG4 ||
            codec_id == CODEC_ID_H263  ||
            codec_id == CODEC_ID_FLV1  ||
            codec_id == CODEC_ID_MSMPEG4V3) {

            // MPEG4, must pu extradata to first frame;
            if (newBuffer) {
                MediaBuffer *clone = mBuffer->clone();
                CHECK(clone != NULL);
                int offset = 0;
                if (isFirstPacket) {
                    offset = streamInfo->getExtradataSize();
                    memcpy(clone->data(), streamInfo->getExtradata(), offset);
                    isFirstPacket = false;
                }
                memcpy(clone->data() + offset, packet->data, size);
                clone->set_range(0, size + offset);
                clone->meta_data()->clear();
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

                *out = clone;
                mBuffer->release();
                mBuffer = NULL;
            }
            {
                //Mutex::Autolock autolock(ffmpeg->mutex());
                ffmpeg->freePacket(packet);
            }
            ZXLOGD("function out");
            return OK;
        }
        else if (codec_id == CODEC_ID_RV10 ||
                 codec_id == CODEC_ID_RV20 ||
                 codec_id == CODEC_ID_RV30 ||
                 codec_id == CODEC_ID_RV40 ) {

            // rv
            if (newBuffer) {
                MediaBuffer *clone = mBuffer->clone();
                CHECK(clone != NULL);

                int offset = 0;
                const int extradata_size = streamInfo->getExtradataSize();
                if (streamInfo->getSliceCount()) {
                    char* slice_count = (char*)clone->data();
                    *slice_count = streamInfo->getSliceCount();
                    offset += 1;
                    if (codec_id == CODEC_ID_RV30 && isFirstPacket) {
                        isFirstPacket = false;
                        char* p_extradata_size = (char*)(clone->data() + offset);
                        *p_extradata_size = streamInfo->getExtradataSize();
                        offset += 1;
                        memcpy(clone->data() + offset, streamInfo->getExtradata(), extradata_size);
                        offset += extradata_size;
                    }
                    memcpy(clone->data() + offset, packet->data, size);
                }
                else {
                    char* slice_count = (char*)clone->data();
                    *slice_count = *((char*)packet->data);
                    slice_count[0]++;
                    offset += 1;
                    if (codec_id == CODEC_ID_RV30 && isFirstPacket) {
                        isFirstPacket = false;
                        char* p_extradata_size = (char*)(clone->data() + offset);
                        *p_extradata_size = streamInfo->getExtradataSize();
                        offset += 1;
                        memcpy(clone->data() + offset, streamInfo->getExtradata(), extradata_size);
                        offset += extradata_size;
                    }
                    memcpy(clone->data() + offset, packet->data + 1, size - 1);
                }
                clone->set_range(0, size + offset);
                clone->meta_data()->clear();
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

                *out = clone;
                mBuffer->release();
                mBuffer = NULL;
            }
            {
                //Mutex::Autolock autolock(ffmpeg->mutex());
                ffmpeg->freePacket(packet);
            }
            ZXLOGD("function out");
            return OK;
        }
        else if (codec_id == CODEC_ID_MPEG2VIDEO ||
                 codec_id == CODEC_ID_MPEG1VIDEO) {

            // mpeg1/mpeg2
            if (newBuffer) {
                MediaBuffer *clone = mBuffer->clone();
                CHECK(clone != NULL);
                int offset = 0;
                if (bVideoSeeked == true) {
                    bVideoSeeked = false;
                    offset = streamInfo->getExtradataSize();
                    memcpy(clone->data(), streamInfo->getExtradata(), offset);
                }
                memcpy(clone->data() + offset, packet->data, size);
                clone->set_range(0, size + offset);
                clone->meta_data()->clear();
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

                *out = clone;
                mBuffer->release();
                mBuffer = NULL;
            }
            {
                //Mutex::Autolock autolock(ffmpeg->mutex());
                ffmpeg->freePacket(packet);
            }
            ZXLOGD("function out");
            return OK;
        }
        else if (codec_id == CODEC_ID_VP6 ||
                 codec_id == CODEC_ID_VP6F||
                 codec_id == CODEC_ID_VP6A) {
            //VP6/VP6F/VP6A
            if (newBuffer) {
                MediaBuffer *clone = mBuffer->clone();
                CHECK(clone != NULL);
                if (codec_id == CODEC_ID_VP6A) {
                    if (size < 3) {
                        //Mutex::Autolock autolock(ffmpeg->mutex());
                        ffmpeg->freePacket(packet);
                        return ERROR_MALFORMED;
                    }
                    size -= 3;
                    memcpy(clone->data(), packet->data + 3, size);
                } else {
                    memcpy(clone->data(), packet->data, size);
                }
                clone->set_range(0, size);
                clone->meta_data()->clear();
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

                *out = clone;
                mBuffer->release();
                mBuffer = NULL;
            }
            {
                //Mutex::Autolock autolock(ffmpeg->mutex());
                ffmpeg->freePacket(packet);
            }
            ZXLOGD("function out");
            return OK;
        }
        else if (codec_id == CODEC_ID_VC1 ||
                 codec_id == CODEC_ID_WMV3) {
            //VC1
            if (newBuffer) {
                MediaBuffer *clone = mBuffer->clone();
                CHECK(clone != NULL);
                int offset = 0;
                if (isFirstPacket) {
                    char* extradata_size = (char*)clone->data();
                    *extradata_size = streamInfo->getExtradataSize();
                    offset += 1;
                    memcpy(clone->data() + offset, streamInfo->getExtradata(), *extradata_size);
                    offset += streamInfo->getExtradataSize();
                    isFirstPacket = false;
                }
                if (codec_id == CODEC_ID_VC1 &&
                        ((streamInfo->getExtradataSize() == 0) ||
                        (streamInfo->getExtradataSize() &&
                                streamInfo->getExtradata()[0] != 0x00))) {
                    char* buf = (char*)clone->data();
                    // add vc1 advanced profile header at the front of stream
                    *(buf + offset + 0) = 0x00;
                    *(buf + offset + 1) = 0x00;
                    *(buf + offset + 2) = 0x01;
                    *(buf + offset + 3) = 0x0d;
                    offset += 4;
                }
                memcpy(clone->data() + offset, packet->data, size);
                clone->set_range(0, size + offset);
                clone->meta_data()->clear();
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

                *out = clone;
                mBuffer->release();
                mBuffer = NULL;
            }
            {
                //Mutex::Autolock autolock(ffmpeg->mutex());
                ffmpeg->freePacket(packet);
            }
            ZXLOGD("function out.");
            return OK;
        }
        else if (codec_id == CODEC_ID_H264) {
            // h264
            if (newBuffer) {
                MediaBuffer *clone = mBuffer->clone();
                CHECK(clone != NULL);
                int osize = 0;
                uint8_t* pout = (uint8_t*)clone->data();
                if (streamInfo->getExtradataSize() > 0
                        && streamInfo->getExtradata()
                        && *(char*)streamInfo->getExtradata() == 1) {
                    h264_mp4toannexb_filter(&mH264BSFContext, (char*)streamInfo->getExtradata(),
                                            streamInfo->getExtradataSize(),
                                            &pout, &osize, packet->data, size);
                    clone->set_range(0, osize);
                } else {
                    int offset = 0;
                    if (isFirstPacket) {
                        offset = streamInfo->getExtradataSize();
                        memcpy(clone->data(), streamInfo->getExtradata(), offset);
                        isFirstPacket = false;
                    }
                    memcpy(clone->data() + offset, packet->data, size);
                    clone->set_range(0, size + offset);
                }
                clone->meta_data()->clear();
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

                *out = clone;
                mBuffer->release();
                mBuffer = NULL;
            }
            {
                //Mutex::Autolock autolock(ffmpeg->mutex());
                ffmpeg->freePacket(packet);
            }

            ZXLOGD("function out.");
            return OK;
        }
        else if (codec_id == CODEC_ID_VP8) {
            // vp8
            if (newBuffer) {
                MediaBuffer *clone = mBuffer->clone();
                CHECK(clone != NULL);
                memcpy(clone->data(), packet->data, size);
                clone->set_range(0, size);
                clone->meta_data()->clear();
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

                *out = clone;
                mBuffer->release();
                mBuffer = NULL;
            }
            {
                //Mutex::Autolock autolock(ffmpeg->mutex());
                ffmpeg->freePacket(packet);
            }
            ZXLOGD("function out");
            return OK;
        }
        else if (codec_id == CODEC_ID_MJPEG) {
            // mjpeg
            if (newBuffer) {
                MediaBuffer *clone = mBuffer->clone();
                CHECK(clone != NULL);
                memcpy(clone->data(), packet->data, size);
                clone->set_range(0, size);
                clone->meta_data()->clear();
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

                *out = clone;
                mBuffer->release();
                mBuffer = NULL;
            }
            {
                //Mutex::Autolock autolock(ffmpeg->mutex());
                ffmpeg->freePacket(packet);
            }
            ZXLOGD("function out");
            return OK;
        }
    }
    else if (streamType == FFmpegStreamInfo::StreamTypeAudio) {             // audio
       // AAC
        if (newBuffer) {
            MediaBuffer *clone = mBuffer->clone();
            CHECK(clone != NULL);
            memcpy(clone->data(), packet->data, size);
            clone->set_range(0, size);
            clone->meta_data()->clear();
            if (dts == AV_NOPTS_VALUE)
                clone->meta_data()->setInt64(kKeyTime, int64_t(INVALID_TIMESTAMP));
            else
                clone->meta_data()->setInt64(kKeyTime, int64_t(dts * mTimescale));

            *out = clone;
            mBuffer->release();
            mBuffer = NULL;
        }
        {
            //Mutex::Autolock autolock(ffmpeg->mutex());
            ffmpeg->freePacket(packet);
        }
        ZXLOGD("function out.");
        return OK;
    }
    else {
        *out = NULL;
    }
    {
        //Mutex::Autolock autolock(ffmpeg->mutex());
        ffmpeg->freePacket(packet);
    }
    ZXLOGD("function out.");
    return OK;
}


//static
static bool isSystemMedia(const char* path) {
   using namespace FFMPEG;
   Regex reg("^/system/media/audio/");
   return reg.exec(path);
}


//static
static bool isAndroidfd(const char* str) {
    using namespace ::FFMPEG;

    const char* pattern = "^androidfd://[0-9]\\{1,\\}_[0-9]\\{1,\\}_[0-9]\\{1,\\}$";
    Regex reg(pattern);

#if 0
    //test regex;
    if (reg.exec(str)) {
        ZXLOGD("regex.exec('%s', '%s') ok.", pattern, str);
    }
    else {
        ZXLOGD("regex.exec('%s', '%s') wrong.", pattern, str);
    }

    const char* test_str = "androidfd://123_456_xxx";
    if (reg.exec(test_str)) {
        ZXLOGD("regex.exec('%s', '%s') ok.", pattern, test_str);
    }
    else {
        ZXLOGD("regex.exec('%s', '%s') wrong.", pattern, test_str);
    }
    ZXLOGD(" isAndroidfd(%s)", str);
#endif

    return reg.exec(str);
}



bool SniffFFmpeg(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *meta) {

    int ret  = false;
    const char* path = source->getUri();

/***************************************************/
/* recognition  area. */

    if (source->flags()) {
        if(androidSource::IsFFMPEG())
        {
            *confidence = 500.1f;
            return true;
        }
    }
    else if (path && strlen(path)&& isAndroidfd(path)) {
        // go androdifd protocol.
        ZXLOGD("is androidfd protocol. so use FFmepgExtractor.");
        ret = true;
    }
    else if (path && strlen(path) && !isSystemMedia(path)) {
        // the media file under system path that is must be use original demuxer

        ret = true;
    }
    else if (isSystemMedia(path)) {
        ZXLOGD("warning: is system media file: %s.", path);
        //  for .ogg
        ret = true;
    }
/****************************************************/
    if (ret) {
        *mimeType = MEDIA_MIMETYPE_CONTAINER_FFMPEG;
        *confidence = 500.1f;
        ZXLOGD("sniff use ffmpeg extractor. and lowest priority.");
    }
    else {
        ZXLOGD("not sniff use ffmpeg extractor.");
    }

    return ret;
}

}

