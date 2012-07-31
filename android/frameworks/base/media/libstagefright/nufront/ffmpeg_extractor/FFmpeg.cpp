#include "FFmpeg.h"
#include <utils/nufrontlog.h>
#include "FFmpegSafeReader.h"
#define LOG_TAG "FFmpeg"

using namespace android;
/* add by gs
 * _GEN_EXTRA_INDEX_FILE_==1 then generate extra index file in /cache
 * index file format
 * | nufront | md5code |complete_flag|  size  |   pos  | timestamp|   pos  | timestamp| ...
 * | 7 byte  | 16 byte |   1 byte    | 4 byte | 8 byte |   8 byte | 8 byte |   8 byte | ...
 * md5code generate from first pos to the end of file
 * save indexfile in NUFRONT_CACHE_PATH
 */
#ifndef _GEN_EXTRA_INDEX_FILE_
#define _GEN_EXTRA_INDEX_FILE_ 1
#endif
extern "C"
{
#include "md5.h"
}
#include <sys/stat.h>
#include <sys/types.h>
#ifndef NUFRONT_CACHE_PATH
#define NUFRONT_CACHE_PATH "/data/mediaserver"
#endif
#define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)

/*audio video sync delay for hardware latency*/
#ifndef NUFRONT_AUDIO_DELAY
#define NUFRONT_AUDIO_DELAY 170000
#endif

/***********************************/
FFmpegStreamInfo::FFmpegStreamInfo() :
    strStreamType(NULL),
    extradata(NULL),
    extradata_size(0),
    streamId (-1),
    codec_id(CODEC_ID_NONE),
    slice_count(0),
    duration(0),
    streamType(StreamTypeNone),
    esds(NULL),
    esds_size(0)
    {

}

FFmpegStreamInfo::~FFmpegStreamInfo() {
    if (extradata) {
        delete[] extradata;
    }
        if(esds!=NULL)
    {
        delete[] esds;
        esds = NULL;
    }
    esds_size = 0;
}
/***********************************/

FFmpeg::FFmpeg()
    : streamVideoIndex(-1),
      streamAudioIndex(-1),
      bEOFError(false),
      scaleVideo(.0f),
      scaleAudio(.0f),
      videoDuration(0),
      audioDuration(0),
      duration(0),
      ctx(NULL),
      ctx_seek(NULL),
      esds(NULL),
      esds_size(0),
      seek_flag(0),
      after_seek_first_video(0),
      first_audio_dts(0),
      first_video_dts(0),
      first_audio_flag(1),
      bExit(false),
      index_size(0),
      seek_over(0),
      mOpenFailed(false)
{
          path[0] = 0;
          indexfilepath[0] = 0;
}


FFmpeg::FFmpeg(const char* f_path)
    : streamVideoIndex(-1),
      streamAudioIndex(-1),
      scaleVideo(.0f),
      scaleAudio(.0f),
      videoDuration(0),
      audioDuration(0),
      duration(0),
      ctx(NULL),
      ctx_seek(NULL),
      esds(NULL),
      esds_size(0),
      seek_flag(0),
      after_seek_first_video(0),
      first_audio_dts(0),
      first_video_dts(0),
      first_audio_flag(1),
      bExit(false),
      index_size(0),
      seek_over(0),
      mOpenFailed(false)
{
          path[0] = 0;
          indexfilepath[0] = 0;
          open(f_path);
}

FFmpeg::~FFmpeg() {
    close();
}

// mutilthread safe;
FFmpegStreamInfo* FFmpeg::getStreamInfo(FFmpegStreamInfo* info,const int streamID) {
    BacktraceAssert(info);
    BacktraceAssert(ctx);

    if(!ctx)
    {
        return NULL;
    }

    //android::AutoMutex _am(ctxMutex);

    BacktraceAssert(streamID >= 0);
    BacktraceAssert(streamID < ctx->nb_streams);

    AVStream* stream = ctx->streams[streamID];
    //FFmpegStreamInfo* info = new FFmpegStreamInfo;

    info->streamId           = streamID;
    info->codec_id           = stream->codec->codec_id;
    info->slice_count        = stream->codec->slice_count;
    if(AV_NOPTS_VALUE==stream->duration
       || stream->duration<ctx->duration)
        info->duration           = ctx->duration/1000.0*double(stream->time_base.den)/stream->time_base.num;
    else
        info->duration           = stream->duration;
    info->time_base          = stream->time_base;
    info->max_sample_size    = stream->max_sample_size;
    if(!(stream->codec->codec_type == AVMEDIA_TYPE_VIDEO && (stream->codec->codec_id == CODEC_ID_MPEG4 || stream->codec->codec_id == CODEC_ID_MSMPEG4V3) && stream->codec->error[0] == FF_MPEG4_EXTRADATA_ERR)) //add by xy
    {
       info->extradata_size     = stream->codec->extradata_size;
       info->extradata          = new char[info->extradata_size];
       memcpy(info->extradata, stream->codec->extradata, info->extradata_size);
   }
   else
      ZXLOGD("MPEG4 EXTRADATA ERROR");
/* add by gs
 * original decoder need esds info
 */
    info->esds = NULL;
    info->esds_size = 0;
//
    if (stream->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
        info->streamType     = FFmpegStreamInfo::StreamTypeAudio;
        info->strStreamType  = "audio";
/* add by gs
 */
        if(stream->codec->esds!=NULL && stream->codec->esds_size>0)
        {
            info->esds_size = stream->codec->esds_size;
            info->esds = new char[info->esds_size];
            memcpy(info->esds,stream->codec->esds,info->esds_size);
        }
//
    }
    else if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        info->streamType     = FFmpegStreamInfo::StreamTypeVideo;
        info->strStreamType  = "video";
    }
    else {
        info->streamType     = FFmpegStreamInfo::StreamTypeNone;
        info->strStreamType  = "other";
    }

    return info;
}

static void log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    static int print_prefix = 1;
    char line[1024];
    if(level>0) {
        return ;
    }
    av_log_format_line(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);

    GSLOGD("%s",line);
    LCHLOGD2("%s",line);
}

int FFmpeg::bitset(int32_t* value, uint8_t* index, uint8_t* ptr,
           int32_t bitvalue, uint8_t bitsize) {
    if(*index+bitsize>=32){
        uint8_t needlen = 32 - *index;
        uint8_t remainlen = bitsize - needlen;
        *value <<= needlen;
        *value |= (bitvalue >> remainlen );
        ptr[0] = ((*value)>>24) & 0xff;
        ptr[1] = ((*value)>>16) & 0xff;
        ptr[2] = ((*value)>>8) & 0xff;
        ptr[3] = (*value) & 0xff;
        *value = bitvalue;
        *index = remainlen;
        return 4;
     } else {
        *value <<= bitsize;
        *value |= bitvalue;
        *index += bitsize;
        return 0;
     }
}

void FFmpeg::open(const char* filepath) {
    BacktraceAssert(filepath);
    BacktraceAssert(path[0]==0);
    //Mutex::Autolock autolock(ctxMutex);
    static volatile int refs = 0;

    if ( refs++ == 0) {
        av_register_all();
        avcodec_register_all();
        av_log_set_callback(log_callback);
    }

    if(androidSource::IsFFMPEG() && (!strncmp(filepath,"androidsource://", 16)))
    {
        androidSource::RegisterAndroidSource();
    }

    int ret = 0;
    path[0] = 0;
    if(!strncmp(filepath,"androidfd://", 12)) {
        char linkpath[4096] = {0};
        int fd = atoi(filepath+12);
        char* final = NULL;
        int64_t offset = 0;
        char* tmpptr = strstr(filepath,"_");
        GSLOGD("filename:%s fd:%d",filepath,fd);
        if(NULL != tmpptr) {
            offset = strtoll(tmpptr+1, &final, 10);
            GSLOGD("offset:%lld",offset);
            if(0 == offset) {
                int pid = getpid();
                sprintf(linkpath,"/proc/%d/fd/%d",pid,fd);
                ret = readlink(linkpath, path, 4096);
                if (ret > 0 && ret < 4096) {
                    path[ret] = 0;
                    /*check whether this thread has permission to read this file*/
                    if(access(path, 4) != 0) {
                        GSLOGD("linkpath:%s, path:%s access failed",linkpath,path);
                        path[0] = 0;
                    }
                } else {
                    path[0] = 0;
                }
                GSLOGD("linkpath:%s, path:%s",linkpath,path);
            }
        }
    }
    if(0 == path[0]) {
        strcpy(path,filepath);
    }

    GSLOGD("av_open_input_file(path:%s, realpath:%s).",filepath, path);
    ret = av_open_input_file(&ctx, path, NULL, NULL, NULL);
    //ret = avformat_open_input(&ctx, path, NULL, NULL);
    if (ret < 0) {
        ZXLOGD("open file error %d. please check path validate!\n",ret);
        ZXLOGD("path: %s  realpath:%s\n", filepath, path);

        BacktraceAssert(0);
        mOpenFailed = true;
        return;
    }
    ZXLOGD("av_open_input_file %s over", path);
    for(ret = 0; ret < ctx->nb_streams; ret++) {
        if(AVMEDIA_TYPE_AUDIO == ctx->streams[ret]->codec->codec_type &&
           NULL != ctx->streams[ret]->codec->esds &&
           ctx->streams[ret]->codec->esds_size > 0) {
            esds_size = ctx->streams[ret]->codec->esds_size;
            if(NULL != esds) {
                delete[] esds;
                esds = NULL;
            }
            esds = new char[esds_size];
            memcpy(esds,ctx->streams[ret]->codec->esds,esds_size);
        }
        if (ctx->streams[ret]->codec->esds!=NULL ) {
            av_free(ctx->streams[ret]->codec->esds);
            ctx->streams[ret]->codec->esds = NULL;
        }
    }
    ZXLOGD("av_find_stream_info(ctx:%x).", ctx);
    if (ctx) {
        /*add max_analyze_duration for some error file*/
        ctx->max_analyze_duration *= 100;
        ret = av_find_stream_info(ctx);
        if (ret < 0) {
            ZXLOGD("av_find_stream_info error!\n");
            BacktraceAssert(0);
            mOpenFailed = true;
            return;
        }
        findAVIndex();
        calcScale();
        findDuration();
        if(streamVideoIndex >= 0 && AVMEDIA_TYPE_VIDEO == ctx->streams[streamVideoIndex]->codec->codec_type
                && CODEC_ID_MPEG4 == ctx->streams[streamVideoIndex]->codec->codec_id
                && NULL == ctx->streams[streamVideoIndex]->codec->extradata) {
            int nnn = av_log2(ctx->streams[streamVideoIndex]->r_frame_rate.num - 1) + 1;
            int32_t value = 0;
            uint8_t index = 0;
            ctx->streams[streamVideoIndex]->codec->extradata_size = (nnn+166+7)/8;
            ctx->streams[streamVideoIndex]->codec->extradata = (uint8_t*)av_mallocz(ctx->streams[streamVideoIndex]->codec->extradata_size+FF_INPUT_BUFFER_PADDING_SIZE);
            uint8_t* ptr = ctx->streams[streamVideoIndex]->codec->extradata;
            /*vos*/
            ptr += bitset(&value,&index,ptr,0x000001b0,32);
            ptr += bitset(&value,&index,ptr,ctx->streams[streamVideoIndex]->codec->profile & 0x0f,4);
            ptr += bitset(&value,&index,ptr,ctx->streams[streamVideoIndex]->codec->level & 0x0f,4);
            /*vol*/
            ptr += bitset(&value,&index,ptr,0x00000120,32);
            ptr += bitset(&value,&index,ptr,0x00,1);/*skip_bits*/
            ptr += bitset(&value,&index,ptr,0x00,8);/*vo_type*/
            ptr += bitset(&value,&index,ptr,0x00,1);/*is_ol_id*/
            ptr += bitset(&value,&index,ptr,0x0f,4);/*aspect_ratio_info*/
            ptr += bitset(&value,&index,ptr,ctx->streams[streamVideoIndex]->codec->sample_aspect_ratio.num,8);
            ptr += bitset(&value,&index,ptr,ctx->streams[streamVideoIndex]->codec->sample_aspect_ratio.den,8);
            ptr += bitset(&value,&index,ptr,0x01,1);/*vol_control_parameters*/
            ptr += bitset(&value,&index,ptr,0x01,2);/*chroma_format*/
            ptr += bitset(&value,&index,ptr,0x01,1);/*low_delay*/
            ptr += bitset(&value,&index,ptr,0x00,1);/*vbv parameters*/
            ptr += bitset(&value,&index,ptr,0x00,2);/*shape*/
            ptr += bitset(&value,&index,ptr,0x01,1);/*check_marker*/
            ptr += bitset(&value,&index,ptr,ctx->streams[streamVideoIndex]->r_frame_rate.num,16);
            ptr += bitset(&value,&index,ptr,0x01,1);/*check_marker*/
            ptr += bitset(&value,&index,ptr,0x01,1);/*fixed_vop_rate*/
            ptr += bitset(&value,&index,ptr,ctx->streams[streamVideoIndex]->r_frame_rate.den,nnn);
            ptr += bitset(&value,&index,ptr,0x01,1);/*marker*/
            ptr += bitset(&value,&index,ptr,ctx->streams[streamVideoIndex]->codec->width,13);
            ptr += bitset(&value,&index,ptr,0x01,1);/*marker*/
            ptr += bitset(&value,&index,ptr,ctx->streams[streamVideoIndex]->codec->height,13);
            ptr += bitset(&value,&index,ptr,0x01,1);/*marker*/
            ptr += bitset(&value,&index,ptr,0x00,1);/*interlaced*/
            ptr += bitset(&value,&index,ptr,0x01,1);/*OBMC*/
            ptr += bitset(&value,&index,ptr,0x00,1);/*vol_sprite_usage*/
            ptr += bitset(&value,&index,ptr,0x00,1);/*not_8_bit*/
            ptr += bitset(&value,&index,ptr,0x00,1);/*mpeg_quant*/
            ptr += bitset(&value,&index,ptr,0x01,1);/*estimation_flag*/
            ptr += bitset(&value,&index,ptr,0x01,1);/*resync_marker*/
            ptr += bitset(&value,&index,ptr,0x00,1);/*data_partitioning*/
            ptr += bitset(&value,&index,ptr,0x00,1);/*scalability*/
            ptr += bitset(&value,&index,ptr,0x0,32-index);
            GSLOGD("set MPEG4Video ExtraData %d ",ctx->streams[streamVideoIndex]->codec->extradata_size);
        }
        if(strncmp(path,"androidsource://", 16) && strncmp(path,"androidfd://", 12) && streamVideoIndex>=0 && !ctx->pb->is_streamed && ctx->pb->seekable && duration/1000000 > ctx->streams[streamVideoIndex]->nb_index_entries) {
/* add by gs
 * load extra index file for play_ctx
 */
#if _GEN_EXTRA_INDEX_FILE_
            int len = strlen(path);
            if(len+strlen(NUFRONT_CACHE_PATH)>=PATH_MAX) {
                GSLOGD("filename len is %d, is too long, we can't save the indexfile",len);
                index_size = NOT_SAVE_INDEX;
                return ;
            }

            sprintf(indexfilepath,"%s%s",NUFRONT_CACHE_PATH,path);
            len = strlen(indexfilepath);
            for(ret=strlen(NUFRONT_CACHE_PATH)+1;ret<len;ret++) {
                if(indexfilepath[ret]=='/') {
                    indexfilepath[ret]='_';
                }
            }
            ret = load_indexfile(indexfilepath,ctx);
            GSLOGD("load indexfile ret:%d path:%s",ret,indexfilepath);
            if(ret==RET_LOAD_FULL_INDEX) {
                index_size = NOT_SAVE_INDEX;
                return ;
            }
#endif
            startThread();
        }
    }
}

int FFmpeg::gen_md5(FILE* fp,char* digest)
{
    if(fp==NULL) {
        return RET_ERR;
    }
    MD5_CTX context;
    int len;
    unsigned char buffer[1024] = {0};
    MD5Init(&context);
    while (len = fread(buffer,1,1024,fp)) {
        MD5Update(&context,buffer,len);
    }
    MD5Final((unsigned char*)digest, &context);
    return RET_OK;
}

int FFmpeg::check_indexfile(FILE* fp)
{
    fseek(fp,0,SEEK_END);
    int len = ftell(fp);
    if(len<28) {
        return RET_ERR;
    }
    fseek(fp,0,SEEK_SET);
    char tmp[16] = {0};
    fread(tmp,1,7,fp);
    if(strcmp(tmp,"nufront")!=0) {
        return RET_ERR;
    }
    fread(tmp,1,16,fp);
    fread(&seek_over,1,1,fp);
    fread(&index_size,4,1,fp);
    len -= 28;
    if(len!=index_size*24) {
        return RET_ERR;
    }
    char md5[16] = {0};
    if(gen_md5(fp,md5)) {
        return RET_ERR;
    }
    if(memcmp(tmp,md5,16)) {
        return RET_ERR;
    }
    return RET_OK;
}

int FFmpeg::load_indexfile(char* indexfile, AVFormatContext* cx)
{
    if(cx==NULL) {
        return RET_ERR;
    }
    int ret = 0;
    FILE* fp = fopen(indexfile,"rb");
    if(fp==NULL) {
        return RET_ERR;
    }
    ret = check_indexfile(fp);
    if(ret!=RET_OK) {
        fclose(fp);
        return ret;
    }
    if(fseek(fp,28,SEEK_SET)!=0) {
        fclose(fp);
        return RET_ERR;
    }
    int64_t pos,timestamp;
    int size,min_distance,i;
    size = min_distance = 0;
    for(i=0;i<index_size;i++) {
        if(fread(&pos,8,1,fp)!=1) {
            break;
        }
        if(fread(&timestamp,8,1,fp)!=1) {
            break;
        }
        if(fread(&size,4,1,fp)!=1) {
            break;
        }
        if(fread(&min_distance,4,1,fp)!=1) {
            break;
        }
//        GSLOGD("load index cx:%x VideoIndex:%d i:%d pos:%lld timestamp:%lld size:%d min_distance:%d",cx,streamVideoIndex,i,pos,timestamp,size,min_distance);
        av_add_index_entry(cx->streams[streamVideoIndex],pos,timestamp,size,min_distance,1);
    }
    fclose(fp);
    if(seek_over && i==index_size) {
        return RET_LOAD_FULL_INDEX;
    }
    return i;
}

int FFmpeg::save_indexfile(char* indexfile, uint8_t over)
{
    FILE* fp = fopen(indexfile,"wb");
    if(fp==NULL) {
        return RET_ERR;
    }
    unsigned char tmpbuf[1024] = {0};
    if(fwrite(tmpbuf,1,28,fp) != 28) {
        fclose(fp);
        return RET_ERR;
    }
    int len,i,count,size;
    MD5_CTX context;
    MD5Init(&context);
    /* ffmpeg need pos,timestamp,size & min_distance
     * md5 need loop update, each bufsize = 1008 = 24*42, every 42 indexes update md5
     */
    for(i=0,count=0,len=0;i<ctx->streams[streamVideoIndex]->nb_index_entries;i++) {
        if(ctx->streams[streamVideoIndex]->index_entries[i].flags == 1) {
            memcpy(tmpbuf+len,&ctx->streams[streamVideoIndex]->index_entries[i].pos,8);
            len+=8;
            memcpy(tmpbuf+len,&ctx->streams[streamVideoIndex]->index_entries[i].timestamp,8);
            len+=8;
            size = ctx->streams[streamVideoIndex]->index_entries[i].size;
            memcpy(tmpbuf+len,&size,4);
            len+=4;
            memcpy(tmpbuf+len,&ctx->streams[streamVideoIndex]->index_entries[i].min_distance,4);
            len+=4;
            if(len>=1008) {
                if(fwrite(tmpbuf,1,len,fp)!=len) {
                    fclose(fp);
                    return RET_ERR;
                }
                MD5Update(&context,tmpbuf,len);
                len = 0;
            }
            count++;
        }
    }
    if(len>0) {
        if(fwrite(tmpbuf,1,len,fp)!=len) {
            fclose(fp);
            return RET_ERR;
        }
        MD5Update(&context,tmpbuf,len);
        len = 0;
    }
    MD5Final(tmpbuf,&context);
    if(i<ctx->streams[streamVideoIndex]->nb_index_entries) {
        over = 0;
    }
    len = RET_OK;
    if((fseek(fp,0,SEEK_SET)!=0) || (fwrite("nufront",1,7,fp)!=7)
     || (fwrite(tmpbuf,1,16,fp)!=16) || (fwrite(&over,1,1,fp)!=1)
     || (fwrite(&count,4,1,fp)!=1)) {
        len = RET_ERR;
    }
    fclose(fp);
    if(RET_OK == len && chmod(indexfile,0777)==0)
        return RET_OK;
    return RET_ERR;
}

int FFmpeg::mk_dir(char* dir, bool isdir)
{
    char tmpstr[PATH_MAX] = {0};
    char* begptr = dir;
    char* endptr = strchr(begptr, ':');
    if(endptr) {
        begptr = endptr + 2;
    } else {
        for(int i = 0; i < (int)strlen(dir); i++, begptr++) {
            if(begptr[0] != '.' && begptr[0] != '/' && begptr[0] != '\\') {
                break;
            }
        }
    }
    for(;;) {
        endptr = strchr(begptr, '/');
        if(endptr != NULL) {
            strncpy(tmpstr, dir, endptr - dir);
            tmpstr[endptr - dir] = '\0';
            if(access(tmpstr, 0) != 0) {
                if(mkdir(tmpstr,0777)!=0) {
                    return RET_ERR;
                }
            }
            begptr = endptr + 1;
            if(begptr - dir >= (int)strlen(dir)) {
                break;
            }
        } else {
            if(isdir && access(dir, 0) != 0) {
                if(mkdir(dir,0777)!=0) {
                    return RET_ERR;
                }
            }
            break;
        }
    }
    return RET_OK;
}

void FFmpeg::startThread()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    int n = pthread_create(&mThread, &attr, run, this);
    pthread_attr_destroy(&attr);
}

void* FFmpeg::run(void* param)
{
    FFmpeg* r = (FFmpeg*) param;
    return r->run_process();
}

void* FFmpeg::run_process()
{
    GSLOGD("start index gen thread path:%s",path);
    int ret,i;
    ret = av_open_input_file(&ctx_seek, path, NULL, NULL, NULL);
    if(ret < 0 && NULL == ctx_seek) {
        return NULL;
    }
    ret = av_find_stream_info(ctx_seek);
    if(ret < 0) {
        av_close_input_file(ctx_seek);
        ctx_seek = NULL;
        return NULL;
    }
    for(i=0; i < ctx_seek->nb_streams; i++) {
        if(i!=streamVideoIndex && i!=streamAudioIndex) {
            ctx_seek->streams[i]->discard = AVDISCARD_ALL;
        }
    }

    if(strcmp(ctx_seek->iformat->name,"mpegts") != 0 && strcmp(ctx_seek->iformat->name,"mpeg") != 0) {
        int64_t seektime = av_rescale_q(duration, AV_TIME_BASE_Q, ctx_seek->streams[streamVideoIndex]->time_base);
        ret = avformat_seek_file(ctx_seek, streamVideoIndex, 0, 0x7fffffffffffffffLL, 0x7fffffffffffffffLL,0);
        seek_over = 1;
/*        Mutex::Autolock autolock(ctxMutex); */
        for(i = 0; i < ctx_seek->streams[streamVideoIndex]->nb_index_entries; i++) {
            if(1 == ctx_seek->streams[streamVideoIndex]->index_entries[i].flags) {
                Mutex::Autolock autolock(ctxMutex);
                av_add_index_entry(ctx->streams[streamVideoIndex],
                    ctx_seek->streams[streamVideoIndex]->index_entries[i].pos,
                    ctx_seek->streams[streamVideoIndex]->index_entries[i].timestamp,
                    ctx_seek->streams[streamVideoIndex]->index_entries[i].size,
                    ctx_seek->streams[streamVideoIndex]->index_entries[i].min_distance,
                    ctx_seek->streams[streamVideoIndex]->index_entries[i].flags);
            }
        }
    } else {
        gen_index_tsps();
    }

    if(ctx_seek) {
        av_close_input_file(ctx_seek);
        ctx_seek = NULL;
    }
    return NULL;
}

int FFmpeg::gen_index_tsps()
{
    int ret = 0;
    int count = 0;
    int64_t timestamp = 0;
    AVPacket pkt;
/* add by gs
 * load extra file for seek_ctx
 * then seek to the last keyframe
 */
#if _GEN_EXTRA_INDEX_FILE_
    ret = load_indexfile(indexfilepath,ctx_seek);
    if(ret >= 0 && ctx_seek->streams[streamVideoIndex]->nb_index_entries > 0) {
        timestamp = ctx_seek->streams[streamVideoIndex]->index_entries[ctx_seek->streams[streamVideoIndex]->nb_index_entries-1].timestamp+1;
    }
    if(timestamp > 1) {
        avformat_seek_file(ctx_seek, streamVideoIndex, first_video_dts, timestamp, seek_max, AVSEEK_FLAG_BACKWARD);
    }
#endif
    GSLOGD("ts gen index start:%lld",systemTime());
    while(!bExit) {
        if(av_read_frame(ctx_seek,&pkt) < 0) {
            seek_over = 1;
            break;
        }
        if(ctx_seek->streams[streamVideoIndex]->nb_index_entries > count + 100) {
            for(; count < ctx_seek->streams[streamVideoIndex]->nb_index_entries ; count++) {
                if(1 == ctx_seek->streams[streamVideoIndex]->index_entries[count].flags) {
                    Mutex::Autolock autolock(ctxMutex);
                    timestamp = ctx->streams[streamVideoIndex]->index_entries[ctx_seek->streams[streamVideoIndex]->nb_index_entries - 1].timestamp;
                    if(ctx_seek->streams[streamVideoIndex]->index_entries[count].timestamp > timestamp) {
                        av_add_index_entry(ctx->streams[streamVideoIndex],
                            ctx_seek->streams[streamVideoIndex]->index_entries[count].pos,
                            ctx_seek->streams[streamVideoIndex]->index_entries[count].timestamp,
                            ctx_seek->streams[streamVideoIndex]->index_entries[count].size,
                            ctx_seek->streams[streamVideoIndex]->index_entries[count].min_distance,
                            ctx_seek->streams[streamVideoIndex]->index_entries[count].flags);
                    }
                }
            }
        }
        av_free_packet(&pkt);
    }
    GSLOGD("ts gen index stop:%lld",systemTime());
    return 0;
}

int FFmpeg::set_seek_exit_flag(int flag) {
    if(NULL != ctx) {
        if(0 == flag) {
            ctx->exit_flag = 0;
        } else {
            ctx->exit_flag = 1;
        }
    }
    return 0;
}

void FFmpeg::close() {
    //Mutex::Autolock autolock(ctxMutex);
    bExit = true;
    if(NULL != ctx_seek) {
        ctx_seek->exit_flag = 1;
    }
    if(NULL != ctx) {
        ctx->exit_flag = 1;
    }

    pthread_join(mThread, NULL);
    if(NULL != ctx_seek) {
        av_close_input_file(ctx_seek);
        ctx_seek = NULL;
    }
    if (ctx) {
/* add by gs
 * save the extra index file
 */
#if _GEN_EXTRA_INDEX_FILE_
        if(NOT_SAVE_INDEX != index_size && 0 != indexfilepath[0] && strncmp(path,"androidsource://", 16) && streamVideoIndex >= 0 && !ctx->pb->is_streamed && ctx->pb->seekable) {
            int ret,count;
            for(ret=0,count=0; ret < ctx->streams[streamVideoIndex]->nb_index_entries; ret++) {
                if(1 == ctx->streams[streamVideoIndex]->index_entries[ret].flags) {
                    count++;
                }
            }
            if(count > index_size) {
                if( RET_OK == (ret = mk_dir(indexfilepath,false)) ) {
                    ret = save_indexfile(indexfilepath,seek_over);
                }
                if(ret < 0) {
                    remove(indexfilepath);
                }
            }
            GSLOGD("save indexfile ret:%d count:%d index_size:%d seek_over:%d err:%d",ret,count,index_size,seek_over,errno);
        }
#endif
        av_close_input_file(ctx);
        ctx = NULL;
        ZXLOGD("av_close_input_file(ctx:%x)", ctx);
    }

    if(esds!=NULL) {
        delete[] esds;
        esds = NULL;
    }
    esds_size = 0;
}

/* add by gs
 *  * forward_seek get the keyframe after the seek time
 *   * backward_seek get the keyframe before the seek time
 *    */
#define FORWARD_SEEK 0
#define BACKWARD_SEEK 1

int FFmpeg::seek_video_backward(int64_t seekdts, int64_t curdts, AVStream* st) {
    int64_t last_keyframe_before_seekdts, first_keyframe_after_seekdts, seek_to_dts;
    int keyframe_count_between_seekdts_curdts,i;
    first_keyframe_after_seekdts = AV_NOPTS_VALUE;
    keyframe_count_between_seekdts_curdts = 0;
    for(i = 0; i < st->nb_index_entries; i++) {
        if(1 == st->index_entries[i].flags && st->index_entries[i].timestamp <= seekdts) {
            last_keyframe_before_seekdts = st->index_entries[i].timestamp;
        }
        if(AV_NOPTS_VALUE == first_keyframe_after_seekdts
            && 1 == st->index_entries[i].flags
            && st->index_entries[i].timestamp >= seekdts) {
            first_keyframe_after_seekdts = st->index_entries[i].timestamp;
        }
        if(1 == st->index_entries[i].flags
            && st->index_entries[i].timestamp >= seekdts
            && st->index_entries[i].timestamp <= curdts) {
            keyframe_count_between_seekdts_curdts++;
        }
        if(st->index_entries[i].timestamp >= curdts) {
            break;
        }
    }
    /* if only one keframe between seekdts & curdts
     * seek to last_keyframe_before_seekdts to avoid seek loop.
     * else seek to the nearest keyframe around seektime
     */
    /*
    if(keyframe_count_between_seekdts_curdts<2) {
        seek_to_dts = last_keyframe_before_seekdts;
    } else
    */
    if(seekdts-last_keyframe_before_seekdts < first_keyframe_after_seekdts-seekdts) {
        seek_to_dts = last_keyframe_before_seekdts;
    } else {
        seek_to_dts = first_keyframe_after_seekdts;
    }
    int ret = avformat_seek_file(ctx, streamVideoIndex, first_video_dts, seek_to_dts, seek_max, FORWARD_SEEK);
    GSLOGD("seek video backward return:%d seek_time:%lld curVDTS:%lld realseektime:%lld lastkeyframe:%lld firstkeyframe:%lld keycount:%d",
           ret,seekdts,curdts,seek_to_dts,last_keyframe_before_seekdts,first_keyframe_after_seekdts,keyframe_count_between_seekdts_curdts);
    return ret;
}

int FFmpeg::seek(const int64 time, const int64_t curVideoDTS) {
    if(NULL != ctx) {
        int ret,ffmpeg_seek_flag;
        int64_t seek_time,cur_pos;
/* add by gs
 * original seek strategy :
 * a. seek to seektime normally
 * b. if a failed seek to seektime backward
 * c. if b failed seek to curtime backward
 *
 * for cts test, we need do seek like android
 * so we change the strategy part a
 * a. if seek is BACKWARD, seek to the nearest keyframe around seektime,
 *    otherwise seek to seektime FORWARD
 */
        ffmpeg_seek_flag = FORWARD_SEEK;//NEARBY_SEEK;
        ret = RET_ERR;
        if(streamVideoIndex >= 0) {
            seek_time = av_rescale_q(time, AV_TIME_BASE_Q, ctx->streams[streamVideoIndex]->time_base);
            if(curVideoDTS > 0 && curVideoDTS > seek_time) {
                ret = seek_video_backward(seek_time+first_video_dts,curVideoDTS+first_video_dts,ctx->streams[streamVideoIndex]);
            } else {
                ret = avformat_seek_file(ctx, streamVideoIndex, first_video_dts, seek_time+first_video_dts, seek_max, ffmpeg_seek_flag);
                GSLOGD("seek video return:%d time:%lld seek_time:%lld curVDTS:%lld flag:%d",
                   ret,time,seek_time,curVideoDTS,ffmpeg_seek_flag);
                if(ret < 0 && BACKWARD_SEEK != ffmpeg_seek_flag) {
                    ret = avformat_seek_file(ctx, streamVideoIndex, first_video_dts, seek_time+first_video_dts, seek_max,BACKWARD_SEEK);
                    GSLOGD("reseek video backward return:%d",ret);
                }
            }
            if(ret != 0) {
                ret = avformat_seek_file(ctx, streamVideoIndex, first_video_dts, curVideoDTS+first_video_dts, seek_max,BACKWARD_SEEK);
                GSLOGD("reseek video backward to curdts return:%d time:%lld seek_time:%lld curVDTS:%lld",
                       ret,time,seek_time,curVideoDTS);
            }
        } else if(streamAudioIndex >= 0){
            ffmpeg_seek_flag = BACKWARD_SEEK;
            seek_time = av_rescale_q(time, AV_TIME_BASE_Q, ctx->streams[streamAudioIndex]->time_base);
            ret = avformat_seek_file(ctx, streamAudioIndex, first_audio_dts, seek_time+first_audio_dts, seek_max, ffmpeg_seek_flag);
            GSLOGD("seek audio return:%d time:%lld seek_time:%lld curVDTS:%lld flag:%d",
                   ret,time,seek_time,curVideoDTS,ffmpeg_seek_flag);
            if(ret != 0) {
                ret = avformat_seek_file(ctx, streamAudioIndex, first_audio_dts, first_audio_dts, seek_max, ffmpeg_seek_flag);
                GSLOGD("reseek audio return:%d time:%lld seek_time:%lld curVDTS:%lld flag:%d",
                   ret,time,seek_time,curVideoDTS,ffmpeg_seek_flag);
            }
        }
        if(RET_OK == ret)
            seek_flag = 1;
        return ret;
    }
    return RET_ERR;
}

const int FFmpeg::getVideoStreamIndex() const {
   // BacktraceAssert(streamVideoIndex >= 0);
    if (streamVideoIndex < 0) {
        ZXLOGD("warning: getVideoStreamIndex() return invalidate value.\n");
    }
    return streamVideoIndex;
}

const int FFmpeg::getAudioStreamIndex() const {
   // BacktraceAssert(streamAudioIndex >= 0);
    if (streamAudioIndex < 0) {
        ZXLOGD("error:  getAudioStreamIndex() return invalidate value.\n");
   //     BacktraceAssert(0);
    }
    return streamAudioIndex;
}

const int64 FFmpeg::getDuration() const {
    return duration;
}

const int64 FFmpeg::getAudioDuration() const {
    return audioDuration;
}

const int64 FFmpeg::getVideoDuration() const {
    return videoDuration;
}

const float FFmpeg::getAudioScale() const {
    // if need to update value ,please call calcScale;
    return scaleAudio;
}

const float FFmpeg::getVideoScale() const {
    // if need to update value ,please call calcScale;
    return scaleVideo;
}

const int   FFmpeg::getStreamCount() const {
    return ((ctx) ? ctx->nb_streams : 0x0);
}

const int   FFmpeg::getStreamType(const int streamId) const {
    BacktraceAssert(streamId >= 0 && streamId < ctx->nb_streams);

    return ((ctx) ? ctx->streams[streamId]->codec->codec_type : 0x0);
}

const int64 FFmpeg::getStreamDuration(const int streamId) const {
    if(!ctx)
        return 0x0;

    BacktraceAssert(streamId >= 0 && streamId < ctx->nb_streams);
    return duration;
//    int64 d ;
//    if(AV_NOPTS_VALUE==ctx->streams[streamId]->duration)
//        d = ctx->duration;
//    else if (ctx->duration>ctx->streams[streamId]->duration)
//        d = ctx->duration;
//    else
//    {
//        d = ctx->streams[streamId]->duration;
//        AVRational r = ctx->streams[streamId]->time_base;
//        d = d * r.num * 1E6 / float(r.den);
//    }
//    return d;
}

const int   FFmpeg::getCodecID(const int streamId) const {
    if(!ctx)
        return 0x0;

    BacktraceAssert(streamId >= 0 && streamId < ctx->nb_streams);

    return ctx->streams[streamId]->codec->codec_id;
}

uint32_t FFmpeg::getVideoRotate(const int streamId) {
    AVDictionaryEntry* ent = NULL;

    if (ctx && streamId >= 0
            && ctx->nb_streams > streamId) {
        AVStream* st;
        st  = ctx->streams[streamId];
        ent = av_dict_get(st->metadata, "rotate", ent, AV_DICT_IGNORE_SUFFIX);
    }

    if (ent && ent->value) {
        return atoi(ent->value);
    }

    return 0;
}

AVDictionaryEntry* FFmpeg::getDictionary(AVDictionaryEntry* tag)
{
    if((ctx) && (ctx->metadata)) {
        return av_dict_get(ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX);
    } else if (ctx
            && 1 == ctx->nb_streams
            && streamAudioIndex >= 0
            && ctx->streams[streamAudioIndex]->metadata){
        return av_dict_get(ctx->streams[0]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX);
    }
    return NULL;
}

const char* FFmpeg::getName()
{
    if(ctx)
    {
        if(ctx->iformat)
        {
            return ctx->iformat->name;
        }
    }

    return NULL;
}

void FFmpeg::calcScale() {

    if(!ctx)
        return;

    struct AVRational ra;

    if (streamVideoIndex >= 0 ) {
        ra = ctx->streams[streamVideoIndex]->time_base;
        scaleVideo = ra.num / (float)ra.den;
    }
    else  {
        ZXLOGD("error: don't calc video scale.\n");
    }

    if (streamAudioIndex >= 0) {
        ra = ctx->streams[streamAudioIndex]->time_base;
        scaleAudio = ra.num / (float)ra.den;
    }
    else {
        ZXLOGD("error: don't calc audio  scale.\n");
    }
}

const char* const FFmpeg::getPath() const {
    return this->path;
}

int FFmpeg::discard_unused_streams()
{
    if(!ctx)
        return RET_ERR;
    for(int i=0;i<ctx->nb_streams;i++) {
        if(i!=streamAudioIndex && i!=streamVideoIndex)
            ctx->streams[i]->discard = AVDISCARD_ALL;
//        else
//            ctx->streams[i]->discard = AVDISCARD_NONE;
    }
    return RET_OK;
}

void FFmpeg::findAVIndex() {
    if(!ctx)
        return;

    BacktraceAssert(ctx->nb_streams > 0);

    if( (streamVideoIndex = getBestStream(AVMEDIA_TYPE_VIDEO,-1) ) < 0 )
        streamVideoIndex = -1;

    if( (streamAudioIndex = getBestStream(AVMEDIA_TYPE_AUDIO,streamVideoIndex) ) < 0 )
        streamAudioIndex = -1;

    discard_unused_streams();
    //assert(streamAudioIndex >= 0 || streamVideoIndex >=0);
    GSLOGD("bset v:%d a:%d",streamVideoIndex,streamAudioIndex);
}

const int64 FFmpeg::videoTimestamp(const int64 ts) {
    //printf("video scale :%lf, ts:%lld, timestamp:%lld\n", getVideoScale(), ts, ts * getVideoScale());
    return int64(getVideoScale() * 1E6 * ts);
}

const int64 FFmpeg::audioTimestamp(const int64 ts) {
    return int64(getAudioScale() * 1E6 * ts);
}

struct AVStream* FFmpeg::videoStream() {
    if(!ctx)
        return NULL;

    const int id = getVideoStreamIndex();
    if (id >= 0) {
        return ctx->streams[id];
    }
    else {
        //assert(0);
        return NULL;
    }
}

struct AVStream* FFmpeg::audioStream() {
    if(!ctx)
        return NULL;

    const int id = getAudioStreamIndex();
    if (id >= 0) {
        return ctx->streams[id];
    }
    else {
        //assert(0);
        return NULL;
    }
}

void FFmpeg::findDuration() {
    if(!ctx) {
        this->duration = -1;
        this->audioDuration = -1;
        this->videoDuration = -1;
        return;
    }

    AVStream* stm = NULL;
    this->duration = -1;
    this->audioDuration = -1;
    this->videoDuration = -1;
    int64_t adts,vdts,sdts;
    bool hasvideo = false;
    adts=vdts=seek_max;
    duration = ctx->duration;// + 500000;
    stm = videoStream();
    if (stm) {
        hasvideo = true;
        //if(stm->start_time!=AV_NOPTS_VALUE)
        //    vdts = av_rescale_q(stm->start_time, stm->time_base, AV_TIME_BASE_Q);
        //else
        if(stm->first_dts!=AV_NOPTS_VALUE)
            vdts = av_rescale_q(stm->first_dts, stm->time_base, AV_TIME_BASE_Q);
        max_video_dts = stm->duration;
        if(AV_NOPTS_VALUE==stm->duration) {
            videoDuration = duration;
        } else {
            videoDuration = videoTimestamp(stm->duration);// + 500000;
            duration = videoDuration;
            //GSLOGD("video time_base num:%d den:%d",stm->time_base.num,stm->time_base.den);
        }
    }
    stm = audioStream();
    if (stm) {
        if(hasvideo) {
            audioDuration = duration;
        } else {
            audioDuration = duration = audioTimestamp(stm->duration);// + 500000;
        }
        if(stm->first_dts!=AV_NOPTS_VALUE)
            adts = av_rescale_q(stm->first_dts, stm->time_base, AV_TIME_BASE_Q);
        max_audio_dts = stm->duration;
        //GSLOGD("audio time_base num:%d den:%d",stm->time_base.num,stm->time_base.den);
    }

    if(adts<vdts) {
       //after_seek_first_video = vdts;
        sdts = adts;
    } else {
        after_seek_first_video = vdts;
        sdts = vdts;
    }
    if(seek_max==sdts) {
        sdts = 0;
        after_seek_first_video = 0;
    }
    stm = videoStream();
    if(stm!=NULL)
        first_video_dts = av_rescale_q(sdts,AV_TIME_BASE_Q, stm->time_base);
    stm = audioStream();
    if(stm!=NULL)
        first_audio_dts = av_rescale_q(sdts,AV_TIME_BASE_Q, stm->time_base);

    GSLOGD("gsInfo starttime:%lld ctx->duration:%lld duration:%lld ad:%lld audio_duration:%lld vd:%lld video_duration:%lld adts:%lld vdts:%lld sdts:%lld afdts:%lld vfdts:%lld asfv:%lld ",
           ctx->start_time,ctx->duration,duration,max_audio_dts,audioDuration,max_video_dts,videoDuration,adts,vdts,sdts,first_audio_dts,first_video_dts,after_seek_first_video);
}

int FFmpeg::readPacket(struct AVPacket* pkt) {
    GSLOGD("function in.");
    BacktraceAssert(pkt);
    BacktraceAssert(ctx);
    int ret;
retry:
    ret =  av_read_frame(ctx, pkt);
    if (ret < 0) {
        bEOFError = true;
        ZXLOGD("readPacket() error.");
        char errbuf[500];
        av_strerror(ret, (char *)errbuf, 500);
        ZJFLOGD("read error, %s", errbuf);
        return AVERROR_EOF;
    }
    if (streamVideoIndex >= 0 && streamAudioIndex >= 0) {
        if(1 == seek_flag) {
            if(AVMEDIA_TYPE_VIDEO != ctx->streams[pkt->stream_index]->codec->codec_type) {
                GSLOGD("gsQ drop audio before video dts:%lld",pkt->dts);
                av_free_packet(pkt);
                goto retry;
            }
            seek_flag = 0;
            if(AV_NOPTS_VALUE != pkt->dts) {
                after_seek_first_video = pkt->dts;
            } else {
                after_seek_first_video = pkt->pts;
            }
            after_seek_first_video = av_rescale_q(after_seek_first_video, ctx->streams[pkt->stream_index]->time_base, AV_TIME_BASE_Q);/* + NUFRONT_AUDIO_DELAY; */
            GSLOGD("gsQ read after seek after_seek_first_video:%lld pts:%lld dts:%lld duration:%d size:%d flags:%d ret:%d",
              after_seek_first_video,pkt->pts,pkt->dts,pkt->duration,pkt->size,pkt->flags,ret);
        }
        else if(0 != after_seek_first_video && AVMEDIA_TYPE_AUDIO == ctx->streams[pkt->stream_index]->codec->codec_type) {
            /* tmpdts = next audio timestamp */
            int64_t tmpdts = av_rescale_q(pkt->dts + pkt->duration, ctx->streams[pkt->stream_index]->time_base, AV_TIME_BASE_Q) - NUFRONT_AUDIO_DELAY;

            /* if audio timestamp before video, drop the packet */
            if(tmpdts < after_seek_first_video) {
                GSLOGD("gsQ drop audio dts:%lld audio_time:%lld",pkt->dts,tmpdts);
                av_free_packet(pkt);
                goto retry;
            } else if(first_audio_flag) {
                /*
                 * for first audio packet,
                 * we read it out right now to init audio decoder,
                 * then add silence packet at second packet
                 */
                first_audio_flag = 0;
                after_seek_first_video = av_rescale_q(after_seek_first_video, AV_TIME_BASE_Q, ctx->streams[pkt->stream_index]->time_base);
                pkt->dts = after_seek_first_video;
                after_seek_first_video += pkt->duration;
                after_seek_first_video = av_rescale_q(after_seek_first_video,ctx->streams[pkt->stream_index]->time_base,AV_TIME_BASE_Q);
                GSLOGD("gsQ first audio add silence next audio, reset dts:%lld after_seek_first_video:%lld",pkt->dts,after_seek_first_video);
            } else if(tmpdts >= after_seek_first_video) {
                /*
                 * if next audio behind video
                 * we check cur packet to add silence or not
                 */

                /* real video time we need*/
                after_seek_first_video += NUFRONT_AUDIO_DELAY;

                /*
                 * tmpdts = next audio timestamp
                 * after_seek_first_video = real video timestamp
                 * so we will get the gap between them
                 */
                tmpdts = tmpdts - after_seek_first_video;

                /*
                 * get the real video timestamp in the audio time base,
                 * which is the first dts we will set to the audio
                 */
                after_seek_first_video = av_rescale_q(after_seek_first_video, AV_TIME_BASE_Q, ctx->streams[pkt->stream_index]->time_base);


                if(CODEC_ID_AMR_WB != ctx->streams[pkt->stream_index]->codec->codec_id &&
                      CODEC_ID_AMR_NB != ctx->streams[pkt->stream_index]->codec->codec_id ) {
                    /*
                     * if pkt->dts > after_seek_first_video,
                     * we need add silence packet before cur packet
                     * else if tmpdts > 0, we need add silence packet and drop cur dts
                     * else we pass cur dts to decode
                     */
                    if(pkt->dts > after_seek_first_video) {
                        if(after_seek_first_video > 0) {
                            pkt->pts = after_seek_first_video;
                            pkt->flags = AUDIO_BEHIND_VIDEO_AFTER_SEEK;
                        }
                    } else if(tmpdts > 0) {
                        /*
                         * if AUDIO_BEFORE_VIDEO_AFTER_SEEK
                         * we use pkt->pts to pass silence duration
                         */
                        pkt->dts = after_seek_first_video;
                        pkt->pts = tmpdts;
                        pkt->flags = AUDIO_BEFORE_VIDEO_AFTER_SEEK;
                    }
                } else {
                    /* amr is not decoded by ffmpeg, not need add silence but drop unused packet direct */
                    if(pkt->dts < after_seek_first_video) {
                        GSLOGD("gsQ drop amr audio audio_dts:%lld video_dts:%lld duration:%d",pkt->dts,after_seek_first_video,pkt->duration);
                        av_free_packet(pkt);
                        goto retry;
                    }
                }
                GSLOGD("gsQ add silence pts:%lld dts:%lld flag:%d tmpdts:%lld",
                       pkt->pts,pkt->dts,pkt->flags,tmpdts);
                after_seek_first_video = 0;
            }
        }
        else if(CODEC_ID_WMAPRO == ctx->streams[pkt->stream_index]->codec->codec_id &&
           ( strstr(ctx->iformat->name,"matroska")!=NULL || strstr(ctx->iformat->name,"webm")!=NULL) ) {
            pkt->dts = AV_NOPTS_VALUE;
        }
    }
/* add by gs make ts start dts=0 */
    if(pkt->stream_index == streamVideoIndex) {
        if(AV_NOPTS_VALUE != pkt->dts) {
            pkt->dts -= first_video_dts;
        }
        if(AV_NOPTS_VALUE != pkt->pts) {
            pkt->pts -= first_video_dts;
        }
    } else if (pkt->stream_index == streamAudioIndex) {
        first_audio_flag = 0;
        if(AV_NOPTS_VALUE != pkt->dts) {
            pkt->dts -= first_audio_dts;
        }
        /*
        if(AV_NOPTS_VALUE != pkt->pts) {
            pkt->pts -= first_audio_dts;
        }
        */
    }
    GSLOGD("function out, dts is %lld, duration is %d", pkt->dts, pkt->duration);
    return RET_OK;
}

void FFmpeg::initPacket(struct AVPacket* pkt) {
    BacktraceAssert(pkt);
    av_init_packet(pkt);
}

void FFmpeg::freePacket(struct AVPacket* pkt) {
    BacktraceAssert(pkt);
    av_free_packet(pkt);
}

AVStream* FFmpeg::stream(const int id) {
    BacktraceAssert(ctx);

    if(ctx)
    {
        BacktraceAssert(id >=0 && id < ctx->nb_streams);

        return ctx->streams[id];
    }

    return NULL;
}

const bool FFmpeg::isFEof() {
    //android::AutoMutex _am(ctxMutex);
    if (url_feof(ctx->pb)) {
        ZXLOGD("isFEof(),  at file end.");
    }
    if (url_ferror(ctx->pb)) {
        ZXLOGD("isFEof(), file handle error.");
    }
    return url_feof(ctx->pb);//|| url_ferror(ctx->pb);
}


float FFmpeg::getTimescale(const int sid) {
    BacktraceAssert(ctx);

    if(ctx)
    {
        BacktraceAssert(sid >= 0 && sid <ctx->nb_streams);

        AVRational ra = ctx->streams[sid]->time_base;
        return ra.num * 1E6 / float(ra.den);
    }

    return 0;
}

const int FFmpeg::getBestStream(const enum AVMediaType type, int related_stream) const {
    int n = 0;
    n = av_find_best_stream(ctx, type, -1, related_stream, NULL, 0);
    return n;
}

AVRational FFmpeg::getVideoTimeBase() {
    AVStream* stm = videoStream();
    if(stm==NULL)
        return AVRational{0,1};
    return stm->time_base;
}
/*a@nufront: match 3gp file format,get "location" from dict which is filled by libavformat\mov.c*/
AVDictionaryEntry*  FFmpeg::getLocation(void)
{
    if(ctx && ctx->iformat)
    {
        if(av_match_ext(".3gp", ctx->iformat->name) && ctx->metadata)
        {
            return av_dict_get(ctx->metadata,"location", NULL, 0);
        }
    }
    return NULL;
}
/*a@nufront: end*/

