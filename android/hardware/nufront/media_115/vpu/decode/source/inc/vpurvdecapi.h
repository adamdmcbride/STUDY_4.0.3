/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2007 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : API for the 8190 RV Decoder
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: on2rvdecapi.h,v $
--  $Date: 2010/12/13 13:04:01 $
--  $Revision: 1.5 $
--
------------------------------------------------------------------------------*/

#ifndef __VPURVDECAPI_H__
#define __VPURVDECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"

/* return values */
#define MAKE_RESULT(sev,fac,code) \
    (((u32)sev << 31) | ((u32)4 << 16) | ((fac<<6) | (code)))

#define VPURVDEC_OK                     MAKE_RESULT(0,0,0x0000)
#define VPURVDEC_OUTOFMEMORY            MAKE_RESULT(1,7,0x000e)
#define VPURVDEC_INVALID_PARAMETER      MAKE_RESULT(1,7,0x0057)
#define VPURVDEC_NOTIMPL                MAKE_RESULT(1,0,0x4001)
#define VPURVDEC_POINTER                MAKE_RESULT(1,0,0x4003)
#define VPURVDEC_FAIL                   MAKE_RESULT(1,0,0x4005)

typedef u32 VpuRvDecRet;

/* custom message handling */
#define VPURV_MSG_ID_Set_RVDecoder_RPR_Sizes 36

typedef u32 VpuRvCustomMessage_ID;

typedef struct
{
    VpuRvCustomMessage_ID message_id;
    u32 num_sizes;
    u32 *sizes;
} VpuRvMsgSetDecoderRprSizes;

/* input and output flag definitions */
#define VPURV_DECODE_MORE_FRAMES    0x00000001
#define VPURV_DECODE_DONT_DRAW      0x00000002
#define VPURV_DECODE_KEY_FRAME      0x00000004
#define VPURV_DECODE_B_FRAME        0x00000008
#define VPURV_DECODE_LAST_FRAME     0x00000200

/* frame formats */
/* Reference picture format types */
typedef enum 
{
    VPURV_REF_FRM_RASTER_SCAN          = 0,
    VPURV_REF_FRM_TILED_DEFAULT        = 1
} VpuRvRefFrmFormat;

/* Output picture format types */
typedef enum
{
    VPURV_OUT_FRM_RASTER_SCAN          = 0,
    VPURV_OUT_FRM_TILED_8X4            = 1
} VpuRvOutFrmFormat;

/* input and output structures */
typedef struct
{
    i32 bIsValid;
    u32 ulSegmentOffset;
} codecSegmentInfo;

typedef struct
{
    u32 dataLength;
    i32 bInterpolateImage;
    u32 numDataSegments;
    codecSegmentInfo *pDataSegments;
    u32 flags;
    u32 timestamp;
    u32 streamBusAddr;
    u32 skipNonReference;
} VpuDecoderInParams;

typedef struct
{
    u32 numFrames;
    u32 notes;
    u32 timestamp;
    u32 width;
    u32 height;
    u8 *pOutFrame;
    VpuRvOutFrmFormat outputFormat;
} VpuRvDecoderOutParams;

/* decoder initialization structure */
typedef struct
{
    u16 outtype;
    u16 pels;
    u16 lines;
    u16 nPadWidth;
    u16 nPadHeight;
    u16 pad_to_32;
    u32 ulInvariants;
    i32 packetization;
    u32 ulStreamVersion;
} VpuDecoderInit;

/* decoding function */
VpuRvDecRet VpuRvDecDecode(u8 *pRV10Packets,
    u8   *pDecodedFrameBuffer, /* unused */
    void *pInputParams,
    void *pOutputParams,
    void *decInst);

/* a@nufront */
typedef struct
{
    u32 *virtualAddress;
    u32 busAddress;
    u32 size;
} VpuRvDecLinearMem;

/* initialization function */
VpuRvDecRet VpuRvDecInit(void *pRV10Init,
    void **pDecInst);

/* release function */
VpuRvDecRet VpuRvDecFree(void *decInst);

/* custom message handling function. Only Set_RPR_Sizes message implemented */
VpuRvDecRet VpuRvDecCustomMessage(void *msg_id, void *decInst);

/* unused, always returns DEC_NOTIMPL */
VpuRvDecRet VpuRvDecHiveMessage(void *msg, void *decInst);

/* function to obtain last decoded picture out from the decoder */
VpuRvDecRet VpuRvDecPeek(void *pOutputParams, void *decInst);

/* function to specify nbr of picture buffers to decoder */
VpuRvDecRet VpuRvDecSetNbrOfBuffers( u32 nbrBuffers, void *global );

/* function to specify reference frame format to use */
VpuRvDecRet VpuRvDecSetReferenceFrameFormat( 
    VpuRvRefFrmFormat referenceFrameFormat, void *global );
/* a@nufront start */
VpuRvDecRet VpuRvDecMallocLinear(void *decInst, u32 size, VpuRvDecLinearMem *info);

VpuRvDecRet VpuRvDecFreeLinear(void *decInst, VpuRvDecLinearMem *info);
/* a@nufront end */

#ifdef __cplusplus
}
#endif

#endif  /* __VPURVDECAPI_H__ */
