#include <sys/types.h>
static inline int32_t min(int32_t a, int32_t b) {
    return (a<b) ? a : b;
}

static inline int32_t max(int32_t a, int32_t b) {
    return (a>b) ? a : b;
}

int msleep(int ms);
void read_content(const char *filename, char *buf, size_t bufsize);
int write_content(const char *filename, const char *buf, size_t bufsize);
int waitForFile(const char *filename, int mode, int timeOut);
