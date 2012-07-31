#ifndef __VPUDECINFO_H__
#define __VPUDECINFO_H__

#include "decapicommon.h"

typedef struct VPUDecBuild_
{
    u32         swBuild;  /* Software build ID */
    u32         hwBuild;  /* Hardware build ID */
    DecHwConfig hwConfig; /* hardware supported configuration */
} VPUDecBuild;

VPUDecBuild VPUDecGetBuild();

#endif /* __VPUDECINFO_H__ */
