/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Nufront Co. LTD
**
*/

#ifndef ANDROID_HARDWARE_CAMERA_V4L2_H
#define ANDROID_HARDWARE_CAMERA_V4L2_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <utils/threads.h>

#include <utils/RefBase.h>
#include <linux/videodev2.h>

#include <utils/String8.h>
#include "videodev2_nusmart.h"
#include "ExifEncoder.h"
#include "ewl.h"
#include "jpegencapi.h"
#include "JpegCompressor.h"

namespace android {

///////////////////////////////////////////////////////////////////////

// enable debug
#define LOG_DEBUG 0
#define NUSMART_CAMERA_RUN_GPU_HARDWARE   0
#define NUSMART_CAMERA_RUN_VPU_HARDWARE   1
#define NUSMART_CAMERA_ISP_EFFECT         1
#define NUSMART_GC300_SCALING             0

///////////////////////////////////////////////////////////////////////
//
//   LOG utils
//
///////////////////////////////////////////////////////////////////////

#if LOG_DEBUG
#define LOG_CAMERA LOGD
#define LOG_CAMERA_PREVIEW LOGD

#define LOG_TIME_DEFINE(n) \
    struct timeval time_start_##n, time_stop_##n; unsigned long log_time_##n = 0;

#define LOG_TIME_START(n) \
    gettimeofday(&time_start_##n, NULL);

#define LOG_TIME_END(n) \
    gettimeofday(&time_stop_##n, NULL); log_time_##n = measure_time(&time_start_##n, &time_stop_##n);

#define LOG_TIME(n) \
    log_time_##n

#else
#define LOG_CAMERA(...)
#define LOG_CAMERA_PREVIEW(...)
#define LOG_TIME_DEFINE(n)
#define LOG_TIME_START(n)
#define LOG_TIME_END(n)
#define LOG_TIME(n)
#endif


/////////////////////////////////////////////////////////
//
//   Camera parameters
//
/////////////////////////////////////////////////////////
#define FRONT_CAM      GC0329
#define BACK_CAM       OV05640

#if !defined (FRONT_CAM) || !defined(BACK_CAM)
#error "Please define the Camera module"
#endif

#define OV05640_PREVIEW_WIDTH            1280
#define OV05640_PREVIEW_HEIGHT           720
#define OV05640_SNAPSHOT_WIDTH           2592
#define OV05640_SNAPSHOT_HEIGHT          1944

#define OV05640_POSTVIEW_WIDTH           640
#define OV05640_POSTVIEW_WIDE_WIDTH      800
#define OV05640_POSTVIEW_HEIGHT          480
#define OV05640_POSTVIEW_BPP             16

#define OV05640_THUMBNAIL_WIDTH          320
#define OV05640_THUMBNAIL_HEIGHT         240
#define OV05640_THUMBNAIL_BPP            16

/* focal length of 3.37mm */
#define OV05640_FOCAL_LENGTH             337
#define OV05640_FOCAL_LENGTH_STR         "3.37"
#define OV05640_HVIEW_ANGLE   56.3
#define OV05640_VVIEW_ANGLE    43.7

///////////////////////////////////////////////////////////
#define GC0329_PREVIEW_WIDTH               640
#define GC0329_PREVIEW_HEIGHT              480
#define GC0329_SNAPSHOT_WIDTH              640
#define GC0329_SNAPSHOT_HEIGHT             480

#define GC0329_THUMBNAIL_WIDTH             160
#define GC0329_THUMBNAIL_HEIGHT            120
#define GC0329_THUMBNAIL_BPP               16

/* focal length of 0.9mm */
#define GC0329_FOCAL_LENGTH                182
#define GC0329_FOCAL_LENGTH_STR            "1.82"

#define GC0329_HVIEW_ANGLE   47.8
#define GC0329_VVIEW_ANGLE    36.9

///////////////////////////////////////////////////////////
#define JOIN(x, y) JOIN_AGAIN(x, y)
#define JOIN_AGAIN(x, y) x ## y

#define MAX_BACK_CAMERA_PREVIEW_WIDTH       JOIN(BACK_CAM,_PREVIEW_WIDTH)
#define MAX_BACK_CAMERA_PREVIEW_HEIGHT      JOIN(BACK_CAM,_PREVIEW_HEIGHT)
#define MAX_BACK_CAMERA_SNAPSHOT_WIDTH      JOIN(BACK_CAM,_SNAPSHOT_WIDTH)
#define MAX_BACK_CAMERA_SNAPSHOT_HEIGHT     JOIN(BACK_CAM,_SNAPSHOT_HEIGHT)
#define BACK_CAMERA_POSTVIEW_WIDTH          JOIN(BACK_CAM,_POSTVIEW_WIDTH)
#define BACK_CAMERA_POSTVIEW_WIDE_WIDTH     JOIN(BACK_CAM,_POSTVIEW_WIDE_WIDTH)
#define BACK_CAMERA_POSTVIEW_HEIGHT         JOIN(BACK_CAM,_POSTVIEW_HEIGHT)
#define BACK_CAMERA_POSTVIEW_BPP            JOIN(BACK_CAM,_POSTVIEW_BPP)
#define BACK_CAMERA_THUMBNAIL_WIDTH         JOIN(BACK_CAM,_THUMBNAIL_WIDTH)
#define BACK_CAMERA_THUMBNAIL_HEIGHT        JOIN(BACK_CAM,_THUMBNAIL_HEIGHT)
#define BACK_CAMERA_THUMBNAIL_BPP           JOIN(BACK_CAM,_THUMBNAIL_BPP)

#define BACK_CAMERA_FOCAL_LENGTH            JOIN(BACK_CAM,_FOCAL_LENGTH)
#define BACK_CAMERA_FOCAL_LENGTH_STR        JOIN(BACK_CAM,_FOCAL_LENGTH_STR)
#define BACK_CAMERA_HVEIW_ANGLE             JOIN(BACK_CAM,_HVIEW_ANGLE)
#define BACK_CAMERA_VVEIW_ANGLE             JOIN(BACK_CAM,_VVIEW_ANGLE)

#define MAX_FRONT_CAMERA_PREVIEW_WIDTH      JOIN(FRONT_CAM,_PREVIEW_WIDTH)
#define MAX_FRONT_CAMERA_PREVIEW_HEIGHT     JOIN(FRONT_CAM,_PREVIEW_HEIGHT)
#define MAX_FRONT_CAMERA_SNAPSHOT_WIDTH     JOIN(FRONT_CAM,_SNAPSHOT_WIDTH)
#define MAX_FRONT_CAMERA_SNAPSHOT_HEIGHT    JOIN(FRONT_CAM,_SNAPSHOT_HEIGHT)

#define FRONT_CAMERA_THUMBNAIL_WIDTH        JOIN(FRONT_CAM,_THUMBNAIL_WIDTH)
#define FRONT_CAMERA_THUMBNAIL_HEIGHT       JOIN(FRONT_CAM,_THUMBNAIL_HEIGHT)
#define FRONT_CAMERA_THUMBNAIL_BPP          JOIN(FRONT_CAM,_THUMBNAIL_BPP)

#define FRONT_CAMERA_FOCAL_LENGTH           JOIN(FRONT_CAM,_FOCAL_LENGTH)
#define FRONT_CAMERA_FOCAL_LENGTH_STR       JOIN(FRONT_CAM,_FOCAL_LENGTH_STR)
#define FRONT_CAMERA_HVEIW_ANGLE             JOIN(FRONT_CAM,_HVIEW_ANGLE)
#define FRONT_CAMERA_VVEIW_ANGLE             JOIN(FRONT_CAM,_VVIEW_ANGLE)

#define DEFAULT_JPEG_THUMBNAIL_WIDTH        256
#define DEFAULT_JPEG_THUMBNAIL_HEIGHT       192


#define CAMERA_SENSOR_SUPPORT_RESOLUTION   0
#define CAMERA_SENSOR_NOT_SUPPORT_RESOLUTION   -1

#define CAMERA_DEV_VIDEO_0   "/dev/video0"
#define CAMERA_DEV_VIDEO_1   "/dev/video1"

#define CAMERA_DEV_NAME_TEMP "/data/videotmp_000"
#define CAMERA_DEV_NAME2_TEMP "/data/videotemp_002"


#define BPP             2
#define MIN(x, y)       (((x) < (y)) ? (x) : (y))

/////////////////////////////////////////////////////////////////////
#define NUM_PREVIEW_BUFFERS     4

#define NUM_CAPTURE_BUFFERS     3

/////////////////////////////////////////////////////////////////////
#define TPATTERN_COLORBAR           1
#define TPATTERN_HORIZONTAL         2
#define TPATTERN_VERTICAL           3


#define V4L2_CID_STREAM_PAUSE               (V4L2_CID_PRIVATE_BASE + 53)

/* FOURCC for FIMC specific */
#define V4L2_PIX_FMT_YUYV           v4l2_fourcc('Y', 'U', 'Y', 'V')
#define V4L2_PIX_FMT_YVYU           v4l2_fourcc('Y', 'V', 'Y', 'U')
#define V4L2_PIX_FMT_VYUY           v4l2_fourcc('V', 'Y', 'U', 'Y')
#define V4L2_PIX_FMT_NV16           v4l2_fourcc('N', 'V', '1', '6')
#define V4L2_PIX_FMT_NV61           v4l2_fourcc('N', 'V', '6', '1')
#define V4L2_PIX_FMT_NV12T          v4l2_fourcc('T', 'V', '1', '2')
#define V4L2_PIX_FMT_NV21           v4l2_fourcc('N', 'V', '2', '1')
#define V4L2_PIX_FMT_NV12           v4l2_fourcc('N', 'V', '1', '2')

#define PROP_KEY_FLASH   "camera.flash.on"

/////////////////////////////////////////////////////////
//
//   Camera AutoFocus Status
//
/////////////////////////////////////////////////////////
enum AF_STATUS {
     AF_PROGRESS = 0,
     AF_SUCCESS,
     AF_FAILED,
};
#define AF_SEARCH_COUNT 40
#define AF_DELAY 50000

#define AFC_CMD_MASK            0x00ff0000
#define AFC_X_MASK              0x000000ff
#define AFC_Y_MASK              0x0000ff00

#define AFC_CMD_SHIFT           16
#define AFC_X_SHIFT             0
#define AFC_Y_SHIFT             8

#define AFC_SET_CMD(_v,_a)      \
        do {\
                 _v |= (_a & (AFC_CMD_MASK >> AFC_CMD_SHIFT)) << AFC_CMD_SHIFT;\
        } while(0)

#define AFC_SET_X(_v,_a)        \
        do {\
                 _v |= (_a & (AFC_X_MASK >> AFC_X_SHIFT)) << AFC_X_SHIFT;\
        } while(0)

#define AFC_SET_Y(_v,_a)        \
        do {\
                 _v |= (_a & (AFC_Y_MASK >> AFC_Y_SHIFT)) << AFC_Y_SHIFT;\
        } while(0)

#define TAKE_PICUTRE_FLAG           0
/////////////////////////////////////////////////////////
//
//   Camera ZOOM Incremental Change Denominator
//
/////////////////////////////////////////////////////////
#define ZOOM_INC_CHANGE_DENOMINATOR 10 //ZOOM Incremental Change Denominator
#define ZOOM_INC_CHANGE_PRECISION   100 //ZOOM Incremental Change precision

////////////////////////////////////////////////////////////
//
//   User-defined type
//
////////////////////////////////////////////////////////////
struct fimc_buffer {
    void    *start;
    size_t  length;
};

struct yuv_fmt_list {
    const char  *name;
    const char  *desc;
    unsigned int    fmt;
    int     depth;
    int     planes;
};

struct camsensor_date_info {
    unsigned int year;
    unsigned int month;
    unsigned int date;
};

/*
 * ISP Output Size Formula
 *
 */
#define ISP_OUTPUT_WIDTH_MASK     0xffff0000
#define ISP_OUTPUT_HEIGHT_MASK    0x0000ffff

#define ISP_OUTPUT_WIDTH_SHIFT   16
#define ISP_OUTPUT_HEIGHT_SHIFT   0

#define ISP_SET_OUTPUT_SIZE(_w,_h)   (((_w) << ISP_OUTPUT_WIDTH_SHIFT) & ISP_OUTPUT_WIDTH_MASK) \
                                     | (((_h) << ISP_OUTPUT_HEIGHT_SHIFT) & ISP_OUTPUT_HEIGHT_MASK)

/*
 * DVFS
 */
#define DVFS_GOVERNOR_SIZE   30
#define DVFS_GOVERNOR_KEY "dvfs.cpufreq.governor"
#define DVFS_GOVERNOR_PERFORMANCE "performance"

class NuCameraV4L2 : public virtual RefBase {
public:

    enum CAMERA_ID {
        CAMERA_ID_BACK  = 1,
        CAMERA_ID_FRONT = 0,
    };

    enum JPEG_QUALITY {
        JPEG_QUALITY_ECONOMY    = 0,
        JPEG_QUALITY_NORMAL     = 50,
        JPEG_QUALITY_SUPERFINE  = 100,
        JPEG_QUALITY_MAX,
    };

    enum OBJECT_TRACKING {
        OBJECT_TRACKING_OFF,
        OBJECT_TRACKING_ON,
        OBJECT_TRACKING_MAX,
    };

    /*Camera sensor mode - Camcorder fix fps*/
    enum SENSOR_MODE {
        SENSOR_MODE_CAMERA,
        SENSOR_MODE_MOVIE,
    };

    /*Camera Shot mode*/
    enum SHOT_MODE {
        SHOT_MODE_SINGLE        = 0,
        SHOT_MODE_CONTINUOUS    = 1,
        SHOT_MODE_PANORAMA      = 2,
        SHOT_MODE_SMILE         = 3,
        SHOT_MODE_SELF          = 6,
    };

    struct gps_info_latiude {
        unsigned int    north_south;
        unsigned int    dgree;
        unsigned int    minute;
        unsigned int    second;
    } gpsInfoLatitude;
    struct gps_info_longitude {
        unsigned int    east_west;
        unsigned int    dgree;
        unsigned int    minute;
        unsigned int    second;
    } gpsInfoLongitude;
    struct gps_info_altitude {
        unsigned int    plus_minus;
        unsigned int    dgree;
        unsigned int    minute;
        unsigned int    second;
    } gpsInfoAltitude;

    NuCameraV4L2();
    virtual ~NuCameraV4L2();

    static NuCameraV4L2* createInstance(void)
    {
        static NuCameraV4L2 singleton;
        return &singleton;
    }
    status_t dump(int fd);

    int             getCameraId(void);
    int             getCameraFd(void);
    int             openCameraDevice(int camera_facing);
    int             initCamera(int index);
    void            DeinitCamera();
    void            resetCamera();
    const __u8*     getCameraSensorName(void);
////////////////////////////////////////////////////////////////////////////////
//
//       recording
//
///////////////////////////////////////////////////////////////////////////////


    int             startRecord(void);
    int             stopRecord(void);
    int             getRecordFrame();
    void            getRecordingSize(int *width, int*height, int*size);
    int             setRecordingSize(int width, int height);
    int             releaseRecordFrame(int index);
    int             nusmart_v4l2_s_fmt(int fp, unsigned int width, unsigned int height, unsigned int fmt, int flag_capture);

////////////////////////////////////////////////////////////////////////////////
//
//       preview
//
///////////////////////////////////////////////////////////////////////////////
    int             startPreview(void);
    int             stopPreview(void);
    void            pausePreview();
    int             getPreviewFrame();
    int             previewPoll();
    int             setPreviewSize(int width, int height, int pixel_format);
    int             getPreviewSize(int *width, int *height, int *frame_size);
    int             getPreviewMaxSize(int *width, int *height);
    int             getPreviewPixelFormat(void);
    int             setPreviewImage(int index, unsigned char *buffer, int size);

////////////////////////////////////////////////////////////////////////////////
//
//       picture
//
///////////////////////////////////////////////////////////////////////////////

    int             setSnapshotCmd(void);
    int             endSnapshot(void);
    int             setSnapshotSize(int width, int height);
    int             getSnapshotSize(int *width, int *height, int *frame_size);
    int             getSnapshotMaxSize(int *width, int *height);
    int             setSnapshotPixelFormat(int pixel_format);
    int             getSnapshotPixelFormat(void);

    int             setJpegQuality(int jpeg_qality);
    int             getJpegQuality(void);
    int             setJpegThumbnailSize(int width, int height);
    int             getJpegThumbnailSize(int *width, int *height);
    int             getSnapshotFrame();
    void            setExifFixedAttribute();
    void            setExifChangedAttribute();
    int             getExif(unsigned char *pExifDst);
    int             setExifOrientationInfo(int orientationInfo);
    void            getPostViewConfig(int*, int*, int*);
    void            getThumbnailConfig(int *width, int *height, int *size);

    int             setAutofocus(void);
    int             zoomIn(void);
    int             zoomOut(void);

    int             jpegInit();
    int             jpegSetInputOutput();
    int             jpegEncoding(int index, int* jpegSize);
    void            jpegGetImage(void *pbuf);


////////////////////////////////////////////////////////////////////////////////
//
//       parameters setting & obtaining
//
///////////////////////////////////////////////////////////////////////////////

    int             setFrameRate(int frame_rate);

    int             setWhiteBalance(int white_balance);
    int             getWhiteBalance(void);

    int             setBrightness(int brightness);
    int             getBrightness(void);

    int             setImageEffect(int image_effect);
    int             getImageEffect(void);

    int             setSceneMode(int scene_mode);
    int             getSceneMode(void);

    int             setFlashMode(int flash_mode);
    int             getFlashMode(void);
    int             flashing(int);

    int             setMetering(int metering_value);
    int             getMetering(void);

    int             setISO(int iso_value);
    int             getISO(void);

    int             setContrast(int contrast_value);
    int             getContrast(void);

    int             setSaturation(int saturation_value);
    int             getSaturation(void);

    int             setSharpness(int sharpness_value);
    int             getSharpness(void);

    int             setWDR(int wdr_value);
    int             getWDR(void);

    int             setAntiShake(int anti_shake);
    int             getAntiShake(void);


    int             setRotate(int angle);
    int             getRotate(void);

    int             setVerticalMirror(void);
    int             setHorizontalMirror(void);

    int             setZoom(int base_w, int base_h, int zoom_level);
    int             getZoom(void);

    int             setObjectTracking(int object_tracking);
    int             getObjectTracking(void);
    int             getObjectTrackingStatus(void);

    int             setSmartAuto(int smart_auto);
    int             getSmartAuto(void);
    int             getAutosceneStatus(void);

    int             setBeautyShot(int beauty_shot);
    int             getBeautyShot(void);

    int             setVintageMode(int vintage_mode);
    int             getVintageMode(void);

    int             setFocusMode(int focus_mode);
    int             getFocusMode(void);

    int             setGPSLatitude(const char *gps_latitude);
    int             setGPSLongitude(const char *gps_longitude);
    int             setGPSAltitude(const char *gps_altitude);
    int             setGPSTimeStamp(const char *gps_timestamp);
    int             setGPSProcessingMethod(const char *gps_timestamp);

    int             setObjectPosition(int x, int y);
    int             setObjectTrackingStartStop(int start_stop);
    int             setTouchAFStartStop(int start_stop);

    int             setAntiBanding(int anti_banding);
    int             getPostview(void);

    int             setGamma(int gamma);
    int             setSlowAE(int slow_ae);
    int             setBatchReflection(void);
    int             setTakepicureFlag(int flag);
    void            setAECValue(void);

////////////////////////////////////////////////////////////////////////////////
//
//       Advanced Features
//
///////////////////////////////////////////////////////////////////////////////

    int             setFaceDetect(int face_detect);
    int             getFaceDetect(void);
    int             setCAFStatus(int on_off);
    int             getAutoFocusResult(void);
    int             cancelAutofocus(void);
    int             setFaceDetectLockUnlock(int facedetect_lockunlock);


    int             setCameraSensorReset(void);
    int             setSensorMode(int sensor_mode); /* Camcorder fix fps */
    int             setShotMode(int shot_mode);     /* Shot mode */

    void            getPreviewFrameAddr(int index, EWLLinearMem_t *) const;
    void            getCaptureFrameAddr(int index, EWLLinearMem_t *) const;
    int             readGovernor(void);
    int             writeGovernor(const char *governor);

private:
    int m_touch_af_start_stop;

    v4l2_streamparm m_streamparm;
    struct nufront_cam_parm   *m_params;

    int             m_flag_init;

    int             m_camera_id;
    int             m_camera_back_dev;

    int             m_cam_fd;

    int             m_cam_fd2;
    struct pollfd   m_events_c2;
    int             m_flag_record_start;

    int             m_preview_v4lformat;
    int             m_preview_width;
    int             m_preview_height;
    int             m_preview_max_width;
    int             m_preview_max_height;

    int             m_snapshot_v4lformat;
    int             m_snapshot_width;
    int             m_snapshot_height;
    int             m_snapshot_max_width;
    int             m_snapshot_max_height;

    int             m_angle;
    int             m_anti_banding;
    int             m_wdr;
    int             m_anti_shake;
    int             m_zoom_level;
    int             m_object_tracking;
    int             m_smart_auto;
    int             m_beauty_shot;
    int             m_vintage_mode;
    int             m_face_detect;
    int             m_object_tracking_start_stop;
    int             m_recording_width;
    int             m_recording_height;
    bool            m_gps_enabled;
    long            m_gps_latitude;  /* degrees * 1e7 */
    long            m_gps_longitude; /* degrees * 1e7 */
    long            m_gps_altitude;  /* metres * 100 */
    long            m_gps_timestamp;
    int             m_sensor_mode; /*Camcorder fix fps */
    int             m_shot_mode; /* Shot mode */
    int             m_exif_orientation;
    int             m_blur_level;
    int             m_video_gamma;
    int             m_slow_ae;
    int             m_caf_on_off;
    int             m_camera_af_flag;

    int             m_flag_camera_start;

    int             m_jpeg_thumbnail_width;
    int             m_jpeg_thumbnail_height;
    int             m_jpeg_quality;
    int             m_sensor_support_resolution; //0 :support , -1: not support
    int             m_closely_width;
    int             m_closely_height;

    int             m_postview_offset;

    const void           *m_ewl_instance;
    const void           *m_jpeg_ewl_instance;
    JpegEncInst           m_jpeg_encoder;
    JpegCompressor       *m_jpeg_compressor;
    EWLLinearMem_t        m_ewl_mem_info[NUM_PREVIEW_BUFFERS];
    EWLLinearMem_t        m_ewl_capture_mem_info[NUM_CAPTURE_BUFFERS];
    EWLLinearMem_t        m_jpeg_outbuf;
    EWLLinearMem_t        m_thumbnail_outbuf;
    int                   m_current_aec_value;
    int                   m_current;
    JpegEncCfg            m_jpeg_enc_cfg;
    JpegEncIn             m_encIn;
    JpegEncOut            m_encOut;
    int                   mBufferSize;
    unsigned int             m_jpeg_image_size;
    unsigned int             m_thumbnail_size;
    char                     m_DVFS_governor[DVFS_GOVERNOR_SIZE];

    exif_attribute_t mExifInfo;
    struct pollfd   m_events_c;
    bool            mFlashFired;

/////////////////////////////////////////////////////////
//
// inline functions
//
/////////////////////////////////////////////////////////
    inline int      m_frameSize(int format, int width, int height);
    inline void     jpegInitEncConfigs();
    inline int     jpegInitInputOutput(int index);
    int thumbnailEncoding(int index);
    int thumbnailInitInOut(int index);
};

extern unsigned long measure_time(struct timeval *start, struct timeval *stop);

}; // namespace android

#endif // ANDROID_HARDWARE_CAMERA_V4L2_H
