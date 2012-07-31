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
--  Description :  Implementation for WEBP
--
------------------------------------------------------------------------------*/

#include "encoder_webp.h"
#include "util.h" // Q16_FLOAT
#include "vp8encapi.h"
#include "OSAL.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

typedef struct ENCODER_WEBP
{
  ENCODER_PROTOTYPE base;

  VP8EncInst instance;
  VP8EncIn encIn;
  OMX_U32 origWidth;
  OMX_U32 origHeight;
  OMX_U32 croppedWidth;
  OMX_U32 croppedHeight;
  //OMX_BOOL frameHeader;
  OMX_BOOL leftoverCrop;
  OMX_BOOL sliceMode;
  OMX_U32 sliceHeight;
  OMX_U32 sliceNumber;
  OMX_COLOR_FORMATTYPE frameType;
} ENCODER_WEBP;


//! Destroy codec instance.
static void encoder_destroy_webp(ENCODER_PROTOTYPE* arg)
{
    ENCODER_WEBP* this = (ENCODER_WEBP*)arg;
    if (this)
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

        OSAL_Free(this);
    }
}

// WEBP does not support streaming.
static CODEC_STATE encoder_stream_start_webp(ENCODER_PROTOTYPE* arg, STREAM_BUFFER* stream)
{
    assert(arg);
    assert(stream);

    /*ENCODER_WEBP* this = (ENCODER_WEBP*)arg;

    CODEC_STATE stat = CODEC_OK;

    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    if (stream->buf_max_size > 16*1024*1024)
    {
        this->encIn.outBufSize = 16*1024*1024;
    }
    else
    {
        this->encIn.outBufSize = stream->buf_max_size;
    }
    this->encIn.busOutBuf = stream->bus_address;
    return stat;*/
    return CODEC_OK;
}

static CODEC_STATE encoder_stream_end_webp(ENCODER_PROTOTYPE* arg, STREAM_BUFFER* stream)
{
    assert(arg);
    assert(stream);

    /*ENCODER_WEBP* this = (ENCODER_WEBP*)arg;

    CODEC_STATE stat = CODEC_OK;

    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    if (stream->buf_max_size > 16*1024*1024)
    {
        this->encIn.outBufSize = 16*1024*1024;
    }
    else
    {
        this->encIn.outBufSize = stream->buf_max_size;
    }
    this->encIn.busOutBuf = stream->bus_address;
    return stat;*/
    return CODEC_OK;
}


// Encode a simple raw frame using underlying hantro codec.
// Converts OpenMAX structures to corresponding Hantro codec structures,
// and calls API methods to encode frame.
// Constraints:
// 1. only full frame encoding is supported, i.e., no thumbnails

static CODEC_STATE encoder_encode_webp(ENCODER_PROTOTYPE* arg, FRAME* frame, STREAM_BUFFER* stream)
{
    assert(arg);
    assert(frame);
    assert(stream);

    ENCODER_WEBP* this = (ENCODER_WEBP*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;
    OMX_U32 i;

#if 0 //Not supported
    if (this->sliceMode) 
    {
        OMX_U32 tmpSliceHeight = this->sliceHeight;

        // Last slice may be smaller so calculate slice height.
        if ((this->sliceHeight * this->sliceNumber) > this->origHeight)
        {
            tmpSliceHeight = this->origHeight - ((this->sliceNumber - 1) * this->sliceHeight);
        }


        if (this->leftoverCrop == OMX_TRUE)
        {
            /*  If picture full (Enough slices) start encoding again */
            if ((this->sliceHeight * this->sliceNumber) >= this->origHeight)
            {
                this->leftoverCrop = OMX_FALSE;
                this->sliceNumber = 1;
            }
            else
            {
                stream->streamlen = 0;
                return CODEC_OK;
            }
        }

        if (this->frameType == OMX_COLOR_FormatYUV422Planar)
        {
            if ((tmpSliceHeight % 8) != 0)
            {
                return CODEC_ERROR_INVALID_ARGUMENT;
            }
        }
        else
        {
            if ((tmpSliceHeight % 16) != 0)
            {
                return CODEC_ERROR_INVALID_ARGUMENT;
            }
        }

        this->encIn.busLuma = frame->fb_bus_address;
        this->encIn.busChromaU = frame->fb_bus_address + (this->origWidth * tmpSliceHeight); // Cb or U
        if (this->frameType == OMX_COLOR_FormatYUV422Planar)
        {
            this->encIn.busChromaV = this->encIn.busChromaU + (this->origWidth * tmpSliceHeight / 2); // Cr or V
        }
        else
        {
            this->encIn.busChromaV = this->encIn.busChromaU + (this->origWidth * tmpSliceHeight / 4); // Cr or V
        }
    }
    else
    {
        this->encIn.busLuma = frame->fb_bus_address;
        this->encIn.busChromaU = frame->fb_bus_address + (this->origWidth * this->origHeight); // Cb or U
        if (this->frameType == OMX_COLOR_FormatYUV422Planar)
        {
            this->encIn.busChromaV = this->encIn.busChromaU + (this->origWidth * this->origHeight / 2); // Cr or V
        }
        else
        {
            this->encIn.busChromaV = this->encIn.busChromaU + (this->origWidth * this->origHeight / 4); // Cr or V
        }
    }
#endif

    this->encIn.busLuma = frame->fb_bus_address;
    this->encIn.busChromaU = frame->fb_bus_address + (this->origWidth
            * this->origHeight);
    this->encIn.busChromaV = this->encIn.busChromaU + (this->origWidth
            * this->origHeight / 4);
    this->encIn.timeIncrement = 1;

    this->encIn.pOutBuf = (u32 *) stream->bus_data; // output stream buffer
    this->encIn.busOutBuf = stream->bus_address; // output bus address
    
    //this->encIn.outBufSize = stream->buf_max_size; // max size of output buffer
    if (stream->buf_max_size > 16*1024*1024)
    {
        this->encIn.outBufSize = 16*1024*1024;
    }
    else
    {
        this->encIn.outBufSize = stream->buf_max_size;
    }

    VP8EncOut encOut;

    VP8EncRet ret = VP8EncStrmEncode(this->instance, &this->encIn, &encOut);
    

    switch (ret)
    {
        /*case VP8ENC_RESTART_INTERVAL:
        {
            // return encoded slice
            stream->streamlen = encOut.jfifSize;
            stat = CODEC_CODED_SLICE;
            this->sliceNumber++;
        }
        break;*/
        case VP8ENC_FRAME_READY:
        {
            /*if((this->sliceMode == OMX_TRUE)
            && (this->sliceHeight * this->sliceNumber) >= this->croppedHeight)
            {
                this->leftoverCrop  = OMX_TRUE;
            }*/
            
            stream->streamlen = encOut.frameSize;
            //printf("Frame size %d\n", stream->streamlen);

            // set each partition pointers and sizes
            for(i=0;i<9;i++)
            {
            stream->pOutBuf[i] = encOut.pOutBuf[i];
            stream->streamSize[i] = encOut.streamSize[i];
            }
             
            stat = CODEC_OK;            
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

// Create WEBP codec instance and initialize it.
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_webp(const WEBP_CONFIG* params)
{
    assert(params);

    VP8EncConfig cfg;
    VP8EncRet ret;
    i32 qValues[10] = {120,108,96,84,72,60,48,24,12,0}; //VP8 Quantization levels
    
    memset(&cfg,0,sizeof(VP8EncConfig));
    
    cfg.width = params->codingWidth;
    cfg.height = params->codingHeight;
    cfg.frameRateDenom = 1;
    cfg.frameRateNum = 1 ;//cfg.frameRateDenom * Q16_FLOAT(params->common_config.nInputFramerate);
    cfg.refFrameAmount = 1;
    
    
    ENCODER_WEBP* this = OSAL_Malloc(sizeof(ENCODER_WEBP));
    
    this->croppedWidth = params->codingWidth;
    this->croppedHeight = params->codingHeight;

    // encIn struct init
    this->instance = 0;
    memset( &this->encIn, 0, sizeof(VP8EncIn));
    this->origWidth = params->pp_config.origWidth;
    this->origHeight = params->pp_config.origHeight;
    this->sliceNumber = 1;
    this->leftoverCrop = 0;
    this->frameType = params->pp_config.formatType;

    // initialize static methods
    this->base.stream_start = encoder_stream_start_webp;
    this->base.stream_end = encoder_stream_end_webp;
    this->base.encode = encoder_encode_webp;
    this->base.destroy = encoder_destroy_webp;
    
    // slice mode configuration
    /*if (params->codingType > 0)
    {
        if (params->sliceHeight > 0)
        {
            this->sliceMode = OMX_TRUE;
            if (this->frameType == OMX_COLOR_FormatYUV422Planar)
            {
                cfg.restartInterval = params->sliceHeight / 8;
            }
            else
            {
                cfg.restartInterval = params->sliceHeight / 16;
            }
            this->sliceHeight = params->sliceHeight;
        }
        else
        {
            ret = VP8ENC_INVALID_ARGUMENT;
        }
    }
    else
    {
        this->sliceMode = OMX_FALSE;
        cfg.restartInterval = 0;
        this->sliceHeight = 0;
    }*/
    
    this->sliceMode = OMX_FALSE;
    this->sliceHeight = 0;

    ret = VP8EncInit(&cfg, &this->instance);
    
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
            coding_ctrl.interpolationFilter = 0;
            coding_ctrl.dctPartitions = 0;
            coding_ctrl.errorResilient = 0;
            coding_ctrl.quarterPixelMv = 0;

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
            if(params->qLevel == -1)
                rate_ctrl.qpHdr = -1;
            else
                rate_ctrl.qpHdr = qValues[params->qLevel];

            //rate_ctrl.pictureRc = 1;

            ret = VP8EncSetRateCtrl(this->instance, &rate_ctrl);
        }
    }
    
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

        pp_config.videoStabilization = 0;

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
            case OMX_COLOR_Format16bitARGB4444:
            case OMX_COLOR_Format12bitRGB444:
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

