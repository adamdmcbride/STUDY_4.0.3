#define LOG_TAG "LVCMD"
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/LVParams.h>

namespace android {
class CPU {
public:
    CPU();
public:
    int Get(int n);
    int Set(int governor);
public:
    void getService();
    sp<IBinder> binder;
};


CPU::CPU()
{
    getService();
}

void CPU::getService(){
    sp<IServiceManager> sm = defaultServiceManager();
    binder = sm->getService(String16("Livall.Server"));
    LOGE("CPU::getAddService %p/n",sm.get());
    if (binder == 0) {
        LOGW("AddService not published, waiting...");
        return;
    }
}

int CPU::Get(int n){
    int i;
    Parcel data, reply;
    CLVParamParse param;
    SLVCmdCPU *pCpu = (SLVCmdCPU*)param.GetParams();
    pCpu->mId = SLVCmdCPU::CMDID_GETGOVERNOR;
    pCpu->mCpus = 2;
    param.To(data);
    binder->transact(LVCMDID_CPU, data, &reply);
    CLVParamParse param2;
    param2.From(reply);
    LOGI("magic=%x\n", param2.GetMagic());
    pCpu = (SLVCmdCPU*)param2.GetParams();
    for(i=0; i<(int)pCpu->mCpus; i++) {
        LOGI("[%d] Governor=%d\n", i, pCpu->u.mGovernor[i]);
    }    
    return pCpu->u.mGovernor[0];
}

int CPU::Set(int governor)
{
    int i;
    Parcel data, reply;
    CLVParamParse param;
    SLVCmdCPU *pCpu = (SLVCmdCPU*)param.GetParams();
    pCpu->mId = SLVCmdCPU::CMDID_SETGOVERNOR;
    pCpu->mCpus = 2;
    pCpu->u.mGovernor[0] = governor;
    pCpu->u.mGovernor[1] = governor;
    param.To(data);
    i = binder->transact(LVCMDID_CPU, data, &reply);
    LOGI("set governor(%d) result=%d", governor, i);
    
    return 0;
}

}; //namespace

int main(int argc, char *argv[])
{
    android::CPU add;
    int governor = add.Get(0);
    governor += 1; 
    if(governor >= android::SLVCmdCPU::GOVERNOR_Max) governor = 0;
    add.Set(governor);

    return 0;
}
