/*
/**
 *Copyright (C), 2009~2019, NUFRONT. Co., Ltd.
 *File name:    OMX_NuAudioDec_Utils.h
 *Author: Zhai Jianfeng Version: v1.0       Date: 2010-09-25
 *
 * This is an header file for an audio NUAUDIO decoder that is fully
 * compliant with the OMX Audio specification.
 * This the file is used internally by the component
 * in its code.
 *
 * @path  $(CSLPATH)\OMAPSW_MPU\linux\audio\src\openmax_il\nuaudio_dec\inc\
 *
 * @rev 1.0
 */
/* --------------------------------------------------------------------------- */
#ifndef OMX_NUAUDIODEC_UTILS__H
#define OMX_NUAUDIODEC_UTILS__H

#include <OMX_Component.h>
#include <OMX_NUFRONT_Common.h>
#include <OMX_NUFRONT_Debug.h>
#include <pthread.h>
#include <sched.h>
#include "OMX_BufQ.h"

#define AUDIO_MANAGER

#ifndef ANDROID
    #define ANDROID
#endif

#ifdef ANDROID
    /* Log for Android system*/
    #undef LOG_TAG
    #define LOG_TAG "OMX_NUAUDIODEC"

    /* PV opencore capability custom parameter index */
    #define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif

/*for ffmpeg porting*/
#include <utils/Log.h>
#include <utils/nufrontlog.h>
#include<internal.h>
#include "ac3.h"
#include "aac.h"
#include "avcodec.h"
#include "avformat.h"
#if ARCH_ARM
#include "arm/aac.h"
#endif
#include "libavcodec/audioconvert.h"
/*end for ffmpeg porting*/

#define OBJECTTYPE_LC 2
#define OBJECTTYPE_LTP 4
#define OBJECTTYPE_HE 5
#define OBJECTTYPE_HE2 29

#define EXIT_COMPONENT_THRD  10
#define AUDIO_DBG 0
#if AUDIO_DBG
#define LOG_AUDIO(dbg...) LOGD(dbg)
#else
#define LOG_AUDIO(dbg...)
#endif
/* ======================================================================= */
/**
 * @def    NUAUDIO_DEC__XXX_VER    Component version
 */
/* ======================================================================= */
#define NUAUDIODEC_MAJOR_VER 1
#define NUAUDIODEC_MINOR_VER 1
/* ======================================================================= */
/**
 * @def    NOT_USED_NUAUDIODEC    Defines a value for "don't care" parameters
 */
/* ======================================================================= */
#define NU_BIT_PER_SAMPLE 16
/* ======================================================================= */
/**
 * @def    NORMAL_BUFFER_NUAUDIODEC    Defines the flag value with all flags turned off
 */
/* ======================================================================= */
#define NORMAL_BUFFER_NUAUDIODEC 0
/* ======================================================================= */
/**
 * @def    OMX_NUAUDIODEC_DEFAULT_SEGMENT    Default segment ID for the LCML
 */
/* ======================================================================= */
#define OMX_NUAUDIODEC_DEFAULT_SEGMENT (0)
/* ======================================================================= */
/**
 * @def    OMX_NUAUDIODEC_SN_TIMEOUT    Timeout value for the socket node
 */
/* ======================================================================= */
#define OMX_NUAUDIODEC_SN_TIMEOUT (-1)
/* ======================================================================= */
/**
 * @def    OMX_NUAUDIODEC_SN_PRIORITY   Priority for the socket node
 */
/* ======================================================================= */
#define OMX_NUAUDIODEC_SN_PRIORITY (10)
/* ======================================================================= */
/**
 * @def    OMX_NUAUDIODEC_NUM_DLLS   number of DLL's
 */
/* ======================================================================= */
#define OMX_NUAUDIODEC_NUM_DLLS (2)

#define NUAUDIODEC_BUFHEADER_VERSION 0x1
/* ======================================================================= */
/**
 ** Default timeout used to come out of blocking calls*
 *
 */
/* ======================================================================= */
#define NUAUDIOD_TIMEOUT (1000) /* millisecs */

#define DONT_CARE 0

/* ======================================================================= */
/**
 * @def    NUAUDIODEC_SBR_CONTENT  flag detection
 */
/* ======================================================================= */

#define NUAUDIODEC_SBR_CONTENT 0x601


/* ======================================================================= */
/**
 * @def    NUAUDIODEC_PS_CONTENT flag  detection
 */
/* ======================================================================= */

#define  NUAUDIODEC_PS_CONTENT 0x602


/* ======================================================================= */
/**
 * @def    NUAUDIODEC_DEBUG   Debug print macro
 */
/* ======================================================================= */

#undef NUAUDIODEC_DEBUG
#define _ERROR_PROPAGATION__


#ifdef  NUAUDIODEC_DEBUG
    #define NUAUDIODEC_DPRINT printf
    #undef NUAUDIODEC_BUFPRINT printf
    #undef NUAUDIODEC_MEMPRINT printf
    #define NUAUDIODEC_STATEPRINT printf
#else
    #define NUAUDIODEC_DPRINT(...)
#endif

#ifdef NUAUDIODEC_STATEDETAILS
    #define NUAUDIODEC_STATEPRINT printf
#else
    #define NUAUDIODEC_STATEPRINT(...)
#endif

#ifdef NUAUDIODEC_BUFDETAILS
    #define NUAUDIODEC_BUFPRINT printf
#else
    #define NUAUDIODEC_BUFPRINT(...)
#endif

#ifdef NUAUDIODEC_MEMDETAILS
    #define NUAUDIODEC_MEMPRINT(...)  fprintf(stdout, "%s %d::  ",__FUNCTION__, __LINE__); \
                                  fprintf(stdout, __VA_ARGS__); \
                                  fprintf(stdout, "\n");
#else
    #define NUAUDIODEC_MEMPRINT(...)
#endif

#define NUAUDIODEC_EPRINT LOGE


/* ======================================================================= */
/**
 * @def    NUAUDIODEC_OMX_ERROR_EXIT   Exit print and return macro
 */
/* ======================================================================= */
#define NUAUDIODEC_OMX_ERROR_EXIT(_e_, _c_, _s_)                            \
    _e_ = _c_;                                                          \
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n**************** OMX ERROR ************************\n");  \
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d : Error Name: %s : Error Num = %x",__LINE__, _s_, _e_);  \
    OMXDBG_PRINT(stderr, ERROR, 4, 0, "\n**************** OMX ERROR ************************\n");  \
    goto EXIT;

/* ======================================================================= */
/**
 * @def    NUAUDIODEC_OMX_CONF_CHECK_CMD   Command check Macro
 */
/* ======================================================================= */
#define NUAUDIODEC_OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3)  \
    {                                                   \
        if(!_ptr1 || !_ptr2 || !_ptr3){                 \
            eError = OMX_ErrorBadParameter;             \
            goto EXIT;                                  \
        }                                               \
    }

/* ======================================================================= */
/**
 * @def    OMX_CONF_INIT_STRUCT   Macro to Initialise the structure variables
 */
/* ======================================================================= */
#define OMX_CONF_INIT_STRUCT(_s_, _name_)       \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 1;      \
    (_s_)->nVersion.s.nVersionMinor = 1;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0

/* ======================================================================= */
/**
 * @def    NUAUDIODEC_BUFDETAILS   Turns buffer messaging on and off
 */
/* ======================================================================= */
#undef NUAUDIODEC_BUFDETAILS
/* ======================================================================= */
/**
 * @def    NUAUDIODEC_STATEDETAILS   Turns state messaging on and off
 */
/* ======================================================================= */
#undef NUAUDIODEC_STATEDETAILS
/* ======================================================================= */
/**
 * @def    NUAUDIODEC_MEMDETAILS   Turns memory messaging on and off
 */
/* ======================================================================= */
#undef NUAUDIODEC_MEMDETAILS

#define NUAUDIODEC_OUTPUT_PORT 1
#define NUAUDIODEC_INPUT_PORT 0
#define NUAUDIODEC_APP_ID  100
#define MAX_NUM_OF_BUFS_NUAUDIODEC 15
#define PARAMETRIC_STEREO_NUAUDIODEC 1
#define NON_PARAMETRIC_STEREO_NUAUDIODEC 0
/* ======================================================================= */
/**
 * @def    NUM_OF_PORTS_NUAUDIODEC   Number of ports
 */
/* ======================================================================= */
#define NUM_OF_PORTS_NUAUDIODEC 2
/* ======================================================================= */
/**
 * @def    STREAM_COUNT_NUAUDIODEC   Number of streams
 */
/* ======================================================================= */
#define STREAM_COUNT_NUAUDIODEC 2

/** Default timeout used to come out of blocking calls*/

/* ======================================================================= */
/**
 * @def    NUAUDIOD_NUM_INPUT_BUFFERS   Default number of input buffers
 *
 */
/* ======================================================================= */
#define NUAUDIOD_NUM_INPUT_BUFFERS 4
/* ======================================================================= */
/**
 * @def    NUAUDIOD_NUM_OUTPUT_BUFFERS   Default number of output buffers
 *
 */
/* ======================================================================= */
#define NUAUDIOD_NUM_OUTPUT_BUFFERS 4

/* ======================================================================= */
/**
 * @def    NUAUDIOD_INPUT_BUFFER_SIZE   Default input buffer size
 *
 */
/* ======================================================================= */
//#define NUAUDIOD_INPUT_BUFFER_SIZE 1536*4
#define NUAUDIOD_INPUT_BUFFER_SIZE 409600 * 3
/* ======================================================================= */
/**
 * @def    NUAUDIOD_OUTPUT_BUFFER_SIZE   Default output buffer size
 *
 */
/* ======================================================================= */
#define NUAUDIOD_OUTPUT_BUFFER_SIZE 200000 /*this value is the max output len of testing file*/
#define NUAUDIO_STERO               2
#define NUAUDIO_MONO                1
/* ======================================================================= */
/**
 * @def    NUAUDIOD_SAMPLING_FREQUENCY   Sampling frequency
 */
/* ======================================================================= */
#define NUAUDIOD_SAMPLING_FREQUENCY 44100
/* ======================================================================= */
/**
 * @def FFMPEG_MAX_OUTPUT_LEN max buflen of output buf for ffmpeg
 */
/* ======================================================================= */
#define FFMPEG_MAX_OUTPUT_LEN 192000
/* ======================================================================= */
/**
 * @def MAX_WORD_VALUE
 */
/* ======================================================================= */
#define  MAX_WORD_VALUE 32767
/* ======================================================================= */
/**
 * @def MIN_WORD_VALUE
 */
/* ======================================================================= */
#define  MIN_WORD_VALUE -32768
/* ======================================================================= */
/**
 * @def    NUAUDIODec macros for MONO,STEREO_INTERLEAVED,STEREO_NONINTERLEAVED
 */
/* ======================================================================= */
/*#define NUAUDIOD_STEREO_INTERLEAVED_STREAM     2
  #define NUAUDIOD_STEREO_NONINTERLEAVED_STREAM  3*/
/* ======================================================================= */
/**
 * @def    NUAUDIODec macros for MONO,STEREO_INTERLEAVED,STEREO_NONINTERLEAVED
 */
/* ======================================================================= */
/* Stream types supported*/
#define MONO_STREAM_NUAUDIODEC                   1
#define STEREO_INTERLEAVED_STREAM_NUAUDIODEC     2
#define STEREO_NONINTERLEAVED_STREAM_NUAUDIODEC  3

#ifdef  OMX_MALLOC_SIZE(_ptr_, _size_,_name_)
#undef   OMX_MALLOC_SIZE(_ptr_, _size_,_name_)

#define OMX_MALLOC_SIZE(_ptr_, _size_,_name_)            \
    _ptr_ = (_name_*)av_malloc(_size_);                         \
    if(_ptr_ == NULL){                                          \
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "***********************************\n");        \
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d :: Malloc Failed\n",__LINE__);               \
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "***********************************\n");        \
        eError = OMX_ErrorInsufficientResources;                \
        goto EXIT;                                              \
    }                                                           \
    memset(_ptr_,0,_size_);                                     \
    OMXDBG_PRINT(stderr, BUFFER, 2, OMX_DBG_BASEMASK, "%d :: Malloced = %p\n",__LINE__,_ptr_);
#endif

#ifdef  OMX_MEMFREE_STRUCT(_pStruct_)
#undef   OMX_MEMFREE_STRUCT(_pStruct_)
#define OMX_MEMFREE_STRUCT(_pStruct_)\
    OMXDBG_PRINT(stderr, BUFFER, 2, OMX_DBG_BASEMASK, "%d :: [FREE] %p\n",__LINE__,_pStruct_); \
    if(_pStruct_ != NULL){\
        av_free(_pStruct_);\
        _pStruct_ = NULL;\
    }
#endif

#ifdef OMX_MMMIXER_DATAPATH(_datapath_,_rendertype_,_streamid_)
#undef  OMX_MMMIXER_DATAPATH(_datapath_,_rendertype_,_streamid_)
#define OMX_MMMIXER_DATAPATH(_datapath_,_rendertype_,_streamid_)\
        {\
        static char str[50];\
        int OMX_MMMIXER_DATAPATH_instreamid = ((_streamid_ >> 4) & 0xF) - 1;\
        int OMX_MMMIXER_DATAPATH_outstreamid = (_streamid_ & 0xF) - 1;\
        OMX_MMMIXER_ITOA(str, OMX_MMMIXER_DATAPATH_instreamid)\
        strcpy((char*)_datapath_,(char*)":i");\
        av_strlcat((char*)_datapath_,(char*)str);\
        av_strlcat((char*)_datapath_,(char*)":o");\
        OMX_MMMIXER_ITOA(str, OMX_MMMIXER_DATAPATH_outstreamid)\
        av_strlcat((char*)_datapath_,(char*)str);\
        av_strlcat((char*)_datapath_,(char*)"/codec\0");\
        }
#endif
/* ======================================================================= */
/**
 * pthread variable to indicate OMX returned all buffers to app
 */
/* ======================================================================= */
pthread_mutex_t bufferReturned_mutex;
pthread_cond_t bufferReturned_condition;

/**
 *
 * NUAUDIO Decoder Profile:0 - MAIN, 1 - LC, 2 - SSR, 3 - LTP.
 */
typedef enum {
    EProfileMain,
    EProfileLC,
    EProfileSSR,
    EProfileLTP
}NUAUDIOProfile;
/* ======================================================================= */
/** COMP_PORT_TYPE_NUAUDIODEC  Port types
 *
 *  @param  INPUT_PORT_NUAUDIODEC                    Input port
 *
 *  @param  OUTPUT_PORT_NUAUDIODEC               Output port
 */
/*  ==================================================================== */
/*This enum must not be changed. */
typedef enum COMP_PORT_TYPE_NUAUDIODEC {
    INPUT_PORT_NUAUDIODEC = 0,
    OUTPUT_PORT_NUAUDIODEC
}COMP_PORT_TYPE_NUAUDIODEC;
/* ======================================================================= */
/** OMX_INDEXAUDIOTYPE_NUAUDIODEC  Defines the custom configuration settings
 *                              for the component
 *
 *  @param  OMX_IndexCustomMode16_24bit_NUAUDIODEC  Sets the 16/24 mode
 *
 *  @param  OMX_IndexCustomModeProfile_NUAUDIODEC  Sets the Profile mode
 *
 *  @param  OMX_IndexCustomModeSBR_NUAUDIODEC  Sets the SBR mode
 *
 *  @param  OMX_IndexCustomModeDasfConfig_NUAUDIODEC  Sets the DASF mode
 *
 *  @param  OMX_IndexCustomModeRAW_NUAUDIODEC  Sets the RAW mode
 *
 *  @param  OMX_IndexCustomModePS_NUAUDIODEC  Sets the ParametricStereo mode
 *
 */
/*  ==================================================================== */
typedef enum OMX_INDEXAUDIOTYPE_NUAUDIODEC {
    OMX_IndexCustomNuAudioDecHeaderInfoConfig = 0xFF000001,
    OMX_IndexCustomNuAudioDecStreamIDConfig,
    OMX_IndexCustomNuAudioDecDataPath,
    OMX_IndexCustomDebug
}OMX_INDEXAUDIOTYPE_NUAUDIODEC;


/* Component Port Context */
typedef struct AUDIODEC_PORT_TYPE {
    /* Used in tunneling, this is handle of tunneled component */
    OMX_HANDLETYPE hTunnelComponent;
    /* Port which has to be tunneled */
    OMX_U32 nTunnelPort;
    /* Buffer Supplier Information */
    OMX_BUFFERSUPPLIERTYPE eSupplierSetting;
    /* Number of buffers */
    OMX_U8 nBufferCnt;
    /* Port format information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pPortFormat;
} AUDIODEC_PORT_TYPE;


/* ======================================================================= */
/** NUAUDIO_DEC_BUFFERLIST: This contains information about a buffer's owner whether
 * it is application or component, number of buffers owned etc.
 *
 * @see OMX_BUFFERHEADERTYPE
 */
/* ==================================================================== */
struct NUAUDIO_DEC_BUFFERLIST{
    /* Array of pointer to OMX buffer headers */
    OMX_BUFFERHEADERTYPE *pBufHdr[MAX_NUM_OF_BUFS_NUAUDIODEC];
    /* Array that tells about owner of each buffer */
    OMX_U32 bufferOwner[MAX_NUM_OF_BUFS_NUAUDIODEC];
    /* Tracks pending buffers */
    OMX_U32 bBufferPending[MAX_NUM_OF_BUFS_NUAUDIODEC];
    /* Number of buffers  */
    OMX_U32 numBuffers;
};

typedef struct NUAUDIO_DEC_BUFFERLIST NUAUDIODEC_BUFFERLIST;

typedef struct PV_OMXComponentCapabilityFlagsType
{
        ////////////////// OMX COMPONENT CAPABILITY RELATED MEMBERS (for opencore compatability)
        OMX_BOOL iIsOMXComponentMultiThreaded;
        OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
        OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
        OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
        OMX_BOOL iOMXComponentSupportsPartialFrames;
        OMX_BOOL iOMXComponentNeedsNALStartCode;
        OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
} PV_OMXComponentCapabilityFlagsType;

/* ======================================================================= */
/** NUAUDIODEC_COMPONENT_PRIVATE: This is the major and main structure of the
 * component which contains all type of information of buffers, ports etc
 * contained in the component.
 *
 * @see OMX_BUFFERHEADERTYPE
 * @see OMX_AUDIO_PARAM_PORTFORMATTYPE
 * @see OMX_PARAM_PORTDEFINITIONTYPE
 * @see NUAUDIOD_LCML_BUFHEADERTYPE
 * @see OMX_PORT_PARAM_TYPE
 * @see OMX_PRIORITYMGMTTYPE
 * @see AUDIODEC_PORT_TYPE
 * @see NUAUDIODEC_BUFFERLIST
 * @see LCML_STRMATTR
 * @see
 */
/* ==================================================================== */

typedef struct NUAUDIODEC_COMPONENT_PRIVATE
{

    OMX_CALLBACKTYPE cbInfo;
    /** Handle for use with async callbacks */
    OMX_PORT_PARAM_TYPE* sPortParam;
    /* Input port information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sInPortFormat;
    /* Output port information */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sOutPortFormat;
    /* Buffer owner information */
    OMX_U32 bIsBufferOwned[NUM_OF_PORTS_NUAUDIODEC];

    /** This will contain info like how many buffers
        are there for input/output ports, their size etc, but not
        BUFFERHEADERTYPE POINTERS. */
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[NUM_OF_PORTS_NUAUDIODEC];
    /* Contains information that come from application */
    OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE* nuaudioParams;

    OMX_AUDIO_PARAM_PCMMODETYPE* pcmParams;

    /** This is component handle */
    OMX_COMPONENTTYPE* pHandle;

    /** Current state of this component */
    OMX_STATETYPE curState;

    /** The component thread handle */
    pthread_t ComponentThread;

    /** The pipes for sending buffers to the thread */
    int dataPipe[2];

    /** The pipes for sending buffers to the thread */
    int cmdPipe[2];

    /** The pipes for sending command data to the thread */
    int cmdDataPipe[2];

    /** Set to indicate component is stopping */
    OMX_U32 bIsEOFSent;

    /** Counts of number of buffers received from App  */
    OMX_U32 app_nBuf;

    OMX_U32 num_Reclaimed_Op_Buff;
    /** Counts of number of input buffers sent to lcml  */
    OMX_U32 num_Sent_Ip_Buff;
    /** Counts of number of output buffers sent to lcml  */
    OMX_U32 num_Op_Issued;
    /** Holds the value of dasf mode, 1: DASF mode or 0: File Mode  */
    OMX_U32 dasfmode;


    /** ID stream ID**/
    OMX_U32 streamID;

    /** Tells whether buffers on ports have been allocated */
    OMX_U32 bPortDefsAllocated;
    /** Tells whether component thread has started */
    OMX_U32 bCompThreadStarted;
    /** Marks the buffer data  */
    OMX_PTR pMarkData;
    /** Marks the buffer */
    OMX_MARKTYPE *pMarkBuf;
    /** Marks the target component */
    OMX_HANDLETYPE hMarkTargetComponent;
    /** Input port enable flag */
    OMX_U32 ipPortEnableFlag;
    /** Input port disble flag */
    OMX_U32 ipPortDisableFlag;
    /** Pointer to port parameter structure */
    OMX_PORT_PARAM_TYPE* pPortParamType;
    /** Pointer to port priority management structure */
    OMX_PRIORITYMGMTTYPE* pPriorityMgmt;


    OMX_BOOL bPreempted;


    /** Contains the port related info of both the ports */
    AUDIODEC_PORT_TYPE *pCompPort[NUM_OF_PORTS_NUAUDIODEC];
    /* Checks whether or not buffer were allocated by appliction */
    OMX_U32 bufAlloced;
    /** Flag to check about execution of component thread */
    OMX_U16 bExitCompThrd;
    /** Pointer to list of input buffers */
    NUAUDIODEC_BUFFERLIST *pInputBufferList;
    /** Pointer to list of output buffers */
    NUAUDIODEC_BUFFERLIST *pOutputBufferList;
    /** it is used for component's create phase arguments */
    //LCML_STRMATTR  *strmAttr;
    /** Contains the version information */
    OMX_U32 nVersion;

    /** Number of input buffers at runtime */
    OMX_U32 nRuntimeInputBuffers;

    /** Number of output buffers at runtime */
    OMX_U32 nRuntimeOutputBuffers;


    OMX_U16 framemode;

    OMX_STRING cComponentName;

    OMX_VERSIONTYPE ComponentVersion;

    OMX_U32 nOpBit;
    OMX_U32 parameteric_stereo;
    OMX_U32 dualMonoMode;
    OMX_U32 SBR;
    OMX_U32 RAW;
    OMX_U32 nFillThisBufferCount;
    OMX_U32 nFillBufferDoneCount;
    OMX_U32 nEmptyThisBufferCount;
    OMX_U32 nEmptyBufferDoneCount;
    OMX_U32 bInitParamsInitialized;
    NUAUDIODEC_BUFFERLIST *pInputBufferListQueue;
    NUAUDIODEC_BUFFERLIST *pOutputBufferListQueue;
    /** To store input buffers recieved while in paused state **/
    OMX_BUFFERHEADERTYPE *pInputBufHdrPending[MAX_NUM_OF_BUFS_NUAUDIODEC];
    OMX_U32 nNumInputBufPending;

    /** To store out buffers received while in puased state **/
    OMX_BUFFERHEADERTYPE *pOutputBufHdrPending[MAX_NUM_OF_BUFS_NUAUDIODEC];
    OMX_U32 nNumOutputBufPending;

    /** Flags to control port disable command **/
    OMX_U32 bDisableCommandPending;
    OMX_U32 bDisableCommandParam;
    /** Flags to control port enable command **/
    OMX_U32 bEnableCommandPending;
    OMX_U32 bEnableCommandParam;

    OMX_U32 nInvalidFrameCount;
    OMX_U32 numPendingBuffers;
    OMX_U32 bNoIdleOnStop;
    /* bIdleCommandPending;*/
    OMX_S32 nOutStandingFillDones;
    OMX_BOOL bIsInvalidState;
    OMX_STRING* sDeviceString;
    pthread_mutex_t AlloBuf_mutex;
    pthread_cond_t AlloBuf_threshold;
    OMX_U8 AlloBuf_waitingsignal;

    pthread_mutex_t InLoaded_mutex;
    pthread_cond_t InLoaded_threshold;
    OMX_U8 InLoaded_readytoidle;

    pthread_mutex_t InIdle_mutex;
    pthread_cond_t InIdle_threshold;
    OMX_U8 InIdle_goingtoloaded;

    pthread_mutex_t codecStop_mutex;
    pthread_cond_t codecStop_threshold;
    OMX_U8 codecStop_waitingsignal;

    pthread_mutex_t codecFlush_mutex;
    pthread_cond_t codecFlush_threshold;
    OMX_U8 codecFlush_waitingsignal;

    /*a@nufront start: add to info compthread*/
    pthread_mutex_t CodecInit_mutex;
    pthread_cond_t CodecInit_threshold;
    /*a@nufront end*/

    OMX_U32 nUnhandledFillThisBuffers;
    OMX_U32 nHandledFillThisBuffers;
    OMX_U32 nUnhandledEmptyThisBuffers;
    OMX_U32 nHandledEmptyThisBuffers;
    OMX_BOOL bFlushOutputPortCommandPending;
    OMX_BOOL bFlushInputPortCommandPending;

    OMX_BOOL bLoadedCommandPending;
    OMX_PARAM_COMPONENTROLETYPE *componentRole;

    OMX_U8 PendingInPausedBufs;
    OMX_BUFFERHEADERTYPE *pInBufHdrPausedPending[MAX_NUM_OF_BUFS_NUAUDIODEC];
    OMX_U8 PendingOutPausedBufs;
    OMX_BUFFERHEADERTYPE *pOutBufHdrPausedPending[MAX_NUM_OF_BUFS_NUAUDIODEC];

    /** Keep buffer timestamps **/
    OMX_S64 arrBufIndex[MAX_NUM_OF_BUFS_NUAUDIODEC];
    /**Keep buffer tickcount*/
    OMX_U32 arrBufIndexTick[MAX_NUM_OF_BUFS_NUAUDIODEC];

    /** Index to arrBufIndex[] and arrBufIndexTick[], used for input buffer timestamps */
    OMX_U8 IpBufindex;
    /** Index to arrBufIndex[] and arrBufIndexTick[], used for input buffer timestamps  */
    OMX_U8 OpBufindex;

    /** Flag to flush SN after EOS in order to process more buffers after EOS**/
    OMX_U8 SendAfterEOS;

    /** Flag to mark the first sent buffer**/
    OMX_U8 first_buff;
    /** First Time Stamp sent **/
    OMX_TICKS first_TS;
    /** Temporal time stamp **/
    OMX_TICKS temp_TS;

    PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;
    OMX_BOOL bConfigData;
    OMX_BOOL reconfigInputPort;
    OMX_BOOL reconfigOutputPort;
    OMX_U8 OutPendingPR;

    struct OMX_NUFRONT_Debug dbg;

    /** Indicate when first output buffer received from DSP **/
    OMX_U32 first_output_buf_rcv;
    /*a@nufront start:*/
    struct AVCodecContext  *spCodecCtx;
    ReSampleContext * spResample;
    OMX_U16 * pOutputBuf;
    OMX_S32 audioStream;
    OMX_BUF_Q_TYPE* spInputBufQ;
    OMX_BUF_Q_TYPE* spOutputBufQ;
    OMX_U32 nSampleSize;
    OMX_U64 nLastDTS;
    OMX_U64 nAnchor;
    OMX_U64 mOutputLen;
    /*a@nufront end:*/

} NUAUDIODEC_COMPONENT_PRIVATE;

/* ================================================================================= * */
/**
 * OMX_ComponentInit() function is called by OMX Core to initialize the component
 * with default values of the component. Before calling this function OMX_Init
 * must have been called.
 *
 * @param *hComp This is component handle allocated by the OMX core.
 *
 * @pre          OMX_Init should be called by application.
 *
 * @post         Component has initialzed with default values.
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see          NuAudioDec_StartCompThread()
 */
/* ================================================================================ * */
#ifndef UNDER_CE
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#else
/*  WinCE Implicit Export Syntax */
#define OMX_EXPORT __declspec(dllexport)
/* ===========================================================  */
/**
 *  OMX_ComponentInit()  Initializes component
 *
 *
 *  @param hComp            OMX Handle
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 *
 */
/*================================================================== */
OMX_EXPORT OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp);
#endif

/* ================================================================================= * */
/**
 * NuAudioDec_StartCompThread() starts the component thread. This is internal
 * function of the component.
 *
 * @param pHandle This is component handle allocated by the OMX core.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE NuAudioDec_StartCompThread(OMX_HANDLETYPE pHandle);
/* ================================================================================= * */
/**
 * NUAUDIODEC_Fill_LCMLInitParams() fills the LCML initialization structure.
 *
 * @param pHandle This is component handle allocated by the OMX core.
 *
 * @param plcml_Init This structure is filled and sent to LCML.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the LCML struct.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
//OMX_ERRORTYPE NUAUDIODEC_Fill_LCMLInitParams(OMX_HANDLETYPE pHandle, LCML_DSP *plcml_Init, OMX_U16 arr[]);
/* ================================================================================= * */
/**
 * NUAUDIODEC_GetBufferDirection() function determines whether it is input buffer or
 * output buffer.
 *
 * @param *pBufHeader This is pointer to buffer header whose direction needs to
 *                    be determined.
 *
 * @param *eDir  This is output argument which stores the direction of buffer.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorBadParameter = In case of invalid buffer
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE NUAUDIODEC_GetBufferDirection(OMX_BUFFERHEADERTYPE *pBufHeader,
                                        OMX_DIRTYPE *eDir);
/* ================================================================================= * */
/**
 * NUAUDIODEC_LCML_Callback() function is callback which is called by LCML whenever
 * there is an even generated for the component.
 *
 * @param event  This is event that was generated.
 *
 * @param arg    This has other needed arguments supplied by LCML like handles
 *               etc.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *
 *  @see         None
 */
/* ================================================================================ * */
//OMX_ERRORTYPE NUAUDIODEC_LCML_Callback (TUsnCodecEvent event,void * args [10]);
/* ================================================================================= * */
/**
 * NUAUDIODEC_HandleCommand() function handles the command sent by the application.
 * All the state transitions, except from nothing to loaded state, of the
 * component are done by this function.
 *
 * @param pComponentPrivate  This is component's private date structure.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *               OMX_ErrorHardware = Hardware error has occured lile LCML failed
 *               to do any said operartion.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_U32 NUAUDIODEC_HandleCommand (NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ================================================================================= * */
/**
 * NUAUDIODEC_HandleDataBuf_FromApp() function handles the input and output buffers
 * that come from the application. It is not direct function wich gets called by
 * the application rather, it gets called eventually.
 *
 * @param *pBufHeader This is the buffer header that needs to be processed.
 *
 * @param *pComponentPrivate  This is component's private date structure.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *               OMX_ErrorHardware = Hardware error has occured lile LCML failed
 *               to do any said operartion.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE NUAUDIODEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE *pBufHeader,
                                           NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate);
/* ================================================================================= * */
/**
 * NUAUDIODEC_FreeCompResources() function frees the component resources.
 *
 * @param *pBufHeader This is the buffer header that needs to be processed.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful processing.
 *               OMX_ErrorInsufficientResources = Not enough memory
 *               OMX_ErrorHardware = Hardware error has occured lile LCML failed
 *               to do any said operartion.
 *
 *  @see         None
 */
/* ================================================================================ * */
OMX_ERRORTYPE NUAUDIODEC_FreeCompResources(OMX_HANDLETYPE pComponent);
/* ================================================================================= * */
/**
 * NUAUDIODEC_CleanupInitParams() function frees only the initialization time
 * memories allocated. For example, it will not close pipes, it will not free the
 * memory allocated to the buffers etc. But it does free the memory of buffers
 * utilized by the LCML etc. It is basically subset of NUAUDIODEC_FreeCompResources()
 * function.
 *
 * @param pComponent This is the component handle.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *
 *  @see         None
 */
/* ================================================================================ * */
void NUAUDIODEC_CleanupInitParams(OMX_HANDLETYPE pComponent);
/* ================================================================================= * */
/**
 * NUAUDIODEC_CleanupInitParamsEx() function frees only the initialization time
 * memories allocated. For example, it will not close pipes, it will not free the
 * memory allocated to the buffers etc. But it does free the memory of buffers
 * utilized by the LCML etc. It is basically subset of NUAUDIODEC_FreeCompResources()
 * function. Called while port disable when port reconfiguration takes place.
 *
 * @param pComponent This is the component handle.
 *
 * @pre          None
 *
 * @post         None
 *
 *  @return      OMX_ErrorNone = Successful Inirialization of the component\n
 *
 *  @see         None
 */
/* ================================================================================ * */
void NUAUDIODEC_CleanupInitParamsEx(OMX_HANDLETYPE pComponent,OMX_U32 indexport);
/* ===========================================================  */
/**
 *  NUAUDIODEC_SetPending()  Called when the component queues a buffer
 * to the LCML
 *
 *  @param pComponentPrivate        Component private data
 *
 *  @param pBufHdr                Buffer header
 *
 *  @param eDir                    Direction of the buffer
 *
 *  @return None
 */
/*================================================================== */
void NUAUDIODEC_SetPending(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber);
/* ===========================================================  */
/**
 *  NUAUDIODEC_ClearPending()  Called when a buffer is returned
 * from the LCML
 *
 *  @param pComponentPrivate        Component private data
 *
 *  @param pBufHdr                Buffer header
 *
 *  @param eDir                    Direction of the buffer
 *
 *  @return None
 */
/*================================================================== */
void NUAUDIODEC_ClearPending(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber) ;
/* ===========================================================  */
/**
 *  NUAUDIODEC_IsPending()
 *
 *
 *  @param pComponentPrivate        Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_U32 NUAUDIODEC_IsPending(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir);
/* ===========================================================  */
/**
 *  NUAUDIODEC_IsValid()
 *
 *
 *  @param pComponentPrivate        Component private data
 *
 *  @return OMX_ErrorNone = Successful
 *          Other error code = fail
 */
/*================================================================== */
OMX_U32 NUAUDIODEC_IsValid(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir) ;

/*=======================================================================*/
/*! @fn NUAUDIODec_GetSampleRateIndexL

 * @brief Gets the sample rate index

 * @param  aRate : Actual Sampling Freq

 * @Return  Index

 */
/*=======================================================================*/
int NUAUDIODec_GetSampleRateIndexL( const int aRate);
int NUAUDIODec_GetSampleRatebyIndex( const int index);
void* NUAUDIODEC_ComponentThread (void* pThreadData);

OMX_U32 NUAUDIODEC_ParseHeader(OMX_BUFFERHEADERTYPE* pBufHeader,
                           NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate);

/*  =========================================================================*/
/*  func    GetBits                                                          */
/*                                                                           */
/*  desc    Gets aBits number of bits from position aPosition of one buffer  */
/*            and returns the value in a TUint value.                        */
/*  =========================================================================*/
OMX_U32 NUAUDIODEC_GetBits(OMX_U32* nPosition, OMX_U8 nBits, OMX_U8* pBuffer, OMX_BOOL bIcreasePosition);

/*=======================================================================*/
/*! @fn SignalIfAllBuffersAreReturned
 * @brief Sends pthread signal to indicate OMX has returned all buffers to app
 * @param  none
 * @Return void
 */
/*=======================================================================*/
void SignalIfAllBuffersAreReturned(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate);
/*=======================================================================*/
/*! @fn NUAUDIODEC_Init
 * @brief init dec
 * @param  naUrl
 * @Return int
 */
/*=======================================================================*/
int NUAUDIODEC_Init(void *);
#endif
