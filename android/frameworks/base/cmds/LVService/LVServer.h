#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/LVParams.h>

namespace android{
class LVServer : public BBinder {
public:
    LVServer();
    ~LVServer();
    static int instantiate();
    virtual status_t onTransact(uint32_t, const Parcel&, Parcel*, uint32_t);
private:
    status_t onTransactCpu(SLVCmdCPU *pCpu);
private:
    pthread_mutex_t mLockCpu;
};

};

