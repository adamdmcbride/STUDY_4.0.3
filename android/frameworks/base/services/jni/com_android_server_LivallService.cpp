/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "LivallService"

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <utils/LVParams.h>

#include <utils/misc.h>
#include <utils/Log.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>

//////////////////////////////////////////////////////////////////////
namespace android
{

static struct {
    jclass      clazz;
}gLivallServiceClassInfo;

static inline int msleep(int ms)
{
    struct timespec  t;
    t.tv_sec  = 0;
    t.tv_nsec = ms*1000000;
    return nanosleep(&t, &t);
}

static void read_content(const char *filename, char *buf, size_t bufsize)
{
    int fd;
    memset(buf, 0, bufsize);
    fd = open(filename, O_RDONLY);
    if(fd >= 0) {
        read(fd, buf, bufsize);
        close(fd);
    }
}

static int write_content(const char *filename, const char *buf, size_t bufsize)
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
//////////////////////////////////////////////////////////////////////
sp<IBinder> getLVService()
{
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder>binder = sm->getService(String16("Livall.Server"));
    LOGE("CPU::getAddService %p/n",sm.get());
    if (binder == 0) {
        LOGW("AddService not published, waiting...");
        return NULL;
    }
    return binder;
}

//////////////////////////////////////////////////////////////////////
static jboolean com_android_server_LivallService_nativeInit(JNIEnv* env, jclass clazz)
{
    return true;
}

/* get iNand CID */
static jstring com_android_server_LivallService_nativeGetSerialNumber(JNIEnv *env, jobject clazz)
{
    char buf[64];
    read_content("/sys/devices/platform/ns115-sdmmc.0/mmc_host/mmc1/mmc1:0001/cid", buf, sizeof(buf));
    return env->NewStringUTF(buf);
}

static jint com_android_server_LivallService_getCpuGovernor(JNIEnv *env, jobject clazz, jint cpu)
{
    int result;
    Parcel data, reply;
    CLVParamParse param;
    SLVCmdCPU *pCpu = (SLVCmdCPU*)param.GetParams();
    sp<IBinder> binder = getLVService();
    if(binder == NULL) return -ESRCH;
    pCpu->mId = SLVCmdCPU::CMDID_GETGOVERNOR;
    pCpu->mCpus = 2;
    param.To(data);
    result = (int)binder->transact(LVCMDID_CPU, data, &reply);
    if(result < 0) return result;
    CLVParamParse param2;
    param2.From(reply);
    //LOGI("magic=%x\n", param2.GetMagic());
    pCpu = (SLVCmdCPU*)param2.GetParams();
#if 1
    for(int i=0; i<(int)pCpu->mCpus; i++) {
        LOGI("[%d] Governor=%d\n", i, pCpu->u.mGovernor[i]);
    }
#endif
    return pCpu->u.mGovernor[cpu];
}

static jint com_android_server_LivallService_setCpuGovernor(JNIEnv *env, jobject clazz, jint cpus, jint governor, jint timeOut)
{
    Parcel data, reply;
    CLVParamParse param;
    SLVCmdCPU *pCpu = (SLVCmdCPU*)param.GetParams();
    sp<IBinder> binder = getLVService();
    if(binder == NULL) return -ESRCH;
    pCpu->mId = SLVCmdCPU::CMDID_SETGOVERNOR;
    pCpu->mCpus = 0;
    for(int i=0; i<31; i++) {
        if(cpus & (1<<i)) {
            pCpu->mCpus = i+1;
            pCpu->u.mGovernor[i] = governor;
            //LOGI("[%d]mCpus=%d, governor=%d\n", i, pCpu->mCpus, governor);
        }
   }
    param.To(data);
    return binder->transact(LVCMDID_CPU, data, &reply);
}

static JNINativeMethod method_table[] = {
    { "nativeInit", "()Z", (void*) com_android_server_LivallService_nativeInit },
    { "nativeGetSerialNumber", "()Ljava/lang/String;", (void*)com_android_server_LivallService_nativeGetSerialNumber },
    //CPU
    { "nativeGetCpuGovernor", "(I)I", (void*)com_android_server_LivallService_getCpuGovernor },
    { "nativeSetCpuGovernor", "(III)I", (void*)com_android_server_LivallService_setCpuGovernor },
};


#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className); \
        var = jclass(env->NewGlobalRef(var));

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

#define GET_FIELD_ID(var, clazz, fieldName, fieldDescriptor) \
        var = env->GetFieldID(clazz, fieldName, fieldDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find field " fieldName);

int register_android_server_LivallService(JNIEnv *env)
{
    jclass clazz;
    clazz = env->FindClass("com/android/server/LivallService");
    if (clazz == NULL) {
        LOGE("Can't find com/android/server/LivallService");
        return -1;
    }

    return AndroidRuntime::registerNativeMethods(env,
                "com/android/server/LivallService", method_table, NELEM(method_table));
    LOG_FATAL_IF(res < 0, "Unable to register LivallService native methods.");

    FIND_CLASS(gLivallServiceClassInfo.clazz, "com/android/server/LivallService");

    return 0;
}

};
