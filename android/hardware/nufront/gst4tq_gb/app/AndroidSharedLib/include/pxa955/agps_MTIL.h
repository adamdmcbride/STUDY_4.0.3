/********************************************************************
	Filename:	agps_MTIL.h
	Created:	2010/05/15

	Purpose:	Constant definitions and function prototypes for 
				telephony interface wrapper to application layer
*********************************************************************/

#ifndef _agps_MTIL_h
#define _agps_MTIL_h

/* Header files includes */
#include "LSM_APIs.h"
#include "CpaClient.h"

/* Macro definitions */
#define AGPS_MTIL_SHARED_OBJ_PATH			"/marvell/usr/lib/libagps_MTIL.so"

#ifndef UNUSEDPARAM
#define UNUSEDPARAM(param)					((void) param)
#endif /* UNUSEDPARAM */

#ifdef SIRF_LPL
#else  /* SIRF_LPL */
#define LOGE	AGPS_TRACE_LOGCAT
#define LOGD	AGPS_TRACE_LOGCAT
#define LOGI	AGPS_TRACE_LOGCAT

//#define EXTRA_DEBUG_TRACES					// Disable this line (Extra Debug Traces) before release

#endif /* SIRF_LPL */


typedef LSM_BOOL (*CP_SetMessageListeners_func)(LSM_MessageListener listener);
typedef LSM_BOOL (*CP_SendNetMessage_func)(eLSM_OTA_TYPE ota_type,
										   eLSM_RRC_STATE rrc_state,
										   eLSM_SESSION_PRIORITY sessionPriority,
										   LSM_UINT8* p_msg_data,
										   LSM_UINT32 msg_size);
typedef LSM_BOOL (*CP_SendMeasurementTerminate_func)(eLSM_OTA_TYPE otaType);
typedef LSM_BOOL (*CP_SendRRCStateEvent_func)(eLSM_RRC_STATE rrc_state);
typedef LSM_BOOL (*CP_NotfiyE911Dialed_func)(tSIRF_VOID);
typedef LSM_BOOL (*CP_Reset_func)(tSIRF_VOID);
typedef LSM_BOOL (*CP_MtlrNotifyLocInd_func)(P_CPA_LCS_LOCATION_IND pCpaLcsLocationInd);
typedef LSM_BOOL (*CP_SendCellInfo_func)(LSM_netCellID *pCellInfo, LSM_BOOL cellInfoValid);
typedef LSM_BOOL (*CP_SendSETIDInfo_func)(SETID_Info *pSETidInfo, LSM_BOOL isSETIDInfoValid);
typedef void     (*logBrief_func)(LSM_CHAR *pMsg);

typedef struct CP2LSM_t
{
	CP_SetMessageListeners_func			CP_SetMessageListeners;
	CP_SendNetMessage_func				CP_SendNetMessage;
	CP_SendMeasurementTerminate_func	CP_SendMeasurementTerminate;
	CP_SendRRCStateEvent_func			CP_SendRRCStateEvent;
	CP_NotfiyE911Dialed_func			CP_NotfiyE911Dialed;
	CP_Reset_func						CP_Reset;
	CP_MtlrNotifyLocInd_func			CP_MtlrNotifyLocInd;
	CP_SendCellInfo_func				CP_SendCellInfo;
	CP_SendSETIDInfo_func				CP_SendSETIDInfo;
	logBrief_func						logBrief;
} CP2LSM;

// C-Plane Functions
typedef int			(*CpaClient_Init_func)(void);
typedef int			(*CpaClient_Deinit_func)(void);
typedef LSM_BOOL	(*CpaClient_SendLcsVerificationResp_func)(unsigned long notifId, unsigned long present, unsigned long verificationResponse);
typedef int			(*CpaClient_GetNitzInfo_func)(AGPS_NITZ_STATUS_MSG* pNitzStatus);
typedef int			(*CpaClient_GetServiceStatus_func)(int* pIsInService);

typedef CP2LSM*		(*CP2LSM_GetEnvPtr_func)(void);

// Invoke this first
#define DECLARE_AGPSMtilLib \
	static void* agpsMTILdlHandle = NULL; \
	int b_mtil_lib_binded = 0; \
	CpaClient_Init_func								CpaClient_Init = NULL; \
	CpaClient_Deinit_func							CpaClient_Deinit = NULL; \
	CpaClient_SendLcsVerificationResp_func			CpaClient_SendLcsVerificationResp = NULL; \
	CpaClient_GetNitzInfo_func						CpaClient_GetNitzInfo = NULL; \
	CpaClient_GetServiceStatus_func					CpaClient_GetServiceStatus = NULL; \
	CP2LSM_GetEnvPtr_func							CP2LSM_GetEnvPtr = NULL; \
	extern void OM_logBrief(LSM_CHAR *pMsg);

// Invoke this only after DECLARE_AGPSMtilLib
#define BIND_AGPSMtilLib \
{ \
	agpsMTILdlHandle = dlopen(AGPS_MTIL_SHARED_OBJ_PATH, RTLD_NOW); \
	if (agpsMTILdlHandle != NULL) \
	{ \
		CpaClient_Init =							(CpaClient_Init_func)						dlsym(agpsMTILdlHandle, "CpaClient_Init"); \
		CpaClient_Deinit =							(CpaClient_Deinit_func)						dlsym(agpsMTILdlHandle, "CpaClient_Deinit"); \
		CpaClient_SendLcsVerificationResp =			(CpaClient_SendLcsVerificationResp_func)	dlsym(agpsMTILdlHandle, "CpaClient_SendLcsVerificationResp"); \
		CpaClient_GetNitzInfo =						(CpaClient_GetNitzInfo_func)				dlsym(agpsMTILdlHandle, "CpaClient_GetNitzInfo"); \
		CpaClient_GetServiceStatus =				(CpaClient_GetServiceStatus_func)			dlsym(agpsMTILdlHandle, "CpaClient_GetServiceStatus"); \
		b_mtil_lib_binded = 1; \
	} \
}

// Invoke this only after BIND_AGPSMtilLib
#define UPDATE_LSMEnv \
{ \
	if (agpsMTILdlHandle != NULL) \
	{ \
		CP2LSM_GetEnvPtr = (CP2LSM_GetEnvPtr_func) dlsym(agpsMTILdlHandle, "CP2LSM_GetEnvPtr"); \
		if (CP2LSM_GetEnvPtr != NULL) \
		{ \
			CP2LSM *cp2lsmenv = CP2LSM_GetEnvPtr(); \
			cp2lsmenv->CP_SetMessageListeners		= CP_SetMessageListeners; \
			cp2lsmenv->CP_SendNetMessage			= cpa_CP_SendNetMessage; \
			cp2lsmenv->CP_SendMeasurementTerminate	= cpa_CP_SendMeasurementTerminate; \
			cp2lsmenv->CP_SendRRCStateEvent			= CP_SendRRCStateEvent; \
			cp2lsmenv->CP_NotfiyE911Dialed			= CP_NotfiyE911Dialed; \
			cp2lsmenv->CP_Reset						= CP_Reset; \
			cp2lsmenv->CP_MtlrNotifyLocInd			= CP_MtlrNotifyLocInd; \
			cp2lsmenv->CP_SendCellInfo 				= CP_SendCellInfo; \
			cp2lsmenv->CP_SendSETIDInfo 			= CP_SendSETIDInfo; \
			cp2lsmenv->logBrief						= OM_logBrief; \
		} \
	} \
}

/* Declarations of external functions */
extern void AGPS_TRACE_LOGCAT(const char *fmt, ...);


#endif /* _agps_MTIL_h */

