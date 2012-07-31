/* TELINK 7619 RIL
 *
 * Based on reference-ril by The Android Open Source Project.
 *
 * Heavily modified for TELINK 7619 modems.
 * Author: dongdong.wang<dongdong.wang@nufront.com>
 */

#ifndef TELINK_RIL_CALL_H
#define TELINK_RIL_CALL_H 1

typedef struct {
    int status;
    int classx;
}RIL_CallWaittingInfo;

void requestHangup(void *data, size_t datalen, RIL_Token t);

void requestDial(void *data, size_t datalen, RIL_Token t);

void requestQueryClirStatus(void *data, size_t datalen, RIL_Token t);

void requestQueryCallWaitingStatus(void *data, size_t datalen, RIL_Token t);

void requestSetCallWaiting(void *data, size_t datalen, RIL_Token t);

void requestSetCallForwardStatus(void *data, size_t datalen, RIL_Token t);

void requestQueryCallForwardStatus(void *data, size_t datalen, RIL_Token t);

#endif
