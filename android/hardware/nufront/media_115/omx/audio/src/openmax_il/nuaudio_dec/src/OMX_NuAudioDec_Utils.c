/*
 *Copyright (C), 2009~2019, NUFRONT. Co., Ltd.
 *File name:     OMX_NuAudioDec_Utils.c
 *Author: Zhai Jianfeng Version: v1.0       Date: 2010-09-25
 *
 * This file implements OMX Component for NUAUDIO Decoder that
 * is fully compliant with the OMX Audio specification 1.0.
 *
*/
/* ------compilation control switches -------------------------*/
/****************************************************************
*  INCLUDE FILES
****************************************************************/
/* ----- system and platform files ----------------------------*/

#include <wchar.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <malloc.h>
#include <memory.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
/*------- Program Header Files -----------------------------------------------*/
#include "OMX_NuAudioDec_Utils.h"
#include <libavcodec/avcodec.h>
#include <utils/nufrontlog.h>
#define NOPTS_VALUE INT64_MAX

#ifndef NU_OUTPUT_SAMPLERATE
#define NU_OUTPUT_SAMPLERATE 48000
#endif

/* ================================================================================= * */
/**
* @fn NuAudioDec_StartCompThread() starts the component thread. This is internal
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
OMX_ERRORTYPE NuAudioDec_StartCompThread(OMX_HANDLETYPE pComponent)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate =
        (NUAUDIODEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    int nRet = 0;

    pComponentPrivate->app_nBuf = 0;
    pComponentPrivate->num_Op_Issued = 0;
    pComponentPrivate->num_Sent_Ip_Buff = 0;
    pComponentPrivate->num_Reclaimed_Op_Buff = 0;
    pComponentPrivate->bIsEOFSent = 0;
    pComponentPrivate->first_output_buf_rcv = 0;

    nRet = pipe (pComponentPrivate->dataPipe);
    if (0 != nRet) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,"Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->cmdPipe);
    if (0 != nRet) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,"Pipe Creation Failed");
    }

    nRet = pipe (pComponentPrivate->cmdDataPipe);
    if (0 != nRet) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,"Pipe Creation Failed");
    }


    nRet = pthread_create (&(pComponentPrivate->ComponentThread), NULL, NUAUDIODEC_ComponentThread, pComponentPrivate);
    if ((0 != nRet) || (!pComponentPrivate->ComponentThread)) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError, OMX_ErrorInsufficientResources,"Thread Creation Failed");
    }

    pComponentPrivate->bCompThreadStarted = 1;

 EXIT:
    return eError;
}

/* ================================================================================= * */
/**
* @fn NUAUDIODEC_FreeCompResources() function frees the component resources.
*
* @param pComponent This is the component handle.
*
* @pre          None
*
* @post         None
*
*  @return      OMX_ErrorNone = Successful Inirialization of the component\n
*               OMX_ErrorHardware = Hardware error has occured.
*
*  @see         None
*/
/* ================================================================================ * */

OMX_ERRORTYPE NUAUDIODEC_FreeCompResources(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 nIpBuf = 0, nOpBuf = 0;
    int nRet=0;

    if (pComponentPrivate->bPortDefsAllocated) {
        nIpBuf = pComponentPrivate->pInputBufferList->numBuffers;
        nOpBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    }

    nRet = close (pComponentPrivate->dataPipe[0]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->dataPipe[1]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->cmdPipe[0]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->cmdPipe[1]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->cmdDataPipe[0]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    nRet = close (pComponentPrivate->cmdDataPipe[1]);
    if (0 != nRet && OMX_ErrorNone == eError) {
        eError = OMX_ErrorHardware;
    }

    if (pComponentPrivate->bPortDefsAllocated) {
        OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->nuaudioParams);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pcmParams);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]->pPortFormat);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]->pPortFormat);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[INPUT_PORT_NUAUDIODEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pCompPort[OUTPUT_PORT_NUAUDIODEC]);
        OMX_MEMFREE_STRUCT(pComponentPrivate->sPortParam);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pPriorityMgmt);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pInputBufferList);
        OMX_MEMFREE_STRUCT(pComponentPrivate->pOutputBufferList);
        OMX_MEMFREE_STRUCT(pComponentPrivate->componentRole);
    }

    if (NULL != pComponentPrivate->spCodecCtx) {
        ZJFLOGD("will free spCodecCtx");
        av_free(pComponentPrivate->spCodecCtx);
        pComponentPrivate->spCodecCtx == NULL;
    }
    pComponentPrivate->bPortDefsAllocated = 0;

    pthread_mutex_destroy(&pComponentPrivate->InLoaded_mutex);
    pthread_cond_destroy(&pComponentPrivate->InLoaded_threshold);

    pthread_mutex_destroy(&pComponentPrivate->InIdle_mutex);
    pthread_cond_destroy(&pComponentPrivate->InIdle_threshold);

    pthread_mutex_destroy(&pComponentPrivate->AlloBuf_mutex);
    pthread_cond_destroy(&pComponentPrivate->AlloBuf_threshold);

    pthread_mutex_destroy(&pComponentPrivate->codecStop_mutex);
    pthread_cond_destroy(&pComponentPrivate->codecStop_threshold);

    pthread_mutex_destroy(&pComponentPrivate->codecFlush_mutex);
    pthread_cond_destroy(&pComponentPrivate->codecFlush_threshold);
    /*a@nufront start: add to info compthread*/
    pthread_mutex_destroy(&pComponentPrivate->CodecInit_mutex);
    pthread_cond_destroy(&pComponentPrivate->CodecInit_threshold);
    /*a@nufront end*/
    return eError;
}


/* ================================================================================= * */
/**
* @fn NUAUDIODEC_HandleCommand() function handles the command sent by the application.
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

OMX_U32 NUAUDIODEC_HandleCommand (NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_U32 i;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *) pComponentPrivate->pHandle;
    OMX_COMMANDTYPE command;
    OMX_STATETYPE commandedState;
    OMX_U32 commandData;
    OMX_U32 ret = 0;
    OMX_U16 arr[10];
    OMX_U32 aParam[3] = {0};
    int inputPortFlag = 0;
    int outputPortFlag = 0;
    char *pArgs = "damedesuStr";



    ret = read (pComponentPrivate->cmdPipe[0], &command, sizeof (command));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError,
                                                eError,
                                                OMX_NUFRONT_ErrorSevere,
                                                NULL);
        goto EXIT;
    }

    ret = read(pComponentPrivate->cmdDataPipe[0], &commandData, sizeof (commandData));
    if (ret == -1) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Reading from the Data pipe\n", __LINE__);
        eError = OMX_ErrorHardware;
        pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                pHandle->pApplicationPrivate,
                                                OMX_EventError,
                                                eError,
                                                OMX_NUFRONT_ErrorSevere,
                                                NULL);
        goto EXIT;
    }

    if (command == OMX_CommandStateSet) {
        commandedState = (OMX_STATETYPE)commandData;
        if (pComponentPrivate->curState == commandedState) {
            pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                     pHandle->pApplicationPrivate,
                                                     OMX_EventError,
                                                     OMX_ErrorSameState,
                                                     OMX_NUFRONT_ErrorMinor,
                                                     NULL);

        }
        else {
            switch(commandedState) {
            case OMX_StateIdle:
                LOG_AUDIO("audio_dbg:NUAUDIODEC_HandleCommand===>>>%d: HandleCommand: Cmd Idle \n",__LINE__);

                if (pComponentPrivate->curState == OMX_StateLoaded || pComponentPrivate->curState == OMX_StateWaitForResources) {
                    char *p = "damedesuStr";
                    if (pComponentPrivate->dasfmode == 1) {
                        pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled= OMX_FALSE;
                        pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated= OMX_FALSE;
                        if(pComponentPrivate->streamID == 0) {
                            OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                            OMX_ERROR4(pComponentPrivate->dbg, ":: Error = OMX_ErrorInsufficientResources\n");
                            OMX_ERROR4(pComponentPrivate->dbg, "**************************************\n");
                            eError = OMX_ErrorInsufficientResources;
                            pComponentPrivate->curState = OMX_StateInvalid;
                            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                                   pHandle->pApplicationPrivate,
                                                                   OMX_EventError,
                                                                   eError,
                                                                   OMX_NUFRONT_ErrorMajor,
                                                                   "AM: No Stream ID Available");
                            goto EXIT;
                        }
                    }

                    if (pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated &&
                        pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled)  {
                        inputPortFlag = 1;
                    }

                    if (!pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated &&
                        !pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled) {
                        inputPortFlag = 1;
                    }

                    if (pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated &&
                        pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled) {
                        outputPortFlag = 1;
                    }

                    if (!pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated &&
                        !pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled) {
                        outputPortFlag = 1;
                    }

                    if (!(inputPortFlag && outputPortFlag)) {
                        pComponentPrivate->InLoaded_readytoidle = 1;
                        pthread_mutex_lock(&pComponentPrivate->InLoaded_mutex);
                        pthread_cond_wait(&pComponentPrivate->InLoaded_threshold, &pComponentPrivate->InLoaded_mutex);
                        pthread_mutex_unlock(&pComponentPrivate->InLoaded_mutex);
                    }

                                        pComponentPrivate->curState = OMX_StateIdle;
                                            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandStateSet,
                                                           pComponentPrivate->curState,
                                                           NULL);
                } else if (pComponentPrivate->curState == OMX_StateExecuting) {
                          LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>cmd is OMX_StateIdle, pComponentPrivate->curState == OMX_StateExecuting.");

                            pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                            pComponentPrivate->curState = OMX_StateIdle;
                        pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                                   pHandle->pApplicationPrivate,
                                                                   OMX_EventCmdComplete,
                                                                   OMX_CommandStateSet,
                                                                   pComponentPrivate->curState,
                                                                   NULL);
                            LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>pos 5.");
                } else if (pComponentPrivate->curState == OMX_StatePause) {
                            char *pArgs = "damedesuStr";
                            pComponentPrivate->bNoIdleOnStop = OMX_TRUE;
                        if (pComponentPrivate->codecStop_waitingsignal == 0){
                        pthread_mutex_lock(&pComponentPrivate->codecStop_mutex);
                    }
                    pComponentPrivate->curState = OMX_StateIdle;
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
                } else {
                    OMX_ERROR4(pComponentPrivate->dbg, "%d: Comp: Sending ErrorNotification: Invalid State\n",__LINE__);
                    pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorIncorrectStateTransition,
                                                           OMX_NUFRONT_ErrorMinor,
                                                           "Invalid State Error");
                }
                break;

            case OMX_StateExecuting:

                if (pComponentPrivate->curState == OMX_StateIdle) {
                  if(!pComponentPrivate->bConfigData){
/*  if running under Android (file mode), these values are not available during this state transition.
    We will have to set the codec config parameters after receiving the first buffer that carries
    the config data */
                    /*
                    char *pArgs = "damedesuStr";
                    OMX_U32 pValues[4];
                    OMX_U32 pValues1[4];

                    if(pComponentPrivate->dasfmode == 1) {
                        if(pComponentPrivate->nuaudioParams->nChannels == OMX_AUDIO_ChannelModeMono) {
                                OMX_PRINT2(pComponentPrivate->dbg, "MONO MODE\n");
                        }

                    }
                    */
                    }
                } else if (pComponentPrivate->curState == OMX_StatePause) {
                    char *pArgs = "damedesuStr";

                    for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d pComponentPrivate->pInputBufHdrPending[%lu] = %d\n",__LINE__,i,
                                      pComponentPrivate->pInputBufHdrPending[i] != NULL);
                        if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                            NUAUDIODEC_SetPending(pComponentPrivate,
                                              pComponentPrivate->pInputBufHdrPending[i],
                                              OMX_DirInput,
                                              __LINE__);
                        }
                    }
                    pComponentPrivate->nNumInputBufPending = 0;
                    for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                        if (pComponentPrivate->pOutputBufHdrPending[i] != NULL) {
                                NUAUDIODEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                        }
                    }
                    pComponentPrivate->nNumOutputBufPending = 0;
                } else {
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventError,
                                                             OMX_ErrorIncorrectStateTransition,
                                                             OMX_NUFRONT_ErrorMinor,
                                                             "Invalid State");
                    OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error: Invalid State Given by Application\n",__LINE__);
                    goto EXIT;
                }

                pComponentPrivate->curState = OMX_StateExecuting;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandStateSet,
                                                        pComponentPrivate->curState,
                                                        NULL);
                break;

            case OMX_StateLoaded:
                if (pComponentPrivate->curState == OMX_StateWaitForResources ){
                    pComponentPrivate->curState = OMX_StateLoaded;
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventCmdComplete,
                                                             OMX_CommandStateSet,
                                                             pComponentPrivate->curState,
                                                             NULL);
                    break;
                }

                if (pComponentPrivate->curState != OMX_StateIdle) {
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventError,
                                                             OMX_ErrorIncorrectStateTransition,
                                                             OMX_NUFRONT_ErrorMinor,
                                                             "Incorrect State Transition");
                    goto EXIT;
                }


                if (pComponentPrivate->pInputBufferList->numBuffers || pComponentPrivate->pOutputBufferList->numBuffers) {
                    pComponentPrivate->InIdle_goingtoloaded = 1;
                    pComponentPrivate->bLoadedCommandPending = OMX_FALSE;
                }

                pComponentPrivate->curState = OMX_StateLoaded;
                eError = EXIT_COMPONENT_THRD;
                pComponentPrivate->bInitParamsInitialized = 0;
                goto EXIT;
                break;

            case OMX_StatePause:
                OMX_PRSTATE2(pComponentPrivate->dbg, "%d: HandleCommand: Cmd Pause: Cur State = %d\n",__LINE__,
                              pComponentPrivate->curState);

                if ((pComponentPrivate->curState != OMX_StateExecuting) &&
                    (pComponentPrivate->curState != OMX_StateIdle)) {
                    pComponentPrivate->cbInfo.EventHandler ( pHandle,
                                                             pHandle->pApplicationPrivate,
                                                             OMX_EventError,
                                                             OMX_ErrorIncorrectStateTransition,
                                                             OMX_NUFRONT_ErrorMinor,
                                                             "Incorrect State Transition");
                    goto EXIT;
                }
                break;

            case OMX_StateWaitForResources:
                if (pComponentPrivate->curState == OMX_StateLoaded) {
                    pComponentPrivate->curState = OMX_StateWaitForResources;
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandStateSet,
                                                            pComponentPrivate->curState,
                                                            NULL);
                }
                else {
                    pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventError,
                                                            OMX_ErrorIncorrectStateTransition,
                                                            OMX_NUFRONT_ErrorMinor,
                                                            NULL);
                   OMX_ERROR4(pComponentPrivate->dbg, "%d :: state transition error\n",__LINE__);
                }
                break;

            case OMX_StateInvalid:
                if (pComponentPrivate->curState != OMX_StateWaitForResources &&
                    pComponentPrivate->curState != OMX_StateInvalid &&
                    pComponentPrivate->curState != OMX_StateLoaded) {

                }

                pComponentPrivate->curState = OMX_StateInvalid;
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        OMX_ErrorInvalidState,
                                                        OMX_NUFRONT_ErrorSevere,
                                                        NULL);
                OMX_ERROR4(pComponentPrivate->dbg, "%d :: Component: Invalid State\n",__LINE__);

                NUAUDIODEC_Ffmpeg_Deinit(pHandle);

                break;

            case OMX_StateMax:
                break;
            } /* End of Switch */
        }
    }else if (command == OMX_CommandMarkBuffer) {
        if(!pComponentPrivate->pMarkBuf) {
            pComponentPrivate->pMarkBuf = (OMX_MARKTYPE *)(commandData);
        }
    } else if (command == OMX_CommandPortDisable) {
        if (!pComponentPrivate->bDisableCommandPending) {
            if(commandData == 0x0){
                for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
                    if (NUAUDIODEC_IsPending(pComponentPrivate,pComponentPrivate->pInputBufferList->pBufHdr[i],OMX_DirInput)) {
                        pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->pInputBufferList->pBufHdr[i]);
                        pComponentPrivate->nEmptyBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);
                    }
                }
                pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled = OMX_FALSE;
            }
            if(commandData == -1){
                pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled = OMX_FALSE;
            }
            if(commandData == 0x1 || commandData == -1){
                pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled = OMX_FALSE;
            }
        }
        if(commandData == 0x0) {
            if(!pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated){

                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        INPUT_PORT_NUAUDIODEC,
                                                        NULL);
                pComponentPrivate->bDisableCommandPending = 0;

            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == 0x1) {
            if (!pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated){
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        OUTPUT_PORT_NUAUDIODEC,
                                                        NULL);

                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }

        if(commandData == -1) {
            if (!pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated &&
                !pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated){

                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        INPUT_PORT_NUAUDIODEC,
                                                        NULL);

                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                        pHandle->pApplicationPrivate,
                                                        OMX_EventCmdComplete,
                                                        OMX_CommandPortDisable,
                                                        OUTPUT_PORT_NUAUDIODEC,
                                                        NULL);
                pComponentPrivate->bDisableCommandPending = 0;
            }
            else {
                pComponentPrivate->bDisableCommandPending = 1;
                pComponentPrivate->bDisableCommandParam = commandData;
            }
        }
    }
    else if (command == OMX_CommandPortEnable) {
        if(!pComponentPrivate->bEnableCommandPending) {
            if(commandData == 0x0 || commandData == -1){
                pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bEnabled = OMX_TRUE;
                if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                }
            }
            if(commandData == 0x1 || commandData == -1){
                char *pArgs = "damedesuStr";

                pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bEnabled = OMX_TRUE;
            }
        }

        if(commandData == 0x0){
            if (pComponentPrivate->curState == OMX_StateLoaded ||
                pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated) {
                                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                pHandle->pApplicationPrivate,
                                OMX_EventCmdComplete,
                                OMX_CommandPortEnable,
                                INPUT_PORT_NUAUDIODEC,
                                NULL);
                pComponentPrivate->bEnableCommandPending = 0;
                pComponentPrivate->reconfigInputPort = 0;

                if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
                    pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
                }

                /* Needed for port reconfiguration */

                for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d pComponentPrivate->pInputBufHdrPending[%lu] = %d\n",__LINE__,i,
                                      pComponentPrivate->pInputBufHdrPending[i] != NULL);
                        if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                                NUAUDIODEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);
                        }
                }
                pComponentPrivate->nNumInputBufPending = 0;
            }
            else {
                pComponentPrivate->bEnableCommandPending = 1;
                pComponentPrivate->bEnableCommandParam = commandData;
            }
        }
        else if(commandData == 0x1) {
            if ((pComponentPrivate->curState == OMX_StateLoaded) ||
                            (pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated)){
                OMX_PRCOMM2(pComponentPrivate->dbg, "Command port enable completed\n\n");
                pComponentPrivate->cbInfo.EventHandler( pHandle,
                                          pHandle->pApplicationPrivate,
                                          OMX_EventCmdComplete,
                                          OMX_CommandPortEnable,
                                          OUTPUT_PORT_NUAUDIODEC,
                                          NULL);
               if(pComponentPrivate->AlloBuf_waitingsignal){
                    pComponentPrivate->AlloBuf_waitingsignal = 0;
                    pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
                    pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                    pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
               }
               /* Needed for port reconfiguration */

               for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                   if (pComponentPrivate->pOutputBufHdrPending[i] != NULL) {
                       NUAUDIODEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                       if(eError != OMX_ErrorNone) {
                           OMX_ERROR4(pComponentPrivate->dbg, ": Error Occurred in LCML QueueBuffer for input\n");
                           pComponentPrivate->curState = OMX_StateInvalid;
                           pComponentPrivate->cbInfo.EventHandler( pHandle,
                                                                    pHandle->pApplicationPrivate,
                                                                    OMX_EventError,
                                                                    eError,
                                                                    OMX_NUFRONT_ErrorSevere,
                                                                    NULL);
                           goto EXIT;
                       }
                   }
               }
               pComponentPrivate->nNumOutputBufPending = 0;
               pComponentPrivate->bEnableCommandPending = 0;
               pComponentPrivate->reconfigOutputPort = 0;
           }
           else {
               pComponentPrivate->bEnableCommandPending = 1;
               pComponentPrivate->bEnableCommandParam = commandData;
               }
           }
           else if(commandData == -1) {
               if (pComponentPrivate->curState == OMX_StateLoaded ||
                    (pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->bPopulated &&
                     pComponentPrivate->pPortDef[OUTPUT_PORT_NUAUDIODEC]->bPopulated)){
                   pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandPortEnable,
                                                            INPUT_PORT_NUAUDIODEC,
                                                            NULL);
                   pComponentPrivate->reconfigInputPort = 0;
                   pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                            pHandle->pApplicationPrivate,
                                                            OMX_EventCmdComplete,
                                                            OMX_CommandPortEnable,
                                                            OUTPUT_PORT_NUAUDIODEC,
                                                            NULL);

                   if(pComponentPrivate->AlloBuf_waitingsignal){
                       pComponentPrivate->AlloBuf_waitingsignal = 0;
                       pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
                       pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
                       pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
                   }
                   pComponentPrivate->reconfigOutputPort = 0;
                   pComponentPrivate->bEnableCommandPending = 0;

                   for (i=0; i < pComponentPrivate->nNumInputBufPending; i++) {
                       if (pComponentPrivate->pInputBufHdrPending[i] != NULL) {
                           NUAUDIODEC_SetPending(pComponentPrivate,pComponentPrivate->pInputBufHdrPending[i],OMX_DirInput,__LINE__);
                       }
                   }
                   pComponentPrivate->nNumInputBufPending = 0;
                   for (i=0; i < pComponentPrivate->nNumOutputBufPending; i++) {
                       if (pComponentPrivate->pOutputBufHdrPending[i] != NULL) {
                           NUAUDIODEC_SetPending(pComponentPrivate,pComponentPrivate->pOutputBufHdrPending[i],OMX_DirOutput,__LINE__);
                       }
                   }
                   pComponentPrivate->nNumOutputBufPending = 0;
               }
               else {
                   pComponentPrivate->bEnableCommandPending = 1;
                   pComponentPrivate->bEnableCommandParam = commandData;
               }
           }

           pthread_mutex_lock(&pComponentPrivate->AlloBuf_mutex);
           pthread_cond_signal(&pComponentPrivate->AlloBuf_threshold);
           pthread_mutex_unlock(&pComponentPrivate->AlloBuf_mutex);
    }else if (command == OMX_CommandFlush) {
        OMX_BUFFERHEADERTYPE * pBufHeader;
        int i, ret = 0;
        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>command == OMX_CommandFlush, commandData is 0x%x", commandData);
        OMX_ERRORTYPE eError = OMX_ErrorNone;
        OMX_DIRTYPE eDir;

        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>command == OMX_CommandFlush, before read dataPipe[0] loop.");
        while(pComponentPrivate->nUnhandledEmptyThisBuffers + pComponentPrivate->nUnhandledFillThisBuffers
                     > pComponentPrivate->nHandledFillThisBuffers + pComponentPrivate->nHandledEmptyThisBuffers){
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));
            if(ret < 0)
                continue;
            pBufHeader->pPlatformPrivate  = pComponentPrivate;
            eError = NUAUDIODEC_GetBufferDirection(pBufHeader, &eDir);
            LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>command == OMX_CommandFlush, after GetBUfferDirection.");

            if (eError != OMX_ErrorNone) {
                LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>command == OMX_CommandFlush, NUAUDIODEC_GetBufferDirection error.");
            }
            if (eDir == OMX_DirInput) {
                OMX_Buf_Q_Put(pComponentPrivate->spInputBufQ, pBufHeader);
                pComponentPrivate->nHandledEmptyThisBuffers++;
            }else if(eDir == OMX_DirOutput){
                OMX_Buf_Q_Put(pComponentPrivate->spOutputBufQ, pBufHeader);
                pComponentPrivate->nHandledFillThisBuffers++;
            }
        }
        if(commandData == 0x0 || commandData == -1) {
            if(pComponentPrivate->spInputBufQ->pCurBufHeader != NULL) {
                LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>EmptyBufferDone for pCurBufHeader.");
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->spInputBufQ->pCurBufHeader);
                pComponentPrivate->spInputBufQ->pCurBufHeader =NULL;
                pComponentPrivate->spInputBufQ->BufFlag = 0;
                pComponentPrivate->spInputBufQ->Offset = 0;
                pComponentPrivate->nEmptyBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
            }

            i = 0;
            while(i < pComponentPrivate->spInputBufQ->BufCount) {
                LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>EmptyBufferDone for BufCount is %d.", pComponentPrivate->spInputBufQ->BufCount);
                OMX_Buf_Q_Get(pComponentPrivate->spInputBufQ, &(pBufHeader));
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                           pComponentPrivate->pHandle->pApplicationPrivate,
                           pBufHeader);
                pComponentPrivate->spInputBufQ->pCurBufHeader =NULL;
                pComponentPrivate->spInputBufQ->BufFlag = 0;
                pComponentPrivate->spInputBufQ->Offset = 0;
                pComponentPrivate->nEmptyBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
            }

            avcodec_flush_buffers(pComponentPrivate->spCodecCtx);

            if (pComponentPrivate->spResample) {
                audio_resample_close(pComponentPrivate->spResample);
                pComponentPrivate->spResample = NULL;
                pComponentPrivate->spResample = av_audio_resample_init(pComponentPrivate->pcmParams->nChannels,
                                                               pComponentPrivate->spCodecCtx->channels,
                                                               pComponentPrivate->pcmParams->nSamplingRate,
                                                               pComponentPrivate->spCodecCtx->sample_rate,
                                                               SAMPLE_FMT_S16, pComponentPrivate->spCodecCtx->sample_fmt,
                                                               16, 10, 0, 0.8);
            }

            pComponentPrivate->nNumInputBufPending=0;
            pComponentPrivate->cbInfo.EventHandler(pHandle,
                                  pHandle->pApplicationPrivate,
                                  OMX_EventCmdComplete,
                                  OMX_CommandFlush,
                                  OMX_DirInput,
                                  NULL);
        }
        if(commandData == 0x1 || commandData == -1){

            while (pComponentPrivate->spOutputBufQ->BufCount) {
                OMX_Buf_Q_Get(pComponentPrivate->spOutputBufQ, &(pBufHeader));
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                 pComponentPrivate->pHandle->pApplicationPrivate,
                                 pBufHeader);
                pComponentPrivate->nFillBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
            }

            avcodec_flush_buffers(pComponentPrivate->spCodecCtx);
            pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_CommandFlush,
                                                           OMX_DirOutput,
                                                           NULL);
        }
    }

 EXIT:
    /* @NOTE: EXIT_COMPONENT_THRD is not REALLY an error, but a signal to ComponentThread.c */
    LOG_AUDIO("audio_debug:NUAUDIODEC_HandleCommand===>>>function out, eError is 0x%x", eError);
    return eError;
}

/* ================================================================================= * */
/**
* @fn NUAUDIODEC_HandleDataBuf_FromApp() function handles the input and output buffers
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

OMX_ERRORTYPE NUAUDIODEC_HandleDataBuf_FromApp(OMX_BUFFERHEADERTYPE* pBufHeader,
                                    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_DIRTYPE eDir;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
    char *pArgs = "damedesuStr";
    OMX_U32 pValues[4];
    OMX_U32 pValues1[4];
    int iObjectType = 0;
    int iSampleRateIndex = 0;
    OMX_U32 nBitPosition = 0;
    OMX_U8* pHeaderStream = (OMX_U8*)pBufHeader->pBuffer;
    OMX_U32 i = 0;

    LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>function in.");

    pBufHeader->pPlatformPrivate  = pComponentPrivate;
    eError = NUAUDIODEC_GetBufferDirection(pBufHeader, &eDir);

    if (eError != OMX_ErrorNone) {
        goto EXIT;
    }
    if (eDir == OMX_DirInput) {
        pComponentPrivate->nHandledEmptyThisBuffers++;

        if (pComponentPrivate->curState == OMX_StateIdle){
                pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           //pComponentPrivate->pInputBufferList->pBufHdr[0]
                                                            pBufHeader
                                                           );
                pComponentPrivate->nEmptyBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);
            goto EXIT;
        }
        if (pBufHeader->nFilledLen > 0 || (pBufHeader->nFlags & OMX_BUFFERFLAG_EOS)) {
            if(pComponentPrivate->SendAfterEOS == 1){
                pComponentPrivate->SendAfterEOS = 0;
                OMX_PRINT2(pComponentPrivate->dbg, "sample rate %ld\n",pComponentPrivate->pcmParams->nSamplingRate);
            }
            if(pBufHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                OMX_PRBUFFER2(pComponentPrivate->dbg, "%d :: bLastBuffer Is Set Here....\n",__LINE__);
                pComponentPrivate->bIsEOFSent = 1;
                pComponentPrivate->SendAfterEOS = 1;
                pBufHeader->nFlags = 0;
            }
            pComponentPrivate->arrBufIndex[pComponentPrivate->IpBufindex] = pBufHeader->nTimeStamp;
            /*Store tick count information*/
            pComponentPrivate->arrBufIndexTick[pComponentPrivate->IpBufindex] = pBufHeader->nTickCount;
            pComponentPrivate->IpBufindex++;
            pComponentPrivate->IpBufindex %= pComponentPrivate->pPortDef[INPUT_PORT_NUAUDIODEC]->nBufferCountActual;
            if(!pComponentPrivate->framemode){
                if(pComponentPrivate->first_buff == 0){
                    pComponentPrivate->first_TS = pBufHeader->nTimeStamp;
                    OMX_PRBUFFER2(pComponentPrivate->dbg, "in ts-%ld\n",pBufHeader->nTimeStamp);
                    pComponentPrivate->first_buff = 1;
                }
            }
            OMX_PRBUFFER2(pComponentPrivate->dbg, "%d Comp:: Sending Filled Input buffer = %p, %p\
                               to LCML\n", __LINE__,pBufHeader,pBufHeader->pBuffer);
            if (pComponentPrivate->curState == OMX_StateExecuting) {
                LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>OMX_DirInput pos 23.");
                if (!NUAUDIODEC_IsPending(pComponentPrivate,pBufHeader,OMX_DirInput)) {
                    int outputLen, retLen = 0;
                    outputLen = FFMPEG_MAX_OUTPUT_LEN;
                    int16_t * pTmpOutput;

                    LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>pComponentPrivate->spOutputBufQ->BufCount is %d.\n",
                                           pComponentPrivate->spOutputBufQ->BufCount);
                    if ( pComponentPrivate->spOutputBufQ->BufCount > 0 && pComponentPrivate->spInputBufQ->BufFlag == 0) {

                        /*current input pBufHeader is valid to handle ,
                        *following operation put it into spInputBufQ
                        *and set it as Current buffer to be decoded
                        */
                        OMX_Buf_Q_Cur_Set(pComponentPrivate->spInputBufQ, pBufHeader);

                        OMX_BUFFERHEADERTYPE * pTmpOutputBufHeader;

                        /*get output buffer from spOutputBufQ to fill
                        *data from cur input data decoding
                        */
                        OMX_Buf_Q_Get(pComponentPrivate->spOutputBufQ,  &pTmpOutputBufHeader);
                        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>after OMX_Buf_Q_Get, pComponentPrivate->spOutputBufQ->BufCount is %d.\n",
                                           pComponentPrivate->spOutputBufQ->BufCount);
                        if (NULL != pComponentPrivate->spResample){
                            /*
                            *if output pcm of cur format's decoding output is not 16bps,
                            *now will set a tmp buf to store decoding output for coveriation
                            */
                            LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>before decode for sample_fmt != SAMPLE_FMT_S16");
                            if (pComponentPrivate->pOutputBuf != NULL){
                                LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>pComponentPrivate->pOutputBuf !=NULL");
                                pTmpOutput = pComponentPrivate->pOutputBuf;
                                 memset(pTmpOutput,0,FFMPEG_MAX_OUTPUT_LEN);
                            }else{
                                LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>pComponentPrivate->pOutputBuf ==NULL");
                                eError = OMX_ErrorHardware;
                                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                                OMX_EventError,
                                                                eError,
                                                                OMX_NUFRONT_ErrorSevere,
                                                                NULL);
                                goto EXIT;
                            }
                        }else{
                            LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>before decode for sample_fmt == SAMPLE_FMT_S16");
                            pTmpOutput = (pTmpOutputBufHeader->pBuffer);
                        }

                        /*decode input data*/
                        AVPacket pkt;
                        av_init_packet(&pkt);
                        if (pComponentPrivate->spInputBufQ->pCurBufHeader->pBuffer) {
                            pkt.data = (const uint8_t *)( pComponentPrivate->spInputBufQ->pCurBufHeader->pBuffer) + pComponentPrivate->spInputBufQ->Offset;
                            pkt.size = (int)pBufHeader->nFilledLen - pComponentPrivate->spInputBufQ->Offset;
                        } else {
                            pkt.data = NULL;
                            pkt.size = 0;
                        }
                        if (pkt.data && (!strncmp(pkt.data, "silencepkt", 10) || !strncmp(pkt.data, "seek", 4))) {
                            int64_t tmp_duration = 0;
                            LOG_AUDIO("audio_dbg:NUAUDIODEC_HandleDataBuf_FromApp===>>>silence packet");
                            retLen = pkt.size;
                            if (!strncmp(pkt.data, "silencepkt", 10)) {
                                sscanf(pkt.data, "silencepkt:%lld", &tmp_duration);
                                if (0 == pComponentPrivate->nSampleSize) {
                                    pComponentPrivate->nSampleSize = 2;
                                }
                                outputLen = (int) ((tmp_duration
                                        * pComponentPrivate->spCodecCtx->sample_rate
                                        * pComponentPrivate->spCodecCtx->channels
                                        * pComponentPrivate->nSampleSize) / 1E6);
                                outputLen = outputLen - outputLen % 2;
                            /*outputLen = pComponentPrivate->mOutputLen;*/
                                memset(pTmpOutput, 0, outputLen);
                                GSLOGD("silence packet len:%d",outputLen);
                            } else {
                                GSLOGD("seek packet");
                                outputLen = 0;
                            }
                        } else {
                            retLen =  avcodec_decode_audio3(pComponentPrivate->spCodecCtx,
                                      (int16_t *)(pTmpOutput),
                                      (int*)&outputLen,
                                       &pkt);
                            if (outputLen > 0 && pComponentPrivate->mOutputLen == 0) {
                                LOG_AUDIO("audio_dbg:NUAUDIODEC_HandleDataBuf_FromApp===>>>set pComponentPrivate->mOutputLen for silence packet");
                                pComponentPrivate->mOutputLen = outputLen;
                            }

                        }

                        if (retLen < 0) {
                            AVCodec *pCodec;
                            avcodec_close(pComponentPrivate->spCodecCtx);
                            pCodec = avcodec_find_decoder(pComponentPrivate->spCodecCtx->codec_id);
                            if (NULL == pCodec){
                                return OMX_ErrorInsufficientResources; /* Codec not found*/
                            }

                            if(avcodec_open(pComponentPrivate->spCodecCtx, pCodec) < 0) {
                                return OMX_ErrorInsufficientResources;
                            }
                        }

                        if (retLen >= 0){
                            LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>modify parameter after decode audio, outputLen is %d.", outputLen);
                            pComponentPrivate->spInputBufQ->Offset +=retLen;

                            if (outputLen > 0
                                    && NULL != pComponentPrivate->spResample) {
                                outputLen = audio_resample(pComponentPrivate->spResample,
                                      (short *)pTmpOutputBufHeader->pBuffer,
                                    (short *) pTmpOutput,
                                    outputLen / (pComponentPrivate->spCodecCtx->channels * pComponentPrivate->nSampleSize));
                                outputLen = outputLen * (pComponentPrivate->pcmParams->nChannels * (pComponentPrivate->pcmParams->nBitPerSample / 8));
                            }

                            pTmpOutputBufHeader->nFilledLen = outputLen;
                        }else {
                            pComponentPrivate->spInputBufQ->Offset = pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen;
                            pTmpOutputBufHeader->nFilledLen = 0;
                        }
                        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>pComponentPrivate->bIsEOFSent is %d, pComponentPrivate->SendAfterEOS is %d", pComponentPrivate->bIsEOFSent, pComponentPrivate->SendAfterEOS);
                        if (NOPTS_VALUE == pComponentPrivate->spInputBufQ->pCurBufHeader->nTimeStamp
                                     && pComponentPrivate->pcmParams->nSamplingRate > 0){
                            int tmpChannelCount = pComponentPrivate->spCodecCtx->channels > 2?2:pComponentPrivate->spCodecCtx->channels;
                            pTmpOutputBufHeader->nTimeStamp = pComponentPrivate->nLastDTS
                                    + ((pTmpOutputBufHeader->nFilledLen * 1000000.0)/(pComponentPrivate->pcmParams->nSamplingRate * 2.0 * tmpChannelCount));
                            pComponentPrivate->nLastDTS = pTmpOutputBufHeader->nTimeStamp;
                        }else {
                            pTmpOutputBufHeader->nTimeStamp = pComponentPrivate->spInputBufQ->pCurBufHeader->nTimeStamp;

                            if (pComponentPrivate->nAnchor != pComponentPrivate->spInputBufQ->pCurBufHeader->nTimeStamp
                                    && pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen > 0) {
                                pComponentPrivate->nLastDTS = pComponentPrivate->nAnchor = pComponentPrivate->spInputBufQ->pCurBufHeader->nTimeStamp;
                            } else {
                                int tmpChannelCount = pComponentPrivate->spCodecCtx->channels > 2?2:pComponentPrivate->spCodecCtx->channels;
                                pTmpOutputBufHeader->nTimeStamp = pComponentPrivate->nLastDTS
                                        + ((pTmpOutputBufHeader->nFilledLen * 1000000.0)/(pComponentPrivate->pcmParams->nSamplingRate * 2.0 * tmpChannelCount));
                                pComponentPrivate->nLastDTS = pTmpOutputBufHeader->nTimeStamp;
                            }
                        }
                        if ( pComponentPrivate->bIsEOFSent == 1
                                && pComponentPrivate->SendAfterEOS == 1
                                && pComponentPrivate->spInputBufQ->Offset == pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen)
                        {
                            pTmpOutputBufHeader->nTimeStamp = pComponentPrivate->nLastDTS;
                            pTmpOutputBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
                        }
                        pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                                  pComponentPrivate->pHandle->pApplicationPrivate,
                                                                  pTmpOutputBufHeader);
                        pComponentPrivate->num_Op_Issued++;
                        pComponentPrivate->nFillBufferDoneCount++;
                        SignalIfAllBuffersAreReturned(pComponentPrivate);

                        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>after decode audio and FillBufferDone.");
                        if(pComponentPrivate->spInputBufQ->Offset == pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen)
                        {
                            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                  pComponentPrivate->spInputBufQ->pCurBufHeader// pBufHeader
                                                                   );
                            OMX_Buf_Q_Cur_Reset(pComponentPrivate->spInputBufQ);
                            pTmpOutputBufHeader= NULL;
                            pComponentPrivate->nEmptyBufferDoneCount++;
                            SignalIfAllBuffersAreReturned(pComponentPrivate);
                        }
                    }else {
                        OMX_Buf_Q_Put(pComponentPrivate->spInputBufQ, pBufHeader);
                    }

                    LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>OMX_DirInput pos 27.");
                    pComponentPrivate->num_Sent_Ip_Buff++;
                    OMX_PRCOMM2(pComponentPrivate->dbg, "Sending Input buffer to Codec\n");
                }
            }else if (pComponentPrivate->curState == OMX_StatePause){
                LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>pComponentPrivate->curState == OMX_StatePause.");
                OMX_Buf_Q_Put(pComponentPrivate->spInputBufQ, pBufHeader);
                pComponentPrivate->pInputBufHdrPending[pComponentPrivate->nNumInputBufPending++] = pBufHeader;
            }
        } else {
            LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>OMX_DirInput pos 28.");
            OMX_PRBUFFER2(pComponentPrivate->dbg, "Forcing EmptyBufferDone\n");
            pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                           pComponentPrivate->pHandle->pApplicationPrivate,
                                                           pBufHeader);
            pComponentPrivate->nEmptyBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
        }
        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>OMX_DirInput pos 29.");
        if(pBufHeader->pMarkData){
            pComponentPrivate->pMarkData = pBufHeader->pMarkData;
            pComponentPrivate->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->pMarkData = pBufHeader->pMarkData;
            pComponentPrivate->pOutputBufferList->pBufHdr[0]->hMarkTargetComponent = pBufHeader->hMarkTargetComponent;
            if(pBufHeader->hMarkTargetComponent == pComponentPrivate->pHandle && pBufHeader->pMarkData){
                pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                       pComponentPrivate->pHandle->pApplicationPrivate,
                                                       OMX_EventMark,
                                                       0,
                                                       0,
                                                       pBufHeader->pMarkData);
            }
        }
        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>OMX_DirInput pos 30.");
    }else if (eDir == OMX_DirOutput) {

        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>eDir == OMX_DirOutput.");
        pComponentPrivate->nHandledFillThisBuffers++;
        if (pComponentPrivate->curState == OMX_StateIdle){
            pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                                      pBufHeader);
            pComponentPrivate->nFillBufferDoneCount++;
            SignalIfAllBuffersAreReturned(pComponentPrivate);
            OMX_PRBUFFER2(pComponentPrivate->dbg, ":: %d %s In idle state return output buffers\n", __LINE__, __FUNCTION__);
            goto EXIT;
        }

        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>OMX_DirOutput, pos 1.");
        if (pComponentPrivate->curState == OMX_StateExecuting) {
            LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>pComponentPrivate->curState == OMX_StateExecuting");

            int outputLen, retLen;
            outputLen = FFMPEG_MAX_OUTPUT_LEN;
            int16_t * pTmpOutput;

            if ( pComponentPrivate->spInputBufQ->BufCount > 0 || pComponentPrivate->spInputBufQ->BufFlag == 1){
                OMX_BUFFERHEADERTYPE * pTmpInputBufHeader;
                /*get input data to be decoding from spInputBufQ*/
                if(pComponentPrivate->spInputBufQ->BufFlag == 0){
                    OMX_Buf_Q_Get(pComponentPrivate->spInputBufQ,  &pTmpInputBufHeader);
                }

                if (NULL != pComponentPrivate->spResample){
                    LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>before decode for sample_fmt != SAMPLE_FMT_S16");
                    /*if out put pcm of cur format decoder is not 16 bit_per_sample
                     *should assign a tmp buf to store output data for conversion
                     */
                    if (pComponentPrivate->pOutputBuf !=NULL){
                        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>pComponentPrivate->pOutputBuf !=NULL");
                        pTmpOutput = pComponentPrivate->pOutputBuf;
                        memset(pTmpOutput,0,FFMPEG_MAX_OUTPUT_LEN);
                    }else{
                        LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>pComponentPrivate->pOutputBuf ==NULL");
                        eError = OMX_ErrorHardware;
                        pComponentPrivate->cbInfo.EventHandler(pComponentPrivate->pHandle,
                                                        pComponentPrivate->pHandle->pApplicationPrivate,
                                                        OMX_EventError,
                                                        eError,
                                                        OMX_NUFRONT_ErrorSevere,
                                                        NULL);
                        goto EXIT;
                    }
                }else{
                    LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>before decode for sample_fmt == SAMPLE_FMT_S16");
                    pTmpOutput = (pBufHeader->pBuffer);
                }

                /*decode source data*/
                AVPacket pkt;
                av_init_packet(&pkt);
                if (pComponentPrivate->spInputBufQ->pCurBufHeader->pBuffer) {
                    pkt.data = (const uint8_t *)(pComponentPrivate->spInputBufQ->pCurBufHeader->pBuffer) + pComponentPrivate->spInputBufQ->Offset;
                    pkt.size = (int)pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen - pComponentPrivate->spInputBufQ->Offset;
                } else {
                    pkt.data = NULL;
                    pkt.size = 0;
                }
                if (pkt.data && (!strncmp(pkt.data, "silencepkt", 10) || !strncmp(pkt.data, "seek", 4))) {
                    int64_t tmp_duration = 0;
                    LOG_AUDIO("audio_dbg:NUAUDIODEC_HandleDataBuf_FromApp===>>>silence packet");
                    retLen = pkt.size;
                    if (!strncmp(pkt.data, "silencepkt", 10)) {
                        sscanf(pkt.data, "silencepkt:%lld", &tmp_duration);
                        if (0 == pComponentPrivate->nSampleSize) {
                            pComponentPrivate->nSampleSize = 2;
                        }
                        outputLen = (int) ((tmp_duration
                                        * pComponentPrivate->spCodecCtx->sample_rate
                                        * pComponentPrivate->spCodecCtx->channels
                                        * pComponentPrivate->nSampleSize) / 1E6);
                        outputLen = outputLen - outputLen % 2;
                        /*outputLen = pComponentPrivate->mOutputLen;*/
                        memset(pTmpOutput, 0, outputLen);
                        GSLOGD("silence packet len:%d",outputLen);
                    } else {
                        GSLOGD("seek packet 2");
                        outputLen = 0;
                        retLen = 0;
                    }
                } else {
                    retLen =  avcodec_decode_audio3(pComponentPrivate->spCodecCtx,
                                      (int16_t *)(pTmpOutput),
                                      (int*)&outputLen,
                                       &pkt);
                    if (outputLen > 0 && pComponentPrivate->mOutputLen == 0) {
                        LOG_AUDIO("audio_dbg:NUAUDIODEC_HandleDataBuf_FromApp===>>>set pComponentPrivate->mOutputLen for silence packet");
                        pComponentPrivate->mOutputLen = outputLen;
                    }
                }

                if (retLen < 0) {
                    AVCodec *pCodec;
                    avcodec_close(pComponentPrivate->spCodecCtx);
                    pCodec = avcodec_find_decoder(pComponentPrivate->spCodecCtx->codec_id);
                    if (NULL == pCodec){
                        return OMX_ErrorInsufficientResources; /* Codec not found*/
                    }

                    if(avcodec_open(pComponentPrivate->spCodecCtx, pCodec) < 0) {
                        return OMX_ErrorInsufficientResources;
                    }
                }

                if (retLen >= 0){

                    LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>modify parameter after decode audio, outputLen is %d.", outputLen);
                    pComponentPrivate->spInputBufQ->Offset +=retLen;
                    if (outputLen > 0
                            && NULL != pComponentPrivate->spResample) {
                        outputLen = audio_resample(pComponentPrivate->spResample,
                              (short *)pBufHeader->pBuffer,
                              (short *) pTmpOutput,
                              outputLen / (pComponentPrivate->spCodecCtx->channels * pComponentPrivate->nSampleSize));
                        outputLen = outputLen * (pComponentPrivate->pcmParams->nChannels * (pComponentPrivate->pcmParams->nBitPerSample / 8));
                    }
                    pBufHeader->nFilledLen = outputLen;
                    /*if out put pcm is 6 channels pcm, should mix it down*/
                }else {
                    pComponentPrivate->spInputBufQ->Offset = pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen;
                    pBufHeader->nFilledLen = 0;
                }

                if (NOPTS_VALUE == pComponentPrivate->spInputBufQ->pCurBufHeader->nTimeStamp
                          && pComponentPrivate->pcmParams->nSamplingRate > 0){
                    int tmpChannelCount = pComponentPrivate->spCodecCtx->channels > 2?2:pComponentPrivate->spCodecCtx->channels;
                    pBufHeader->nTimeStamp = pComponentPrivate->nLastDTS
                                  + ((pBufHeader->nFilledLen * 1000000.0)/(pComponentPrivate->pcmParams->nSamplingRate * 2.0 * tmpChannelCount));
                    pComponentPrivate->nLastDTS = pBufHeader->nTimeStamp;
                }else {
                    pBufHeader->nTimeStamp = pComponentPrivate->spInputBufQ->pCurBufHeader->nTimeStamp;

                    if (pComponentPrivate->nAnchor != pComponentPrivate->spInputBufQ->pCurBufHeader->nTimeStamp
                            && pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen > 0) {
                        pComponentPrivate->nLastDTS = pComponentPrivate->nAnchor = pComponentPrivate->spInputBufQ->pCurBufHeader->nTimeStamp;
                    } else {
                        int tmpChannelCount = pComponentPrivate->spCodecCtx->channels > 2?2:pComponentPrivate->spCodecCtx->channels;
                        pBufHeader->nTimeStamp = pComponentPrivate->nLastDTS +
                                 + ((pBufHeader->nFilledLen * 1000000.0)/(pComponentPrivate->pcmParams->nSamplingRate * 2.0 * tmpChannelCount));
                        pComponentPrivate->nLastDTS = pBufHeader->nTimeStamp;
                    }
                }
                if ( pComponentPrivate->bIsEOFSent == 1
                        && pComponentPrivate->SendAfterEOS == 1
                        && pComponentPrivate->spInputBufQ->BufCount == 0
                        && pComponentPrivate->spInputBufQ->Offset == pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen){
                    pBufHeader->nTimeStamp = pComponentPrivate->nLastDTS;
                    pBufHeader->nFlags |= OMX_BUFFERFLAG_EOS;
                }
                pComponentPrivate->cbInfo.FillBufferDone (pComponentPrivate->pHandle,
                                                                  pComponentPrivate->pHandle->pApplicationPrivate,
                                                                  pBufHeader);
                pComponentPrivate->num_Op_Issued++;
                pComponentPrivate->nFillBufferDoneCount++;
                SignalIfAllBuffersAreReturned(pComponentPrivate);

                LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>after decode audio and FillBufferDone.");
                /**/
                if(pComponentPrivate->spInputBufQ->Offset == pComponentPrivate->spInputBufQ->pCurBufHeader->nFilledLen){
                /**/
                    pComponentPrivate->cbInfo.EmptyBufferDone (pComponentPrivate->pHandle,
                                                                   pComponentPrivate->pHandle->pApplicationPrivate,
                                                                   pComponentPrivate->spInputBufQ->pCurBufHeader// pBufHeader
                                                                   );
                    OMX_Buf_Q_Cur_Reset(pComponentPrivate->spInputBufQ);
                    pTmpInputBufHeader= NULL;
                    pComponentPrivate->nEmptyBufferDoneCount++;
                    SignalIfAllBuffersAreReturned(pComponentPrivate);
                }
            }else {
                OMX_Buf_Q_Put(pComponentPrivate->spOutputBufQ, pBufHeader);
            }
        }else if (pComponentPrivate->curState == OMX_StatePause) {
            pComponentPrivate->pOutputBufHdrPending[pComponentPrivate->nNumOutputBufPending++] = pBufHeader;
        }

    }else {
        OMX_PRBUFFER2(pComponentPrivate->dbg, "%d : BufferHeader %p, Buffer %p Unknown ..........\n",__LINE__,pBufHeader, pBufHeader->pBuffer);
        eError = OMX_ErrorBadParameter;
    }
 EXIT:
    LOG_AUDIO("audio_debug:NUAUDIODEC_HandleDataBuf_FromApp===>>>Exiting from  HandleDataBuf_FromApp.");
    OMX_PRINT1(pComponentPrivate->dbg, "%d : Exiting from  HandleDataBuf_FromApp: %x \n",__LINE__,eError);
    if(eError == OMX_ErrorBadParameter) {
        OMX_ERROR4(pComponentPrivate->dbg, "%d : Error = OMX_ErrorBadParameter\n",__LINE__);
    }
    return eError;
}

/* ================================================================================= * */
/**
* @fn NUAUDIODEC_GetBufferDirection() function determines whether it is input buffer or
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
                                                         OMX_DIRTYPE *eDir)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate = pBufHeader->pPlatformPrivate;
    OMX_U32 nBuf;// = pComponentPrivate->pInputBufferList->numBuffers;
    OMX_BUFFERHEADERTYPE *pBuf = NULL;
    int flag = 1;
    OMX_U32 i=0;

    nBuf = pComponentPrivate->pOutputBufferList->numBuffers;
    i = 0;
    while(i < nBuf) {
        pBuf = pComponentPrivate->pOutputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirOutput;
            OMX_PRINT1(pComponentPrivate->dbg, "%d :: Buffer %p is OUTPUT BUFFER\n",__LINE__, pBufHeader);
            flag = 0;
            goto EXIT;
        }
        i++;
    }
    nBuf = pComponentPrivate->pInputBufferList->numBuffers;
    i = 0;
    while(i < nBuf) {
        pBuf = pComponentPrivate->pInputBufferList->pBufHdr[i];
        if(pBufHeader == pBuf) {
            *eDir = OMX_DirInput;
            OMX_PRINT1(pComponentPrivate->dbg, "%d :: Buffer %p is INPUT BUFFER\n",__LINE__, pBufHeader);
            flag = 0;
            goto EXIT;
        }
        i++;
    }

    if (flag == 1) {
        NUAUDIODEC_OMX_ERROR_EXIT(eError, OMX_ErrorBadParameter,
                              "Buffer Not Found in List : OMX_ErrorBadParameter");
    }

 EXIT:
    return eError;
}


/* ========================================================================== */
/**
* @NUAUDIODEC_Ffmpeg_Deinit() This function is called by the component during
* de-init to close component thread, Command pipe, data pipe & LCML pipe.
*
* @param pComponent  handle for this instance of the component
*
* @pre
*
* @post
*
* @return none
*/
/* ========================================================================== */

void NUAUDIODEC_Ffmpeg_Deinit(OMX_HANDLETYPE pComponent)
{
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)pComponent;
    NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    OMX_BUFFERHEADERTYPE *pBufHeader;

    OMX_U32 i=0;
    LOG_AUDIO("audio_debug:NUAUDIODEC_Ffmpeg_Deinit===>>>deinit nuaudio decoder");

    if (pComponentPrivate->spResample) {
        audio_resample_close(pComponentPrivate->spResample);
        pComponentPrivate->spResample = NULL;
    }

    if(NULL != pComponentPrivate->pOutputBuf) {
        av_free(pComponentPrivate->pOutputBuf);
        pComponentPrivate->pOutputBuf = NULL;
    }

    if(NULL != pComponentPrivate->spInputBufQ){
        OMX_Buf_Q_Destroy(pComponentPrivate->spInputBufQ);
        pComponentPrivate->spInputBufQ = NULL;
    }
    if(NULL != pComponentPrivate->spOutputBufQ){
        OMX_Buf_Q_Destroy(pComponentPrivate->spOutputBufQ);
        pComponentPrivate->spOutputBufQ = NULL;
    }
    if (NULL != pComponentPrivate->spCodecCtx) {
        avcodec_close(pComponentPrivate->spCodecCtx);
    }
}
/*=======================================================================*/
/*! @fn NUAUDIODec_GetSampleRateIndexL

 * @brief Gets the sample rate index

 * @param  aRate : Actual Sampling Freq

 * @Return  Index

 */
/*=======================================================================*/
int NUAUDIODec_GetSampleRateIndexL( const int aRate)
{
    int index = 0;
    OMXDBG_PRINT(stderr, PRINT, 2, 0, "%d::aRate:%d\n",__LINE__,aRate);

    switch( aRate ){
    case 96000:
        index = 0;
        break;
    case 88200:
        index = 1;
        break;
    case 64000:
        index = 2;
        break;
    case 48000:
        index = 3;
        break;
    case 44100:
        index = 4;
        break;
    case 32000:
        index = 5;
        break;
    case 24000:
        index = 6;
        break;
    case 22050:
        index = 7;
        break;
    case 16000:
        index = 8;
        break;
    case 12000:
        index = 9;
        break;
    case 11025:
        index = 10;
        break;
    case 8000:
        index = 11;
        break;
    default:
        OMXDBG_PRINT(stderr, PRINT, 2, 0, "Invalid sampling frequency\n");
        break;
    }

    OMXDBG_PRINT(stderr, PRINT, 2, 0, "%d::index:%d\n",__LINE__,index);
    return index;
}


int NUAUDIODec_GetSampleRatebyIndex( const int index)
{
    int sample_rate = 0;

    switch( index ){
    case 0:
        sample_rate = 96000;
        break;
    case 1:
        sample_rate = 88200;
        break;
    case 2:
        sample_rate = 64000;
        break;
    case 3:
        sample_rate = 48000;
        break;
    case 4:
        sample_rate = 44100;
        break;
    case 5:
        sample_rate = 32000;
        break;
    case 6:
        sample_rate = 24000;
        break;
    case 7:
        sample_rate = 22050;
        break;
    case 8:
        sample_rate = 16000;
        break;
    case 9:
        sample_rate = 12000;
        break;
    case 10:
        sample_rate = 11025;
        break;
    case 11:
        sample_rate = 8000;
        break;
    default:
        OMXDBG_PRINT(stderr, PRINT, 2, 0, "Invalid index\n");
        break;
    }

    return sample_rate;
}
/* ========================================================================== */
/**
* @NUAUDIODEC_SetPending() This function marks the buffer as pending when it is sent
* to DSP/
*
* @param pComponentPrivate This is component's private date area.
*
* @param pBufHdr This is poiter to OMX Buffer header whose buffer is sent to DSP
*
* @param eDir This is direction of buffer i.e. input or output.
*
* @pre None
*
* @post None
*
* @return none
*/
/* ========================================================================== */
void NUAUDIODEC_SetPending(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************INPUT BUFFER %d IS PENDING Line %lu, :%p******************************\n",i,lineNumber,pBufHdr);
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 1;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************OUTPUT BUFFER %d IS PENDING Line, %lu :%p******************************\n",i,lineNumber,pBufHdr);
            }
        }
    }
}

/* ========================================================================== */
/**
* @NUAUDIODEC_ClearPending() This function clears the buffer status from pending
* when it is received back from DSP.
*
* @param pComponentPrivate This is component's private date area.
*
* @param pBufHdr This is poiter to OMX Buffer header that is received from
* DSP/LCML.
*
* @param eDir This is direction of buffer i.e. input or output.
*
* @pre None
*
* @post None
*
* @return none
*/
/* ========================================================================== */
void NUAUDIODEC_ClearPending(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate,
            OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir, OMX_U32 lineNumber)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        i = 0;
        while (i < pComponentPrivate->pInputBufferList->numBuffers) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                pComponentPrivate->pInputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************INPUT BUFFER %d IS RECLAIMED Line %lu, :%p******************************\n",i,lineNumber,pBufHdr);
            }
            i++;
        }
    }
    else {
        i = 0;
        while (i < pComponentPrivate->pOutputBufferList->numBuffers) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                pComponentPrivate->pOutputBufferList->bBufferPending[i] = 0;
                OMX_PRBUFFER2(pComponentPrivate->dbg, "*******************OUTPUT BUFFER %d IS RECLAIMED Line %lu, :%p******************************\n",i,lineNumber,pBufHdr);
            }
        i++;
        }
    }
}

/* ========================================================================== */
/**
* @NUAUDIODEC_IsPending() This function checks whether or not a buffer is pending.
*
* @param pComponentPrivate This is component's private date area.
*
* @param pBufHdr This is poiter to OMX Buffer header of interest.
*
* @param eDir This is direction of buffer i.e. input or output.
*
* @pre None
*
* @post None
*
* @return none
*/
/* ========================================================================== */
OMX_U32 NUAUDIODEC_IsPending(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_BUFFERHEADERTYPE *pBufHdr, OMX_DIRTYPE eDir)
{
    OMX_U16 i;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pInputBufferList->pBufHdr[i]) {
                return pComponentPrivate->pInputBufferList->bBufferPending[i];
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBufHdr == pComponentPrivate->pOutputBufferList->pBufHdr[i]) {
                return pComponentPrivate->pOutputBufferList->bBufferPending[i];
            }
        }
    }
    return -1;
}


/* ========================================================================== */
/**
* @NUAUDIODEC_IsValid() This function identifies whether or not buffer recieved from
* LCML is valid. It searches in the list of input/output buffers to do this.
*
* @param pComponentPrivate This is component's private date area.
*
* @param pBufHdr This is poiter to OMX Buffer header of interest.
*
* @param eDir This is direction of buffer i.e. input or output.
*
* @pre None
*
* @post None
*
* @return status of the buffer.
*/
/* ========================================================================== */
OMX_U32 NUAUDIODEC_IsValid(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U8 *pBuffer, OMX_DIRTYPE eDir)
{
    OMX_U16 i;
    int found=0;

    if (eDir == OMX_DirInput) {
        for (i=0; i < pComponentPrivate->pInputBufferList->numBuffers; i++) {
            if (pBuffer == pComponentPrivate->pInputBufferList->pBufHdr[i]->pBuffer) {
                found = 1;
            }
        }
    }
    else {
        for (i=0; i < pComponentPrivate->pOutputBufferList->numBuffers; i++) {
            if (pBuffer == pComponentPrivate->pOutputBufferList->pBufHdr[i]->pBuffer) {
                found = 1;
            }
        }
    }
    return found;
}



/*  =========================================================================*/
/*  func    GetBits                                                          */
/*                                                                           */
/*  desc    Gets aBits number of bits from position aPosition of one buffer  */
/*            and returns the value in a TUint value.                        */
/*  =========================================================================*/
OMX_U32 NUAUDIODEC_GetBits(OMX_U32* nPosition, OMX_U8 nBits, OMX_U8* pBuffer, OMX_BOOL bIcreasePosition)
{
    OMX_U32 nOutput;
    OMX_U32 nNumBitsRead = 0;
    OMX_U32 nBytePosition = 0;
    OMX_U8  nBitPosition =  0;
    nBytePosition = *nPosition / 8;
    nBitPosition =  *nPosition % 8;

    if (bIcreasePosition)
        *nPosition += nBits;
    nOutput = ((OMX_U32)pBuffer[nBytePosition] << (24+nBitPosition) );
    nNumBitsRead = nNumBitsRead + (8 - nBitPosition);
    if (nNumBitsRead < nBits)
    {
        nOutput = nOutput | ( pBuffer[nBytePosition + 1] << (16+nBitPosition));
        nNumBitsRead = nNumBitsRead + 8;
    }
    if (nNumBitsRead < nBits)
    {
        nOutput = nOutput | ( pBuffer[nBytePosition + 2] << (8+nBitPosition));
        nNumBitsRead = nNumBitsRead + 8;
    }
    if (nNumBitsRead < nBits)
    {
        nOutput = nOutput | ( pBuffer[nBytePosition + 3] << (nBitPosition));
        nNumBitsRead = nNumBitsRead + 8;
    }
    nOutput = nOutput >> (32 - nBits) ;
    return nOutput;
}
/* ========================================================================== */
/**
* @SignalIfAllBuffersAreReturned() This function send signals if OMX returned all buffers to app
*
* @param NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate
*
* @pre None
*
* @post None
*
* @return None
*/
/* ========================================================================== */
void SignalIfAllBuffersAreReturned(NUAUDIODEC_COMPONENT_PRIVATE *pComponentPrivate)
{
    if((pComponentPrivate->nEmptyThisBufferCount == pComponentPrivate->nEmptyBufferDoneCount) &&
       (pComponentPrivate->nFillThisBufferCount == pComponentPrivate->nFillBufferDoneCount))
    {
        if(pthread_mutex_lock(&bufferReturned_mutex) != 0)
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: bufferReturned_mutex mutex lock error\n",__LINE__);
        }
        pthread_cond_broadcast(&bufferReturned_condition);
        OMX_PRINT1(pComponentPrivate->dbg, "Sending pthread signal that OMX has returned all buffers to app");
        if(pthread_mutex_unlock(&bufferReturned_mutex) != 0)
        {
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: bufferReturned_mutex mutex unlock error\n",__LINE__);
        }
        return;
    }
}

/* ========================================================================== */
/**
* @NUAUDIODEC_Init()
*
* @param pThreadData
*
* @pre None
*
* @post None
*
* @return None
*/
/* ========================================================================== */
OMX_S32 NUAUDIODEC_Ffmpeg_Init(NUAUDIODEC_COMPONENT_PRIVATE* pComponentPrivate, void* codecCtx)
{
    OMX_S32                     Status;
    OMX_S32                     fd;
    OMX_S64                     offset;
    OMX_S64                     len;
    AVCodec                     *pCodec;
    OMX_S32                     i = 0;
    ZJFLOGD("function in.");
    pComponentPrivate->spResample = NULL;

    if (NULL == codecCtx) {
        return OMX_ErrorInsufficientResources;
    }
    memcpy(pComponentPrivate->spCodecCtx, codecCtx, sizeof(AVCodecContext));
    LOG_AUDIO("audio_debug:NUFRONT_init===>>>pComponentPrivate->spCodecCtx->codec_id is %d", pComponentPrivate->spCodecCtx->codec_id);
    pCodec = avcodec_find_decoder(pComponentPrivate->spCodecCtx->codec_id);
    if(pCodec==NULL){
        LOG_AUDIO("audio_debug:NUFRONT_init===>>>can not find decoder");
        return OMX_ErrorInsufficientResources; /* Codec not found*/
    }
    if(pCodec->capabilities & CODEC_CAP_TRUNCATED)
        pComponentPrivate->spCodecCtx->flags|=CODEC_FLAG_TRUNCATED;
    LOG_AUDIO("audio_debug:NUFRONT_init===>>>set nChannels and nSamplingRate,\n \
                                                pComponentPrivate->spCodecCtx->channels is %d,\n \
                                                pComponentPrivate->spCodecCtx->sample_rate is %d,\n \
                                                pComponentPrivate->spCodecCtx->frame_size is %d",
                                                pComponentPrivate->spCodecCtx->channels,
                                                pComponentPrivate->spCodecCtx->sample_rate,
                                                pComponentPrivate->spCodecCtx->frame_size);

    if(OMX_Buf_Q_Init(&(pComponentPrivate->spInputBufQ)) < 0)
        return OMX_ErrorInsufficientResources;
    if(OMX_Buf_Q_Init(&(pComponentPrivate->spOutputBufQ)) < 0)
        return OMX_ErrorInsufficientResources;

    LOG_AUDIO("audio_debug:NUFRONT_init===>>>open decode");

    if(avcodec_open(pComponentPrivate->spCodecCtx, pCodec)<0)
        return OMX_ErrorInsufficientResources; // Could not open codec

    if (pComponentPrivate->spCodecCtx->sample_rate <= 2 * NU_OUTPUT_SAMPLERATE
            && SAMPLE_FMT_S16 == pComponentPrivate->spCodecCtx->sample_fmt
            && NUAUDIO_STERO >= pComponentPrivate->spCodecCtx->channels) {
        pComponentPrivate->pcmParams->nSamplingRate = pComponentPrivate->spCodecCtx->sample_rate;
        pComponentPrivate->pcmParams->nChannels = pComponentPrivate->spCodecCtx->channels;
    } else {
        /*audio flinger only can handle 2 * hal_sample_rate down sampling.
         so, if samplerate of media file is out of this range, we should
         handle the satuation here.*/
        if (pComponentPrivate->spCodecCtx->sample_rate > 2 * NU_OUTPUT_SAMPLERATE) {
            pComponentPrivate->pcmParams->nSamplingRate = NU_OUTPUT_SAMPLERATE;
        } else {
            pComponentPrivate->pcmParams->nSamplingRate = pComponentPrivate->spCodecCtx->sample_rate;
        }

        if (NUAUDIO_STERO < pComponentPrivate->spCodecCtx->channels) {
            pComponentPrivate->pcmParams->nChannels = NUAUDIO_STERO;
        } else {
            pComponentPrivate->pcmParams->nChannels = pComponentPrivate->spCodecCtx->channels;
        }

        if (SAMPLE_FMT_S16 == pComponentPrivate->spCodecCtx->sample_fmt) {
            pComponentPrivate->nSampleSize = 2;
        }

        if (SAMPLE_FMT_U8 == pComponentPrivate->spCodecCtx->sample_fmt) {
            pComponentPrivate->nSampleSize = 1;
        }

        if (SAMPLE_FMT_S32 == pComponentPrivate->spCodecCtx->sample_fmt
                || SAMPLE_FMT_FLT == pComponentPrivate->spCodecCtx->sample_fmt) {
            pComponentPrivate->nSampleSize = 4;
        }

        if (SAMPLE_FMT_DBL == pComponentPrivate->spCodecCtx->sample_fmt) {
            pComponentPrivate->nSampleSize = 8;
        }

        if (pComponentPrivate->pOutputBuf) {
            av_free(pComponentPrivate->pOutputBuf);
            pComponentPrivate->pOutputBuf = NULL;
        }
        if ((pComponentPrivate->pOutputBuf = (int16_t *)av_malloc(FFMPEG_MAX_OUTPUT_LEN * 10))<0 ) {
            LOG_AUDIO("audio_debug:NUFRONT_INIT===>>>get pOutputBuf failed.");
            return OMX_ErrorInsufficientResources;
        } else {
            LOG_AUDIO("audio_debug:NUFRONT_INIT===>>>get pOutputBuf successed.");
        }

        if (pComponentPrivate->spResample) {
            audio_resample_close(pComponentPrivate->spResample);
            pComponentPrivate->spResample = NULL;
        }

        pComponentPrivate->spResample = av_audio_resample_init(pComponentPrivate->pcmParams->nChannels,
                                                               pComponentPrivate->spCodecCtx->channels,
                                                               pComponentPrivate->pcmParams->nSamplingRate,
                                                               pComponentPrivate->spCodecCtx->sample_rate,
                                                               SAMPLE_FMT_S16, pComponentPrivate->spCodecCtx->sample_fmt,
                                                               16, 10, 0, 0.8);
    }

    ZJFLOGD("will signal component thread");
    pthread_mutex_lock(&pComponentPrivate->CodecInit_mutex);
    pthread_cond_signal(&pComponentPrivate->CodecInit_threshold);
    pthread_mutex_unlock(&pComponentPrivate->CodecInit_mutex);
    LOG_AUDIO("audio_debug:NUFRONT_INIT===>>>function out.");
    ZJFLOGD("function out.");
    return OMX_ErrorNone;
}

