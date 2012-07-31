#ifndef __FFMPEG_SAFE_READER_H__
#define __FFMPEG_SAFE_READER_H__

#include <stdio.h>
#include <pthread.h>

#define UINT64_C(C)  C##LL
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

#include "FFmpeg.h"
#include <utils/threads.h>

//#define DEFINE_MUTEX
#ifndef DEFINE_MUTEX
    using namespace android;
#endif

#define SYNC volatile
namespace FFMPEG {

#ifdef DEFINE_MUTEX
typedef int status_t;
class Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    Mutex();
    Mutex(const char* name);
    Mutex(int type, const char* name = NULL);
    ~Mutex();

    // lock or unlock the mutex
    status_t    lock();
    void        unlock();

    // lock if possible; returns 0 on success, error otherwise
    status_t    tryLock();

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class Autolock {
    public:
        inline Autolock(Mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };

private:
    friend class Condition;

    // A mutex cannot be copied
                Mutex(const Mutex&);
    Mutex&      operator = (const Mutex&);

    pthread_mutex_t mMutex;
};


typedef Mutex::Autolock AutoMutex;


class Condition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    Condition();
    Condition(int type);
    ~Condition();
    // Wait on the condition variable.  Lock the mutex before calling.
    status_t wait(Mutex& mutex);
    // Signal the condition variable, allowing one thread to continue.
    void signal();
    // Signal the condition variable, allowing all threads to continue.
    void broadcast();

private:
    pthread_cond_t mCond;
};
#endif

struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    SYNC int nb_packets;
    int size;
    int abort_request;
    Mutex *mutex;
    Condition *cond;
    bool bIsEos;
} ;


class Waiter {                //代替sleep;
private:
    Condition mCond;
    //Mutex     mutex;
public:
    void wait(int us) {
        Mutex m;
        m.lock();
        mCond.waitRelative(m, us * 1000);
        m.unlock();
    }
};

// mutilthread is safely;
class FFmpegSafeReader {
protected:
    FFmpegSafeReader(const char* path);
    ~FFmpegSafeReader();
public:
    int readVideoPacket(AVPacket* pkt);
    int readAudioPacket(AVPacket* pkt);
    int seek(int64_t tm, const int streamType);
    void exit();
    void startThread();
    FFmpeg* getffmpeg() const;
    const bool isEnd();
    void setStreamType(int streamType);
    void start();
    void readSignal();
    bool IsOpenFailed() { return mOpenFailed; }
public:
    void add_ref();
    void release_ref();
public:
    static FFmpegSafeReader* create(const char* path);
private:
    Mutex mutexRef;
    int mRef;


protected:
    void packet_queue_init(PacketQueue *q);
    void packet_queue_flush(PacketQueue *q);
    void packet_queue_end(PacketQueue *q);
    int  packet_queue_put(PacketQueue *q, AVPacket* pkt);
    void packet_queue_abort(PacketQueue *q);
    int  packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);
    int  packet_queue_element_count(PacketQueue* q);

protected:
    static void* run(void*);             // main process thread.
    void* run_process();
    Mutex  mutexProcess;
    Condition condProcess;
    SYNC int run_process_count;

private:
    Waiter          mWaiter;

private:
    PacketQueue     videoQ;
    PacketQueue     audioQ;
    static AVPacket flush_pkt;
    FFmpeg*         ffmpeg;
    char*           path;
    SYNC bool       mSeeking;                   // query  seeking command;
    SYNC bool       mSeekingProcess;           // in seeking.
    Mutex           *mutexSeek;
    Condition       *condSeek;
    SYNC bool       bExit;                         // can read packet;
    SYNC bool       bStart;
    SYNC int64_t    mLastSeekTime;
    pthread_t       mThread;
    AVPacket        mPkt;
    bool            mHasAudio;
    bool            mHasVideo;
    bool            mStart;
    bool            mIsEof;
    Mutex           *mutexStart;
    Condition       *condStart;
    Mutex           *mutexRead;
    Condition       *condRead;
    int             mStreamSeekFlag;
    bool            mAudioEos;
    bool            mVideoEos;
//add by gs
    int64_t         curVideoDTS;
    SYNC bool       mStopReadOutFlag;
    bool            mOpenFailed;
};

}

#endif
