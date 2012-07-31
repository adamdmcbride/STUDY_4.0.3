/*
 *Copyright (C), 2009~2019, NUFRONT. Co., Ltd.
 *File name:     OMX_BufQ.h 
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
#ifndef OMX_BUFQ__H
#define OMX_BUFQ__H
#include "OMX_Core.h"
#include <pthread.h>

/* Buf Queue element type */
typedef struct OMX_BUF_ELE_TYPE
{
	OMX_BUFFERHEADERTYPE * pBufHeader; //pointer to head of queue
	struct OMX_BUF_ELE_TYPE * next;    //pointer to next element of buffer queue
}OMX_BUF_ELE_TYPE;

/* Buf Queue head type */
typedef struct OMX_BUF_Q_TYPE
{
	OMX_BUF_ELE_TYPE * FirstBuf, *LastBuf; //point to first and last element of buffer queue
	OMX_U32 BufCount;                      //counter of queue elements
	OMX_BUFFERHEADERTYPE * pCurBufHeader;  //point to the element current handling
	OMX_U32 Offset;                        //record Offset of buffer current being decode
	OMX_S32 BufFlag;                       //indicate whether has buffer been under handling
	pthread_mutex_t mutex;                 //lock for member various
}OMX_BUF_Q_TYPE;

/* ================================================================================= * */
/**
 * OMX_Buf_Q_Init() function is called by OMX Component to initialize the BufQ
 *
 * @param ** q  This is Queue handle .
 *
 * @pre          should be called by application.
 *
 * @post         Queue has initialzed with default values.
 *
 *  @return      0 successful 
 *               -1 failed 
 *
 */
/* ================================================================================ * */
int OMX_Buf_Q_Init(OMX_BUF_Q_TYPE ** q);
/* ================================================================================= * */
/**
 * OMX_Buf_Q_Destroy() function is called by OMX Component to Destroy the BufQ
 *
 * @param * q  This is Queue handle .
 *
 * @pre          should be called by application.
 *
 * @post         Queue has destroyed.
 *
 *  @return      0 successful 
 *               -1 failed 
 *
 */
/* ================================================================================ * */
int OMX_Buf_Q_Destroy(OMX_BUF_Q_TYPE * q);
/* ================================================================================= * */
/**
 * OMX_Buf_Q_Put() function is called by OMX Component to Put Buf into Queue
 *
 * @param * q  This is Queue handle .
 * @param * pBUfHeader the buf been put into q . 
 *
 * @pre          should be called by application.
 *
 * @post         Queue has destroyed.
 *
 *  @return      0 successful 
 *               -1 failed 
 *
 */
/* ================================================================================ * */
int OMX_Buf_Q_Put(OMX_BUF_Q_TYPE *q,  OMX_BUFFERHEADERTYPE * pBufHeader);
/* ================================================================================= * */
/**
 * OMX_Buf_Q_Get() function is called by OMX Component to get Buf from Queue
 *
 * @param * q  This is Queue handle . 
 * @param * pBUfHeader the buf been get from q . 
 *
 * @pre          should be called by application.
 *
 * @post         Queue has destroyed.
 *
 *  @return      0 successful 
 *               -1 failed 
 *
 */
/* ================================================================================ * */
int OMX_Buf_Q_Get(OMX_BUF_Q_TYPE *q,  OMX_BUFFERHEADERTYPE ** pBufHeader);
/* ================================================================================= * */
/**
 * OMX_Buf_Q_Cur_Set() function is called by OMX Component to set current handling buf
 *
 * @param * q  This is Queue handle . 
 * @param * pBUfHeader the buf been set as cur buf . 
 *
 * @pre          should be called by application.
 *
 * @post         Queue has destroyed.
 *
 *  @return    NULL  
 *              
 */
/* ================================================================================ * */
void OMX_Buf_Q_Cur_Set(OMX_BUF_Q_TYPE * q,  OMX_BUFFERHEADERTYPE * pBufHeader);
/* ================================================================================= * */
/**
 * OMX_Buf_Q_Cur_Reset() function is called by OMX Component to reset current handling buf
 *
 * @param * q  This is Queue handle . 
 *
 * @pre          should be called by application.
 *
 * @post         Queue has destroyed.
 *
 *  @return    NULL  
 *              
 */
/* ================================================================================ * */
void OMX_Buf_Q_Cur_Reset(OMX_BUF_Q_TYPE * q);
#endif

