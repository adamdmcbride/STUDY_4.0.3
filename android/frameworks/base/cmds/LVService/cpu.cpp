#define LOG_TAG "CPU"
#include "global.h"
#include <utils/LVParams.h>
namespace android{

#define MAXCPUS     2
#define CPUMASK     3

static const char *CPUGOVERNOR[] = {
    "performance",
    "conservative",
    "ondemand",
    "powersave"
};
#define CPUGOVERNOR_PERFORMANCE SLVCmdCPU::GOVERNOR_PERFORMANCE
#define CPUGOVERNOR_CONSERVATIVE    SLVCmdCPU::GOVERNOR_CONSERVATIVE
#define CPUGOVERNOR_ONDEMAND        SLVCmdCPU::GOVERNOR_ONDEMAND
#define CPUGOVERNOR_POWERSAVE       SLVCmdCPU::GOVERNOR_POWERSAVE
#define SIZE_CPUGOVERNOR    sizeof(CPUGOVERNOR)/sizeof(CPUGOVERNOR[0])

//CPU settings
static inline void getCpuGovernorFile(int cpu, char *buf)
{
    sprintf(buf, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", cpu);
}

/*
reutrn: bit for cpu.
    bit 0: cpu0
    bit 1: cpu1
*/
static unsigned int getCpuOnline(unsigned int *cpus)
{
    unsigned int online = 0;
    char buf[32];
    unsigned int cpu_count = 0;
    read_content("/sys/devices/system/cpu/online", buf, sizeof(buf));
    char *p = strtok(buf, "-");
    while(p != NULL) {
        int d = atoi(p);
        online |= (1<<d);
        cpu_count ++;
        p = strtok(NULL, "-");
    }
    if(cpus) *cpus = cpu_count;
    return online;
}

static int waitCpuOnline(unsigned int cpuon, int timeOut, int *usedTime)
{
    if(usedTime) *usedTime = 0;
    do{
        unsigned int online = getCpuOnline(NULL);
        if(cpuon==(online&cpuon)) return 0;
        msleep(10);
        if(usedTime) *usedTime += 10;
    }while(timeOut > 0);
    if(usedTime) *usedTime = timeOut;
    return -ETIME;
}

static int getCpuGovernor(int cpu)
{
    int i;
    char filename[128];
    char buf[64];
    if(cpu<0 || cpu>1) cpu = 0;
    getCpuGovernorFile(cpu, filename);
    read_content(filename, buf, sizeof(buf));
    for(i=0; i<SIZE_CPUGOVERNOR; i++) {
        if(!strncmp(CPUGOVERNOR[i], buf, strlen(CPUGOVERNOR[i])))
            return i;
    }
    return -EIO;
}

static int writeCpuGovernor(int cpu, int governor, int check)
{
    int ret;
    char filename[128];
    char buf[64];
    getCpuGovernorFile(cpu, filename);
    ret = write_content(filename, CPUGOVERNOR[governor], strlen(CPUGOVERNOR[governor]));
    if(ret != 0) return ret;
    if(check) {
        read_content(filename, buf, sizeof(buf));
        if(strncmp(CPUGOVERNOR[governor], buf, strlen(CPUGOVERNOR[governor]))) {
            return -EIO;
        }
    }
    return 0;
}

static int setCpuGovernor(int governor, int timeOut)
{
    int ret;
    char filename[128];
    char buf[64];
    if(governor<0 || governor>=SIZE_CPUGOVERNOR) {
        return -EINVAL;
    }
    if(getCpuGovernor(0) == governor) {
        return 0;
    }
    // Must set performance first
    getCpuGovernorFile(0, filename);
    write_content(filename, CPUGOVERNOR[CPUGOVERNOR_PERFORMANCE], strlen(CPUGOVERNOR[CPUGOVERNOR_PERFORMANCE]));
    getCpuGovernorFile(1, filename);
    ret = waitForFile(filename, R_OK|W_OK, timeOut);
    if(ret != 0) {
        return ret;
    }
    ret = writeCpuGovernor(1, governor, 1);
    if(ret != 0) return ret;
    ret = writeCpuGovernor(0, governor, 1);
    if(ret != 0) return ret;
    return 0;
}

int GetCpuOnlines(SLVCmdCPU *pCpu)
{
    unsigned int cpus;
    pCpu->u.mOnlines = getCpuOnline(&cpus);
    pCpu->mCpus = cpus;
    return 0;
}


int GetCpuGovernor(SLVCmdCPU *pCpu)
{
    int i;
    for(i=0; i<MAXCPUS; i++) {
        pCpu->u.mGovernor[i] = getCpuGovernor(i);
    }
    pCpu->mCpus = MAXCPUS;
    return 0;
}

int SetCpuGovernor(SLVCmdCPU *pCpu, int timeOut)
{
    int i;
    int ret;
    int cpus;
    int usedTime=0;
    char filename[128];
    ret = writeCpuGovernor(0, CPUGOVERNOR_PERFORMANCE, 1);
    if(ret) return ret;
    getCpuGovernorFile(1, filename);
    ret = waitForFile(filename, R_OK|W_OK, timeOut);
    if(ret) return ret;
    cpus = (int)pCpu->mCpus;
    cpus = min(cpus, MAXCPUS);
    for(i=cpus-1; i>=0; i--) {
        if(pCpu->u.mGovernor[i]>=0 && pCpu->u.mGovernor[i]<SIZE_CPUGOVERNOR) {
            ret = writeCpuGovernor(i, pCpu->u.mGovernor[i], 1);
            //LOGI("SetCpuGovernor[%d]=%d, ret=%d", i, pCpu->u.mGovernor[i], ret);
        }
    }
    return 0;
}

};
