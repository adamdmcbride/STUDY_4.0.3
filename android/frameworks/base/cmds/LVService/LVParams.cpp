#include <utils/LVParams.h>
namespace android {

CLVParamParse::CLVParamParse()
{
    magic = MAGICID;
    mPId = getpid();
}

CLVParamParse::~CLVParamParse()
{
}


int CLVParamParse::From(const Parcel& data)
{
    SParam *p = getData();
    if(data.dataSize() != sizeof(SParam)) {
        //LOGE()
        return -1;
    }
    memcpy(p, data.data(), sizeof(SParam));
    return 0;
}

int CLVParamParse::To(Parcel& data)
{
    data.setDataPosition(0);
    data.setData((const uint8_t* )getData(), sizeof(SParam));
    return 0;
}

};

