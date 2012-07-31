/*
 *Copyright (C), 2009~2019, NUFRONT. Co., Ltd.
 *File name:      OMX_NuAudioDecoder.c
 *Author: Zhai Jianfeng Version: v1.0       Date: 2010-09-25
 *
 *This file implements OMX Component for NUAUDIO decoder that
 *is fully compliant with the OMX Audio specification 1.5.
 *
*/
/****************************************************************
*  INCLUDE FILES
****************************************************************/
/* ----- system and platform files ----------------------------*/

#include <utils/Log.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/nufrontlog.h>
/*------- Program Header Files -----------------------------------------------*/

#include "OMX_NuAudioDec_Utils.h"

/* define component role */
#define NUAUDIO_DEC_ROLE "audio_decoder.nuaudio"

/****************************************************************
*  EXTERNAL REFERENCES NOTE : only use if not found in header file
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PUBLIC DECLARATIONS Defined here, used elsewhere
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/

/****************************************************************
*  PRIVATE DECLARATIONS Defined here, used only here
****************************************************************/
/*--------data declarations -----------------------------------*/

/*--------function prototypes ---------------------------------*/
static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE hComp,
                                   OMX_CALLBACKTYPE* pCallBacks, OMX_PTR pAppData);
static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID);

static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE hComp, OMX_COMMANDTYPE nCommand,
                                  OMX_U32 nParam, OMX_PTR pCmdData);

static OMX_ERRORTYPE GetParameter(OMX_HANDLETYPE hComp, OMX_INDEXTYPE nParamIndex,
                                  OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR ComponentParamStruct);
static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR pComponentConfigStructure);
static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR pComponentConfigStructure);

static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE hComp, OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE GetState (OMX_HANDLETYPE hComp, OMX_STATETYPE* pState);
static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort,
                                             OMX_TUNNELSETUPTYPE* pTunnelSetup);

static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle);

static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                                     OMX_IN OMX_U32 nPortIndex,
                                     OMX_IN OMX_PTR pAppPrivate,
                                     OMX_IN OMX_U32 nSizeBytes);

static OMX_ERRORTYPE FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE UseBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                                OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                                OMX_IN OMX_U32 nPortIndex,
                                OMX_IN OMX_PTR pAppPrivate,
                                OMX_IN OMX_U32 nSizeBytes,
                                OMX_IN OMX_U8* pBuffer);


static OMX_ERRORTYPE GetExtensionIndex(OMX_IN  OMX_HANDLETYPE hComponent,
                                       OMX_IN  OMX_STRING cParameterName,
                                       OMX_OUT OMX_INDEXTYPE* pIndexType);

static OMX_ERRORTYPE ComponentRoleEnum(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_OUT OMX_U8 *cRole,
                                       OMX_IN OMX_U32 nIndex);

/*-------------------------------------------------------------------*/
/**
  * OMX_ComponentInit() Set the all the function pointers of component
  *
  * This method will update the component function pointer to the handle
  *
  * @param hComp         handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_ErrorInsufficientResources If the malloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComp)
{
    LOG_AUDIO("audio_dbg:OMX_ComponentInit===>>>for NUFRONT function in.");
    OMX_U8 * fileName = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef_ip = NULL, *pPortDef_op = NULL;
    OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFormat = NULL;
    OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE *nuaudio_ip = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *nuaudio_op = NULL;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate;
    AUDIODEC_PORT_TYPE *pCompPort = NULL;
    NUAUDIODEC_BUFFERLIST *pTemp = NULL;
    struct AVCodecContext  *spCodecCtxTmp;
    /*m@nufront start:*/
    if (pHandle->pComponentPrivate != NULL) {
        LOG_AUDIO("audio_dbg:OMX_ComponentInit===>>>fileName is %s.", fileName);
        fileName = pHandle->pComponentPrivate;
    }
    /*m@nufront end*/
    OMX_S32 i = 0;
    OMXDBG_PRINT(stderr, PRINT, 1, 0, "%d :: Entering OMX_ComponentInit\n", __LINE__);

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pHandle,1,1);
    pHandle->SetCallbacks = SetCallbacks;
    pHandle->GetComponentVersion = GetComponentVersion;
    pHandle->SendCommand = SendCommand;
    pHandle->GetParameter = GetParameter;
    pHandle->SetParameter = SetParameter;
    pHandle->GetExtensionIndex = GetExtensionIndex;
    pHandle->GetConfig = GetConfig;
    pHandle->SetConfig = SetConfig;
    pHandle->GetState = GetState;
    pHandle->EmptyThisBuffer = EmptyThisBuffer;
    pHandle->FillThisBuffer = FillThisBuffer;
    pHandle->ComponentTunnelRequest = ComponentTunnelRequest;
    pHandle->ComponentDeInit = ComponentDeInit;
    pHandle->AllocateBuffer =  AllocateBuffer;
    pHandle->FreeBuffer = FreeBuffer;
    pHandle->UseBuffer = UseBuffer;
    pHandle->ComponentRoleEnum = ComponentRoleEnum;

    OMX_MALLOC_GENERIC(pHandle->pComponentPrivate, NUAUDIODEC_COMPONENT_PRIVATE);

    pComponentPrivate = pHandle->pComponentPrivate;
    pComponentPrivate->pHandle = pHandle;
    OMX_DBG_INIT(pComponentPrivate->dbg, "OMX_DBG_NUAUDIODEC");

#ifdef ANDROID /* leave this now, we may need them later. */
    pComponentPrivate->iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_FALSE;
    pComponentPrivate->iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_FALSE;
#endif
    /*a@nufront start: add to malloc spCodec buffer*/
    ZJFLOGD("will malloc avcodecctx buffer.");
    //spCodecCtxTmp = av_malloc(sizeof(struct AVCodecContext));
    spCodecCtxTmp = avcodec_alloc_context();
    if (NULL == spCodecCtxTmp) {
        eError = OMX_ErrorInsufficientResources;
        ZJFLOGE("malloc AVCodecCtx error.");
        goto EXIT;
    }
    avcodec_get_context_defaults(spCodecCtxTmp);
    spCodecCtxTmp->esds = NULL;
    ZJFLOGD("after malloc avcodecctx buffer.");
    //memset(spCodecCtxTmp, 0, sizeof(struct AVCodecContext));
    ZJFLOGD("after memset spCodecCtx.");
    pComponentPrivate->spCodecCtx = spCodecCtxTmp;
    ZJFLOGD("after envalued spCodecCtx.");
    /*a@nufront end*/


    OMX_MALLOC_GENERIC(pCompPort, AUDIODEC_PORT_TYPE);
    pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC] =  pCompPort;

    OMX_MALLOC_GENERIC(pCompPort, AUDIODEC_PORT_TYPE);
    pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC] = pCompPort;
    OMX_MALLOC_GENERIC(pTemp, NUAUDIODEC_BUFFERLIST);

    pComponentPrivate->pInputBufferList = pTemp;
    OMX_MALLOC_GENERIC(pTemp, NUAUDIODEC_BUFFERLIST);
    pComponentPrivate->pOutputBufferList = pTemp;

    pComponentPrivate->pInputBufferList->numBuffers = 0;
    pComponentPrivate->pOutputBufferList->numBuffers = 0;

    for (i=0; i < MAX_NUM_OF_BUFS_NUAUDIODEC; i++) {
        pComponentPrivate->pInputBufferList->pBufHdr[i] = NULL;
        pComponentPrivate->pOutputBufferList->pBufHdr[i] = NULL;
    }
    pComponentPrivate->bufAlloced = 0;

    OMX_MALLOC_GENERIC(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->sPortParam, OMX_PORT_PARAM_TYPE);
    OMX_MALLOC_GENERIC(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    pComponentPrivate->sPortParam->nPorts = NUM_OF_PORTS_NUAUDIODEC;
    pComponentPrivate->sPortParam->nStartPortNumber = 0x0;

    pComponentPrivate->nuaudioParams = NULL;
    pComponentPrivate->pcmParams = NULL;
    OMX_MALLOC_GENERIC(nuaudio_ip,OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE);
    OMX_MALLOC_GENERIC(nuaudio_op,OMX_AUDIO_PARAM_PCMMODETYPE);
    pComponentPrivate->nuaudioParams = nuaudio_ip;
    pComponentPrivate->pcmParams = nuaudio_op;

    pComponentPrivate->dasfmode = 0;
    pComponentPrivate->bCompThreadStarted = 0;
    pComponentPrivate->bExitCompThrd = 0;

    pComponentPrivate->parameteric_stereo = 0;
    pComponentPrivate->bInitParamsInitialized = 0;
    pComponentPrivate->pMarkBuf = NULL;
    pComponentPrivate->pMarkData = NULL;
    pComponentPrivate->nEmptyBufferDoneCount = 0;
    pComponentPrivate->nEmptyThisBufferCount = 0;
    pComponentPrivate->nFillBufferDoneCount = 0;
    pComponentPrivate->nFillThisBufferCount = 0;
    pComponentPrivate->bDisableCommandParam = 0;
    pComponentPrivate->bEnableCommandParam = 0;
    pComponentPrivate->nUnhandledFillThisBuffers = 0;
    pComponentPrivate->nHandledFillThisBuffers = 0;
    pComponentPrivate->nUnhandledEmptyThisBuffers = 0;
    pComponentPrivate->nHandledEmptyThisBuffers = 0;
    pComponentPrivate->SendAfterEOS = 1;
    pComponentPrivate->bFlushOutputPortCommandPending = OMX_FALSE;
    pComponentPrivate->bFlushInputPortCommandPending = OMX_FALSE;
    pComponentPrivate->first_buff = 0;
    pComponentPrivate->first_TS = 0;
    pComponentPrivate->bConfigData = 1;  /* assume the first buffer received will contain only config data */
    pComponentPrivate->reconfigInputPort = 0;
    pComponentPrivate->reconfigOutputPort = 0;
    pComponentPrivate->framemode = 0;
    /*a@nufront:add to handle frame without DTS*/
    pComponentPrivate->nLastDTS= -1;
    pComponentPrivate->nAnchor = -1;

    for (i=0; i < MAX_NUM_OF_BUFS_NUAUDIODEC; i++) {
        pComponentPrivate->pInputBufHdrPending[i] = NULL;
        pComponentPrivate->pOutputBufHdrPending[i] = NULL;
    }
    pComponentPrivate->nNumInputBufPending = 0;
    pComponentPrivate->nNumOutputBufPending = 0;

    for (i=0; i < MAX_NUM_OF_BUFS_NUAUDIODEC; i++) {
        pComponentPrivate->pInBufHdrPausedPending[i] = NULL;
        pComponentPrivate->pOutBufHdrPausedPending[i] = NULL;
    }
    pComponentPrivate->PendingInPausedBufs = 0;
    pComponentPrivate->PendingOutPausedBufs = 0;

    pComponentPrivate->nInvalidFrameCount = 0;
    pComponentPrivate->bDisableCommandPending = 0;
    pComponentPrivate->bEnableCommandPending = 0;

    pComponentPrivate->SBR = 0;
    pComponentPrivate->RAW = 0; /* doesn't do anything, consider deleting */
    pComponentPrivate->numPendingBuffers = 0;
    pComponentPrivate->bNoIdleOnStop= OMX_FALSE;
    pComponentPrivate->nOutStandingFillDones = 0;
    pComponentPrivate->nOpBit = 0;
    pComponentPrivate->dualMonoMode = 0;
    pComponentPrivate->bIsInvalidState = OMX_FALSE;
    pComponentPrivate->sOutPortFormat.eEncoding = OMX_AUDIO_CodingPCM;
    /* Initialize device string to the default value */
    OMX_MALLOC_SIZE(pComponentPrivate->sDeviceString,(100*sizeof(OMX_STRING)),OMX_STRING);
    strcpy((char*)pComponentPrivate->sDeviceString,"/eteedn:i0:o0/codec\0");

    /* initialize role name */
    OMX_MALLOC_GENERIC(pComponentPrivate->componentRole,OMX_PARAM_COMPONENTROLETYPE);
    strcpy((char*)pComponentPrivate->componentRole->cRole, NUAUDIO_DEC_ROLE);

    OMX_MALLOC_GENERIC(pPortDef_ip, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_MALLOC_GENERIC(pPortDef_op, OMX_PARAM_PORTDEFINITIONTYPE);

    pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC] = pPortDef_ip;
    pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC] = pPortDef_op;

    pthread_mutex_init(&pComponentPrivate->AlloBuf_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->AlloBuf_threshold, NULL);
    pComponentPrivate->AlloBuf_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->InLoaded_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InLoaded_threshold, NULL);
    pComponentPrivate->InLoaded_readytoidle = 0;

    pthread_mutex_init(&pComponentPrivate->InIdle_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->InIdle_threshold, NULL);
    pComponentPrivate->InIdle_goingtoloaded = 0;

    pthread_mutex_init(&pComponentPrivate->codecStop_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->codecStop_threshold, NULL);
    pComponentPrivate->codecStop_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->codecFlush_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->codecFlush_threshold, NULL);
    pComponentPrivate->codecFlush_waitingsignal = 0;

    pthread_mutex_init(&pComponentPrivate->CodecInit_mutex, NULL);
    pthread_cond_init (&pComponentPrivate->CodecInit_threshold, NULL);

    /* Set input port defaults */
    pPortDef_ip->nSize                              = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_ip->nPortIndex                         = INPUT_PORT_NUAUDIODEC;
    pPortDef_ip->eDir                               = OMX_DirInput;
    pPortDef_ip->nBufferCountActual                 = NUAUDIOD_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferCountMin                    = NUAUDIOD_NUM_INPUT_BUFFERS;
    pPortDef_ip->nBufferSize                        = NUAUDIOD_INPUT_BUFFER_SIZE;
    pPortDef_ip->nBufferAlignment                   = DSP_CACHE_ALIGNMENT;
    pPortDef_ip->bEnabled                           = OMX_TRUE;
    pPortDef_ip->bPopulated                         = OMX_FALSE;
    pPortDef_ip->eDomain                            = OMX_PortDomainAudio;
    pPortDef_ip->format.audio.eEncoding             = OMX_AUDIO_CodingNUAUDIO;
    pPortDef_ip->format.audio.cMIMEType             = NULL;
    pPortDef_ip->format.audio.pNativeRender         = NULL;
    pPortDef_ip->format.audio.bFlagErrorConcealment = OMX_FALSE;

    /* Set input port defaults */
    pPortDef_op->nSize                              = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    pPortDef_op->nPortIndex                         = OUTPUT_PORT_NUAUDIODEC;
    pPortDef_op->eDir                               = OMX_DirOutput;
    pPortDef_op->nBufferCountMin                    = NUAUDIOD_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferCountActual                 = NUAUDIOD_NUM_OUTPUT_BUFFERS;
    pPortDef_op->nBufferSize                        = NUAUDIOD_OUTPUT_BUFFER_SIZE;
    pPortDef_op->nBufferAlignment                   = DSP_CACHE_ALIGNMENT;
    pPortDef_op->bEnabled                           = OMX_TRUE;
    pPortDef_op->bPopulated                         = OMX_FALSE;
    pPortDef_op->eDomain                            = OMX_PortDomainAudio;
    pPortDef_op->format.audio.eEncoding             = OMX_AUDIO_CodingPCM;
    pPortDef_op->format.audio.cMIMEType             = NULL;
    pPortDef_op->format.audio.pNativeRender         = NULL;
    pPortDef_op->format.audio.bFlagErrorConcealment = OMX_FALSE;

    OMX_MALLOC_GENERIC(pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_MALLOC_GENERIC(pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]->pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);

    pComponentPrivate->bPreempted = OMX_FALSE;

    /* Set input port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = INPUT_PORT_NUAUDIODEC;
    //pPortFormat->nIndex             = OMX_IndexParamAudioNuAudio;
    pPortFormat->nIndex             = OMX_IndexParamAudioAac;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingNUAUDIO;

    /* Set output port format defaults */
    pPortFormat = pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]->pPortFormat;
    OMX_CONF_INIT_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    pPortFormat->nPortIndex         = OUTPUT_PORT_NUAUDIODEC;
    pPortFormat->nIndex             = OMX_IndexParamAudioPcm;
    pPortFormat->eEncoding          = OMX_AUDIO_CodingPCM;

    /* NUAUDIO format defaults */
    /*OMX_CONF_INIT_STRUCT(nuaudio_ip, OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE);
    nuaudio_ip->nPortIndex = INPUT_PORT_NUAUDIODEC;
    nuaudio_ip->nSampleRate = NUAUDIOD_SAMPLING_FREQUENCY;
    nuaudio_ip->nChannels = STEREO_INTERLEAVED_STREAM_NUAUDIODEC;
    nuaudio_ip->eChannelMode = OMX_AUDIO_ChannelModeStereo;
    nuaudio_ip->eNUAUDIOProfile = OMX_AUDIO_NUAUDIOObjectLC;
    nuaudio_ip->eNUAUDIOStreamFormat = OMX_AUDIO_NUAUDIOStreamFormatMP2ADTS;
*/
#ifdef ANDROID
/* the default mode of operation for android */
  /*  nuaudio_ip->eNUAUDIOStreamFormat = OMX_AUDIO_NUAUDIOStreamFormatRAW;*/
#endif

    /* PCM format defaults */
    OMX_CONF_INIT_STRUCT(nuaudio_op, OMX_AUDIO_PARAM_PCMMODETYPE);
    nuaudio_op->eNumData= OMX_NumericalDataSigned;
    nuaudio_op->ePCMMode = OMX_AUDIO_PCMModeLinear;
    nuaudio_op->nPortIndex = OUTPUT_PORT_NUAUDIODEC;
    nuaudio_op->nBitPerSample = NU_BIT_PER_SAMPLE;
    nuaudio_op->nChannels = STEREO_INTERLEAVED_STREAM_NUAUDIODEC;
    nuaudio_op->nSamplingRate = NUAUDIOD_SAMPLING_FREQUENCY;
    nuaudio_op->bInterleaved = OMX_TRUE;

    pComponentPrivate->bPortDefsAllocated = 1;
    pComponentPrivate->mOutputLen = 0;
    eError = NuAudioDec_StartCompThread(pHandle);
    if (eError != OMX_ErrorNone) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::Error returned from the Component\n",
                       __LINE__);
        goto EXIT;
    }

 EXIT:
    if(OMX_ErrorNone != eError) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: ************* ERROR: Freeing Other Malloced Resources\n",__LINE__);
        OMX_MEMFREE_STRUCT(pPortDef_ip);
        OMX_MEMFREE_STRUCT(pPortDef_op);
        OMX_MEMFREE_STRUCT(nuaudio_ip);
        OMX_MEMFREE_STRUCT(nuaudio_op);
        OMX_MEMFREE_STRUCT(pTemp);
    }
    LOG_AUDIO("audio_dbg:OMX_ComponentInit===>>>funtion out.");
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Exiting OMX_ComponentInit\n", __LINE__);
    return eError;
}
/*-------------------------------------------------------------------*/
/**
  *  SendCommand() used to send the commands to the component
  *
  * This method will be used by the application.
  *
  * @param phandle         handle for this instance of the component
  * @param Cmd             Command to be sent to the component
  * @param nParam          indicates commmad is sent using this method
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SendCommand (OMX_HANDLETYPE phandle,
                                  OMX_COMMANDTYPE Cmd,
                                  OMX_U32 nParam,OMX_PTR pCmdData)
{
    LOG_AUDIO("audio_dbg:SendCommand===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int nRet;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)phandle;
    NUAUDIODEC_COMPONENT_PRIVATE *pCompPrivate = NULL;

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pHandle,1,1)
    pCompPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef _ERROR_PROPAGATION__
    if (pCompPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
     OMX_ERROR4(pCompPrivate->dbg, "%d ::Error returned from the Component\n",
                       __LINE__);
        goto EXIT;
    }
#else
    if(pCompPrivate->curState == OMX_StateInvalid){
       OMX_ERROR4(pCompPrivate->dbg, "%d ::Error returned from the Component\n",
                       __LINE__);
       NUAUDIODEC_OMX_ERROR_EXIT(eError, OMX_ErrorInvalidState,"OMX_ErrorInvalidState");
    }
#endif
    LOG_AUDIO("audio_dbg:SendCommand===>>>Cmd=%d. ", Cmd);
    switch(Cmd) {
    case OMX_CommandStateSet:
    LOG_AUDIO("audio_dbg:SendCommand===>>>Cmd=OMX_CommandStateSet. nParam is %d, pCompPrivate->curState is %d", nParam, pCompPrivate->curState);
        if (nParam == OMX_StateLoaded) {
            pCompPrivate->bLoadedCommandPending = OMX_TRUE;
        }
        if(pCompPrivate->curState == OMX_StateLoaded) {
            if((nParam == OMX_StateExecuting) || (nParam == OMX_StatePause)) {
                pCompPrivate->cbInfo.EventHandler (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorIncorrectStateTransition,
                                                   OMX_NUFRONT_ErrorMinor,
                                                   NULL);
                OMX_ERROR4(pCompPrivate->dbg, "%d :: Incorrect St Tr fm Loaded to Executing By App\n",__LINE__);
                goto EXIT;
            }

            if(nParam == OMX_StateInvalid) {
                pCompPrivate->curState = OMX_StateInvalid;
                pCompPrivate->cbInfo.EventHandler (pHandle,
                                                   pHandle->pApplicationPrivate,
                                                   OMX_EventError,
                                                   OMX_ErrorInvalidState,
                                                   OMX_NUFRONT_ErrorMinor,
                                                   NULL);
                OMX_ERROR4(pCompPrivate->dbg, "%d :: Incorrect State Tr from Loaded to Invalid by Application\n",__LINE__);
                goto EXIT;
            }
        }
        break;
    case OMX_CommandFlush:
        LOG_AUDIO("audio_dbg:SendCommand===>>>Cmd=OMX_CommandFlush, nParam is %d. ",nParam );
        if(nParam > 1 && nParam != -1) {
            NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
        }
        break;
    case OMX_CommandPortDisable:
        LOG_AUDIO("audio_dbg:SendCommand===>>>Cmd=OMX_CommandPortDisable");
        OMX_PRDSP2(pCompPrivate->dbg, "%d :: NUAUDIODEC: Entered switch - Command Port Disable \n",__LINE__);
        break;
    case OMX_CommandPortEnable:
        LOG_AUDIO("audio_dbg:SendCommand===>>>Cmd=OMX_CommandPortEnable");
        OMX_PRDSP2(pCompPrivate->dbg, "%d :: NUAUDIODEC: Entered switch - Command Port Enable \n",__LINE__);
        break;
    case OMX_CommandMarkBuffer:
        LOG_AUDIO("audio_dbg:SendCommand===>>>Cmd=OMX_CommandMarkBuffer, nParam is %d",nParam);
        OMX_PRDSP2(pCompPrivate->dbg, "%d :: NUAUDIODEC: Entered switch - Command Mark Buffer\n",__LINE__);
        if(nParam > 0) {
            NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
        }
        break;

    default:
        LOG_AUDIO("audio_dbg:SendCommand===>>>default branch.");
        OMX_ERROR4(pCompPrivate->dbg, "%d :: NUAUDIODEC: Entered switch - Default\n",__LINE__);
        pCompPrivate->cbInfo.EventHandler(
                                          pHandle, pHandle->pApplicationPrivate,
                                          OMX_EventError,
                                          OMX_ErrorBadParameter,
                                          OMX_NUFRONT_ErrorMinor,
                                          "Invalid Command");
        break;
    }

        LOG_AUDIO("audio_dbg:SendCommand===>>>before write to cmd 1.");
        nRet = write (pCompPrivate->cmdPipe[1], &Cmd, sizeof(Cmd));
        LOG_AUDIO("audio_dbg:SendCommand===>>>after write to cmd 1.");
    if (nRet == -1) {
        OMX_ERROR2(pCompPrivate->dbg, "EXITING:: write to cmd pipe failed!!!\n");
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorInsufficientResources,"write failed: OMX_ErrorInsufficientResources");
    }


    if (Cmd == OMX_CommandMarkBuffer) {
        LOG_AUDIO("audio_dbg:SendCommand===>>>OMX_CommandMarkBuffer, before write to cmd 2.");
        nRet = write (pCompPrivate->cmdDataPipe[1], &pCmdData,sizeof(OMX_PTR));
        LOG_AUDIO("audio_dbg:SendCommand===>>>after write to cmd 2, nRet is %d.", nRet);
        if (nRet == -1) {
            OMX_ERROR2(pCompPrivate->dbg, "EXITING:: write to cmd data pipe failed for MarkBuffer!!!\n");
            NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorInsufficientResources,"write failed: OMX_ErrorInsufficientResources");
        }
    }else {
        nRet = write (pCompPrivate->cmdDataPipe[1], &nParam,
                      sizeof(OMX_U32));
        LOG_AUDIO("audio_dbg:SendCommand===>>>after write to cmd 2, nRet is %d.", nRet);
        if (nRet == -1) {
            OMX_ERROR2(pCompPrivate->dbg, "EXITING:: command data pipe write failed\n");
            NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorInsufficientResources,"write failed: OMX_ErrorInsufficientResources");
        }
    }

    /* add for acoustic control */
 EXIT:
    if (pCompPrivate != NULL) {
    OMX_PRINT1(pCompPrivate->dbg, "%d :: Returning = 0x%x\n",
                       __LINE__, eError);
    }
    return eError;
}

/*-------------------------------------------------------------------*/
/* ================================================================================= * */
/**
* @fn GetParameter() function gets the various parameter values of the
* component.
*
* @param hComp         This is component handle.
*
* @param nParamIndex   This is enumerate values which specifies what kind of
*                      information is to be retreived form the component.
*
* @param ComponentParameterStructure      This is output argument which is
*                                         filled by the component component
*
* @pre          The component should be in loaded state.
*
* @post         None
*
* @return      OMX_ErrorNone = Successful Inirialization of the component\n
*              OMX_ErrorBadPortIndex = Bad port index specified by application.
*/
/* ================================================================================ * */
static OMX_ERRORTYPE GetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR ComponentParameterStructure)
{
    LOG_AUDIO("audio_dbg:GetParameter===>>>function in for NUFRONT of NuAudioDecoder.");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    NUAUDIODEC_COMPONENT_PRIVATE  *pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pParameterStructure;
    pParameterStructure = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;

    NUAUDIODEC_OMX_CONF_CHECK_CMD(hComp,1,1)
    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);
    OMX_PRINT1 (pComponentPrivate->dbg, "%d :: Entering OMX_GetParameter\n", __LINE__);
    NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate, ComponentParameterStructure, 1)
    if (ComponentParameterStructure == NULL) {
        eError = OMX_ErrorBadParameter;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from GetParameter",__LINE__);
        goto EXIT;
    }

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorInvalidState from GetParameter",__LINE__);
        goto EXIT;
    }
#else
    if(pComponentPrivate->curState == OMX_StateInvalid) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d ::Error returned from the Component\n",
                       __LINE__);
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,"write failed: OMX_ErrorIncorrectStateOperation");
    }
#endif

    switch(nParamIndex){
    case OMX_IndexParamAudioInit:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case OMX_IndexParamAudioInit. ");
        NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam ,1,1)
        memcpy(ComponentParameterStructure, pComponentPrivate->sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
           pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex ==
                  pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->nPortIndex) {
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        } else {
            OMX_ERROR2(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamAudioPortFormat:
        if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex
                   == pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->nPortIndex) {
            LOG_AUDIO("audio_dbg:GetParameter===>>>nPortIndex ==INPUT_PORT_NUAUDIODEC. ");
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex
                   > pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
                LOG_AUDIO("audio_dbg:GetParameter===>>> eError = OMX_ErrorNoMore; ");
            } else {
            LOG_AUDIO("audio_dbg:GetParameter===>>> eError = OMX_NoError; ");
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]->pPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        }else if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex
                    ==pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->nPortIndex){
            if(((OMX_AUDIO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex
                    > pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]->pPortFormat->nPortIndex) {
                eError = OMX_ErrorNoMore;
            }
            else {
                memcpy(ComponentParameterStructure, pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]->pPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
        } else {
        LOG_AUDIO("audio_dbg:GetParameter===>>>nPortIndex invalid. ");
            OMX_ERROR2(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from GetParameter \n",__LINE__);
            eError = OMX_ErrorBadPortIndex;
        }
        break;

   case OMX_IndexParamAudioAac:
   case OMX_IndexParamAudioRa:
   case OMX_IndexParamAudioMp3:
   case OMX_IndexParamAudioAdpcm:
   case OMX_IndexParamAudioG723:
   case OMX_IndexParamAudioG729:
   case OMX_IndexParamAudioAmr:
   case OMX_IndexParamAudioWma:
   case OMX_IndexParamAudioSbc:
   case OMX_IndexParamAudioMidi:
   case OMX_IndexParamAudioGsm_FR:
   case OMX_IndexParamAudioMidiLoadUserSound:
   case OMX_IndexParamAudioG726:
   case OMX_IndexParamAudioGsm_EFR:
   case OMX_IndexParamAudioGsm_HR:
   case OMX_IndexParamAudioPdc_FR:
   case OMX_IndexParamAudioPdc_HR:
   case OMX_IndexParamAudioPdc_EFR:
   case OMX_IndexParamAudioTdma_FR:
   case OMX_IndexParamAudioTdma_EFR:
   case OMX_IndexParamAudioQcelp8:
   case OMX_IndexParamAudioQcelp13:
   case OMX_IndexParamAudioEvrc:
   case OMX_IndexParamAudioSmv:
   case OMX_IndexParamAudioVorbis:
        break;
   case OMX_IndexParamAudioNuAudio:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case OMX_IndexParamAudioNuAudio. ");
            if (NULL == ComponentParameterStructure) {
                eError = OMX_ErrorBadPortIndex;
                break;
            }
        memcpy(ComponentParameterStructure, pComponentPrivate->spCodecCtx, sizeof(OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE));
        break;
    case OMX_IndexParamAudioPcm:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case OMX_IndexParamAudioPcm. ");
        NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate->pcmParams ,1,1)
        memcpy(ComponentParameterStructure,pComponentPrivate->pcmParams,sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        break;
    case OMX_IndexParamCompBufferSupplier:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case OMX_IndexParamCompBufferSupplier. ");
        if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirInput) {
            OMX_PRINT2(pComponentPrivate->dbg, ":: GetParameter OMX_IndexParamCompBufferSupplier \n");
        }
        else if(((OMX_PARAM_BUFFERSUPPLIERTYPE *)(ComponentParameterStructure))->nPortIndex == OMX_DirOutput) {
            OMX_PRINT2(pComponentPrivate->dbg, ":: GetParameter OMX_IndexParamCompBufferSupplier \n");
        }
        else {
            OMX_ERROR2(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from GetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;

    case OMX_IndexParamVideoInit:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case OMX_IndexParamVideoInit. ");
    case OMX_IndexParamImageInit:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case OMX_IndexParamImageInit. ");
    case OMX_IndexParamOtherInit:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case OMX_IndexParamOtherInit. ");
#ifdef ANDROID
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: Entering OMX_IndexParamVideoInit\n", __LINE__);
        OMX_PRINT2(pComponentPrivate->dbg, "%d :: Entering OMX_IndexParamImageInit/OtherInit\n", __LINE__);
        NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam ,1,1)
        memcpy(ComponentParameterStructure,pComponentPrivate->sPortParam,sizeof(OMX_PORT_PARAM_TYPE));
        eError = OMX_ErrorNone;
#else
        eError = OMX_ErrorUnsupportedIndex;
#endif
        break;


    case OMX_IndexParamPriorityMgmt:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case OMX_IndexParamPriorityMgmt. ");
        NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate->pPriorityMgmt ,1,1)
        memcpy(ComponentParameterStructure, pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
        break;

#ifdef ANDROID
    case (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
    {
        LOG_AUDIO("audio_dbg:GetParameter===>>>case PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX. ");
        PV_OMXComponentCapabilityFlagsType* pCap_flags = (PV_OMXComponentCapabilityFlagsType *) ComponentParameterStructure;
        if (NULL == pCap_flags)
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: ERROR PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX\n", __LINE__);
            eError =  OMX_ErrorBadParameter;
            goto EXIT;
        }
        memcpy(pCap_flags, &(pComponentPrivate->iPVCapabilityFlags), sizeof(PV_OMXComponentCapabilityFlagsType));
    eError = OMX_ErrorNone;
    }
    break;
#endif
    default:
        LOG_AUDIO("audio_dbg:GetParameter===>>>case default. ");
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    if (pComponentPrivate != NULL) {
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",
                       __LINE__, eError);
    }
    return eError;
}


/* ================================================================================= * */
/**
* @fn SetParameter() function sets the various parameter values of the
* component.
*
* @param hComp         This is component handle.
*
* @param nParamIndex   This is enumerate values which specifies what kind of
*                      information is to be set for the component.
*
* @param ComponentParameterStructure      This is input argument which contains
*                                         the values to be set for the component.
*
* @pre          The component should be in loaded state.
*
* @post         None
*
* @return      OMX_ErrorNone = Successful Inirialization of the component\n
*              OMX_ErrorBadPortIndex = Bad port index specified by application.
*/
/* ================================================================================ * */
static OMX_ERRORTYPE SetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR pCompParam)
{
    LOG_AUDIO("audio_dbg:SetParameter===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle= (OMX_COMPONENTTYPE*)hComp;
    NUAUDIODEC_COMPONENT_PRIVATE  *pComponentPrivate;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pComponentParam = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pComponentParamPort = NULL;
    OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE *pCompNuAudioParam = NULL;
    OMX_AUDIO_PARAM_PCMMODETYPE *pCompPcmParam = NULL;
    OMX_PARAM_COMPONENTROLETYPE  *pRole;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;

    NUAUDIODEC_OMX_CONF_CHECK_CMD(hComp,1,1)
    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate, pCompParam, 1)

    if (pComponentPrivate->curState != OMX_StateLoaded) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,"OMX_ErrorIncorrectStateOperation");
    }

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorInvalidState from SetParameter",__LINE__);
        goto EXIT;
    }
#endif

    switch(nParamIndex) {
    case OMX_IndexParamAudioPortFormat:
        {
            pComponentParam = (OMX_AUDIO_PARAM_PORTFORMATTYPE *)pCompParam;
            if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]->pPortFormat->nPortIndex ) {
                memcpy(pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            } else if ( pComponentParam->nPortIndex == pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]->pPortFormat->nPortIndex ) {
                memcpy(pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]->pPortFormat, pComponentParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            } else {
                OMX_ERROR2(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
        }
        break;
   case OMX_IndexParamAudioAac:
   case OMX_IndexParamAudioRa:
   case OMX_IndexParamAudioMp3:
   case OMX_IndexParamAudioAdpcm:
   case OMX_IndexParamAudioG723:
   case OMX_IndexParamAudioG729:
   case OMX_IndexParamAudioAmr:
   case OMX_IndexParamAudioWma:
   case OMX_IndexParamAudioSbc:
   case OMX_IndexParamAudioMidi:
   case OMX_IndexParamAudioGsm_FR:
   case OMX_IndexParamAudioMidiLoadUserSound:
   case OMX_IndexParamAudioG726:
   case OMX_IndexParamAudioGsm_EFR:
   case OMX_IndexParamAudioGsm_HR:
   case OMX_IndexParamAudioPdc_FR:
   case OMX_IndexParamAudioPdc_HR:
   case OMX_IndexParamAudioPdc_EFR:
   case OMX_IndexParamAudioTdma_FR:
   case OMX_IndexParamAudioTdma_EFR:
   case OMX_IndexParamAudioQcelp8:
   case OMX_IndexParamAudioQcelp13:
   case OMX_IndexParamAudioEvrc:
   case OMX_IndexParamAudioSmv:
   case OMX_IndexParamAudioVorbis:
        {
        }
        break;
   case OMX_IndexParamAudioNuAudio:
        ZJFLOGD("case OMX_IndexParamAudioNuAudio.");
        LOG_AUDIO("audio_dbg:SetParameter===>>>swith (nParamIndex) case is %d", nParamIndex);
        ZJFLOGD("pCompParam is 0x%x", pCompParam);
        if (NULL == pCompParam) {
            eError = OMX_ErrorBadPortIndex;
            ZJFLOGE("error for OMX_IndexParamAudioNuAudio");
        }
        pComponentPrivate->pcmParams->nSamplingRate = ((struct AVCodecContext *)pCompParam)->sample_rate;
        pComponentPrivate->pcmParams->nChannels = ((struct AVCodecContext *)pCompParam)->channels;
        NUAUDIODEC_Ffmpeg_Init(pComponentPrivate, pCompParam);
        break;
    case OMX_IndexParamPortDefinition:
        {
            pComponentParamPort = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
            if (pComponentParamPort->nPortIndex == 0) {
                if (pComponentParamPort->eDir != OMX_DirInput) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Invalid input buffer Direction\n",__LINE__);
                    eError = OMX_ErrorBadParameter;
                    goto EXIT;
                }
                if (pComponentParamPort->format.audio.eEncoding != OMX_AUDIO_CodingNUAUDIO) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Invalid format Parameter\n",__LINE__);
                    eError = OMX_ErrorBadParameter;
                    goto EXIT;
                }
            } else if (pComponentParamPort->nPortIndex == 1) {
                if (pComponentParamPort->eDir != OMX_DirOutput) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Invalid Output buffer Direction\n",__LINE__);
                    eError = OMX_ErrorBadParameter;
                    goto EXIT;
                }
                if (pComponentParamPort->format.audio.eEncoding != OMX_AUDIO_CodingPCM) {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Invalid format Parameter\n",__LINE__);
                    eError = OMX_ErrorBadParameter;
                    goto EXIT;
                }
            } else {
                OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
            OMX_PRINT2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
            if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                        pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->nPortIndex) {
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
                memcpy(pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC], pCompParam,sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            }
            else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                        pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->nPortIndex) {
                OMX_PRINT2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamPortDefinition \n",__LINE__);
                memcpy(pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC], pCompParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            }
            else {
                OMX_ERROR2(pComponentPrivate->dbg, "%d :: OMX_ErrorBadPortIndex from SetParameter",__LINE__);
                eError = OMX_ErrorBadPortIndex;
            }
        }
        break;
    case OMX_IndexParamPriorityMgmt:
        {
            OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamPriorityMgmt \n",__LINE__);
            NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate->pPriorityMgmt,1,1)
            memcpy(pComponentPrivate->pPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
        }
        break;

    case OMX_IndexParamAudioInit:
        OMX_PRDSP2(pComponentPrivate->dbg, "%d :: SetParameter OMX_IndexParamAudioInit \n",__LINE__);
        NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate->sPortParam, 1,1)
        memcpy(pComponentPrivate->sPortParam, (OMX_PORT_PARAM_TYPE*)pCompParam, sizeof(OMX_PORT_PARAM_TYPE));
        break;
    case OMX_IndexParamStandardComponentRole:
        if (pCompParam) {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate->componentRole, 1,1)
            memcpy(pComponentPrivate->componentRole, (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        } else {
            eError = OMX_ErrorBadParameter;
        }
        break;

    case OMX_IndexParamAudioPcm:
        if(pCompParam){
            pCompPcmParam = (OMX_AUDIO_PARAM_PCMMODETYPE *)pCompParam;
            NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate->pcmParams, 1,1)
            memcpy(pComponentPrivate->pcmParams, pCompPcmParam, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        else{
            eError = OMX_ErrorBadParameter;
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadParameter from SetParameter",__LINE__);
        }
        if (((OMX_AUDIO_PARAM_PCMMODETYPE *)(pCompParam))->nBitPerSample == 24) {
            pComponentPrivate->nOpBit = 1;
        }
        else {
            pComponentPrivate->nOpBit = 0;
        }
        break;

    case OMX_IndexParamCompBufferSupplier:
        if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
           pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->nPortIndex) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyInput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));

        }
        else if(((OMX_PARAM_PORTDEFINITIONTYPE *)(pCompParam))->nPortIndex ==
                pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->nPortIndex) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: SetParameter OMX_IndexParamCompBufferSupplier \n");
            sBufferSupplier.eBufferSupplier = OMX_BufferSupplyOutput;
            memcpy(&sBufferSupplier, pCompParam, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        }
        else {
            OMX_ERROR2(pComponentPrivate->dbg, ":: OMX_ErrorBadPortIndex from SetParameter");
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    default:
        OMX_ERROR2(pComponentPrivate->dbg, "%d :: SetParameter OMX_ErrorUnsupportedIndex \n",__LINE__);
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    if (pComponentPrivate != NULL) {
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",
                            __LINE__, eError);
    }
    LOG_AUDIO("audio_dbg:SetParameter===>>>function out, eError is %d", eError);
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  SetConfig() Sets the configraiton to the component
  *
  * @param hComp         handle for this instance of the component
  * @param nConfigIndex
  * @param ComponentConfigStructure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE SetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    LOG_AUDIO("audio_dbg:SetConfig===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*)hComp;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    //NUFRONT_OMX_DSP_DEFINITION* pDspDefinition = NULL;
    OMX_S16* deviceString = NULL;
    //NUFRONT_OMX_DATAPATH dataPath;
    OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE *nuaudio_params = NULL;
    OMX_U32 pValues[4];

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pHandle,1,1)
        pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorBadParameter from SetConfig\n",__LINE__);
        goto EXIT;
    }
#endif

    switch (nConfigIndex) {
    case OMX_IndexParamAudioAac:
    case OMX_IndexParamAudioRa:
    case OMX_IndexParamAudioMp3:
    case OMX_IndexParamAudioAdpcm:
    case OMX_IndexParamAudioG723:
    case OMX_IndexParamAudioG729:
    case OMX_IndexParamAudioAmr:
    case OMX_IndexParamAudioWma:
    case OMX_IndexParamAudioSbc:
    case OMX_IndexParamAudioMidi:
    case OMX_IndexParamAudioGsm_FR:
    case OMX_IndexParamAudioMidiLoadUserSound:
    case OMX_IndexParamAudioG726:
    case OMX_IndexParamAudioGsm_EFR:
    case OMX_IndexParamAudioGsm_HR:
    case OMX_IndexParamAudioPdc_FR:
    case OMX_IndexParamAudioPdc_HR:
    case OMX_IndexParamAudioPdc_EFR:
    case OMX_IndexParamAudioTdma_FR:
    case OMX_IndexParamAudioTdma_EFR:
    case OMX_IndexParamAudioQcelp8:
    case OMX_IndexParamAudioQcelp13:
    case OMX_IndexParamAudioEvrc:
    case OMX_IndexParamAudioSmv:
    case OMX_IndexParamAudioVorbis:
    case OMX_IndexParamAudioNuAudio:
        nuaudio_params = (OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE*)ComponentConfigStructure;
        break;

    case OMX_IndexCustomNuAudioDecHeaderInfoConfig:
        {
            break;
        }
    case  OMX_IndexCustomNuAudioDecDataPath:
        deviceString = (OMX_S16*)ComponentConfigStructure;
        if (deviceString == NULL) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        break;

    case OMX_IndexCustomDebug:
    OMX_DBG_SETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
    break;

    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
 EXIT:
    if (pComponentPrivate != NULL) {
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",
                            __LINE__, eError);
    }
    return eError;
}


/* ================================================================================= * */
/**
* @fn SetCallbacks() Sets application callbacks to the component
*
* @param pComponent  This is component handle.
*
* @param pCallBacks  Application callbacks
*
* @param pAppData    Application specified private data.
*
* @pre          None
*
* @post         None
*
* @return      OMX_ErrorNone = Successful Inirialization of the component
*              OMX_ErrorBadParameter = If callback argument is NULL.
*/
/* ================================================================================ * */

static OMX_ERRORTYPE SetCallbacks (OMX_HANDLETYPE pComponent,
                                   OMX_CALLBACKTYPE* pCallBacks,
                                   OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*)pComponent;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate;

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pHandle,1,1)
    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pCallBacks, pCallBacks->EventHandler, pCallBacks->EmptyBufferDone)
    NUAUDIODEC_OMX_CONF_CHECK_CMD(pCallBacks->FillBufferDone, 1, 1)

    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    pHandle->pApplicationPrivate = pAppData;
    OMX_PRSTATE2(pComponentPrivate->dbg, "****************** Component State Set to Loaded\n\n");
    pComponentPrivate->curState = OMX_StateLoaded;

 EXIT:
    return eError;
}

/* ================================================================================= * */
/**
* @fn GetComponentVersion() Sets application callbacks to the component. Currently this
* function is not implemented.
*
* @param hComp  This is component handle.
*
* @param pComponentName  This is component name.
*
* @param pComponentVersion  This output argument will contain the component
*                           version when this function exits successfully.
*
* @param pSpecVersion    This is specification version.
*
* @param pComponentUUID  This specifies the UUID of the component.
*
* @pre          None
*
* @post         None
*
* @return      OMX_ErrorNone = Successful Inirialization of the component
*/
/* ================================================================================ * */
static OMX_ERRORTYPE GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName,
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion,
                                          OMX_UUIDTYPE* pComponentUUID)
{
    LOG_AUDIO("audio_dbg:GetComponentVersion===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE*) hComp;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorInvalidState from GetComponentVersion",__LINE__);
        goto EXIT;
    }
#endif

    /* Copy component version structure */
    if(pComponentVersion != NULL && pComponentName != NULL) {
        strcpy(pComponentName, pComponentPrivate->cComponentName);
        memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), sizeof(pComponentPrivate->ComponentVersion.s));
    }
    else {
        OMX_ERROR2(pComponentPrivate->dbg, "%d :: OMX_ErrorBadParameter from GetComponentVersion",__LINE__);
        eError = OMX_ErrorBadParameter;
    }

    OMX_PRINT1(pComponentPrivate->dbg, "%d :: Returning = 0x%x\n",__LINE__,eError);
 EXIT:
    return eError;
}

/* ================================================================================= * */
/**
* @fn GetConfig() gets the configuration of the component depending on the value
* of nConfigINdex. This function is currently not implemented.
*
* @param hComp  This is component handle.
*
* @param nConfigIndex  This is config index to get the configuration of
*                      component.
*
* @param ComponentConfigStructure This is configuration structure that is filled
* by the component depending on the value of nConfigIndex.
*
* @pre          None
*
* @post         None
*
* @return      OMX_ErrorNone = Successful Inirialization of the component
*/
/* ================================================================================ * */
static OMX_ERRORTYPE GetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex,
                                OMX_PTR ComponentConfigStructure)
{
    LOG_AUDIO("audio_dbg:GetConfig===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate;
    //NUFRONT_OMX_STREAM_INFO *streamInfo;

    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComp)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorInvalidState from GetConfig \n",__LINE__);
        goto EXIT;
    }
#endif

    if (nConfigIndex ==  OMX_IndexParamAudioNuAudio) {
        memcpy(ComponentConfigStructure, pComponentPrivate->nuaudioParams,
                   sizeof(OMX_AUDIO_PARAM_NUAUDIOPROFILETYPE));
    }
    else if(nConfigIndex == OMX_IndexCustomDebug) {
    OMX_DBG_GETCONFIG(pComponentPrivate->dbg, ComponentConfigStructure);
    }

 EXIT:
    return eError;
}

/*-------------------------------------------------------------------*/
/* ================================================================================= * */
/**
* @fn GetState() Gets the current state of the component.
*
* @param pComponent  This is component handle.
*
* @param pState      This is the output argument that contains the state of the
*                    component.
*
* @pre          None
*
* @post         None
*
* @return      OMX_ErrorNone = Successful Inirialization of the component
*              OMX_ErrorBadParameter = if output argument is NULL.
*/
/* ================================================================================ * */
static OMX_ERRORTYPE GetState (OMX_HANDLETYPE pComponent, OMX_STATETYPE* pState)
{
    LOG_AUDIO("audio_dbg:GetState===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;

    if (!pState) {
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d :: About to exit GetState with bad parameter\n", __LINE__);
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pHandle,1,1);
    if (pHandle && pHandle->pComponentPrivate) {
        *pState =  ((NUAUDIODEC_COMPONENT_PRIVATE*) pHandle->pComponentPrivate)->curState;
    } else {
        OMXDBG_PRINT(stderr, STATE, 2, 0, "%d :: In GetState\n", __LINE__);
        OMXDBG_PRINT(stderr, STATE, 2, 0, "Component State Set to Loaded\n\n");
        *pState = OMX_StateLoaded;
    }

    eError = OMX_ErrorNone;

 EXIT:
    return eError;
}

/* ================================================================================= * */
/**
* @fn EmptyThisBuffer() This function is used by application to sent the filled
* input buffers to the component.
*
* @param pComponent  This is component handle.
*
* @param pBuffer     This is pointer to the buffer header that come from the
*                    application.
*
* @pre          None
*
* @post         None
*
* @return      OMX_ErrorNone = Successful exit of the function
*              OMX_ErrorBadParameter =  Bad input argument
*              OMX_ErrorBadPortIndex = Bad port index supplied by the
*              application
*/
/* ================================================================================ * */
static OMX_ERRORTYPE EmptyThisBuffer (OMX_HANDLETYPE pComponent,
                                      OMX_BUFFERHEADERTYPE* pBuffer)
{
    LOG_AUDIO("audio_dbg:EmptyThisBuffer===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    int ret=0;
    pPortDef = ((NUAUDIODEC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[INPUT_PORT_NUAUDIODEC];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorInvalidState from EmptyThisBuffer\n", __LINE__);
        goto EXIT;
    }
#endif
    if(!pPortDef->bEnabled) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,"OMX_ErrorIncorrectStateOperation");
    }
    if (pBuffer == NULL) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"OMX_ErrorBadParameter");
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"Bad Size");
    }
    if (pBuffer->nInputPortIndex != INPUT_PORT_NUAUDIODEC) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }

    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorVersionMismatch,"OMX_ErrorVersionMismatch");
    }
    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        OMX_PRSTATE2(pComponentPrivate->dbg, "%d pComponentPrivate->curState = %d \n",__LINE__,pComponentPrivate->curState);
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,"OMX_ErrorIncorrectStateOperation");
    }

    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Component Sending Filled ip buff %p to Component Thread\n",__LINE__,pBuffer);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "\n------------------------------------------\n\n");


    pComponentPrivate->pMarkData = pBuffer->pMarkData;
    pComponentPrivate->hMarkTargetComponent = pBuffer->hMarkTargetComponent;
    ret = write (pComponentPrivate->dataPipe[1], &pBuffer,sizeof(OMX_BUFFERHEADERTYPE*));
    if (ret == -1) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }
    else
    {
    LOG_AUDIO("audio_dbg:EmptyThisBuffer===>>>after write to pipe.");
        pComponentPrivate->nUnhandledEmptyThisBuffers++;
        pComponentPrivate->nEmptyThisBufferCount++;
    }

 EXIT:
    LOG_AUDIO("audio_dbg:EmptyThisBuffer===>>>function out.");
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  FillThisBuffer() This callback is used to send the output buffer to
  *  the component
  *
  * @param pComponent    handle for this instance of the component
  * @param nPortIndex    output port number
  * @param pBuffer       buffer to be sent to codec
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The INPUT_PORT_NUAUDIODEC parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE FillThisBuffer (OMX_HANDLETYPE pComponent,
                                     OMX_BUFFERHEADERTYPE* pBuffer)
{
    LOG_AUDIO("audio_dbg:FillThisBuffer===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet=0;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;

    pPortDef = ((NUAUDIODEC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[OUTPUT_PORT_NUAUDIODEC];

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorInvalidState from FillThisBuffer\n", __LINE__);
        goto EXIT;
    }
#endif
    if(!pPortDef->bEnabled) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,"write failed: OMX_ErrorIncorrectStateOperation");
    }
    if (pBuffer == NULL) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"write failed: OMX_ErrorBadParameter");
    }
    if (pBuffer->nSize != sizeof(OMX_BUFFERHEADERTYPE)) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: EmptyThisBuffer: Bad Size\n",__LINE__);
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,"write failed: Bad Size");
    }
    if (pBuffer->nOutputPortIndex != OUTPUT_PORT_NUAUDIODEC) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: EmptyThisBuffer: BadPortIndex\n",__LINE__);
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"write failed: BadPortIndex");
    }
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d::pBuffer->nVersion.nVersion:%lu\n",__LINE__,pBuffer->nVersion.nVersion);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d::pComponentPrivate->nVersion:%lu\n",__LINE__,pComponentPrivate->nVersion);
    if (pBuffer->nVersion.nVersion != pComponentPrivate->nVersion) {
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: FillThisBuffer: BufferHeader Version Mismatch\n",__LINE__);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d::pBuffer->nVersion.nVersion:%lu\n",__LINE__,pBuffer->nVersion.nVersion);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d::pComponentPrivate->nVersion:%lu\n",__LINE__,pComponentPrivate->nVersion);
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorVersionMismatch,"write failed: OMX_ErrorVersionMismatch");
    }
    if(pComponentPrivate->curState != OMX_StateExecuting && pComponentPrivate->curState != OMX_StatePause) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,"write failed: OMX_ErrorIncorrectStateOperation");
    }
    pBuffer->nFilledLen = 0;

    if(pComponentPrivate->pMarkBuf){
        pBuffer->hMarkTargetComponent = pComponentPrivate->pMarkBuf->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkBuf->pMarkData;
        pComponentPrivate->pMarkBuf = NULL;
    }

    if (pComponentPrivate->pMarkData) {
        pBuffer->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
        pBuffer->pMarkData = pComponentPrivate->pMarkData;
        pComponentPrivate->pMarkData = NULL;
    }
    nRet = write (pComponentPrivate->dataPipe[1], &pBuffer,sizeof (OMX_BUFFERHEADERTYPE*));
    if (nRet == -1) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorHardware,"write failed: OMX_ErrorHardware");
    }
    else
    {
        pComponentPrivate->nUnhandledFillThisBuffers++;
        pComponentPrivate->nFillThisBufferCount++;
    }

EXIT:
    return eError;
}

/* ================================================================================= * */
/**
* @fn ComponentDeInit() This function deinitializes the component. It is called
* from OMX Core, not by application. Albeit, Application does call
* OMX_FreeHandle of OMX Core and which in turn calls this function.
*
* @param pHandle  This is component handle.
*
* @pre          None
*
* @post        This function should clean or free as much resources as
*              possible.
*
* @return      OMX_ErrorNone = On Success
*              Appropriate error number in case any error happens.
*/
/* ================================================================================ * */
static OMX_ERRORTYPE ComponentDeInit(OMX_HANDLETYPE pHandle)
{
    LOG_AUDIO("audio_dbg:ComponentDeInit===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ERRORTYPE eError1 = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComponent = (OMX_COMPONENTTYPE *)pHandle;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE threadError = OMX_ErrorNone;
    int pthreadError = 0;
    struct OMX_NUFRONT_Debug dbg;

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponent,1,1)
    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)pComponent->pComponentPrivate;
    NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate,1,1)
    dbg = pComponentPrivate->dbg;

    pComponentPrivate->bExitCompThrd = 1;
    write (pComponentPrivate->cmdPipe[1], &pComponentPrivate->bExitCompThrd, sizeof(OMX_U16));
    pthreadError = pthread_join(pComponentPrivate->ComponentThread, (void*)&threadError);
    if(0 != pthreadError) {
        OMX_ERROR4(dbg, ":: First Error in ComponentDeinit: From pthread_join\n");
        eError = OMX_ErrorHardware;
        goto EXIT;
    }
    if (OMX_ErrorNone != threadError && OMX_ErrorNone != eError) {
        eError = OMX_ErrorInsufficientResources;
        OMX_ERROR4(dbg, "%d :: Error while closing Component Thread\n",__LINE__);
        goto EXIT;
    }

    eError1 = NUAUDIODEC_FreeCompResources(pHandle);
    if (OMX_ErrorNone != eError1) {
        if (OMX_ErrorNone == eError) {
            OMX_ERROR4(dbg, ":: First Error in ComponentDeinit: From FreeCompResources\n");
            eError = eError1;
        }
    }

    OMX_MEMFREE_STRUCT(pComponentPrivate->sDeviceString);
    OMXDBG_PRINT(stderr, BUFFER, 2, 0, ":: Freeing: pComponentPrivate = %p\n",pComponentPrivate);
    OMX_PRINT1(dbg, "::*********** ComponentDeinit is Done************** \n");
    OMX_DBG_CLOSE(dbg);
    OMX_MEMFREE_STRUCT(pComponentPrivate);

 EXIT:
    return eError;
}


/* ================================================================================= * */
/**
* @fn ComponentTunnelRequest() This function estabilishes the tunnel between two
* components. This is not implemented currently.
*
* @param hComp  Handle of this component.
*
* @param nPort Port of this component on which tunneling has to be done.
*
* @param hTunneledComp Handle of the component with which tunnel has to be
*                      established.
*
* @param nTunneledPort Port of the tunneling component.
*
* @param pTunnelSetup Tunnel Setuup parameters.
*
* @pre          None
*
* @post        None
*
* @return      OMX_ErrorNone = On Success
*              Appropriate error number in case any error happens.
*/
/* ================================================================================ * */
static OMX_ERRORTYPE ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort,
                                             OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    LOG_AUDIO("audio_dbg:ComponentTunnelRequest===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    eError = OMX_ErrorNotImplemented;
    return eError;
}



/* ================================================================================= * */
/**
* @fn AllocateBuffer() This function allocated the memory for the buffer onm
* request from application.
*
* @param hComponent  Handle of this component.
*
* @param pBuffer  Pointer to the buffer header.
*
* @param nPortIndex  Input port or Output port
*
* @param pAppPrivate Application private data.
*
* @param nSizeBytes Size of the buffer that is to be allocated.
*
* @pre          None
*
* @post        Requested buffer should get the memory allocated.
*
* @return      OMX_ErrorNone = On Success
*              OMX_ErrorBadPortIndex = Bad port index from app
*/
/* ================================================================================ * */
static OMX_ERRORTYPE AllocateBuffer (OMX_IN OMX_HANDLETYPE hComponent,
                   OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                   OMX_IN OMX_U32 nPortIndex,
                   OMX_IN OMX_PTR pAppPrivate,
                   OMX_IN OMX_U32 nSizeBytes)
{
    LOG_AUDIO("audio_dbg:AllocateBuffer===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader = NULL;

    NUAUDIODEC_OMX_CONF_CHECK_CMD(hComponent,1,1);
    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: OMX_ErrorInvalidState from AllocateBuffer\n", __LINE__);
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
#endif

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pComponentPrivate, 1, 1);

    pPortDef = ((NUAUDIODEC_COMPONENT_PRIVATE*)pComponentPrivate)->pPortDef[nPortIndex];

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pPortDef, 1, 1);
    if (!pPortDef->bEnabled) {
        pComponentPrivate->AlloBuf_waitingsignal = 1;
        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
    }

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);
    memset((pBufferHeader), 0, sizeof(OMX_BUFFERHEADERTYPE));

    OMX_MALLOC_SIZE_DSPALIGN(pBufferHeader->pBuffer,nSizeBytes,OMX_U8);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Malloced = %p\n",__LINE__,pBufferHeader->pBuffer);
    pBufferHeader->nVersion.nVersion = NUAUDIODEC_BUFHEADER_VERSION;

    OMX_PRBUFFER2(pComponentPrivate->dbg, "********************************************\n");
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Allocated BufHeader %p Buffer = %p, on port %ld\n",__LINE__,pBufferHeader,pBufferHeader->pBuffer, nPortIndex);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Ip Num = %ld\n",__LINE__,pComponentPrivate->pInputBufferList->numBuffers);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: Op Num = %ld\n",__LINE__,pComponentPrivate->pOutputBufferList->numBuffers);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "********************************************\n");

    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;


    if (nPortIndex == INPUT_PORT_NUAUDIODEC) {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;
        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->pBufHdr[%ld] = %p\n",
                      pComponentPrivate->pInputBufferList->numBuffers,pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers]);
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 1;
        OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pInputBufferList->numBuffers = %ld\n",pComponentPrivate->pInputBufferList->numBuffers);
        OMX_PRBUFFER2(pComponentPrivate->dbg, "pPortDef->nBufferCountMin = %lu\n",pPortDef->nBufferCountMin);
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = 1;
        }
    } else if (nPortIndex == OUTPUT_PORT_NUAUDIODEC) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;
        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 1;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = 1;
        }
    } else {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadPortIndex,"OMX_ErrorBadPortIndex");
    }

    if((pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated == pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled)&&
       (pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated == pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle)){
            pComponentPrivate->InLoaded_readytoidle = 0;
            pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
            pthread_cond_signal(&pComponentPrivate->InLoaded_threshold);
            pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
    }

    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = NUAUDIODEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = NUAUDIODEC_MINOR_VER;
    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);

    *pBuffer = pBufferHeader;
    pComponentPrivate->bufAlloced = 1;

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }

 EXIT:
    if(OMX_ErrorNone != eError) {
        OMX_PRINT1(pComponentPrivate->dbg, "%d :: ************* ERROR: Freeing Other Malloced Resources\n",__LINE__);
        OMX_MEMFREE_STRUCT_DSPALIGN(pBufferHeader->pBuffer, OMX_U8);
        OMX_MEMFREE_STRUCT(pBufferHeader);
    }
    return eError;
}

/* ================================================================================= * */
/**
* @fn FreeBuffer() This function frees the meomory of the buffer specified.
*
* @param hComponent  Handle of this component.
*
* @param nPortIndex  Input port or Output port
*
* @param pBuffer  Pointer to the buffer header.
*
* @pre          None
*
* @post        Requested buffer should get the memory allocated.
*
* @return      OMX_ErrorNone = On Success
*              OMX_ErrorBadPortIndex = Bad port index from app
*/
/* ================================================================================ * */
static OMX_ERRORTYPE FreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    LOG_AUDIO("audio_dbg:FreeBuffer===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    NUAUDIODEC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_U8* buff;
    int i;
    int inputIndex = -1;
    int outputIndex = -1;
    OMX_COMPONENTTYPE *pHandle;

    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *) (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: pComponentPrivate = %p\n", __LINE__,pComponentPrivate);
    for (i=0; i < MAX_NUM_OF_BUFS_NUAUDIODEC; i++) {
        buff = (OMX_U8 *)pComponentPrivate->pInputBufferList->pBufHdr[i];
        if (buff == (OMX_U8 *)pBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Found matching input buffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "buff = %p\n",buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            inputIndex = i;
            break;
        }
        else {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: This is not a match\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: buff = %p\n",__LINE__,buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }

    for (i=0; i < MAX_NUM_OF_BUFS_NUAUDIODEC; i++) {
        buff = (OMX_U8 *)pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if (buff == (OMX_U8 *)pBuffer) {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Found matching output buffer\n");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "buff = %p\n",buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "pBuffer = %p\n",pBuffer);
            outputIndex = i;
            break;
        }
        else {
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: This is not a match\n",__LINE__);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: buff = %p\n",__LINE__,buff);
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: pBuffer = %p\n",__LINE__,pBuffer);
        }
    }

    if (inputIndex != -1) {
        if (pComponentPrivate->pInputBufferList->bufferOwner[inputIndex] == 1) {
            OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]->pBuffer, OMX_U8);
            buff = NULL;
        }

        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d: Freeing: %p IP Buf Header\n\n",__LINE__,
                        pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);

        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList->pBufHdr[inputIndex]);
        pComponentPrivate->pInputBufferList->pBufHdr[inputIndex] = NULL;
        pComponentPrivate->pInputBufferList->numBuffers--;

        if (pComponentPrivate->pInputBufferList->numBuffers <
            pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->nBufferCountMin) {
            pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated = OMX_FALSE;
        }

        OMX_PRSTATE2(pComponentPrivate->dbg, "CurrentState = %d\nbLoadedCommandPending = %d\nInput port bEnabled = %d\n",
               pComponentPrivate->curState,
               pComponentPrivate->bLoadedCommandPending,
               pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled);

        if(pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           !pComponentPrivate->reconfigInputPort &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
            OMX_ERROR4(pComponentPrivate->dbg, "OMX_EventError:: OMX_ErrorPortUnpopulated at line %d\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated,
                                                    nPortIndex,
                                                    NULL);
        }
    } else if (outputIndex != -1) {
        if (pComponentPrivate->pOutputBufferList->bBufferPending[outputIndex]) {
            pComponentPrivate->numPendingBuffers++;
        }
        if (pComponentPrivate->pOutputBufferList->bufferOwner[outputIndex] == 1) {
            OMX_MEMFREE_STRUCT_DSPALIGN(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pBuffer, OMX_U8);
            buff = NULL;
        }

        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d: Freeing: %p OP Buf Header\n\n",__LINE__,
                        pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]->pOutputPortPrivate);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex]);
        pComponentPrivate->pOutputBufferList->pBufHdr[outputIndex] = NULL;
        pComponentPrivate->pOutputBufferList->numBuffers--;

        OMX_PRBUFFER2(pComponentPrivate->dbg, "pComponentPrivate->pOutputBufferList->numBuffers = %lu\n",pComponentPrivate->pOutputBufferList->numBuffers);
        if (pComponentPrivate->pOutputBufferList->numBuffers <
            pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->nBufferCountMin) {

            pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated = OMX_FALSE;
        }

        OMX_PRCOMM2(pComponentPrivate->dbg, "CurrentState = %d\nbLoadedCommandPending = %d\nOutput port bEnabled = %d\nreconfig = %d",
               pComponentPrivate->curState,
               pComponentPrivate->bLoadedCommandPending,
               pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled,
               pComponentPrivate->reconfigOutputPort);
        if(pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled &&
           pComponentPrivate->bLoadedCommandPending == OMX_FALSE &&
           !pComponentPrivate->reconfigOutputPort &&
           (pComponentPrivate->curState == OMX_StateIdle ||
            pComponentPrivate->curState == OMX_StateExecuting ||
            pComponentPrivate->curState == OMX_StatePause)) {
            OMX_ERROR4(pComponentPrivate->dbg, "OMX_EventError:: OMX_ErrorPortUnpopulated at line %d\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorPortUnpopulated,
                                                    nPortIndex,
                                                    NULL);
        }
    }
    else {
        OMX_ERROR2(pComponentPrivate->dbg, "%d::Returning OMX_ErrorBadParameter\n",__LINE__);
        eError = OMX_ErrorBadParameter;
    }
        if ((!pComponentPrivate->pInputBufferList->numBuffers &&
            !pComponentPrivate->pOutputBufferList->numBuffers) &&
            pComponentPrivate->InIdle_goingtoloaded)
            {
                pComponentPrivate->InIdle_goingtoloaded = 0;
                pthread_mutex_lock(&pComponentPrivate->InIdle_mutex);
                pthread_cond_signal(&pComponentPrivate->InIdle_threshold);
                pthread_mutex_unlock(&pComponentPrivate->InIdle_mutex);
            }

        pComponentPrivate->bufAlloced = 0;

    if ((pComponentPrivate->bDisableCommandPending) &&
         (pComponentPrivate->pInputBufferList->numBuffers == 0))
    {
        OMX_PRCOMM2(pComponentPrivate->dbg, "calling command completed for input port disable\n");
        pComponentPrivate->bDisableCommandPending = 0;
        pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        INPUT_PORT_NUAUDIODEC,
                                                        NULL);
    }


    if ((pComponentPrivate->bDisableCommandPending) &&
         (pComponentPrivate->pOutputBufferList->numBuffers == 0))
    {
        OMX_PRCOMM2(pComponentPrivate->dbg, "calling command completed for output port disable\n");
        pComponentPrivate->bDisableCommandPending = 0;
        pComponentPrivate->cbInfo.EventHandler( pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandPortDisable,
                                                OUTPUT_PORT_NUAUDIODEC,
                                                NULL);
    }


    OMX_PRCOMM2(pComponentPrivate->dbg, "checking if port disable is pending\n");
    OMX_PRDSP2(pComponentPrivate->dbg, "::::disableCommandPending = %ld\n",pComponentPrivate->bDisableCommandPending);
    OMX_PRDSP2(pComponentPrivate->dbg, "::::disableCommandParam = %ld\n",pComponentPrivate->bDisableCommandParam);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "::::pOutputBufferList->numBuffers = %ld\n", pComponentPrivate->pOutputBufferList->numBuffers);
    OMX_PRBUFFER2(pComponentPrivate->dbg, "::::pInputBufferList->numBuffers = %ld\n", pComponentPrivate->pInputBufferList->numBuffers);

    return eError;
}


/* ================================================================================= * */
/**
* @fn UseBuffer() This function is called by application when app allocated the
* memory for the buffer and sends it to application for use of component.
*
* @param hComponent  Handle of this component.
*
* @param ppBufferHdr  Double pointer to the buffer header.
*
* @param nPortIndex  Input port or Output port
*
* @param pAppPrivate Application private data.
*
* @param nSizeBytes Size of the buffer that is to be allocated.
*
* @param pBuffer    Pointer to data buffer which was allocated by the
* application.
*
* @pre          None
*
* @post        None
*
* @return      OMX_ErrorNone = On Success
*              OMX_ErrorBadPortIndex = Bad port index from app
*/
/* ================================================================================ * */
static OMX_ERRORTYPE UseBuffer (
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer)
{
    LOG_AUDIO("audio_dbg:UseBuffer===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHeader;


    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

#ifdef _ERROR_PROPAGATION__
    if (pComponentPrivate->curState == OMX_StateInvalid){
        eError = OMX_ErrorInvalidState;
        OMX_ERROR4(pComponentPrivate->dbg, "%d::OMX_ErrorInvalidState from UseBuffer\n",__LINE__);
        goto EXIT;
    }
#endif

    pPortDef = ((NUAUDIODEC_COMPONENT_PRIVATE*)
                pComponentPrivate)->pPortDef[nPortIndex];
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: pPortDef = %p\n", __LINE__,pPortDef);
    OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: pPortDef->bEnabled = %d\n", __LINE__,pPortDef->bEnabled);

    NUAUDIODEC_OMX_CONF_CHECK_CMD(pPortDef, 1, 1);
    if (!pPortDef->bEnabled) {
        pComponentPrivate->AlloBuf_waitingsignal = 1;

        pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
        pthread_cond_wait(&pComponentPrivate->AlloBuf_threshold, &pComponentPrivate->AlloBuf_mutex);
        pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
    }
    if(!pPortDef->bEnabled) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorIncorrectStateOperation,
                              "Port is Disabled: OMX_ErrorIncorrectStateOperation");
    }

    if(pPortDef->bPopulated) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError,OMX_ErrorBadParameter,
                              "Bad Size or Port Disabled : OMX_ErrorBadParameter");
    }

    OMX_MALLOC_GENERIC(pBufferHeader, OMX_BUFFERHEADERTYPE);

    memset((pBufferHeader), 0x0, sizeof(OMX_BUFFERHEADERTYPE));
    if (nPortIndex == OUTPUT_PORT_NUAUDIODEC) {
        pBufferHeader->nInputPortIndex = -1;
        pBufferHeader->nOutputPortIndex = nPortIndex;

        pComponentPrivate->pOutputBufferList->pBufHdr[pComponentPrivate->pOutputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pOutputBufferList->bBufferPending[pComponentPrivate->pOutputBufferList->numBuffers] = 0;
        pComponentPrivate->pOutputBufferList->bufferOwner[pComponentPrivate->pOutputBufferList->numBuffers++] = 0;
        if (pComponentPrivate->pOutputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }
    else {
        pBufferHeader->nInputPortIndex = nPortIndex;
        pBufferHeader->nOutputPortIndex = -1;

        pComponentPrivate->pInputBufferList->pBufHdr[pComponentPrivate->pInputBufferList->numBuffers] = pBufferHeader;
        pComponentPrivate->pInputBufferList->bBufferPending[pComponentPrivate->pInputBufferList->numBuffers] = 0;
        pComponentPrivate->pInputBufferList->bufferOwner[pComponentPrivate->pInputBufferList->numBuffers++] = 0;
        if (pComponentPrivate->pInputBufferList->numBuffers == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;
        }
    }

    if((pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated == pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled)&&
       (pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated == pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled) &&
       (pComponentPrivate->InLoaded_readytoidle))
        {
            pComponentPrivate->InLoaded_readytoidle = 0;
            pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
            pthread_cond_signal(&pComponentPrivate->InLoaded_threshold);
            pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
        }

    pBufferHeader->pAppPrivate = pAppPrivate;
    pBufferHeader->pPlatformPrivate = pComponentPrivate;
    pBufferHeader->nAllocLen = nSizeBytes;
    pBufferHeader->nVersion.s.nVersionMajor = NUAUDIODEC_MAJOR_VER;
    pBufferHeader->nVersion.s.nVersionMinor = NUAUDIODEC_MINOR_VER;

    pComponentPrivate->nVersion = pBufferHeader->nVersion.nVersion;

    pBufferHeader->pBuffer = pBuffer;
    pBufferHeader->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    *ppBufferHdr = pBufferHeader;
    pComponentPrivate->bufAlloced = 1;
    OMX_PRBUFFER2(pComponentPrivate->dbg, "pBufferHeader = %p\n",pBufferHeader);

    if (pComponentPrivate->bEnableCommandPending && pPortDef->bPopulated) {
        SendCommand (pComponentPrivate->pHandle,
                     OMX_CommandPortEnable,
                     pComponentPrivate->bEnableCommandParam,NULL);
    }
 EXIT:
    return eError;
}

/* ================================================================================= */
/**
* @fn GetExtensionIndex() description for GetExtensionIndex
GetExtensionIndex().
Returns index for vendor specific settings.
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */
static OMX_ERRORTYPE GetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    LOG_AUDIO("audio_dbg:GetExtensionIndex===>>>function in for NUFRONT of NuAudioDecoder");
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (!(strcmp(cParameterName,"OMX.NUFRONT.index.config.nuaudiodecHeaderInfo"))) {
        *pIndexType = OMX_IndexCustomNuAudioDecHeaderInfoConfig;
        OMXDBG_PRINT(stderr, DSP, 2, 0, "OMX_IndexCustomNuAudioDecHeaderInfoConfig\n");
    }
    else if(!(strcmp(cParameterName,"OMX.NUFRONT.index.config.nuaudiodecstreamIDinfo"))){
        *pIndexType = OMX_IndexCustomNuAudioDecStreamIDConfig;
    }
    else if(!(strcmp(cParameterName,"OMX.NUFRONT.index.config.nuaudiodec.datapath"))) {
        *pIndexType = OMX_IndexCustomNuAudioDecDataPath;
    }
    else if(!(strcmp(cParameterName,"OMX.NUFRONT.NUAUDIO.Decode.Debug"))) {
    *pIndexType = OMX_IndexCustomDebug;
    }
    else {
        eError = OMX_ErrorBadParameter;
        OMXDBG_PRINT(stderr, ERROR, 4, 0, "%d::OMX_ErrorBadParameter from GetExtensionIndex\n",__LINE__);
    }

    return eError;
}

/* ================================================================================= */
/**
* @fn ComponentRoleEnum() description for ComponentRoleEnum()

Returns the role at the given index
*
*  @see         OMX_Core.h
*/
/* ================================================================================ */
static OMX_ERRORTYPE ComponentRoleEnum(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_U8 *cRole,
            OMX_IN OMX_U32 nIndex)
{
    LOG_AUDIO("audio_dbg:ComponentRoleEnum===>>>function in for NUFRONT of NuAudioDecoder");
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_PRINT1(pComponentPrivate->dbg, "%d :: pComponentPrivate = 0x%p\n",__LINE__, pComponentPrivate);
    if(nIndex == 0){
        if (cRole == NULL) {
            eError = OMX_ErrorBadParameter;
    }
    else {
            OMX_PRINT2(pComponentPrivate->dbg, "%d :: index=0\n",__LINE__);
            memcpy(cRole, &pComponentPrivate->componentRole->cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE);
            OMX_PRINT2(pComponentPrivate->dbg, "::::In ComponenetRoleEnum: cRole is set to %s\n",cRole);
    }
    }
    else {
        eError = OMX_ErrorNoMore;
    }
    return eError;
}


