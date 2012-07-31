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
--  Description :  Implementation for VP8
--
------------------------------------------------------------------------------*/

#include "encoder_vp8.h"
#include "util.h" // Q16_FLOAT
#include "vp8encapi.h"
#include "OSAL.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

#if !defined (ENCH1)
#error "SPECIFY AN ENCODER PRODUCT (ENCH1) IN COMPILER DEFINES!"
#endif

#define REFERENCE_FRAME_AMOUNT 1

typedef struct ENCODER_VP8
{
  ENCODER_PROTOTYPE base;

  VP8EncInst instance;
  VP8EncIn encIn;
  OMX_U32 origWidth;
  OMX_U32 origHeight;
  //OMX_U32 nPFrames;
  OMX_U32 nIFrameCounter;
  OMX_U32 nEstTimeInc;
  OMX_U32 nAllowedPictureTypes;
  OMX_U32 nTotalFrames;
  OMX_BOOL bStabilize;
} ENCODER_VP8;

// destroy codec instance
static void encoder_destroy_vp8(ENCODER_PROTOTYPE* arg)
{
    ENCODER_VP8* this = (ENCODER_VP8*)arg;
    if ( this)
    {
        this->base.stream_start = 0;
        this->base.stream_end = 0;
        this->base.encode = 0;
        this->base.destroy = 0;

        if (this->instance)
        {
            VP8EncRelease(this->instance);
            this->instance = 0;
        }

        OSAL_Free( this);
    }
}

static CODEC_STATE encoder_stream_start_vp8(ENCODER_PROTOTYPE* arg,
                                              STREAM_BUFFER* stream)
{
    assert(arg);
    assert(stream);

    return CODEC_OK;
}

static CODEC_STATE encoder_stream_end_vp8(ENCODER_PROTOTYPE* arg,
                                            STREAM_BUFFER* stream)
{
    assert(arg);
    assert(stream);

    return CODEC_OK;
}

static CODEC_STATE encoder_encode_vp8(ENCODER_PROTOTYPE* arg, FRAME* frame,
                                        STREAM_BUFFER* stream)
{
    assert(arg);
    assert(frame);
    assert(stream);
    ENCODER_VP8* this = (ENCODER_VP8*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;
    VP8EncOut encOut;
    VP8EncRet ret;
    VP8EncRateCtrl rate_ctrl;
    OMX_U32 i;

    this->encIn.busLuma = frame->fb_bus_address;
    this->encIn.busChromaU = frame->fb_bus_address + (this->origWidth
            * this->origHeight);
    this->encIn.busChromaV = this->encIn.busChromaU + (this->origWidth
            * this->origHeight / 4);

    this->encIn.timeIncrement = this->nEstTimeInc;

    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    this->encIn.outBufSize = stream->buf_max_size;
    this->encIn.busOutBuf = stream->bus_address;

    // The bus address of the luminance component buffer of the picture to be stabilized.
    // Used only when video stabilization is enabled.
    if (this->bStabilize && (frame->fb_bufferSize == (2 * frame->fb_frameSize))) {
        this->encIn.busLumaStab = frame->fb_bus_address + frame->fb_frameSize;
    }
    else
    {
        this->encIn.busLumaStab = 0;
    }

    if (frame->frame_type == INTRA_FRAME)
    {
        this->encIn.codingType = VP8ENC_INTRA_FRAME;
    }
    else if (frame->frame_type == PREDICTED_FRAME)
    {
        this->encIn.codingType = VP8ENC_PREDICTED_FRAME;
    }
    else
    {
        assert( 0);
    }

    if (this->nTotalFrames == 0)
    {
        this->encIn.timeIncrement = 0;
    }

    // Set previous frame reference picture mode
    if (frame->bPrevRefresh && frame->bUsePrev)
        this->encIn.ipf = VP8ENC_REFERENCE_AND_REFRESH;
    else if (frame->bUsePrev)
        this->encIn.ipf = VP8ENC_REFERENCE;
    else if (frame->bPrevRefresh)
        this->encIn.ipf = VP8ENC_REFRESH;
    else
        this->encIn.ipf = VP8ENC_NO_REFERENCE_NO_REFRESH;

    // Set Golden frame reference picture mode
    if (frame->bGoldenRefresh && frame->bUseGolden)
        this->encIn.grf = VP8ENC_REFERENCE_AND_REFRESH;
    else if (frame->bUseGolden)
        this->encIn.grf = VP8ENC_REFERENCE;
    else if (frame->bGoldenRefresh)
        this->encIn.grf = VP8ENC_REFRESH;
    else
        this->encIn.grf = VP8ENC_NO_REFERENCE_NO_REFRESH;

    // Set Alternate frame reference picture mode
    if (frame->bAltRefresh && frame->bUseAlt)
        this->encIn.arf = VP8ENC_REFERENCE_AND_REFRESH;
    else if (frame->bUseAlt)
        this->encIn.arf = VP8ENC_REFERENCE;
    else if (frame->bAltRefresh)
        this->encIn.arf = VP8ENC_REFRESH;
    else
        this->encIn.arf = VP8ENC_NO_REFERENCE_NO_REFRESH;

    ret = VP8EncGetRateCtrl(this->instance, &rate_ctrl);

    if (ret == VP8ENC_OK && (rate_ctrl.bitPerSecond != frame->bitrate))
    {
        rate_ctrl.bitPerSecond = frame->bitrate;
        ret = VP8EncSetRateCtrl(this->instance, &rate_ctrl);
    }

    if (ret == VP8ENC_OK)
        ret = VP8EncStrmEncode(this->instance, &this->encIn, &encOut);

    switch (ret)
    {
        case VP8ENC_FRAME_READY:
        {
            this->nTotalFrames++;

            stream->streamlen = encOut.frameSize;

            // set each partition pointers and sizes
            for(i=0;i<9;i++)
            {
            stream->pOutBuf[i] = encOut.pOutBuf[i];
            stream->streamSize[i] = encOut.streamSize[i];
            }

            if (encOut.codingType == VP8ENC_INTRA_FRAME)
            {
                stat = CODEC_CODED_INTRA;
            }
            else if (encOut.codingType == VP8ENC_PREDICTED_FRAME)
            {
                stat = CODEC_CODED_PREDICTED;
            }
            else
            {
                stat = CODEC_OK;
            }
        }
        break;
        case VP8ENC_NULL_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case VP8ENC_INSTANCE_ERROR:
        {
            stat = CODEC_ERROR_UNSPECIFIED;
        }
        break;
        case VP8ENC_INVALID_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case VP8ENC_INVALID_STATUS:
        {
            stat = CODEC_ERROR_INVALID_STATE;
        }
        break;
        case VP8ENC_OUTPUT_BUFFER_OVERFLOW:
        {
            stat = CODEC_ERROR_BUFFER_OVERFLOW;
        }
        break;
        case VP8ENC_HW_TIMEOUT:
        {
            stat = CODEC_ERROR_HW_TIMEOUT;
        }
        break;
        case VP8ENC_HW_BUS_ERROR:
        {
            stat = CODEC_ERROR_HW_BUS_ERROR;
        }
        break;
        case VP8ENC_HW_RESET:
        {
            stat = CODEC_ERROR_HW_RESET;
        }
        break;
        case VP8ENC_SYSTEM_ERROR:
        {
            stat = CODEC_ERROR_SYSTEM;
        }
        break;
        case VP8ENC_HW_RESERVED:
        {
            stat = CODEC_ERROR_RESERVED;
        }
        break;
        default:
        {
            stat = CODEC_ERROR_UNSPECIFIED;
        }
        break;
    }

    return stat;
}

// create codec instance and initialize it
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_vp8(const VP8_CONFIG* params)
{
    VP8EncConfig cfg;

    memset(&cfg,0,sizeof(VP8EncConfig));

    cfg.width = params->common_config.nOutputWidth;
    cfg.height = params->common_config.nOutputHeight;
    //cfg.frameRateDenom = 1;
    //cfg.frameRateNum = cfg.frameRateDenom * Q16_FLOAT(params->common_config.nInputFramerate);
    cfg.frameRateNum = TIME_RESOLUTION;
    cfg.frameRateDenom = cfg.frameRateNum / Q16_FLOAT(params->common_config.nInputFramerate);
    cfg.refFrameAmount = REFERENCE_FRAME_AMOUNT;

    ENCODER_VP8* this = OSAL_Malloc(sizeof(ENCODER_VP8));

    this->instance = 0;
    memset( &this->encIn, 0, sizeof(VP8EncIn));
    this->origWidth = params->pp_config.origWidth;
    this->origHeight = params->pp_config.origHeight;
    this->base.stream_start = encoder_stream_start_vp8;
    this->base.stream_end = encoder_stream_end_vp8;
    this->base.encode = encoder_encode_vp8;
    this->base.destroy = encoder_destroy_vp8;

    this->bStabilize = params->pp_config.frameStabilization;
    this->nIFrameCounter = 0;
    this->nEstTimeInc = cfg.frameRateDenom;

    VP8EncRet ret = VP8EncInit(&cfg, &this->instance);

    // Setup coding control
    if (ret == VP8ENC_OK)
    {
        VP8EncCodingCtrl coding_ctrl;
        ret = VP8EncGetCodingCtrl(this->instance, &coding_ctrl);

        if (ret == VP8ENC_OK)
        {
            coding_ctrl.filterLevel = VP8ENC_FILTER_LEVEL_AUTO;
            coding_ctrl.filterSharpness = VP8ENC_FILTER_SHARPNESS_AUTO;
            coding_ctrl.filterType = 0;

            switch (params->vp8_config.eLevel)
            {
                case OMX_VIDEO_VP8Level_Version0:
                    coding_ctrl.interpolationFilter = 0;
                    break;
                case OMX_VIDEO_VP8Level_Version1:
                    coding_ctrl.interpolationFilter = 1;
                    coding_ctrl.filterType = 1;
                    break;
                case OMX_VIDEO_VP8Level_Version2:
                    coding_ctrl.interpolationFilter = 1;
                    coding_ctrl.filterLevel = 0;
                    break;
                case OMX_VIDEO_VP8Level_Version3:
                    coding_ctrl.interpolationFilter = 2;
                    coding_ctrl.filterLevel = 0;
                    break;
                default:
                    printf("Invalid VP8 eLevel\n");
                    coding_ctrl.interpolationFilter = 0;
                    break;
            }

            coding_ctrl.dctPartitions = params->vp8_config.nDCTPartitions;
            coding_ctrl.errorResilient = params->vp8_config.bErrorResilientMode;
            coding_ctrl.quarterPixelMv = 1;

            ret = VP8EncSetCodingCtrl(this->instance, &coding_ctrl);

        }
    }

    // Setup rate control
    if (ret == VP8ENC_OK)
    {
        VP8EncRateCtrl rate_ctrl;
        ret = VP8EncGetRateCtrl(this->instance, &rate_ctrl);
        if (ret == VP8ENC_OK)
        {

            // Optional. Set to -1 to use default.
            if (params->rate_config.nPictureRcEnabled >= 0)
            {
                rate_ctrl.pictureRc = params->rate_config.nPictureRcEnabled;
            }

            // Optional settings. Set to -1 to use default.
            if (params->rate_config.nQpDefault >= 0)
            {
                rate_ctrl.qpHdr = params->rate_config.nQpDefault;
            }

            // Optional settings. Set to -1 to use default.
            if (params->rate_config.nQpMin >= 0)
            {
                rate_ctrl.qpMin = params->rate_config.nQpMin;
                if(rate_ctrl.qpHdr != -1 && rate_ctrl.qpHdr < rate_ctrl.qpMin)
                {
                    rate_ctrl.qpHdr = rate_ctrl.qpMin;
                }
            }

            // Optional settings. Set to -1 to use default.
            if (params->rate_config.nQpMax > 0)
            {
                rate_ctrl.qpMax = params->rate_config.nQpMax;
                if(rate_ctrl.qpHdr > rate_ctrl.qpMax)
                {
                    rate_ctrl.qpHdr = rate_ctrl.qpMax;
                }
            }

            // Optional. Set to -1 to use default.
            if (params->rate_config.nTargetBitrate >= 0)
            {
                rate_ctrl.bitPerSecond = params->rate_config.nTargetBitrate;
            }

            switch (params->rate_config.eRateControl)
            {
                case OMX_Video_ControlRateDisable:
                    rate_ctrl.pictureSkip = 0;
                    break;
                case OMX_Video_ControlRateVariable:
                    rate_ctrl.pictureSkip = 0;
                    break;
                case OMX_Video_ControlRateConstant:
                    rate_ctrl.pictureSkip = 0;
                    break;
                case OMX_Video_ControlRateVariableSkipFrames:
                    rate_ctrl.pictureSkip = 1;
                    break;
                case OMX_Video_ControlRateConstantSkipFrames:
                    rate_ctrl.pictureSkip = 1;
                    break;
                case OMX_Video_ControlRateMax:
                    rate_ctrl.pictureSkip = 0;
                    break;
                default:
                    break;
            }

            ret = VP8EncSetRateCtrl(this->instance, &rate_ctrl);
        }
    }

    // Setup preprocessing
    if (ret == VP8ENC_OK)
    {
        VP8EncPreProcessingCfg pp_config;

        ret = VP8EncGetPreProcessing(this->instance, &pp_config);

        // input image size
        pp_config.origWidth = params->pp_config.origWidth;
        pp_config.origHeight = params->pp_config.origHeight;

        // cropping offset
        pp_config.xOffset = params->pp_config.xOffset;
        pp_config.yOffset = params->pp_config.yOffset;

        switch (params->pp_config.formatType)
        {
            case OMX_COLOR_FormatYUV420PackedPlanar:
            case OMX_COLOR_FormatYUV420Planar:
                pp_config.inputType = VP8ENC_YUV420_PLANAR;
                break;
            case OMX_COLOR_FormatYUV420PackedSemiPlanar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
                pp_config.inputType = VP8ENC_YUV420_SEMIPLANAR;
                break;
            case OMX_COLOR_FormatYCbYCr:
                pp_config.inputType = VP8ENC_YUV422_INTERLEAVED_YUYV;
                break;
            case OMX_COLOR_FormatCbYCrY:
                pp_config.inputType = VP8ENC_YUV422_INTERLEAVED_UYVY;
                break;
            case OMX_COLOR_Format16bitRGB565:
                pp_config.inputType = VP8ENC_RGB565;
                break;
            case OMX_COLOR_Format16bitBGR565:
                pp_config.inputType = VP8ENC_BGR565;
                break;
            case OMX_COLOR_Format12bitRGB444:
                pp_config.inputType = VP8ENC_RGB444;
                break;
            case OMX_COLOR_Format16bitARGB4444:
                pp_config.inputType = VP8ENC_RGB444;
                break;
            case OMX_COLOR_Format16bitARGB1555:
                pp_config.inputType = VP8ENC_RGB555;
                break;
            case OMX_COLOR_Format24bitRGB888:
            case OMX_COLOR_Format25bitARGB1888:
            case OMX_COLOR_Format32bitARGB8888:
                pp_config.inputType = VP8ENC_RGB888;
                break;
            case OMX_COLOR_Format24bitBGR888:
                pp_config.inputType = VP8ENC_BGR888;
                break;
            default:
                ret = VP8ENC_INVALID_ARGUMENT;
                break;
        }

        switch (params->pp_config.angle)
        {
            case 0:
                pp_config.rotation = VP8ENC_ROTATE_0;
                break;
            case 90:
                pp_config.rotation = VP8ENC_ROTATE_90R;
                break;
            case 270:
                pp_config.rotation = VP8ENC_ROTATE_90L;
                break;
            default:
                ret = VP8ENC_INVALID_ARGUMENT;
                break;
        }

        // Enables or disables the video stabilization function. Set to a non-zero value will
        // enable the stabilization. The input image dimensions (origWidth, origHeight)
        // have to be at least 8 pixels bigger than the final encoded image dimensions. Also when
        // enabled the cropping offset (xOffset, yOffset) values are ignored.
        this->bStabilize = params->pp_config.frameStabilization;
        pp_config.videoStabilization = params->pp_config.frameStabilization;

        if (ret == VP8ENC_OK)
        {
            ret = VP8EncSetPreProcessing(this->instance, &pp_config);
        }
    }

    if (ret != VP8ENC_OK)
    {
        OSAL_Free(this);
        return NULL;
    }

    return (ENCODER_PROTOTYPE*) this;
}

CODEC_STATE HantroHwEncOmx_encoder_frame_rate_vp8(ENCODER_PROTOTYPE* arg, OMX_U32 xFramerate)
{
    ENCODER_VP8* this = (ENCODER_VP8*)arg;

    this->nEstTimeInc = (OMX_U32) (TIME_RESOLUTION / Q16_FLOAT(xFramerate));

    return CODEC_OK;
}
