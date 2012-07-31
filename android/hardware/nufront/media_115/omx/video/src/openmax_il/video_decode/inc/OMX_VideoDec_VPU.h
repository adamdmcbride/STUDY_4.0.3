#ifndef OMX_VIDDEC_VPU__H
#define OMX_VIDDEC_VPU__H

#include "h264decapi.h"
#include "mp4decapi.h"
#include "mpeg2decapi.h"
#include "rvdecapi.h"
#include "vc1decapi.h"
#include "vp6decapi.h"
#include "vp8decapi.h"
#include "jpegdecapi.h"
#include "OMX_Core.h"
#include "misc.h"
/*ldc@nufront*/
#ifdef CONFIG_VPU_PP
#include "ppapi.h"
#endif

typedef enum VPU_MIMETYPE_SUPPORT {
    VPU_MIMETYPE_H264 = 0,
    VPU_MIMETYPE_H263,
    VPU_MIMETYPE_MPEG4,
    VPU_MIMETYPE_MPEG2,
    VPU_MIMETYPE_MPEG1,
    VPU_MIMETYPE_DIVX,
    VPU_MIMETYPE_RV,
    VPU_MIMETYPE_SORENSON,
    VPU_MIMETYPE_VC1,
    VPU_MIMETYPE_VP6,
    VPU_MIMETYPE_VP8,
    VPU_MIMETYPE_JPEG
} VPU_MIMETYPE_SUPPORT;

typedef struct VPU_DEC_PARAM_ {
    OMX_U32 width;  /*Output width in pixels (multiple of 16). */
    OMX_U32 height; /*Output height in pixels (multiple of 16). */
    OMX_U32 displayWidth;  /*Original width of video element stream. */
    OMX_U32 displayHeight; /*Original height of video element stream. */
    OMX_U8* inStreamBuf;
#ifdef INPUT_VPUMEM
    OMX_U8* inStreamBufBus;
#endif
    OMX_U32 inLen;
    OMX_U32 picId;
    OMX_TICKS timestamp;
    OMX_U32 nVersion; // only be used by rv
    OMX_U32 nMaxFrameWidth; // only be used by rv & vc1
    OMX_U32 nMaxFrameHeight; // only be used by rv & vc1
    OMX_U32 nVC1DecStrmFmt; // only be used by vc1
    MP4DecStrmFmt strmFmt;
    VPU_MIMETYPE_SUPPORT mimeType;
    OMX_U32 seekFlag;  /*uesd by H.264*/
    OMX_U32 flip;  /*uesd by VP6*/
} VPU_DEC_PARAM;

typedef enum VC1DecStrmFmt_ {
    VC1DEC_VC1,
    VC1DEC_WMV3
} VC1DecStrmFmt;

/*ldc@nufront*/
#ifdef CONFIG_VPU_PP
typedef struct VPUPPOutBuffers_ {
    u32* poutBuf_va[PP_MAX_MULTIBUFFER-1];
    u32 bufSize[PP_MAX_MULTIBUFFER-1];
    PPOutputBuffers ppOutBufs;
    u32 standloneFlag; /*a@nufront: used by rv&vp6*/
} VPUPPOutBuffers;
#endif
typedef struct VPUDirectBufStru_ {
    u32 magic[2];
    u8 * pbuf;
    u32 bufSize;
} VPUDirectBufStru;

typedef struct VPU_GETPIC_PARAM_ {
    u32 width;
    u32 height;
    u8* outPicBuf;
    u32 outLen;
    u32 isIDR;
    u32 picId;
    u32 flip;  /*uesd by VP6*/
    u32 bufIdx; /*used by VP6&RV*/
    u32 gotISlice; /*once got I slice, it will be OMX_TRUE*/
} VPU_GETPIC_PARAM;

OMX_ERRORTYPE vpuInit(void *pComponentPrivate);
OMX_ERRORTYPE vpuDecode(void *pComponentPrivate);
OMX_ERRORTYPE vpuGetPic(void *pComponentPrivate);
OMX_ERRORTYPE vpuRelease(void *pComponentPrivate);

OMX_ERRORTYPE StartUseVPUMem(void ** pHandle);
OMX_ERRORTYPE StopUseVPUMem(void * handle);
OMX_ERRORTYPE MallocVPUMem(void* handle,unsigned int nSizeBytes,void* inStreamMem);
OMX_ERRORTYPE FreeVPUMem(void* handle,void* inStreamMem);

#endif
