/* TELINK 7619 RIL
 *
 * Based on reference-ril by The Android Open Source Project.
 *
 * Heavily modified for TELINK 7619 modems.
 * Author: dongdong.wang<dongdong.wang@nufront.com>
 */
#ifndef TELINK_RIL_REFERENCE_RIL_H
#define TELINK_RIL_REFERENCE_RIL_H 1

void setRadioState(RIL_RadioState newState);

static void onRequest (int request, void *data, size_t datalen, RIL_Token t);

static RIL_RadioState currentState();

static int onSupports (int requestCode);

static void onCancel (RIL_Token t);

static const char *getVersion();

static int isRadioOn();

static int getCardStatus(RIL_CardStatus_v6 **pp_card_status);

static void freeCardStatus(RIL_CardStatus_v6 *p_card_status);

static void onDataCallListChanged(void *param);

static void pollSIMState (void *param);

//telink misc request
void requestScreenState(void *data, size_t datalen, RIL_Token t);

#endif

