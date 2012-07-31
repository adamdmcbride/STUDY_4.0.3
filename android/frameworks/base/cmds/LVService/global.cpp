#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include "global.h"

int msleep(int ms)
{
    struct timespec  t;
    t.tv_sec  = 0;
    t.tv_nsec = ms*1000000;
    return nanosleep(&t, &t);
}

void read_content(const char *filename, char *buf, size_t bufsize)
{
    int fd;
    memset(buf, 0, bufsize);
    fd = open(filename, O_RDONLY);
    if(fd >= 0) {
        read(fd, buf, bufsize);
        close(fd);
    }
}

int write_content(const char *filename, const char *buf, size_t bufsize)
{
    int fd;
    fd = open(filename, O_WRONLY);
    if(fd >= 0) {
        size_t wr = write(fd, buf, bufsize);
        close(fd);
        return (wr==bufsize)?0:-errno;
    }
    return -errno;
}

int waitForFile(const char *filename, int mode, int timeOut)
{
    int ret = 0;
    do{
        ret = access(filename, mode);
        if(ret == 0) return 0;
        msleep(10);
        timeOut -= 10;
    }while(timeOut > 0);
    return ret;
}
