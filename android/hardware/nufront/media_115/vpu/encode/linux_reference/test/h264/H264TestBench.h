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
--  Abstract : H264 Encoder testbench for linux
--
------------------------------------------------------------------------------*/
#ifndef _H264TESTBENCH_H_
#define _H264TESTBENCH_H_

#include "basetype.h"
#include "h264encapi.h"

/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define DEFAULT -100
#define MAX_BPS_ADJUST 20

/* Structure for command line options */
typedef struct
{
    char *input;
    char *inputMvc;
    char *output;
    char *userData;
    i32 firstPic;
    i32 lastPic;
    i32 width;
    i32 height;
    i32 lumWidthSrc;
    i32 lumHeightSrc;
    i32 horOffsetSrc;
    i32 verOffsetSrc;
    i32 outputRateNumer;
    i32 outputRateDenom;
    i32 inputRateNumer;
    i32 inputRateDenom;
    i32 level;
    i32 hrdConformance;
    i32 cpbSize;
    i32 intraPicRate;
    i32 constIntraPred;
    i32 disableDeblocking;
    i32 mbRowPerSlice;
    i32 qpHdr;
    i32 qpMin;
    i32 qpMax;
    i32 bitPerSecond;
    i32 picRc;
    i32 mbRc;
    i32 picSkip;
    i32 rotation;
    i32 inputFormat;
    i32 colorConversion;
    i32 videoBufferSize;
    i32 videoRange;
    i32 chromaQpOffset;
    i32 filterOffsetA;
    i32 filterOffsetB;
    i32 trans8x8;
    i32 enableCabac;
    i32 cabacInitIdc;
    i32 testId;
    i32 burst;
    i32 bursttype;
    i32 quarterPixelMv;
    i32 sei;
    i32 byteStream;
    i32 videoStab;
    i32 gopLength;
    i32 intraQpDelta;
    i32 fixedIntraQp;    
    i32 mbQpAdjustment;
    i32 testParam;
    i32 mvOutput;
    i32 mvPredictor;
    i32 cirStart;
    i32 cirInterval;
    i32 intraSliceMap1;
    i32 intraSliceMap2;
    i32 intraSliceMap3;
    i32 intraAreaEnable;
    i32 intraAreaTop;
    i32 intraAreaLeft;
    i32 intraAreaBottom;
    i32 intraAreaRight;
    i32 roi1AreaEnable;
    i32 roi2AreaEnable;
    i32 roi1AreaTop;
    i32 roi1AreaLeft;
    i32 roi1AreaBottom;
    i32 roi1AreaRight;
    i32 roi2AreaTop;
    i32 roi2AreaLeft;
    i32 roi2AreaBottom;
    i32 roi2AreaRight;
    i32 roi1DeltaQp;
    i32 roi2DeltaQp;
    i32 viewMode;
    i32 psnr;
    i32 bpsAdjustFrame[MAX_BPS_ADJUST];
    i32 bpsAdjustBitrate[MAX_BPS_ADJUST];
} commandLine_s;

void TestNaluSizes(i32 * pNaluSizes, u8 * stream, u32 strmSize, u32 picBytes);


#endif
