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

#include "telink-call.h"

#define LOG_TAG "RIL"
#include <utils/Log.h>

void requestDial(void *data, size_t datalen, RIL_Token t)
{
    RIL_Dial *p_dial;
    char *cmd;
    const char *clir;
    int ret;

    p_dial = (RIL_Dial *)data;

    switch (p_dial->clir) {
        case 1: clir = "I"; break;  /*invocation*/
        case 2: clir = "i"; break;  /*suppression*/
        default:
        case 0: clir = ""; break;   /*subscription default*/
    }

    asprintf(&cmd, "ATD%s%s;", p_dial->address, clir);

    ret = at_send_command(cmd, NULL);

    free(cmd);

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

void requestHangup(void *data, size_t datalen, RIL_Token t)
{
    int *p_line;

    int ret;
    char *cmd;

    p_line = (int *)data;

    // 3GPP 22.030 6.5.5
    // "Releases a specific active call X"
    asprintf(&cmd, "AT+CHLD=1%d", p_line[0]);

    ret = at_send_command(cmd, NULL);

    free(cmd);

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

void requestQueryClirStatus(void *data, size_t datalen, RIL_Token t)
{
	ATResponse *p_response = NULL;
	int err;
	int response[2];
	char *line;

	err = at_send_command_singleline("AT+CLIR?", "+CLIR:", &p_response);

	if (err < 0 || p_response->success == 0) {
		RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
		goto error;
	}

	line = p_response->p_intermediates->line;

	err = at_tok_start(&line);
	if (err < 0) goto error;

	err = at_tok_nextint(&line, &(response[0]));
	if (err < 0) goto error;

	err = at_tok_nextint(&line, &(response[1]));
	if (err < 0) goto error;

	RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));

	at_response_free(p_response);
	return;

error:
	LOGE("requestQueryClirStatus must never return an error when radio is on");
	RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
	at_response_free(p_response);
}


void requestQueryCallWaitingStatus(void *data, size_t datalen, RIL_Token t)
{
	int err;
	ATResponse *p_response = NULL;
	int response = 0;
	char *line;
	int classx = 0;
	RIL_CallWaittingInfo callwaittinginfo;
	ATLine *p_cur;

	callwaittinginfo.status = 0;
	callwaittinginfo.classx = 0;
	
	err = at_send_command_multiline("AT+CCWA=1,2", "+CCWA:", &p_response);
	if (err < 0 || p_response->success == 0) {
		goto error;
	}

	for (p_cur = p_response->p_intermediates; p_cur != NULL; p_cur = p_cur->p_next){
		char *line = p_cur->line;
		
		err = at_tok_start(&line);
		if (err < 0) {
			goto error;
		}

		err = at_tok_nextint(&line, &response);
		if (err < 0) {
			goto error;
		}
		
		if(response == 1) callwaittinginfo.status = response;
		
		err = at_tok_nextint(&line, &classx);
		if (err < 0) {
			goto error;
		}
		callwaittinginfo.classx = callwaittinginfo.classx + classx;
	}


	LOGE("Call Waiting Status response:{%d ,%d}",callwaittinginfo.status,callwaittinginfo.classx);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, &callwaittinginfo, sizeof(callwaittinginfo));
	at_response_free(p_response);
	return;
error:
	at_response_free(p_response);
	LOGE("requestQueryCallWaitingStatus error !!!");
	RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestSetCallWaiting(void *data, size_t datalen, RIL_Token t)
{
	int enable;
	int serviceclass;
	char *cmd;
	int err;
	ATResponse *p_response = NULL;

	enable = ((int *)data)[0];
	serviceclass = ((int *)data)[1];

	LOGD("enable is %d , and serviceclass is %d ",enable,serviceclass);
	
	asprintf(&cmd, "AT+CCWA=1,%d,%d", enable,serviceclass);
	err = at_send_command(cmd, &p_response);
	if (err < 0 || p_response->success == 0) {
		goto error;
	}

	free(cmd);

	at_response_free(p_response);
	RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
	return;
error:
	free(cmd);
	at_response_free(p_response);
	RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

void requestSetCallForwardStatus(void *data, size_t datalen, RIL_Token t)
{
	RIL_CallForwardInfo*  p_info;
	char *cmd;
	int err;
	char *line;
	int toa = 0;
	ATResponse *p_response = NULL;

	p_info = (RIL_CallForwardInfo*)data;

	LOGD("p_info->reason=%d,p_info->status=%d,p_info->number=%s,p_info->toa=%d,p_info->timeSeconds=%d,p_info->serviceClass=%d",
		p_info->reason,p_info->status,p_info->number,p_info->toa,p_info->timeSeconds,p_info->serviceClass);
	if(p_info->timeSeconds>0)
	{
		asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,%d,,,%d", p_info->reason,p_info->status,p_info->number,p_info->toa,p_info->serviceClass,p_info->timeSeconds);
	}
	else
	{
		asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,%d", p_info->reason,p_info->status,p_info->number,p_info->toa,p_info->serviceClass);
	}
	err = at_send_command(cmd, &p_response);

	if (err < 0 || p_response->success == 0) {
		goto error;
	}

	free(cmd);
	at_response_free(p_response);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
	return;

error:
	free(cmd);
	at_response_free(p_response);
	RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


void requestQueryCallForwardStatus(void *data, size_t datalen, RIL_Token t)
{
	RIL_CallForwardInfo*  p_info;
	char *cmd;
	int err;
	char *line;
	int response1 = 0;
	int response2 = 0;
	int toa = 0;
	char* fwd_number =NULL;
	int i=0;
	int str_len=0;

	RIL_CallForwardInfo result_info;
	RIL_CallForwardInfo* p_result[4];

	p_result[0]= &result_info;

	ATResponse *p_response = NULL;

	memset(&result_info,0,sizeof(RIL_CallForwardInfo));

	p_info = (RIL_CallForwardInfo*)data;

	asprintf(&cmd, "AT+CCFC=%d,2", p_info->reason);

	err = at_send_command_singleline(cmd, "+CCFC:", &p_response);

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

	err = at_tok_nextint(&line, &response2);

	if (err < 0) {
		goto error;
	}

	if (at_tok_hasmore(&line)) {
		err = at_tok_nextstr(&line, &fwd_number);
		str_len = strlen(fwd_number);
		LOGD("!!!!!fwd_number =%s,length =%d,err=%d",fwd_number,str_len,err);
		if (err < 0) {
			goto error;
		}
	}

	if (at_tok_hasmore(&line)) {
		err = at_tok_nextint(&line, &toa);
		LOGD("toa=%d,err=%d",toa,err);

		//if (err < 0) {
		// goto error;
		//}
	}

	result_info.reason = p_info->reason;
	result_info.serviceClass = 1;

	if(response1 == 0)
	{
		result_info.status=0;

	}
	else if (response1 == 1)
	{
		result_info.status=1;
		result_info.number=fwd_number;
		result_info.toa = toa;
	}
	else
		goto error;

	free(cmd);
	LOGD("!!!!!result_info.number=%s,result_info.toa =%d",result_info.number,result_info.toa);
	RIL_onRequestComplete(t, RIL_E_SUCCESS, &p_result, sizeof(p_result[0]));
	at_response_free(p_response);
	return;

error:
	free(cmd);
	at_response_free(p_response);
	RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

