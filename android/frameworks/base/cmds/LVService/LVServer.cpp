#define LOG_TAG "LVCMD"
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include "LVServer.h"
#include "global.h"
namespace android{

int GetCpuOnlines(SLVCmdCPU *pCpu);
int GetCpuGovernor(SLVCmdCPU *pCpu);
int SetCpuGovernor(SLVCmdCPU *pCpu, int timeOut);

    
static struct sigaction oldact;
static pthread_key_t sigbuskey;

LVServer::LVServer() : mLockCpu(PTHREAD_MUTEX_INITIALIZER)
{
    pthread_key_create(&sigbuskey, NULL);
}

LVServer::~LVServer()
{
    pthread_mutex_destroy(&mLockCpu);
    pthread_key_delete(sigbuskey);
}

int LVServer::instantiate()
{
    int r = defaultServiceManager()->addService(
                  String16("Livall.Server"), new LVServer());
    LOGI("LVServer instantiate return=%d/n", r);
    return r;
}

status_t LVServer::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    status_t status = NO_ERROR;
    CLVParamParse param;
    param.From(data);
    if(param.GetMagic() != MAGICID) return BAD_VALUE;
    switch(code) {
    case LVCMDID_CPU:
        pthread_mutex_lock(&mLockCpu);
        status = onTransactCpu((SLVCmdCPU*)param.GetParams());
        pthread_mutex_unlock(&mLockCpu);
        if(reply) param.To(*reply);
        return status;
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

status_t LVServer::onTransactCpu(SLVCmdCPU *pCpu)
{
    status_t status = NO_ERROR;
    //LOGI("onTransactCpu, id=%x\n", pCpu->mId);
    switch(pCpu->mId) {
    case SLVCmdCPU::CMDID_ONLINES:
        status = 0;
        break;
    case SLVCmdCPU::CMDID_GETGOVERNOR:
        status = GetCpuGovernor(pCpu);
        break;
    case SLVCmdCPU::CMDID_SETGOVERNOR:
        status = SetCpuGovernor(pCpu, 2*1000);
        break;
    }
    return status;
}

};
