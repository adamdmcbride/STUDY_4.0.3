/**
 * Author : Lasse Bigum (lasse.bigum@csr.com)
 * Group : CSR HPE
 * Date : 2011.05.16
 * Project : SS GSD4t
 */
#include "gps_logging.h"
#include "sirf_pal.h"

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       I N T E R F A C E                               *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/

FILE* mPosFile = NULL;

void DBGPRINTF(unsigned int logLevel, const char * format, ...)
{
    char text_out[512] = { 0 };

    va_list ap;
    va_start( ap, format );
    vsnprintf(text_out, sizeof(text_out) - 1, format, ap);

    switch (logLevel)
    {
        case SIRF_LOGLEVEL_VERBOSE:
        {
            LOGV("%s", text_out);
            break;
        }
        case SIRF_LOGLEVEL_DEBUG:
        {
            LOGD("%s", text_out);
            break;
        }
        case SIRF_LOGLEVEL_WARNING:
        {
            LOGW("%s", text_out);
            break;
        }
        case SIRF_LOGLEVEL_ERROR:
        {
            LOGE("%s", text_out);
            break;
        }
        case SIRF_LOGLEVEL_INFO:
        default:
        {
            LOGI("%s", text_out);
            break;
        }

    }
    if (mPosFile != NULL)
    {
#if 0 //Enable to get timestamp in log
        tSIRF_DATE_TIME date_time;
        SIRF_PAL_OS_TIME_DateTime(&date_time);
       
        
        fprintf(mPosFile, "<%4d:%02d:%02d:%02d:%02d:%02d>: %s\r\n", date_time.year, date_time.month, date_time.day, date_time.hour,
                date_time.minute, date_time.second, text_out);
#else
        fprintf(mPosFile, "%s\r\n", text_out);        
        fflush(mPosFile);
#endif        
    }
    va_end(ap);
}

void sirf_interface_log_init(char *logPath)
{
    char data_string[256] = { 0 };
    tSIRF_DATE_TIME date_time;

    SIRF_LOGD("%s called - path: %s", __FUNCTION__, logPath);

    if (NULL == mPosFile)
    {
        char fullPath[256];
        sprintf(fullPath, "%s%s", logPath, MPOS_FILE);
        SIRF_LOGD("full path: %s",fullPath);
        mPosFile = fopen(fullPath, "wb");
    }

    if (SIRF_PAL_OS_TIME_DateTime(&date_time) == SIRF_SUCCESS)
    {
        snprintf(data_string, sizeof(data_string) - 1,
                "CSR GPS Interface Log Message <%4d:%02d:%02d:%02d:%02d:%02d>",
                date_time.year, date_time.month, date_time.day, date_time.hour,
                date_time.minute, date_time.second);

        SIRF_LOGI(data_string);
        SIRF_LOGI("******************* Version Information, LSM & LPL *******************");
        SIRF_LOGI("%s", LSM_GetLSMVersion());
    }
}

void sirf_interface_log_deinit(void)
{
    if (mPosFile)
    {
        fclose(mPosFile);
        mPosFile = NULL;
    }
}


void sirf_interface_log_time_stamp(void)
{
    char data_string[256]= {0};
    tSIRF_DATE_TIME date_time;

    if(MPOS_FILE == NULL)
    {
        return; //Do Nothing!!!
    }

    if (SIRF_PAL_OS_TIME_DateTime(&date_time) == SIRF_SUCCESS)
    {
        snprintf(data_string, sizeof(data_string) - 1 , "CSR GPS TimeStamp Message <%4d:%02d:%02d:%02d:%02d:%02d>",
            date_time.year, date_time.month, date_time.day,
            date_time.hour, date_time.minute, date_time.second);

        SIRF_LOGD(data_string);
    }
}

#if ENABLE_NMEA_FILE
FILE* mNmeaFile = NULL;

void NMEAPRINTF( const char * format, ... )
{
    char text_out[512] = { 0 };
    va_list ap;

    if (mNmeaFile != NULL)
    {
        va_start( ap, format );
        vsnprintf( text_out, sizeof(text_out)-1, format, ap );
        va_end(ap);
        fprintf(mNmeaFile, "%s\r\n", text_out);
        fflush(mNmeaFile);
    }
}

void sirf_nmea_log_init(void)
{
    SIRF_LOGV("%s: called", __FUNCTION__);
    tSIRF_DATE_TIME date_time;
    char nmeafilename[128]= {0};
    
    if (SIRF_PAL_OS_TIME_DateTime(&date_time) == SIRF_SUCCESS)
    {
        snprintf(nmeafilename, sizeof(nmeafilename) - 1 , "sirf_nmea_%4d%02d%02d%02d%02d.txt", 
            date_time.year, date_time.month, date_time.day,
            date_time.hour, date_time.minute);
    }
    else
    {
        snprintf(nmeafilename, sizeof(nmeafilename) - 1, "%s", NMEA_FILE);
    }


    if (NULL == nmeafilename)
    {
        mNmeaFile = fopen(nmeafilename, "wb");
    }
}

void sirf_nmea_log_deinit(void)
{
    SIRF_LOGV("%s: called", __FUNCTION__);
    if (mNmeaFile)
    {
        fclose(mNmeaFile);
        mNmeaFile = NULL;
    }
}
#endif
