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
#include <fcntl.h>

#define LOG_TAG "RIL"
#include <utils/Log.h>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cutils/properties.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <dirent.h>

#include "telink-pdp.h"

static int pppd = -1; //check pppd is connected 

void requestSetupDefaultPDP(void *data, size_t datalen, RIL_Token t)
{
    const char *apn, *user, *pass, *auth;
    char *addresses = NULL;
    char *gateways = NULL;
    char *dnses = NULL;
    const char *type = NULL;
    const char *disecho = "ATE0\r\n";
	char *cmd;
    RIL_Data_Call_Response_v6 response;
   	pid_t pid;

    (void) data;
    (void) datalen;

    char *ip_response[3] = { "1", PPP_TTY_PATH, ""};
	
    int err = -1;
	int cme_err = -1;
    int fd;
    char buffer[20];
    char exit_code[PROPERTY_VALUE_MAX];
    static char local_ip[PROPERTY_VALUE_MAX];
    static char dns1[PROPERTY_VALUE_MAX];
	
    int retry = POLL_PPP_SYSFS_RETRY;
    memset(&response, 0, sizeof(response));
	
    (void) data;
    (void) datalen;

    apn = ((const char **) data)[2];
    user = ((const char **) data)[3];
    pass = ((const char **) data)[4];
    auth = ((const char **) data)[5];
    type = getNWType(((const char **) data)[6]);

    //LOGD(" requestSetupDefaultPDP, apn=%s, username=%s, passwd=%s, auth=%s", apn?apn:"<NULL>", user?user:"<NULL>", pass?pass:"<NULL>", auth);
	LOGD("pppd is ------------- %d \n",pppd);

	if(pppd == 1)
    {
        // clean up any existing PPP connection before activating new PDP context
        LOGW("Stop existing PPPd before activating PDP");
		//stop pppd
		pid = getPppdPid("pppd");
		LOGD("get pppd pid is %d -- but will stop it and reconnect. \n",pid);
		if(pid >= 0) kill(pid, SIGINT);
        pppd = 0;
        sleep(3);
    }

    //s_lastPdpFailCause = PDP_FAIL_ERROR_UNSPECIFIED;
    LOGD("setup data connection to APN '%s'", apn);

	asprintf(&cmd, "AT+CGDCONT=%d,\"IP\",\"%s\"", RIL_CID_IP, apn);
	err = at_send_command(cmd, NULL);
	free(cmd);

    if (err != 0) {
        LOGE("%s() CGDCONT failed: %d", __func__, err);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }
	sleep(1);
		
	fd = open(DATA_CALL_DEVICE, O_RDWR);
		if (fd >= 0) {
			/* disable echo on serial ports */
			err = write(fd, disecho, strlen(disecho));
			if (err < 0) {
				LOGD("echo ATE0 err=%d\n,",err);
				goto ppp_error;
			}else{
				LOGD("echo ATE0 pass");
			}
			close(fd);
		} else {
			LOGE("### ERROR openning %s !", DATA_CALL_DEVICE);
			goto ppp_error;
		}

	sleep(1);

        //err = property_set("ctl.start", "pppd_gprs");
		err = system("pppd call gprs &");
        pppd = 1;
        if (err < 0)
        {
            LOGW("Can not start PPPd");
            goto ppp_error;
        }
        LOGD("PPPd run ...");
     // Wait for PPP interface ready
     sleep(9);

        do
        {
            // Check whether PPPD exit abnormally
            property_get(PROPERTY_PPPD_EXIT_CODE, exit_code, "");
            if(strcmp(exit_code, "") != 0)
            {
                LOGW("PPPd exit with code %s", exit_code);
                retry = 0;
                break;
            }

            fd  = open(PPP_OPERSTATE_PATH, O_RDONLY);
            if (fd >= 0)
            {
                buffer[0] = 0;
                read(fd, buffer, sizeof(buffer));
                close(fd);
                if(!strncmp(buffer, "up", strlen("up")) || !strncmp(buffer, "unknown", strlen("unknown")))
                {
                    // Should already get local IP address from PPP link after IPCP negotation
                    // system property net.ppp0.local-ip is created by PPPD in "ip-up" script
                    local_ip[0] = 0;
                    property_get("net.ppp0.local-ip", local_ip, "");
                    property_get("net.ppp0.dns1", dns1, "");
                    if(!strcmp(local_ip, ""))
                    {
                        LOGW("PPP link is up but no local IP is assigned - Will retry %d times after %d seconds", retry, POLL_PPP_SYSFS_SECONDS);
                    }
                    else
                    {
                        LOGD("PPP link is up with local IP address %s", local_ip);
                        // other info like dns will be passed to framework via property (set in ip-up script)
                        //ip_response[2] = local_ip;
         
						//set the reponse
					    response.addresses = local_ip;
					    response.dnses = dns1;
						//set default value
					    response.gateways = gateways;
					    response.ifname = "ppp0"; //"/dev/ttyACM0";
					    response.active = 2;
					    response.type = (char *) type;
					    response.status = 0;
					    response.cid = 1;
					    response.suggestedRetryTime = -1;
		                // now we think PPPd is ready
		                break;
                  }
                }
                else
                {
                    LOGW("PPP link status in %s is %s. Will retry %d times after %d seconds", \
                         PPP_OPERSTATE_PATH, buffer, retry, POLL_PPP_SYSFS_SECONDS);
                }
            }
            else
            {
                LOGW("Can not detect PPP state in %s. Will retry %d times after %d seconds", \
                     PPP_OPERSTATE_PATH, retry-1, POLL_PPP_SYSFS_SECONDS);
            }
            sleep(POLL_PPP_SYSFS_SECONDS);
        }
        while (--retry);

        if(retry == 0)
            goto ppp_error;

	    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(response));
	    free(addresses);
	    free(gateways);
	    free(dnses);
        return;

ppp_error:
        /* If PPPd is already launched, stop it */
        if(pppd)
        {
	        pid = getPppdPid("pppd");
			LOGD("pppd pid is:%d \n",pid);
			if(pid >= 0) kill(pid, SIGINT);
            pppd = 0;
	     	LOGD("stop pppd and set it to %d",pppd);
        }
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
	    free(addresses);
	    free(gateways);
	    free(dnses);
}

int isdir(char *path)
{
    struct stat buf;
    if(lstat(path,&buf)<0)
    {
        printf("lstat err for %s \n",path);
        return 0;
    }
    if(S_ISDIR(buf.st_mode)||S_ISLNK(buf.st_mode))
    {
        return 1;
    }
    return 0;
}

char* readName(char *filename)
{
	FILE *file;
	static char buf[1024];
	file=fopen(filename,"r");
	if(!file)return NULL;
	while(fgets(buf,sizeof(buf),file))
	{
	  if(strncmp(buf,"Name:",5)==0)
	        {
	                printf("%s\n",buf);
	                break;
	        }
	}
	fclose(file);
	return buf;
}
int getPppdPid(char* name){
	char buf1[1024],buf2[1024];
	char path1[1024],path2[1024],*str,*ptr;
	DIR *db,*directory;
	struct dirent *p;
	int len;
	int i;
	int pid;

	db=opendir("/proc/");
	if(db==NULL)return -1;
	while ((p=readdir(db)))
	  {
	    if((strcmp(p->d_name,".")==0)||(strcmp(p->d_name,"..")==0))
	       continue;
	    else
	     {
	        memset(buf1,0,sizeof(buf1));
	        sprintf(buf1,"/proc/%s",p->d_name);
	        if(isdir(buf1))
	        {
	          memset(buf2,0,sizeof(buf2));
	          sprintf(buf2,"%s/status",buf1);
		    ptr = readName(buf2);
		    if(ptr ==NULL) continue;
		    //printf("------ptr is %s \n",ptr);	 
			str = strtok(ptr,":");
			str = strtok(NULL,":");
			if(strstr(str, name)) {
				//LOGD("--------------------------find!!! and pid is %s \n",p->d_name);
				pid = atoi(p->d_name);
				//LOGD("-------------------------int  pid is %d \n",pid);
				closedir(db);
				//LOGD("------------------------1");
				return pid;
				}
			else
		       continue;

	        }
	     }
	 }

	closedir(db);
	return -1;
}

/* CHECK There are several error cases if PDP deactivation fails
 * 24.008: 8, 25, 36, 38, 39, 112
 */
void requestDeactivateDefaultPDP(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int enap = 0;
    int err, i;
    char *line;
    (void) data;
    (void) datalen;
   	pid_t pid;
	char *cmd;
	int fd;
   
    err = at_send_command_singleline("AT+CGACT?", "+CGACT:", &p_response);
    if (err != 0)
        goto error;

    line = p_response->p_intermediates->line;
    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &enap);
    if (err < 0)
        goto error;

    if (enap == DATA_CONNECTED) {

		asprintf(&cmd, "AT+CGACT=0,1");
        err = at_send_command(cmd, NULL); /* TODO: can return CME error */
		free(cmd);
		
        if (err != 0)//at_get_error_type(err) != CME_ERROR
            goto error;

		//stop pppd
		pid = getPppdPid("pppd");
		LOGD("get pppd pid is %d -- and stop it. \n",pid);
		if(pid >= 0) kill(pid, SIGINT);
		sleep(1);

		fd	= open(PPP_OPERSTATE_PATH, O_RDONLY);
		//todo 
		if (fd >= 0) sleep(2);
		else LOGD("ppp disconneced ");
		
		pppd = 0;
    }else if (enap == DATA_NOT_CONNECTED){
		LOGD("%s  data not connected.",__func__);
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


void requestQueryAvailableNetworks(void *data, size_t datalen, RIL_Token t)
{
#if 1
	int err;
	int num = 0, i;
	int state;
	ATResponse *p_response = NULL;
	char *response[8];
	char *line;

	err = at_send_command_singleline("AT+COPS=?", "+COPS:", &p_response);

	if (err < 0 || p_response->success == 0) {
		goto error;
	}

	line = p_response->p_intermediates->line;

	while((line = strchr(line, '('))){
		line++;
		num++;
	}

	line = p_response->p_intermediates->line;

	err = at_tok_start(&line);

	if (err < 0) {
		goto error;
	}

	for (i=0; i<num; i++){
		line = strchr(line, '(');
		line++;

		err = at_tok_nextint(&line, &state);
		if (err < 0) {
			goto error;
		}

		response[i*4+3] = stateToString(state);

		err = at_tok_nextstr(&line, &response[i*4+0]);
		if (err < 0) {
			goto error;
		}

		err = at_tok_nextstr(&line, &response[i*4+1]);
		if (err < 0) {
			goto error;
		}

		err = at_tok_nextstr(&line, &response[i*4+2]);
		if (err < 0) {
			goto error;
		}
	}

	RIL_onRequestComplete(t, RIL_E_SUCCESS, response, num*4*sizeof(char *));
	at_response_free(p_response);
	return;

error:
	at_response_free(p_response);
	RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
#endif
}

char *stateToString(int state)
{
	switch (state){
		case 0:
			return "unknown";
		case 1:
			return "available";
		case 2:
			return "current";
		case 3:
			return "forbidden";
		default:
			return "unknown";
	}
}

/**
 * RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
 *
 * Specify that the network should be selected automatically.
*/
void requestSetNetworkSelectionAutomatic(void *data, size_t datalen, RIL_Token t)
{
    (void) data; (void) datalen;
    int err = 0;
    ATResponse *p_response = NULL;

	err = at_send_command("AT+COPS=0", &p_response);
	
	if (err < 0 || p_response->success == 0) {
		RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
	} else {
		RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
	}
	
	at_response_free(p_response);

	return;
}

/**
 * RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL
 *
 * Manually select a specified network.
 *
 * The radio baseband/RIL implementation is expected to fall back to
 * automatic selection mode if the manually selected network should go
 * out of range in the future.
 */
void requestSetNetworkSelectionManual(void *data, size_t datalen,RIL_Token t)
{
    (void) datalen;
    int err = 0;
    const char *mccMnc = (const char *) data;
	char * cmd = NULL;
	ATResponse *atresponse;

    /* Check inparameter. */
    if (mccMnc == NULL)
        goto error;

	asprintf(&cmd, "AT+COPS=1,2,\"%s\"", mccMnc);

    /* Build and send command. */
    err = at_send_command(cmd, &atresponse);

    if (err < 0 || atresponse->success == 0)
        goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

/**
 * RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
 *
 * Query the preferred network type (CS/PS domain, RAT, and operation mode)
 * for searching and registering.
	 0 GSM single mode
	 1 GSM / UMTS Dual mode
	 2 UTRAN (UMTS)
 */
void requestGetPreferredNetworkType(void *data, size_t datalen,
                                    RIL_Token t)
{
    (void) data; (void) datalen;
    int err = 0;
    int response = 0;
    int prefer_net;
    char *line;
    ATResponse *atresponse;

    err = at_send_command_singleline("AT+XRAT?", "+XRAT:", &atresponse);
    if (err < 0 || atresponse->success == 0)
        goto error;

    line = atresponse->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0)
        goto error;

    err = at_tok_nextint(&line, &prefer_net);
    if (err < 0)
        goto error;

    if (prefer_net < 0 || prefer_net >= 7)
        goto error;

    switch (prefer_net) {
    case PREF_NET_TYPE_2G:
        response = PREF_NET_TYPE_GSM_ONLY;
        break;
    case PREF_NET_TYPE_3G:
        response = PREF_NET_TYPE_WCDMA;
        break;
    case PREF_NET_TYPE_dual:
        response = PREF_NET_TYPE_GSM_WCDMA_AUTO;
        break;
    default:
        response = PREF_NET_TYPE_GSM_WCDMA_AUTO;
        break;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));

finally:
    at_response_free(atresponse);
    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    goto finally;
}

void requestSetPreferredNetworkType(void *data, size_t datalen,RIL_Token t){
		(void) data; (void) datalen;
		int err = 0;
		ATResponse *atresponse;
		int act = ((int *)data)[0];
		char * cmd = NULL;
		int prefer_act = 2; //preferred network type is WCDMA when dual mode

		if (act < 0 || act >= 3)
			goto error;

		if(act == 1)
			asprintf(&cmd, "AT+XRAT=%d,%d",act,prefer_act);
		else
			asprintf(&cmd, "AT+XRAT=%d",act);

		err = at_send_command(cmd, &atresponse);

		if (err < 0 || atresponse->success == 0)
			goto error;

		RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
		at_response_free(atresponse);
		return;

	error:
		RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
		at_response_free(atresponse);
}

