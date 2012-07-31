#include "FFmpegSafeReader.h"
#include <unistd.h>
#include <utils/nufrontlog.h>

#define LOG_TAG "FFmpegSafeReader"

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_AUDIOQ_SIZE (20 * 16 * 1024)
#define MIN_FRAMES 5

namespace FFMPEG
{

AVPacket FFmpegSafeReader::flush_pkt;


FFmpegSafeReader::FFmpegSafeReader(const char* p) :
    ffmpeg(NULL),
    path(NULL),
    mSeeking(false),
    mSeekingProcess(false),
    bExit(false),
    bStart(false),
    mutexSeek(new Mutex),
    condSeek(new Condition),
    mLastSeekTime(-1),
    mRef(0),
    run_process_count(0),
    mHasAudio(false),
    mHasVideo(false),
    mStart(false),
    mIsEof(false),
    mutexStart(new Mutex),
    condStart(new Condition),
    mutexRead(new Mutex),
    condRead(new Condition),
    mStreamSeekFlag(0),
    mAudioEos(false),
    mVideoEos(false),
    curVideoDTS(0),
    mStopReadOutFlag(false),
    mOpenFailed(false)
{

    assert(p);
    int n = strlen(p);
    path = new char[n + 1];
    memcpy(path, p, n);
    path[n] = 0;

    packet_queue_init(&videoQ);
    packet_queue_init(&audioQ);

    ZXLOGD("ffmpeg.1");
    ffmpeg = new FFmpeg(path);
    {
        ZXLOGD("ffmpeg.2");
        Mutex::Autolock autolock(ffmpeg->mutex());
        ffmpeg->initPacket(&mPkt);
    }

    if (ffmpeg->IsOpenFailed()) {
        mOpenFailed = true;
        return;
    } else {
        mOpenFailed = false;
    }

    startThread();
    ZXLOGD("constructor.");
}

FFmpegSafeReader::~FFmpegSafeReader()
{
    ZJFLOGD("funciton in.");
    exit();

    if (ffmpeg)
    {
        delete ffmpeg;
        ffmpeg = NULL;
    }

    packet_queue_end(&videoQ);
    packet_queue_end(&audioQ);

    if (path)
    {
        delete[] path;
        path = NULL;
    }
    if (mutexSeek)
    {
        delete mutexSeek;
    }
    if (condSeek)
    {
        delete condSeek;
    }
    if (mutexStart) {
        delete mutexStart;
    }
    if (condStart) {
        delete condStart;
    }
    if (mutexRead) {
        delete mutexRead;
    }
    if (condRead) {
        delete condRead;
    }

    ZXLOGD("destructor.");
    ZJFLOGD("funciton out.");
}

FFmpeg* FFmpegSafeReader::getffmpeg() const
{
    assert(ffmpeg);
    return ffmpeg;
}

void FFmpegSafeReader::exit()
{
    if (bExit)
    {
        return;
    }

    ZXLOGD("exit();");

    bExit  = true;
    if (bExit) {
        AutoMutex _am(*mutexStart);
        mStart = true;
        condStart->signal();
    }
    if (bExit) {
        AutoMutex _am(*mutexSeek);
        condSeek->signal();
    }
    if (bExit) {
        AutoMutex _am(*mutexRead);
        condRead->signal();
    }

    bStart = false;
    packet_queue_abort(&videoQ);
    packet_queue_abort(&audioQ);

    // wait thread over;
    pthread_join(mThread, NULL);
    ZXLOGD("mThread is quit.");

}

FFmpegSafeReader* FFmpegSafeReader::create(const char* p)
{
    FFmpegSafeReader* r =  new FFmpegSafeReader(p);
    r->add_ref();
    return r;
}

void FFmpegSafeReader::add_ref()
{
    AutoMutex _am(mutexRef);
    mRef++;
}

void FFmpegSafeReader::release_ref()
{
    ZJFLOGD("function in.");
    int n = 0;
    {
        AutoMutex _am(mutexRef);
        n = mRef--;
        if (n <= 0) return;
    }
    if (n == 1)
    {
        delete this;
    }
    ZJFLOGD("n is %d", n);
    ZJFLOGD("function out.");
}

void FFmpegSafeReader::startThread()
{


    ZXLOGD("startThread()...");
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    int n = pthread_create(&mThread, &attr, run, this);
    assert(n == 0);
    pthread_attr_destroy(&attr);
    ZXLOGD("startThread, usleep(20). new thread:%d, ret:%d", mThread, n);
    //mWaiter.wait(100);
    //usleep(20);
}

int FFmpegSafeReader::readVideoPacket(AVPacket* pkt)
{
    //assert(mStart);

    while(mSeeking || mStopReadOutFlag) {
        ZXLOGD("don't read video packet , at seeking.");
        //usleep(10);
        mWaiter.wait(10);
    }

    {
        Mutex::Autolock autolock(mutexRef);

        if (bExit)
        {
            if (videoQ.nb_packets <= 0)
            {
                return RET_ERR;
            }
        }
        ZXLOGD("read video packet.");
    }
    while(true) {
        const int n = packet_queue_element_count(&videoQ);
        if ((n >= 1)||(videoQ.bIsEos))
            break;
        mWaiter.wait(1000);
    }
    if(videoQ.bIsEos) {
       ZXLOGD("video have read ending NULL pkt");
       return RET_ERR;
    }
    int ret =  packet_queue_get(&videoQ, pkt, true);
    ZXLOGD("gsQout index:%d dts:%lld pts:%lld ret:%d",
         pkt->stream_index,pkt->dts,pkt->pts,ret);
//add by gs
    if(ret==1)
        curVideoDTS = pkt->dts;
///////////
    return ret;
}

int FFmpegSafeReader::readAudioPacket(AVPacket* pkt)
{
    //assert(mStart);

    while(mSeeking || mStopReadOutFlag) {
        GSLOGD("don't read audio packet , at seeking. mSeekingL:%d mStopReadOutFlag:%d sys:%lld",mSeeking,mStopReadOutFlag,systemTime());
        mWaiter.wait(10);
        //usleep(10);
    }
    /* 0x01 means VIDEO_SEEK doing
     * 0x02 means AUDIO_SEEK doing
     * if mStreamSeekFlag==0x01 ||  mStreamSeekFlag==0x02
     * it means seeking is not finished
     * we add seek_packet to avoid real packet be read out
     */
    if(mHasAudio && mHasVideo
        && (0x01 == mStreamSeekFlag || 0x02 == mStreamSeekFlag)) {
        av_new_packet(pkt,5);
        memcpy((void*)pkt->data,"seek",4);
        pkt->dts = 0;
        pkt->flags = 1;
        GSLOGD("add seek packet sys:%lld",systemTime());
        return RET_OK;
    }

    {
        Mutex::Autolock autolock(mutexRef);
        int npkt = packet_queue_element_count(&audioQ);
        if (bExit) {
            ZXLOGD("read thread is exited.");

            if (npkt <= 0)
                return RET_ERR;
        }
        ZXLOGD("read audio packet.");
    }
    while(true) {
        const int n = packet_queue_element_count(&audioQ);
        if ((n >= 1)||(audioQ.bIsEos))
            break;
        mWaiter.wait(1000);
    }
    if(audioQ.bIsEos) {
       ZXLOGD("audio have read ending NULL pkt");
       return RET_ERR;
    }
    int ret = packet_queue_get(&audioQ, pkt, true);

    ZXLOGD("readaudiopacket::gsQout index:%d dts:%lld pts:%lld ret:%d",
         pkt->stream_index,pkt->dts,pkt->pts,ret);
    return ret;
}

int FFmpegSafeReader::seek(int64_t tm, const int streamType)
{
    ZJFLOGD("function in.");
    ZJFLOGD("tm is %lld", tm);
    int ret = RET_OK;
    mStopReadOutFlag = true;
    {
    AutoMutex _am(*mutexSeek);
    if (mHasAudio && mHasVideo) {
        if (mStreamSeekFlag == 0x3) {
            mStreamSeekFlag = 0x0;
        }
        if (FFmpegStreamInfo::StreamTypeVideo == streamType) {
            mStreamSeekFlag |= 0x1;
        }
        if (FFmpegStreamInfo::StreamTypeAudio == streamType) {
            mStreamSeekFlag |= 0x2;
        }
    }
    ZJFLOGD("mStreamSeekFlag is %d", mStreamSeekFlag);
    if (mSeeking)
    {
        ZXLOGD("at before seeked, and wait, mSeeking is %d, mLastSeekTime is %lld, tm is %lld.", mSeeking, mLastSeekTime, tm);
        /*
        int n = 10000;
        while(n--)
        {
            mWaiter.wait(10);
        }
        if (n <= 0)
        {
            ZXLOGD("wait mSeeking state 2 second. please checked!!");
        }*/
        ZJFLOGD("will  wait condSeek signal, mSeeking is %d, mStreamSeekFlag is %d", mSeeking, mStreamSeekFlag);
        condSeek->waitRelative(*mutexSeek, 300000000);
        if (0x3 == mStreamSeekFlag) {
            mStopReadOutFlag = false;
            return ret;
        }
    } else if (0x3 == mStreamSeekFlag){
        mStopReadOutFlag = false;
        return ret;
    }
    if (bExit)
    {
        ZXLOGD("read packet thread end, don't seek.");
        mSeeking = false;
        mStopReadOutFlag = false;
        return ret;
    }


    mSeeking = true;
    if (mHasVideo) {
        mVideoEos = false;
        packet_queue_flush(&videoQ);
    }

    if (mHasAudio) {
        mAudioEos = false;
        packet_queue_flush(&audioQ);
    }
    mLastSeekTime = tm;

    ZXLOGD("seek %lld", tm);

    if (ffmpeg)
    {
        mSeekingProcess = true;
        {
            Mutex::Autolock autolock(ffmpeg->mutex());
//add by gs
            ret = ffmpeg->seek(tm,curVideoDTS);
//////////
        }
        ZXLOGD("seek  %lld ok.", tm);
        mSeekingProcess = false;
        mIsEof = false;
        condSeek->signal();
    }
    }
    {
        AutoMutex _am(*mutexRead);
        condRead->signal();
    }
    mStopReadOutFlag = false;
    return ret;
}

const bool FFmpegSafeReader::isEnd()
{
    AutoMutex autolock(mutexRef);
    ZXLOGD("audioQ.packets:%d, videoQ.packets:%d, bExit:%d", audioQ.nb_packets, videoQ.nb_packets, bExit);
    return  (audioQ.nb_packets <= 0 && videoQ.nb_packets <= 0);
}

void* FFmpegSafeReader::run_process()
{
    ZJFLOGD("function in.");
    ZXLOGD("run_process enter.");
    {
        Mutex::Autolock autolock(mutexProcess);
        this->run_process_count ++;
        condProcess.signal();
        if (run_process_count > 1)
        {
            ZXLOGD("run_process connot run, will quit.");
            return NULL;
        }
    }
    while(!mStart){
        AutoMutex _am(*mutexStart);
            if(!mStart) {
                ZXLOGD("start read thread, wait start signal.");
                condStart->waitRelative(*mutexStart, 3000000000);
                continue;
            }
    }
    ZJFLOGD("mHasAudio is %d, mHasVideo is %d", mHasAudio, mHasVideo);
    while (!bExit)
    {
        ZJFLOGD("begine while, mSeeking is %d, iseof is %d", mSeeking, mIsEof);
        //AutoMutex _am(*mutexSeek);
        {
            AutoMutex _am(*mutexSeek);
            if (mSeeking)
            {
                //packet_queue_flush(&videoQ);
                //packet_queue_flush(&audioQ);
                if (mSeekingProcess)
                {
                    ZXLOGD("seeking,   wait seek signal.");
                    condSeek->waitRelative(*mutexSeek, 300000000);
                    continue;
                }
                mSeeking = false;
                //packet_queue_flush(&videoQ);
                //packet_queue_flush(&audioQ);
                condSeek->signal();

                //packet_queue_abort(&videoQ);
                //packet_queue_abort(&audioQ);
                //packet_queue_flush(&videoQ);
                //packet_queue_flush(&audioQ);
                ZXLOGD("continue seek. flush packet queue.");
            }

            if (!mSeeking && mIsEof) {
                ZJFLOGD("will wait for iseof");
                condSeek->waitRelative(*mutexSeek, 300000000);
                ZJFLOGD("after iseof wait");
                continue;
            }
        }
        if (ffmpeg)
        {
            ZJFLOGD("ffmpeg is true.");
            //const int audio_num = packet_queue_element_count(&audioQ);
            //const int video_num = packet_queue_element_count(&videoQ);
            {
            AutoMutex _am(*mutexRead);
            if (audioQ.size + videoQ.size > (MAX_QUEUE_SIZE)
                || ((audioQ.size  > MIN_AUDIOQ_SIZE || !mHasAudio || mAudioEos)
                    && (videoQ.nb_packets > MIN_FRAMES || !mHasVideo || mVideoEos))) {
                if (mHasAudio && audioQ.size == 0 && !mAudioEos) {
                    packet_queue_put(&audioQ, NULL);
                    mAudioEos = true;
                }
                if (mHasVideo && videoQ.nb_packets == 0 && !mVideoEos) {
                    packet_queue_put(&videoQ, NULL);
                    mVideoEos = true;
                }

                ZJFLOGD("audioQ.size + videoQ.size is %d", audioQ.size + videoQ.size );
                ZJFLOGD("audioQ.size is %d", audioQ.size);
                ZJFLOGD("videoQ.nb_packets is %d", videoQ.nb_packets);
                //ZXLOGD("zx, read packet delay.  usleep(10)");
                // mWaiter.wait(10000);
                condRead->waitRelative(*mutexRead, 300000000);
                continue;
            }
            }
            AVPacket *pkt=&mPkt;
            int ret = -1;
            {
            AutoMutex _am(*mutexSeek);
            if (!mIsEof){
            {
                Mutex::Autolock autolock(ffmpeg->mutex());
                ret = ffmpeg->readPacket(pkt);
            }
            if (ret >= 0)
            {
                ZXLOGD("ffmpeg->readPacet() ok.   audio_num: %d, audio_size: %d video_num:%d video_size : %d, totalsize: %d", audioQ.nb_packets, audioQ.size, videoQ.nb_packets, videoQ.size , audioQ.size + videoQ.size);
                AVStream* stm;
                {
                    Mutex::Autolock autolock(ffmpeg->mutex());
                    stm = ffmpeg->stream(pkt->stream_index);
                }
                if (stm->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
                        mHasAudio &&
                        !mAudioEos &&
                        pkt->stream_index == ffmpeg->getAudioStreamIndex())
                {
                    ZJFLOGD("will packet_queue_put for audio, pkt->flags is %d, pkt->pts is %d, pkt->dts is %d", pkt->flags, pkt->pts, pkt->dts);
                    if (AUDIO_BEFORE_VIDEO_AFTER_SEEK == pkt->flags) {
                        AVPacket tmpPktEntry;
                        av_new_packet(&tmpPktEntry,30);
                        sprintf((char*)tmpPktEntry.data,"silencepkt:%lld",pkt->pts);
                        tmpPktEntry.dts = pkt->dts;
                        tmpPktEntry.stream_index = pkt->stream_index;
                        ret = packet_queue_put(&audioQ,&tmpPktEntry);
                        GSLOGD("add extra3 silencepkt dts:%lld duration:%lld ret:%d sys:%lld",tmpPktEntry.dts,pkt->pts,ret,systemTime());
                    } else {
                        if (AUDIO_BEHIND_VIDEO_AFTER_SEEK == pkt->flags && pkt->pts < pkt->dts && 0 != pkt->duration){
                            AVPacket tmpPktEntry;
                            int i = 0, silence_pkt_num = 0;
                            int64_t tmp_duration = av_rescale_q(pkt->duration, stm->time_base, AV_TIME_BASE_Q);
                            CHECK(pkt->duration);
                            silence_pkt_num = (pkt->dts - pkt->pts) / pkt->duration;
                            ZJFLOGD("pkt->flags is 2, pkt->pts is %lld, pkt->dts is %lld, silence_pkt_num is %d, pkt->duration is %d", pkt->pts, pkt->dts, silence_pkt_num, pkt->duration);
                            for (i = 0; i < silence_pkt_num; i++) {
                                av_new_packet(&tmpPktEntry,30);
                                sprintf((char*)tmpPktEntry.data,"silencepkt:%lld",tmp_duration);
                                tmpPktEntry.dts = pkt->pts + i * pkt->duration;
                                tmpPktEntry.stream_index = pkt->stream_index;
                                ret = packet_queue_put(&audioQ,&tmpPktEntry);
                                GSLOGD("add extra1 silencepkt dts:%lld duration:%lld ret:%d sys:%lld",tmpPktEntry.dts,tmp_duration,ret,systemTime());
                            }
                            tmp_duration = (pkt->dts - pkt->pts) % pkt->duration;
                            tmp_duration = av_rescale_q(tmp_duration, stm->time_base, AV_TIME_BASE_Q);
                            if(tmp_duration > 0) {
                                av_new_packet(&tmpPktEntry,30);
                                sprintf((char*)tmpPktEntry.data,"silencepkt:%lld",tmp_duration);
                                tmpPktEntry.dts = pkt->pts + silence_pkt_num * pkt->duration;
                                tmpPktEntry.stream_index = pkt->stream_index;
                                ret = packet_queue_put(&audioQ,&tmpPktEntry);
                                GSLOGD("add extra2 silencepkt dts:%lld duration:%lld ret:%d sys:%lld",tmpPktEntry.dts,tmp_duration,ret,systemTime());
                            }
                        }
                        ret = packet_queue_put(&audioQ, pkt);
                        ZXLOGD("gsQIn index:%d dts:%lld pts:%lld ret:%d",
                        pkt->stream_index,pkt->dts,pkt->pts,ret);
                    }
                }
                else if (stm->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
                        mHasVideo &&
                        !mVideoEos &&
                        pkt->stream_index == ffmpeg->getVideoStreamIndex())
                {
                    ZJFLOGD("will packet_queue_put for video");
                    ret = packet_queue_put(&videoQ, pkt);
                    ZXLOGD("gsQIn index:%d dts:%lld pts:%lld ret:%d",
                     pkt->stream_index,pkt->dts,pkt->pts,ret);
                }
                else {
                    //Mutex::Autolock autolock(ffmpeg->mutex());
                    ffmpeg->freePacket(pkt);
                }

            }
            if (ret < 0)               // end of file.
            {
                mIsEof = true;
                ZJFLOGD("av_read_packet eof packet.");
                //bExit = true;
                //ZXLOGD("ffmpeg->readPacket() at end file.");
                //usleep(10);
                if (mHasAudio && !mAudioEos) {
                    ret = packet_queue_put(&audioQ, NULL);
                }
                if (mHasVideo && !mVideoEos) {
                    ret = packet_queue_put(&videoQ, NULL);
                }
            }
        }
        }
        }
        ZJFLOGD("at end of while");
    }

    ZXLOGD("run_process exit.");
    return NULL;
}
void* FFmpegSafeReader::run(void *args)
{
    FFmpegSafeReader* r = (FFmpegSafeReader*) args;
    return r->run_process();
}
void FFmpegSafeReader::packet_queue_init(PacketQueue* q)
{
    memset(q, 0, sizeof(PacketQueue));
    //q->mutex = SDL_CreateMutex();
    //q->cond = SDL_CreateCond();
    q->mutex = new Mutex();
    q->cond  = new Condition();
    //packet_queue_put(q, &flush_pkt);
}

void FFmpegSafeReader::packet_queue_flush(PacketQueue* q)
{
    AVPacketList *pkt, *pkt1;

    //SDL_LockMutex(q->mutex);
    AutoMutex _am(*q->mutex);

    for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1)
    {
        pkt1 = pkt->next;
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->bIsEos = false;
    //SDL_UnlockMutex(q->mutex);
}

void FFmpegSafeReader::packet_queue_end(PacketQueue *q)
{
    ZXLOGD("packet_queue_end().");
    packet_queue_flush(q);
    //SDL_DestroyMutex(q->mutex);
    //SDL_DestroyCond(q->cond);
    delete q->mutex;
    q->mutex = NULL;
    delete q->cond;
    q->cond = NULL;
}

int FFmpegSafeReader::packet_queue_put(PacketQueue *q, AVPacket* pkt)
{
    ZJFLOGD("function in.");
    AVPacketList *pkt1;

    /* duplicate the packet */
    if (pkt!=&flush_pkt && (NULL != pkt && av_dup_packet(pkt) < 0))
        return RET_ERR;

    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    memset(pkt1, 0, sizeof(AVPacketList));
    if (!pkt1)
        return RET_ERR;
    if (NULL == pkt) {
        ZJFLOGD("set bIsEof flag");
        pkt1->bIsEof = true;
    } else {
        pkt1->pkt = *pkt;
    }
    pkt1->next = NULL;


    //SDL_LockMutex(q->mutex);
    AutoMutex _am(*q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    if (NULL == pkt) {
        q->size += sizeof(*pkt1);
    } else {
        q->size += pkt1->pkt.size + sizeof(*pkt1);
    }
    /* XXX: should duplicate packet data in DV case */
    //SDL_CondSignal(q->cond);
    q->cond->signal();
    //SDL_UnlockMutex(q->mutex);
    ZJFLOGD("function out.");
    return RET_OK;
}

void FFmpegSafeReader::packet_queue_abort(PacketQueue* q)
{
    //SDL_LockMutex(q->mutex);
    AutoMutex _am(*q->mutex);
    q->abort_request = 1;
    //SDL_CondSignal(q->cond);
    //SDL_UnlockMutex(q->mutex);
    q->cond->signal();
}


// obtain's packet must release it.
int  FFmpegSafeReader::packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
    ZJFLOGD("function in.");
    AVPacketList *pkt1;
    int ret = RET_ERR;

    //SDL_LockMutex(q->mutex);
    AutoMutex _am(*q->mutex);

    for(;;)
    {
        if (q->abort_request)
        {
            ret = RET_ERR;
            ZXLOGD("packet_queue_get(),   abort_request.");
            ZJFLOGD("will return -1");
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1)
        {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            if (pkt1->bIsEof) {
                q->size -= sizeof(*pkt1);
                ZJFLOGD("will set bExit to exit read thread");
                q->bIsEos = true;
                ZJFLOGD("will return -1");
                ret = RET_ERR;
            } else {
                q->size -= pkt1->pkt.size + sizeof(*pkt1);
                *pkt = pkt1->pkt;
                ret = RET_GET_ONE;
            }
            av_free(pkt1);
            ZXLOGD("packet_queue_get(),  read packet ok.");
            break;
        }
        else if (!block)
        {
            ret = RET_EMPTY;
            //q->n_backets = 0;
            ZXLOGD("packet_queue_get(),  packet queue empty.");
            break;
        }
        else
        {
            //SDL_CondWait(q->cond, q->mutex);
            q->cond->wait(*q->mutex);
            if (bExit)
            {
                ZJFLOGD("will return -1");
                ret = RET_ERR;
                break;
            }
        }
    }
    //SDL_UnlockMutex(q->mutex);
    ZJFLOGD("function out.");
    return ret;
}

int FFmpegSafeReader::packet_queue_element_count(PacketQueue* q)
{
    //AutoMutex _am(*q->mutex);
    return q->nb_packets;
}
void FFmpegSafeReader::setStreamType(int streamType) {
    ZJFLOGD("function in.");
    ZJFLOGD("streamType is %d", streamType);
    if (streamType == 0) {
        mHasAudio = true;
    } else if(streamType == 1) {
        mHasVideo = true;
    }
    ZJFLOGD("function out.");
}
void FFmpegSafeReader::start() {
    ZJFLOGD("function in.");
    AutoMutex _am(*mutexStart);
    if (mStart) {
        return;
    }
    mStart = true;
    condStart->signal();
    ZJFLOGD("function out");
}
void FFmpegSafeReader::readSignal() {
    ZJFLOGD("function in.");
    AutoMutex _am(*mutexRead);
    condRead->signal();
    ZJFLOGD("function out.");
}
/*********************************/
#ifdef DEBUG_MUTEX
inline Mutex::Mutex()
{
    pthread_mutex_init(&mMutex, NULL);
}
inline Mutex::Mutex(const char* name)
{
    pthread_mutex_init(&mMutex, NULL);
}
inline Mutex::Mutex(int type, const char* name)
{
    if (type == SHARED)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    else
    {
        pthread_mutex_init(&mMutex, NULL);
    }
}
inline Mutex::~Mutex()
{
    pthread_mutex_destroy(&mMutex);
}
inline status_t Mutex::lock()
{
    return -pthread_mutex_lock(&mMutex);
}
inline void Mutex::unlock()
{
    pthread_mutex_unlock(&mMutex);
}
inline status_t Mutex::tryLock()
{
    return -pthread_mutex_trylock(&mMutex);
}
/*********************************/
inline Condition::Condition()
{
    pthread_cond_init(&mCond, NULL);
}
inline Condition::Condition(int type)
{
    if (type == SHARED)
    {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&mCond, &attr);
        pthread_condattr_destroy(&attr);
    }
    else
    {
        pthread_cond_init(&mCond, NULL);
    }
}
inline Condition::~Condition()
{
    pthread_cond_destroy(&mCond);
}
inline status_t Condition::wait(Mutex& mutex)
{
    return -pthread_cond_wait(&mCond, &mutex.mMutex);
}
inline void Condition::signal()
{
    pthread_cond_signal(&mCond);
}
inline void Condition::broadcast()
{
    pthread_cond_broadcast(&mCond);
}
#endif
/*********************************/

}
