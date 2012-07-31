#include "def.h"
#include <stdarg.h>

/*******************************************************/
/** TimeUS ****/
int64 TimeUS::calcInterval(const TimeUS& tc) {
    TimeUS t;   
    return t.timeUs() - tc.timeUs();    
}

TimeUS::TimeUS() {
    gettimeofday(&this->tm, NULL);
}

int64 TimeUS::timeUs() const {
    return tm.tv_sec * 1E6 + tm.tv_usec;
}

int64 TimeUS::refreshUs() {
    gettimeofday(&this->tm, NULL);
    return timeUs();
}
/****************************************************/
/** MediaStreamCapture ******************************/
//capture video stream packet and save into log file.
MediaStreamCapture::MediaStreamCapture(int ncap, const char* const p) :
    nCount(ncap),
    nCapIndex(0),
    path(p),
    fp(NULL){
    fp = fopen((char*)path, "wb");
    assert(fp);
}

MediaStreamCapture::~MediaStreamCapture() {
    if (fp) {
        fclose(fp);    
        fp = NULL;
    } 
}

void MediaStreamCapture::saveBuffer(char* buff, const int buff_len) {
    assert(buff);
    assert(buff_len > 0);
    assert(fp);

    if (nCapIndex++ >= captureCount()) {
        fwrite(buff, buff_len, 1, fp);
    }
} 

void MediaStreamCapture::updateCaptureCount(const int n) {
    this->nCount = n;
    this->nCapIndex = 0;
} 
/*************************************************/
// regex lib

namespace FFMPEG {

Regex::Regex(const char* p, int match_count, bool show_error)
    : reg(NULL),
      matchCount( match_count < 1 ? 1 : match_count), 
      matchs(NULL), 
      showError(show_error)
     {
        reg = new regex_t;
        int err;
        if(err = regcomp(reg, p, 0)) {
            char buff[512] = "";
            buff[regerror(err, reg, buff, sizeof(buff))] = 0;
            printError("regcomp('%s') error, %s\n", buff);
            delete reg;
            reg = NULL;
        }
}

Regex::~Regex() {
        if (matchs) {
            delete[] matchs;
        }
        if (reg) {
            regfree(reg);
            delete reg;
        }
}

bool Regex::exec(const char* str) {
    if (!reg || matchCount < 1) return false;

    int err;
    if (!matchs) {
        matchs = new regmatch_t[matchCount];
    }
    if (err = regexec(reg, str, matchCount, matchs, 0)) {
        char buff[512] = "";
        buff[regerror(err, reg, buff, sizeof(buff))] = 0;
        printError("regexec('%s') error. %s", str, buff);
        return false;
    }
    return true;
}

void Regex::printError(const char* fmt, ...) {
    if (showError) {
        char buff[1024] = "" ;
        va_list args;
        va_start(args, fmt);
        vsnprintf(buff, sizeof(buff), fmt, args);
        printf("%s\n", buff);
        va_end(args);
    } 
}
}
