/**
 * Author : Lasse Bigum (lasse.bigum@csr.com)
 * Group : CSR HPE
 * Date : 2011.05.16
 * Project : SS GSD4t
 */

#ifndef GPS_LOGGING_H_
#define GPS_LOGGING_H_

#ifndef LOG_TAG
#define  LOG_TAG  "gps_sirf"
#endif
#include <cutils/log.h>

#define SIRF_LOGLEVEL_VERBOSE     0
#define SIRF_LOGLEVEL_DEBUG       1
#define SIRF_LOGLEVEL_INFO        2
#define SIRF_LOGLEVEL_WARNING     3
#define SIRF_LOGLEVEL_ERROR       4

#define  SIRF_LOGV(...)   DBGPRINTF(SIRF_LOGLEVEL_VERBOSE, __VA_ARGS__)
#define  SIRF_LOGD(...)   DBGPRINTF(SIRF_LOGLEVEL_DEBUG, __VA_ARGS__)
#define  SIRF_LOGI(...)   DBGPRINTF(SIRF_LOGLEVEL_INFO, __VA_ARGS__)
#define  SIRF_LOGW(...)   DBGPRINTF(SIRF_LOGLEVEL_WARNING, __VA_ARGS__)
#define  SIRF_LOGE(...)   DBGPRINTF(SIRF_LOGLEVEL_ERROR, __VA_ARGS__)

//#define BRIEF_LOGFILE       "/BriefLog.txt" //include if original BriefLog should be used
#define DETAILED_LOGFILE    "/DetailedLog.txt"
#define AGPS_LOGFILE        "/AGPSLog.txt"
#define SLC_LOGFILE         "/SLCLog.gp2"
#define MPOS_FILE           "/BriefLog.txt"
#define NMEA_FILE           "/sirf_nmea.txt"

/* Function Prototypes */
void DBGPRINTF(unsigned int logLevel, const char * format, ...);
void sirf_interface_log_init(char *logPath);
void sirf_interface_log_deinit(void);
void sirf_interface_log_time_stamp(void);

#if ENABLE_NMEA_FILE
void NMEAPRINTF( const char * format, ... );
void sirf_nmea_log_init(void);
void sirf_nmea_log_deinit(void);
#endif

#endif /* GPS_LOGGING_H_ */
