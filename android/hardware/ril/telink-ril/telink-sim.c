/* TELINK 7619 RIL
 *
 * Based on reference-ril by The Android Open Source Project.
 *
 * Heavily modified for TELINK 7619 modems.
 * Author: dongdong.wang<dongdong.wang@nufront.com>
 */

#include <string.h>
#include <errno.h>

#include <telephony/ril.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "atchannel.h"
#include "at_tok.h"

#include "telink-sim.h"
#include "reference-ril.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>


static int fac_flag = 0;           /* add by alex */
static int pin_retries = 3;
static int puk_retries = 10;
static int pin2_retries = 3;
static int puk2_retries = 10;

static UICC_Type sim_type = UICC_TYPE_UNKNOWN;

static int stk_service_running = 0;

/**
 * Get the number of retries left for pin functions
 */
int getNumRetries (int request) {
    ATResponse *atresponse = NULL;
    int err;
    int num_retries = -1;

    err = at_send_command_singleline("AT+XPINCNT", "+XPINCNT:", &atresponse);
    if (err != 0) {
        LOGE("%s() AT+XPINCNT error", __func__);
        return -1;
    }

    switch (request) {
	case RIL_REQUEST_SET_FACILITY_LOCK:
       /* add by alex */
       if(fac_flag == 1)
       {
          sscanf(atresponse->p_intermediates->line, "+XPINCNT: %*d,%d",
                &num_retries);
          fac_flag = 0;
          break;
       }
       /* end */
    case RIL_REQUEST_ENTER_SIM_PIN:
    case RIL_REQUEST_CHANGE_SIM_PIN:
        sscanf(atresponse->p_intermediates->line, "+XPINCNT: %d",
               &num_retries);
        break;
	case RIL_REQUEST_ENTER_SIM_PIN2:
    case RIL_REQUEST_CHANGE_SIM_PIN2:
		sscanf(atresponse->p_intermediates->line, "+XPINCNT: %*d,%d",
               &num_retries);
        break;
    case RIL_REQUEST_ENTER_SIM_PUK:
        sscanf(atresponse->p_intermediates->line, "+XPINCNT: %*d,%*d,%d",
               &num_retries);
        break;
    case RIL_REQUEST_ENTER_SIM_PUK2:
        sscanf(atresponse->p_intermediates->line, "+XPINCNT: %*d,%*d,%*d,%d",
               &num_retries);
        break;
    default:
        num_retries = -1;
    break;
    }

    at_response_free(atresponse);
    return num_retries;
}

/**
 * Enter SIM PIN, might be PIN, PIN2, PUK, PUK2, etc.
 *
 * Data can hold pointers to one or two strings, depending on what we
 * want to enter. (PUK requires new PIN, etc.).
 *
 * FIXME: Do we need to return remaining tries left on error as well?
 *        Also applies to the rest of the requests that got the retries
 *        in later commits to ril.h.
 */
void  requestEnterSimPin(void*  data, size_t  datalen, RIL_Token  t,int request)
{
		ATResponse	 *p_response = NULL;
		int 		  err;
		char*		  cmd = NULL;
		const char**  strings = (const char**)data;;
    	int num_retries = -1;

		if ( datalen == 2*sizeof(char*)  ) {
			asprintf(&cmd, "AT+CPIN=\"%s\"", strings[0]);
		} else if ( datalen == 3*sizeof(char*) ) {
			asprintf(&cmd, "AT+CPIN=\"%s\",\"%s\"", strings[0], strings[1]);
		} else
			goto error;

		err = at_send_command(cmd,&p_response);
	
		free(cmd);

		if (err != 0 || p_response->success == 0) {
			goto error;
		} else {
			RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
			setRadioState(RADIO_STATE_SIM_READY);
			RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,NULL, 0);
		}
		at_response_free(p_response);

error:
		if(request == RIL_REQUEST_ENTER_SIM_PIN2 || request == RIL_REQUEST_ENTER_SIM_PUK2){
			num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PIN2);
			if(num_retries <= 0) {
				RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,NULL,NULL);
				num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PUK2);
				RIL_onRequestComplete(t, RIL_E_SIM_PUK2, &num_retries, sizeof(int));
				return;
			}
		}
        num_retries = getNumRetries(request);
		RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, &num_retries, sizeof(int *));
}

void requestChangePassword(void *data, size_t datalen, RIL_Token t,
                           char *facility, int request)
{
    int err = 0;
	ATResponse	 *p_response = NULL;
    char *oldPassword = NULL;
    char *newPassword = NULL;
    int num_retries = -1;
	 char *cmd;
    if (datalen != 3 * sizeof(char *) || strlen(facility) != 2)
        goto error;


    oldPassword = ((char **) data)[0];
    newPassword = ((char **) data)[1];
	asprintf(&cmd, "AT+CPWD=\"%s\",\"%s\",\"%s\"", facility,oldPassword, newPassword);

    err = at_send_command(cmd, &p_response);
    if (err != 0 || p_response->success == 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
	if(request == RIL_REQUEST_CHANGE_SIM_PIN2){
		num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PIN2);
		if(num_retries <= 0) {
			RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,NULL,NULL);
			num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PUK2);
			RIL_onRequestComplete(t, RIL_E_SIM_PUK2, &num_retries, sizeof(int));
			return;
		} 
	}
    num_retries = getNumRetries(request);
    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &num_retries, sizeof(int));
}


/**
 * RIL_REQUEST_SET_FACILITY_LOCK
 *
 * Enable/disable one facility lock.
 */
void requestSetFacilityLock(void *data, size_t datalen, RIL_Token t, int request)
{
	
		char* fac =NULL;
		char* mode =NULL;
		char* passwrd =NULL;
		char* cls = NULL;
		int err;
		char *cmd;
		int num_retries = -1;
		ATResponse *p_response = NULL;
	
		fac =  ((char **)data)[0] ;
		mode =	((char **)data)[1] ;
		passwrd =  ((char **)data)[2] ;
		cls =  ((char **)data)[3] ;

		LOGD("fac =%s,mode=%s,passwrd=%s,cls=%s",fac,mode,passwrd,cls);
        
        /* add by alex */
        if(strncmp(fac, "FD", 2) == 0)
        {
           fac_flag = 1;
        }
        /* end */
	
		asprintf(&cmd, "AT+CLCK=\"%s\",%s,\"%s\",%s", fac,mode,passwrd,cls);
	
		err = at_send_command(cmd, &p_response);
		if (err < 0 || p_response->success == 0) {
			goto error;
		}
	
		free(cmd);
		RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
		at_response_free(p_response);
		return;
	error:
		free(cmd);
		at_response_free(p_response);
		if(strncmp(fac, "FD", 2) == 0){
			num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PIN2);
			if(num_retries <= 0) {
				RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,NULL,NULL);
				num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PUK2);
				RIL_onRequestComplete(t, RIL_E_SIM_PUK2, &num_retries, sizeof(int));
				return;
			} 
		}		
		num_retries = getNumRetries(request);
	    RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &num_retries, sizeof(int));
}

/**
 * RIL_REQUEST_QUERY_FACILITY_LOCK
 *
 * Query the status of a facility lock state.
 */
void requestQueryFacilityLock(void *data, size_t datalen, RIL_Token t)
{
	
		char* fac =NULL;
		char* mode =NULL;
		char* passwrd =NULL;
		int err;
		char *line =NULL;
		int response1=0;
		char *cmd;
		ATResponse *p_response = NULL;
	
		fac =  ((char **)data)[0] ;
		mode =	((char **)data)[2] ;
		passwrd =  ((char **)data)[1] ;
		LOGD("requestQueryFacilityLock !!!!!fac =%s,mode=%s,passwrd=%s",fac,mode,passwrd);
	
		asprintf(&cmd, "AT+CLCK=\"%s\",2", fac);
	
		err = at_send_command_singleline(cmd, "+CLCK:", &p_response);
	
	
		if (err < 0 || p_response->success == 0) {
			goto error;
		}
	
		line = p_response->p_intermediates->line;
	
		err = at_tok_start(&line);
	
		if (err < 0) {
			goto error;
		}
	
		err = at_tok_nextint(&line, &response1);
	
		if (err < 0) {
			goto error;
		}
	
		free(cmd);
		if((SIM_PIN == getSIMStatus())&&(0==strcmp("SC",fac)))
		{
			response1=1;
		}
		RIL_onRequestComplete(t, RIL_E_SUCCESS, &(response1), sizeof(int));
		at_response_free(p_response);
		return;
	error:
		free(cmd);
		at_response_free(p_response);
		RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void  requestSIM_IO(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    RIL_SIM_IO_Response sr;
    int err;
    char *cmd = NULL;
    RIL_SIM_IO_v6 *p_args;
    char *line;
	int cvt_done = 0;
	UICC_Type UiccType = getUICCType();
	int num_retries = -1;

    memset(&sr, 0, sizeof(sr));

    p_args = (RIL_SIM_IO_v6 *)data;
	/* FIXME handle pin2 */
	if(p_args->pin2 != NULL){
		
		asprintf(&cmd, "AT+CPBS=\"FD\",\"%s\"",p_args->pin2);
		err = at_send_command(cmd,&p_response);
		
		if (err < 0 || p_response->success == 0) 
	        goto error;
	}

    if (p_args->data == NULL) {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,\"\",\"%s\"",
                    p_args->command, p_args->fileid,
                    p_args->p1, p_args->p2, p_args->p3,p_args->path);
    } else {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,\"%s\",\"%s\"",
                    p_args->command, p_args->fileid,
                    p_args->p1, p_args->p2, p_args->p3, p_args->data,p_args->path);
    }

    err = at_send_command_singleline(cmd, "+CRSM:", &p_response);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(sr.sw1));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(sr.sw2));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(sr.simResponse));
        if (err < 0) goto error;
    }

    /*
     * In case the command is GET_RESPONSE and cardtype is 3G SIM
     * convert to 2G FCP
     */
    if (p_args->command == 0xC0 && UiccType != UICC_TYPE_SIM) {
		LOGD("we convert the sim to fcp");
        err = convertSimIoFcp(&sr, &sr.simResponse);
        if (err < 0)
            goto error;
        cvt_done = 1; /* sr.simResponse needs to be freed */
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &sr, sizeof(sr));
    //at_response_free(p_response);
    free(cmd);

	at_response_free(p_response);
	if (cvt_done)
	free(sr.simResponse);
	return;

error:
	if(p_args->pin2 != NULL){
		num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PIN2);
		if(num_retries <= 0) {
			RIL_onUnsolicitedResponse(RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED,NULL,NULL);
			num_retries = getNumRetries(RIL_REQUEST_ENTER_SIM_PUK2);
			RIL_onRequestComplete(t, RIL_E_SIM_PUK2, &num_retries, sizeof(int));
		} else{
           RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, &num_retries, sizeof(int));
        }
	}else{
       RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
	
    at_response_free(p_response);
    free(cmd);
}

char char2nib(char c)
{
    if (c >= 0x30 && c <= 0x39)
        return c - 0x30;

    if (c >= 0x41 && c <= 0x46)
        return c - 0x41 + 0xA;

    if (c >= 0x61 && c <= 0x66)
        return c - 0x61 + 0xA;

    return 0;
}

int parseTlv(/*in*/ const char *stream,
             /*in*/ const char *end,
             /*out*/ struct tlv *tlv)
{
#define TLV_STREAM_GET(stream, end, p)  \
    do {                                \
        if (stream + 1 >= end)          \
            goto underflow;             \
        p = ((unsigned)char2nib(stream[0]) << 4)  \
          | ((unsigned)char2nib(stream[1]) << 0); \
        stream += 2;                    \
    } while (0)

    size_t size;

    TLV_STREAM_GET(stream, end, tlv->tag);
    TLV_STREAM_GET(stream, end, size);
    if (stream + size * 2 > end)
        goto underflow;
    tlv->data = &stream[0];
    tlv->end  = &stream[size * 2];
    return 0;

underflow:
    return -1;
#undef TLV_STREAM_GET
}


//telink sim i/o
static int fcp_to_ts_51011(/*in*/ const char *stream, /*in*/ size_t len,
        /*out*/ struct ts_51011_921_resp *out)
{
    const char *end = &stream[len];
    struct tlv fcp;
    int ret = parseTlv(stream, end, &fcp);
    const char *what = NULL;
#define FCP_CVT_THROW(_ret, _what)  \
    do {                    \
        ret = _ret;         \
        what = _what;       \
        goto except;        \
    } while (0)

    if (ret < 0)
        FCP_CVT_THROW(ret, "ETSI TS 102 221, 11.1.1.3: FCP template TLV structure");
    if (fcp.tag != 0x62)
        FCP_CVT_THROW(-1, "ETSI TS 102 221, 11.1.1.3: FCP template tag");

    /*
     * NOTE: Following fields do not exist in FCP template:
     * - file_acc
     * - file_status
     */

    memset(out, 0, sizeof(*out));
    while (fcp.data < fcp.end) {
        unsigned char fdbyte;
        size_t property_size;
        struct tlv tlv;
        ret = parseTlv(fcp.data, end, &tlv);
        if (ret < 0)
            FCP_CVT_THROW(ret, "ETSI TS 102 221, 11.1.1.3: FCP property TLV structure");
        property_size = (tlv.end - tlv.data) / 2;

        switch (tlv.tag) {
            case 0x80: /* File size, ETSI TS 102 221, 11.1.1.4.1 */
                /* File size > 0xFFFF is not supported by ts_51011 */
                if (property_size != 2)
                    FCP_CVT_THROW(-2, "3GPP TS 51 011, 9.2.1: Unsupported file size");
                /* be16 on both sides */
                ((char*)&out->file_size)[0] = TLV_DATA(tlv, 0);
                ((char*)&out->file_size)[1] = TLV_DATA(tlv, 1);
                break;
            case 0x83: /* File identifier, ETSI TS 102 221, 11.1.1.4.4 */
                /* Sanity check */
                if (property_size != 2)
                    FCP_CVT_THROW(-1, "ETSI TS 102 221, 11.1.1.4.4: Invalid file identifier");
                /* be16 on both sides */
                ((char*)&out->file_id)[0] = TLV_DATA(tlv, 0);
                ((char*)&out->file_id)[1] = TLV_DATA(tlv, 1);
                break;
            case 0x82: /* File descriptior, ETSI TS 102 221, 11.1.1.4.3 */
                /* Sanity check */
                if (property_size < 2)
                    FCP_CVT_THROW(-1, "ETSI TS 102 221, 11.1.1.4.3: Invalid file descriptor");
                fdbyte = TLV_DATA(tlv, 0);
                /* ETSI TS 102 221, Table 11.5 for FCP fields */
                /* 3GPP TS 51 011, 9.2.1 and 9.3 for 'out' fields */
                if ((fdbyte & 0xBF) == 0x38) {
                    out->file_type = 2; /* DF of ADF */
                } else if ((fdbyte & 0xB0) == 0x00) {
                    out->file_type = 4; /* EF */
                    out->file_status = 1; /* Not invalidated */
                    ++out->data_size; /* file_structure field is valid */
                    if ((fdbyte & 0x07) == 0x01) {
                        out->file_structure = 0; /* Transparent */
                    } else {
                        if (property_size < 5)
                            FCP_CVT_THROW(-1, "ETSI TS 102 221, 11.1.1.4.3: Invalid non-transparent file descriptor");
                        ++out->data_size; /* record_size field is valid */
                        out->record_size = TLV_DATA(tlv, 3);
                        if ((fdbyte & 0x07) == 0x06) {
                            out->file_structure = 3; /* Cyclic */
                        } else if ((fdbyte & 0x07) == 0x02) {
                            out->file_structure = 1; /* Linear fixed */
                        } else {
                            FCP_CVT_THROW(-1, "ETSI TS 102 221, 11.1.1.4.3: Invalid file structure");
                        }
                    }
                } else {
                    out->file_type = 0; /* RFU */
                }
                break;
        }
        fcp.data = tlv.end;
    }

 finally:
    return ret;

 except:
 #undef FCP_CVT_THROW
    LOGE("%s() FCP to TS 510 11: Specification violation: %s.", __func__, what);
    goto finally;
}

int convertSimIoFcp(RIL_SIM_IO_Response *sr, char **cvt)
{
    int err;
    /* size_t pos; */
    size_t fcplen;
    struct ts_51011_921_resp resp;
    void *cvt_buf = NULL;

    if (!sr->simResponse || !cvt) {
        err = -1;
        goto error;
    }

    fcplen = strlen(sr->simResponse);
    if ((fcplen == 0) || (fcplen & 1)) {
        err = -1;
        goto error;
    }

    err = fcp_to_ts_51011(sr->simResponse, fcplen, &resp);
    if (err < 0)
        goto error;

    cvt_buf = malloc(sizeof(resp) * 2 + 1);
    if (!cvt_buf) {
        err = -1;
        goto error;
    }

    err = binaryToString((unsigned char*)(&resp),
                   sizeof(resp), cvt_buf);
    if (err < 0)
        goto error;

    /* cvt_buf ownership is moved to the caller */
    *cvt = cvt_buf;
    cvt_buf = NULL;

finally:
    return err;

error:
    free(cvt_buf);
    goto finally;
}


int binaryToString(/*in*/ const unsigned char *binary,
                   /*in*/ size_t len,
                   /*out*/ char *string)
{
    int pos;
    const unsigned char *it;
    const unsigned char *end = &binary[len];
    static const char nibbles[] =
        {'0', '1', '2', '3', '4', '5', '6', '7',
         '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    if (end < binary)
        return -1;

    for (pos = 0, it = binary; it != end; ++it, pos += 2) {
        string[pos + 0] = nibbles[*it >> 4];
        string[pos + 1] = nibbles[*it & 0x0f];
    }
    string[pos] = 0;
    return 0;
}

UICC_Type getUICCType()
{
	//we got the type yet, so return immediate
	if(sim_type != UICC_TYPE_UNKNOWN) return sim_type;

	int err;
	ATResponse *p_response;
	char *line;
	int card_type;

	//telink proprietary AT
	err = at_send_command_singleline("AT+XUICC?","+XUICC:",&p_response);
	if (err < 0 || p_response->success == 0) {
		goto error;
	}
	
	line = p_response->p_intermediates->line;

	err = at_tok_start(&line);
	if (err < 0) {
		goto error;
	}

	err = at_tok_nextint(&line, &card_type);
	if (err < 0)
		goto error;

	switch(card_type){
		case 0:
			sim_type = UICC_TYPE_SIM;
			break;
		case 1:
			sim_type = UICC_TYPE_USIM;
			break;
		default:
			sim_type = UICC_TYPE_UNKNOWN;
			break;
	}

	return sim_type;
error:
	return UICC_TYPE_UNKNOWN;
}

//--------------------STK-------------------------------------
int get_stk_service_running(void)
{
    return stk_service_running;
}

void set_stk_service_running(int running)
{
    stk_service_running = running;
}

int init_stk_service(void)
{
    int err;
    int rilresponse = RIL_E_SUCCESS;

    err = at_send_command("AT+CFUN=6",NULL);
    if (err != 0) {
        LOGE("%s() Failed to activate (U)SAT profile", __func__);
        rilresponse = RIL_E_GENERIC_FAILURE;
    }

	LOGD("----init_STK_SERVICE-----%d",rilresponse);
    return rilresponse;
}
/**
 * RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
 *
 * Turn on STK unsol commands.
 */
void requestReportStkServiceIsRunning(void *data, size_t datalen, RIL_Token t)
{
    int ret;
    (void)data;
    (void)datalen;

    ret = init_stk_service();

    set_stk_service_running(1);

    RIL_onRequestComplete(t, ret, NULL, 0);
}

void reportStkProCommand(const char *s){
	char *temp = s;
	char *stkchar =NULL;
	int err = 0;
	err = at_tok_start(&temp);
	
	err = at_tok_nextstr(&temp, &stkchar);

	if(at_tok_hasmore(&temp)){
		return;}

	RIL_onUnsolicitedResponse (RIL_UNSOL_STK_PROACTIVE_COMMAND, stkchar, sizeof(char *));
}

void reportStkEvent(const char *s){
	char *temp = s;
	char *stkchar =NULL;
	int err = 0;
	err = at_tok_start(&temp);
	
	err = at_tok_nextstr(&temp, &stkchar);

	if(at_tok_hasmore(&temp))
		return;

	RIL_onUnsolicitedResponse (RIL_UNSOL_STK_EVENT_NOTIFY, stkchar, sizeof(char *));
}

void getCachedStkMenu(void)
{
    int err;
    char *resp;
    ATLine *cursor;
    ATResponse *p_response = NULL;

    err = at_send_command_multiline("AT+CSIM=22,\"8010000006FFFFFFFFFFFF\"", "", &p_response);

    if (err != 0)
        return;

cleanup:
    at_response_free(p_response);
}

/**
 * RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
 *
 * Requests to send a SAT/USAT envelope command to SIM.
 * The SAT/USAT envelope command refers to 3GPP TS 11.14 and 3GPP TS 31.111.
 */
void requestStkSendEnvelopeCommand(void *data, size_t datalen, RIL_Token t)
{
    char *line = NULL;
    char *stkResponse = NULL;
    int err;
    ATResponse *atresponse = NULL;
    const char *ec = (const char *) data;
    (void)datalen;
	char *cmd = NULL;
	
	LOGD("-----requestStkSendEnvelopeCommand---------%s",ec);
	
	asprintf(&cmd, "AT+SATE=\"%s\"", ec);

    err = at_send_command(cmd, NULL);

    if (err != 0)
        goto error;

	RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    //RIL_onRequestComplete(t, RIL_E_SUCCESS, stkResponse, sizeof(char *));
    at_response_free(atresponse);
    return;

error:
    at_response_free(atresponse);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
 *
 * Requests to send a terminal response to SIM for a received
 * proactive command.
 */
void requestStkSendTerminalResponse(void *data, size_t datalen,
                                    RIL_Token t)
{
    int err;
    int rilresponse = RIL_E_SUCCESS;
    //(void)datalen;
    const char *stkResponse = (const char *) data;

	LOGD("-----requestStkSendTerminalResponse---------%s---datalen : %d",stkResponse,datalen);
	int len = strlen(stkResponse);
	char *cmd = NULL;

	asprintf(&cmd, "AT+CSIM=%d,\"80140000%02x%s\"", len+10, len>>1,stkResponse);
/*
	if(get_stk_service_running() <= 1){
	    err = at_send_command("AT+STKTR=37,0,0", NULL);
		set_stk_service_running(10);
	}
	else
*/		err = at_send_command(cmd, NULL);

    if (err != 0)
        rilresponse = RIL_E_GENERIC_FAILURE;

    RIL_onRequestComplete(t, rilresponse, NULL, 0);
}

