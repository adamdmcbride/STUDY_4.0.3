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

#ifndef META_DATA_H_

#define META_DATA_H_

#include <sys/types.h>

#include <stdint.h>

#include <utils/RefBase.h>
#include <utils/KeyedVector.h>

namespace android {

// The following keys map to int32_t data unless indicated otherwise.
enum {
    /* a@nufront begin */
    kKeyMetaKeyStart       = 'MKST',
    kKeyFFmpegCodecPointer = 'fCPT',
    kKeyFFmpegCodecPointerSize = 'fCPS',
    /* a@nufront end */

    kKeyMIMEType          = 'mime',  // cstring
    kKeyWidth             = 'widt',  // int32_t, image pixel
    kKeyHeight            = 'heig',  // int32_t, image pixel
    kKeyDisplayWidth      = 'dWid',  // int32_t, display/presentation
    kKeyDisplayHeight     = 'dHgt',  // int32_t, display/presentation
    /*a@nufront start*/
    kKeyCompAlgo          = 'algo',  // int32_t, mkv compression algorithm
    kKeyCompSettings      = 'cmps',  // int32_t, settings needed by decompressor

    kKeyTimeType          = 'Bdts',  // int, pts or dts
    kKeyMPEG4             = 'mpg4',  // raw data
    kKeyMPEG2             = 'mpg2',  // raw data
    kKeyVC1Type           = 'vc1t',  // raw data
    /*a@nufront end*/

    // a rectangle, if absent assumed to be (0, 0, width - 1, height - 1)
    kKeyCropRect          = 'crop',

    kKeyRotation          = 'rotA',  // int32_t (angle in degrees)
    /*a@nufront start*/
    kKeyFlip              = 'flip',  // int32_t (flip horizontally or vertically)
    // a rectangle, if absent assumed to be (0, 0, width - 1, height - 1), store the original frame size
    kKeyVideoRect          = 'Rect',
    /*a@nufront end*/
    kKeyIFramesInterval   = 'ifiv',  // int32_t
    kKeyStride            = 'strd',  // int32_t
    kKeySliceHeight       = 'slht',  // int32_t
    kKeyChannelCount      = '#chn',  // int32_t
    kKeySampleRate        = 'srte',  // int32_t (audio sampling rate Hz)
    kKeyFrameRate         = 'frmR',  // int32_t (video frame rate fps)
    kKeyBitRate           = 'brte',  // int32_t (bps)
    kKeyESDS              = 'esds',  // raw data
    kKeyAVCC              = 'avcc',  // raw data
    /*a@nufront start*/
    kKeyMPEG4InMKV        = 'mp4m',  // raw data
    /*a@nufront end*/
    kKeyD263              = 'd263',  // raw data
    kKeyVorbisInfo        = 'vinf',  // raw data
    kKeyVorbisBooks       = 'vboo',  // raw data
    kKeyWantsNALFragments = 'NALf',
    kKeyIsSyncFrame       = 'sync',  // int32_t (bool)
    kKeyIsCodecConfig     = 'conf',  // int32_t (bool)
    kKeyTime              = 'time',  // int64_t (usecs)
    kKeyDecodingTime      = 'decT',  // int64_t (decoding timestamp in usecs)
    kKeyNTPTime           = 'ntpT',  // uint64_t (ntp-timestamp)
    kKeyTargetTime        = 'tarT',  // int64_t (usecs)
    kKeyDriftTime         = 'dftT',  // int64_t (usecs)
    kKeyAnchorTime        = 'ancT',  // int64_t (usecs)
    kKeyDuration          = 'dura',  // int64_t (usecs)
    kKeyColorFormat       = 'colf',
    kKeyPlatformPrivate   = 'priv',  // pointer
    kKeyDecoderComponent  = 'decC',  // cstring
    kKeyBufferID          = 'bfID',
    kKeyMaxInputSize      = 'inpS',
    kKeyThumbnailTime     = 'thbT',  // int64_t (usecs)
    kKeyTrackID           = 'trID',
    kKeyIsDRM             = 'idrm',  // int32_t (bool)
    kKeyAlbum             = 'albu',  // cstring
    kKeyArtist            = 'arti',  // cstring
    kKeyAlbumArtist       = 'aart',  // cstring
    kKeyComposer          = 'comp',  // cstring
    kKeyGenre             = 'genr',  // cstring
    kKeyTitle             = 'titl',  // cstring
    kKeyYear              = 'year',  // cstring
    kKeyAlbumArt          = 'albA',  // compressed image data
    kKeyAlbumArtMIME      = 'alAM',  // cstring
    kKeyAuthor            = 'auth',  // cstring
    kKeyCDTrackNumber     = 'cdtr',  // cstring
    kKeyDiscNumber        = 'dnum',  // cstring
    kKeyDate              = 'date',  // cstring
    kKeyWriter            = 'writ',  // cstring
    kKeyCompilation       = 'cpil',  // cstring
    kKeyLocation          = 'loc ',  // cstring
    kKeyTimeScale         = 'tmsl',  // int32_t
    // video profile and level
    kKeyVideoProfile      = 'vprf',  // int32_t
    kKeyVideoLevel        = 'vlev',  // int32_t
    // Set this key to enable authoring files in 64-bit offset
    kKey64BitFileOffset   = 'fobt',  // int32_t (bool)
    kKey2ByteNalLength    = '2NAL',  // int32_t (bool)

    // Identify the file output format for authoring
    // Please see <media/mediarecorder.h> for the supported
    // file output formats.
    kKeyFileType          = 'ftyp',  // int32_t

    // Track authoring progress status
    // kKeyTrackTimeStatus is used to track progress in elapsed time
    kKeyTrackTimeStatus   = 'tktm',  // int64_t

    kKeyNotRealTime       = 'ntrt',  // bool (int32_t)

    // Ogg files can be tagged to be automatically looping...
    kKeyAutoLoop          = 'autL',  // bool (int32_t)

    kKeyValidSamples      = 'valD',  // int32_t

    kKeyIsUnreadable      = 'unre',  // bool (int32_t)

    // An indication that a video buffer has been rendered.
    kKeyRendered          = 'rend',  // bool (int32_t)

    // The language code for this media
    kKeyMediaLanguage     = 'lang',  // cstring

    // To store the timed text format data
    kKeyTextFormatData    = 'text',  // raw data

    kKeyRequiresSecureBuffers = 'secu',  // bool (int32_t)
    /*a@nufront start: add meta data key for AVCodecContext*/
    kKeyCodecCtxAVClass            = 'fAVC'    ,
    kKeyCodecCtxBitRate            = 'fBRt'    ,
//    kKeyCodecCtxBitRateTolerance   = 'fBRT'    ,
    kKeyCodecCtxFlags              = 'fFLG'    ,
    kKeyCodecCtxSubID              = 'fSID'  ,
    kKeyCodecCtxMeMethod           = 'fMeM'  ,
    kKeyCodecCtxExtraData          = 'fExD'  ,
    kKeyCodecCtxExtraDataSize      = 'fExS'  ,
    kKeyCodecCtxTimeBaseNum        = 'fTBN' ,
    kKeyCodecCtxTimeBaseDen        = 'fTBD'  ,
    kKeyCodecCtxWidth              = 'fWd '  ,
    kKeyCodecCtxHeight             = 'fHt '  ,
    kKeyCodecCtxGopSize            = 'fGop'  ,
    kKeyCodecCtxPixFmt             = 'fPfm'  ,
 //   kKeyCodecCtxDrawHorizBandFunc  = 'fDHB'  ,
    kKeyCodecCtxSampleRate         = 'fSR '  ,
    kKeyCodecCtxChannels           = 'fChn',
    kKeyCodecCtxSampleFmt          = 'fSF '  ,
    kKeyCodecCtxFrameSize          = 'fFsz'  ,
    kKeyCodecCtxFrameNumber        = 'fFNm'  ,
 //   kKeyCodecCtxDelay              = 'fDel'  ,
 //   kKeyCodecCtxQcompress          = 'fQc '  ,
 //   kKeyCodecCtxQblur                ,
 //   kKeyCodecCtxQmin                ,
 //   kKeyCodecCtxQmax                ,
 //   kKeyCodecCtxMaxQdiff            ,
 //   kKeyCodecCtxMaxBFrames          ,
 //   kKeyCodecCtxBQuantFactor        ,
 //   kKeyCodecCtxRcStrategy          ,
 //   kKeyCodecCtxBFrameStrategy      ,
//    kKeyCodecCtxCodec               ,
 //   kKeyCodecCtxPrivData            ,
//    kKeyCodecCtxRtpPayloadSize        ,
 //   kKeyCodecCtxRtpCallbackFunc     ,
 //   kKeyCodecCtxMvBits              ,
 //   kKeyCodecCtxHeaderBits          ,
 //   kKeyCodecCtxITexBits            ,
 //   kKeyCodecCtxPTexBits            ,
 //   kKeyCodecCtxICount              ,
 //   kKeyCodecCtxPCount              ,
 //   kKeyCodecCtxSkipCount           ,
 //   kKeyCodecCtxMiscBits            ,
    kKeyCodecCtxFrameBits           = 'fFBt',
 //   kKeyCodecCtxOpaque              ,
 //   kKeyCodecCtxCodecNameArry       ,
    kKeyCodecCtxCodecType           = 'fCTp',
    kKeyCodecCtxCodecID             = 'fCID',
    kKeyCodecCtxCodecTag            = 'fCTg',
 //   kKeyCodecCtxWorkaroundBugs      ,
 //   kKeyCodecCtxLumaElimThreshold   ,
 //   kKeyCodecCtxChromaElimThreshold ,
 //   kKeyCodecCtxStrictStdCompliance ,
 //   kKeyCodecCtxBQuantOffset        ,
 //   kKeyCodecCtxErrorRecognition    ,
 //   kKeyCodecCtxGetBufferFunc       ,
 //   kKeyCodecCtxReleaseBufferFunc   ,
 //   kKeyCodecCtxHasBFrames          ,
    kKeyCodecCtxBlockAlign          = 'fBkA',
 //   kKeyCodecCtxParseOnly           ,
 //   kKeyCodecCtxMpegQuant           ,
 //   kKeyCodecCtxStatsOut            ,
 //   kKeyCodecCtxStatsIn             ,
 //   kKeyCodecCtxRcQsquish           ,
 //   kKeyCodecCtxRcQmodAmp           ,
 //   kKeyCodecCtxRcQmodFreq          ,
 //   kKeyCodecCtxRcOverride          ,
 //   kKeyCodecCtxRcOverrideCount     ,
 //   kKeyCodecCtxRcEq                ,
 //   kKeyCodecCtxRcMaxRate           ,
 //   kKeyCodecCtxRcMinRate           ,
 //   kKeyCodecCtxRcBufferSize        ,
 //   kKeyCodecCtxRcBufferAggressivity,
 //   kKeyCodecCtxIQuantFactor,
 //   kKeyCodecCtxIQuantOffset,
 //   kKeyCodecCtxRcInitialCplx,
 //   kKeyCodecCtxDctAlgo             ,
 //   kKeyCodecCtxLumiMasking         ,
 //   kKeyCodecCtxTemporalCplxMasking ,
 //   kKeyCodecCtxSpatialCplxMasking  ,
 //   kKeyCodecCtxPMasking,
 //   kKeyCodecCtxDarkMasking,
 //   kKeyCodecCtxIdctAlgo,
 //   kKeyCodecCtxSliceCount,
 //   kKeyCodecCtxSliceOffset,
 //   kKeyCodecCtxErrorConcealment,
 //   kKeyCodecCtxDspMask,
    kKeyCodecCtxBitsPerCodedSample = 'fBPS',
 //   kKeyCodecCtxPredictionMethod,
    kKeyCodecCtxSampleAspectRationNum = 'fSAN',
    kKeyCodecCtxSampleAspectRationDen = 'fSAD',
 //   kKeyCodecCtxCodedFrame,
 //   kKeyCodecCtxDebug,
 //   kKeyCodecCtxDebugMv,
 //   kKeyCodecCtxErrorArry,
 //   kKeyCodecCtxMeCmp,
 //   kKeyCodecCtxMeSubCmp,
 //   kKeyCodecCtxMbCmp,
 //   kKeyCodecCtxIldctCmp,
 //   kKeyCodecCtxDiaSize,
 //   kKeyCodecCtxLastPredictorCount,
 //   kKeyCodecCtxPreMe,
 //   kKeyCodecCtxMePreCmp,
 //   kKeyCodecCtxPreDiaSize,
 //   kKeyCodecCtxMeSubpelQuality,
 //   kKeyCodecCtxGetFormatFunc,
 //   kKeyCodecCtxDtgActiveFormat,
 //   kKeyCodecCtxMeRange,
 //   kKeyCodecCtxIntraQuantBias,
 //   kKeyCodecCtxInterQuantBias,
 //   kKeyCodecCtxColorTableId,
 //   kKeyCodecCtxInternalBufferCount,
 //   kKeyCodecCtxInternalBuffer,
 //   kKeyCodecCtxGlobalQuality,
 //   kKeyCodecCtxCoderType,
 //   kKeyCodecCtxContextModel,
 //   kKeyCodecCtxSliceFlags,
 //   kKeyCodecCtxXvmcAcceleration,
 //   kKeyCodecCtxMbDecision,
 //   kKeyCodecCtxIntraMatrix,
 //   kKeyCodecCtxInterMatrix,
 //   kKeyCodecCtxStreamCodecTag,
 //   kKeyCodecCtxScenechangeThreshold,
 //   kKeyCodecCtxLmin,
 //   kKeyCodecCtxLmax,
 //   kKeyCodecCtxPalctrl,
 //   kKeyCodecCtxNoiseReduction,
 //   kKeyCodecCtxRegetBufferFunc,
 //   kKeyCodecCtxRcInitialBufferOccupancy,
 //   kKeyCodecCtxInterThreshold,
    kKeyCodecCtxFlags2 = 'fFl2',
 //   kKeyCodecCtxErrorRate,
 //   kKeyCodecCtxAntialiasAlgo,
 //   kKeyCodecCtxQuantizerNoiseShaping,
 //   kKeyCodecCtxThreadCount,
 //   kKeyCodecCtxExecuteFunc,
 //   kKeyCodecCtxThreadOpaque,
 //   kKeyCodecCtxMeThreshold,
 //   kKeyCodecCtxMbThreshold,
 //   kKeyCodecCtxIntraDcPrecision,
 //   kKeyCodecCtxNsseWeight,
 //   kKeyCodecCtxSkipTop,
 //   kKeyCodecCtxSkipBottom,
    kKeyCodecCtxProfile = 'fPrf',
    kKeyCodecCtxLevel = 'fLev',
 //   kKeyCodecCtxLowres,
    kKeyCodecCtxCodedWidth,
    kKeyCodecCtxCodedHeight = 'fCHt',
 //   kKeyCodecCtxFrameSkipThreshold,
 //   kKeyCodecCtxFrameSkipFactor,
 //   kKeyCodecCtxFrameSkipExp,
 //   kKeyCodecCtxFrameSkipCmp,
 //   kKeyCodecCtxBorderMasking,
 //   kKeyCodecCtxMbLmin,
//    kKeyCodecCtxMbLmax,
 //   kKeyCodecCtxMePenaltyCompensation,
 //   kKeyCodecCtxSkipLoopFilter,
 //   kKeyCodecCtxSkipIdct,
 //   kKeyCodecCtxSkipFrame,
 //   kKeyCodecCtxBidirRefine,
 //   kKeyCodecCtxBrdScale,
 //   kKeyCodecCtxCrf,
 //   kKeyCodecCtxCqp,
 //   kKeyCodecCtxKeyintMin,
 //   kKeyCodecCtxRefs,
 //   kKeyCodecCtxChromaoffset,
 //   kKeyCodecCtxBframebias,
 //   kKeyCodecCtxTrellis,
 //   kKeyCodecCtxComplexityblur,
 //   kKeyCodecCtxDeblockalpha,
 //   kKeyCodecCtxDeblockbeta,
 //   kKeyCodecCtxPartitions,
 //   kKeyCodecCtxDirectPred,
 //   kKeyCodecCtxCutoff,
 //   kKeyCodecCtxScenechangeFactor,
 //   kKeyCodecCtxMv0Threshold,
 //   kKeyCodecCtxBSensitivity,
 //   kKeyCodecCtxCompressionLevel,
 //   kKeyCodecCtxMinPredictionOrder,
 //   kKeyCodecCtxMaxPredictionOrder,
 //   kKeyCodecCtxLpcCoeffPrecision,
 //   kKeyCodecCtxPredicitionOrderMethod,
 //   kKeyCodecCtxMinPartitionOrder,
 //   kKeyCodecCtxMaxPartitionOrder,
 //   kKeyCodecCtxTimecodeFrameStart,
 //   kKeyCodecCtxRequestChannels,
 //   kKeyCodecCtxDrcScale,
 //   kKeyCodecCtxReorderedOpaque,
   kKeyCodecCtxBitsPerRawSample = 'fBPR',
    kKeyCodecCtxChannelLayout = 'fChL',
 //   kKeyCodecCtxRequestChannelLayout,
 //   kKeyCodecCtxRcMaxAvailableVbvUse,
 //   kKeyCodecCtxRcMinVbvOverflowUse,
 //   kKeyCodecCtxHwaccel,
    kKeyCodecCtxTicksPerFrame = 'fTPF',
 //   kKeyCodecCtxHwAccelContext,
 //   kKeyCodecCtxColorPrimariex,
 //   kKeyCodecCtxColorTrc,
    kKeyCodecCtxColorspace = 'fCSp',
 //   kKeyCodecCtxColorRange,
 //   kKeyCodecCtxChromaSampleLocation,
//    kKeyCodecCtxExecute2Func,
//    kKeyCodecCtxWeightedPPred,
//    kKeyCodecCtxAqMode,
//    kKeyCodecCtxAqStrength,
//    kKeyCodecCtxPsyRd,
//    kKeyCodecCtxPsyTrellis,
//    kKeyCodecCtxRcLookahead,
//    kKeyCodecCtxCrfMax,
//    kKeyCodecCtxLogLevelOffset,
//    kKeyCodecCtxLpcType,
//    kKeyCodecCtxLpcPasses,
//    kKeyCodecCtxSlices,
//    kKeyCodecCtxSubtitleHeader,
//    kKeyCodecCtxSubtitleHeaderSize,
//    kKeyCodecCtxPkt,
//    kKeyCodecCtxIsCopy,
//    kKeyCodecCtxThreadType,
//    kKeyCodecCtxActiveThreadType,
//    kKeyCodecCtxThreadSafeCallbacks,
//    kKeyCodecCtxVbvDelay,
    kKeyCodecCtxAudioServiceType = 'fAST',
  //  kKeyCodecCtxRequestSampleFmt,
 //   kKeyCodecCtxErrRecognition,
 //   kKeyCodecCtxInternal,
 //   kKeyCodecCtxFieldOrder,
//    kKeyCodecCtxPtsCorrectionNumFaultyPts,
//    kKeyCodecCtxPtsCorrectionNumFaultyDts,
//    kKeyCodecCtxPtsCorrectionLastPts,
//    kKeyCodecCtxPtsCorrectionLastDts
    /*a@nufront end*/
};

enum {
    kTypeESDS        = 'esds',
    kTypeAVCC        = 'avcc',
    kTypeD263        = 'd263',
    /* a@nufront begin */
    kTypeMPEG2       = 'mpg2',
    /* a@nufront end */
};

enum {
    FLIP_H           = 1, /* flip source image horizontally */
    FLIP_V           = 2, /* flip source image vertically */
};

class MetaData : public RefBase {
public:
    MetaData();
    MetaData(const MetaData &from);

    enum Type {
        TYPE_NONE     = 'none',
        TYPE_C_STRING = 'cstr',
        TYPE_INT32    = 'in32',
        TYPE_INT64    = 'in64',
        TYPE_FLOAT    = 'floa',
        TYPE_POINTER  = 'ptr ',
        TYPE_RECT     = 'rect',
    };

    void clear();
    bool remove(uint32_t key);

    bool setCString(uint32_t key, const char *value);
    bool setInt32(uint32_t key, int32_t value);
    bool setInt64(uint32_t key, int64_t value);
    bool setFloat(uint32_t key, float value);
    bool setPointer(uint32_t key, void *value);

    bool setRect(
            uint32_t key,
            int32_t left, int32_t top,
            int32_t right, int32_t bottom);

    bool findCString(uint32_t key, const char **value);
    bool findInt32(uint32_t key, int32_t *value);
    bool findInt64(uint32_t key, int64_t *value);
    bool findFloat(uint32_t key, float *value);
    bool findPointer(uint32_t key, void **value);

    bool findRect(
            uint32_t key,
            int32_t *left, int32_t *top,
            int32_t *right, int32_t *bottom);

    bool setData(uint32_t key, uint32_t type, const void *data, size_t size);

    bool findData(uint32_t key, uint32_t *type,
                  const void **data, size_t *size) const;

protected:
    virtual ~MetaData();

private:
    struct typed_data {
        typed_data();
        ~typed_data();

        typed_data(const MetaData::typed_data &);
        typed_data &operator=(const MetaData::typed_data &);

        void clear();
        void setData(uint32_t type, const void *data, size_t size);
        void getData(uint32_t *type, const void **data, size_t *size) const;

    private:
        uint32_t mType;
        size_t mSize;

        union {
            void *ext_data;
            float reservoir;
        } u;

        bool usesReservoir() const {
            return mSize <= sizeof(u.reservoir);
        }

        void allocateStorage(size_t size);
        void freeStorage();

        void *storage() {
            return usesReservoir() ? &u.reservoir : u.ext_data;
        }

        const void *storage() const {
            return usesReservoir() ? &u.reservoir : u.ext_data;
        }
    };

    struct Rect {
        int32_t mLeft, mTop, mRight, mBottom;
    };

    KeyedVector<uint32_t, typed_data> mItems;

    // MetaData &operator=(const MetaData &);
};

}  // namespace android

#endif  // META_DATA_H_
