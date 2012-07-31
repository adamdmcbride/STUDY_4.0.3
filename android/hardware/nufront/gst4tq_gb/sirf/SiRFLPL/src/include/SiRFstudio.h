/**
 * @addtogroup lpl3.0
 * @{
 */


/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2007-2011 by SiRF Technology, a CSR plc Company        *
 *                        Inc. All rights reserved.   
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.’s confidential and       *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.’s  express written            *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 *
 *  MODULE:         LPL SiRFstudio APIs
 *
 *  FILENAME:       SiRFstudio.h
 *
 *  DESCRIPTION:    This header file includes LPL SiRFStudio APIs.
 *
 ***************************************************************************
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File: //customs/customer/Marvell-U1/sirf/Software/sirf/SiRFLPL/src/include/SiRFstudio.h $
 *
 *  $DateTime: 2011/10/17 14:33:40 $
 *
 *  $Revision: #2 $
 *
 ***************************************************************************/

/**
* @file SiRFstudio.h
*  LPL API Header file.
*/

#ifndef __SiRFstudio_H__
#define __SiRFstudio_H__

#include "sirf_types.h"
#ifdef SIRF_AGPS
#include "sirf_errors.h"
#endif /*SIRF_AGPS*/

#define     MAX_NUMBER_OF_CONTEXT   12
#define     MAX_SESSIONS            5
   /** @def LPL_MAX_FQDN_LEN
    *  @brief MAX FQDN length as defined in SUPL1.0 */
#define     LPL_MAX_FQDN_LEN        (256)

/**
* The types for locationMethod
*/

/**
* The MS-based AGPS location method computes the location using the GPS on the MS side,
* with the assistance data provided by the server. The assistance data can include
* reference time, reference location, satellite ephemeris, almanac, etc.
*/
#define METHOD_MS_BASED_AGPS    0x00000001
/**
* The MS-Assisted AGPS location method computes the location on the server side,
* with the measurements from the GPS on the MS side. Assistance data may be provided
* by the server to assist client making the measurements.
*/
#define METHOD_MS_ASSISTED_AGPS 0x00000002
/**
* The autonomous GPS location method computes the location solely by GPS itself,
* without assistance from server side or other components.
*/
#define METHOD_AUTONOMOUS_GPS   0x00000004
/**
* Cell ID Location method uses cell towers' ID and its known location to compute a
* mobile handset's location.
*/
#define METHOD_CELL_ID          0x00000008
/**
* The Observed Time Difference (OTD) method of computing the location, e.g. the
* enhanced OTD in GSM.
*/
#define METHOD_OTD              0x00000010
/**
* Observed Time Difference of Arrival (OTDOA).
*/
#define METHOD_OTDOA            0x00000020
/**
* Advanced Forward Link Trilateration (AFLT).
* To determine location, the phone takes measurements of signals from nearby cellular
* base stations (towers) and reports the time/distance readings back to the network,
* which are then used to triangulate an approximate location of the handset.
*/
#define METHOD_AFLT             0x00000040
/**
* WIFI pisitioning computes a terminal's location using the wireless access point
* and its location.
*/
#define METHOD_WIFI             0x00000080

/**
* The types of NMEA sentences
*/
typedef enum
{
   /** GPGGA type of NMEA sentence */
   GPGGA = 1,
   /** GPGLL type of NMEA sentence */
   GPGLL = 4,
   /** GPGSA type of NMEA Sentence */
   GPGSA = 7,
   /** GPGSV type of NMEA sentence */
   GPGSV = 3,
   /** GPRMC type of NMEA sentence */
   GPRMC = 2,
   /** GPVTG type of NMEA sentence */
   GPVTG = 5,
   /** GPMSS type of NMEA sentence */
   GPMSS = 8,
   /** PSRFEPE type of NMEA sentence */
   PSRFEPE = 6
}   NMEA_MessagesType;

/**
 * The types of sessions
 * Note: Only NORMAL_SESSION is supported in this release
 *
 */
typedef enum
{
   /**
   This is the session type for a normal session (with lower priority)
   */
   NORMAL_SESSION,
   /**
   This is the session type for a emergency session (with the highest processing priority)
   */
   EMERGENCY_SESSION
}   eSessionPriority;

/**
* The version info for each LPL release
*/
typedef struct SUPL_version
{
   /**
   * Major version.
   */
   tSIRF_UINT8   maj;        /*Why MUST be 1 for ULP2.0??? */
   /**
   * Minor version.
   */
   tSIRF_UINT8   min;        /*MUST be 0 for 1.0 and 2.0 */
   /**
   * service indicator.
   */
   tSIRF_UINT8   servind;    /*MUST be 0 for 1.0 and 2.0 */
}   SUPL_version;

/**
* The types of Error Codes
*/
typedef enum
{
   /** Valid position */
   VALID_POSITION
   /** Used previously computed position */
   ,PREV_COMPUTED_POSITION

   /** An error has occured during OTA communication */
   ,OTA_ERROR
   /** An error has occured in GPS chipset */
   ,AGPS_ERROR
   /** The GPS chipset has obsolete almanac */
   ,OBSOLETE_ALMANAC

#if defined(LPL_SUPL2_0_0) || defined(LPL_SUPL1_5_0)
   /** The GPS chipset lost position during preiodic triggered report */
   ,GPS_LOST_POS
#endif /* LPL_SUPL2_0_0 or LPL_SUPL1_5_0 */
   ,AI3_ERROR
}   ErrorCodeType;

/**
 * The LPL_Coordinate structure contains WGS84 coordinate values,
 * including latitude, longitude, and altitude. The latitude and longitude
 * values are in decimal degrees and the altitude values are in meters. It was
 * called Coordinate, but is changed to the current name because of a conflict
 * with SUPL2.0 ASN.1 code. 
 */
typedef struct LPL_Coordinate
{
   /**
    * This indicates if the position is 2D (false) or 3D (true).
    */
   tSIRF_UINT8   position3D;
   /**
    * This is the value of latitude in degrees (as a floating point tSIRF_DOUBLE), in reference to the WGS84 reference ellipsoid (a positive value represents north of equator, a negative value represents south of equator)
    */
   tSIRF_DOUBLE  latitude;
   /**
    * This is the value of longitude in degrees (as a floating point tSIRF_DOUBLE), in reference to the WGS84 reference ellipsoid (a positive value represents east of the Greenwich Meridian, a negative value represents west of the Greenwich Meridian)
    */
   tSIRF_DOUBLE  longitude;
   /**
    * This is a signed value (in meters) representing the height of the user with respect to WGS84 reference ellipsoid. Valid only if position3D is true.
    */
   tSIRF_FLOAT   altitude;
} LPL_Coordinate;

/**
 * The GPSSatelliteInfo structure is used by the GPSLocationContext
 * to provide information related to a single GPS Satellite used in computing the
 * associated Location.
 * Note: azimuth and elevationAngle are not supported in this release
 */
typedef struct GPSSatelliteInfo
{
   /**
   * This is an integer number indicating the GPS satellite id. The range is from 1 to 32.
   */
   tSIRF_UINT8   satelliteId;
   /**
   * This is an integer number indicating the satellite's signal-to-noise ratio in dBHz. The range is from 0 to 90.
   */
   tSIRF_UINT8   signalStrength;
   /**
   * This is an integer number indicating the azimuth of the associated satellite in degress. The range is from 0 to 359.
   */
   tSIRF_UINT16  azimuth;
   /**
   * This is an integer number indicating the elevation angle of the associated satellite in degress. The range is from 0 to 90.
   */
   tSIRF_UINT8   elevationAngle;
}   GPSSatelliteInfo;

/**
 * The GPSLocationContext structure contains additional information about the GPS
 * Satellites used in computing the associated Location.
 */
typedef struct GPSLocationContext
{
   /**
   aidingTime in seconds is the time from SUPL-START or received SUPL-INIT to the last
   SUPL-POS message with aiding data. If there is no aiding, aidingTime = 0.
   */
   tSIRF_UINT16      aidingTime;
   /**
   ttff in seconds is the time from GPS_RESET to the first fix from GPS. If there is no
   fix from GPS, ttff = 0;
   */
   tSIRF_UINT16      ttff;
   /**
   msaPositionTime in seconds is the time from SUPL-START or SUPL-POSINIT to the SUPL-END
   with position. If not in MSA position method, msaPositionTime = 0;
   */
   tSIRF_UINT16      msaPositionTime;
   /**
   Because LPL processes GPS and SUPL messages in parallel, actualTTFF is no longer
   the sum of aidingTime, ttff, and msaPositionTime any more. Its computation follows:
   1. In MSA Mode:
      The time difference between when LPL getting SUPL-END with position or the first
      fix from GPS whichever is earlier, and when LPL sending GPS_REST or SUPL-START/
      SUPL-POSINIT whichever is earlier.
   2. In MSB Mode:
      The time difference between when LPL getting the first fix from GPS, and when LPL
      sending GPS_REST or SUPL-START/SUPL-POSINIT whichever is earlier.
   3. In Standalone or Autonomous Mode:
      The time difference between when LPL getting the first fix from GPS, and when LPL
      sending GPS_RESET.
   */
   tSIRF_UINT16      actualTTFF;
   /**
   This is the number of GPS Satellites used to calculate the associated Location. The range is from 0 to 12.
   */
   tSIRF_UINT8       numSatsUsed;
   /**
   *   This is the GPSSatelliteInfo object containing specific information about one of the Satellites used in the Location computation.
   *   Index of the Satellite for which to get information must be between 0 and the numSatsUsed.
   */
   GPSSatelliteInfo    gpsSatelliteInfo[MAX_NUMBER_OF_CONTEXT];
}   GPSLocationContext;

/* Choice tag constants */
#define T_LocationContext_GPS         1

#define T_LocationContext_CELL_ID     2
/**
 * The LocationContext structure contains additional information
 */
typedef struct LocationContext
{
   /**
   Type of location context
   */
   tSIRF_UINT8       t;
   /**
   Union of different location context types
   */
   union
   {
      /* t = 1 */
      /**
      GPS location context
      */
      GPSLocationContext*     gpsLocationContext;
   }   u;
}   LocationContext;

typedef enum ePositionState_
{
   PS_INIT_STATE       = 0
   ,PS_GOT_AIDING      = 1             /*don't ever change this since SiRFstudioDemo depends on it to display cyan color */
   ,PS_GOT_POSITION        = 2             /*recommend not to change this */
   ,PS_OVERALL_TIMEOUT = 3
   ,PS_OTA_ERROR       = 4

   /*GPS related errors */
   ,PS_GPS_ERR_NO_ENOUGH_SV                /*Not Enough satellites tracked in MESSAGE_AI3_RESPONSE */
   ,PS_GPS_ERR_NO_AIDING                   /*GPS Aiding data missing in MESSAGE_AI3_RESPONSE */
   ,PS_GPS_ERR_NEED_MORE_TIME_IN_AI3_RESP  /*Need more time in MESSAGE_AI3_RESPONSE */
   ,PS_GPS_NO_FIX_FULL_SEARCH              /*No fix available after full search in MESSAGE_AI3_RESPONSE */
   ,PS_GPS_ERR_POS_RESLT_ERROR             /*m->positionResultError = %d in MESSAGE_AI3_RESPONSE or other errors */
   ,PS_GPS_UNKNOWN_AI3_ERROR
   ,PS_GPS_MISSING_POSITION_RESULT_SECT

   /*MSA related errors */
#if defined(LPL_SUPL2_0_0) || defined(LPL_SUPL1_5_0)
   ,PS_LOST_POSITION                       /*lost position during triggered position report */
#endif/*LPL_SUPL2_0_0 or LPL_SUPL1_5_0 */
   ,PS_SESSION_END                         /*tell the application that the session is over after all pos are reported */

   /*Out of Heap Memory */
   ,PS_OUTOF_HEAP_MEMORY                   /*internal memory for LPL is out. */

   /*Other errors */
   ,PS_AI3_TIMEOUT                         /*timeout because GPS cannot compute position within max_req_time */

   ,PS_SUPL_TIMEOUT                        /*timeout because of LPL doesn't receive SUPL reply for a sent SUPL message */

   ,PS_CHIPSET_ERROR                       /* Wrong message received from chip set */

   ,PS_GOT_REF_LOC                         /* Cell ID reference Location */
   /*invalid state */
   ,PS_NOT_VALID                           /*this is an invalid state */


}   ePositionState;

/**
 * The Location structure contains position information expressed in
 * WGS84 coordinate frame, the uncertainty information associated with this
 * position, the time stamp of this location, and the speed and heading
 * information. It also contains the validity flag of the position, and the
 * context from which the position information is derived. 
 */
typedef struct Location
{
   /**
    * The property {@link LocationContext} contains more information about the assosicated location.
    */
   LocationContext     context;
   /**
    * "heading" is an integer that represents the heading of the users in degrees. 0 degree is the True North, and the angle increases towards the East.
    */
   tSIRF_UINT16              heading;
   /**
    * "horizontalVelocity" is float that represents the horizontal velocity of the user in kilometers per hour.
    */
   tSIRF_FLOAT              horizontalVelocity;
   /**
    * "horizontalVelocityUncert" represents the horizontal velocity uncertainty in meters per second.
    */
   tSIRF_UINT16              horizontalVelocityUncert;
   /**
    * "horUncert" is an interger value (represents error in meters) that represents the standard deviation of a 2-D error ellipse.
    */
   tSIRF_UINT16              horUncert;
   /**
    * "locationMethod" is a bitwise combination of different location methods.
      For example, a hybrid position using both METHOD_MS_BASED_AGPS and METHOD_CELL_ID method will have a value of 9 (0x00000001 & 0x00000008).
    */
   tSIRF_UINT32        locationMethod;
   /**
    * This is the position of the users generated by the location engine.
    */
   LPL_Coordinate      position;
   /**
    * This variable indicates if the associated position is valid (0) or not (any other value representing the error code)
    */
   tSIRF_UINT8               status;
   /**
    * This is the time stamp when the associated position is generated. Time is provided for GMT-0.
    */
   tSIRF_UINT64           timeStamp;
   /**
    * "verticalVelocity" is a tSIRF_FLOAT that represents the vertical velocity of the user in kilometers per hour.
    */
   tSIRF_FLOAT               verticalVelocity;
   /**
    * "verticalVelocityUncert" represents the vertical velocity uncertainty in meters per second.
    */
   tSIRF_UINT16              verticalVelocityUncert;
   /**
    * "verUncert" is an interger value (represents error in meters) that represents the standard deviation of a 1-D error curve
    */
   tSIRF_UINT16              verticalUncert;
   /**
    * This indicates the processing state of the engine. 0 = position is not available, 1 = aiding has been received, 2 = position is available
    */
   ePositionState      positionState;
   /**
    * This indicates the position ID
    */
   tSIRF_UINT16              positionID;
   /**
   * This indicates major Horizontal Error
   */
   tSIRF_UINT8               uncertaintySemiMajor;
   /**
   * This indicates minor Horizontal Error
   */
   tSIRF_UINT8               uncertaintySemiMinor;
   /**
   * This indicates angle
   */
   tSIRF_UINT8               orientationMajorAxis;

#if defined(LPL_SUPL2_0_0) || defined(LPL_SUPL1_5_0)
   tSIRF_UINT32        numOfPrdRptFixes;    /*for application to know that how many fixes are requested */
   tSIRF_BOOL             isPrdRpt;
#endif /*LPL_SUPL2_0_0 or LPL_SUPL2_0_0*/
}   Location;

/**
 * The AGPS_QOS structure is for applications to specify the quality of
 * services they desire.
 */
typedef struct AGPS_QOS
{
   /** This is the requested horizontal error in meters.
    *  The location engine shall try to provide a position with
    *  horizontal error less than the specified value in 95% of the cases.
    */
   tSIRF_UINT32 horizontalAccuracy;
   /** This is the requested vertical error in meters.
    *  The location engine shall try to provide a position with
    *  vertical error less than the specified value in 95% of the cases.
    */
   tSIRF_UINT32 verticalAccuracy;
   /** This is the requested response time for the first position fix,
    *  in seconds. The location engine shall try to provide a position
    *  with the specified amount of time
    */
   tSIRF_UINT8        maxResponseTime;

   /** This parameter sets a maximum limit (in seconds) on age of the cached
    *  position, if it is to be used for the current session.
    */
   tSIRF_UINT16       maxLocationAge;
}   AGPS_QOS;

/**
 * The GeoSession structure encapsulates a GeoServices Session.
 * An instance of GeoSession handles all events (reqests/responses of locations) for one GeoSession.
 * Multiple simultaneous geosession are supported. To Start using the GeoServices functionalities, an
 * application must first obtain an instance of GeoSession from the  GM_openSession() method.
 * When done with the session, the application must call the GM_closeSession() method.
 * Note: One session at a time only is supported in this release 
 */
typedef struct GeoSession
{
   tSIRF_UINT32 id;
   tSIRF_UINT8  type;
}   GeoSession;

/**
 * The eGPSHW_UNIT enum shows type of GPS Uint used for LPL.
 */
typedef enum eGPSHW_UNIT_
{
   EUART_PORT,         /*lp & lt */
   EBLUETOOTH_PORT,    /*SiRF GPS chip on other device through bluetooth */
   ETCPIP,             /*SiRF GPS chip on other device through TCP/IP */
   ETRACKER,           /*for 3wt */
}   eGPSHW_UNIT;

#define MAX_COMM_PORT_LEN  (20)
#define MAX_BT_PORT_LEN    (50)
/**
 * The LPL_serialConfig structure encapsulates serial port initialization data for LPL.
 */
typedef struct LPL_serialConfig
{
   tSIRF_BOOL needToOpenComport;          /*If the app opens the comport itself, set it to 0 */
   tSIRF_CHAR m_SerialPort[MAX_PORT_NUM_STRING_LENGTH];             /*serial port name */
   tSIRF_INT32  m_BaudRate;                  /*serial port baude rate */

   /* the virtual comport mechanism is phased out, but keep here in case it's used */
   tSIRF_BOOL needToOpenVirtualPort;      /*If the app opens the virtual comport itself, set it to 0 */
   tSIRF_CHAR m_NMEASerialPort[MAX_COMM_PORT_LEN];         /*virtual port name */
   tSIRF_BOOL bUseNMEA;                   /*is virtual port used? */
}   LPL_serialConfig;

/**
 * The LPL_bluetoothConfig  structure encapsulates BlueTooth Address and port initialization data for LPL.
 */
typedef struct LPL_bluetoothConfig
{
   tSIRF_UINT16 m_BTAddress[MAX_BT_PORT_LEN];             /*  it specifies the BT Address in hexadecimal bigendian format string */
   tSIRF_UINT16 BTPort;                      /*  it specifies the BT Port number  */

}   LPL_bluetoothConfig;

/**
 * The LPL_TCPIPConfig  structure encapsulates BlueTooth Address and port initialization data for LPL.
 */
typedef struct LPL_TCPIPConfig
{
   tSIRF_INT8   serverAddress[LPL_MAX_FQDN_LEN];            /*  it specifies the server address */
   tSIRF_UINT16 serverPort;                   /*  it specifies the server port */
   tSIRF_INT8   bindServerIP[LPL_MAX_FQDN_LEN];
   tSIRF_UINT16 bindServerPort;
   tSIRF_BOOL   isBind;

}   LPL_TCPIPConfig;

/**
 * The LPL_serialConfig structure encapsulates serial port initialization data for LPL.
 */
typedef struct LPL_trackerCfg
{
   tSIRF_CHAR      m_SerialPort[MAX_PORT_NUM_STRING_LENGTH];                 /*serial port name */
   tSIRF_INT32     m_BaudRate;                      /*serial port baude rate */
   tSIRF_UINT32    gpsStartMode;                    /*for 3tw only */
   tSIRF_UINT32    gpsClockOffset;                  /*for 3tw only */
   tSIRF_CONFIG    gsd4tConfig;                     /* for 4t */
}   LPL_trackerCfg;

typedef struct LPL_GPS_UNIT_
{
   eGPSHW_UNIT configType;
   union
   {
      LPL_serialConfig    serialConfig;     /*EUART_PORT */
      LPL_bluetoothConfig btConfig;      /*EBLUETOOTH_PORT */
      LPL_TCPIPConfig     tcpipConfig;   /*ETCPIP */
      LPL_trackerCfg      trackerCfg;     /*ETRACKER */
   } DeviceConfig;

}  LPL_GPS_UNIT;
/**
 * The LPL_memConfig structure encapsulates heap memory initialization data for LPL.
 */
typedef struct LPL_memConfig
{
   tSIRF_UINT32  memSize;    /*size of heap memory for LPL */
   tSIRF_INT8   *pMem;      /*point to the requested memory location. */
}   LPL_memConfig;

typedef enum
{
   NO_GPS_POWER_CONTROL,       /* LPL cannot control GPS power */
   HAS_GPS_POWER_CONTROL       /* LPL can control GPS power */
} LPL_GPSPwrCtrlCap;           /* see LPL_powerControl data structure */

typedef enum
{
   DISABLE_CGPSC,              /* disable Cooperative GPS power save and control */
   ENABLE_CGPSC                /* enable Cooperative GPS power save and control if allowed */
} LPL_CGPSCCtrl;               /* see LPL_powerControl data structure */

typedef struct LPL_powerControl
{
   LPL_GPSPwrCtrlCap   gpsPwrCtrlCap;            /*What power control capability does platform has */
   LPL_CGPSCCtrl       cgpsCtrl;                     /*disable or enalbe power control */
}   LPL_powerControl;

/**
 * The GM_InitData structure encapsulates the OS resource data for all sessions after LPL starts running.
 * Currently there are two resoureces that get reused for all sessions:
 * SiRF Managed Heap Memory. The real memory could be provided by the application interfacing with LPL. By default
 * SiRF Heap Manager will allocate a pool of memory.
 * Serial Ports: Since GPS serial ports are dedicated links, there is no need to open and close it for each session.
 * Note: More Resources could be added. But the interface itself should be changed even binary won't be compatible.
 */
typedef struct GM_InitData
{
   LPL_GPS_UNIT        gpsDevice;
   LPL_memConfig       lpl_mem;
   LPL_powerControl    lplPowerControl;
}   GM_InitData;

#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/

   /**
   * This function is called by the GeoSession object whenever a new position is available, as long as it meets the requested QOS
   * parameters and the reporting interval requested.
   * @param pLoc A pointer to the new location object being reported to the application.
   */
   typedef tSIRF_VOID (*LocationListener) (Location* pLoc);

   /**
   * This function is called by the GeoSession object whenever a new NMEA sentence is available.
   * @param type A byte indicating the type of NMEA sentence, as defined by the constants in the NMEAMessagesType enum.
   * @param message A String object containing the NMEA sentence.
   * @param messageLength An integer representing the length of the NMEA sentence
   */
   typedef tSIRF_VOID (*NMEA_Listener) (tSIRF_UINT8 type, tSIRF_UINT8* message, tSIRF_UINT32 messageLength);

   /* GeoManager functions */
   /**
    * The GeoManager is the control entity for all GeoServices clients in a platform.
    * This controls the number of GeoSessions which can be allowed at any one time.
    * In order to start a new GeoServices session, a client application must first request
    * a session object from the GeoManager.
    * Note: One session at a time only is supported in this release 
    */

   /**
   * GM_closeSession() closes an open GeoSession and releases the resources associated with it.
   * @param pSession a pointer to a valid GeoSession object to be closed
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_closeSession(GeoSession* pSession);

   /**
   * GM_getMaxNumberOfSessions() gets the maximum number of opened sessions allowed at any given time.
   * @return tSIRF_UINT8. Maximum number of opened sessions allowed.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_UINT8       GM_getMaxNumberOfSessions(tSIRF_VOID);

   /**
   * GM_getSessionCount() gets the number of sessions which are currently open.
   * @return tSIRF_UINT8. Number of sessions opened.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_UINT8       GM_getSessionCount(tSIRF_VOID);

   /**
   * GM_openSession() creats a new geosession if the maximum number of sessions has not been reached.
   * @param sessionType This parameter should be set to the desired session type (NORMAL_SESSION or EMERGENCY_SESSION).
   * @return pGeoSession. A pointer to the newly created geosession is returned.
   * <p> Note: One session at a time only is supported in this release </p>
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   GeoSession* GM_openSession(tSIRF_UINT8 sessionType);

   /**
   * GM_setAgpsServer is used to configure which SiRFStudio server the GeoServices platform should use. This
   * applies to all sessions in the platform.
   * @param ipAddress The IP Address of the server
   * @param portNumber The TCP Port of the server
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setAgpsServer(tSIRF_CHAR* ipAddress, tSIRF_UINT16 portNumber);

   /**
   * GM_setApproximateLocation is used to configure the approximate position (if available) to be used by the platform to imporove the AGPS performance.
   * Providing an approximate position improves the AGPS performance, but it is not required.
   * @param approximatePositionValid tSIRF_BOOL indicating if the approximate position parameters are valid. If set to false, the approximate position will not be used.
   * @param latitude The Latitude for the Approximate Position.
   * @param longitude The Longitude for the Approximate Position.
   * @param altitude The Altitude for the Approximate Position.
   * @param estimatedVerticalError The Estimated Vertical Error for the Approximate Position.
   * @param estimatedHorizontalError The Estimated Horizontal Error for the Approximate Position.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GM_setApproximateLocation(tSIRF_UINT8 approximatePositionValid,
                                               tSIRF_DOUBLE latitude,
                                               tSIRF_DOUBLE longitude,
                                               tSIRF_INT16 altitude,
                                               tSIRF_INT16 estimatedVerticalError,
                                               tSIRF_UINT32 estimatedHorizontalError);

  /**
   * GM_setReferenceTime is used to configure the Reference UTC Time (if available) to be used by the platform to imporove the AGPS performance.
   * Providing an reference time improves the AGPS performance, but it is optional.
   * @param utcTime UTC Time in (miliseconds) 
   * @param timestamp System Reference Time (miliseconds)
   * @param uncertainty uncertainty of time accuracy (miliseconds)
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID GM_setReferenceTime( tSIRF_UINT64 utcTime, tSIRF_UINT64 timestamp, tSIRF_INT32 uncertainty);


   /* GeoSession functions */
   /**
   * GS_getId Gets the unique Id of this GeoSession.
   *
   * @param pGS Pointer to the GeoSession object
   * @return tSIRF_UINT32 The unique Id of this geosession.
    */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_UINT32      GS_getId(GeoSession* pGS);

   /**
   * GS_getType Gets the type of this GeoSession.
   *
   * @param pGS Pointer to the GeoSession object
   * @return tSIRF_UINT8 The type of this geosession.
    */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_UINT8       GS_getType(GeoSession* pGS);

   /**
   * setLocationListener() Specify a {@link LocationListener} to get continuous locations from this GeoSession. Only one
   * LocationListener is allowed per session, and setting a new listener overrides the previous one.
   *
   * @param pGS Pointer to the GeoSession object
   * @param pQos Pointer to the requested quality of position for the Location. If NULL, it has default values
   * @param numFixes This is the number of locations desired by the application. If it is set to 0, continuous locations shall be output until the Listener is stopped.
   * @param timeBetweenFixes This is the number of seconds the application would like to see between consecutive locations.
   * @param listener pointer to a {@link LocationListener} object.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_UINT8       GS_setLocationListener(GeoSession* pGS, AGPS_QOS* pQos,
         tSIRF_UINT8 numFixes, tSIRF_UINT8 timeBetweenFixes,
         LocationListener listener);

   /**
   * setNMEAListner() Specify a NMEAListener to get continuous NMEA stream from this GeoSession.
   * Only one NMEAListener is allowed per session, and setting a new listener overrides the previous one.
   *
   * @param pGS Pointer to the GeoSession object
   * @param listener pointer to a NMEA_Listener object.
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID        GS_setNMEAListener(GeoSession* pGS, NMEA_Listener listener);

   /**
   * GM_Init() Initialize serial port and heap memory for all sessions before LPL runs
   *
   * @param pInitData Pointer to GM initialzation data that includes seiral port and heap memory info
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_UINT8   GM_Init(GM_InitData *pInitData);

   /**
   * GM_Release() Is called to finalize LPL
   *
   */
#ifdef DLL
   DLL_EXPORT
#endif /* DLL */
   tSIRF_VOID  GM_Release(tSIRF_VOID);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif      /* __SiRFstudio_H__ */

/**
 * @}
 * End of file.
 */
