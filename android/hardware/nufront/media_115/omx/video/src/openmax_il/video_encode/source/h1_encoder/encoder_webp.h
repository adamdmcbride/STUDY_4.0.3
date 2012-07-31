/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  WEBP encoder header
--
------------------------------------------------------------------------------*/

#ifndef ENCODER_WEBP_H_
#define ENCODER_WEBP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "codec.h"
#include "OMX_Image.h"
#include "vp8encapi.h"
    
typedef struct WEBP_CONFIG {
    // ----------config-----------

    OMX_S32 qLevel;     // The quantization level. [0..9] -1=auto
    OMX_U32 sliceHeight;    

    OMX_U32 codingWidth;
    OMX_U32 codingHeight;
    OMX_U32 yuvFormat; // output picture YUV format
    PRE_PROCESSOR_CONFIG pp_config;
    

} WEBP_CONFIG;

// create codec instance
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_webp(const WEBP_CONFIG* config);

#ifdef __cplusplus
}
#endif

#endif /*CODEC_WEBP_H_*/
