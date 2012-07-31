/*///////////////////////////////////////////////////////////////////////////// */
/*                      SiRF Technology Inc. */
/* */
/*  SiRF Technology Inc.  Confidential and proprietary.   */
/*  This source code is the sole property of SiRF Technology Inc.  Reproduction */
/*  or utilization of this source code in whole or in part is forbidden without */
/*  the written consent of SiRF Technology Inc. */
/*  Use, duplication, or disclosure by the goverment is subject to restrictions. */
/* */
/*  (c) SiRF Technology Inc.  2007 - All Rights Reserved. */
/* */
/*///////////////////////////////////////////////////////////////////////////// */
/*  File:           LPLAdaptationLayer.h */
/*  Description:    This file contains a few macros, OS independant type definitions and structures */

#ifndef _LPL_ADAPTATION_LAYER_H
#define _LPL_ADAPTATION_LAYER_H

#include <stdarg.h>
#include "LPLTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*specifies infinite waiting time period for the the event */
#define LPL_INFINITE -1
/*specifies max length for error description from platform in LPLGetLastError */
#define LPL_MAX_ERROR_DES_LEN 50
/*specifies max length for various required object names */
#define LPL_MAX_OBJ_NAME      50
/*specifies max length of debug string */
#define LPL_MAX_DEBUG_STR_LEN  1024
/*specifies new line character */
#define LPL_NEWLINESTR "\x0D\x0A"
/*encoding format UTF-8 */
#define LPL_UTF8 1
/* Open the file if exist,else return error */
#define OPEN_IF_EXIST   0x01
/* Open the file if exist or create the new file */
#define CREATE_NEW_ALWAYS  0x02
/* Open the file in append mode if exist, else create new file */
#define OPEN_APPEND_MODE 0x04


/* All handles are stored inside LPL as void* and they are returned it in the same form. */
/* One has to take care for required castings. */

/* for platform dependent FILE Handling */
typedef void* LPLFileHandler;
/* for platform dependent serial port handle */
typedef void* LPLSerialHandle;
/* for platform dependent task handle */
typedef void* LPLTaskHandle;
/* for platform dependent event object */
typedef void* LPLEventHandle;
/* for platform dependent critical_section */
typedef void* LPLCriticalSectionHandle;
/* for platform dependent critical_section */
typedef void* LPLNetworkHandle;
/* for platform dependent critical_section */
typedef void* LPLTimerHandle;

typedef void* LPLSemHandle;

/*to pass network configuration parameters */
typedef struct _LPLNetworkConfig
{
    LPLChar serverIP[64];
    tSIRF_UINT32 serverPort;
    LPLChar bindServerIP[64];
    tSIRF_UINT32 bindServerPort;
    LPLBool isBind; 
    LPLBool isSecure;
}LPLNetworkConfig;

/*to pass serial configuration parameters */
typedef struct _LPLSerialConfig
{
    LPLChar portName[10];
    tSIRF_UINT32  baudRate;
    LPLBool isRead;
    LPLBool isWrite;
}LPLSerialConfig;

/*To pass the file openinng parameter */
typedef struct _LPLFileConfig
{
    LPLChar fileName[256];
    LPLBool isRead;
    LPLBool isWrite;
    tSIRF_UINT8 fileCreationMode;
}LPLFileConfig;

/*to pass serial configuration parameters */
typedef struct _LPLBTConfig
{
    tSIRF_UINT16 BTaddress[50];
    tSIRF_UINT16 BTport;
}LPLBTConfig;

/*define platform specific function declarations  */
#if defined(LPL_LINUX) || defined(__SYMBIAN32__)
typedef void * (*LPLTaskRoutine)(void* param);
#else
typedef void (*LPLTaskRoutine)(void* param);
#endif
typedef void (*LPLTimerCallBack)(void * param);
typedef void (*DataCallBack) (tSIRF_UINT32 type , tSIRF_UINT8 *data);

/* Get Last Error API */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLGetLastError(tSIRF_UINT32* errCode, LPLChar* errorDescriptor);

/*Task creation APIs */
/* Creates Task, pass the task handle as void* in taskHandlePtr, */
/* Task entry function is specified by LPLTaskRoutine (callback) */
/* and the parameter passed to entry function is */
/* param */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLCreateTask(LPLTaskHandle* taskHandlePtr,
                      LPLTaskRoutine callback,
                      void* param, LPLChar* taskName);
/* Destorys the task specified by LPLTaskHandle */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLDestroyTask(LPLTaskHandle* taskHandlePtr);

/*Critical Section APIs */
/*Creates the Critical Section and the handle is passed in LPLCriticalSection */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLCreateCriticalSection(LPLCriticalSectionHandle* criticalSectionPtr);
/*Enters the Ciritcal Section specified by LPLCriticaclSectionHandle */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLEnterCriticalSection(LPLCriticalSectionHandle criticalSectionPtr);
/*Leaves the Ciritcal Section specified by LPLCriticaclSectionHandle */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLLeaveCriticalSection(LPLCriticalSectionHandle criticalSectionPtr);
/* Destorys the Critical Section specified by LPLCriticalSection */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLDestroyCriticalSection(LPLCriticalSectionHandle* criticalSectionPtr);

/*Event APIs */
/*Creates the Event and passes the Handle in LPLEventHandle */
/* if manualreset is true then event is resetted manually, automatically otherwise */
/* initialStateReset if true indicates initial state as reset, unreset otherwise */
/* name of event object */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLCreateEvent(LPLEventHandle* handlePtr, LPLBool manualreset,
                       LPLBool initialStateReset, LPLChar* name);
/* waits on the event object specified by LPLEventHandle for the specified timePeriod */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLWaitOnEvent(LPLEventHandle eventHandle, tSIRF_INT32 timePeriod);
/*sets the event specified by LPLEventHandle */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLSetEvent(LPLEventHandle eventHandle);
/* Destorys the Event specified by LPLEventHandle */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLDestroyEvent(LPLEventHandle* handlePtr);

/*Timer APIs */
/* Creates a aperiodic Timer i.e timer of non-periodical nature, */
/* of the given timerName, which expires in specified timePeriod  */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLCreateTimer(LPLTimerHandle* timerHandlePtr,
                       tSIRF_INT32 timePeriod, LPLChar* timerName, tSIRF_UINT8 timerID);
/* sets the callback function for the specified timer */
/* which takes specified param as parameter */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLTimerSetCallBack(LPLTimerHandle* timerHandlePtr,
                            LPLTimerCallBack callBack, void* param);
/* starts the timer */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLStartTimer(LPLTimerHandle* timerHandlePtr);
/* stops the timer */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLStopTimer(LPLTimerHandle* timerHandlePtr);
/* checks if timer is running */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLIsTimerEnabled(LPLTimerHandle* timerHandlePtr);
/* Destorys the Timer indicated by LPLTimerHandle */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLDestroyTimer(LPLTimerHandle* timerHandlePtr);
/* resets the time period of timer specified by LPLTimerHandle */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLSetTimerPeriod(LPLTimerHandle* timerHandlePtr, int timerPeriod);

/* Network Creation APIs */
/* Creates Network connection, required handle is passed in LPLNetworkHandle. */
/* Network connection config info is passed in configIngo */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLCreateNetworkConnection(LPLNetworkHandle* networkHandlePtr,
                                   LPLNetworkConfig* configInfo);
/*Destorys the network connection specified by LPLNetworkHandle */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLDestroyNetworkConnection(LPLNetworkHandle* networkHandlePtr);
/* Reads data avaialble in the specified network connection in inBuf */
/* which has inBufLen length. dataRead indicates number of bytes Read. */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLReadNetworkBytes(LPLNetworkHandle networkHandlePtr,
                            tSIRF_UINT8* inBuf, tSIRF_UINT16 inBufLen,
                            tSIRF_UINT16* dataRead);
/* writes data in specified network connection. */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLWriteNetworkBytes(LPLNetworkHandle networkHandlePtr,
                             tSIRF_UINT8 *data, tSIRF_UINT16 size);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLWriteCPNetworkBytes(LPLNetworkHandle networkHandlePtr,
                               tSIRF_UINT8 *data, tSIRF_UINT16 size,
                               tSIRF_UINT8 type, tSIRF_UINT8 isLast); /* type = 0 for RRLP, 1 for RRC */

/*File I/O APIs */
/* Opens file of given filename and attributes (file openinng & access mode). */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLOpenFile(LPLFileHandler* fileHandlerPtr,
                    LPLFileConfig* configInfo,
                    tSIRF_INT32* lastErr);
/* closes file  */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLCloseFile(LPLFileHandler* fileHandlerPtr);
/* write binary data to file, prefixTimeStamp if true then one should add timestamp before logging  */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLWriteBinaryToFile(LPLFileHandler fileHandler,
                                 tSIRF_UINT8* data, tSIRF_UINT16 dataLen,
                                 LPLChar* debugStr, LPLBool prefixTimeStamp);
/* writes text to file,  prefixTimeStamp if true then one should add timestamp before logging  */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLWriteTextToFile(LPLFileHandler fileHandler, LPLChar* debugStr,
                               LPLBool prefixTimeStamp);
/* writes data in format provided */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
int LPLWriteToFileFormat(LPLFileHandler fileHandler,
                                const char *format, va_list ap);

/*writes complete block to file, does not perform any formatting on data */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLWriteToFile(LPLFileHandler fileHandler,
                                   tSIRF_UINT8 *data,
                                   tSIRF_UINT32 size,
                                   tSIRF_UINT32 *dataWritten,
                                         tSIRF_INT32* lastErr);

/*Read complete block from file, does not perform any formatting or parsing on data */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLReadFromFile(LPLFileHandler fileHandler,
                                  tSIRF_UINT8* inBuf,
                                  tSIRF_UINT32 inBufLen,
                                  tSIRF_UINT32* dataRead,
                                  tSIRF_INT32* lastErr);

/*Set the file pointer at the begining of file */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLSetFilePointer(LPLFileHandler fileHandler, tSIRF_UINT32 offset, tSIRF_INT32* lastErr);
                                 
/* Serial  APIs */
/*Creates Serial connection, Serial connection required info is passed in configInfo */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLCreateSerialConnection(LPLSerialHandle* serialHandlePtr,
                                         LPLSerialConfig* configInfo,
                                         tSIRF_INT32* lastErr);
/* destroys the serial connection */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLDestroySerialConnection(LPLSerialHandle* serialHandlePtr);
/* read bytes from serial conection */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLReadSerialBytes(LPLSerialHandle serialHandlePtr,
                                  tSIRF_UINT8* inBuf,
                                  tSIRF_UINT16 inBufLen,
                                  tSIRF_UINT32* dataRead,
                                  tSIRF_INT32* lastErr);
/*writes bytes to serial connection */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLWriteSerialBytes(LPLSerialHandle serialHandlePtr,
                                   tSIRF_UINT8 *data,
                                   tSIRF_UINT32 size,
                                   tSIRF_INT32* lastErr);

/*System APIs */
/*returns current time in seconds */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLUINT64 LPLGetCurrentSystemTime(LPLBool isSec);
/* returns current time in format specified - yet to be implemented */
/*   dd/mm/yyyy:hh:mm:ss */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLGetUTCTimeAndDate(LPLChar* dateAndTime, LPLUINT64 UTCInterval, tSIRF_UINT8 len);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLGetDateTime(LPL_TIME_STRUCT* dateAndTime, LPLBool isLocalTime);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool  LPLGetCurrentGpsTime(tSIRF_UINT16* weekno, tSIRF_UINT32* timeOfWeek);


/*Sleeps for given time period (in milliseconds) */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLSleep(tSIRF_UINT32 timeInMillis);

/*convert the input data of encoding type (inputFormat) to UNICODE and store the output in outputBuffer. */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLStringConvertToUnicode(tSIRF_UINT16 *outputBuffer, tSIRF_UINT8 outputBufLen,
                                     tSIRF_UINT8 *inputData,tSIRF_UINT8 inputDataSize,
                                     tSIRF_UINT8 inputFormat);

/*platform dependent memory allocation deallocation APIs */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void* LPLAllocate(tSIRF_UINT32 size);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLDeAllocate(tSIRF_UINT8 *buffer);

/*functions to be used by platform  */
#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void* LPLMalloc(int size);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLFree(void* ptr);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPL_STATUS LPLSetGPSState(LPLSerialHandle *serialHandlePtr, LPL_requestedGPSAction gpsPwReq);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPL_STATUS LPLGetGPSPowerState(LPLSerialHandle *serialHandlePtr, LPL_curGPSState *state);


#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPL_STATUS LPLInformIdlePeriod(tSIRF_UINT16 idleDurationInSec);


#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
const char* LPLGetAdaptationLayerVersion(void);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLCreateSemaphore(LPLSemHandle *semHdl, tSIRF_INT32 initCount, tSIRF_INT32 maxCount, LPLChar *name);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLReleaseSemaphore(LPLSemHandle semHdl);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLWaitForSemaphore(LPLSemHandle semHdl,int timePeriod);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLDestroySemaphore(LPLSemHandle *semHdl);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */

LPLBool LPLResumeSerialConnection(LPLSerialHandle* serialHandlePtr , LPLBTConfig *btconfig);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
LPLBool LPLSuspendSerialConnection(LPLSerialHandle* serialHandlePtr);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLExtractSerialPortNumber( char *serialPortName ,tSIRF_UINT32 *portNum);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */

void LPLSetAdaptationLayerDataCallBack( DataCallBack data);

/** Function is used to allow the main thread to wait for trigger message from either network module 
or serial module, it will be dummy function for the platform where all the three threads can run independently */

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLWaitForMessage(void);

#ifdef DLL
__declspec(dllexport)
#endif /* DLL */
void LPLConvertUTCToGPSTime(LPL_TIME_STRUCT *utcTime,tSIRF_UINT16 *weekno,tSIRF_UINT32 *timeOfWeek);


#ifdef __cplusplus
}
#endif

#endif
