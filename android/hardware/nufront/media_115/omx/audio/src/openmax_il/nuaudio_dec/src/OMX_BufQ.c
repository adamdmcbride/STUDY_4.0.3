/**
* @file OMX_BufQ.c
*
* This file implements OMX Component for NUAUDIO/NUVIDEO decoder.
*
* @path  $(CSLPATH)\
*
* @rev  1.0
* author:     zhai
* date:        2011-7-16
*/

#include "OMX_BufQ.h"
#include <utils/Log.h>

/* ================================================================================= * */
/**
 * OMX_Buf_Q_Init() function is called by OMX Component to initialize the BufQ
 *
 * Author        zjf@nufront
 * @param ** q   This is Queue handle .
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
int OMX_Buf_Q_Init(OMX_BUF_Q_TYPE ** q)
{
    *q = malloc(sizeof(OMX_BUF_Q_TYPE));
    if(NULL == *q){
        return -1;
    }
    memset(*q, 0, sizeof(OMX_BUF_Q_TYPE));
    ((OMX_BUF_Q_TYPE *)*q)->BufCount = 0;
    ((OMX_BUF_Q_TYPE *)*q)->Offset = 0;
    ((OMX_BUF_Q_TYPE *)*q)->BufFlag = 0;
    ((OMX_BUF_Q_TYPE *)*q)->pCurBufHeader = NULL;
    ((OMX_BUF_Q_TYPE *)*q)->FirstBuf = ((OMX_BUF_Q_TYPE *)*q)->LastBuf = NULL;
    pthread_mutex_init(&(((OMX_BUF_Q_TYPE *)*q)->mutex), NULL);
    return 0;
}

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
int OMX_Buf_Q_Destroy(OMX_BUF_Q_TYPE * q)
{
    pthread_mutex_destroy(&q->mutex);
    free(q);
    return 0;
}
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
int OMX_Buf_Q_Put(OMX_BUF_Q_TYPE *q,  OMX_BUFFERHEADERTYPE * pBufHeader)
{
    OMX_BUF_ELE_TYPE * ele1;
    /* duplicate the block */
   
    ele1 = malloc(sizeof(OMX_BUF_ELE_TYPE));
    if (!ele1)
    {
        return -1;
    }
    memset(ele1, 0, sizeof(OMX_BUF_ELE_TYPE));
    ele1->pBufHeader = pBufHeader;
    ele1->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if (!q->LastBuf)
        q->FirstBuf = ele1;
    else
        q->LastBuf->next = ele1;
    q->LastBuf = ele1;
    q->BufCount++;
    /* XXX: should duplicate block data in DV case */
    pthread_mutex_unlock(&q->mutex);
    return 0;
}
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

int OMX_Buf_Q_Get(OMX_BUF_Q_TYPE * q,  OMX_BUFFERHEADERTYPE ** pBufHeader)
{
    OMX_BUF_ELE_TYPE *ele1;
    pthread_mutex_lock(&q->mutex);
    for(;q->BufCount > 0;) {
        ele1 = q->FirstBuf;
        if (ele1 != NULL) {
            q->FirstBuf = ele1->next;
            if (!q->FirstBuf)
                q->LastBuf = NULL;
        q->BufCount--; 
        *pBufHeader = ele1->pBufHeader;
        q->Offset = 0;
            q->BufFlag = 1;
            q->pCurBufHeader = ele1->pBufHeader;
            free(ele1);
            break;
        } 
    }
    pthread_mutex_unlock(&q->mutex);
    return 0;
}
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
void OMX_Buf_Q_Cur_Set(OMX_BUF_Q_TYPE * q,  OMX_BUFFERHEADERTYPE * pBufHeader)
{
    q->pCurBufHeader = pBufHeader;
    q->Offset = 0;
    q->BufFlag = 1;
}
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
void OMX_Buf_Q_Cur_Reset(OMX_BUF_Q_TYPE * q)
{
    q->Offset = 0;
    q->BufFlag = 0;
    q->pCurBufHeader = NULL;
}
