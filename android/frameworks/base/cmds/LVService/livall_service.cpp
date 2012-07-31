
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

#include <private/android_filesystem_config.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <signal.h>
#include <stdio.h>
#include <unistd.h>


#include "LVServer.h"
using namespace android;

static void blockSignals()
{
    sigset_t mask;
    int cc;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGUSR1);
    cc = sigprocmask(SIG_BLOCK, &mask, NULL);
    assert(cc == 0);
}

int main(int argc, const char* const argv[])
{
    LOGI("System server is starting with pid=%d.\n", getpid());

    blockSignals();
    
    // You can trust me, honestly!
    LOGW("*** Current priority: %d\n", getpriority(PRIO_PROCESS, 0));
    setpriority(PRIO_PROCESS, 0, -1);

    sp<ProcessState> proc(ProcessState::self());

    sp<IServiceManager> sm = defaultServiceManager();
    LOGI("ServiceManager: %p", sm.get());
    LVServer::instantiate();
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}

