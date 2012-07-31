/* TELINK 7619 RIL
 *
 * Based on reference-ril by The Android Open Source Project.
 *
 * Heavily modified for TELINK 7619 modems.
 * Author: dongdong.wang<dongdong.wang@nufront.com>
 */
 
#ifndef TELINK_RIL_SIM_H
#define TELINK_RIL_SIM_H 1

#define TLV_DATA(tlv, pos) (((unsigned)char2nib(tlv.data[(pos) * 2 + 0]) << 4) | \
                            ((unsigned)char2nib(tlv.data[(pos) * 2 + 1]) << 0))

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))

struct tlv {
    unsigned    tag;
    const char *data;
    const char *end;
};

typedef enum {
    UICC_TYPE_UNKNOWN,
    UICC_TYPE_SIM,
    UICC_TYPE_USIM,
} UICC_Type;

typedef enum {
    SIM_ABSENT = 0,
    SIM_NOT_READY = 1,
    SIM_READY = 2, /* SIM_READY means the radio state is RADIO_STATE_SIM_READY */
    SIM_PIN = 3,
    SIM_PUK = 4,
	SIM_NETWORK_PERSO = 5,				/* Network Personalization lock */
	SIM_PIN2 = 6,						/* SIM PIN2 lock */
	SIM_PUK2 = 7,						/* SIM PUK2 lock */
	SIM_NETWORK_SUBSET_PERSO = 8,		/* Network Subset Personalization */
	SIM_SERVICE_PROVIDER_PERSO = 9, 	/* Service Provider Personalization */
	SIM_CORPORATE_PERSO = 10,			/* Corporate Personalization */
	SIM_SIM_PERSO = 11, 				/* SIM/USIM Personalization */
	SIM_STERICSSON_LOCK = 12,			/* ST-Ericsson Extended SIM */
	SIM_BLOCKED = 13,					/* SIM card is blocked */
	SIM_PERM_BLOCKED = 14,				/* SIM card is permanently blocked */
	SIM_NETWORK_PERSO_PUK = 15, 		/* Network Personalization PUK */
	SIM_NETWORK_SUBSET_PERSO_PUK = 16,	/* Network Subset Perso. PUK */
	SIM_SERVICE_PROVIDER_PERSO_PUK = 17,/* Service Provider Perso. PUK */
	SIM_CORPORATE_PERSO_PUK = 18,		/* Corporate Personalization PUK */
	SIM_SIM_PERSO_PUK = 19, 			/* SIM Personalization PUK (unused) */
	SIM_PUK2_PERM_BLOCKED = 20			/* PUK2 is permanently blocked */
} SIM_Status;

struct ts_51011_921_resp {
    uint8_t   rfu_1[2];
    uint16_t  file_size; /* be16 */
    uint16_t  file_id;   /* be16 */
    uint8_t   file_type;
    uint8_t   rfu_2;
    uint8_t   file_acc[3];
    uint8_t   file_status;
    uint8_t   data_size;
    uint8_t   file_structure;
    uint8_t   record_size;
} __attribute__((packed));

//telink add function
void  requestSIM_IO(void *data, size_t datalen, RIL_Token t);

void requestEnterSimPin(void *data, size_t datalen, RIL_Token t, int request);

void requestChangePassword(void *data, size_t datalen, RIL_Token t, char *facility, int request);

void requestChangePassword(void *data, size_t datalen, RIL_Token t, char *facility, int request);

void requestQueryFacilityLock(void *data, size_t datalen, RIL_Token t);

void requestSetFacilityLock(void *data, size_t datalen, RIL_Token t, int request);

int getNumRetries(int request);

SIM_Status getSIMStatus();

char char2nib(char c);

int convertSimIoFcp(RIL_SIM_IO_Response *sr, char **cvt);

UICC_Type getUICCType();

int binaryToString(/*in*/ const unsigned char *binary,
                   /*in*/ size_t len,
                   /*out*/ char *string);

int parseTlv(/*in*/ const char *stream,
             /*in*/ const char *end,
             /*out*/ struct tlv *tlv);


//--------------------STK-------------------------------------
int get_stk_service_running(void);

void set_stk_service_running(int running);

int init_stk_service(void);

void getCachedStkMenu(void);

void requestReportStkServiceIsRunning(void *data, size_t datalen, RIL_Token t);

void reportStkProCommand(const char *s);

#endif

