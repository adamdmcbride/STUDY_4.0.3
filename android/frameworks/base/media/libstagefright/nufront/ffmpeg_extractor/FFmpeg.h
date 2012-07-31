#ifndef __FFmpeg_H__
#define __FFmpeg_H__

#include "def.h"

#undef  UINT64_C
#define UINT64_C(C)  C##LL
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#include <utils/threads.h>
#include "androidSource.h"

#ifndef SYNC
#define SYNC volatile
#endif

#ifndef RET_LOAD_FULL_INDEX
#define RET_LOAD_FULL_INDEX 0
#endif
#ifndef RET_OK
#define RET_OK 0
#endif
#ifndef RET_ERR
#define RET_ERR (-1)
#endif
#ifndef RET_GET_ONE
#define RET_GET_ONE 1
#endif
#ifndef RET_EMPTY
#define RET_EMPTY 0
#endif
#ifndef NOT_SAVE_INDEX
#define NOT_SAVE_INDEX (-1)
#endif

#ifndef AUDIO_BEFORE_VIDEO_AFTER_SEEK
#define AUDIO_BEFORE_VIDEO_AFTER_SEEK 0x1001
#endif

#ifndef AUDIO_BEHIND_VIDEO_AFTER_SEEK
#define AUDIO_BEHIND_VIDEO_AFTER_SEEK 0x1002
#endif

class FFmpeg;
//class android::Mutex;

class FFmpegStreamInfo {
public:
    FFmpegStreamInfo();
    ~FFmpegStreamInfo();

public:
    enum {
        StreamTypeNone = 0,
        StreamTypeVideo ,
        StreamTypeAudio ,
    };

public:
    friend class     FFmpeg;
    const int        getStreamId()      { return streamId; }
    const CodecID    getCodecId()       { return codec_id; }
    const int        getSliceCount()    { return slice_count; }
    const int64_t    getDuration()      { return duration; }
    const AVRational getTime_base()     { return time_base; }
    const char*      getStreamTypeStr() { return strStreamType; }
    const int        getStreamType()    { return streamType; }
    const char*      getExtradata()     { return extradata; }
    const int        getExtradataSize() { return extradata_size; }
    const int        getMaxSampleSize() { return max_sample_size; }
    float            getTimescale()     { return time_base.num / float(time_base.den) * 1E6; }
    const char*      getEsds()          { return esds; }
    int              getEsdsSize()      { return esds_size; }
private:
    int         streamId;
    CodecID     codec_id;
    int         slice_count;        // codec->slice_count;
    int64_t     duration;
    AVRational  time_base;
    const char* strStreamType;
    int         streamType;
    char*       extradata;
    int         extradata_size;
    int         max_sample_size;
//add by gs
    int         esds_size;
    char*       esds;
};

class FFmpeg {
    private:
        int                     streamVideoIndex;
        int                     streamAudioIndex;
        float                   scaleVideo;
        float                   scaleAudio;
        int64                   videoDuration;
        int64                   audioDuration;
        int64                   duration;
        char                    path[PATH_MAX];
        struct AVFormatContext* ctx;
        struct AVFormatContext* ctx_seek;
        android::Mutex          ctxMutex;
        bool                    bEOFError;
        int64_t                 first_video_dts;
        int64_t                 first_audio_dts;
        int64_t                 video_offset_time;
        int64_t                 max_video_dts;
        int64_t                 max_audio_dts;
        int                     m_AudioDuration;
        int                     m_VideoDuration;
        int                     seek_flag;
        int64_t                 after_seek_first_video;
        int                     tsFlag;
        int                     first_audio_flag;
        /*add to indicate open failure*/
        bool                    mOpenFailed;
    private:
        static int ref_count;
        static int add_ref();
        static int release_ref();

    public:
        ::android::Mutex&                  mutex() { return ctxMutex;}
        static void* run(void*);             // main process thread.
        void* run_process();
        pthread_t       mThread;
        void startThread();
        SYNC bool bExit;
    public:
        FFmpeg();
        FFmpeg(const char* path);
        ~FFmpeg();
        FFmpegStreamInfo*   getStreamInfo(FFmpegStreamInfo* info, const int streamId);
        void                open(const char* path);
        void                close();
        int                 seek(const int64 time, const int64_t curVideoDTS);
//        void                seek(const float t, int flag);
        float               getTimescale(const int streamId);
        const int           getVideoStreamIndex() const;
        const int           getAudioStreamIndex() const;
        const int64         getDuration() const;
        const int64         getAudioDuration() const;
        const int64         getVideoDuration() const;
        const float         getAudioScale() const;     //before use it please execute calcScale();
        const float         getVideoScale() const;     //before use it please execute calcScale();
        const char* const
                            getPath() const;
        const int           getStreamCount() const;
        const int           getStreamType(const int streamId)  const;
        const int           getCodecID(const int streamId) const;
        const int64_t       getStreamDuration(const int streamId) const;
        const int64         videoTimestamp(const int64 ts); //convert pts/dts to timestamp;
        const int64         audioTimestamp(const int64 ts);
        int                 readPacket(struct AVPacket* pkt);
        void                initPacket(struct AVPacket*);
        void                freePacket(struct AVPacket*);
        AVStream*           stream(int id);
        const bool          isFEof();
        const int           getBestStream(const enum AVMediaType type, int related_stream) const;
        AVDictionaryEntry* getDictionary(AVDictionaryEntry* tag);
        const char*        getName();
        uint32_t            getVideoRotate(const int streamId);
        AVDictionaryEntry*  getLocation(void);
        /*add to get mOpenFailed*/
        bool                IsOpenFailed() { return mOpenFailed; }

    public:
//add by gs
        char*                   esds;
        int                     esds_size;
        AVRational          getVideoTimeBase();
    protected:
        void                calcScale();
        void                findAVIndex();             // find Video Audio stream index;
        struct AVStream*    videoStream();
        struct AVStream*    audioStream();
        void                findDuration();
        int                 discard_unused_streams();
        int                 load_indexfile(char* mediafilepath, AVFormatContext* ctx);
        int                 save_indexfile(char* mediafilepath, uint8_t over);
        int                 check_indexfile(FILE* fp);
        int                 gen_md5(FILE* fp,char* md5);
        int                 mk_dir(char* path, bool isdir);
        int                 index_size;
        uint8_t             seek_over;
        char                indexfilepath[PATH_MAX];
        int                 bitset(int32_t* value, uint8_t* index, uint8_t* ptr, int32_t bitvalue, uint8_t bitsize);
        int                 seek_video_backward(int64_t seekdts, int64_t curdts, AVStream* st);
        int                 gen_index_tsps();
     public:
        int                 set_seek_exit_flag(int flag);
};

#endif
