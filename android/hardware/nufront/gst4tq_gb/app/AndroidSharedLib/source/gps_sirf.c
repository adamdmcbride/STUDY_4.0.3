/**
 * @addtogroup lpl3.0
 * @{
 */


/*
 *
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2011 by SiRF Technology, a CSR plc Company.
 *    All rights reserved.
 *
 *    This Software is protected by United States copyright laws and
 *    international treaties.  You may not reverse engineer, decompile
 *    or disassemble this Software.
 *
 *    WARNING:
 *    This Software contains SiRF Technology Inc.s confidential and
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this
 *    Software without SiRF Technology, Inc.s  express written
 *    permission.   Use of any portion of the contents of this Software
 *    is subject to and restricted by your signed written agreement with
 *    SiRF Technology, Inc.
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/app/AndroidSharedLib/source/gps_sirf.c $
 *
 *  $DateTime: 2012/02/20 11:03:46 $
 *
 *  $Revision: #21 $
 */

/* SiRF Software Interface Header files */


#include "gps_sirf.h"
#include "gps_sirf_cpa.h"
#include "gps_logging.h"
#include "cpaclient.h"

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****                  I N T E R F A C E                    *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

#define AGPS_GPRSIF_LEN 20 // GPRS interface like: ccinet0


static char g_gprs_interface[30];

#define SET_CAPABILITIES_MSB 0
#define SET_CAPABILITIES_MSA 1
#define SET_CAPABILITIES_ALL 2
static int gCapabilities = SET_CAPABILITIES_ALL;

typedef struct
{
	tSIRF_BOOL   is_valid;
	tSIRF_UINT32 freq;    /**< first spur frequency */
	tSIRF_UINT16 cno;     /**< first spur power */
}t_test_mode_data;

static struct
{
	tSIRF_UINT32 freq;    /**< first spur frequency */
	tSIRF_UINT16 cno;     /**< first spur power */
} temp_tm_data;

t_test_mode_data final_tm_data;
t_test_mode_data tm_data[10];
tSIRF_UINT8      tm_count = 0;
tSIRF_UINT32     test_count = 0;


static tSIRF_MSG_SSB_TEST_MODE_DATA   tm4_struct;
static tSIRF_INT32 testModeType = 0;
static tSIRF_INT32 testModeSvId = 0;

static int g_cpaLcsInvokeHandle = (int) NULL;

/* Forward declaration */

#define NI_TEST_BUILD 0 //Do not activate unless you know what it is
#define CW_SAMPLES 3

static unsigned RilRequestPending = 0;
static unsigned Noupdatetosessionhandler = 0;
static unsigned stopupdatepending = 0;
static volatile int android_status_report_ready = 0;

static LSM_BOOL g_del_aiding_request = LSM_FALSE;

char ca_path[256];
extern void set_gprs_interface(char * s);

int getNotificationId(void)
{
	static int notification_id = 0;
	return ++notification_id;
}

static void report_gps_pwr_on(void)
{
	gpsStatus.status = GPS_STATUS_ENGINE_ON;
	if(gpsCallbacks.status_cb)
	{
		SIRF_LOGI("GPS_STATUS_ENGINE_ON");
		gpsCallbacks.status_cb(&gpsStatus);
	}

	gpsStatus.status = GPS_STATUS_SESSION_BEGIN;
	if(gpsCallbacks.status_cb)
	{
		SIRF_LOGI("GPS_STATUS_SESSION_BEGIN");
		gpsCallbacks.status_cb(&gpsStatus);
	}

	return;
}

static void report_gps_pwr_off(void)
{
	gpsStatus.status = GPS_STATUS_SESSION_END;
	if(gpsCallbacks.status_cb)
	{
		SIRF_LOGI("GPS_STATUS_SESSION_END");
		gpsCallbacks.status_cb(&gpsStatus);
	}

	gpsStatus.status = GPS_STATUS_ENGINE_OFF;
	if(gpsCallbacks.status_cb)
	{
		SIRF_LOGI("GPS_STATUS_ENGINE_OFF");
		gpsCallbacks.status_cb(&gpsStatus);
	}

	return;
}

static void android_gps_pwr_control(int OnOff)
{
	static int onoff_status = ANDROID_GPS_PWR_OFF;

	//SIRF_LOGD("%s called", __FUNCTION__);

	if(onoff_status == OnOff)
	{
		/* Do nothing!!! */
		return;
	}

	onoff_status = OnOff;

	if(onoff_status == ANDROID_GPS_PWR_ON)
	{
		report_gps_pwr_on();
	}
	else if(onoff_status == ANDROID_GPS_PWR_OFF)
	{
		report_gps_pwr_off();
	}
	else
	{
		SIRF_LOGD("ERROR : OnOff = %d", OnOff);
	}

	return;
}

LSM_SETcapabilities setcap;


int configureSETCapabilities(int mode)
{
	switch(mode)
	{
		case SET_CAPABILITIES_MSB:
			setcap.msAssisted = 0;
			setcap.msBased = 1;
			break;

		case SET_CAPABILITIES_MSA:
			setcap.msAssisted = 1;
			setcap.msBased =0;
			break;
		case SET_CAPABILITIES_ALL:
			setcap.msAssisted = 1;
			setcap.msBased = 1;
			break;
		default:
			setcap.msAssisted = 0;
			setcap.msBased = 1;
			break;
	}

	setcap.autonomous = 1;
	setcap.eCID = 0;

	LSM_setCapabilities(&setcap);
	return 0;

}
static int android_mo_session_start(void)
{

	if(GPS_POSITION_MODE == GPS_POSITION_MODE_MS_BASED)
	{
		configureSETCapabilities(gCapabilities);
		SIRF_LOGI("Position Mode : GPS_POSITION_MODE_MS_BASED");
		if(LSM_TRUE != requestAllRilInfo())
		{
			SIRF_LOGE("requestAllRilInfo Fail!!!!!");
		}
	}
	else if(GPS_POSITION_MODE == GPS_POSITION_MODE_MS_ASSISTED)
	{
		configureSETCapabilities(gCapabilities);
		SIRF_LOGI("Position Mode : GPS_POSITION_MODE_MS_ASSISTED");
		if(LSM_TRUE != requestAllRilInfo())
		{
			/* Setting SET capapbilities to default */
			g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
			configureSETCapabilities(gCapabilities);
			g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSB;

			/* Selecting standalone method */
			g_GSD4tConfig.aidingType = LSM_NO_AIDING;
			g_GSD4tConfig.networkStatus = 0;
			SIRF_LOGE("requestAllRilInfo Fail!!!!! - Fallback to Standalone from MSA");
		}

	}
	else
	{
		SIRF_LOGI("Position Mode : GPS_POSITION_MODE_STANDALONE");
	}

	if (0 == getLSMSessionCfgData(&LSM_SessionCfg))
	{
		SIRF_LOGE("ERROR : cannot obtain session cfg data");
		return SIRF_FAILURE;
	}

	/* Session Start!!!!!!!!!!!! */
	memset(&svStatus, 0 , sizeof(SiRFGpsSvStatus));  /* inititialize varialble per every session */
	memset(&locationInfo, 0, sizeof(GpsLocation));
	memset(&androidSVStatus, 0, sizeof(GpsSvStatus));
	locationInfo.size = sizeof(GpsLocation);

	posCnt = 0;
	posQoSMet = 0;
	saved_used_in_fix_mask = 0;

	/* Enable SiRFAware */
	//LSM_SessionCfg.SessionCfgBitMap.isSiRFAwareSet =1;
	//LSM_SessionCfg.isSiRFAwareAvailable = 1;

	/* fix session handler crashes during the data connection request */
	pthread_mutex_lock(&sAndroidSessionMutex);
	if(android_session_check == ANDROID_CP_SESSION)
	{
		g_SessionID = INVAILD_SESSION_HANDLE;
		pthread_mutex_unlock(&sAndroidSessionMutex);
		return SIRF_FAILURE;
	}
	pthread_mutex_unlock(&sAndroidSessionMutex);

	g_SessionID= LSM_Start(&LSM_SessionCfg);
	if (INVAILD_SESSION_HANDLE == g_SessionID)
	{
		SIRF_LOGE("sirf_gps_start : LSM_Start Failed!!!!");
		return -1;
	}

	return SIRF_SUCCESS;
}

static int android_mo_session_stop()
{
	unsigned long cur_SessionID;

	if(g_SessionID != INVAILD_SESSION_HANDLE)
	{
		cur_SessionID = g_SessionID;
		g_SessionID = INVAILD_SESSION_HANDLE;
		posQoSMet = 0;

		SIRF_LOGI("LSM_Stop Called");
		LSM_Stop(cur_SessionID, LSM_FALSE);  // let it be not urgent
		//SIRF_PAL_OS_THREAD_Sleep(1 * 1000);  // No sleep allowed pls.
		if(stopupdatepending)
		{
			stopupdatepending = 0;
			tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release(semwaitstop); //wait to close for 3 sec max
			if ( result != SIRF_SUCCESS )
			{
				SIRF_LOGD( "timeout: semwaitstop Release : 0x%X\n", result );
				//return SIRF_FAILURE;
			}
			SIRF_LOGD("semaphore rel for stop");
		}
		else
		{
			SIRF_LOGD("No semaphore to rel in stop");
		}
	}
	else
	{
		SIRF_LOGD("No running Session, Do nothing!!!");
		return SIRF_FAILURE;
	}

	return SIRF_SUCCESS;
}

static int android_ni_session_start(void)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if(LSM_TRUE != requestAllRilInfo())
	{
		SIRF_LOGE("requestAllRilInfo Fail!!!!");

		return SIRF_FAILURE;
	}

	/* fix session handler crashes during the data connection request */
	pthread_mutex_lock(&sAndroidSessionMutex);
	if(android_session_check == ANDROID_CP_SESSION)
	{
		g_NI_Session = NO_NI_SESSION;
		pthread_mutex_unlock(&sAndroidSessionMutex);
		return SIRF_FAILURE;
	}
	pthread_mutex_unlock(&sAndroidSessionMutex);


	SIRF_LOGI("SUPL NI Session starts");

	SIRF_LOGD("msg size = %d", supl_init_msg_length);
	CP_Reset(); 
	if(LSM_TRUE != CP_SendNetMessage(LSM_SUPL_RRLP_OR_RRC, LSM_RRC_STATE_IDLE, NO_EMERGENCY, \
				(LSM_UINT8 *)SUPL_INIT_MESSAGE, supl_init_msg_length))
	{
		SIRF_LOGE("CP_SendNetMessage Fail!!!!");
		return SIRF_FAILURE;
	}
	else
	{
		g_NI_Session = NI_SESSION_RUNNING;
	}

	return SIRF_SUCCESS;
}

static int android_ni_session_stop(void)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if(LSM_FALSE == CP_SendMeasurementTerminate (LSM_SUPL_RRLP_OR_RRC))
	{
		SIRF_LOGE("CP_SendMeasurementTerminate Failed for NI");
	}

	g_NI_Session = NO_NI_SESSION;
	SIRF_LOGD("SUPL NI Session Terminated");

	return SIRF_SUCCESS;
}

static int android_session_handler_resume(eAndroidSession request)
{
	//SIRF_LOGD("%s: called", __FUNCTION__);

	if(android_session_check != ANDROID_SESSION_NONE)
	{
		SIRF_LOGI("android_session_handler is already running!!!");
		if (request != ANDROID_CP_SESSION) /* CP session has highest priorty */
		{
			if(android_session_check == ANDROID_CP_SESSION)
				return SIRF_FAILURE;  /* CP session is already running */

			if(android_session_check == ANDROID_MO_SESSION && request == ANDROID_NI_SESSION)
			{
				SIRF_LOGI("discard NI SESSION while MO Session is running");
				return SIRF_FAILURE;
			}
			else if(android_session_check == ANDROID_NI_SESSION && request == ANDROID_MO_SESSION)
			{
				tSIRF_RESULT result;

				//android_ni_session_stop(); //block this function!!!, this function is called in android_session_handler!!!!!
				Noupdatetosessionhandler = 1;
				if( g_NI_Session == NI_SESSION_RUNNING)
				{
					g_NI_Session = NI_SESSION_CLOSING;
					SIRF_LOGI("Accepted MO Session as NI has low priority");
					result = SIRF_PAL_OS_SEMAPHORE_Release( semNiPosition );
					if ( result != SIRF_SUCCESS )
					{
						SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
						return SIRF_FAILURE;
					}
				}
				else
				{
					SIRF_LOGI("NI session is closing now. MO session will start as soon as NI session is closed");
				}
			}
			else
			{
				return SIRF_FAILURE;
			}
		}
		else /* Requesting Session is CP */
		{
			if(android_session_check == ANDROID_MO_SESSION )// android_session_check == ANDROID_NI_SESSION)
			{
				SIRF_LOGD("Updating LM about session closing");
				Noupdatetosessionhandler = 1;
				if (INVAILD_SESSION_HANDLE != g_SessionID) /* in case that CP_SendNetMessage is called ahead of LSM_Start function call */
				{
					tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release( semMoPosition );
					if ( result != SIRF_SUCCESS )
					{
						SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
						return SIRF_FAILURE;
					}
				}
			}
			else if (android_session_check == ANDROID_NI_SESSION)
			{
				tSIRF_RESULT result;
				SIRF_LOGI("Closing NI session as CP session took over"); /* I belive LM does not care about NI session */
				Noupdatetosessionhandler = 1;
				if( g_NI_Session == NI_SESSION_RUNNING)
				{
					g_NI_Session = NI_SESSION_CLOSING;
					SIRF_LOGI("Accepted CP Session as NI has low priority");
					result = SIRF_PAL_OS_SEMAPHORE_Release( semNiPosition );
					if ( result != SIRF_SUCCESS )
					{
						SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
						return SIRF_FAILURE;
					}
				}
				else
				{
					SIRF_LOGD("NI session is closing now. CP session will start as soon as NI session is closed");
				}

			}
		}
		SIRF_PAL_OS_THREAD_Sleep(20);
	} 
	else
	{
		//SIRF_LOGI("android_session_handler no session is running!!!\n");
	}

	//SIRF_LOGD("resume android_session_handler");
	android_session_check = request;

	if(NULL != semAndroidSessionHandler)
	{
		tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release( semAndroidSessionHandler );
		if ( result != SIRF_SUCCESS )
		{
			SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
			return SIRF_FAILURE;
		}
		switch(android_session_check)
		{
			case ANDROID_SESSION_NOT_READY:
				SIRF_LOGD("session resumed : ANDROID_SESSION_NOT_READY");
				break;
			case ANDROID_SESSION_NONE:
				SIRF_LOGD("session resumed : ANDROID_SESSION_NONE");
				break;
			case ANDROID_MO_SESSION:
				SIRF_LOGD("session resumed : ANDROID_MO_SESSION");	      
				break;
			case ANDROID_NI_SESSION:
				SIRF_LOGD("session resumed : ANDROID_NI_SESSION");	      
				break;
			case ANDROID_CP_SESSION:
				SIRF_LOGD("session resumed : ANDROID_CP_SESSION");	      
				break;
			default:
				SIRF_LOGW("session resumed = %d ",android_session_check);
				break;
		}

	}

	//SIRF_LOGD("android_session_handler_resume success");

	return SIRF_SUCCESS;
}

static int android_session_handler_suspend(void)
{
	//SIRF_LOGD("%s: called", __FUNCTION__);

	if(android_session_check == ANDROID_SESSION_NONE)
	{
		SIRF_LOGE("no session to suspend !!!");
		return SIRF_FAILURE;
	}

	if(android_session_check == ANDROID_MO_SESSION)
	{
		SIRF_LOGI("Suspending MO session!!!");
		if(NULL != semMoPosition)
		{
			tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release( semMoPosition );
			if (SIRF_SUCCESS != result)
			{
				SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
				return SIRF_FAILURE;
			}
		}
	}
	else if(android_session_check == ANDROID_NI_SESSION)
	{
		SIRF_LOGI("Suspending NI session!!!");
		if(NULL != semNiPosition)
		{
			tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release( semNiPosition );
			if (SIRF_SUCCESS != result)
			{
				SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
				return SIRF_FAILURE;
			}
		}
	}
	else if(android_session_check == ANDROID_CP_SESSION)
	{
		SIRF_LOGD( "Suspend:CP session is running");
	}

	//SIRF_LOGD("android_session_handler_suspend success");

	return SIRF_SUCCESS;
}

static int create_semaphore_for_android_session_handler(void)
{
	tSIRF_RESULT result;

	result = SIRF_PAL_OS_SEMAPHORE_Create(&semAndroidSessionHandler, 0); //default value : 0
	if(SIRF_SUCCESS != result)
	{
		SIRF_LOGE("Semaphore (semAndroidSessionHandler) Creation Failed!");
		return result;
	}

	result = SIRF_PAL_OS_SEMAPHORE_Create(&semMoPosition, 0); //default value : 0
	if(SIRF_SUCCESS != result)
	{
		SIRF_LOGE("Semaphore (semMoPosition) Creation Failed!");
		return result;
	}

	result = SIRF_PAL_OS_SEMAPHORE_Create(&semNiPosition, 0); //default value : 0
	if(SIRF_SUCCESS != result)
	{
		SIRF_LOGE("Semaphore (semNiPosition) Creation Failed!");
		return result;
	}

	result = SIRF_PAL_OS_SEMAPHORE_Create(&semRilEvent, 0); //default value : 0
	if(SIRF_SUCCESS != result)
	{
		SIRF_LOGE("Semaphore (semRilEvent) Creation Failed!");
		return result;
	}
	result = SIRF_PAL_OS_SEMAPHORE_Create(&semwaitstop, 0); //default value : 0
	if(SIRF_SUCCESS != result)
	{
		SIRF_LOGE("Semaphore (semwaitstop) Creation Failed!");
		return result;
	}

	return SIRF_SUCCESS;
}

static void delete_semaphore_for_android_session_handler(void)
{
	SIRF_PAL_OS_SEMAPHORE_Delete( semAndroidSessionHandler);
	semAndroidSessionHandler = NULL;

	SIRF_PAL_OS_SEMAPHORE_Delete( semMoPosition );
	semMoPosition = NULL;

	SIRF_PAL_OS_SEMAPHORE_Delete( semNiPosition );
	semNiPosition = NULL;

	SIRF_PAL_OS_SEMAPHORE_Delete( semRilEvent);
	semRilEvent = NULL;

	SIRF_PAL_OS_SEMAPHORE_Delete( semwaitstop);
	semwaitstop = NULL;

	return;
}

#define GPS_OFF 0
#define GPS_ON  1

void report_GPS_ONOFF(int mode)
{
	static int prev_mode = GPS_OFF;
	FILE *fp = NULL, *fp2 = NULL;
	char onoff[20] = {0,};
	char onoff2[20] = {0,};

	if(prev_mode == mode)
	{
		SIRF_LOGD("GPS ONOFF : Do Nothing!!!");
		return;
	}
	else
	{
		prev_mode = mode;
	}

	SIRF_LOGD("GPS ONOFF Report : %d", mode);

	if(g_GSD4tConfig.isATTNetworkOperator == 1) /*AT&T Network Operator Only */
	{
		fp = fopen("/sys/class/power_supply/battery/batt_gps","wb"); //overwrite
		if(fp != NULL)
		{
			sprintf(onoff,"%d", mode);
			fwrite(onoff, 1,strlen(onoff), fp);
			fclose(fp);
		}
		else
		{
			SIRF_LOGE("GPS ONOFF Mode : File Open Error - batt_gps!!!!");
		}
	}

	if(g_GSD4tConfig.isEMC_ENABLE == 1) /*EMC Enable for N1 */
	{
		fp2 = fopen("/sys/module/tegra2_emc/parameters/emc_enable", "w+");
		if(fp2 != NULL)
		{
			if(mode == GPS_OFF)
			{
				sprintf(onoff2,"%s", "Y");
			}
			else
			{
				sprintf(onoff2,"%s", "N");
			}
			fwrite(onoff2, 1,strlen(onoff2), fp2);
			fclose(fp2);
		}
		else
		{
			SIRF_LOGE("GPS ONOFF Mode : File Open Error - emc_enable !!!!");
		}
	}

}


void android_session_handler(void *arg)
{
	eAndroidSessionHandlerState state = IdleState;

	SIRF_LOGD("Android Session Handler Starting");
	run_session_handler = 1;

	create_semaphore_for_android_session_handler();
	android_session_check = ANDROID_SESSION_NONE;

	while(run_session_handler)
	{
		switch(state)
		{
			case IdleState:
				{
					if(!Noupdatetosessionhandler)
					{
						SIRF_LOGI("IdleState:updating session check");
						android_session_check = ANDROID_SESSION_NONE;
						if(DataConnectionStatus != AGPS_DATA_CONNECTION_CLOSED)
						{
							SIRF_LOGE("Session is closed without releasing data connection");
							releaseDataConnection();
						}
					}
					else
					{
						SIRF_LOGI("IdleState:not updating session check");
						Noupdatetosessionhandler = 0;
					}
					controlPowerWakeLock(GPS_WAKE_LOCK_DISABLE);  // no power saving for now
					SIRF_PAL_OS_SEMAPHORE_Wait(semAndroidSessionHandler, SIRF_TIMEOUT_INFINITE); /* Blocking Here!!!!!!!!!! */
					SIRF_LOGI("========== NEW Session ==========\n");

					if(android_session_check== ANDROID_MO_SESSION)
					{
						state = MOSession;
					}
					else if(android_session_check == ANDROID_NI_SESSION)
					{
						state = NISession;
					}
					else if(android_session_check == ANDROID_CP_SESSION)
					{
						state = CPSession;
					}
					else
					{
						state = UnknownSession;  // dangerous if android_session_check hooked up
					}
					break;
				}

			case MOSession:
				{
					SIRF_LOGI("Session State Machine : MOSession");
					controlPowerWakeLock(GPS_WAKE_LOCK_ENABLE);
					if (!g_del_aiding_request)
					{
						/* Skip the config file load section as the application wants to control reset mode */
#ifdef CSR_GPS_CONFIG
						csr_property_load_config();
#elif defined CUSTOMER_GPS_CONFIG
						sirf_sec_property_load_config();
						sirf_sec_check_factory_test_mode(); //Check CW Test Mode is enabled or not!!!
#endif
					}
					if(android_mo_session_start() == SIRF_SUCCESS)
					{
						android_gps_pwr_control(ANDROID_GPS_PWR_ON); //for better understanding, move this function to here
						//SIRF_LOGD("android_session_handler blocked. wait for MO Session Close...");
						SIRF_PAL_OS_SEMAPHORE_Wait(semMoPosition, SIRF_TIMEOUT_INFINITE); /* Blocking Here!!!!!!!!!! */
						//SIRF_LOGD("MO Session Closing..");
						android_gps_pwr_control(ANDROID_GPS_PWR_OFF); //LM sometimes misses session end and engine off notifications

						if(android_mo_session_stop() == SIRF_SUCCESS)
						{
							android_gps_pwr_control(ANDROID_GPS_PWR_OFF); //for better understanding, move this function to here
							state = IdleState;
						}
						else
						{
							state = ErrorState;
						}

						configureSETCapabilities(gCapabilities);
					}
					else
					{
						SIRF_LOGE("android_mo_session_start fail!!!!");
						state = IdleState;
					}

					if (g_del_aiding_request)
					{
						/* Reset code reset mode to HOT and remove g_del_aiding_request */
						g_GSD4tConfig.resetType = LSM_RESET_TYPE_HOT_RESET;
						g_del_aiding_request = LSM_FALSE;
					}

					break;
				}

			case NISession:
				{
					SIRF_LOGI("Session State Machine : NISession");
					controlPowerWakeLock(GPS_WAKE_LOCK_ENABLE);
#ifdef CSR_GPS_CONFIG
					csr_property_load_config();
#elif defined CUSTOMER_GPS_CONFIG
					sirf_sec_property_load_config();
#endif
					g_GSD4tConfig.defaultResponseTimeoutofNI = SUPL_NI_NOTIFICATION_DEFAULT_RESPONSE_TIMEOUT;
					SIRF_LOGI("NI Session starting");
#if NI_TEST_BUILD
					SIRF_LOGI("NI_TEST_BUILD - NI session opened sending cp_reset");
					CP_Reset();
#endif
					configureSETCapabilities(gCapabilities);
					if(android_ni_session_start() == SIRF_SUCCESS)
					{
						android_gps_pwr_control(ANDROID_GPS_PWR_ON); //for better understanding, move this function to here - updates status
						SIRF_LOGD("android_session_handlerblocked. wait for NI Session Close...");
#if NI_TEST_BUILD
						tSIRF_RESULT ret = SIRF_PAL_OS_SEMAPHORE_Wait(semNiPosition, 70000); //Blocking Here!!!!!!!!!!
#else
						tSIRF_RESULT ret = SIRF_PAL_OS_SEMAPHORE_Wait(semNiPosition, 40000); //Blocking Here!!!!!!!!!!
#endif
						if(ret == SIRF_PAL_OS_SEMAPHORE_WAIT_TIMEOUT)
						{
							SIRF_LOGW("NI Session Closing timeout");
						}

						SIRF_LOGI("NI Session Closing normal");
						if(android_ni_session_stop() == SIRF_SUCCESS)
						{
							android_gps_pwr_control(ANDROID_GPS_PWR_OFF); //for better understanding, move this function to here - updates status
							state = IdleState;
						}
						else
						{
							state = ErrorState;
						}
					}
					else
					{
						state = IdleState;
					}

					break;
				}

			case CPSession:
				{
					SIRF_LOGI("Android Session State Machine : CPSession");
					break;
				}

			case UnknownSession:
			case ErrorState:
			default:
				{
					run_session_handler = 0;  // do clean up here
					SIRF_LOGE("ERROR : UnknownSession");

					break;
				}
		}
	}

	delete_semaphore_for_android_session_handler();

	SIRF_LOGD("android_session_handler exit");

	return;
}

static int android_session_handler_close(void)
{
	//SIRF_LOGD("%s: called", __FUNCTION__);

	run_session_handler = 0;

	if(android_session_check != ANDROID_SESSION_NONE)
	{
		if(SIRF_SUCCESS != android_session_handler_suspend())
		{
			SIRF_LOGE("android_session_handler_suspend fail!!!");
			return -1;
		}
	}

	if(NULL != semAndroidSessionHandler)
	{
		tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release( semAndroidSessionHandler );
		if ( result != SIRF_SUCCESS )
		{
			SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
			return SIRF_FAILURE;
		}
	}
	else
	{
		return SIRF_FAILURE;
	}

	//SIRF_LOGD("android_session_handler_close success");

	return SIRF_SUCCESS;
}

static int android_nmea_report(GpsUtcTime timestamp, const char * nmea, int length)
{
	pthread_mutex_lock(&sAndroidStatusEventMutex);

	if(length >= NMEA_SENTENCE_LENGTH)
	{
		SIRF_LOGE("NMEA length is too long");
		length = NMEA_SENTENCE_LENGTH - 1;
		return SIRF_FAILURE;
	}
	if(sNmeaSentenceCount >= NMEA_SENTENCE_COUNT)
	{
		SIRF_LOGE("NMEA Buffer Overflow!!!!");
		pthread_mutex_unlock(&sAndroidStatusEventMutex);
		return SIRF_FAILURE;
	}

	sAndroidStatusPendingCallbacks |= ANDROID_STATUS_REPORT_NMEA;
	sNmeaBuffer[sNmeaSentenceCount].timestamp = timestamp;
	memcpy(sNmeaBuffer[sNmeaSentenceCount].nmea, nmea, length);
	sNmeaBuffer[sNmeaSentenceCount].nmea[length] =0;
	sNmeaBuffer[sNmeaSentenceCount].length = length;
	sNmeaSentenceCount++;

	pthread_cond_signal(&sAndroidStatusEventCond);
	pthread_mutex_unlock(&sAndroidStatusEventMutex);

	return SIRF_SUCCESS;
}


static int android_status_report(eAndroidStatusReport report)
{
	pthread_mutex_lock(&sAndroidStatusEventMutex);
	sAndroidStatusPendingCallbacks |= report;
	pthread_cond_signal(&sAndroidStatusEventCond);
	pthread_mutex_unlock(&sAndroidStatusEventMutex);

	return SIRF_SUCCESS;
}

int SendAP_PowerOff_Notification(void)
{

	if(android_session_check == ANDROID_SESSION_NONE)
	{
		SIRF_LOGI("LPL saying power off");
		android_status_report(ANDROID_STATUS_REPORT_POWER_SAVING);
	}
	else
	{
		SIRF_LOGI("LPL Power OFF: Toast");
	}
	return 0;

}



void android_status_report_handler(void *arg)
{
	int pendingCallbacks = 0;
	int nmeaSentenceCount = 0;
	int i = 0;
	int ok_to_run =  1;

	SIRF_LOGD("Android Status Report Handler Starting");

	android_status_report_ready = 1;

	while(ok_to_run)
	{
		pthread_mutex_lock(&sAndroidStatusEventMutex);
		while (sAndroidStatusPendingCallbacks == 0)
		{
			pthread_cond_wait(&sAndroidStatusEventCond, &sAndroidStatusEventMutex);
		}

		pendingCallbacks = sAndroidStatusPendingCallbacks;
		sAndroidStatusPendingCallbacks = 0;

		nmeaSentenceCount = sNmeaSentenceCount;
		sNmeaSentenceCount = 0;

		if(pendingCallbacks & ANDROID_STATUS_REPORT_SATELITES)
		{
			memcpy(&androidSVStatusCopy, &androidSVStatus, sizeof(androidSVStatusCopy));
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_NMEA)
		{
			memcpy(&sNmeaBufferCopy, &sNmeaBuffer, nmeaSentenceCount * sizeof(sNmeaBuffer[0]));
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_LOCATION)
		{
			memcpy(&locationInfoCopy, &locationInfo, sizeof(locationInfoCopy));
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_DATACONNECTION)
		{
			memcpy(&agpsStatusCopy, &agpsStatus, sizeof(agpsStatusCopy));
		}

		pthread_mutex_unlock(&sAndroidStatusEventMutex);

		if(pendingCallbacks & ANDROID_STATUS_REPORT_SATELITES)
		{
			if(gpsCallbacks.sv_status_cb != NULL)
			{
				gpsCallbacks.sv_status_cb(&androidSVStatusCopy);
			}
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_NMEA)
		{
			if(gpsCallbacks.nmea_cb != NULL)
			{
				for ( i = 0; i < nmeaSentenceCount; i++ )
				{
					gpsCallbacks.nmea_cb(sNmeaBufferCopy[i].timestamp, sNmeaBufferCopy[i].nmea, sNmeaBufferCopy[i].length);
				}
			}
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_LOCATION)
		{
			if(gpsCallbacks.location_cb != NULL)
			{
				gpsCallbacks.location_cb(&locationInfoCopy);
			}
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_DATACONNECTION)
		{
			if(agpsCallbacks.status_cb != NULL)
			{
				agpsCallbacks.status_cb(&agpsStatusCopy);
				SIRF_LOGI("DATA Connection Req - Type : %d, Status : %d", agpsStatusCopy.type, agpsStatusCopy.status);
			}
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_NI_NOTIFICATION)
		{
			SIRF_LOGI("NiNotificationStatusReport");
			if(gpsNiCallbacks.notify_cb != NULL)
			{
				gpsNiCallbacks.notify_cb(&niNotification);
			}
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_CP_NI_NOTIFICATION)
		{
			SIRF_LOGI("CpNiNotificationStatusReport");
			if(gpsNiCallbacks.notify_cb != NULL)
			{
				gpsNiCallbacks.notify_cb(&CP_mtlrNotification);
			}
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_REF_LOC_REQ)
		{
			if(gpsRilCallbacks.request_refloc)
			{
				SIRF_LOGI("Send request_refloc to Android Framework");
				gpsRilCallbacks.request_refloc(AGPS_RIL_REQUEST_REFLOC_CELLID);
			}
			else
			{
				SIRF_LOGE("ERROR : No request_refloc");
			}
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_SETID_REQ)
		{
			if(gpsRilCallbacks.request_setid)
			{
				SIRF_LOGI("Send request_setid to Android Framework");
				if(LSM_TRUE == isSETIDIMSI ())
				{
					gpsRilCallbacks.request_setid(AGPS_RIL_REQUEST_SETID_IMSI);
				}
				else
				{
					gpsRilCallbacks.request_setid(AGPS_RIL_REQUEST_SETID_MSISDN);
				}
			}
			else
			{
				SIRF_LOGE("ERROR : No request_setid");
			}
		}
		if(pendingCallbacks & ANDROID_STATUS_REPORT_POWER_SAVING)
		{
			SIRF_LOGD("Setting POWER OFF");
			controlPowerWakeLock(GPS_WAKE_LOCK_DISABLE);
			break;
		}

		if(pendingCallbacks & ANDROID_STATUS_REPORT_EXIT)
		{
			break;
		}
	}

	SIRF_LOGD("android_status_report_handler exit");

	return;
}

static int android_status_report_handler_close(void)
{
	pthread_mutex_lock(&sAndroidStatusEventMutex);
	sAndroidStatusPendingCallbacks |= ANDROID_STATUS_REPORT_EXIT;
	pthread_cond_signal(&sAndroidStatusEventCond);
	pthread_mutex_unlock(&sAndroidStatusEventMutex);

	return SIRF_SUCCESS;
}



static void sirf_gps_load_default_config(void)
{
	strlcpy((char *)g_GSD4tConfig.project_name, "Nufront", sizeof(g_GSD4tConfig.project_name));
	strlcpy((char *)g_userPalConfig.on_off_port, ONOFF_GPIO, sizeof(g_userPalConfig.on_off_port));
	strlcpy((char *)g_userPalConfig.reset_port, RESET_GPIO, sizeof(g_userPalConfig.reset_port));
	g_GSD4tConfig.debugging        = 1;
	g_GSD4tConfig.frequency_aiding = 0;
	g_GSD4tConfig.sensor_aiding    = 0;
	g_GSD4tConfig.set_id_imsi      = 1;
	g_GSD4tConfig.ssl_enabled   = 0;
	g_GSD4tConfig.isReAidingParamSet = 0;
	g_GSD4tConfig.reAidingTimeIntervalLimit = 20;
	g_GSD4tConfig.isControlPlaneEnabled = 1;
	g_GSD4tConfig.isATTNetworkOperator = 0;
	g_GSD4tConfig.isEMC_ENABLE = 0;

	ATTNetworkOperatorEnabled = 0;

	/* initialize 4t params */
	g_GSD4tConfig.trackerConfig.start_mode                   = SIRFNAV_UI_CTRL_MODE_AUTO;

	g_GSD4tConfig.trackerConfig.ref_clk_offset               = SIRFNAV_UI_CTRL_DEFAULT_REF_CLK_OFFSET;
	g_GSD4tConfig.trackerConfig.ref_clk_warmup_delay         = SIRFNAV_UI_CTRL_DEFAULT_REF_CLK_WARMUP_DELAY;
	g_GSD4tConfig.trackerConfig.ref_clk_frequency            = 16369000; //if working with TCXO at 26MHz and no HW config file 26000000 can be used as value here
	g_GSD4tConfig.trackerConfig.ref_clk_uncertainty          = SIRFNAV_UI_CTRL_DEFAULT_REF_CLK_UNC;

	g_GSD4tConfig.trackerConfig.storage_mode                 = SIRFNAV_UI_CTRL_MODE_STORAGE_PERIODIC_ALL;
	g_GSD4tConfig.trackerConfig.lna_type                     = SIRFNAV_UI_CTRL_MODE_EXT_LNA_OFF;
	g_GSD4tConfig.trackerConfig.debug_settings               = SIRFNAV_UI_CTRL_MODE_TEXT_ENABLE | SIRFNAV_UI_CTRL_MODE_RAW_MSG_ENABLE;
	g_GSD4tConfig.trackerConfig.tracker_port_select          = SIRFNAV_UI_CTRL_MODE_PORT_SEL_UART;
	strlcpy((char *)g_GSD4tConfig.trackerConfig.tracker_port, UART_DRIVER_PATH, sizeof(g_GSD4tConfig.trackerConfig.tracker_port));

	g_GSD4tConfig.trackerConfig.io_pin_configuration_mode    = SIRFNAV_UI_CTRL_MODE_IO_CONFIGURATION_ENABLE;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[0]      = SIRFNAV_UI_CTRL_DEFAULT_PIN0_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[1]      = SIRFNAV_UI_CTRL_DEFAULT_PIN1_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[2]      = SIRFNAV_UI_CTRL_DEFAULT_PIN2_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[3]      = SIRFNAV_UI_CTRL_DEFAULT_PIN3_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[4]      = SIRFNAV_UI_CTRL_DEFAULT_PIN4_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[5]      = 0x003F;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[6]      = SIRFNAV_UI_CTRL_DEFAULT_PIN6_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[7]      = SIRFNAV_UI_CTRL_DEFAULT_PIN7_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[8]      = SIRFNAV_UI_CTRL_DEFAULT_PIN8_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[9]      = SIRFNAV_UI_CTRL_DEFAULT_PIN9_CONFIG;
	g_GSD4tConfig.trackerConfig.io_pin_configuration[10]     = SIRFNAV_UI_CTRL_DEFAULT_PIN10_CONFIG;

	g_GSD4tConfig.trackerConfig.uart_baud_rate               = SIRFNAV_UI_CTRL_DEFAULT_BAUD_RATE;
	g_GSD4tConfig.trackerConfig.code_load_baud_rate          = SIRFNAV_UI_CTRL_DEFAULT_BAUD_RATE;
	g_GSD4tConfig.trackerConfig.uart_max_preamble            = SIRFNAV_UI_CTRL_DEFAULT_MAX_PREAMBLE;
	g_GSD4tConfig.trackerConfig.uart_idle_byte_wakeup_delay  = SIRFNAV_UI_CTRL_DEFAULT_IDLE_BYTE_WAKEUP_DELAY;
	g_GSD4tConfig.trackerConfig.uart_hw_fc                   = SIRFNAV_UI_CTRL_MODE_HW_FLOW_CTRL_OFF;

	g_GSD4tConfig.trackerConfig.i2c_host_address             = SIRFNAV_UI_CTRL_DEFAULT_I2C_HOST_ADDRESS;
	g_GSD4tConfig.trackerConfig.i2c_tracker_address          = SIRFNAV_UI_CTRL_DEFAULT_I2C_TRACKER_ADDRESS;
	g_GSD4tConfig.trackerConfig.i2c_mode                     = SIRFNAV_UI_CTRL_MODE_I2C_MODE_MULTI_MASTER;
	g_GSD4tConfig.trackerConfig.i2c_rate                     = SIRFNAV_UI_CTRL_MODE_I2C_RATE_400_KBPS;

	g_GSD4tConfig.trackerConfig.spi_rate                     = SIRFNAV_UI_CTRL_MODE_SPI_RATE_4_MHZ;

	g_GSD4tConfig.trackerConfig.on_off_control               = SIRFNAV_UI_CTRL_ON_OFF_EDGE_RE |
		SIRFNAV_UI_CTRL_ON_OFF_USE_GPIO |
		SIRFNAV_UI_CTRL_ON_OFF_OFF_ENABLED;

	g_GSD4tConfig.trackerConfig.flash_mode                   = SIRFNAV_UI_CTRL_FLASH_MODE_DO_NOTHING;

	g_GSD4tConfig.trackerConfig.weak_signal_enabled          = SIRF_TRUE;

	g_GSD4tConfig.trackerConfig.backup_LDO_mode_enabled      = SIRFNAV_UI_CTRL_MODE_BACKUP_LDO_ENABLE;

	return;
}



static LSM_BOOL isSETIDIMSI(void)
{
	if(g_GSD4tConfig.set_id_imsi == 1)
	{
		return LSM_TRUE;
	}
	else
	{
		return LSM_FALSE;
	}
}


/* For acquire_wake_lock or release_wake_lock!!!! */
LSM_BOOL controlPowerWakeLock(int enable)
{
	if ((!gpsCallbacks.acquire_wakelock_cb) ||
			(!gpsCallbacks.release_wakelock_cb))
	{
		SIRF_LOGW("Warning! null pointer in acquire_wakelock_cb / release_wakelock_cb");
		return SIRF_FAILURE;
	}

	if (enable == GPS_WAKE_LOCK_ENABLE)
	{
		gpsCallbacks.acquire_wakelock_cb();
		SIRF_LOGD(">>>INFO: GPS_WAKE_LOCK_ENABLE");
	}
	else
	{
		SIRF_LOGD(">>>INFO: GPS_WAKE_LOCK_DISABLE");
		gpsCallbacks.release_wakelock_cb();
	}

	return SIRF_SUCCESS;
}

//For logging data in SD Card (Customer Request)
LSM_BOOL isLoggingDirectoryAvailable(void)
{
	int status = 0;

	status = access("/sdcard/gps/", F_OK);
	if(status == 0)
	{
		SIRF_LOGD("/sdcard/gps/ exists");
		return SIRF_SUCCESS;
	}
	else
	{
		SIRF_LOGD("/sdcard/gps/ doesn't exist");
		return SIRF_FAILURE;
	}
}


static LSM_BOOL getCELLInfo(void)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	SIRF_LOGD("### blocked. wait for Refloc Response...");

	pthread_mutex_lock(&sAndroidRilMutex);
	android_status_report(ANDROID_STATUS_REPORT_REF_LOC_REQ);
	pthread_mutex_unlock(&sAndroidRilMutex);

	tSIRF_RESULT ret = SIRF_PAL_OS_SEMAPHORE_Wait(semRilEvent, 3000);
	if (ret == SIRF_PAL_OS_SEMAPHORE_WAIT_TIMEOUT)
	{
		return LSM_FALSE;
	}

	SIRF_LOGD("got Refloc Response");

	return LSM_TRUE;
}

static LSM_BOOL getSETIDInfo (void)
{
	SIRF_LOGD("### blocked. wait for Set ID Response...");

	pthread_mutex_lock(&sAndroidRilMutex);
	android_status_report(ANDROID_STATUS_REPORT_SETID_REQ);
	pthread_mutex_unlock(&sAndroidRilMutex);

	tSIRF_RESULT ret = SIRF_PAL_OS_SEMAPHORE_Wait(semRilEvent, 3000);
	if (ret == SIRF_PAL_OS_SEMAPHORE_WAIT_TIMEOUT)
	{
		return LSM_FALSE;
	}

	SIRF_LOGD("got Set ID Response");

	return LSM_TRUE;
}

static LSM_BOOL send_data_connection_request(void)
{
	agpsStatus.size = sizeof(AGpsStatus);
	agpsStatus.type = AGPS_TYPE_SUPL;
	agpsStatus.status = GPS_REQUEST_AGPS_DATA_CONN;
	agpsStatus.ipaddr = 0xFFFFFFFF;
	pthread_mutex_lock(&sAndroidRilMutex);
	android_status_report(ANDROID_STATUS_REPORT_DATACONNECTION);
	pthread_mutex_unlock(&sAndroidRilMutex);

	return SIRF_SUCCESS;
}

static LSM_BOOL requestDataConnection(void)
{
	tSIRF_RESULT ret;
	unsigned retry_count = 0;

	// SIRF_LOGD("g to r dc b mutex");
	pthread_mutex_lock(&sAndroidRilMutex);
	DataConnectionStatus = AGPS_DATA_CONNECTION_OPENING;
	pthread_mutex_unlock(&sAndroidRilMutex);
	SIRF_LOGD("g to r dc a mutex");

	do
	{
		RilRequestPending = 1;
		if(send_data_connection_request() != SIRF_SUCCESS)
		{
			return SIRF_FAILURE;
		}

		ret = SIRF_PAL_OS_SEMAPHORE_Wait(semRilEvent, 3000);
		if(ret == SIRF_PAL_OS_SEMAPHORE_WAIT_TIMEOUT)
		{
			retry_count++;
			RilRequestPending = 0;
			SIRF_LOGI("retrying for data connection");
		}
		else if( ret == 0)
		{
			SIRF_LOGI("got DataConnection Response");
			return LSM_TRUE;
		}
		else
		{
			RilRequestPending = 0;
			SIRF_LOGE("Failure in  DataConnection request");
			return LSM_FALSE;  /* cata strophic failure */
		}
	} while(retry_count != 2);

	RilRequestPending = 0;

	return LSM_FALSE;
}

static LSM_BOOL requestAllRilInfo(void)
{
	if(LSM_TRUE == requestDataConnection())
	{
		if(LSM_TRUE == getSETIDInfo())
		{
			if(LSM_TRUE == getCELLInfo())
			{
				SIRF_LOGD("got all RIL info");
			}
			else
			{
				SIRF_LOGE("requestAllRilInfo Failure:Could not get CELL info"); // hack: what to do
				return LSM_FALSE;
			}
		}
		else
		{
			SIRF_LOGW("requestAllRilInfo Failure:Could not get SETID info");
			return LSM_FALSE;
		}
	}
	else
	{
		SIRF_LOGD("DataConnection request error");
		return LSM_FALSE;
	}

	return LSM_TRUE;
}

LSM_BOOL releaseDataConnection(void)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	agpsStatus.size = sizeof(AGpsStatus);
	agpsStatus.type = AGPS_TYPE_SUPL;
	agpsStatus.status = GPS_RELEASE_AGPS_DATA_CONN;
	agpsStatus.ipaddr = 0xFFFFFFFF;
	pthread_mutex_lock(&sAndroidRilMutex);
	if(DataConnectionStatus == AGPS_DATA_CONNECTION_OPEN || DataConnectionStatus == AGPS_DATA_CONNECTION_OPENING)
	{
		android_status_report(ANDROID_STATUS_REPORT_DATACONNECTION);
	}
	DataConnectionStatus = AGPS_DATA_CONNECTION_CLOSED;
	pthread_mutex_unlock(&sAndroidRilMutex);

	SIRF_LOGD("Data connection released");

	return LSM_TRUE;
}

/* AGPSInterface!!!!!!!!!!!!!!!!! */
static void  sirf_agps_init( AGpsCallbacks* callbacks )
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if(callbacks != NULL)
	{
		agpsCallbacks.status_cb = callbacks->status_cb;
	}
}

static int  sirf_agps_data_conn_open( const char* apn )
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	pthread_mutex_lock(&sAndroidRilMutex);
	DataConnectionStatus = AGPS_DATA_CONNECTION_OPEN;
	pthread_mutex_unlock(&sAndroidRilMutex);
	SIRF_LOGD("con open recvd with apn = %s", apn);
	if(RilRequestPending)
	{
		RilRequestPending = 0;
		tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release(semRilEvent );
		if ( result != SIRF_SUCCESS )
		{
			SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
		}
	}
	else
	{
		SIRF_LOGD("con open:no ril request was pending");
	}

	return 0;
}

static int  sirf_agps_data_conn_closed(const int connId)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	pthread_mutex_lock(&sAndroidRilMutex);
	DataConnectionStatus = AGPS_DATA_CONNECTION_CLOSED;
	pthread_mutex_unlock(&sAndroidRilMutex);
	SIRF_LOGD("con_closed from ril");

	return 0;
}

static int  sirf_agps_data_conn_failed(void)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	pthread_mutex_lock(&sAndroidRilMutex);
	DataConnectionStatus = AGPS_DATA_CONNECTION_CLOSED;
	pthread_mutex_unlock(&sAndroidRilMutex);
	SIRF_LOGW("Data conn failed");

	if(RilRequestPending)
	{
		RilRequestPending = 0;
		tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release(semRilEvent );
		if ( result != SIRF_SUCCESS )
		{
			SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
		}
	}
	else
	{
		SIRF_LOGW("con_failed: no ril request pending now");
	}

	return 0;
}

static int  sirf_agps_set_server(AGpsType type, const char* hostname, int port)
{
	SIRF_LOGD("%s: called", __FUNCTION__);
	SIRF_LOGD("Type:%d, hostname:%s, port:%d", type, hostname, port);

	if(hostname == NULL)
	{
		SIRF_LOGE("Host name NULL error");
		return -1;
	}

	memset(g_GSD4tConfig.serverAddress,0,sizeof(g_GSD4tConfig.serverAddress));
	strcpy(g_GSD4tConfig.serverAddress, hostname);
	g_GSD4tConfig.serverPort = (tSIRF_UINT16) port;

	LSM_setAgpsServer(g_GSD4tConfig.serverAddress, g_GSD4tConfig.serverPort);

	return 0;
}

static const AGpsInterface sirfAGpsInterface =
{
	sizeof(AGpsInterface),
	sirf_agps_init,
	sirf_agps_data_conn_open,
	sirf_agps_data_conn_closed,
	sirf_agps_data_conn_failed,
	sirf_agps_set_server,
};

/* GpsNiInterface!!!!!!!!!!!!!!!!!! */
static void sirf_gps_ni_init (GpsNiCallbacks *callbacks)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if(callbacks != NULL)
	{
		gpsNiCallbacks.notify_cb = callbacks->notify_cb;
	}

	return;
}

static void sirf_gps_ni_respond (int notif_id, GpsUserResponseType user_response)
{
	tSIRF_RESULT result;

	SIRF_LOGD("%s: called", __FUNCTION__);

	GPS_NI_RESPONSE = (int) user_response;

#ifdef CONTROL_PLANE_SUPPORT
	// Identify if this callback returns the user's verification to C-Plane MT-LR (CPA) or SUPL (LPL)
	if (g_cpaLcsInvokeHandle == notif_id)
	{
		// C-Plane MT-LR User Response
		unsigned long verificationResponse; // Verification Response [range: CPA_LCS_VERIFICATION_RESPONSE] */
		int present;

		switch (user_response)
		{
			case GPS_NI_RESPONSE_ACCEPT:
				present = 1;
				verificationResponse = CPA_LCS_PERMISSION_GRANTED;
				break;
			case GPS_NI_RESPONSE_DENY:
				present = 1;
				verificationResponse = CPA_LCS_PERMISSION_DENIED;
				break;
			case GPS_NI_RESPONSE_NORESP:
				// intentionally empty ...
			default:
				present = 0;  // This response should not be sent to CPA
				verificationResponse = CPA_LCS_PERMISSION_DENIED; // dummy value
		}

		SIRF_LOGD("%s: C-P MTLR Response: notif_id=%d, present=%d, verificationResponse=%ld", __FUNCTION__, notif_id, present, verificationResponse);

		// Send the verification response only if user accepts or denies. Else, skip it.
		if (present)
		{
			if (LSM_FALSE == CpaClient_SendLcsVerificationResp(notif_id, present, verificationResponse))
			{
				SIRF_LOGE( "ERROR: in return from CpaClient_SendLcsVerificationResp()");
			}
		}
		else
		{
			SIRF_LOGD("%s: C-P MTLR Response: Timeout occured!", __FUNCTION__);
		}
	}

	else
#endif
	{
		SIRF_LOGD("%s: SUPL NI Response: notif_id=%d, user_response=%d", __FUNCTION__, notif_id, user_response);

		// SUPL NI User Response
		if (NULL != semNiResponse)
		{
			result = SIRF_PAL_OS_SEMAPHORE_Release( semNiResponse );
			if ( result != SIRF_SUCCESS )
			{
				SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
			}
		}
	}

	// Reset the handle. Assumption: Process only one notification (C-Plane or SUPL) at a time.
	g_cpaLcsInvokeHandle = (int) NULL;

	return;
}

static const GpsNiInterface sirfGpsNiInterface =
{
	sizeof(GpsNiInterface),
	sirf_gps_ni_init,
	sirf_gps_ni_respond
};


/* GpsDebugInterface!!!!!!!!!!!! */
static size_t sirf_gps_get_internal_state(char* buffer, size_t bufferSize)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if (bufferSize > 0)
	{
		strncpy(buffer, "GPS SiRF Debug Text", bufferSize-1);
	}

	return strlen(buffer);
}

static const GpsDebugInterface sirfGpsDebugInterface =
{
	sizeof(GpsDebugInterface),
	sirf_gps_get_internal_state,
};


/* AGpsRilInterface Interface */
static void sirf_agps_ril_init( AGpsRilCallbacks* callbacks )
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if(callbacks == NULL)
	{
		SIRF_LOGE("ERROR : AGpsRilCallbacks NULL Error!!!!");
		return;
	}
	gpsRilCallbacks.request_setid = callbacks->request_setid;
	gpsRilCallbacks.request_refloc = callbacks->request_refloc;

	return;
}

static void sirf_agps_ril_set_ref_location (const AGpsRefLocation *agps_reflocation, size_t sz_struct)
{
	int type, mcc, mnc, lac, cid;
	AGpsRefLocation *pAGpsRefLoc = NULL;
	LSM_BOOL result;

	SIRF_LOGD("%s: called", __FUNCTION__);

	pAGpsRefLoc = (AGpsRefLocation*) agps_reflocation;
	if(pAGpsRefLoc == NULL)
	{
		SIRF_LOGE("ERROR : RefLocation NULL !!!!");
		return;
	}

	type = pAGpsRefLoc->type;

	// For CMCC
	// get mcc, mnc from http://en.wikipedia.org/wiki/Mobile_Network_Code
	if (pAGpsRefLoc->u.cellID.mcc == 460 &&
			(pAGpsRefLoc->u.cellID.mnc == 0 ||
			 pAGpsRefLoc->u.cellID.mnc == 2 ||
			 pAGpsRefLoc->u.cellID.mnc == 7))
	{
		type = AGPS_REF_LOCATION_TYPE_GSM_CELLID;
#ifdef CSR_GPS_CONFIG
		csr_property_load_cmcc_config();
		SIRF_LOGI("Load CMCC SUPL settings in config file\n");
#endif
	}

	mcc = pAGpsRefLoc->u.cellID.mcc;
	mnc = pAGpsRefLoc->u.cellID.mnc;
	lac = pAGpsRefLoc->u.cellID.lac;
	cid = pAGpsRefLoc->u.cellID.cid;

	if(cid != -1)
	{
		g_GSD4tConfig.isCellInfoValid = LSM_TRUE;
	}
	else //No Service
	{
		g_GSD4tConfig.isCellInfoValid = LSM_FALSE;
		g_GSD4tConfig.CellIDInfo.eNetworkType = LSM_INVALID_NT_TYPE;
		SIRF_LOGW("Cell ID is not available");
	}

	if(type == AGPS_REF_LOCATION_TYPE_GSM_CELLID)
	{
		g_GSD4tConfig.CellIDInfo.eNetworkType = LSM_GSM;
		g_GSD4tConfig.CellIDInfo.m.gsm_cellid.mcc = (LSM_UINT16) mcc;
		g_GSD4tConfig.CellIDInfo.m.gsm_cellid.mnc = (LSM_UINT16) mnc;
		g_GSD4tConfig.CellIDInfo.m.gsm_cellid.lac = (LSM_UINT16) lac;
		g_GSD4tConfig.CellIDInfo.m.gsm_cellid.cid = (LSM_UINT16) cid;
		SIRF_LOGI("GSM_CELLID : mcc = %u, mnc = %u, lac = %u, cid = %u", mcc, mnc, lac, cid);
	}
	else if (type == AGPS_REF_LOCATION_TYPE_UMTS_CELLID)
	{
		g_GSD4tConfig.CellIDInfo.eNetworkType = LSM_WCDMA;
		g_GSD4tConfig.CellIDInfo.m.wcdma_cellid.mcc = (LSM_UINT16) mcc;
		g_GSD4tConfig.CellIDInfo.m.wcdma_cellid.mnc = (LSM_UINT16) mnc;
		g_GSD4tConfig.CellIDInfo.m.wcdma_cellid.ucid = (LSM_UINT32) cid;
		SIRF_LOGI("WCDMA_CELLID : mcc = %u, mnc = %u, cid = %u", mcc, mnc, cid);
	}

	result = CP_SendCellInfo(&(g_GSD4tConfig.CellIDInfo), LSM_TRUE);

	SIRF_LOGD( "CP_SendCellInfo result = %d", (int)result);

	if(NULL != semRilEvent)
	{
		tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release( semRilEvent);
		if ( result != SIRF_SUCCESS )
		{
			SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
		}
	}

	return;
}

static void sirf_agps_ril_set_set_id(AGpsSetIDType type, const char* setid)
{
	LSM_UINT8 SETidValue[SET_ID_LENGTH];
	LSM_UINT8 setid_converted[16];
	eLSM_SET_IDENTIFICATION setidtype;
	int i = 0;
	int error = 0;
	int set_id_length = 0;

	SIRF_LOGD("%s: called", __FUNCTION__);

	if(LSM_TRUE == isSETIDIMSI())
	{
		setidtype = LSM_IMSI;
	}
	else
	{
		setidtype = LSM_MSISDN;
	}
	if((type != AGPS_SETID_TYPE_IMSI) && (LSM_IMSI == setidtype))
	{
		SIRF_LOGE("IMSI : SETID TYPE ERROR : %d", type);
		error = 1;
	}
	else if((type != AGPS_SETID_TYPE_MSISDN) && (LSM_MSISDN == setidtype))
	{
		SIRF_LOGE("MSISDN : SETID TYPE ERROR : %d", type);
		error = 1;
	}

	if(setid == NULL)
	{
		SIRF_LOGE("SETID NULL ERROR");
		error = 1;
	}

	set_id_length = strlen(setid);

	SIRF_LOGI("SET ID = %s, length = %d", setid, set_id_length);

	if((set_id_length != 15) && (LSM_IMSI == setidtype))
	{
		SIRF_LOGE("SETID IMSI Length Error");
		error = 1;
	}

	if((set_id_length > 15) && (LSM_MSISDN == setidtype))
	{
		SIRF_LOGE("SETID MSISDN Length Error");
		error = 1;
	}

	if(setidtype == LSM_MSISDN)  
	{
		memset(setid_converted, 0xf, sizeof(setid_converted));
	}
	else
	{
		memset(setid_converted, 0, sizeof(setid_converted));
	}

	if(!error)
	{
		for(i =0; i < set_id_length; i++)
		{
			if( ((*(setid + i)) >= '0') && ((*(setid + i)) <= '9'))
			{
				setid_converted[i] = (*(setid+i)) - '0';
			}
			else
			{
				setid_converted[i] = (*(setid+i)) - 'A' + 10;
			}
		}

		for( i = 0 ; i < 8; i++)
		{
			SETidValue[i] = (setid_converted[i*2] << 4) | setid_converted[i*2 + 1];
		}

		g_GSD4tConfig.isSETIDInfoValid = LSM_TRUE;
		g_GSD4tConfig.SETIDInfo.SETidType = setidtype;
		memcpy(g_GSD4tConfig.SETIDInfo.SETidValue, SETidValue, SET_ID_LENGTH);

		if( LSM_TRUE == CP_SendSETIDInfo(&(g_GSD4tConfig.SETIDInfo), LSM_TRUE))
		{
			SIRF_LOGD("SETID Info Valid : CP_SendSETIDInfo Success");
		}
		else
		{
			SIRF_LOGE("CP_SendSETIDInfo Failed!!");
		}
	}

	if(NULL != semRilEvent)
	{
		tSIRF_RESULT result = SIRF_PAL_OS_SEMAPHORE_Release( semRilEvent );
		if ( result != SIRF_SUCCESS )
		{
			SIRF_LOGE( "APP: ERROR: SIRF_PAL_OS_SEMAPHORE_Release() error: 0x%X\n", result );
		}
	}

	return;
}

static void sirf_agps_ril_ni_message (uint8_t *msg, size_t len)
{
	int i = 0;
	char temp[2];
	char text[SUPL_INIT_MSG_LENGTH];

	SIRF_LOGD("%s: called", __FUNCTION__);

	text[0] = '\0';

	for(i =0; i < (int)len; i++)
	{
		sprintf(temp, "%02X", msg[i]);
		strcat(text, temp);
	}

	SIRF_LOGD("SUPL NI Message - length %d", len);
	SIRF_LOGD("%s", text);

	/* copy the sms or wap push into the SUPL INIT Msg buff after emptying the buffer */
	supl_init_msg_length = len;
	memset(SUPL_INIT_MESSAGE, 0, SUPL_INIT_MSG_LENGTH);
	memcpy(SUPL_INIT_MESSAGE, msg, supl_init_msg_length);

	pthread_mutex_lock(&sAndroidSessionMutex);
	if(SIRF_SUCCESS != android_session_handler_resume(ANDROID_NI_SESSION) )
	{
		SIRF_LOGE("android_session_handler_resume fail");
	}
	pthread_mutex_unlock(&sAndroidSessionMutex);

	return;
}

static void sirf_agps_ril_update_network_state(int connected, int type, int roaming, const char* extra_info)
{
	SIRF_LOGD("%s: called - connected: %d, type: %d, roaming:%d, extra:%s", __FUNCTION__, connected, type, roaming, extra_info);
}

static void sirf_agps_ril_update_network_availability(int avaiable, const char* apn)
{
	SIRF_LOGI("%s: called", __FUNCTION__);
	SIRF_LOGD("network available: %d apn:%s", avaiable, apn);

}

static const AGpsRilInterface sirfAGpsRilInterface =
{
	sizeof(AGpsRilInterface),
	sirf_agps_ril_init,
	sirf_agps_ril_set_ref_location,
	sirf_agps_ril_set_set_id,
	sirf_agps_ril_ni_message,
	sirf_agps_ril_update_network_state,
	sirf_agps_ril_update_network_availability,
};

#ifdef RAGPS_MISC_CALLBACK
void  sirf_agps_misc_init( AGpsMiscCallbacks* callbacks )
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if( callbacks != NULL )
	{
		agpsMiscCallbacks  = *callbacks;
	}
	else
	{
		SIRF_LOGE("AGpsMiscCallbacks ERROR");
	}

	return;
}


//in the future, add the sensor interface here
static const AGpsMiscInterface sirfAGpsMiscInterface =
{
	sizeof(AGpsMiscInterface),
	sirf_agps_misc_init,
};

#endif

#ifdef TEST_MODE_SHARED_LIB

void sirf_gps_testmode_result_cb(SiRFGpsTestModeResult * result)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if(result == NULL)
	{
		SIRF_LOGE("ERROR : SiRFGpsTestModeResult NULL");
		return;
	}

	if(result->CompleteFlag != 1)
	{
		SIRF_LOGD("Test Mode - Current CN0 = %f , TOW = %f", result->CN0, result->gps_tow);
	}
	else
	{
		SIRF_LOGD("Test Mode Complete");
		SIRF_LOGD("CN0 Mean = %f, Sigma = %f ", result->cno_mean, result->cno_sigma);
	}

	return;
}

SiRFGpsTestModeResultCallbacks sGpsTestModeResultCallbacks =
{
	sirf_gps_testmode_result_cb,
};

static int sirf_testmode_init()
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	dlHandle = dlopen(GPS_TESTMODE_SHARED_LIB_PATH, RTLD_NOW);
	if (dlHandle == NULL)
	{
		SIRF_LOGE("Error  :Dynamic Loading of Testmode Shared Lib!!!: %s\n", dlerror());
		return -1;
	}

	p_get_gps_testmode_interface = (const SiRFGpsTestModeInterface* (*)()) dlsym(dlHandle, "get_gps_testmode_interface");
	if (p_get_gps_testmode_interface == NULL)
	{
		SIRF_LOGE("Symbol 'get_gps_testmode_interface' was not found in %s\n", GPS_TESTMODE_SHARED_LIB_PATH);
		return -1;
	}

	sGpsTestModeInterface = p_get_gps_testmode_interface();
	if(sGpsTestModeInterface == NULL)
	{
		SIRF_LOGE("get_gps_testmode_interface fail!!!!");
		return -1;
	}

	sGpsTestModeInterface->init(&sGpsTestModeResultCallbacks); //register callback function!!!!

	return 0;
}

int sirf_testmode_start(int mode, int sv_id, int period)
{
	int ret = 0;

	SIRF_LOGD("%s: called. mode:%d, sv_id:%d, period:%d", __FUNCTION__, mode, sv_id, period);

	if(sGpsTestModeInterface == NULL)
	{
		ret = sirf_testmode_init();
		if(ret != SIRF_SUCCESS)
		{
			SIRF_LOGE("ERROR : sirf_testmode_int Fail!!!!");
			return -1;
		}
	}

	ret = sGpsTestModeInterface->start(TEST_MODE_5, sv_id, period);
	if(ret != SIRF_SUCCESS)
	{
		SIRF_LOGE("ERROR : sirf_testmode_start Fail!!!!");
		return -1;
	}

	return 0;
}

int sirf_testmode_stop(void)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	if(sGpsTestModeInterface == NULL)
	{
		SIRF_LOGE("ERROR : sGpsTestModeInterface NULL!!!");
		return -1;
	}

	sGpsTestModeInterface->stop();

	return 0;
}

#endif


/*
Description:
This funtion is called from SiRFNav_Output
report sv status info when MO Session only!!!
 */
void sirf_gps_send_sv_status(SiRFGpsSvStatus* pGpsSvStatus)
{
	int num_svs;
	int i;

	if(g_SessionID == INVAILD_SESSION_HANDLE || g_GSD4tConfig.isTestModeEnable)
	{
		return;
	}

	memset(&androidSVStatus, 0, sizeof(GpsSvStatus));

	for(i = 0, num_svs = 0; i < 12; i++)
	{
		if((pGpsSvStatus->sv_list[i].prn > 0
					&& pGpsSvStatus->sv_list[i].prn <= 32) && (pGpsSvStatus->sv_list[i].snr !=0)) //need to check
		{
			androidSVStatus.sv_list[num_svs].prn       = pGpsSvStatus->sv_list[i].prn;
			androidSVStatus.sv_list[num_svs].snr       = pGpsSvStatus->sv_list[i].snr;
			androidSVStatus.sv_list[num_svs].elevation = pGpsSvStatus->sv_list[i].elevation;
			androidSVStatus.sv_list[num_svs].azimuth   = pGpsSvStatus->sv_list[i].azimuth;
#if 0
			SIRF_LOGD("SV PRN = %d, SNR = %f, Elev = %f, Azi = %f", 
					svStatus.sv_list[num_svs].prn,
					svStatus.sv_list[num_svs].snr,
					svStatus.sv_list[num_svs].elevation,
					svStatus.sv_list[num_svs].azimuth);
#endif
			num_svs++;
		}
	}
	androidSVStatus.num_svs          = num_svs;
	androidSVStatus.ephemeris_mask   = pGpsSvStatus->ephemeris_mask;
	androidSVStatus.almanac_mask     = pGpsSvStatus->almanac_mask;
	pthread_mutex_lock(&sPosQosMetMutex);
	if(posQoSMet == 1) //first fix
	{
		saved_used_in_fix_mask = pGpsSvStatus->used_in_fix_mask;
		posQoSMet = 2;
	}
	else if (posQoSMet == 2) //after first fix
	{
		if(!pGpsSvStatus->nav_valid)
		{
			saved_used_in_fix_mask = pGpsSvStatus->used_in_fix_mask;
		}

	}
	pthread_mutex_unlock(&sPosQosMetMutex);
	androidSVStatus.used_in_fix_mask = saved_used_in_fix_mask;
	androidSVStatus.size = sizeof(GpsSvStatus);
	//SIRF_LOGV( " SV Status info: eph mask: 0x%x, almanac mask 0x%x, fix_num 0x%x, num_sv = %d\n", androidSVStatus.ephemeris_mask, androidSVStatus.almanac_mask, pGpsSvStatus->used_in_fix_mask , androidSVStatus.num_svs);
	android_status_report(ANDROID_STATUS_REPORT_SATELITES);

	return;
}

#if ENABLE_DEBUG_LISTENER
tSIRF_VOID LSM_Debug_Listener( LSM_CHAR* pMsg )
{
	char data_string[256]= {0};
	tSIRF_DATE_TIME date_time;

	if(NULL == pMsg)
	{
		SIRF_LOGW("APP: NULL debug Message.");
		return;
	}

	if (SIRF_PAL_OS_TIME_DateTime(&date_time) == SIRF_SUCCESS)
	{
		snprintf(data_string, sizeof(data_string) - 1 , "<%4d:%02d:%02d:%02d:%02d:%02d> %s",
				date_time.year, date_time.month, date_time.day,
				date_time.hour, date_time.minute, date_time.second,
				pMsg );

		SIRF_LOGD(data_string);
	}
	else
	{
		SIRF_LOGD(pMsg);
	}
}
#endif /* ENABLE_DEBUG_LISTENER */

#if ENABLE_SSB_LISTENER
tSIRF_VOID LSM_SSB_Listener( LSM_UINT8* message, LSM_UINT32 messageLength )
{
	char msg[512];

	snprintf(msg, sizeof(msg)-1, ">> %02X %02X %02X %02X %02X %02X %02X %02X\n",
			*(message),
			*(message+1),
			*(message+2),
			*(message+3),
			*(message+4),
			*(message+5),
			*(message+6),
			*(message+7) );
	SIRF_LOGV(msg);
}
#endif /* ENABLE_SSB_LISTENER */

#if ENABLE_NMEA_LISTENER
tSIRF_VOID SiRF_NMEA_Listener( LSM_UINT8 type, LSM_UINT8* message, LSM_UINT32 messageLength )
{
	time_t cur_time = {0};

	//SIRF_LOGD("%s: called. messageLength=%d", __FUNCTION__, messageLength);

	time ( &cur_time );

	if(g_SessionID == INVAILD_SESSION_HANDLE || g_GSD4tConfig.isTestModeEnable) //MO Session Only
	{
		//SIRF_LOGD("%s: do nothing!!!", __FUNCTION__);
		return;
	}

	if(messageLength >= NMEA_SENTENCE_LENGTH)
	{
		SIRF_LOGE("ERROR : NMEA Length Overflow!!!!");
		return;
	}

	if(type == 1)
	{
		//SIRF_LOGD("just save GPGGA String - 1");
		memset(NMEA_GPGGA, 0, NMEA_SENTENCE_LENGTH);
		memcpy( NMEA_GPGGA, message, messageLength );
		NMEA_GPGGA[messageLength] = 0;
		NMEA_GPGGA_LEN = messageLength;
		NMEA_GPGGA_TIMESTAMP = cur_time;
		return;
	}
	else if(type == 2) //GPRMC String do nothing and save GPRMC String
	{
		//SIRF_LOGD("just save GPRMC String - 2");
		memset(NMEA_GPRMC, 0, NMEA_SENTENCE_LENGTH);
		memcpy( NMEA_GPRMC, message, messageLength );
		NMEA_GPRMC[messageLength] = 0;
		NMEA_GPRMC_LEN = messageLength;
		NMEA_GPRMC_TIMESTAMP = cur_time;
		return;
	}
#if 0 /* Block GPGLL output */
	else if(type == 4) //GPGLL
	{
		//SIRF_LOGV("just save GPGLL String - 4");
		memset(NMEA_GPGLL, 0, NMEA_SENTENCE_LENGTH);
		memcpy( NMEA_GPGLL, message, messageLength );
		NMEA_GPGLL[messageLength] = 0;
		NMEA_GPGLL_LEN = messageLength;
		NMEA_GPGLL_TIMESTAMP = cur_time;
		return;
	}
#endif
	android_nmea_report(cur_time, (char *)message, messageLength);

#if ENABLE_NMEA_FILE
	message[messageLength] = 0;
	NMEAPRINTF((char *)message);
#endif
}

#endif /* ENABLE_NMEA_LISTENER */

#if ENABLE_SERIAL_LISTENER
static tSIRF_VOID LSM_Serial_Listener(LSM_UINT8* message, LSM_UINT32 messageLength)
{
	return;
}
#endif /* ENABLE_SERIAL_LISTENER */

typedef float elem_type;
int compare(const void *f1, const void *f2)
{
	return ( *(elem_type*)f1 > *(elem_type*)f2 ) ? 1 : -1; 
}

void quick_sort(elem_type * array, int n)
{
	qsort(array, n, sizeof(elem_type), compare);
	return;
}

tSIRF_VOID LSM_TestMode_Listener( LSM_UINT32 msg_id, LSM_UINT8* message, LSM_UINT32 msg_length)
{
#ifdef CUSTOMER_GPS_CONFIG
	CW_TESTMODE_RESULT tempCWTestModeResult;
	float CW_REPORT_VAL;
	float CN0;
	int i;

	FILE *fp = NULL;
	char txtCNOVal[20] = {0,};

	//SIRF_LOGI("%s entered, msgId: %d, len: %d", __FUNCTION__, msg_id, msg_length);

	switch(msg_id)
	{
		case SIRF_MSG_SSB_MEASURED_TRACKER:
			{
				tSIRF_MSG_SSB_MEASURED_TRACKER *measured_tracker_msg = (tSIRF_MSG_SSB_MEASURED_TRACKER *)message;
				CN0 = 0.0f;
				for ( i=0; i<SIRF_NUM_POINTS; i++ )
				{
					CN0 += (float)(measured_tracker_msg->chnl[0].cno[i]);
				}
				CN0 /= 10.0f;
#if 0
				SIRF_LOGD( "Meas Tracker - Time: %4.1f s, TOW:%6.2f s, Ch0 - SV:%2d, CN0:%4.1f dB-Hz\n",
						(float)(SIRF_PAL_OS_TIME_SystemTime() ) / 1000.0f,
						(float)measured_tracker_msg->gps_tow / 100.0f,
						measured_tracker_msg->chnl[0].svid,
						CN0 );
#endif
				sCNOTestCnt++;
				if(g_GSD4tConfig.isCNOTest && sCNOTestCnt >= CNOTestDiscardCnt)
				{
					androidSVStatus.sv_list[0].prn  = 1;
					androidSVStatus.sv_list[0].snr  = CN0;
					androidSVStatus.num_svs = 1;
					androidSVStatus.size = sizeof(GpsSvStatus);
					SIRF_LOGD("sv_status_cb called in SIRF_MSG_SSB_MEASURED_TRACKER: %d, %f",
							androidSVStatus.sv_list[0].prn,
							androidSVStatus.sv_list[0].snr);
					android_status_report(ANDROID_STATUS_REPORT_SATELITES);
				}

				break;
			}

		case SIRF_MSG_SSB_TEST_MODE_DATA_3: //For Test Mode 4 & Test Mode 6
			{
				tSIRF_MSG_SSB_TEST_MODE_DATA_3 tm5_struct;

				memset(&androidSVStatus,0,sizeof(GpsSvStatus));
				memcpy(&tm5_struct, message, sizeof(tSIRF_MSG_SSB_TEST_MODE_DATA_3));

				//SIRF_LOGD("SIRF_MSG_SSB_TEST_MODE_DATA_3 : Svid = %d, SNR = %f", tm5_struct.svid, ((float)(tm5_struct.cno_mean))/10);

				if (!g_GSD4tConfig.isCNOTest)
				{
					androidSVStatus.sv_list[0].prn       = tm5_struct.svid;
					androidSVStatus.sv_list[0].snr       = ((float)(tm5_struct.cno_mean))/10;
					androidSVStatus.num_svs = 1;
					androidSVStatus.size = sizeof(GpsSvStatus);
					SIRF_LOGD("sv_status_cb called in SIRF_MSG_SSB_TEST_MODE_DATA 3: %d, %f",
							androidSVStatus.sv_list[0].prn,
							androidSVStatus.sv_list[0].snr);
					android_status_report(ANDROID_STATUS_REPORT_SATELITES);
				}

				break;
			}

		case SIRF_MSG_SSB_TEST_MODE_DATA_7:
			{
				static tSIRF_MSG_SSB_TEST_MODE_DATA_7 tm7_struct;

				memcpy(&tm7_struct, message, sizeof(tSIRF_MSG_SSB_TEST_MODE_DATA_7));
				/* capture the test mode messages here and call the callback functions */
				SIRF_LOGD("tm7 >>  stat: %d, spur1 : %10d %2.1f, spur2: %10d %2.1f, spur3 : %10d %2.1f, spur4: %10d %2.1f",
						tm7_struct.test_status,
						(int) tm7_struct.spur1_frequency,
						(tm7_struct.spur1_sig_to_noise / 10.0),
						(int) tm7_struct.spur2_frequency,
						(tm7_struct.spur2_sig_to_noise/ 10.0),
						(int) tm7_struct.spur3_frequency,
						(tm7_struct.spur3_sig_to_noise / 10.0),
						(int) tm7_struct.spur4_frequency,
						(tm7_struct.spur4_sig_to_noise / 10.0));

				tempCWTestModeResult.cno = 0;
				tempCWTestModeResult.freq = 0;
				if((tm7_struct.spur1_frequency > (1575620000 - CW_FREQUENCE_RANGE)) &&
						(tm7_struct.spur1_frequency < (1575620000 + CW_FREQUENCE_RANGE)))
				{
					tempCWTestModeResult.cno  = tm7_struct.spur1_sig_to_noise / 10.0;
					tempCWTestModeResult.freq = tm7_struct.spur1_frequency;
				}

				if((tm7_struct.spur2_frequency > (1575620000 - CW_FREQUENCE_RANGE)) &&
						(tm7_struct.spur2_frequency < (1575620000 + CW_FREQUENCE_RANGE)) && ((tm7_struct.spur2_sig_to_noise /10.0) >= tempCWTestModeResult.cno))
				{
					tempCWTestModeResult.cno  = tm7_struct.spur2_sig_to_noise / 10.0;
					tempCWTestModeResult.freq = tm7_struct.spur2_frequency;
				}

				if((tm7_struct.spur3_frequency > (1575620000 - CW_FREQUENCE_RANGE)) &&
						(tm7_struct.spur3_frequency < (1575620000 + CW_FREQUENCE_RANGE)) && ((tm7_struct.spur3_sig_to_noise / 10.0) >= tempCWTestModeResult.cno))
				{
					tempCWTestModeResult.cno  = tm7_struct.spur3_sig_to_noise / 10.0;
					tempCWTestModeResult.freq = tm7_struct.spur3_frequency;
				}

				if((tm7_struct.spur4_frequency > (1575620000 - CW_FREQUENCE_RANGE)) &&
						(tm7_struct.spur4_frequency < (1575620000 + CW_FREQUENCE_RANGE)) && ((tm7_struct.spur4_sig_to_noise / 10.0) >= tempCWTestModeResult.cno))
				{
					tempCWTestModeResult.cno  = tm7_struct.spur4_sig_to_noise / 10.0;
					tempCWTestModeResult.freq = tm7_struct.spur4_frequency;
				}

				if(CWTestModeResultPeakValue.cno <= tempCWTestModeResult.cno)
				{
					CWTestModeResultPeakValue.cno = tempCWTestModeResult.cno;
					CWTestModeResultPeakValue.freq = tempCWTestModeResult.freq;
				}

				if(tm7_struct.test_status)
				{
					SIRF_LOGD("received final Peak CW data>> %f, %10u", CWTestModeResultPeakValue.cno, (unsigned int)CWTestModeResultPeakValue.freq);

					final_tm_data.is_valid = tm7_struct.test_status;
					final_tm_data.cno = (CWTestModeResultPeakValue.cno*10);
					final_tm_data.freq = (unsigned int)CWTestModeResultPeakValue.freq;

					/* Example on how to write output to a file 
					   fp = fopen("/tmp/sv_cno.info","w+");

					   if(fp != NULL) 
					   {
					   SIRF_LOGD("##### factory test mode : file write : SNR = %.1f #####", CWTestModeResultPeakValue.cno);
					   fwrite(txtCNOVal, 1,strlen(txtCNOVal), fp);
					   fclose(fp);
					   }
					   else
					   {
					   SIRF_LOGE("CW Test Mode : File Open Error!!!!");
					   }
					 */		
					CWTestModeResultPeakValue.cno = 0;
					CWTestModeResultPeakValue.freq = 0;

				}
				break;
			}
	}
#else
	char msg[512];

	switch (msg_id)
	{
		case SIRF_MSG_SSB_TEST_MODE_DATA_3:
			{
				GpsSvStatus svStatus;
				tSIRF_MSG_SSB_TEST_MODE_DATA_3 tm5_struct;

				memset(&svStatus, 0, sizeof(GpsSvStatus));
				svStatus.size = sizeof(GpsSvStatus);
				memcpy(&tm5_struct, message, sizeof(tSIRF_MSG_SSB_TEST_MODE_DATA_3));

				if (gpsCallbacks.sv_status_cb && tm5_struct.svid )
				{
					svStatus.sv_list[0].prn = tm5_struct.svid;
					svStatus.sv_list[0].snr = tm5_struct.cno_mean/10;
					svStatus.num_svs = 1;
					android_status_report(ANDROID_STATUS_REPORT_SATELITES);
					SIRF_LOGD("sv_status_cb called in SIRF_MSG_SSB_TEST_MODE_DATA 3: %d, %f",
							svStatus.sv_list[0].prn,
							svStatus.sv_list[0].snr);
				}
				break;
			}

		case SIRF_MSG_SSB_TEST_MODE_DATA:
			{
				GpsSvStatus svStatus;

				memset(&svStatus,0,sizeof(GpsSvStatus));
				svStatus.size = sizeof(GpsSvStatus);
				memcpy(&tm4_struct, message, sizeof(tSIRF_MSG_SSB_TEST_MODE_DATA));
				if (gpsCallbacks.sv_status_cb && tm4_struct.svid)
				{
					svStatus.sv_list[0].prn = tm4_struct.svid;
					svStatus.sv_list[0].snr = tm4_struct.cno_mean/10;
					svStatus.num_svs = 1;

					android_status_report(ANDROID_STATUS_REPORT_SATELITES);

					SIRF_LOGD("sv_status_cb called in SIRF_MSG_SSB_TEST_MODE_DATA: %d, %f",
							svStatus.sv_list[0].prn,
							svStatus.sv_list[0].snr);
				}
				// capture the test mode messages here and call the callback functions
				snprintf(msg, sizeof(msg)-1, ">> %02d %02d",
						tm4_struct.svid,
						tm4_struct.cno_mean);
				SIRF_LOGD(msg);
				break;
			}

		case SIRF_MSG_SSB_TEST_MODE_DATA_7:     // 0x07 0x3F
			{
				static tSIRF_MSG_SSB_TEST_MODE_DATA_7 tm7_struct;

				memcpy(&tm7_struct, message, sizeof(tSIRF_MSG_SSB_TEST_MODE_DATA_7));
				// capture the test mode messages here and call the callback functions
				snprintf(msg, sizeof(msg)-1, "tm7 >> %10d %10d, %10d %10d, %10d %10d, %10d %10d",
						(int) tm7_struct.spur1_frequency,
						tm7_struct.spur1_sig_to_noise,
						(int) tm7_struct.spur2_frequency,
						tm7_struct.spur2_sig_to_noise,
						(int) tm7_struct.spur3_frequency,
						tm7_struct.spur3_sig_to_noise,
						(int) tm7_struct.spur4_frequency,
						tm7_struct.spur4_sig_to_noise);
				SIRF_LOGD(msg);

				if ((tm7_struct.spur1_frequency > (1575620000 - 1000)) &&
						(tm7_struct.spur1_frequency < (1575620000 + 1000)))
				{
					temp_tm_data.cno  = tm7_struct.spur1_sig_to_noise;
					temp_tm_data.freq = tm7_struct.spur1_frequency;

					snprintf(msg, sizeof(msg)-1, "tm7 in range >> %10d %10d ",
							(int) tm7_struct.spur1_frequency,
							tm7_struct.spur1_sig_to_noise);
				}

				else if ((tm7_struct.spur2_frequency > (1575620000 - 1000)) &&
						(tm7_struct.spur2_frequency < (1575620000 + 1000)))
				{
					temp_tm_data.cno  = tm7_struct.spur2_sig_to_noise;
					temp_tm_data.freq = tm7_struct.spur2_frequency;

					snprintf(msg, sizeof(msg)-1, "tm7 in range >> %10d %10d",
							(int) tm7_struct.spur2_frequency,
							tm7_struct.spur2_sig_to_noise);
				}

				else if ((tm7_struct.spur3_frequency > (1575620000 - 1000)) &&
						(tm7_struct.spur3_frequency < (1575620000 + 1000)))
				{
					temp_tm_data.cno  = tm7_struct.spur3_sig_to_noise;
					temp_tm_data.freq = tm7_struct.spur3_frequency;

					snprintf(msg, sizeof(msg)-1, "tm7 in range >> %10d %10d",
							(int) tm7_struct.spur3_frequency,
							tm7_struct.spur3_sig_to_noise);
				}

				else if ((tm7_struct.spur4_frequency > (1575620000 - 1000)) &&
						(tm7_struct.spur4_frequency < (1575620000 + 1000)))
				{
					snprintf(msg, sizeof(msg)-1, "tm7 in range >> %10d %10d",
							(int) tm7_struct.spur4_frequency,
							tm7_struct.spur4_sig_to_noise);
					temp_tm_data.cno  = tm7_struct.spur4_sig_to_noise;
					temp_tm_data.freq = tm7_struct.spur4_frequency;
				}

				if (tm7_struct.test_status)
				{
					tm_data[tm_count].cno = temp_tm_data.cno;
					tm_data[tm_count].freq = temp_tm_data.freq;
					tm_data[tm_count].is_valid = LSM_TRUE;
					temp_tm_data.cno = 0;
					temp_tm_data.freq = 0;
					tm_count += 1;
					if (10 == tm_count)
					{
						int i;

						tm_count = 0;
						final_tm_data.cno = 0;
						for (i = 0; i < 10; i++)
						{
							final_tm_data.cno += tm_data[i].cno;
						}
						final_tm_data.cno /= 10;
						final_tm_data.freq = tm_data[9].freq;
						final_tm_data.is_valid = LSM_TRUE;
					}
					SIRF_LOGD("received final TM data>> %d, %10d, %10d",final_tm_data.is_valid,final_tm_data.cno, final_tm_data.freq);
				}
			}
	}
#endif
}

static void LSM_Position_CallBack(LsmCallbackData *pLsmCallbackData)
{
	tSIRF_RESULT result;
	int i = 0, j = 0;
	int location_report_check = 0;

	SIRF_LOGD("%s: called", __FUNCTION__);

	if (LSM_SUPL_NETWORK_RESUME & pLsmCallbackData->eLsmEvent)
	{
		SIRF_LOGD("APP: INFO: LSM_SUPL_NETWORK_RESUME");

		/* after checking the state, return current status */

		/* for NI and MSA cases, data connection status is checked before session begins */

		if(GPS_POSITION_MODE == GPS_POSITION_MODE_MS_BASED && g_SessionID != INVAILD_SESSION_HANDLE)
		{
			if(LSM_TRUE != requestAllRilInfo())
			{
				pLsmCallbackData->eLsmEvent = LSM_SUPL_NETWORK_FAILED;
			}
			else
			{
				pLsmCallbackData->eLsmEvent = LSM_SUPL_NETWORK_RESUME; /* keep resume state */
			}
		}
	}

	if (LSM_SUPL_NETWORK_CLOSED & pLsmCallbackData->eLsmEvent)
	{
		SIRF_LOGD("APP: INFO: LSM_SUPL_NETWORK_CLOSED");
		releaseDataConnection();
	}

	if (LSM_SUPL_NETWORK_FAILED & pLsmCallbackData->eLsmEvent)
	{
		SIRF_LOGD("APP: INFO: LSM_SUPL_NETWORK_FAILED");
		releaseDataConnection();
	}    

	if(pLsmCallbackData->locReport.ReportDataBitMap.isGPSInfoSet == 1)
	{

	}

	if (LSM_LPL_REF_LOC & pLsmCallbackData->eLsmEvent)
	{
		/* Cell Based Positioning. */
		SIRF_LOGD("got reference location");
	}

	if ( LSM_LPL_LOC_W_QOS & pLsmCallbackData->eLsmEvent )
	{
		pthread_mutex_lock(&sPosQosMetMutex);
		if(posQoSMet == 0)
		{
			posQoSMet = 1;
		}
		pthread_mutex_unlock(&sPosQosMetMutex);
		locationInfo.flags = ( GPS_LOCATION_HAS_LAT_LONG \
				|GPS_LOCATION_HAS_ALTITUDE \
				|GPS_LOCATION_HAS_SPEED \
				|GPS_LOCATION_HAS_BEARING \
				|GPS_LOCATION_HAS_ACCURACY );

		locationInfo.longitude = pLsmCallbackData->locReport.Coordinate.longitude;
		locationInfo.latitude = pLsmCallbackData->locReport.Coordinate.latitude;
		locationInfo.altitude = pLsmCallbackData->locReport.Coordinate.altitude;
		locationInfo.bearing = pLsmCallbackData->locReport.heading;
		locationInfo.speed = (pLsmCallbackData->locReport.Velocity.horizontalVelocity);
		locationInfo.timestamp = pLsmCallbackData->locReport.timeStamp;
		locationInfo.accuracy = pLsmCallbackData->locReport.Coordinate.horUncert;

		//gpsUtcTime = pLsmCallbackData->locReport.timeStamp; //for NMEA Time Stamp!!!

		if(g_GSD4tConfig.isTestModeEnable == LSM_TRUE)
		{
			SIRF_LOGD("Test Mode Enabled in LSM_Position_Callback");
			return;
		}

		posCnt++;

		if((g_GSD4tConfig.numberOfFixes != 0) && (posCnt > g_GSD4tConfig.numberOfFixes))
		{
			location_report_check = 0;
			SIRF_LOGD("Location Report Disabled");
		}
		else
		{
			location_report_check = 1;
		}

#if ENABLE_NMEA_FILE
		if( g_SessionID != INVAILD_SESSION_HANDLE && location_report_check)
		{
			NMEAPRINTF(NMEA_GPGGA);
			NMEAPRINTF(NMEA_GPRMC);
			//NMEAPRINTF(NMEA_GPGLL);
		}
#endif

		if(g_SessionID != INVAILD_SESSION_HANDLE && location_report_check) //report location info when MO session only!!
		{
			android_nmea_report(NMEA_GPGGA_TIMESTAMP, NMEA_GPGGA, NMEA_GPGGA_LEN);
			android_nmea_report(NMEA_GPRMC_TIMESTAMP, NMEA_GPRMC, NMEA_GPRMC_LEN);
			//android_nmea_report(NMEA_GPGLL_TIMESTAMP,  NMEA_GPGLL, NMEA_GPGLL_LEN);
			android_status_report(ANDROID_STATUS_REPORT_LOCATION);

			SIRF_LOGI("Position Fix (%d of %d) : Latitude %lf   Longtitude %lf", posCnt, g_GSD4tConfig.numberOfFixes, locationInfo.latitude, locationInfo.longitude);
		}

	}

	if ( LSM_TO_CLOSE & pLsmCallbackData->eLsmEvent )
	{
		SIRF_LOGD("got LSM_TO_CLOSE  but NI session status = %d",g_NI_Session);

		/* SUPL NI Session */
		if(g_NI_Session == NI_SESSION_RUNNING)
		{
			pthread_mutex_lock(&sAndroidSessionMutex);
			g_NI_Session = NI_SESSION_CLOSING;
			if(SIRF_SUCCESS != android_session_handler_suspend())
			{
				SIRF_LOGD("android_session_handler_suspend fail!!!");
			}
			pthread_mutex_unlock(&sAndroidSessionMutex);
		}
	}

	if( LSM_CP_SESSION_STARTED & pLsmCallbackData->eLsmEvent )
	{
		SIRF_LOGI("CP Session started!!");
		pthread_mutex_lock(&sAndroidSessionMutex);
		if(SIRF_SUCCESS != android_session_handler_resume(ANDROID_CP_SESSION) )
		{
			SIRF_LOGE("android_session_handler_resume failed for CP session!!!");
		}
		pthread_mutex_unlock(&sAndroidSessionMutex);
	}
}

#if ENABLE_SUPL_INIT_LISTENER

#define UCS2_ENCODING 0
#define GSM_DEFAULT_ENCODING 1
#define UTF8_ENCODING 2


/* From OMA SUPL 1.0
   noNotificationNoVerification(0), notificationOnly(1),
   notificationAndVerficationAllowedNA(2),
   notificationAndVerficationDeniedNA(3), privacyOverride(4),
 */

typedef enum
{
	noNotificationNoVerification        = 0,
	notificationOnly                    = 1,
	notificationAndVerficationAllowedNA = 2,
	notificationAndVerficationDeniedNA  = 3,
	privacyOverride                     = 4
} NotificationType;


tSIRF_VOID LSM_SUPL_Init_Listener(LSM_AGPS_SUPL_INIT* pSuplInitData,
		LSM_AGPS_UserResponse* pUserResponse, tSIRF_VOID* pUserData)
{
	LSM_UINT32 encType;

	/* Do Something related with NI Verification and notification here!!!!
	   niNotification.notification_id = (int)(*(pSuplInitData->notification->requestorId)); */

	SIRF_LOGD("%s: called", __FUNCTION__);

	if(pSuplInitData == NULL)
	{
		SIRF_LOGE("LSM_Supl_Init_Listener : NULL Error");
		return;
	}

	if(pSuplInitData->m.notificationPresent != 1)
	{
		SIRF_LOGE("ERROR: No Notification Present");
		return;
	}

	/* Initialize niNotification!!!!! */
	memset(&niNotification, 0, sizeof(GpsNiNotification));

	niNotification.notification_id = getNotificationId();
	niNotification.ni_type = GPS_NI_TYPE_UMTS_SUPL;

	niNotification.requestor_id[0] = '\0';
	niNotification.text[0] = '\0';


	/* Comments from Android Framework!!!! */

	/*************************************************************************
	 *     A note about timeout
	 *     According to the protocol, in the need_notify and need_verify case,
	 *     a default response should be sent when time out.
	 *
	 *     In some GPS hardware, the GPS driver (under HAL) can handle the timeout case
	 *     and this class GpsNetInitiatedHandler does not need to do anything.
	 *
	 *     However, the UI should at least close the dialog when timeout. Further,
	 *     for more general handling, timeout response should be added to the Handler here.
	 */

	NotificationType notificationType = pSuplInitData->notification.notificationType;

	switch (notificationType)
	{
		case noNotificationNoVerification:
			{
				niNotification.notify_flags = 0; //Do nothing!!!!
				SIRF_LOGI("NotificationType : noNotificationNoVerification");
				break;
			}

		case notificationOnly:
			{
				niNotification.notify_flags = GPS_NI_NEED_NOTIFY;
				SIRF_LOGI("NotificationType : notificationOnly");
				break;
			}

		case notificationAndVerficationAllowedNA: //Allowed on No-Answer
			{
				niNotification.notify_flags = GPS_NI_NEED_NOTIFY | GPS_NI_NEED_VERIFY;
				niNotification.timeout = g_GSD4tConfig.defaultResponseTimeoutofNI;//15000; //specify Time out value here!!!!!
				niNotification.default_response = GPS_NI_RESPONSE_ACCEPT;
				SIRF_LOGI("NotificationType : notificationAndVerficationAllowedNA");
				break;
			}

		case notificationAndVerficationDeniedNA: //Denied on No-Answer
			{
				niNotification.notify_flags = GPS_NI_NEED_NOTIFY | GPS_NI_NEED_VERIFY;
				niNotification.timeout = g_GSD4tConfig.defaultResponseTimeoutofNI;//15000; //specify Time out value here!!!!!, timeout = 0 means that wait for response infinitely!!!
				niNotification.default_response = GPS_NI_RESPONSE_DENY;
				SIRF_LOGI("NotificationType : notificationAndVerficationDeniedNA");
				break;
			}

		case privacyOverride:
			{
				niNotification.notify_flags = GPS_NI_PRIVACY_OVERRIDE;
				SIRF_LOGI("NotificationType : privacyOverride");
				break;
			}

		default:
			{
				SIRF_LOGE("NotificationType : error!!!!");
				break;
			}
	}

	if(pSuplInitData->notification.m.requestorIdPresent == 1)
	{
		char temp[2];
		int i;

		SIRF_LOGD("requestorIdPresent");

		niNotification.requestor_id[0] = '\0';
		// Have to change requestorId format to Hex string, one byte reserved for '\0'
		if (pSuplInitData->notification.requestorIdLength > (GPS_NI_SHORT_STRING_MAXLEN - 1)/2)
		{
			SIRF_LOGE("ERROR:notification.requestorIdLength is too long");
		}
		else
		{
			for (i = 0; i < (int) pSuplInitData->notification.requestorIdLength; ++i)
			{
				sprintf(temp, "%02x", pSuplInitData->notification.requestorId[i]);
				strcat(niNotification.requestor_id, temp);
			}
			SIRF_LOGD("requestor_id:%s", niNotification.requestor_id);
		}

		SIRF_LOGD("requestor_id:%d : %X %X %X %X %X %X %X %X %X %X, ", (int)pSuplInitData->notification.requestorIdLength,
				niNotification.requestor_id[0],
				niNotification.requestor_id[1],
				niNotification.requestor_id[2],
				niNotification.requestor_id[3],
				niNotification.requestor_id[4],
				niNotification.requestor_id[5],
				niNotification.requestor_id[6],
				niNotification.requestor_id[7],
				niNotification.requestor_id[8],
				niNotification.requestor_id[9]);
	}

	if(pSuplInitData->notification.m.clientNamePresent == 1)
	{
		char temp[2];
		int i;

		SIRF_LOGD("clientNamePresent");

		//memcpy(niNotification.text, pSuplInitData->notification.clientName, pSuplInitData->notification.clientNameLength);
		niNotification.text[0] = '\0';
		// Have to change requestorId format to Hex string, one byte reserved for '\0'
		if (pSuplInitData->notification.clientNameLength > (GPS_NI_LONG_STRING_MAXLEN - 1)/2)
		{
			SIRF_LOGE("ERROR:notification.clientNameLength is too long");
		}
		else
		{
			for (i = 0; i < (int) pSuplInitData->notification.clientNameLength; ++i)
			{
				sprintf(temp, "%02x", pSuplInitData->notification.clientName[i]);
				strcat(niNotification.text, temp);
			}
			SIRF_LOGD("client name:%s", niNotification.text);
		}

		SIRF_LOGD("client name:%d : %x %x %x %x %x %x %x %x %x %x, ",(int)(pSuplInitData->notification.clientNameLength),
				pSuplInitData->notification.clientName[0],
				pSuplInitData->notification.clientName[1],
				pSuplInitData->notification.clientName[2],
				pSuplInitData->notification.clientName[3],
				pSuplInitData->notification.clientName[4],
				pSuplInitData->notification.clientName[5],
				pSuplInitData->notification.clientName[6],
				pSuplInitData->notification.clientName[7],
				pSuplInitData->notification.clientName[8],
				pSuplInitData->notification.clientName[9]);
	}

	encType = pSuplInitData->notification.encodingType;
	SIRF_LOGD("Encoding Type = %d", (int)encType);
	switch(encType)
	{
		case UCS2_ENCODING:
			{
				niNotification.requestor_id_encoding = GPS_ENC_SUPL_UCS2;
				niNotification.text_encoding = GPS_ENC_SUPL_UCS2;
				break;
			}

		case GSM_DEFAULT_ENCODING:
			{
				niNotification.requestor_id_encoding = GPS_ENC_SUPL_GSM_DEFAULT;
				niNotification.text_encoding = GPS_ENC_SUPL_GSM_DEFAULT;
				break;
			}

		case UTF8_ENCODING:
			{
				niNotification.requestor_id_encoding = GPS_ENC_SUPL_UTF8;
				niNotification.text_encoding = GPS_ENC_SUPL_UTF8;
				break;
			}

		default:
			{
				niNotification.requestor_id_encoding = GPS_ENC_UNKNOWN;
				niNotification.text_encoding = GPS_ENC_UNKNOWN;
				SIRF_LOGE("ERROR : Unknown Encoding!!!!");
				break;
			}
	}

	//Do nothing on niNotification.extras!!!!!!

	android_status_report(ANDROID_STATUS_REPORT_NI_NOTIFICATION);

	SIRF_LOGD("blocked. wait for NI Response...");
	SIRF_PAL_OS_SEMAPHORE_Wait(semNiResponse, SIRF_TIMEOUT_INFINITE); //Android has 15 sec timeout for this one
	SIRF_LOGD("got NI Response");

	switch(GPS_NI_RESPONSE)
	{
		case GPS_NI_RESPONSE_ACCEPT:
			{
				SIRF_LOGI("GPS_NI_RESPONSE_ACCEPT");
				pUserResponse->UserResponse = LSM_AGPS_ACCEPT;
				break;
			}

		case GPS_NI_RESPONSE_DENY:
			{
				SIRF_LOGI("GPS_NI_RESPONSE_DENY");
				pUserResponse->UserResponse = LSM_AGPS_REJECT;
				break;
			}

		case GPS_NI_RESPONSE_NORESP:
			{
				SIRF_LOGI("GPS_NI_RESPONSE_NORESP");
				pUserResponse->UserResponse = LSM_AGPS_IGNORE;
				break;
			}

		default:
			{
				SIRF_LOGE("ERROR : Unknown NI Response!!!!");
				pUserResponse->UserResponse = LSM_AGPS_IGNORE;
				break;
			}
	}
}

#endif


// This function calls Android NI API to pop up a window upon receiving C-Plane MT-LR Location Indication
//LSM_BOOL CP_MtlrNotifyLocInd (P_CPA_LCS_LOCATION_IND pCpaLcsLocationInd)
LSM_BOOL CP_MtlrNotifyLocInd (P_CPA_LCS_LOCATION_IND pCpaLcsLocationInd)
{
#if 1
	GpsNiNotification *pNiNotification = &CP_mtlrNotification;
	LSM_BOOL result = LSM_TRUE;
	int i, len;
	char tmpBuf[10];

	SIRF_LOGD("%s: called", __FUNCTION__);

	memset(pNiNotification, 0, sizeof(GpsNiNotification));

	pNiNotification->size = sizeof(GpsNiNotification);

	if (g_cpaLcsInvokeHandle != (int) NULL)
	{
		SIRF_LOGW("%s: Warning! Notification already in progress. Handle will be overwritten. g_cpaLcsInvokeHandle=%d", __FUNCTION__, g_cpaLcsInvokeHandle);
	}

	// Save the invoke handle for the verification callback.
	// Assumption: We handle only one Notification at a time. If there are more, save the last handle and process it.
	g_cpaLcsInvokeHandle = pCpaLcsLocationInd->invokeHandle;

	pNiNotification->notification_id = (int) pCpaLcsLocationInd->invokeHandle;
	pNiNotification->ni_type = GPS_NI_TYPE_UMTS_CTRL_PLANE;
	pNiNotification->notify_flags = GPS_NI_NEED_NOTIFY | GPS_NI_NEED_VERIFY; //| GPS_NI_PRIVACY_OVERRIDE ;
	pNiNotification->timeout = 15000; // in mSec 

	// default response, for the case of Timeout
	pNiNotification->default_response = GPS_NI_RESPONSE_NORESP;

	SIRF_LOGD("%s: notif_id=0x%x, ni_type=%d, notify_flags=%d, timeout=%d, default_response=%d",
			__FUNCTION__,
			pNiNotification->notification_id,
			pNiNotification->ni_type,
			pNiNotification->notify_flags,
			pNiNotification->timeout,
			pNiNotification->default_response);

	// requestor_id + encoding
	if ((pCpaLcsLocationInd->optRequestorId.present) &&
			(pCpaLcsLocationInd->optRequestorId.requestorIdString.length > 0))
	{
		if (pCpaLcsLocationInd->optRequestorId.requestorIdString.length > sizeof(pNiNotification->requestor_id) - 1)
		{
			len = sizeof(pNiNotification->requestor_id) - 1;
		}

		else
		{
			len = pCpaLcsLocationInd->optRequestorId.requestorIdString.length;
		}

		for (i = 0; i < len; i++)
		{
			sprintf(tmpBuf, "%02X", pCpaLcsLocationInd->optRequestorId.requestorIdString.name[i]);
			strncat((char *) pNiNotification->requestor_id, tmpBuf, sizeof(pNiNotification->requestor_id));
		}

		pNiNotification->requestor_id_encoding = pCpaLcsLocationInd->optRequestorId.dataCodingScheme;

		SIRF_LOGD("%s: (optRequestorId) requestor_id=%s, encoding=%d", __FUNCTION__, pCpaLcsLocationInd->optRequestorId.requestorIdString.name, pNiNotification->requestor_id_encoding);
	}
	else
	{
		if ((pCpaLcsLocationInd->optClientExternalId.present) &&
				(pCpaLcsLocationInd->optClientExternalId.optExternalAddress.present) &&
				(pCpaLcsLocationInd->optClientExternalId.optExternalAddress.addressLength > 0))
		{
			if (pCpaLcsLocationInd->optClientExternalId.optExternalAddress.addressLength > sizeof(pNiNotification->requestor_id) - 1)
			{
				len = sizeof(pNiNotification->requestor_id) - 1;
			}
			else
			{
				len = pCpaLcsLocationInd->optClientExternalId.optExternalAddress.addressLength;
			}

			for (i = 0; i < len; i++)
			{
				sprintf(tmpBuf, "%02X", pCpaLcsLocationInd->optClientExternalId.optExternalAddress.externalAddress[i]);
				strncat((char *) pNiNotification->requestor_id, tmpBuf, sizeof(pNiNotification->requestor_id));
			}

			pNiNotification->requestor_id_encoding = GPS_ENC_SUPL_UTF8;

			SIRF_LOGD("%s: (optExternalAddress) requestor_id=%s, encoding=%d", __FUNCTION__, pCpaLcsLocationInd->optClientExternalId.optExternalAddress.externalAddress, pNiNotification->requestor_id_encoding);
		}
	}

	// text (for client ...) + encoding
	if ((pCpaLcsLocationInd->optClientName.present) &&
			(pCpaLcsLocationInd->optClientName.clientNameString.length > 0))
	{
		if (pCpaLcsLocationInd->optClientName.clientNameString.length > sizeof(pNiNotification->text) - 1)
		{
			len = sizeof(pNiNotification->text) - 1;
		}

		else
		{
			len = pCpaLcsLocationInd->optClientName.clientNameString.length;
		}

		for (i = 0; i < len; i++)
		{
			sprintf(tmpBuf, "%02X", pCpaLcsLocationInd->optClientName.clientNameString.name[i]);
			strncat((char *) pNiNotification->text, tmpBuf, sizeof(pNiNotification->text));
		}

		if (pCpaLcsLocationInd->optClientName.dataCodingScheme == 0x0F)
		{
			//GSM7BITS format for 0x0F
			//pNiNotification->text_encoding = GPS_ENC_SUPL_GSM_DEFAULT;
			//GSM7BITS is translated into UTF-8 in telephony. Android has bug.
			pNiNotification->text_encoding = GPS_ENC_SUPL_UTF8;
		}
		else
		{
			pNiNotification->text_encoding = GPS_ENC_SUPL_UTF8; //pCpaLcsLocationInd->optClientName.dataCodingScheme;
		}

		SIRF_LOGD("%s: forClientText=%s, encoding=%d", __FUNCTION__, pCpaLcsLocationInd->optClientName.clientNameString.name, pNiNotification->text_encoding);
	}

	// extras - TBD

	android_status_report(ANDROID_STATUS_REPORT_CP_NI_NOTIFICATION);

	return result;
#endif
}


static LSM_BOOL getLSMCommonConfiguration(LSM_commonCfgData *pCommonCfgData)
{
	//when sirf_gps_init is called, these common configurations should be set by location manager!!!!

	/* GPS Port Settings parameter */
	pCommonCfgData->lsmInitData.gpsDevice.configType = LSM_ETRACKER;

	memcpy(&pCommonCfgData->lsmInitData.gpsDevice.DeviceConfig.trackerCfg.gsd4tConfig,
			&g_GSD4tConfig.trackerConfig,
			sizeof(pCommonCfgData->lsmInitData.gpsDevice.DeviceConfig.trackerCfg.gsd4tConfig));
	strlcpy(pCommonCfgData->lsmInitData.gpsDevice.DeviceConfig.trackerCfg.m_SerialPort,
			(char *)g_GSD4tConfig.trackerConfig.tracker_port,
			sizeof(pCommonCfgData->lsmInitData.gpsDevice.DeviceConfig.trackerCfg.m_SerialPort));

	pCommonCfgData->lsmInitData.gpsDevice.DeviceConfig.trackerCfg.m_BaudRate = 115200;
	pCommonCfgData->lsmInitData.gpsDevice.DeviceConfig.trackerCfg.gpsStartMode = SIRFNAV_UI_CTRL_MODE_AUTO;

	/* use LPL default heap */
	pCommonCfgData->lsmInitData.lpl_mem.memSize = LPL_HEAP;
	pCommonCfgData->lsmInitData.lpl_mem.pMem = NULL;

	pCommonCfgData->lsmInitData.lplPowerControl.cgpsCtrl = LSM_DISABLE_CGPSC;
	pCommonCfgData->lsmInitData.lplPowerControl.gpsPwrCtrlCap = LSM_NO_GPS_POWER_CONTROL;

	/*set ICD */
	pCommonCfgData->CommonCfgBitMap.isSLCICDSet = 1;
	pCommonCfgData->ICDFromUser.AI3ICD = AI3_REV_NUM_22;
	pCommonCfgData->ICDFromUser.FICD   = F_REV_NUM_21;

	/* Network Setting parameter */
	pCommonCfgData->CommonCfgBitMap.isCommonNTWKParamSet = 1;
	pCommonCfgData->commonNetworkCfg.bSecure = g_GSD4tConfig.ssl_enabled;

	/*Reaiding Setting parameter*/
	pCommonCfgData->CommonCfgBitMap.isReAidingParamSet = 1;
	pCommonCfgData->reAidingParam.reAidingTimeIntervalLimit = 15; /* 15 minutes interval */
	pCommonCfgData->reAidingParam.SVLimit = 12; /* Keep it max(12) to make re-aiding independent on satellite count. */

#if 1 //sirf_agps_set_server called before sirf_gps_init!!!!
	strcpy(pCommonCfgData->commonNetworkCfg.SuplServerInfo.ipAddr,
			g_GSD4tConfig.serverAddress);
	pCommonCfgData->commonNetworkCfg.SuplServerInfo.port = g_GSD4tConfig.serverPort;
#endif

	pCommonCfgData->commonNetworkCfg.BindAddress.bBind = 0;

	/* Reset Type */
	pCommonCfgData->CommonCfgBitMap.isResetTypeSet = 1;
	pCommonCfgData->gpsResetType = LSM_RESET_TYPE_HOT_RESET; //(eLSM_RESET_TYPE) g_GSD4tConfig.resetType;

	/* SET Capabilities */
	pCommonCfgData->CommonCfgBitMap.isCapSet = 1;
	pCommonCfgData->PosMethodCapabilities.cap.msBased = (LSM_UINT8)g_GSD4tConfig.isPosTechMSB;
	pCommonCfgData->PosMethodCapabilities.cap.msAssisted = (LSM_UINT8)g_GSD4tConfig.isPosTechMSA;
	pCommonCfgData->PosMethodCapabilities.cap.autonomous = (LSM_UINT8)g_GSD4tConfig.isPosTechAuto;
	pCommonCfgData->PosMethodCapabilities.cap.eCID = (LSM_UINT8)g_GSD4tConfig.isPosTechECID;
	pCommonCfgData->PosMethodCapabilities.prefLocMethod = (eLSM_PREFERRED_LOC_METHOD)g_GSD4tConfig.prefLocMethod;

	/* Aiding Type */
	pCommonCfgData->CommonCfgBitMap.isAidingTypeParamSet = 1;
	/* The Local aiding will be set only if [STANDALONE + GPSPlus] configuration provided. */
	if((LSM_NO_AIDING == g_GSD4tConfig.aidingType)
			&& ((1 == g_GSD4tConfig.isCapCGEE) ||((1 == g_GSD4tConfig.isCapSGEE))))
	{
		g_GSD4tConfig.aidingType = LSM_LOCAL_AIDING;
	}

	pCommonCfgData->aidingType = g_GSD4tConfig.aidingType;

	/* Logging Information */
	pCommonCfgData->CommonCfgBitMap.isLogInfoSet = 1;

	if(g_GSD4tConfig.debugging)
	{
		pCommonCfgData->logInfo.loggingtype = 2; //Fix Type Single : 1, Multi : 2 //(eLSM_LOGGING_TYPE)pAgpsCfg->loggingType;
		//pCommonCfgData->logInfo.logfiles.briefLogFileName    = g_GSD4tConfig.briefLogFileName;
		pCommonCfgData->logInfo.logfiles.briefLogFileName    = NULL; //Switch with line above to enable original BriefLog - remember to change gpioDeInitialization as well
		pCommonCfgData->logInfo.logfiles.agpsLogFileName     = g_GSD4tConfig.agpsLogFileName;
		pCommonCfgData->logInfo.logfiles.detailedLogFileName = g_GSD4tConfig.detailedLogFileName;
		pCommonCfgData->logInfo.logfiles.slcLogFileName      = g_GSD4tConfig.slcLogFileName;
	}
	else
	{
		pCommonCfgData->logInfo.loggingtype = 0;
		pCommonCfgData->logInfo.logfiles.briefLogFileName    = NULL;
		pCommonCfgData->logInfo.logfiles.agpsLogFileName     = NULL;
		pCommonCfgData->logInfo.logfiles.detailedLogFileName = NULL;
		pCommonCfgData->logInfo.logfiles.slcLogFileName      = NULL;
	}

	/* Set default permission file for SSL */
	ca_path[0] = '\0';
	strcpy(ca_path, "/system/etc/AGPS_CA.pem");

#ifdef LPL_CLM
	/* CLM */
	pCommonCfgData->CommonCfgBitMap.isCLMCfgDataSet = 1;
	pCommonCfgData->clmCfgData.aidingCap.LPL_CGEE_AIDING     = g_GSD4tConfig.isCapCGEE;//0; //pAgpsCfg->isCapCGEE;
	pCommonCfgData->clmCfgData.aidingCap.LPL_SGEE_AIDING     = g_GSD4tConfig.isCapSGEE;//0; //pAgpsCfg->isCapSGEE;
	pCommonCfgData->clmCfgData.aidingCap.LPL_NAV_BITS_AIDING = g_GSD4tConfig.isCapNavBitAiding;//0; //pAgpsCfg->isCapNavBitAiding;

#endif /* LPL_CLM */

	/* Posiiton Callback. This is a mandatory parameter */
	pCommonCfgData->lplStatusCallback = LSM_Position_CallBack;

#if ENABLE_NMEA_LISTENER
	pCommonCfgData->CommonCfgBitMap.isNMEAenabled = 1;
	pCommonCfgData->nmeaCallback = SiRF_NMEA_Listener;
#endif

#if ENABLE_DEBUG_LISTENER
	pCommonCfgData->CommonCfgBitMap.isDbgListenerSet = 1;
	pCommonCfgData->dbgCallback = LSM_Debug_Listener;
#endif

#if ENABLE_SSB_LISTENER
	pCommonCfgData->CommonCfgBitMap.isSSBenabled = 1;
	pCommonCfgData->ssbCallback = LSM_SSB_Listener;
#endif

#if ENABLE_SERIAL_LISTENER
	pCommonCfgData->CommonCfgBitMap.isSerialListenerSet = 1;
	pCommonCfgData->CommonCfgBitMap.isSTATenable = 0; /*SSBStatInfo*/
	pCommonCfgData->serialCallback = LSM_Serial_Listener;
#endif

#if ENABLE_TESTMODE_LISTENER
	pCommonCfgData->CommonCfgBitMap.isTestModeListenerSet = 1;
	pCommonCfgData->testModeCallback = LSM_TestMode_Listener;
#endif

#if ENABLE_SUPL_INIT_LISTENER
	pCommonCfgData->SuplInitCallbackInfo.suplInitCallback = LSM_SUPL_Init_Listener;
	pCommonCfgData->SuplInitCallbackInfo.pUserData = NULL;
	pCommonCfgData->CommonCfgBitMap.isSuplInitCallbackSet = 1;
#endif

	pCommonCfgData->CommonCfgBitMap.isFreqAidingSet = g_GSD4tConfig.frequency_aiding;
	pCommonCfgData->FreqParam.freqXferMethod = LSM_FREQUENCY_TRANSFER_METHOD_COUNTER; /* COUNTER PULL */
	pCommonCfgData->FreqParam.frequencyNominalValue = 2.6e7; /* 26 MHz */

	return 1;
}

static LSM_BOOL getLSMSessionCfgData(LSM_SIsessionCfgData *pSessionCfgData)
{
	/* Approximate Location */
	pSessionCfgData->SessionCfgBitMap.isApproxLocInfoSet = g_GSD4tConfig.isApproximateLocationPresent;
	pSessionCfgData->approxLoc.lat = g_GSD4tConfig.approximateLatitude;
	pSessionCfgData->approxLoc.lon = g_GSD4tConfig.approximateLongitude;
	pSessionCfgData->approxLoc.alt = g_GSD4tConfig.approximateAltitude;
	pSessionCfgData->approxLoc.horErr = DEFAULT_HOR_ERROR;
	pSessionCfgData->approxLoc.verErr = DEFAULT_VER_ERROR;

	/* Network Parameter settings */
	pSessionCfgData->SessionCfgBitMap.isSessionNTWKParamSet = 1;
	pSessionCfgData->sessionNetworkCfg.BindAddress.bBind = 0; /* FALSE */
	pSessionCfgData->sessionNetworkCfg.bSecure = g_GSD4tConfig.isSecure;
	strcpy(pSessionCfgData->sessionNetworkCfg.SuplServerInfo.ipAddr, g_GSD4tConfig.serverAddress);
	pSessionCfgData->sessionNetworkCfg.SuplServerInfo.port =g_GSD4tConfig.serverPort;

	/* Reset Type */
	pSessionCfgData->SessionCfgBitMap.isResetTypeSet = 1;
	pSessionCfgData->gpsResetType = (eLSM_RESET_TYPE)g_GSD4tConfig.resetType;

	/* Aiding Type */
	pSessionCfgData->SessionCfgBitMap.isAidingTypeParamSet = 1;
	/* Set Local Aiding only if we have Standalone + GPS Plus in configuration settings */
	if((LSM_NO_AIDING == g_GSD4tConfig.aidingType)
			&& ((1 == g_GSD4tConfig.isCapCGEE) ||((1 == g_GSD4tConfig.isCapSGEE))))
	{
		g_GSD4tConfig.aidingType = LSM_LOCAL_AIDING;
	}
	pSessionCfgData->aidingType = (eLSM_AIDING_TYPE)g_GSD4tConfig.aidingType;

	/* QoS */
	pSessionCfgData->QoS.horizontalAccuracy = g_GSD4tConfig.QoSHorizontalAccuracy;
	pSessionCfgData->QoS.verticalAccuracy = g_GSD4tConfig.QoSVerticalAccuracy;
	pSessionCfgData->QoS.maxResponseTime = g_GSD4tConfig.QoSMaxResponseTime;
	pSessionCfgData->isQoSInSUPLStart = g_GSD4tConfig.QoSinSUPLSTART;

	SIRF_LOGD("QoP Configuration : Horizontal Accuracy : %d, Vertical Accuracy : %d, TTFF Response Time : %d", \
			g_GSD4tConfig.QoSHorizontalAccuracy, g_GSD4tConfig.QoSVerticalAccuracy, \
			g_GSD4tConfig.QoSMaxResponseTime );
	pSessionCfgData->numFixes = g_GSD4tConfig.numberOfFixes;
	pSessionCfgData->timeInterval = g_GSD4tConfig.timeBetweenFixes;

	/* network status */
	pSessionCfgData->isNWConnectionAvailable = g_GSD4tConfig.networkStatus;

	/* preference location method */
	pSessionCfgData->SessionCfgBitMap.isPrefLocationMethodSet = g_GSD4tConfig.isPrefLocationMethodSet;
	pSessionCfgData->prefLocationMethod = g_GSD4tConfig.prefLocMethod;

	/* Test mode Configuration */
	pSessionCfgData->isTestModeEnable = g_GSD4tConfig.isTestModeEnable;
	pSessionCfgData->testModeType = g_GSD4tConfig.testModeType;
	pSessionCfgData->testModeSvId = g_GSD4tConfig.testModeSvId;
	pSessionCfgData->testModePeriod = g_GSD4tConfig.testModePeriod;

	return (1); /* TRUE */
}

static tSIRF_RESULT gpioInitialization(tSIRF_VOID)
{
	tSIRF_RESULT result;

	result = SIRF_PAL_HW_OpenRESET(SIRF_PAL_HW_RESET_HIGH);
	if(SIRF_SUCCESS == result)
	{
		result = SIRF_PAL_HW_OpenON_OFF(SIRF_PAL_HW_ON_LOW);
		if(SIRF_SUCCESS != result)
		{
			SIRF_LOGE("Failed to open On_Off port in Main \n");
			return result;
		}
	}
	else
	{
		SIRF_LOGE("Failed to open Reset port in Main \n");
		return result;
	}

	return result;
}

static tSIRF_RESULT gpioDeInitialization(tSIRF_VOID)
{
	tSIRF_RESULT result;
	tSIRF_RESULT ret_val = SIRF_SUCCESS;

	result = SIRF_PAL_HW_CloseRESET();
	if(SIRF_SUCCESS == ret_val)
	{
		ret_val = result;
	}

	result = SIRF_PAL_HW_CloseON_OFF();
	if(SIRF_SUCCESS == ret_val)
	{
		ret_val = result;
	}

	return ret_val;
}

static void GSD4t_Config_init(void)
{
	static int initialized_check = 0;

	SIRF_LOGD("%s: called", __FUNCTION__);
	if(initialized_check)
	{
		SIRF_LOGD("GSD4t Config Already Initialized");
		return;
	}
	else
	{
		initialized_check = 1;
	}
	sirf_gps_load_default_config(); //load default values first. initialize the values.
	sirf_gps_load_config(); //load CSR SiRF GPS Config for hardware configurations

	//when sirf_gps_init is called, these common configurations should be set by location manager!!!!
	g_GSD4tConfig.isSecure = LSM_FALSE;
	strcpy(g_GSD4tConfig.serverAddress, "supl.google.com");

	g_GSD4tConfig.serverPort = 7276; 
	g_GSD4tConfig.resetType = LSM_RESET_TYPE_HOT_RESET; //Default Hot Reset!!!

	//SET Capability : MSB, MSA, Standalone!!!!
	if (gCapabilities == SET_CAPABILITIES_ALL || gCapabilities == SET_CAPABILITIES_MSB)
	{
		g_GSD4tConfig.isPosTechMSB = LSM_TRUE;
	}
	if (gCapabilities == SET_CAPABILITIES_ALL || gCapabilities == SET_CAPABILITIES_MSA)
	{
		g_GSD4tConfig.isPosTechMSA = LSM_TRUE;
	}
	g_GSD4tConfig.isPosTechAuto = LSM_TRUE;
	g_GSD4tConfig.isPosTechECID = LSM_FALSE;

	//Default Postion Mode : MSB!!!!
	g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
	g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSB;
	g_GSD4tConfig.aidingType = LSM_NETWORK_AIDING;
	g_GSD4tConfig.networkStatus = LSM_TRUE;

	/* Log File Name !!!! */
	//sprintf(g_GSD4tConfig.briefLogFileName, "%s%s", g_GSD4tConfig.log_path, BRIEF_LOGFILE);
	g_GSD4tConfig.briefLogFileName[0] = '\0'; //Switch with above line to use original BriefLog writer, remember to change name of mpos in gps_logging.h AND enable the name in getLSMCommonConfiguration
	sprintf(g_GSD4tConfig.agpsLogFileName, "%s%s", g_GSD4tConfig.log_path, AGPS_LOGFILE);
	sprintf(g_GSD4tConfig.detailedLogFileName, "%s%s", g_GSD4tConfig.log_path, DETAILED_LOGFILE);
	sprintf(g_GSD4tConfig.slcLogFileName, "%s%s", g_GSD4tConfig.log_path, SLC_LOGFILE);
	sprintf(g_GSD4tConfig.sirfInterfaceLogFileName, "%s%s", g_GSD4tConfig.log_path, SIRF_INTERFACE_LOG);
	sprintf(g_GSD4tConfig.nmeaLogFileName, "%s%s", g_GSD4tConfig.log_path, NMEA_FILE);

	//By default, Angry GPS shows it enabled so ...!!!
	g_GSD4tConfig.isCapCGEE = 0;

	g_GSD4tConfig.isCapSGEE = 0;

	g_GSD4tConfig.isCapNavBitAiding = 0;

	//Coarse Location Injection Disabled!!!!
	g_GSD4tConfig.approximateLatitude = 37.37;
	g_GSD4tConfig.approximateLongitude = -121.91;
	g_GSD4tConfig.approximateAltitude = 0;
	g_GSD4tConfig.isApproximateLocationPresent = 0;

	//QoS Default Setting!!!
	g_GSD4tConfig.QoSinSUPLSTART = 1;
	g_GSD4tConfig.maxLocAge = 0;
	g_GSD4tConfig.QoSMaxResponseTime = 240;  //Alex change to 240 second
	g_GSD4tConfig.QoSVerticalAccuracy = 250; // change to 250 m
	g_GSD4tConfig.QoSHorizontalAccuracy = 100;

	/* Cell Info and SETID Info is Invalid by Default!!! */
	g_GSD4tConfig.isCellInfoValid = LSM_FALSE;
	g_GSD4tConfig.isSETIDInfoValid = LSM_FALSE;

	/* Time Between Fixes : 1 */
	g_GSD4tConfig.timeBetweenFixes = 1;
	g_GSD4tConfig.numberOfFixes = 0;

#ifdef LPL_CLM
	/* SGEE Configuration */
	/* TODO: We need to provide a dedicated SGEE Server info.
	   Following info is our internal testing only, must not go to customer.
	 */
	//char sgee_server_addr[2][80] = { "192.168.54.214", "192.168.54.214" };

	char sgee_server_addr[2][80] = { "eedemo1.sirf.com", "eedemo1.sirf.com" };
	tSIRF_UINT16 sgee_server_port = 80;
	char sgee_server_file[] = "/diff/packedDifference.f2p7enc.ee";
	char sgee_server_auth[] = "c2lyZjpTaXJmU2dlZTIwMDU=";
	memcpy(g_GSD4tConfig.SGEEserverAddress[0],  sgee_server_addr[0], sizeof(sgee_server_addr[0]));
	memcpy(g_GSD4tConfig.SGEEserverAddress[1],  sgee_server_addr[1], sizeof(sgee_server_addr[1]));
	g_GSD4tConfig.SGEEserverPort = sgee_server_port;
	memcpy(g_GSD4tConfig.SGEEserverFile, sgee_server_file, sizeof(sgee_server_file));
	memcpy(g_GSD4tConfig.SGEEserverAuth, sgee_server_auth, sizeof(sgee_server_auth));
#endif

	/* Test Mode Configuration */
	g_GSD4tConfig.isTestModeEnable = LSM_FALSE; //disable here if testmodes are needed
	g_GSD4tConfig.testModeType = LSM_TESTMODE_4;
	g_GSD4tConfig.testModeSvId = 0x01;
	g_GSD4tConfig.testModePeriod = 30;
	g_GSD4tConfig.isCNOTest = 1;

	g_GSD4tConfig.defaultResponseTimeoutofNI = SUPL_NI_NOTIFICATION_DEFAULT_RESPONSE_TIMEOUT;

	/* Initialize RESET and ON_OFF lines */
	g_userPalConfig.on_off_line_usage = UI_CTRL_MODE_HW_ON_GPIO;
	g_userPalConfig.ext_sreset_n_line_usage = UI_CTRL_MODE_HW_RESET_GPIO;

	SIRF_LOGI("GSD4t Config Initialized Successfully");

	return;
}

#ifdef LPL_CLM


tEE_DOWNLOAD_STATUS EE_Download_CallBackFn(tEE_DOWNLOAD_CALLBACK_TYPE callbackType, tSIRF_UINT8 *pSGEEFileData, tSIRF_UINT32 fileLengthSGEE, tEE_DOWNLOAD_STATUS retStatus)
{
	if(EE_DOWNLOAD_DATA_CALLBACK == callbackType)
	{
		if((NULL == pSGEEFileData) || (0 == fileLengthSGEE))
		{
			SIRF_LOGE("EED: ERROR!! Null or Zero length data.");
			return EE_DOWNLOAD_GENERIC_FAILURE;
		}

		if(LSM_TRUE != CP_SendNetMessage( LSM_EE_PUSH, LSM_RRC_STATE_IDLE, NO_EMERGENCY,
					(LSM_UINT8*)pSGEEFileData, (LSM_UINT32)fileLengthSGEE))
		{
			SIRF_LOGE("EED: ERROR!! EE Data not accepted.");
			return EE_DOWNLOAD_GENERIC_FAILURE;
		}
		else
		{
			SIRF_LOGD("EED: SUCCESS!! EE Data accepted.");
			return EE_DOWNLOAD_SUCCESS;
		}
	}
	else if(EE_DOWNLOAD_STATUS_CALLBACK == callbackType)
	{
		switch( retStatus )
		{
			case EE_DOWNLOAD_SUCCESS: /* Download over successfully. */
				{
					SIRF_LOGI("EED: Download over successfully.");
					if(LSM_TRUE != CP_SendNetMessage( LSM_EE_PUSH, LSM_RRC_STATE_IDLE, NO_EMERGENCY,
								(LSM_UINT8*)NULL, (LSM_UINT32)0x00))
					{
						SIRF_LOGE("EED: Download stop indication rejected.");
					}

					break;
				}

			case EE_DOWNLOAD_GENERIC_FAILURE:
				{

					SIRF_LOGW("EED: EE Download start failed as already running.");
					if(LSM_TRUE != CP_SendNetMessage( LSM_EE_PUSH, LSM_RRC_STATE_IDLE, NO_EMERGENCY,
								(LSM_UINT8*)NULL, (LSM_UINT32)0x00))
					{
						SIRF_LOGW("EED: Download stop indication rejected.");
					}

					break;
				}

			case EE_DOWNLOAD_STOPPED:
				{
					/* EE Download Stopped successfully.*/
					break;
				}

			case EE_DOWNLOAD_ALREADY_STARTED:
				{
					/* EE Download start failed as already running. */
					SIRF_LOGW("EED: EE Download start failed as already running.");

					break;
				}

			case EE_DOWNLOAD_NOT_RUNNNING:
				{
					/* EE Download Stop failed as not started yet. */
					SIRF_LOGW("EED: EE Download Stop failed as not started yet.");

					break;
				}

			case EE_DOWNLOAD_NET_ERROR:
				{
					/* Failed! Network Error */
					SIRF_LOGE("EED: EE Download Network Error");
					if(LSM_TRUE != CP_SendNetMessage( LSM_EE_PUSH, LSM_RRC_STATE_IDLE, NO_EMERGENCY,
								(LSM_UINT8*)NULL, (LSM_UINT32)0x00))
					{
						SIRF_LOGE("EED: Download stop indication rejected.");
					}

					break;
				}

			case EE_DOWNLOAD_THREAD_EXITING:
				{
					SIRF_LOGD("EED: EE Download Thread Exit");
					/* EE Download Exiting the EE downloader thread. */

					break;
				}

			default:
				{
					/* Failed */
					break;
				}
		}
	}
	return EE_DOWNLOAD_SUCCESS;
}

int sirf_sgee_download_start(void)
{
	LSM_UINT32  predTime = 0, ageTime = 0;
	LSM_SINT32  remainingTime = 0;
	tSIRF_CHAR eeDwnldVersion[128];

	if((LSM_TRUE != g_GSD4tConfig.isCapSGEE))
	{
		SIRF_LOGD("APP: SGEE File download, No SGEE capability. ");
		return LSM_FALSE;
	}
	if(LSM_TRUE == LSM_GetSGEEFileAge(&ageTime, &predTime))
	{
		remainingTime  = (predTime - ageTime);
		if( remainingTime <= (4 * 3600) )  /* require a new file if less than 4 hours remained. */
		{
			SIRF_LOGI("APP: SIF : Need for SGEE File download.: Age = %d", remainingTime);
			/* Push EE data.*/
			EE_Download_GetLibVersion(eeDwnldVersion);
			SIRF_LOGD("APP: CLM : EE_Download version %s\n", eeDwnldVersion);
			if ( SIRF_SUCCESS != EE_Download_Init(g_GSD4tConfig.SGEEserverAddress, g_GSD4tConfig.SGEEserverPort,
						g_GSD4tConfig.SGEEserverFile, g_GSD4tConfig.SGEEserverAuth, EE_DOWNLOAD_OTA_FORMAT_FF2))
			{
				SIRF_LOGE("APP: CLM : EE_Download_Init failed \n.");
				return LSM_FALSE;
			}
			else
			{
				SIRF_LOGI("APP: CLM : EE_Download_Init success.\n");

				if ( SIRF_SUCCESS != EE_Download_Start(EE_Download_CallBackFn))
				{
					SIRF_LOGE("APP: CLM : EE_Download_Start failed .\n");
					return LSM_FALSE;
				}
				else
				{
					SIRF_LOGD("APP: CLM : EE_Download_Start success .\n");
				}
			}
		}
		else
		{
			SIRF_LOGI("APP: SIF : No need for SGEE File download.");
		}
	}

	return LSM_TRUE;
}


int sirf_sgee_download_stop(void)
{
	LSM_BOOL retVal = SIRF_FAILURE;

	if ( SIRF_SUCCESS == EE_Download_Stop())
	{
		SIRF_LOGD("EE_Download_Stop Success.\n");
		retVal = SIRF_SUCCESS;
	}

	return retVal;
}

#endif

tSIRF_RESULT GSD4t_reset(void)
{
	SIRF_LOGD("GSD4t_reset");

	if (SIRF_SUCCESS != SIRF_PAL_HW_WriteRESET(SIRF_PAL_HW_RESET_LOW))
	{
		return SIRF_FAILURE;
	}

	SIRF_PAL_OS_THREAD_Sleep(5);

	if (SIRF_SUCCESS != SIRF_PAL_HW_WriteRESET(SIRF_PAL_HW_RESET_HIGH))
	{
		return SIRF_FAILURE;
	}

	SIRF_PAL_OS_THREAD_Sleep(5);

	return SIRF_SUCCESS;
}


static void sirf_gps_set_gprs_interface(void)
{
	extern void MRIL_get_gprs_interface(char *gprs_buf);

	SIRF_LOGD("%s: called", __FUNCTION__);
	g_gprs_interface[0] = '\0';

	MRIL_get_gprs_interface(g_gprs_interface);

	set_gprs_interface(g_gprs_interface);
}

static LSM_BOOL GSD4t_Init(void)
{
	tSIRF_RESULT result;
	int try_no = 0;

	SIRF_LOGD("%s: called", __FUNCTION__);

	Heap_Open();

	result = SIRF_PAL_Init();
	if ( SIRF_SUCCESS !=    result)
	{
		SIRF_LOGE("ERROR: SIRF_PAL_Init Failed %u", (unsigned int)result);
		Heap_Close();
		return SIRF_FAILURE;
	}

	if (0 == getLSMCommonConfiguration(&LSM_CommonCfg))
	{
		SIRF_LOGE("APP: ERROR: Init cannot set LSM Common configuration data");
		return (SIRF_FAILURE);
	}

	SIRF_LOGI("APP: INFO: LSM version is %s", LSM_GetLSMVersion());

	while(try_no < 3)
	{

		if (LSM_FALSE == LSM_Init(&LSM_CommonCfg))
		{
			SIRF_LOGE("LSM_Init Failed!!!!! Retry Again!!!");
			result = gpioInitialization();
			if ( result != SIRF_SUCCESS )
			{
				SIRF_LOGE( "APP: gpioInitialization() error: 0x%X\n", (unsigned int)result );
				return result;
			}

			SIRF_PAL_OS_THREAD_Sleep(200); //200ms Delay
			/* reset the tracker!!! */
			GSD4t_reset();
		}
		else
		{
			SIRF_LOGI("LSM_Init Success!!!");
			break;
		}
		try_no++;        
	}

	if(try_no >= 3)
	{
		SIRF_LOGE("LSM Init Failed after 3 retrials!!!!");
		return SIRF_FAILURE;
	}

	result = SIRF_PAL_OS_SEMAPHORE_Create(&semNiResponse, 0); //default value : 0
	if(SIRF_SUCCESS != result)
	{
		SIRF_LOGE("Semaphore (semNiResponse) Creation Failed!");
		return result;
	}

	g_SessionID = INVAILD_SESSION_HANDLE;
	g_NI_Session = NO_NI_SESSION;

	GPS_POSITION_MODE = GPS_POSITION_MODE_MS_BASED;
	g_Initialization_Status = GSD4t_INITIALIZED;

	return SIRF_SUCCESS;
}

static LSM_BOOL GSD4t_Deinit(void)
{
	tSIRF_RESULT result;

	if(g_Initialization_Status == GSD4t_UNINITIALIZED)
	{
		SIRF_LOGE("ERROR: GSD4t UnInitialized!!!");
		return SIRF_FAILURE;
	}

	g_SessionID = INVAILD_SESSION_HANDLE;
	g_NI_Session = NO_NI_SESSION;

	SIRF_PAL_OS_SEMAPHORE_Delete( semNiResponse);
	semNiResponse = NULL;

	if (LSM_FALSE == LSM_Deinit())
	{
		SIRF_LOGE("APP: ERROR: Deinit failed calling LSM_Deinit");
	}

	result = gpioDeInitialization();
	if ( result != SIRF_SUCCESS )
	{
		SIRF_LOGE("gpioDeInitialization() error: 0x%X\n", (unsigned int)result );
		return result;
	}

	/* Destroy the PAL */
	result = SIRF_PAL_Destroy();
	if (SIRF_SUCCESS !=  result)
	{
		SIRF_LOGE("ERROR: SIRF_PAL_Destroy Failed %u", (unsigned int)result);
	}

	Heap_Close(); //Heap_Open() called by GSD4t_Init function!!

	//sirf_interface_log_deinit();
#if ENABLE_NMEA_FILE
	sirf_nmea_log_deinit();
#endif

	g_Initialization_Status = GSD4t_UNINITIALIZED;

	return SIRF_SUCCESS;
}

#define HW_INITIALIZATION_FAIL       0
#define HW_INITIALIZATION_SUCCESS 1
#define HW_UNINITIALIZED       2
static int GSD4t_HW_Init(void)
{
	static int hw_initialized = HW_UNINITIALIZED;
	tSIRF_RESULT result;

	if(hw_initialized != HW_UNINITIALIZED)
	{
		SIRF_LOGW("GSD4t HW Init, do nothing!!! - status = %d", hw_initialized);
		return SIRF_FAILURE; //already Initialized!!!
	}

	/* Initialize GPIO and Reset the GSD4t!!!! */
	result = gpioInitialization();
	if ( result != SIRF_SUCCESS )
	{
		SIRF_LOGE( "APP: gpioInitialization() error: 0x%X\n", (unsigned int)result );
		return result;
	}

	SIRF_PAL_OS_THREAD_Sleep(200); //200ms Delay

	/* reset the tracker!!! */
	GSD4t_reset();

	if(SIRF_SUCCESS != GSD4t_Init())
	{
		SIRF_LOGE("GSD4t_Init Fail!!!");
		hw_initialized = HW_INITIALIZATION_FAIL;
		GSD4t_Deinit();
		return -1;
	}

	SIRF_LOGI("GSD4t HW Initialized successfully");
	hw_initialized = HW_INITIALIZATION_SUCCESS;

	return SIRF_SUCCESS;
}

#define GPS_UPLD_WAKE_LOCK_NAME "gps_tracker_upload"

static void* tracker_upload_thread(void *param)
{
	tSIRF_RESULT result;
	int ret;
	int flag_wakelock_acquired = 0;
	FILE *fLock;

	SIRF_LOGD("%s: called", __FUNCTION__);

	// Acquire a system wakelock. Before the "boot complete" delay.
	fLock = fopen("/sys/power/wake_lock", "wb");
	if (!fLock)
		SIRF_LOGE("Error! cannot open /sys/power/wake_lock");
	else
	{
		fwrite(GPS_UPLD_WAKE_LOCK_NAME, strlen(GPS_UPLD_WAKE_LOCK_NAME), 1, fLock);
		fclose(fLock);
		flag_wakelock_acquired = 1;
	}

	SIRF_PAL_OS_THREAD_Sleep(3000); // delay, wait for boot complete

	controlPowerWakeLock(GPS_WAKE_LOCK_ENABLE);
	ret = GSD4t_HW_Init();

	controlPowerWakeLock(GPS_WAKE_LOCK_DISABLE);

	// Release the system wakelock
	if (flag_wakelock_acquired)
	{
		flag_wakelock_acquired = 0;
		fLock = fopen("/sys/power/wake_unlock", "wb");
		if (!fLock)
			SIRF_LOGE("Error! cannot open /sys/power/wake_unlock");
		else
		{
			fwrite(GPS_UPLD_WAKE_LOCK_NAME, strlen(GPS_UPLD_WAKE_LOCK_NAME), 1, fLock);
			fclose(fLock);
		}
	}

	pthread_mutex_lock(&sTrackerUploadMutex);
	if (ret == SIRF_SUCCESS)
	{
		sTrackerUploadDone = 1;
	}
	else
	{
		sTrackerUploadDone = -1;
	}
	pthread_cond_signal(&sTrackerUploadCond);
	pthread_mutex_unlock(&sTrackerUploadMutex);

	SIRF_LOGD("%s: exit. ret: %d", __FUNCTION__, ret);
	return NULL;
}

static pthread_t tracker_upload_thread_id;

static void start_delayed_tracker_upload()
{
	pthread_attr_t attr;
	static int started = 0;

	if (started)
	{
		SIRF_LOGW("%s: already started", __FUNCTION__);
		return;
	}

	pthread_attr_init(&attr);    
	pthread_create(&tracker_upload_thread_id, &attr, tracker_upload_thread, NULL);

	SIRF_LOGI("%s: exit", __FUNCTION__);
	started = 1;
	return;
}

void printGSD4tHWconfig(void)
{
	SIRF_LOGD("****** GSD4t Hardware Configuration ******");
	SIRF_LOGD("*** [PROJECT : %s]", g_GSD4tConfig.project_name);
	SIRF_LOGD("*** [UART DRIVER : %s]", g_GSD4tConfig.trackerConfig.tracker_port);
	SIRF_LOGD("*** [RESET GPIO : %s]", g_userPalConfig.reset_port);
	SIRF_LOGD("*** [ONOFF GPIO : %s]", g_userPalConfig.on_off_port);
	SIRF_LOGD("*** [TCXO CLOCK : %s]", ( g_GSD4tConfig.trackerConfig.ref_clk_frequency == 26000000 ? "26MHz" : "16.369MHz" ));
	SIRF_LOGD("*** [EXTERNAL LNA : %s]", ( g_GSD4tConfig.trackerConfig.lna_type == SIRFNAV_UI_CTRL_MODE_EXT_LNA_ON ? "ENABLED" : "DISABLED" ));
	SIRF_LOGD("*** [UART BAUD RATE : %d]", g_GSD4tConfig.trackerConfig.uart_baud_rate);
	SIRF_LOGD("*** [FREQUENCY AIDING : %s]", ( g_GSD4tConfig.frequency_aiding == 1 ? "ENABLED" : "DISABLED" ));
	SIRF_LOGD("*** [SENSOR AIDING : %s]", ( g_GSD4tConfig.sensor_aiding == 1 ? "ENABLED" : "DISABLED" ));
	SIRF_LOGD("*** [SETID : %s]", ( g_GSD4tConfig.set_id_imsi == 1 ? "IMSI" : "MSISDN" ));
	SIRF_LOGD("*** [SSL : %s]", ( g_GSD4tConfig.ssl_enabled == 1 ? "ENABLED" : "DISABLED" ));
	SIRF_LOGD("*** [DEBUGGING FILES : %s]", ( g_GSD4tConfig.debugging == 1 ? "ENABLED" : "DISABLED" ));
	SIRF_LOGD("*** [REAIDING TIME : %d (m)]",g_GSD4tConfig.reAidingTimeIntervalLimit);
	SIRF_LOGD("*** [CONTROL PLANE : %s]",( g_GSD4tConfig.isControlPlaneEnabled == 1 ? "ENABLED" : "DISABLED" ));
	SIRF_LOGD("*** [LOG PATH : %s]", g_GSD4tConfig.log_path);

	if(g_GSD4tConfig.isATTNetworkOperator)
		SIRF_LOGD("*** [ATT NETWORK OPERATOR]");

	return;
}

static int sirf_gps_init(GpsCallbacks* callbacks)
{
	tSIRF_RESULT result;
	static int gps_init_check = 0;
	int ret;

	SIRF_LOGD("%s: called", __FUNCTION__);

#ifdef CONTROL_PLANE_SUPPORT
	UPDATE_LSMEnv
#endif


		set_gprs_interface("ccinet0");


	if( (callbacks != NULL) && (callbacks->size == sizeof(GpsCallbacks)) )
	{
		gpsCallbacks  = *callbacks;
	}
	else
	{
		SIRF_LOGE("Callback Function Error!!!!!!!!");
		return -1;
	}

	if(gpsCallbacks.set_capabilities_cb) //for the safety, call this function here!!!!
	{
		SIRF_LOGD("Configure GPS Capabilites - Scheduling, MSB, MSA");
		gpsCallbacks.set_capabilities_cb(GPS_CAPABILITY_SCHEDULING | GPS_CAPABILITY_MSB  | GPS_CAPABILITY_MSA);
	}

	gpsStatus.size = sizeof(GpsStatus);
	locationInfo.size = sizeof(GpsLocation);
	niNotification.size = sizeof(GpsNiNotification);
	agpsStatus.size = sizeof(AGpsStatus);

	if(gps_init_check == 1)
	{
		SIRF_LOGW("Android GPS is Already initialized!!!, Do nothing!!!");
		return 0;
	}

	/* wait for tracker uploading */
	pthread_mutex_lock(&sTrackerUploadMutex);
	while (sTrackerUploadDone == 0) 
	{
		pthread_cond_wait(&sTrackerUploadCond, &sTrackerUploadMutex);
	}
	pthread_mutex_unlock(&sTrackerUploadMutex);
	SIRF_LOGD("Tracker Upload is done");

	if (sTrackerUploadDone == -1)
	{
		SIRF_LOGE("sirf_gps_init failed reason:GSD4t_Init Fail!!!");
		return SIRF_FAILURE; 
	}

	if(g_GSD4tConfig.debugging == 1)
	{
		sirf_interface_log_init(g_GSD4tConfig.log_path);
	}

	SIRF_LOGD("%s", GSD4t_SHAREDLIB_VERSION);
	printGSD4tHWconfig(); //print the hardware configuration for GSD4t
	if(gpsCallbacks.create_thread_cb)
	{
		gpsCallbacks.create_thread_cb("sirf_session_handler", android_session_handler, NULL);
		while( ANDROID_SESSION_NOT_READY == android_session_check ) {
			SIRF_PAL_OS_THREAD_Sleep(20);
		}
		SIRF_LOGD("Create android_session_handler thread");

		gpsCallbacks.create_thread_cb("sirf_status_report_handler", android_status_report_handler, NULL);
		while(0 == android_status_report_ready)
		{
			SIRF_PAL_OS_THREAD_Sleep(20);    
		}
		SIRF_LOGD("Create android_status_report_handler thread");
	}

	if(g_GSD4tConfig.isControlPlaneEnabled == 1) /* for MP3 Player */
	{
		//DO NOTHING
		;
	}

#ifdef CONTROL_PLANE_SUPPORT
	ret = CpaClient_Init();

	if (ret < 0)
	{
		SIRF_LOGE("ERROR: CpaClient_Init failed! error=%d", ret);
		return SIRF_FAILURE;
	}
	else
	{
		SIRF_LOGD("CpaClient_Init OK");
	}
#endif


	SIRF_LOGI("SIRF GSD4t Initialization Success");
	gps_init_check = 1;

	return 0;
}

static void sirf_gps_cleanup(void)
{
	tSIRF_RESULT result;

	SIRF_LOGD("%s: called", __FUNCTION__);

#if 0 //Harold, shouldn't exit the thread!!!, during the ON/OFF Test, Android can't endure this. It can be a burden for the system!!
	android_session_handler_close();
	android_status_report_handler_close();
	if(SIRF_SUCCESS != GSD4t_Deinit())
	{
		SIRF_LOGE("ERROR GSD4t_Deinit() Fail!");
	}
#endif

#if 0 /* sometimes LM sends stop and cleanup at the same time it can cause reset problem*/
	memset(&gpsCallbacks,0,sizeof(gpsCallbacks));
	memset(&agpsCallbacks,0,sizeof(agpsCallbacks));
	memset(&gpsNiCallbacks,0,sizeof(gpsNiCallbacks));
	memset(&gpsRilCallbacks,0,sizeof(gpsRilCallbacks));
	memset(&agpsMiscCallbacks,0,sizeof(agpsMiscCallbacks));

	sirf_interface_log_deinit();  // Bye Bye GPS
#endif

	return;
}

static int sirf_gps_start(void)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	/* Load Config file for Test Mode */
	pthread_mutex_lock(&sAndroidSessionMutex);
	if(SIRF_SUCCESS != android_session_handler_resume(ANDROID_MO_SESSION) )
	{
		SIRF_LOGE("sirf_gps_start Fail!!!!");
		pthread_mutex_unlock(&sAndroidSessionMutex);
		return -1;
	}
	pthread_mutex_unlock(&sAndroidSessionMutex);

	return SIRF_SUCCESS;
}

static int sirf_gps_stop()
{
	tSIRF_RESULT result;

	SIRF_LOGD("%s: called", __FUNCTION__);
	if(g_GSD4tConfig.isTestModeEnable && g_GSD4tConfig.testModeType == LSM_TESTMODE_7)
	{
		SIRF_LOGI("CW Test Mode Enabled, it automatically closes the session before sirf_gps_stop called");

		pthread_mutex_lock(&sAndroidSessionMutex);
		if(SIRF_SUCCESS != android_session_handler_suspend())
		{
			SIRF_LOGE("android_session_handler_suspend fail!!!");
		}
		pthread_mutex_unlock(&sAndroidSessionMutex);


		return 0;
	}

	if(android_session_check == ANDROID_CP_SESSION)
	{
		SIRF_LOGD("gps_stop success as CP S running");
		return 0;
	}
	pthread_mutex_lock(&sAndroidSessionMutex);
	stopupdatepending = 1;
	if(SIRF_SUCCESS != android_session_handler_suspend())
	{
		pthread_mutex_unlock(&sAndroidSessionMutex);
		SIRF_LOGE("android_session_handler_suspend fail!!!");
		return 0;
	}
	pthread_mutex_unlock(&sAndroidSessionMutex);

	SIRF_LOGD("gps_stop wait");

	result = SIRF_PAL_OS_SEMAPHORE_Wait(semwaitstop, 4000); 
	if( result != 0)
	{
		stopupdatepending = 0; // avoid sem leak, go to normal idle
		SIRF_LOGW("gps_stop timeout");
	}
	else
	{
		SIRF_LOGD("gps_stop success");
	}

	return 0;
}

static int sirf_gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
	SIRF_LOGD("%s: called", __FUNCTION__);

	return 0;
}

static int  sirf_gps_inject_location(double latitude, double longitude, float accuracy)
{
	SIRF_LOGD("%s: called", __FUNCTION__);
	return 0;
}

static void sirf_gps_delete_aiding_data(GpsAidingData flags)
{
	SIRF_LOGD("%s: called, flags: %X", __FUNCTION__, flags);

	/* These flags er still unused - if requred add them and remove them from this list
	   GPS_DELETE_TIME             
	   GPS_DELETE_IONO          
	   GPS_DELETE_HEALTH        
	   GPS_DELETE_SVDIR         
	   GPS_DELETE_SVSTEER       
	   GPS_DELETE_SADATA        
	   GPS_DELETE_RTI           
	   GPS_DELETE_CELLDB_INFO   
	 */

	if ( flags | GPS_DELETE_ALL || 
			flags | GPS_DELETE_ALMANAC ||
			flags | GPS_DELETE_POSITION ||
			flags | GPS_DELETE_UTC)
	{
		CP_Reset();
		g_GSD4tConfig.resetType = LSM_RESET_TYPE_COLD_RESET;
		SIRF_LOGI("Cold Start!!!!");
	}
	else if ( flags | GPS_DELETE_EPHEMERIS)
	{
		g_GSD4tConfig.resetType = LSM_RESET_TYPE_WARM_RESET;
		SIRF_LOGI("Warm Start!!!!");
	}
	else 
	{
		g_GSD4tConfig.resetType = LSM_RESET_TYPE_HOT_RESET;
		SIRF_LOGI("Hot Start!!!!");
	}

	g_del_aiding_request = LSM_TRUE;
}

/**
 * min_interval represents the time between fixes in milliseconds.
 * preferred_accuracy represents the requested fix accuracy in meters.
 * preferred_time represents the requested time to first fix in milliseconds.
 */
static int sirf_gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence, \
		uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
{
	SIRF_LOGD("%s: called", __FUNCTION__);
	SIRF_LOGD("min_interval = %u, preferred_accuacy=%u, preferred_time =%u", min_interval, preferred_accuracy, preferred_time);

	/* Position Mode Set!!! */
	switch(mode)
	{
		case GPS_POSITION_MODE_MS_BASED:
			{
				g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
				g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSB;
				g_GSD4tConfig.aidingType = LSM_NETWORK_AIDING;
				g_GSD4tConfig.networkStatus = 1;
				SIRF_LOGD("GPS_POSITION_MODE_MS_BASED");
				break;
			}

		case GPS_POSITION_MODE_MS_ASSISTED:
			{
				g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
				g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSA;
				g_GSD4tConfig.aidingType = LSM_NETWORK_AIDING;
				g_GSD4tConfig.networkStatus = 1;
				SIRF_LOGD("GPS_POSITION_MODE_MS_ASSISTED");
				break;
			}

		default:
			{
				g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
				g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSB;
				g_GSD4tConfig.aidingType = LSM_NO_AIDING;
				g_GSD4tConfig.networkStatus = 0;
				SIRF_LOGD("GPS_POSITION_MODE_STANDALONE with Local Aiding");
				break;
			}
	}

	GPS_POSITION_MODE = mode;

	if(recurrence == GPS_POSITION_RECURRENCE_PERIODIC)
	{
		g_GSD4tConfig.numberOfFixes = 0;
		SIRF_LOGD("GPS_POSITION_RECURRENCE_PERIODIC");
	}
	else
	{
		g_GSD4tConfig.numberOfFixes = 1;
		SIRF_LOGD("GPS_POSITION_RECURRENCE_SINGLE");
	}

	g_GSD4tConfig.timeBetweenFixes = (tSIRF_UINT8)(min_interval / 1000);
	SIRF_LOGD("TBF : %d milliseconds", (int)min_interval);

	g_GSD4tConfig.QoSHorizontalAccuracy = preferred_accuracy;
	SIRF_LOGD("preferred_accuracy : %d meters", (int)preferred_accuracy);

	g_GSD4tConfig.QoSMaxResponseTime =(tSIRF_UINT8)(preferred_time / 1000);
	SIRF_LOGD("the requested time to first fix : %d milliseconds", preferred_time);

	return 0;
}

static const void* sirf_gps_get_extension(const char* name)
{
	SIRF_LOGD("%s called : %s", __FUNCTION__, name);

	if(0 == strcmp(name, AGPS_INTERFACE))
	{
		return &sirfAGpsInterface;
	}

	if(0 == strcmp(name, GPS_NI_INTERFACE))
	{
		return &sirfGpsNiInterface;
	}

	if (0 == strcmp(name, GPS_DEBUG_INTERFACE))
	{
		return &sirfGpsDebugInterface;
	}

	if (0 == strcmp(name, AGPS_RIL_INTERFACE))
	{
		return &sirfAGpsRilInterface;
	}
#if 0 //Sensor interface not used 
	if (0 == strcmp(name, GPS_MISC_INTERFACE))
	{
		return &sirfAGpsMiscInterface;
	}
#endif
	SIRF_LOGW("%s: interface %s not supported", __FUNCTION__ , name);

	return NULL;
}

void sirf_gps_update_gps_parameters(void)
{
	SIRF_LOGD("%s: called", __FUNCTION__);
	return;
}

static const GpsInterface  sirfGpsInterface =
{
	sizeof(GpsInterface),
	sirf_gps_init,
	sirf_gps_start,
	sirf_gps_stop,
	sirf_gps_cleanup,
	sirf_gps_inject_time,
	sirf_gps_inject_location,
	sirf_gps_delete_aiding_data,
	sirf_gps_set_position_mode,
	sirf_gps_get_extension,
};

void initialize_gps(void)
{
	SIRF_LOGI("%s: called", __FUNCTION__);

	if(g_GSD4tConfig.debugging == 1)    
	{
		sirf_interface_log_init(g_GSD4tConfig.log_path);
	}

	GSD4t_Config_init();

	///TODO: Enable GPS chip power
	SIRF_LOGI("LSM Opened and GPS driver on");
	start_delayed_tracker_upload();
}

tSIRF_INT32 sirf_gps_test_mode(LSM_SINT32 cmd, LSM_SINT32 svId, LSM_CHAR *data )
{
	static int isGPSon = 0;

	SIRF_LOGI("%s: called: cmd: %d, svid: %d", __FUNCTION__, cmd, svId);

	if (isGPSon == 0)
	{
		system("echo on > /proc/driver/sirf");
		SIRF_LOGD("LSM Opened and GPS driver on");
		usleep(100*1000); // delay 100ms
		isGPSon = 1;	
	}

	switch (cmd)
	{
		case GPS_SIRF_CMD_TEST_MODE_4:
			{
				SIRF_LOGD("GPS_SIRF_CMD_TEST_MODE_4");

				/* Reset the chip */
				SIRF_PAL_HW_OpenRESET(SIRF_PAL_HW_RESET_LOW);
				usleep(100*1000); // delay 100ms
				SIRF_PAL_HW_OpenRESET(SIRF_PAL_HW_RESET_HIGH);

				GSD4t_Config_init();
				GPS_POSITION_MODE = GPS_POSITION_MODE_STANDALONE;
				g_GSD4tConfig.isTestModeEnable = LSM_TRUE;
				g_GSD4tConfig.resetType = LSM_RESET_TYPE_COLD_RESET;
				GSD4t_HW_Init();

				g_GSD4tConfig.testModeType = LSM_TESTMODE_4;
				g_GSD4tConfig.testModeSvId = svId;
				g_GSD4tConfig.testModePeriod = 10;  //change for other period
				g_GSD4tConfig.isCNOTest = 0;        //change for CNO test

				android_session_check = ANDROID_SESSION_NONE;
				isGPSon = LSM_TRUE;
				android_mo_session_start();
				break;
			}
		case GPS_SIRF_CMD_TEST_MODE_7:
			{
				SIRF_LOGD("GPS_SIRF_CMD_TEST_MODE_7");


				/* Reset the chip */
				SIRF_PAL_HW_OpenRESET(SIRF_PAL_HW_RESET_LOW);
				usleep(100*1000); // delay 100ms
				SIRF_PAL_HW_OpenRESET(SIRF_PAL_HW_RESET_HIGH);

				GSD4t_Config_init();
				GPS_POSITION_MODE = GPS_POSITION_MODE_STANDALONE;
				g_GSD4tConfig.isTestModeEnable = LSM_TRUE;
				g_GSD4tConfig.resetType = LSM_RESET_TYPE_COLD_RESET;
				GSD4t_HW_Init();

				testModeType = cmd;
				g_GSD4tConfig.resetType = LSM_RESET_TYPE_TEST_MODE;
				g_GSD4tConfig.testModeType = LSM_TESTMODE_7;
				g_GSD4tConfig.trackerConfig.start_mode = SIRFNAV_UI_CTRL_MODE_TEST;
				g_GSD4tConfig.testModeSvId = svId;
				g_GSD4tConfig.testModePeriod = 1;
				g_GSD4tConfig.isCNOTest = 0;
				sCWTestCnt = 0;
				CWTestModeResultPeakValue.cno = 0;
				CWTestModeResultPeakValue.freq = 0;
				memset(CW_MEASURED, 0, CW_SAMPLES);

				android_session_check = ANDROID_SESSION_NONE;
				isGPSon = LSM_TRUE;
				android_mo_session_start();
				break;
			}
		case GPS_SIRF_CMD_TEST_MODE_READ:
			{
				if (NULL == data)
				{
					SIRF_LOGE("%s: null pointer in data", __FUNCTION__);
					return SIRF_FAILURE;
				}
				SIRF_LOGD("final TM out>> %d, %10d, %10d", final_tm_data.is_valid, final_tm_data.cno, final_tm_data.freq);
				switch (testModeType)
				{
					case GPS_SIRF_CMD_TEST_MODE_4:
						memcpy(data, &tm4_struct, sizeof(tSIRF_MSG_SSB_TEST_MODE_DATA));
						break;
					case GPS_SIRF_CMD_TEST_MODE_7:
						memcpy(data, &final_tm_data, sizeof(t_test_mode_data));
						break;
					default:
						SIRF_LOGE("%s: illegal testModeType: %d", __FUNCTION__, (int) testModeType);
						return SIRF_FAILURE;
						break;
				}
				break;
			}
		case GPS_SIRF_CMD_TEST_MODE_OFF:
			{
				android_mo_session_stop();
				isGPSon = LSM_FALSE;
				break;
			}
		default:
			SIRF_LOGE("%s: illegal cmd", __FUNCTION__);
			return SIRF_FAILURE;
			break;
	}
	return SIRF_SUCCESS;
}



/***************************************
 * Configuration files read from file system       *
 ****************************************/

//Common functions

int set_property(char *name, char *value)
{
	int i;

	for (i = 0; i < MAX_PROPERTY_NUM; i++)
	{
		char *n, *v;

		if (property_list[i].name && strcmp(property_list[i].name, name))
			continue;

		if (property_list[i].name == NULL)
		{
			n = malloc(strlen(name) + 1);
			if (!n)
			{
				SIRF_LOGE("malloc failed. %d\n", errno);
				return -1;
			}
			memset(n, 0x00, strlen(name) + 1);

			v = malloc(strlen(value) + 1);
			if (!v)
			{
				SIRF_LOGE("malloc failed. %d\n", errno);
				free(n);
				return -1;
			}
			memset(v, 0x00, strlen(value) + 1);

			strcpy(n, name);
		}
		else
		{
			n = property_list[i].name;
			v = property_list[i].value;

			if (strlen(v) < strlen(value))
			{
				v = malloc(strlen(value) + 1);
				if (!v)
				{
					printf("malloc failed. %d\n", errno);
					return -1;
				}
				memset(v, 0x00, strlen(value) + 1);

				free(property_list[i].value);
			}
		}

		strcpy(v, value);

		property_list[i].name = n;
		property_list[i].value = v;

		return 0;
	}

	return -1;
}

char *get_property(char *name)
{
	int i;

	for (i = 0; i < MAX_PROPERTY_NUM; i++)
	{
		if (!property_list[i].name)
		{
			continue;
		}

		if (!strcmp(property_list[i].name, name))
		{
			return property_list[i].value;
		}
	}

	return NULL;
}

static void parser(char *line)
{
	char *buf = line;
	char *name, *value;

	if (line[0] == '#') /* comment */
		return;

	name = strsep(&buf, "=");
	value = strsep(&buf, "=");

	//printf("%s: name %s value %s\n", __FUNCTION__, name, value);

	set_property(name, value);

	return ;
}

static char *my_rindex(char *buf, char ch)
{
	int len = strlen(buf);
	int i;

	if (len == 0)
	{
		return NULL;
	}

	for (i = len-1; i >= 0; i--)
	{
		if (buf[i] == ch)
		{
			return &buf[i];
		}
	}

	return NULL;
}

int load_properties(const char *path)
{
	char line[256];
	FILE *fp;

	fp = fopen(path, "r");
	if (!fp)
	{
		SIRF_LOGE("%s: failed to open %s (%d)\n", __FUNCTION__, path, errno);
		return -1;
	}

	while (fgets(line, sizeof(line), fp))
	{
		char *nl;
		/* remove CR, LF */
		while ((nl = my_rindex(line, '\n')) || (nl = my_rindex(line, '\r')) )
		{
			*nl = '\0';
		}
		parser(line);
	}

	fclose(fp);

	return 0;
}

void print_properties()
{
	int i;

	for (i = 0; i < MAX_PROPERTY_NUM; i++)
	{
		if (!property_list[i].name)
		{
			continue;
		}

		SIRF_LOGD("%s: %s\n", property_list[i].name, property_list[i].value);
	}
}

int free_properties()
{
	int i;

	for (i = 0; i < MAX_PROPERTY_NUM; i++)
	{
		if (!property_list[i].name)
		{
			continue;
		}

		free(property_list[i].name);
		free(property_list[i].value);

		property_list[i].name = NULL;
		property_list[i].value = NULL;
	}

	return 0;
}


//CSR GPS HW CONFIG FILE



static void sirf_gps_project_name_load(void)
{
	char *project_name = get_property("PROJECT");
	char *cap_str      = get_property("CAPABILITY");

	if (project_name)
	{
		strlcpy(g_GSD4tConfig.project_name, project_name, sizeof(g_GSD4tConfig.project_name));
		SIRF_LOGD("***HW [PROJECT : %s]", g_GSD4tConfig.project_name);
	}
	else
	{
		SIRF_LOGD("***HW [CSR SiRF plc, GPS Solution]");
	}

	/* MSA, MSB, ALL capabilities */
	if (cap_str)
	{
		if (strcmp(cap_str, "MSA") == 0)
		{
			gCapabilities = SET_CAPABILITIES_MSA;
		}
		else if (strcmp(cap_str, "MSB") == 0)
		{
			gCapabilities = SET_CAPABILITIES_MSB;
		}
		else if (strcmp(cap_str, "MSAB") == 0)
		{
			gCapabilities = SET_CAPABILITIES_ALL;
		}
	}
	return;
}

static void sirf_gps_driver_path_load(void)
{
	char *uart_driver = get_property("UART_DRIVER");
	char *reset_gpio = get_property("RESET_GPIO");
	char *onoff_gpio = get_property("ONOFF_GPIO");
	int port = 7276;

	if (!uart_driver)
	{
		uart_driver = "/dev/ttyS1";
		SIRF_LOGE("UART_DRIVER Path is NULL!!!");
	}

	if(!reset_gpio)
	{
		reset_gpio = "/dev/gps_sirf";
		SIRF_LOGE("RESET_GPIO Path is NULL!!!");
	}

	if(!onoff_gpio)
	{
		onoff_gpio = "/dev/gps_sirf";
		SIRF_LOGE("ONOFF_GPIO Path is NULL!!!");
	}

	SIRF_LOGD("***HW [UART DRIVER : %s]", uart_driver);
	SIRF_LOGD("***HW [RESET GPIO : %s]", reset_gpio);
	SIRF_LOGD("***HW [ONOFF GPIO : %s]", onoff_gpio);

	strlcpy((char *)g_userPalConfig.on_off_port, onoff_gpio, sizeof(g_userPalConfig.on_off_port));
	strlcpy((char *)g_userPalConfig.reset_port, reset_gpio, sizeof(g_userPalConfig.reset_port));
	strlcpy((char *)g_GSD4tConfig.trackerConfig.tracker_port, uart_driver, sizeof(g_GSD4tConfig.trackerConfig.tracker_port));

	return;
}

static void sirf_gps_lna_config_load(void)
{
	char *external_lna_str = get_property("EXTERNAL_LNA");
	int external_lna = 0;

	if (external_lna_str)
	{
		if (external_lna_str[0] == '1')
		{
			external_lna = 1;
			g_GSD4tConfig.trackerConfig.lna_type = SIRFNAV_UI_CTRL_MODE_EXT_LNA_ON;
		}
		else
		{
			external_lna = 0;
			g_GSD4tConfig.trackerConfig.lna_type = SIRFNAV_UI_CTRL_MODE_EXT_LNA_OFF;
		}
	}

	SIRF_LOGD("***HW [EXTERNAL LNA : %s]", ( external_lna == 1 ? "ENABLED" : "DISABLED" ));

	return;
}

static void sirf_gps_tcxo_config_load(void)
{
	char *ref_clock = get_property("REF_CLOCK_26MHZ");
	int tcxo_26MHz = 0;

	if (ref_clock)
	{
		if (ref_clock[0] == '1')
		{
			tcxo_26MHz = 1;
			g_GSD4tConfig.trackerConfig.ref_clk_frequency = 26000000;
		}
		else
		{
			tcxo_26MHz = 0;
			g_GSD4tConfig.trackerConfig.ref_clk_frequency = 16369000;
		}
	}

	SIRF_LOGD("***HW [TCXO CLOCK : %s]", ( tcxo_26MHz == 1 ? "26MHz" : "16.369MHz" ));

	return;
}

static void sirf_gps_uart_baud_rate_config(void)
{
	char *baud_rate = get_property("UART_BAUD_RATE");
	int sel_baud_rate = 0;

	if (baud_rate)
	{
		sel_baud_rate = atoi(baud_rate);
	}

	switch(sel_baud_rate)
	{
		case 0:
			SIRF_LOGD("***HW [UART BAUD RATE : 115200]");
			g_GSD4tConfig.trackerConfig.uart_baud_rate = 115200;
			break;

		case 1:
			SIRF_LOGD("***HW [UART BAUD RATE : 230400]");
			g_GSD4tConfig.trackerConfig.uart_baud_rate = 230400;
			break;

		case 2:
			SIRF_LOGD("***HW [UART BAUD RATE : 460800]");
			g_GSD4tConfig.trackerConfig.uart_baud_rate = 460800;
			break;

		case 3:
			SIRF_LOGD("***HW [UART BAUD RATE : 57600]");
			g_GSD4tConfig.trackerConfig.uart_baud_rate = 57600;
			break;

		default:
			SIRF_LOGD("***HW [DEFAULT UART BAUD RATE : 115200]");
			g_GSD4tConfig.trackerConfig.uart_baud_rate = 115200;
			break;
	}

	/* set code load baud rate to be the same as baud rate */
	g_GSD4tConfig.trackerConfig.code_load_baud_rate = g_GSD4tConfig.trackerConfig.uart_baud_rate;
	g_GSD4tConfig.trackerConfig.uart_baud_rate = 115200; // best baud on u1

	return;
}

static void sirf_gps_frequency_aiding_config(void)
{
	char *frequency_aiding = get_property("FREQUENCY_AIDING");
	int frequency_aiding_enabled = 0;

	if (frequency_aiding)
	{
		if (frequency_aiding[0] == '1')
		{
			frequency_aiding_enabled = 1;
			g_GSD4tConfig.frequency_aiding = 1;
		}
		else
		{
			frequency_aiding_enabled = 0;
			g_GSD4tConfig.frequency_aiding = 0;
		}
	}

	SIRF_LOGD("***HW [FREQUENCY AIDING : %s]", ( frequency_aiding_enabled == 1 ? "ENABLED" : "DISABLED" ));

	return;
}

static void sirf_gps_sensor_aiding_config(void)
{
	char *sensor_aiding = get_property("SENSOR_AIDING");
	int sensor_aiding_enabled = 0;

	if (sensor_aiding)
	{
		if (sensor_aiding[0] == '1')
		{
			sensor_aiding_enabled = 1;
			g_GSD4tConfig.sensor_aiding = 1;
		}
		else
		{
			sensor_aiding_enabled = 0;
			g_GSD4tConfig.sensor_aiding = 0;
		}
	}

	SIRF_LOGD("***HW [SENSOR AIDING : %s]", ( sensor_aiding_enabled == 1 ? "ENABLED" : "DISABLED" ));

	return;
}

static void sirf_gps_setid_imsi_config(void)
{
	char *set_id_imsi = get_property("SET_ID_IMSI");

	if (set_id_imsi)
	{
		if (set_id_imsi[0] == '1')
		{
			g_GSD4tConfig.set_id_imsi = 1;
		}
		else
		{
			g_GSD4tConfig.set_id_imsi = 0;
		}
	}

	SIRF_LOGD("***HW [SETID : %s]", ( g_GSD4tConfig.set_id_imsi == 1 ? "IMSI" : "MSISDN" ));

	return;
}

static void sirf_gps_debugging_files_config_load(void)
{
	char *debugging_files = get_property("DEBUGGING_FILES");
	int debugging_files_enabled = 0;

	if (debugging_files)
	{
		if (debugging_files[0] == '1')
		{
			debugging_files_enabled = 1;
			g_GSD4tConfig.debugging = 1;
		}
		else
		{
			debugging_files_enabled = 0;
			g_GSD4tConfig.debugging = 0;
		}
	}

	SIRF_LOGD("***HW [DEBUGGING FILES : %s]", ( debugging_files_enabled == 1 ? "ENABLED" : "DISABLED" ));

	return;
}

//DEBUGGING_FILLES_PATH

static void sirf_gps_debugging_files_path_load(void)
{

	int path_exist = 0;
	char *path = get_property("LOG_PATH");

	if (path == NULL)
	{
		strcpy(g_GSD4tConfig.log_path, "/data");
	}
	else
	{
		path_exist = access(path, F_OK);
		if(path_exist == 0)
		{
			//SIRF_LOGD("%s exists", path);
			strcpy(g_GSD4tConfig.log_path, path);

		}
		else
		{
			SIRF_LOGD("%s doesn't exists", path);
			strcpy(g_GSD4tConfig.log_path, "/data");
		}
	}

	SIRF_LOGD("***HW [LOG_PATH : %s]", g_GSD4tConfig.log_path);

	return;
}


static void sirf_gps_reaiding_config_load(void)
{
	char *reaiding = get_property("REAIDING");    

	if(reaiding)
	{
		g_GSD4tConfig.reAidingTimeIntervalLimit = atoi(reaiding);

		if(g_GSD4tConfig.reAidingTimeIntervalLimit > 0)
		{
			g_GSD4tConfig.reaidingSVLimit = 12;
			g_GSD4tConfig.isReAidingParamSet = 1;
		}
		else
			g_GSD4tConfig.isReAidingParamSet = 0;

	}

	SIRF_LOGD("***HW [ReAiding time : %d (m)]",g_GSD4tConfig.reAidingTimeIntervalLimit);

	return;
}

static void sirf_gps_ssl_version_load(void)
{
	char *ssl_version_files = get_property("CERTI_VERSION");
	int ssl_version = 0;

	if(ssl_version_files)
	{
		ssl_version = atoi(ssl_version_files);
	}

	switch(ssl_version)
	{
		case 0:
			PAL_ssl_version = 0;
			break;

		case 1:
			PAL_ssl_version = 1;
			break;

		case 2:
			PAL_ssl_version = 2;
			break;

		default:
			PAL_ssl_version = 2;

			break;
	}

	SIRF_LOGD("***HW [CERTI_VERSION : %d]", ssl_version);

	return;

}

static void sirf_gps_cp_response_time_load(void)
{
	char *cp_reposne_time_option = get_property("CP_RESPONSETIME");
	int cp_response = 0; 

	if(cp_reposne_time_option)
	{
		cp_response = atoi(cp_reposne_time_option);
	}

	switch(cp_response)
	{
		case 0:
			CP_response_time = 0;
			break;

		case 1:
			CP_response_time = 1;
			break;

		case 2:
			CP_response_time = 2;
			break;

		case 3:
			CP_response_time = 3;
			break;

		default:
			CP_response_time = 2;
			break;
	}
	SIRF_LOGD("***HW [CP_RESPONSETIME : %d]", cp_response);

	return;

}


static void sirf_gps_ssl_enable_config_load(void)
{
	char *ssl_enabled_files = get_property("SSL_ENABLED");

	if (ssl_enabled_files)
	{
		if (ssl_enabled_files[0] == '1')
		{
			g_GSD4tConfig.ssl_enabled = 1;
		}
		else
		{
			g_GSD4tConfig.ssl_enabled = 0;
		}
	}

	SIRF_LOGD("***HW [SSL ENABLED : %s]", ( g_GSD4tConfig.ssl_enabled == 1 ? "TRUE" : "FALSE" ));

	return;
}

static void sirf_gps_cp_enable_config_load(void)
{
	char *cp_enable = get_property("CONTROL_PLANE");

	if (cp_enable)
	{
		if (cp_enable[0] == '1')
		{
			g_GSD4tConfig.isControlPlaneEnabled = 1;
		}
		else
		{
			g_GSD4tConfig.isControlPlaneEnabled = 0;
		}
	}

	SIRF_LOGD("***HW [CONTROL_PLANE ENABLED : %s]", ( g_GSD4tConfig.isControlPlaneEnabled == 1 ? "TRUE" : "FALSE" ));

	return;
}

static void sirf_gps_ATT_Network_Operator_check(void)
{
	char *att_network = get_property("ATT_NETWORK_OPERATOR");

	if (att_network)
	{
		if (att_network[0] == '1')
		{
			g_GSD4tConfig.isATTNetworkOperator = 1;
			ATTNetworkOperatorEnabled = 1;
		}
		else
		{
			g_GSD4tConfig.isATTNetworkOperator = 0;
			ATTNetworkOperatorEnabled = 0;
		}
	}
	else
	{
		return;
	}

	SIRF_LOGD("***HW [ATT_NETWORK_OPERATOR ENABLED : %s]", ( g_GSD4tConfig.isATTNetworkOperator == 1 ? "TRUE" : "FALSE" ));

	return;
}

static void sirf_gps_EMC_Enable_check(void)
{
	char *emc_enble = get_property("EMC_ENABLE");

	if (emc_enble)
	{
		if (emc_enble[0] == '1')
		{
			g_GSD4tConfig.isEMC_ENABLE= 1;
		}
		else
		{
			g_GSD4tConfig.isEMC_ENABLE= 0;
		}
	}
	else
	{
		return;
	}

	SIRF_LOGD("***HW [EMC_ENABLE : %s]", ( g_GSD4tConfig.isEMC_ENABLE == 1 ? "TRUE" : "FALSE" ));

	return;
}

static int sirf_gps_load_config(void)
{
	char *path = SIRF_GPS_CONFIG_FILE;
	int n;

	n = load_properties(path);
	if (n < 0)
	{
		SIRF_LOGE("%s: load properties error",__FUNCTION__);
		return -1;
	}
	sirf_gps_project_name_load();
	sirf_gps_driver_path_load();
	sirf_gps_lna_config_load();
	sirf_gps_tcxo_config_load();
	sirf_gps_uart_baud_rate_config();
	sirf_gps_frequency_aiding_config();
	sirf_gps_sensor_aiding_config();
	sirf_gps_setid_imsi_config();
	sirf_gps_debugging_files_config_load();
	sirf_gps_debugging_files_path_load();
	sirf_gps_reaiding_config_load();
	sirf_gps_ssl_enable_config_load();
	sirf_gps_ssl_version_load();
	sirf_gps_cp_response_time_load();
	sirf_gps_cp_enable_config_load();
	sirf_gps_ATT_Network_Operator_check();
	sirf_gps_EMC_Enable_check();
	free_properties();

	SIRF_LOGD("%s: successfully loaded for GPS Config File",__FUNCTION__);

	return 0;
}




/* CSR RUNTIME config file */
#ifdef CSR_GPS_CONFIG

static void csr_config_set_supl(void)
{
	char *addr_str  = get_property("SUPL_SERVER");
	char *port_str  = get_property("SUPL_PORT");
	char *ssl_str   = get_property("SSL");
	char *protocol  = get_property("SSL_PROTOCOL");
	char *pca_path  = get_property("AGPS_CA_PATH");
	int port        = 7276;
	char default_addr_str[] = "supl.google.com";
	char default_pca_path[] = "/system/etc/AGPS_CA.pem";

	/* Setting SUPL server */
	if (!addr_str || strlen(addr_str) == 0 || strlen(addr_str) > 128)
	{
		SIRF_LOGW("%s: warning! setting default SUPL server ...", __FUNCTION__);
		addr_str = default_addr_str;
	}

	memset(g_GSD4tConfig.serverAddress,0,sizeof(g_GSD4tConfig.serverAddress));
	strcpy(g_GSD4tConfig.serverAddress, addr_str);

	/* Setting SUPL port */
	if (port_str)
	{
		port = atoi(port_str);
	}

	g_GSD4tConfig.serverPort = (tSIRF_UINT16) port;
	LSM_setAgpsServer(g_GSD4tConfig.serverAddress, g_GSD4tConfig.serverPort);

	/* Setting security */
	if (ssl_str)
	{
		if (ssl_str[0] == '1')
		{
			g_GSD4tConfig.isSecure = 1;
			LSM_setSecure(LSM_TRUE); /* for SUPL NI Secure Configuration */
		}
		else
		{
			g_GSD4tConfig.isSecure = 0;
			LSM_setSecure(LSM_FALSE); /* for SUPL NI Non-Secure Configuration */
		}
	}

	/* Setting protocol */
	if (0 == strcmp(protocol, "SUPL_RRLP"))
	{
		g_GSD4tConfig.AssistProtocol = 0;
	}
	else if (0 == strcmp(protocol, "SUPL_RRC"))
	{
		g_GSD4tConfig.AssistProtocol = 0;
	}
	else if (strcmp(protocol, "CP_RRLP") == 0)
	{
		g_GSD4tConfig.AssistProtocol = 1;
	}
	else if (strcmp(protocol, "CP_RRC") == 0)
	{
		g_GSD4tConfig.AssistProtocol = 2;
	}

	SIRF_LOGD("*CSR Config [SUPL : %s:%d - secure: %d - protocol: %d]", 
			g_GSD4tConfig.serverAddress, 
			g_GSD4tConfig.serverPort, 
			g_GSD4tConfig.isSecure, 
			g_GSD4tConfig.AssistProtocol);

	/* Setting AGPS CA path */
	if (!pca_path || strlen(pca_path) == 0 || strlen(pca_path) > 255)
	{
		SIRF_LOGW("%s: warning! setting default pca path ...", __FUNCTION__);
		pca_path = default_pca_path;
	}

	strncpy(ca_path, pca_path, sizeof(ca_path));
	SIRF_LOGD("*CSR Config [SUPL CA: %s]", ca_path);
}

static void csr_config_set_aiding(void)
{
	char *if_str            = get_property("INSTANT_FIX");
	char *aiding_type_str   = get_property("AIDING_TYPE");
	char *pref_str          = get_property("PREFERENCE");
	char *cap_str           = get_property("CAPABILITY");

	if (if_str)
	{
		if (if_str[0] == '1')
		{
			g_GSD4tConfig.isCapCGEE = 1;
			g_GSD4tConfig.isCapSGEE = 1;
		}
		else
		{
			g_GSD4tConfig.isCapCGEE = 0;
			g_GSD4tConfig.isCapSGEE = 0;
		}
	}

	/* Setting Aiding type */
	if (aiding_type_str)
	{
		if (strcmp(aiding_type_str, "NO_AIDING") == 0)
		{
			g_GSD4tConfig.aidingType = LSM_NO_AIDING;
		}
		else if (strcmp(aiding_type_str, "LOCAL") == 0)
		{
			g_GSD4tConfig.aidingType = LSM_LOCAL_AIDING;
		}
		else if (strcmp(aiding_type_str, "NETWORK") == 0)
		{
			g_GSD4tConfig.aidingType = LSM_NETWORK_AIDING;
		}

	}

	/* Setting preference */
	if (pref_str)
	{
		if (strcmp(pref_str, "NONE") == 0)
		{
			g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_NONE;
			g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
			g_GSD4tConfig.networkStatus = 0;
			GPS_POSITION_MODE = GPS_POSITION_MODE_STANDALONE;

		}
		else if (strcmp(pref_str, "MSA") == 0)
		{
			g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSA;
			g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
			g_GSD4tConfig.networkStatus = 1;
			GPS_POSITION_MODE = GPS_POSITION_MODE_MS_ASSISTED;
		}
		else if (strcmp(pref_str, "MSB") == 0)
		{
			g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSB;
			g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
			g_GSD4tConfig.networkStatus = 1;
			GPS_POSITION_MODE = GPS_POSITION_MODE_MS_BASED;
		}

	}

	/* MSA, MSB, ALL capabilities */
	if (cap_str)
	{
		if (strcmp(cap_str, "MSA") == 0)
		{
			configureSETCapabilities(SET_CAPABILITIES_MSA);
			gCapabilities = SET_CAPABILITIES_MSA;
		}
		else if (strcmp(cap_str, "MSB") == 0)
		{
			configureSETCapabilities(SET_CAPABILITIES_MSB);
			gCapabilities = SET_CAPABILITIES_MSB;
		}
		else if (strcmp(cap_str, "MSAB") == 0)
		{
			configureSETCapabilities(SET_CAPABILITIES_ALL);
			gCapabilities = SET_CAPABILITIES_ALL;
		}
	}

	SIRF_LOGD("*CSR Config [SIF: %d,Aiding type: %d, Preference: %d]", g_GSD4tConfig.isCapSGEE, g_GSD4tConfig.aidingType, g_GSD4tConfig.prefLocMethod);

}


static void csr_config_set_reset(void)
{
	char *reset_mode_str        = get_property("RESET_MODE");
	char *no_of_fix_str         = get_property("NO_OF_FIXES");
	char *time_between_fix_str  = get_property("TIME_B_FIXES");

	/* Setting reset mode */
	if (reset_mode_str)
	{
		if (strcmp(reset_mode_str, "COLD") == 0)
		{
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_COLD_RESET;
		}
		else if (strcmp(reset_mode_str, "HOT") == 0)
		{
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_HOT_RESET;
		}
		else if (strcmp(reset_mode_str, "WARM") == 0)
		{
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_WARM_RESET;
		}
		else if (strcmp(reset_mode_str, "FACTORY") == 0)
		{
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_FACTORY_RESET;
		}
		else
		{
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_HOT_RESET;
		}
	}

	/* Setting number of fixes */
	if (no_of_fix_str)
	{
		g_GSD4tConfig.numberOfFixes  = atoi(no_of_fix_str);

	}

	/* Setting time between fixes */
	if (time_between_fix_str)
	{
		g_GSD4tConfig.timeBetweenFixes= atoi(time_between_fix_str);

	}


	SIRF_LOGD("*CSR Config [RESET_MODE : %d - No. of fixes %d - time between fixes: %d]", 
			g_GSD4tConfig.resetType, 
			g_GSD4tConfig.numberOfFixes, 
			g_GSD4tConfig.timeBetweenFixes);
}

static int csr_property_load_config(void)
{
	char *path = CSR_GPS_CONFIG_FILE;
	int n;

	n = load_properties(path);

	if (n < 0)
	{
		SIRF_LOGD("%s: %s file not found",__FUNCTION__, path);
		return -1;
	}

	csr_config_set_supl();
	csr_config_set_aiding();
	csr_config_set_reset();

	free_properties();

	return 0;
}

static void csr_config_set_cmcc_supl(void)
{
	char *addr_str  = get_property("SUPL_SERVER_CMCC");
	char *port_str  = get_property("SUPL_PORT_CMCC");
	char *ssl_str   = get_property("SSL_CMCC");
	char *protocol  = get_property("SSL_PROTOCOL_CMCC");
	char *pca_path  = get_property("AGPS_CA_PATH_CMCC");
	int port        = 7276;
	char default_addr_str[] = "supl.google.com";
	char default_pca_path[] = "/system/etc/AGPS_CA.pem";

	/* Setting SUPL server */
	if (!addr_str || strlen(addr_str) == 0 || strlen(addr_str) > 128)
	{
		SIRF_LOGW("%s: warning! setting default SUPL server ...", __FUNCTION__);
		addr_str = default_addr_str;
	}

	memset(g_GSD4tConfig.serverAddress,0,sizeof(g_GSD4tConfig.serverAddress));
	strcpy(g_GSD4tConfig.serverAddress, addr_str);

	/* Setting SUPL port */
	if (port_str)
	{
		port = atoi(port_str);
	}

	g_GSD4tConfig.serverPort = (tSIRF_UINT16) port;
	LSM_setAgpsServer(g_GSD4tConfig.serverAddress, g_GSD4tConfig.serverPort);

	/* Setting security */
	if (ssl_str)
	{
		if (ssl_str[0] == '1')
		{
			g_GSD4tConfig.isSecure = 1;
			LSM_setSecure(LSM_TRUE); /* for SUPL NI Secure Configuration */
		}
		else
		{
			g_GSD4tConfig.isSecure = 0;
			LSM_setSecure(LSM_FALSE); /* for SUPL NI Non-Secure Configuration */
		}
	}

	/* Setting protocol */
	if (!protocol)
	{
		g_GSD4tConfig.AssistProtocol = 0;
	}
	else if (0 == strcmp(protocol, "SUPL_RRLP"))
	{
		g_GSD4tConfig.AssistProtocol = 0;
	}
	else if (0 == strcmp(protocol, "SUPL_RRC"))
	{
		g_GSD4tConfig.AssistProtocol = 0;
	}
	else if (strcmp(protocol, "CP_RRLP") == 0)
	{
		g_GSD4tConfig.AssistProtocol = 1;
	}
	else if (strcmp(protocol, "CP_RRC") == 0)
	{
		g_GSD4tConfig.AssistProtocol = 2;
	}

	SIRF_LOGD("*CSR Config [SUPL : %s:%d - secure: %d - protocol: %d]", 
			g_GSD4tConfig.serverAddress, 
			g_GSD4tConfig.serverPort, 
			g_GSD4tConfig.isSecure, 
			g_GSD4tConfig.AssistProtocol);

	/* Setting AGPS CA path */
	if (!pca_path || strlen(pca_path) == 0 || strlen(pca_path) > 255)
	{
		SIRF_LOGW("%s: warning! setting default pca path ...", __FUNCTION__);
		pca_path = default_pca_path;
	}

	strncpy(ca_path, pca_path, sizeof(ca_path));
	SIRF_LOGD("*CSR Config [SUPL CA: %s]", ca_path);
}

static int csr_property_load_cmcc_config(void)
{
	char *path = CSR_GPS_CONFIG_FILE;
	int n;

	n = load_properties(path);

	if (n < 0)
	{
		SIRF_LOGE("%s: %s file not found",__FUNCTION__, path);
		return -1;
	}

	csr_config_set_cmcc_supl();

	free_properties();

	return 0;
}

#elif defined CUSTOMER_GPS_CONFIG

static void sirf_sec_property_set_ssl(void)
{
	char *ssl_str = get_property("SSL");
	int ssl_enabled = 0;

	// in secgps.conf, SSL=0 means use SSL.
	if (ssl_str)
	{
		if (ssl_str[0] == '0')
		{
			ssl_enabled = 0;
		}
		else
		{
			ssl_enabled = 1;
		}
	}
	else
	{
		ssl_enabled = 0;
	}

	// 0 : SSL on
	// 1 : SSL off

	g_GSD4tConfig.isSecure = ssl_enabled;

	if(g_GSD4tConfig.isSecure == 1)
	{
		LSM_setSecure(LSM_TRUE); /* for SUPL NI Secure Configuration */
	}
	else
	{
		LSM_setSecure(LSM_FALSE);
	}

	SIRF_LOGD("*AngryGPS [SSL : %s]", ( ssl_enabled == 1 ? "ENABLED" : "DISABLED" ));

	return;
}

static void sirf_sec_property_set_operation_mode(void)
{
	int mode = GPS_POSITION_MODE_MS_BASED;
	char *str = get_property("OPERATION_MODE");
	char *src_str = get_property("OPERATION_TEST_MODE");
	char *cap_str = get_property("CAPABILITY");

	/* MSA, MSB, ALL capabilities */
	if (cap_str)
	{
		if (strcmp(cap_str, "MSA") == 0)
		{
			configureSETCapabilities(SET_CAPABILITIES_MSA);
			gCapabilities = SET_CAPABILITIES_MSA;
		}
		else if (strcmp(cap_str, "MSB") == 0)
		{
			configureSETCapabilities(SET_CAPABILITIES_MSB);
			gCapabilities = SET_CAPABILITIES_MSB;
		}
		else if (strcmp(cap_str, "MSAB") == 0)
		{
			configureSETCapabilities(SET_CAPABILITIES_ALL);
			gCapabilities = SET_CAPABILITIES_ALL;
		}
	}


	if (atoi(src_str) == 0)
	{
		if(g_GSD4tConfig.isTestModeEnable)
		{
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_COLD_RESET;
			g_GSD4tConfig.isTestModeEnable = LSM_FALSE;
			SIRF_LOGD("Test Mode already executed, run Cold Start!!!");
		}

		if (str)
		{
			if (strcmp(str, "MSBASED") == 0)
			{
				mode = GPS_POSITION_MODE_MS_BASED;
				g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
				g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSB;
				g_GSD4tConfig.aidingType = LSM_NETWORK_AIDING;
				g_GSD4tConfig.networkStatus = 1;
				GPS_POSITION_MODE = GPS_POSITION_MODE_MS_BASED;
				//SIRF_LOGD("[GPS_POSITION_MODE_MS_BASED]");
			}
			else if (strcmp(str, "MSASSISTED") == 0)
			{
				mode = GPS_POSITION_MODE_MS_ASSISTED;
				g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
				g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSA;
				g_GSD4tConfig.aidingType = LSM_NETWORK_AIDING;
				g_GSD4tConfig.networkStatus = 1;
				GPS_POSITION_MODE = GPS_POSITION_MODE_MS_ASSISTED;
				//SIRF_LOGD("[GPS_POSITION_MODE_MS_ASSISTED]");
			}
			else
			{
				g_GSD4tConfig.isPrefLocationMethodSet = LSM_TRUE;
				g_GSD4tConfig.prefLocMethod = LSM_PREFERRED_LOC_METHOD_MSB;
				g_GSD4tConfig.aidingType = LSM_NO_AIDING;
				g_GSD4tConfig.networkStatus = 0;
				//SIRF_LOGD("[GPS_POSITION_MODE_STANDALONE]");
				GPS_POSITION_MODE = GPS_POSITION_MODE_STANDALONE;
			}
		}
	}
	else
	{
		GPS_POSITION_MODE = GPS_POSITION_MODE_STANDALONE;
		g_GSD4tConfig.isTestModeEnable = LSM_TRUE;
		g_GSD4tConfig.resetType = LSM_RESET_TYPE_COLD_RESET;

		if (str)
		{
			if (strcmp(str, "H/W SENSITIVITY TEST") == 0)
			{
				SIRF_LOGD("[LSM_TESTMODE_4 : H/W SENSITIVITY TEST]");
				g_GSD4tConfig.testModeType = LSM_TESTMODE_4;
				g_GSD4tConfig.testModeSvId = 0x01;
				g_GSD4tConfig.testModePeriod = 10;
				g_GSD4tConfig.isCNOTest = 0;
			}
			else if (strcmp(str, "H/W CNO TEST") == 0)
			{
				SIRF_LOGD("[LSM_TESTMODE_4 : H/W CNO TEST]");
				g_GSD4tConfig.testModeType = LSM_TESTMODE_4;
				g_GSD4tConfig.testModeSvId = 0x01;
				g_GSD4tConfig.testModePeriod = 120;
				g_GSD4tConfig.isCNOTest = 1;
				sCNOTestCnt = 0;
			}
			else if (strcmp(str, "H/W CW TEST") == 0)
			{
				SIRF_LOGD("[LSM_TESTMODE_7 : H/W CW TEST]");
				g_GSD4tConfig.resetType = LSM_RESET_TYPE_TEST_MODE;
				g_GSD4tConfig.testModeType = LSM_TESTMODE_7;
				g_GSD4tConfig.testModeSvId = 0x00;
				g_GSD4tConfig.testModePeriod = 0;
				g_GSD4tConfig.isCNOTest = 0;
				sCWTestCnt = 0;
				CWTestModeResultPeakValue.cno = 0;
				CWTestModeResultPeakValue.freq = 0;
				memset(CW_MEASURED, 0, CW_SAMPLES);
			}
		}
	}

	//  SIRF_LOGD("%s: successfully set to %d",__FUNCTION__, mode);

	return;
}

static void sirf_sec_property_set_start_mode(void)
{
	char *start_mode_str = get_property("START_MODE");
	int start_mode = 0;
	if (start_mode_str)
	{
		if (start_mode_str[0] == '0')
		{
			start_mode = 0;
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_COLD_RESET;
		}
		else if (start_mode_str[0] == '1')
		{
			start_mode = 1;
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_HOT_RESET;
		}
		else if (start_mode_str[0] == '2')
		{
			start_mode = 2;
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_WARM_RESET;
		}
		else
		{
			start_mode = 1;
			g_GSD4tConfig.resetType = LSM_RESET_TYPE_HOT_RESET;
		}
	}
	SIRF_LOGD("*AngryGPS [START_MODE : %d]", start_mode);
	return;
}

static void sirf_sec_property_set_host(void)
{
	char *addr_str = get_property("SUPL_HOST");
	char *port_str = get_property("SUPL_PORT");
	int port       = 7276;
	char default_addr_str[] = "supl.google.com";

	if (!addr_str || strlen(addr_str) == 0 || strlen(addr_str) > 128)
	{
		SIRF_LOGW("%s: warning! setting default SUPL server ...", __FUNCTION__);
		addr_str = default_addr_str;
	}

	if (port_str)
	{
		port = atoi(port_str);
	}

	memset(g_GSD4tConfig.serverAddress,0,sizeof(g_GSD4tConfig.serverAddress));
	strcpy(g_GSD4tConfig.serverAddress, addr_str);
	g_GSD4tConfig.serverPort = (tSIRF_UINT16) port;

	LSM_setAgpsServer(g_GSD4tConfig.serverAddress, g_GSD4tConfig.serverPort);
	SIRF_LOGD("*AngryGPS [SUPL : %s:%d]", addr_str, port);

	return;
}

static void sirf_sec_property_set_agps_mode(void)
{
	char *agps_mode_str = get_property("AGPS_MODE");
	int agps_mode = 0;

	// in secgps.conf, AGPS_MODE=0 means use SUPL. else is Control Plan

	if (agps_mode_str)
	{
		if (agps_mode_str[0] == '0')
		{
			agps_mode = 0;
			g_GSD4tConfig.defaultResponseTimeoutofNI = SUPL_NI_NOTIFICATION_DEFAULT_RESPONSE_TIMEOUT; //6000;
		}
		else
		{
			agps_mode = 1;
			g_GSD4tConfig.defaultResponseTimeoutofNI = CP_NI_NOTIFICATION_DEFAULT_RESPONSE_TIMEOUT;//20000;
		}
	}

	// agps_mode = 0 is SUPL
	// agps_mode = 1 is Control Plan

	SIRF_LOGD("*AngryGPS [AGPS MODE : %s]",( agps_mode == 1 ? "CONTROL PLANE" : "USER PLANE" ));

	return;
}

static void sirf_sec_property_set_gpsplus_mode(void)
{
	char *gps_plus_mode_str = get_property("ENABLE_XTRA");
	//SIRF_LOGD("mode : %s", gps_plus_mode_str);

	if (gps_plus_mode_str)
	{
		if (gps_plus_mode_str[0] == '1')
		{
			g_GSD4tConfig.isCapCGEE =  1;
			g_GSD4tConfig.isCapSGEE =  1;
			SIRF_LOGD("*AngryGPS [SIF: Enabled.]");
		}
		else
		{
			g_GSD4tConfig.isCapCGEE = 0;
			g_GSD4tConfig.isCapSGEE =  0;
			SIRF_LOGD("*AngryGPS [SIF: Disabled.]");
		}
	}

	//SIRF_LOGD("[GPS PLUS : %s]",  ( g_GSD4tConfig.isCapCGEE == 1 ? "ENABLED" : "DISABLED" ));

	return;
}

static void sirf_sec_property_set_dynamic_acc_mode(void)
{
	char *dynamic_acc_mode_str = get_property("DYNAMIC_ACCURACY");
	int dynamic_acc_mode = 0;

	if (dynamic_acc_mode_str)
	{
		if (dynamic_acc_mode_str[0] == '0')
		{
			dynamic_acc_mode = 0;
			g_GSD4tConfig.numberOfFixes = 1;
			SIRF_LOGD("*AngryGPS [TTFF TEST : SINGLE SHOT]");
		}
		else
		{
			dynamic_acc_mode = 1;
			g_GSD4tConfig.numberOfFixes = 0;
			SIRF_LOGD("*AngryGPS [TRACKING MODE]");
		}
	}

	// dynamic_accuracy = 0 is off
	// dynamic_accuracy = 1 is on

	//SIRF_LOGD("[DYNAMIC_ACCURACY : %s]", ( dynamic_acc_mode == 1 ? "ENABLED" : "DISABLED" ));
}

static void sirf_sec_property_set_acc(void)
{
	char *value_str = get_property("ACCURACY");
	int acc_value = 50;

	if (value_str)
	{
		acc_value = atoi(value_str);
		//SIRF_LOGD("%s: acc_value = %d",__FUNCTION__, acc_value);
	}

	g_GSD4tConfig.QoSHorizontalAccuracy = acc_value;
	SIRF_LOGD("*AngryGPS [HORIZONTAL ACCURACY = %d]", acc_value);
}

static int sirf_sec_property_load_config(void)
{
	char *path = SEC_CONFIG_FILE;
	int n;

	strcpy(ca_path, "/system/etc/AGPS_CA.pem");

	n = load_properties(path);

	if (n < 0)
	{
		SIRF_LOGD("%s: load properties error",__FUNCTION__);
		return -1;
	}

	sirf_sec_property_set_gpsplus_mode();
	sirf_sec_property_set_start_mode();
	sirf_sec_property_set_operation_mode();
	sirf_sec_property_set_ssl();
	sirf_sec_property_set_host();
	//sirf_sec_property_set_gps_logging();
	sirf_sec_property_set_agps_mode();
	sirf_sec_property_set_dynamic_acc_mode();
	sirf_sec_property_set_acc();

	free_properties();

	//SIRF_LOGD("%s: successfully loaded for LbsTestMode",__FUNCTION__);

	return 0;
}

static LSM_BOOL sirf_sec_check_factory_test_mode(void)
{
	FILE *fp = NULL;

	////fp = fopen("/data/data/com.sec.android.app.factorytest/files/gps_started","r");	
	fp = fopen("/tmp/gps_started","r");
	if( fp != NULL)
	{
		SIRF_LOGD("%s: called - factory test mode enabled.", __FUNCTION__);
		GPS_POSITION_MODE = GPS_POSITION_MODE_STANDALONE;
		g_GSD4tConfig.isTestModeEnable = LSM_TRUE;
		g_GSD4tConfig.testModeType = LSM_TESTMODE_7;
		g_GSD4tConfig.testModeSvId = 0x00;
		g_GSD4tConfig.testModePeriod = 0;
		g_GSD4tConfig.isCNOTest = 0;
		g_GSD4tConfig.resetType =  LSM_RESET_TYPE_TEST_MODE;
		sCWTestCnt = 0;
		CWTestModeResultPeakValue.cno = 0;
		CWTestModeResultPeakValue.freq = 0;
		memset(CW_MEASURED, 0, CW_SAMPLES);
		return LSM_TRUE;
	}
	else
	{
		return LSM_FALSE;
	}
}


#else

SIRF_LOGD("No GPS_CONFIG define found, using default from code");

#endif //GPS CONFIG options


const GpsInterface* sirf_get_gps_interface(struct gps_device_t* dev)
{

	SIRF_LOGD("%s: called", __FUNCTION__);

	return &sirfGpsInterface;
}

static int sirf_open_gps_driver(const struct hw_module_t* module, char const* name, struct hw_device_t** device)
{
	static int initialize_check = 0;

	SIRF_LOGD("%s: called", __FUNCTION__);

	memset(&sirf_gps_device, 0, sizeof(sirf_gps_device));

	sirf_gps_device.common.tag = HARDWARE_DEVICE_TAG;
	sirf_gps_device.common.version = 0;
	sirf_gps_device.common.module = (struct hw_module_t *)module;
	sirf_gps_device.get_gps_interface = sirf_get_gps_interface;

	*device = (struct hw_device_t*)&sirf_gps_device;

	if (initialize_check == 0)
	{
		initialize_check = 1;
		initialize_gps();
	}

	return 0;
}

static struct hw_module_methods_t sirf_gps_module_methods =
{
	.open = sirf_open_gps_driver
};

const struct hw_module_t HAL_MODULE_INFO_SYM =
{
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = GPS_HARDWARE_MODULE_ID,
	.name = "sirf_gsd4t",
	.author = "CSR SiRF",
	.methods = &sirf_gps_module_methods,
};





/**
 * @}
 * End of file.
 */

