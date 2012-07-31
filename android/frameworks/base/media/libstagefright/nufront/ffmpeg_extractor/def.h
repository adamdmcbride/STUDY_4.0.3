#ifndef __DEF_H__
#define __DEF_H__

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>
#include <regex.h>

#define ZX_LOG_ON
#ifdef ZX_LOG_ON
#   include <utils/Log.h>
#   define logd(format, args...)  LOGD(format, ##args)
#else
#   define logd(format, ...)
#endif




/*******************************************/
//read packet consume.
#define logd_consume(format, args...) LOGD(format , ##args)

//packet info, pts, dts.
#define logd_pkt(format, args...) LOGD(format, ##args)

//print info.
#define logd(format, args...)    LOGD(format, ##args)

//source info
#define logd_file(fomat, args...) LOGD(format, ##args)

//ffmpegSafeReader  ref count;
#define logd_ref(format, args...) LOGD(format, ##args)
/*******************************************/


typedef long long int int64;

const int64 seek_min = (-0x7fffffffffffffffLL - 1);
const int64 seek_max = (0x7fffffffffffffffLL);


class TimeUS {
    private:
        struct timeval tm;

    public:
        static int64 calcInterval(const TimeUS& tc);

        TimeUS();
        int64 timeUs() const;
        int64 refreshUs();
};


// capture video stream packet and save into log file.
class MediaStreamCapture {
public:
    void saveBuffer(char* buff, const int buff_len);
    void updateCaptureCount(const int nCount);
    const int captureCount() { return nCount; }
    const int capturedNum()  { return nCapIndex;}

public:
    MediaStreamCapture(int nCap, const char* path) ;
    ~MediaStreamCapture();

private:
    int       nCount;            //sum of capture packet;
    int       nCapIndex;         //
    FILE      *fp;               //save capture packet to handle of file .
    const char* const path;
};


namespace FFMPEG {
class Regex {
public:
    Regex(const char* p, int match_count = 1, bool show_error = false);
    ~Regex();
    bool exec(const char* str);
    regmatch_t* getMatchs();
private:
    void printError(const char* fmt, ...);
private:
    regex_t* reg;
    bool     showError;
    int      matchCount;
    regmatch_t*  matchs;
};
}

#endif
