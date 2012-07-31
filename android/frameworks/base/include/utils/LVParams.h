/*
mParams 参数以\0分隔每一个字符串参数。最后一个为\0\0
*/

#if !defined(__HENGAI_CLASS_LVPARAMS_H__)
#define __HENGAI_CLASS_LVPARAMS_H__
#include <sys/types.h>
#include <binder/Parcel.h>

namespace android {
#define LVCMDID_CPU         1

struct SLVCmdCPU{
    enum{
        GOVERNOR_PERFORMANCE=0,
        GOVERNOR_CONSERVATIVE,
        GOVERNOR_ONDEMAND,
        GOVERNOR_POWERSAVE,
        GOVERNOR_Max,
    };
    enum{
    	CMDID_ONLINES = 0,
    	CMDID_GETGOVERNOR,
	CMDID_SETGOVERNOR,
    };
    uint32_t mId;
    uint32_t mCpus;
    union{
        uint32_t mOnlines;	//bit
        uint32_t mGovernor[16];  //max support 16 cpus
    }u;
};

#define	MAGICID  0x091226AB
struct SParam {
    uint32_t magic;
    uint32_t mPId;      //process id
    union{
        SLVCmdCPU mCpu;
        char     mParams[1024];     //save all params
    }u;
};

class CLVParamParse : SParam{
public:
    CLVParamParse();
    ~CLVParamParse();
    int From(const Parcel& data);
    int To(Parcel& data);
public:
    uint32_t GetMagic() {return magic;}
    void *GetParams() {return getData()->u.mParams;}
private:
    SParam *getData() {return (SParam*)&magic;}
};

};
#endif  //__HENGAI_CLASS_LVPARAMS_H__
