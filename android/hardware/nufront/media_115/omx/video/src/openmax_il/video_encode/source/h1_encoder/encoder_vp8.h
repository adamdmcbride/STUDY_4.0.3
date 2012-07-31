/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2011 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  VP8 encoder header
--
------------------------------------------------------------------------------*/

#ifndef ENCODER_VP8_H_
#define ENCODER_VP8_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "codec.h"
#include "OMX_Video.h"

typedef struct VP8_CONFIG_
{
    /*OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE error_ctrl_config;*/

    OMX_VIDEO_PARAM_VP8TYPE     vp8_config;
    
    ENCODER_COMMON_CONFIG       common_config;
    RATE_CONTROL_CONFIG         rate_config;
    PRE_PROCESSOR_CONFIG        pp_config;
} VP8_CONFIG;

// create codec instance
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_vp8(const VP8_CONFIG* config);

// change encoding frame rate
CODEC_STATE HantroHwEncOmx_encoder_frame_rate_vp8(ENCODER_PROTOTYPE* arg, OMX_U32 xFramerate);

#ifdef __cplusplus
}
#endif
#endif /*ENCODER_VP8_H_*/
