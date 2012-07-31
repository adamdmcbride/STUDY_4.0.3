/* TELINK 7619 RIL
 *
 * Based on reference-ril by The Android Open Source Project.
 *
 * Heavily modified for TELINK 7619 modems.
 * Author: dongdong.wang<dongdong.wang@nufront.com>
 */
	 
#ifndef TELINK_RIL_PDP_H
#define TELINK_RIL_PDP_H 1

/* pathname returned from RIL_REQUEST_SETUP_DATA_CALL / RIL_REQUEST_SETUP_DEFAULT_PDP */
#define PPP_TTY_PATH "ppp0"
#define PPP_OPERSTATE_PATH "/sys/class/net/ppp0/operstate"
#define PROPERTY_PPPD_EXIT_CODE "net.gprs.ppp-exit"
#define PROPERTY_SVC_PPPD_CODE "init.svc.pppd_gprs"
#define POLL_PPP_SYSFS_SECONDS  4
#define POLL_PPP_SYSFS_RETRY    5
#define DATA_CALL_DEVICE "/dev/ttyACM0"
#define SERVICE_PPPD_GPRS "pppd_gprs"
#define RIL_CID_IP 1
#define getNWType(data) ((data) ? (data) : "IP")

#define DATA_NOT_CONNECTED 0
#define DATA_CONNECTED 1

/*
 * variable and defines to keep track of preferred network type
 * the PREF_NET_TYPE defines correspond to CFUN arguments for
 * different radio states
	 0 GSM single mode
	 1 GSM / UMTS Dual mode
	 2 UTRAN (UMTS)
 */
#define PREF_NET_TYPE_2G 0
#define PREF_NET_TYPE_dual 1
#define PREF_NET_TYPE_3G 2

void requestSetupDefaultPDP(void *data, size_t datalen, RIL_Token t);

void requestDeactivateDefaultPDP(void *data, size_t datalen, RIL_Token t);
	 
int getPppdPid(char* name);

void requestQueryAvailableNetworks(void *data, size_t datalen, RIL_Token t);

char *stateToString(int state);

void requestSetNetworkSelectionAutomatic(void *data, size_t datalen, RIL_Token t);

void requestSetNetworkSelectionManual(void *data, size_t datalen,RIL_Token t);

void requestGetPreferredNetworkType(void *data, size_t datalen,RIL_Token t);

void requestSetPreferredNetworkType(void *data, size_t datalen,RIL_Token t);

struct operatorPollParams {
    RIL_Token t;
    int loopcount;
};

#endif




