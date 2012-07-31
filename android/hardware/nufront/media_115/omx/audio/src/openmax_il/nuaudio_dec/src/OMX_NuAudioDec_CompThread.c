/*
 *Copyright (C), 2009~2019, NUFRONT. Co., Ltd.
 *File name:     OMX_NuAudioDec_CompThread.c
 *Author: Zhai Jianfeng Version: v1.0       Date: 2010-09-25*
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
#include <sys/select.h>
#include <memory.h>
#include <fcntl.h>
#include <signal.h>

#include <string.h>
#include <stdio.h>

#ifdef ANDROID
#include <utils/threads.h>
#include <linux/prctl.h>
#endif

#include "OMX_NuAudioDec_Utils.h"
#include <utils/nufrontlog.h>
/* ================================================================================= * */
/**
* @fn NUAUDIODEC_ComponentThread() This is component thread that keeps listening for
* commands or event/messages/buffers from application or from LCML.
*
* @param pThreadData This is thread argument.
*
* @pre          None
*
* @post         None
*
*  @return      OMX_ErrorNone = Always
*
*  @see         None
*/
/* ================================================================================ * */
void* NUAUDIODEC_ComponentThread (void* pThreadData)
{
    int status;
    struct timespec tv;
    int fdmax;
    fd_set rfds;
    OMX_U32 nRet;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    NUAUDIODEC_COMPONENT_PRIVATE* pComponentPrivate = (NUAUDIODEC_COMPONENT_PRIVATE*)pThreadData;
    OMX_COMPONENTTYPE *pHandle = pComponentPrivate->pHandle;
    ZJFLOGD("function in.");
    ZJFLOGD("wait for OMXCodec signal to run component thread");
    /**
    pthread_mutex_lock(&pComponentPrivate->CodecInit_mutex);
    pthread_cond_wait(&pComponentPrivate->CodecInit_threshold, &pComponentPrivate->CodecInit_mutex);
    pthread_mutex_unlock(&pComponentPrivate->CodecInit_mutex);
    **/
    ZJFLOGD("after OMXCodec signal.");
#ifdef ANDROID
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    prctl(PR_SET_NAME, (unsigned long)"NUAUDIOComponent", 0, 0, 0);
#endif
/*
    eError = NUAUDIODEC_Ffmpeg_Init((void *)pComponentPrivate); //a@nufront: init ffmpeg
    if(eError != OMX_ErrorNone){
        pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorInsufficientResources,
                                                    OMX_NUFRONT_ErrorSevere,
                                                    "Error from Component Thread in init");
        goto EXIT;
    }

    */
    fdmax = pComponentPrivate->cmdPipe[0];

    if (pComponentPrivate->dataPipe[0] > fdmax) {
        fdmax = pComponentPrivate->dataPipe[0];
    }


    while (1) {
        FD_ZERO (&rfds);
        FD_SET (pComponentPrivate->cmdPipe[0], &rfds);
        FD_SET (pComponentPrivate->dataPipe[0], &rfds);

        tv.tv_sec = 1;
        tv.tv_nsec = 0;

        sigset_t set;
        sigemptyset (&set);
        sigaddset (&set, SIGALRM);
        status = pselect (fdmax+1, &rfds, NULL, NULL, &tv, &set);
        if (pComponentPrivate->bExitCompThrd == 1) {
            LOG_AUDIO("audio_dbg:NUAUDIODEC_ComponentThread===>>> will return for pComponentPrivate->bExitCompThrd == 1.");
            goto EXIT;
        }

        if (0 == status) {
             OMX_ERROR2(pComponentPrivate->dbg, "\n\n\n!!!!!  Component Time Out !!!!!!!!!!!! \n");
        }
        else if (-1 == status) {
            LOG_AUDIO("audio_dbg:NUAUDIODEC_ComponentThread===>>>will return for -1 == status.");
            OMX_ERROR4(pComponentPrivate->dbg, "%d :: Error in Select\n", __LINE__);
            pComponentPrivate->cbInfo.EventHandler (pHandle,
                                                    pHandle->pApplicationPrivate,
                                                    OMX_EventError,
                                                    OMX_ErrorInsufficientResources,
                                                    OMX_NUFRONT_ErrorSevere,
                                                    "Error from COmponent Thread in select");
            eError = OMX_ErrorInsufficientResources;
        }
        else if ((FD_ISSET (pComponentPrivate->dataPipe[0], &rfds))) {
            OMX_S32 ret;
            OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
            OMX_PRCOMM2(pComponentPrivate->dbg, "%d :: DATA pipe is set in Component Thread\n",__LINE__);
            ret = read(pComponentPrivate->dataPipe[0], &pBufHeader, sizeof(pBufHeader));

            if (ret == -1) {
                OMX_ERROR2(pComponentPrivate->dbg, "%d :: Error while reading from the pipe\n",__LINE__);
            }

            eError = NUAUDIODEC_HandleDataBuf_FromApp (pBufHeader,pComponentPrivate);

            if (eError != OMX_ErrorNone) {
                OMX_ERROR2(pComponentPrivate->dbg, "%d :: Error From HandleDataBuf_FromApp\n",__LINE__);
                LOG_AUDIO("audio_dbg:NUAUDIODEC_ComponentThread===>>>will return for eError != OMX_ErrorNone.");
                break;
            }

        }
        else if (FD_ISSET (pComponentPrivate->cmdPipe[0], &rfds)) {
            nRet = NUAUDIODEC_HandleCommand (pComponentPrivate);
            if (nRet == EXIT_COMPONENT_THRD) {
            LOG_AUDIO("audio_dbg:NUAUDIODEC_ComponentThread===>>>nRet == EXIT_COMPONENT_THRD");
            NUAUDIODEC_Ffmpeg_Deinit(pHandle);

            pComponentPrivate->curState = OMX_StateLoaded;

            if(pComponentPrivate->bPreempted==0){
                pComponentPrivate->cbInfo.EventHandler(
                                                           pHandle, pHandle->pApplicationPrivate,
                                                           OMX_EventCmdComplete,
                                                           OMX_ErrorNone,pComponentPrivate->curState, NULL);
            } else {
                pComponentPrivate->cbInfo.EventHandler(pHandle,
                                                           pHandle->pApplicationPrivate,
                                                           OMX_EventError,
                                                           OMX_ErrorResourcesLost,
                                                           OMX_NUFRONT_ErrorMajor,
                                                           NULL);
                pComponentPrivate->bPreempted = 0;
            }
                goto EXIT;
            }

        }

    }
 EXIT:

    pComponentPrivate->bCompThreadStarted = 0;

    return (void*)eError;
}
