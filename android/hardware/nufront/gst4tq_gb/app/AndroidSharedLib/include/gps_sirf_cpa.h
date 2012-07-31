/* Standard Header Files */
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>

#include "LSM_APIs.h"
#include "gps.h"

#define  LOG_TAG  "gps_sirf"

#include <cutils/log.h>
#define  GPS_DEBUG 1
#define  DFR(...)   LOGD(__VA_ARGS__)
#if GPS_DEBUG
#  define  D(...)   LOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

#include "gps_cpa_lsm_intf.h"


#define GPS_WAKE_LOCK_ENABLE  0
#define GPS_WAKE_LOCK_DISABLE 1

GpsNiNotification CP_mtlrNotification;

LSM_BOOL sirf_cpa_send_msg_to_ril(int  oem_ril_gps_request, void *data, size_t length);
LSM_BOOL sirf_cpa_message_received_from_ril(const void *data, size_t datalen);
LSM_BOOL wait_for_cpsession_complete(void);

int set_frequency_transfer(int request_id, int enable);
int sirf_inject_hw_frequency(double frequency,  double accuracy);

LSM_BOOL cpa_CP_SendMeasurementTerminate(eLSM_OTA_TYPE otaType);
LSM_BOOL cpa_CP_SendNetMessage(eLSM_OTA_TYPE ota_type,
                           eLSM_RRC_STATE rrc_state,
                           eLSM_SESSION_PRIORITY sessionPriority,
                           LSM_UINT8* p_msg_data,
                           LSM_UINT32 msg_size);

