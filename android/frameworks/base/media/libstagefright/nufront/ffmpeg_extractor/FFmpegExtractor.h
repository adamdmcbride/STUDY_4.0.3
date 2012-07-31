#ifndef  __FFMPEG_EXTRACTOR_H__
#define  __FFMPEG_EXTRACTOR_H__

#include "def.h"
#include <utils/Log.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include "include/ESDS.h"
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <utils/String8.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}
#include "FFmpeg.h"
#include "FFmpegSafeReader.h"

namespace FFMPEG {
using namespace android;


/*******************************************/
// hp added.
struct H264BSFContext {
    uint8_t  length_size;
    uint8_t  first_idr;
    uint8_t *sps_pps_data;
    uint32_t size;
} ;
/*******************************************/

struct Track {
    Track           *next;
    sp<MetaData>    meta;
    unsigned int    streamId;
    bool            bUseDts;
};

class FFmpegExtractor :
    public MediaExtractor{
/*  constructor area; */
public:
    FFmpegExtractor(const sp<DataSource> &source);
protected:
    virtual ~FFmpegExtractor();

/* function'ss area; */
public:
    virtual size_t          countTracks();
    virtual sp<MediaSource> getTrack(size_t index);
    virtual sp<MetaData>    getTrackMetaData(size_t index, uint32_t flags);
    virtual sp<MetaData>    getMetaData();
    bool                    IsOpenFailed() { return mOpenFailed; }
private:
    status_t                readMetaData();
    status_t                parseChunk();
    const char*             findMimeFromMap(int codec);

/* member variable's area; */
private:
    sp<DataSource>  mDataSource;
    Track           *mFirstTrack;
    Track           *mLastTrack;
    sp<MetaData>    mFileMetaData;
    bool            mHaveMetadata;
    H264BSFContext  mH264BSFContext;
    FFmpeg          *ffmpeg;
    FFmpegSafeReader *ffmpegReader;
    bool             mHasVideo;
    bool             mOpenFailed;
private:
};




class FFmpegStreamSource :
    public MediaSource {
public:
    FFmpegStreamSource(const sp<MetaData> &format,
                       const sp<DataSource> &dataSource,
                       int sourceStream,
                       bool bIsDts,
                       FFmpegSafeReader* reader);
private:
    FFmpegStreamSource(const FFmpegStreamSource &);
    FFmpegStreamSource& operator= (const FFmpegStreamSource &);

protected:
    ~FFmpegStreamSource();

protected:
    int                     readPacket(AVPacket* pkt, const int sourceType);
public:
    virtual status_t        start(MetaData *params = NULL);
    virtual status_t        stop();
    virtual sp<MetaData>    getFormat();
    virtual status_t        read(MediaBuffer **buffer,
                                 const ReadOptions *options = NULL);
    virtual status_t        set_seek_exit_flag(SeekExitFlags flag);

private:
    Mutex               mLock;
    sp<MetaData>        mFormat;
    sp<DataSource>      mDataSource;
    bool                mStarted;
    MediaBufferGroup    *mGroup;
    MediaBuffer         *mBuffer;
    uint8_t             *mSrcBuffer;
    FFmpeg              *ffmpeg;
    const int           mSourceStream;
    AVPacket            *packet;                /* data packet that read from ffmpeg; */
    AVPacket            mPkt;
    bool                mbIsDts;
    bool                isFirstPacket;
    float               mTimescale;
    H264BSFContext      mH264BSFContext;
    FFmpegSafeReader    *ffmpegReader;
    FFmpegStreamInfo    *streamInfo;
    int64_t             mLastDts;
    size_t              mMaxInputSize;
};
bool SniffFFmpeg(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *meta);
}


#ifndef AV_RL32
#   define AV_RL32(x)                           \
((((const uint8_t*)(x))[3] << 24) |         \
 (((const uint8_t*)(x))[2] << 16) |         \
 (((const uint8_t*)(x))[1] <<  8) |         \
  ((const uint8_t*)(x))[0])
#endif
#ifndef AV_RB32
#   define AV_RB32(x)                           \
    ((((const uint8_t*)(x))[0] << 24) |         \
     (((const uint8_t*)(x))[1] << 16) |         \
     (((const uint8_t*)(x))[2] <<  8) |         \
      ((const uint8_t*)(x))[3])
#endif

#ifndef AV_RB16
#   define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])
#endif

#ifndef AV_WB32
#   define AV_WB32(p, d) do {                   \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif


#endif
