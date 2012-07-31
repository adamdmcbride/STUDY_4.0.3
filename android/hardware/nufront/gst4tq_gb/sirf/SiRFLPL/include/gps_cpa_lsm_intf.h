/**
 * @addtogroup CP agent shared data types
 * @{
 */

/*
 *                   SiRF Technology, Inc. GPS Software
 *
 *    Copyright (c) 2006-2008 by SiRF Technology, Inc.  All rights reserved.
 *
 *    This Software is protected by United States copyright laws and
 *    international treaties.  You may not reverse engineer, decompile
 *    or disassemble this Software.
 *
 *    WARNING:
 *    This Software contains SiRF Technology Inc.�s confidential and
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this
 *    Software without SiRF Technology, Inc.�s  express written
 *    permission.   Use of any portion of the contents of this Software
 *    is subject to and restricted by your signed written agreement with
 *    SiRF Technology, Inc.
 *
 *
 *  Keywords for Perforce.  Do not modify.
 *
 *  $File:
 *
 *  $DateTime:
 *
 *  $Revision: #1 $
 */

/**
 * @file   GPS_CPA_LSM_INTF.H
 *
 * @brief
 */

#ifndef GPS_CPA_LSM_INTF_H
#define GPS_CPA_LSM_INTF_H

#include "sirf_types.h"

/*--------------------------------------------------------------------
                          LITERAL CONSTANTS
--------------------------------------------------------------------*/

/*--------------------------------------------------------------------
                                 TYPES
--------------------------------------------------------------------*/

/*----------------------------------------------------------
Reference Location Assistance Data
----------------------------------------------------------*/
typedef struct
{
    tSIRF_UINT8  latitude_sign;
    tSIRF_UINT32 latitude;
    tSIRF_INT32  longitude;
}EllipsoidPoint;

typedef struct
{
    tSIRF_UINT8  latitude_sign;
    tSIRF_UINT32 latitude;
    tSIRF_INT32  longitude;
    tSIRF_UINT8  altitude_direction;
    tSIRF_UINT16 altitude;
}EllipsoidPointAltitude;

typedef struct
{
    tSIRF_UINT8  latitude_sign;
    tSIRF_UINT32 latitude;
    tSIRF_INT32  longitude;
    tSIRF_UINT8  altitude_direction;
    tSIRF_UINT32 altitude;
    tSIRF_UINT8  uncert_semi_major;
    tSIRF_UINT8  uncert_semi_minor;
    //  -- Actual value orientationMajorAxis = IE value * 2
    tSIRF_INT8   orient_major;
    tSIRF_UINT8  confidence;
    tSIRF_UINT8  uncert_alt;
}EllipsoidPointAltitudeUncertEllipsoid;

typedef struct
{
    tSIRF_UINT8  latitude_sign;
    tSIRF_UINT32 latitude;
    tSIRF_INT32  longitude;
    tSIRF_UINT8  uncert_circle;
}EllipsoidPointUncertCircle;

typedef struct
{
    tSIRF_UINT8  latitude_sign;
    tSIRF_UINT32 latitude;
    tSIRF_INT32  longitude;
    tSIRF_UINT8  uncert_semi_major;
    tSIRF_UINT8  uncert_semi_minor;
    //  -- Actual value orientationMajorAxis = IE value * 2
    tSIRF_INT8   orient_major;
    tSIRF_UINT8  confidence;
}EllipsoidPointUncertEllipse;

typedef enum
{
    EllipsoidPointType = 0,
    EllipsoidPointUncertCircleType = 1,
    EllipsoidPointUncertEllipseType = 3,
    EllipsoidPointAltitudeType = 8,
    EllipsoidPointAltitudeUncertEllipsoidType = 9,
}ShapeType;

typedef struct
{
    ShapeType     shapeType;
    union
    {
        EllipsoidPoint                        ellipsoidPoint;
        EllipsoidPointUncertCircle            ellipsoidPointUncertCircle;
        EllipsoidPointUncertEllipse           ellipsoidPointUncertEllipse;
        EllipsoidPointAltitude                ellipsoidPointAltitude;        
        EllipsoidPointAltitudeUncertEllipsoid ellipsoidPointAltitudeUncertEllipsoid;        
    }shape;
}GAD_PositionEstimate;

typedef enum
{
    HorizontalVelocityType = 0,
    HorizontalWithVerticalVelocityType = 1,
    HorizontalVelocityWithUncertaintyType = 8,
    HorizontalWithVerticalVelocityAndUncertaintyType = 9,
}VelocityType;

typedef struct
{
   tSIRF_UINT16 bearing;
   tSIRF_UINT16 horizontalSpeed;
}HorizontalVelocity;

typedef struct
{
   /* 0 - upward, 1 - downward */
   tSIRF_UINT8  verticalSpeedDirection;
   tSIRF_UINT16 bearing;
   tSIRF_UINT16 horizontalSpeed;
   tSIRF_UINT8  verticalSpeed;
}HorizontalWithVerticalVelocity;

typedef struct
{
   tSIRF_UINT16 bearing;
   tSIRF_UINT16 horizontalSpeed;
   tSIRF_UINT8  horizontalSpeedUncertainty;
}HorizontalVelocityWithUncertainty;

typedef struct
{
   /* 0 - upward, 1 - downward */
   tSIRF_UINT8  verticalSpeedDirection;
   tSIRF_UINT16 bearing;
   tSIRF_UINT16 horizontalSpeed;
   tSIRF_UINT8  verticalSpeed;
   tSIRF_UINT8  horizontalUncertaintySpeed;
   tSIRF_UINT8  verticalUncertaintySpeed;
}HorizontalWithVerticalVelocityAndUncertainty;

typedef struct
{
   VelocityType velocityType;
   union
   {
      HorizontalVelocity                           horizontalVelocity;
      HorizontalWithVerticalVelocity               horizontalWithVeriticalVelocity;
      HorizontalVelocityWithUncertainty            horizontalVelocityWithUncertainty;
      HorizontalWithVerticalVelocityAndUncertainty horizontalWithVeriticalVelocityAndUncertainty;
   }velocity;
}GAD_VelocityEstimate;

typedef struct
{
    EllipsoidPointAltitudeUncertEllipsoid ellipsoidPointAltitudeUncertEllipsoid;
}ReferenceLocation;

/*----------------------------------------------------------
Reference Time Assistance Data
----------------------------------------------------------*/
typedef struct
{
   tSIRF_UINT32  gps_tow;
   tSIRF_UINT16  gps_week;
}GPS_Time;

typedef struct
{
    tSIRF_UINT8   satID;
    tSIRF_UINT16  tlm_word;
    tSIRF_BOOL    anti_spoof;
    tSIRF_BOOL    alert;
    tSIRF_UINT8   tlm_reserved;
}GPS_TOW_Assist;

typedef struct
{
    tSIRF_BOOL      tow_assist_present;
    GPS_Time        gps_time;
    tSIRF_UINT8     num_of_sat;
    GPS_TOW_Assist  tow_assist[16];
}GPS_ReferenceTime;

/*----------------------------------------------------------
Navigation Model Assistance Data
----------------------------------------------------------*/
typedef struct
{
    tSIRF_UINT8   codeOnL2;
    tSIRF_UINT8   ura_ind;
    tSIRF_UINT8   svhealth;
    tSIRF_UINT8   iode;
    tSIRF_UINT8   L2Pflag;
    tSIRF_UINT32  esr1;
    tSIRF_UINT32  esr2;
    tSIRF_UINT32  esr3;
    tSIRF_UINT16  esr4;
    tSIRF_INT8    t_gd;
    tSIRF_UINT16  toc;
    tSIRF_INT8    af2;
    tSIRF_INT16   af1;
    tSIRF_INT32   af0;
    tSIRF_INT16   c_rs;
    tSIRF_INT16   delta_n;
    tSIRF_INT32   m0;
    tSIRF_INT16   c_uc;
    tSIRF_UINT32  eccentricity;
    tSIRF_INT16   c_us;
    tSIRF_UINT32  a_sqrt;
    tSIRF_UINT16  toe;
    tSIRF_UINT8   fit_flag;
    tSIRF_UINT8   aoda;
    tSIRF_INT16   c_ic;
    tSIRF_INT32   omega_0;
    tSIRF_INT16   c_is;
    tSIRF_INT32   angle_inclination;
    tSIRF_INT16   c_rc;
    tSIRF_INT32   omega;
    tSIRF_INT32   omegadot;
    tSIRF_INT16   idot;
} EphemeriesInformationPerSatellite;

typedef enum
{
    SAT_STS_NS_NN_U,
    SAT_STS_ES_NN_U,
    SAT_STS_ES_SN,
    SAT_STS_NS_NN,
    SAT_STS_REVD,
    SAT_STS_INVLD
}SatelliteStatus;

typedef struct
{
    tSIRF_UINT8                        satID;
    SatelliteStatus                    satelliteStatus;
    EphemeriesInformationPerSatellite  ephParam;
} EphemerisDataPerSatellite;

typedef struct
{
   tSIRF_UINT8                num_of_eph;
   EphemerisDataPerSatellite  eph[16];
	//EphemerisDataPerSatellite  eph[1]; // this is changed after queue was overflowing
} GPS_NavigationModel;  //EphemerisData;

/*----------------------------------------------------------
Ionospheric Model Assistance Data
----------------------------------------------------------*/
typedef struct
{
    tSIRF_INT8  alpha0;
    tSIRF_INT8  alpha1;
    tSIRF_INT8  alpha2;
    tSIRF_INT8  alpha3;
    tSIRF_INT8  beta0;
    tSIRF_INT8  beta1;
    tSIRF_INT8  beta2;
    tSIRF_INT8  beta3;
}GPS_IonosphericModel;

/*----------------------------------------------------------
UTC Model Assistance Data
----------------------------------------------------------*/
typedef struct
{
    tSIRF_INT32  a0;
    tSIRF_INT32  a1;
    tSIRF_INT8   deltaTls;
    tSIRF_UINT8  t_ot;
    tSIRF_UINT8  wn_t;
    tSIRF_UINT8  wn_lsf;
    tSIRF_INT8   dn;
    tSIRF_INT8   deltaT_lsf;
} GPS_UTC_Model;

/*----------------------------------------------------------
Almanac Assistance Data
----------------------------------------------------------*/
typedef struct
{
    tSIRF_UINT8   dataID;  /* What is dataID? What is the purpose of this field? */
    tSIRF_UINT8   satID;
    tSIRF_UINT16  eccentricity;
    tSIRF_UINT8   toa;
    tSIRF_INT16   ksii;
    tSIRF_INT16   omegadot;
    tSIRF_UINT8   svhealth;
    tSIRF_UINT32  a_sqrt;
    tSIRF_INT32   omega_0;
    tSIRF_INT32   omega;
    tSIRF_INT32   m0;
    tSIRF_INT16   af0;
    tSIRF_INT16   af1;
}AlmanacElement;

typedef struct
{
    tSIRF_UINT16    wna;
    tSIRF_UINT8     num_of_almc;
    AlmanacElement  almanac[32];
} GPS_Almanac;  //almanacData;

/*----------------------------------------------------------
Acquisition Assistance Data
----------------------------------------------------------*/
typedef struct
{
    struct
    {
        tSIRF_BOOL  dopl_extra_present;
        tSIRF_BOOL  az_el_present;
    } m;
    tSIRF_UINT8  satID;
    tSIRF_INT16  doppler0;
    struct
    {
        tSIRF_UINT8  doppler1;
        tSIRF_UINT8  dopplerUncertainty;
    }dopl_extra;
    tSIRF_INT16  svCodePhase;
    tSIRF_UINT8  svCodePhaseInt;
    tSIRF_UINT8  gpsBitNum;
    tSIRF_INT16  codePhaseUncertainty;
    struct
    {
        tSIRF_INT16  azimuth;
        tSIRF_UINT8  elevation;
    }az_el;
} AcqAssistElement;

typedef struct
{
    tSIRF_INT32       tow_msec;
    tSIRF_UINT8       num_of_sat;
    AcqAssistElement  acq[16];
} GPS_AcquisitionAssistance;

/*----------------------------------------------------------
GPS RT Integrity Assistance Data
----------------------------------------------------------*/
typedef struct
{
    tSIRF_UINT8  satID;
}IntegrityElement;

typedef struct
{
    tSIRF_UINT8       num_of_sat;
    IntegrityElement  realtime_integrity[16]; // This is defined for 64 in RRC but 16 in RRLP
}GPS_RealTimeIntegrity;

/*----------------------------------------------------------
Request for additional assistance (GPS_assist_req)
----------------------------------------------------------*/
typedef struct
{
    tSIRF_UINT8  satID;
    tSIRF_UINT8  ioDE;
}SatelliteData;

typedef struct
{
   struct
   {
      tSIRF_BOOL satelliteInfo_present;
   } m;
   tSIRF_UINT16    gpsWeek;
   tSIRF_UINT8     gpsToe;
   tSIRF_UINT8     toeLimit;
   tSIRF_UINT8     num_of_sat;
   SatelliteData   satInfo[16];
} GPS_NavModelAddlDataReq;

typedef struct
{
   struct
   {
       tSIRF_BOOL  navigationModelDataPresent;
   } m;
   tSIRF_BOOL        almanacRequested;
   tSIRF_BOOL        utcModelRequested;
   tSIRF_BOOL        ionosphericModelRequested;
   tSIRF_BOOL        dgpsCorrectionsRequested;
   tSIRF_BOOL        referenceLocationRequested;
   tSIRF_BOOL        referenceTimeRequested;
   tSIRF_BOOL        acquisitionAssistanceRequested;
   tSIRF_BOOL        realTimeIntegrityRequested;
   tSIRF_BOOL        navigationModelRequested;
   GPS_NavModelAddlDataReq  navigationModelData;
} GPS_AdditionalAssistanceDataRequest;

typedef enum
{
    undefined_error,
    not_enough_gps_satellites,
    gps_assist_data_missing
}PosErrorReason;

typedef struct
{
    tSIRF_BOOL      assist_req_present;
    PosErrorReason  reason;
    GPS_AdditionalAssistanceDataRequest  assist_req;
}GPS_pos_error;

/*----------------------------------------------------------
GPS location result for MSB (location)
----------------------------------------------------------*/
typedef struct
{
    tSIRF_UINT8   valid;
    tSIRF_UINT8   velocity_estimate_present;
    GPS_Time      time_of_fix;
    GAD_PositionEstimate position_estimate;
    GAD_VelocityEstimate velocity_estimate;
} PositionEstimateInfo;

/*----------------------------------------------------------
GPS measurement results for MSA (GPS_meas)
----------------------------------------------------------*/
typedef enum
{
    GPS_MULTIPATH_NOT_MEAS,
    GPS_MULTIPATH_LOW,
    GPS_MULTIPATH_HIGH,
    GPS_MULTIPATH_INVLD
}multipath_indctr;

typedef struct
{
    tSIRF_UINT8       PRN;
    tSIRF_UINT8       c_no;
    tSIRF_INT16       svDoppler;
    tSIRF_UINT16      svCodePhaseWholeChips;
    tSIRF_UINT16      svCodePhaseFractionalChips;
    multipath_indctr  multipathIndicator;
    tSIRF_UINT8       pseudorangeRMSError;
}GPS_MeasurementParam;

typedef struct
{
    tSIRF_UINT8   valid;
    GPS_Time      time_of_fix;
    tSIRF_UINT8   num_of_sat;
    GPS_MeasurementParam   measurements[16];   /* max 16 instances */
} GPS_MeasurementResults;

/*----------------------------------------------------------
Position Measurement Commands (pos_meas)
----------------------------------------------------------*/
typedef  enum
{
    METHOD_TYPE_MSA,           // MS assisted
    METHOD_TYPE_MSB,           // MS based
    METHOD_TYPE_MSB_PREF,      // MS based is preferred, but MS assisted is allowed
    METHOD_TYPE_MSA_PREF,      // MS assisted is preferred, but MS based is allowed

    METHOD_TYPE_INVLD
}meas_method_type;

typedef enum
{
    RRLP_POS_METHOD_EOTD,
    RRLP_POS_METHOD_GPS,
    RRLP_POS_METHOD_EOTD_OR_GPS,

    RRC_POS_METHOD_OTDOA,
    RRC_POS_METHOD_GPS,
    RRC_POS_METHOD_OTDOA_OR_GPS,
    RRC_POS_METHOD_CELL_ID,

    POS_METHOD_INVLD
}pos_method_type;

typedef struct
{
    meas_method_type  method_type;        // RRLP/RRC: indicated whether MSA or MSB version is allowed/required
    pos_method_type   pos_method;         // RRLP/RRC: indicated which location method(s) should be used
    tSIRF_UINT8       response_time;      // RRLP: 0-7 (response time is 2^N seconds)
    tSIRF_BOOL        mult_sets;          // RRLP: indicated whether MS is requested to send multiple sets
    tSIRF_BOOL        hor_acc_present;    // flag indicating whether accuracy is present or not
    tSIRF_UINT16      hor_acc;            // RRLP: 0-127 (need to convert to meters by 10*(1.1^k-1) )
    tSIRF_BOOL        vert_acc_present;
    tSIRF_UINT8       vert_acc;
}GPS_start_meas_cmnd_rrlp;

typedef struct
{
    tSIRF_UINT8   reporting_amount;   // RRC: 1,2,4,6,16,32,64,255(infinite)
    tSIRF_UINT16  reporting_interval; // RRC: 250,500,1000,2000,3000,4000,6000,8000,12000,16000,20000,24000,28000,32000,64000 (milliseconds)
}Periodic_reporting_criteria;

typedef struct
{
    tSIRF_BOOL  Notused;  // Not supported in current release
}Event_reporting_criteria;

typedef enum
{
    periodic_reporting,
    event_reporting,
    no_reporting
}Reporting_Criteria_Type;

typedef struct
{
    tSIRF_UINT8 response_time_present;
    tSIRF_UINT8 response_time;
    Reporting_Criteria_Type reporting_criteria_type;
    union
    {
        Event_reporting_criteria  event_reporting_criteria;
        Periodic_reporting_criteria  periodic_reporting_criteria;
    }reporting_criteria;
    struct
    {
        meas_method_type  method_type;        // RRLP/RRC: indicated whether MSA or MSB version is allowed/required
        pos_method_type   pos_method;         // RRLP/RRC: indicated which location method(s) should be used
        tSIRF_BOOL        hor_acc_present;    // indicates whether horizonal accuracy is present or not
        tSIRF_UINT8       hor_acc;            // RRC:  0-127 (need to convert to meters by 10*(1.1^k-1) )
        tSIRF_BOOL        vert_acc_present;    // indicates whether vertical accuracy is present or not
        tSIRF_UINT8       vert_acc;           // RRC:  0-127 (need to convert to meters by 45*(1.025^k-1) )
        tSIRF_BOOL        gps_timing_of_cell_wanted; // Not used for now
        tSIRF_BOOL        addl_assist_data_request;  // Not used for now
    }reporting_quantity;

}GPS_start_meas_cmnd_rrc;

/*typedef enum
{
    RRLP_measurement = 8,
    RRC_measurement  = 9
}Pos_Measure_Type;*/



typedef enum
{
    GPS_AGPS_INPUT_ASSIST_DATA,     // input assistance data
    GPS_AGPS_START_POSN_MEAS,       // start position measurement
    GPS_AGPS_ASSIST_DATA_RESET,     // assistance data reset, not used ( call lsm_start with COLD reset type).
    GPS_AGPS_ASSIST_DATA_DELIVERED, // assistance data delivered
    GPS_AGPS_MEAS_ABORT             // Not sent to LSM, handled by CP Agent
}LSM_Input_Msg_Type;


/*----------------------------------------------------------
LSM/LPL positioning reporting types

Command                        Data Structure
GPS_AGPS_LOCATION_RESULT    -> GPS_loc_result
GPS_AGPS_MEAS_RESULT        -> GPS_meas_result
GPS_AGPS_POSITION_ERROR     -> need additional assistance data will be the part of it
----------------------------------------------------------*/
typedef enum
{
    GPS_AGPS_LOCATION_RESULT,       // UE-based
    GPS_AGPS_MEAS_RESULT,           // UE-assisted
    GPS_AGPS_POSITION_ERROR         // need additional assistance data will be the part of it
}CPAgent_Input_Msg_type;

typedef struct
{
   CPAgent_Input_Msg_type type;
   union
   {
      PositionEstimateInfo postion_estimate;
      GPS_MeasurementResults measurements;
      GPS_pos_error pos_error;
   }data;
}CPAgent_InputMsg;

/*----------------------------------------------------------
Assistance data types

Command                        Data Structure
GPS_ASSIST_REF_LOC          -> GPS_assist_ref_loc
GPS_ASSIST_REF_TIME         -> GPS_assist_ref_time
GPS_ASSIST_NAV_MODEL        -> GPS_assist_nav_model
GPS_ASSIST_IONO_MODEL       -> GPS_assist_iono_model
GPS_ASSIST_UTC_MODEL        -> GPS_assist_utc_model
GPS_ASSIST_ALMANAC          -> GPS_assist_almanac
GPS_ASSIST_ACQ              -> GPS_assist_acq
GPS_ASSIST_INTEGRITY        -> GPS_assist_integrity
----------------------------------------------------------*/
typedef enum
{
    GPS_ASSIST_REF_LOC,             // reference location
    GPS_ASSIST_REF_TIME,            // reference time
    GPS_ASSIST_NAV_MODEL,           // single navigation model
    GPS_ASSIST_IONO_MODEL,          // ionospheric model
    GPS_ASSIST_UTC_MODEL,           // UTC model
    GPS_ASSIST_ALMANAC,             // almanac data
    GPS_ASSIST_ACQ,                 // acquisition assistance
    GPS_ASSIST_INTEGRITY,            // Integrity ( satellite health)

}SubMsgType;

typedef enum
{
    RRLP_measurement,
    RRC_measurement
}Pos_Measure_Type;

typedef struct
{
    Pos_Measure_Type  measure_type;
    union
    {
        GPS_start_meas_cmnd_rrc   rrc;
        GPS_start_meas_cmnd_rrlp  rrlp;
    }start_meas_cmd;
}GPS_start_meas_cmd;


typedef struct
{
    tSIRF_UINT8 ref_location_present;
    tSIRF_UINT8 ref_time_present;
    tSIRF_UINT8 nav_model_present;
    tSIRF_UINT8 iono_model_present;
    tSIRF_UINT8 utc_model_present;
    tSIRF_UINT8 almanac_present;
    tSIRF_UINT8 acq_assist_present;
    tSIRF_UINT8 rt_integrity_present;
    ReferenceLocation ref_location;
    GPS_ReferenceTime ref_time;
    GPS_NavigationModel nav_model;
    GPS_IonosphericModel iono_model;
    GPS_UTC_Model utc_model;
    GPS_Almanac almanac;
    GPS_AcquisitionAssistance acq_assist;
    GPS_RealTimeIntegrity rt_integrity;
}GPS_assist_data;

typedef struct
{
   LSM_Input_Msg_Type type;
   union
   {
      GPS_assist_data assist_data;
      GPS_start_meas_cmd meas_cmd;
   }data;
}LSM_Input_Msg;

typedef tSIRF_BOOL (*LSM_CP_Agent_Clbk)( CPAgent_Input_Msg_type type,
                                         void                *data,
                                         int                  size);

tSIRF_BOOL LSM_Input(LSM_Input_Msg_Type  type,
                     void                *data,
                     int                 size);


#endif /* GPS_CPA_LSM_INTF_H */
