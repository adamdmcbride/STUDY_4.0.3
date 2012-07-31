/*
 * Video for Linux Two header file for samsung
 *
 * Copyright (C) 2012, Nufront Co. LTD
 *
 * This header file contains several v4l2 APIs to be proposed to v4l2
 * community and until bein accepted, will be used restrictly in Samsung's
 * camera interface driver FIMC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Alternatively, Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LINUX_VIDEODEV2_SAMSUNG_H
#define __LINUX_VIDEODEV2_SAMSUNG_H

////////////////////////////////////////////////////////////////////////////////
//
//  Nufront Control Extension
//
////////////////////////////////////////////////////////////////////////////////

#define V4L2_CTRL_CLASS_USER 0x00980000
#define V4L2_CTRL_CLASS_CAMERA 0x009a0000       // Camera class controls
#define V4L2_CID_BASE                   (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_CAMERA_CLASS_BASE      (V4L2_CTRL_CLASS_CAMERA | 0x900)
#define V4L2_CID_PRIVATE_BASE			0x08000000

//input
#define V4L2_CID_NUSMART_CAMERA_DEV_INPUT	(V4L2_CID_PRIVATE_BASE + 120)

//output
#define V4L2_CID_NUSMART_STREAM_OUTPUT_PATH   (V4L2_CID_PRIVATE_BASE + 121)
enum v4l2_stream_output_path {
  STREAM_NONE    = 0,
  STREAM_CAPTURE = 1, //main-path
  STREAM_PREVIEW = 2, //self-path
  STREAM_BOTH    = 3,
};

// frame rate
#define V4L2_CID_CAMERA_FRAME_RATE		(V4L2_CID_PRIVATE_BASE + 122)
enum v4l2_frame_rate {
	FRAME_RATE_AUTO	= 0,
	FRAME_RATE_7	= 7,
	FRAME_RATE_15	= 15,
	FRAME_RATE_30	= 30,
	FRAME_RATE_45 = 45,
	FRAME_RATE_60	= 60,
	FRAME_RATE_90 = 90,
	FRAME_RATE_120	= 120,
	FRAME_RATE_MAX
};


// special effect
#define V4L2_CID_CAMERA_SPECIAL_EFFECT			(V4L2_CID_PRIVATE_BASE + 124)
enum v4l2_effect_mode {
	IMAGE_EFFECT_NONE = 0,
	IMAGE_EFFECT_BLUEISH,
	IMAGE_EFFECT_REDISH,
	IMAGE_EFFECT_AQUA,
	IMAGE_EFFECT_MONO,
	IMAGE_EFFECT_SEPIA,
	IMAGE_EFFECT_NEGATIVE,
	IMAGE_EFFECT_OVEREXPOSURE,
	IMAGE_EFFECT_SOLARIZE,
	IMAGE_EFFECT_MAX,
};

// camera sensor facing, only get
#define V4L2_CID_CAMERA_FACING (V4L2_CID_PRIVATE_BASE + 125)
enum v4l2_camera_facing {
  NUSMART_CAMERA_FACING_BACK  = 0,
  NUSMART_CAMERA_FACING_FRONT = 1,
};

// get exposure control value
#define V4L2_CID_CAMERA_GET_AEC		(V4L2_CID_PRIVATE_BASE + 126)

//scene mode
#define V4L2_CID_PRE_CAP		(V4L2_CID_PRIVATE_BASE + 127)

//scene mode
#define V4L2_CID_CAMERA_SCENE_MODE		(V4L2_CID_PRIVATE_BASE + 128)
enum v4l2_scene_mode {
	SCENE_MODE_BASE,
	SCENE_MODE_NONE,
	SCENE_MODE_PORTRAIT,
	SCENE_MODE_NIGHTSHOT,
	SCENE_MODE_BACK_LIGHT,
	SCENE_MODE_LANDSCAPE,
	SCENE_MODE_SPORTS,
	SCENE_MODE_PARTY_INDOOR,
	SCENE_MODE_BEACH_SNOW,
	SCENE_MODE_SUNSET,
	SCENE_MODE_DUST_DAWN,
	SCENE_MODE_FALL_COLOR,
	SCENE_MODE_FIREWORKS,
	SCENE_MODE_TEXT,
	SCENE_MODE_CANDLE_LIGHT,
	SCENE_MODE_MAX,
};

//flash mode
#define V4L2_CID_CAMERA_FLASH_MODE		(V4L2_CID_PRIVATE_BASE + 129)
enum v4l2_flash_mode {
	FLASH_MODE_OFF,
	FLASH_MODE_ON,
	FLASH_MODE_AUTO,
	FLASH_MODE_MAX,
};

extern "C" {

#define FLASH_VAL		       "/sys/class/gpio/gpio58/value"
#define AUTO_FLASH_THRESHOLD  0x80

}
//auto focus
#define V4L2_CID_CAMERA_AUTO_FOCUS		(V4L2_CID_CAMERA_CLASS_BASE + 12)
enum v4l2_auto_focus {
    AFC_CMD_SING,   // single
    AFC_CMD_CONT,   // continuous
    AFC_CMD_PAUS,   // pause
    AFC_CMD_RELE,   // release
    AFC_CMD_RELA,   // re-launch
};

// white balance
#define V4L2_CID_CAMERA_WHITE_BALANCE		(V4L2_CID_BASE + 13)
enum v4l2_wb_mode {
	WHITE_BALANCE_AUTO = 0,
	WHITE_BALANCE_SUNNY,
	WHITE_BALANCE_CLOUDY,
	WHITE_BALANCE_OFFICE,
	WHITE_BALANCE_HOME,
	WHITE_BALANCE_MAX,
};

//brightness
#define V4L2_CID_CAMERA_BRIGHTNESS		(V4L2_CID_BASE + 0)
enum v4l2_ev_mode {
	EV_MINUS_4 = 0,
	EV_MINUS_3,
	EV_MINUS_2,
	EV_MINUS_1,
	EV_DEFAULT,
	EV_PLUS_1,
	EV_PLUS_2,
	EV_PLUS_3,
	EV_PLUS_4,
	EV_MAX,
};

//vertical flip
#define V4L2_CID_CAMERA_VFLIP                      (V4L2_CID_BASE + 21)

//horizontal flip
#define V4L2_CID_CAMERA_HFLIP                      (V4L2_CID_BASE + 20)

//contrast
#define V4L2_CID_CAMERA_CONTRAST		(V4L2_CID_BASE + 1)
enum v4l2_contrast_mode {
       CONTRAST_MINUS_3 = 0,
	CONTRAST_MINUS_2,
	CONTRAST_MINUS_1,
	CONTRAST_DEFAULT,
	CONTRAST_PLUS_1,
	CONTRAST_PLUS_2,
	CONTRAST_PLUS_3,
	CONTRAST_MAX,
};

//saturation
#define V4L2_CID_CAMERA_SATURATION		(V4L2_CID_BASE + 2)
enum v4l2_saturation_mode {
       SATURATION_MINUS_3 = 0,
	SATURATION_MINUS_2,
	SATURATION_MINUS_1,
	SATURATION_DEFAULT,
	SATURATION_PLUS_1,
	SATURATION_PLUS_2,
	SATURATION_PLUS_3,
	SATURATION_MAX,
};


//ISO
#define V4L2_CID_CAMERA_ISO			(V4L2_CID_PRIVATE_BASE + 75)
enum v4l2_iso_mode {
	ISO_AUTO = 0,
	ISO_50,
	ISO_100,
	ISO_200,
	ISO_400,
	ISO_800,
	ISO_1600,
	ISO_SPORTS,
	ISO_NIGHT,
	ISO_MOVIE,
	ISO_MAX,
};

//metering
#define V4L2_CID_CAMERA_METERING		(V4L2_CID_PRIVATE_BASE + 76)
enum v4l2_metering_mode {
	METERING_BASE = 0,
	METERING_MATRIX,
	METERING_CENTER,
	METERING_SPOT,
	METERING_MAX,
};

//sharpness
#define V4L2_CID_CAMERA_SHARPNESS		(V4L2_CID_PRIVATE_BASE + 79)
enum v4l2_sharpness_mode {
	SHARPNESS_MINUS_2 = 0,
	SHARPNESS_MINUS_1,
	SHARPNESS_DEFAULT,
	SHARPNESS_PLUS_1,
	SHARPNESS_PLUS_2,
	SHARPNESS_MAX,
};

//Wide Dynamic Range
#define V4L2_CID_CAMERA_WDR			(V4L2_CID_PRIVATE_BASE + 80)
enum v4l2_wdr_mode {
	WDR_OFF,
	WDR_ON,
	WDR_MAX,
};

//anti-shake
#define V4L2_CID_CAMERA_ANTI_SHAKE		(V4L2_CID_PRIVATE_BASE + 81)
enum v4l2_anti_shake_mode {
	ANTI_SHAKE_OFF,
	ANTI_SHAKE_STILL_ON,
	ANTI_SHAKE_MOVIE_ON,
	ANTI_SHAKE_MAX,
};

#define V4L2_CID_CAMERA_TOUCH_AF_START_STOP	(V4L2_CID_PRIVATE_BASE + 82)
enum v4l2_touch_af {
	TOUCH_AF_STOP = 0,
	TOUCH_AF_START,
	TOUCH_AF_MAX,
};

#define V4L2_CID_CAMERA_SMART_AUTO		(V4L2_CID_PRIVATE_BASE + 83)
enum v4l2_smart_auto {
	SMART_AUTO_OFF = 0,
	SMART_AUTO_ON,
	SMART_AUTO_MAX,
};

#define V4L2_CID_CAMERA_VINTAGE_MODE		(V4L2_CID_PRIVATE_BASE + 84)
enum v4l2_vintage_mode {
	VINTAGE_MODE_BASE,
	VINTAGE_MODE_OFF,
	VINTAGE_MODE_NORMAL,
	VINTAGE_MODE_WARM,
	VINTAGE_MODE_COOL,
	VINTAGE_MODE_BNW,
	VINTAGE_MODE_MAX,
};

//zoom
//#define V4L2_CID_CAMERA_ZOOM    (V4L2_CID_PRIVATE_BASE + 90)
enum v4l2_zoom_level {
	ZOOM_LEVEL_0 = 0,
	ZOOM_LEVEL_1,
	ZOOM_LEVEL_2,
	ZOOM_LEVEL_3,
	ZOOM_LEVEL_4,
	ZOOM_LEVEL_5,
	ZOOM_LEVEL_6,
	ZOOM_LEVEL_7,
	ZOOM_LEVEL_8,
	ZOOM_LEVEL_9,
	ZOOM_LEVEL_10,
/*
	ZOOM_LEVEL_11,
	ZOOM_LEVEL_12,
	ZOOM_LEVEL_13,
	ZOOM_LEVEL_14,
	ZOOM_LEVEL_15,
	ZOOM_LEVEL_16,
	ZOOM_LEVEL_17,
	ZOOM_LEVEL_18,
	ZOOM_LEVEL_19,
	ZOOM_LEVEL_20,
	*/
	ZOOM_LEVEL_MAX,
};

//face-detection
#define V4L2_CID_CAMERA_FACE_DETECTION		(V4L2_CID_PRIVATE_BASE + 91)
enum v4l2_face_detection {
	FACE_DETECTION_OFF = 0,
	FACE_DETECTION_ON,
	FACE_DETECTION_NOLINE,
	FACE_DETECTION_ON_BEAUTY,
	FACE_DETECTION_MAX,
};

#define V4L2_CID_CAMERA_SMART_AUTO_STATUS	(V4L2_CID_PRIVATE_BASE + 92)
enum v4l2_smart_auto_status {
	SMART_AUTO_STATUS_AUTO = 0,
	SMART_AUTO_STATUS_LANDSCAPE,
	SMART_AUTO_STATUS_PORTRAIT,
	SMART_AUTO_STATUS_MACRO,
	SMART_AUTO_STATUS_NIGHT,
	SMART_AUTO_STATUS_PORTRAIT_NIGHT,
	SMART_AUTO_STATUS_BACKLIT,
	SMART_AUTO_STATUS_PORTRAIT_BACKLIT,
	SMART_AUTO_STATUS_ANTISHAKE,
	SMART_AUTO_STATUS_PORTRAIT_ANTISHAKE,
	SMART_AUTO_STATUS_MAX,
};

#define V4L2_CID_CAMERA_SHT_TIME (V4L2_CID_PRIVATE_BASE + 93)

#define V4L2_CID_CAMERA_BEAUTY_SHOT		(V4L2_CID_PRIVATE_BASE + 94)
enum v4l2_beauty_shot {
	BEAUTY_SHOT_OFF = 0,
	BEAUTY_SHOT_ON,
	BEAUTY_SHOT_MAX,
};

#define V4L2_CID_CAMERA_AEAWB_LOCK_UNLOCK	(V4L2_CID_PRIVATE_BASE + 95)
enum v4l2_ae_awb_lockunlock {
	AE_UNLOCK_AWB_UNLOCK = 0,
	AE_LOCK_AWB_UNLOCK,
	AE_UNLOCK_AWB_LOCK,
	AE_LOCK_AWB_LOCK,
	AE_AWB_MAX
};

#define V4L2_CID_CAMERA_FACEDETECT_LOCKUNLOCK	(V4L2_CID_PRIVATE_BASE + 96)
enum v4l2_face_lock {
	FACE_LOCK_OFF = 0,
	FACE_LOCK_ON,
	FIRST_FACE_TRACKING,
	FACE_LOCK_MAX,
};

//focus mode
#define V4L2_CID_CAMERA_FOCUS_MODE		(V4L2_CID_PRIVATE_BASE + 99)
enum v4l2_focusmode {
	FOCUS_MODE_AUTO = 0,
	FOCUS_MODE_MACRO,
	FOCUS_MODE_FACEDETECT,
	FOCUS_MODE_AUTO_DEFAULT,
	FOCUS_MODE_MACRO_DEFAULT,
	FOCUS_MODE_FACEDETECT_DEFAULT,
	FOCUS_MODE_INFINITY,
    FOCUS_MODE_CONT_VIDEO,
	FOCUS_MODE_MAX,
};

#define V4L2_CID_CAMERA_OBJ_TRACKING_STATUS	(V4L2_CID_PRIVATE_BASE + 100)
enum v4l2_obj_tracking_status {
	OBJECT_TRACKING_STATUS_BASE,
	OBJECT_TRACKING_STATUS_PROGRESSING,
	OBJECT_TRACKING_STATUS_SUCCESS,
	OBJECT_TRACKING_STATUS_FAIL,
	OBJECT_TRACKING_STATUS_MISSING,
	OBJECT_TRACKING_STATUS_MAX,
};

#define V4L2_CID_CAMERA_OBJ_TRACKING_START_STOP	(V4L2_CID_PRIVATE_BASE + 101)
enum v4l2_ot_start_stop {
	OT_STOP = 0,
	OT_START,
	OT_MAX,
};

#define V4L2_CID_CAMERA_CAF_START_STOP		(V4L2_CID_PRIVATE_BASE + 102)
enum v4l2_caf_start_stop {
	CAF_STOP = 0,
	CAF_START,
	CAF_MAX,
};

#define V4L2_CID_CAMERA_ANTI_BANDING		(V4L2_CID_PRIVATE_BASE + 105)
enum v4l2_anti_banding {
	ANTI_BANDING_AUTO	= 0,
	ANTI_BANDING_50HZ	= 1,
	ANTI_BANDING_60HZ	= 2,
	ANTI_BANDING_OFF	= 3,
};

#define V4L2_CID_CAMERA_SET_GAMMA		(V4L2_CID_PRIVATE_BASE + 106)
enum v4l2_gamma_mode {
	GAMMA_OFF	= 0,
	GAMMA_ON	= 1,
	GAMMA_MAX,
};

//auto exposure
#define V4L2_CID_CAMERA_SET_SLOW_AE		(V4L2_CID_PRIVATE_BASE + 107)
enum v4l2_slow_ae_mode {
	SLOW_AE_OFF,
	SLOW_AE_ON,
	SLOW_AE_MAX,
};


#define V4L2_CID_CAMERA_ISP_OUTPUT_SIZE  (V4L2_CID_PRIVATE_BASE + 123)

/* Pixel format FOURCC depth Description */
/* 12  Y/CbCr 4:2:0 64x32 macroblocks */
#define V4L2_PIX_FMT_NV12T    v4l2_fourcc('T', 'V', '1', '2')

/*
 *  * V4L2 extention for digital camera
 *   */
/* Strobe flash light */
enum v4l2_strobe_control {
	/* turn off the flash light */
	V4L2_STROBE_CONTROL_OFF		= 0,
	/* turn on the flash light */
	V4L2_STROBE_CONTROL_ON		= 1,
	/* act guide light before splash */
	V4L2_STROBE_CONTROL_AFGUIDE	= 2,
	/* charge the flash light */
	V4L2_STROBE_CONTROL_CHARGE	= 3,
};

enum v4l2_strobe_conf {
	V4L2_STROBE_OFF			= 0,	/* Always off */
	V4L2_STROBE_ON			= 1,	/* Always splashes */
	/* Auto control presets */
	V4L2_STROBE_AUTO		= 2,
	V4L2_STROBE_REDEYE_REDUCTION	= 3,
	V4L2_STROBE_SLOW_SYNC		= 4,
	V4L2_STROBE_FRONT_CURTAIN	= 5,
	V4L2_STROBE_REAR_CURTAIN	= 6,
	/* Extra manual control presets */
	/* keep turned on until turning off */
	V4L2_STROBE_PERMANENT		= 7,
	V4L2_STROBE_EXTERNAL		= 8,
};

enum v4l2_strobe_status {
	V4L2_STROBE_STATUS_OFF		= 0,
	/* while processing configurations */
	V4L2_STROBE_STATUS_BUSY		= 1,
	V4L2_STROBE_STATUS_ERR		= 2,
	V4L2_STROBE_STATUS_CHARGING	= 3,
	V4L2_STROBE_STATUS_CHARGED	= 4,
};

/* capabilities field */
/* No strobe supported */
#define V4L2_STROBE_CAP_NONE		0x0000
/* Always flash off mode */
#define V4L2_STROBE_CAP_OFF		0x0001
/* Always use flash light mode */
#define V4L2_STROBE_CAP_ON		0x0002
/* Flashlight works automatic */
#define V4L2_STROBE_CAP_AUTO		0x0004
/* Red-eye reduction */
#define V4L2_STROBE_CAP_REDEYE		0x0008
/* Slow sync */
#define V4L2_STROBE_CAP_SLOWSYNC	0x0010
/* Front curtain */
#define V4L2_STROBE_CAP_FRONT_CURTAIN	0x0020
/* Rear curtain */
#define V4L2_STROBE_CAP_REAR_CURTAIN	0x0040
/* keep turned on until turning off */
#define V4L2_STROBE_CAP_PERMANENT	0x0080
/* use external strobe */
#define V4L2_STROBE_CAP_EXTERNAL	0x0100

/* Set mode and Get status */
struct v4l2_strobe {
	/* off/on/charge:0/1/2 */
	enum v4l2_strobe_control control;
	/* supported strobe capabilities */
	__u32 capabilities;
	enum v4l2_strobe_conf mode;
	enum v4l2_strobe_status status;	/* read only */
	/* default is 0 and range of value varies from each models */
	__u32 flash_ev;
	__u32 reserved[4];
};

#define VIDIOC_S_STROBE     _IOWR('V', 83, struct v4l2_strobe)
#define VIDIOC_G_STROBE     _IOR('V', 84, struct v4l2_strobe)

/* Object recognition and collateral actions */
enum v4l2_recog_mode {
	V4L2_RECOGNITION_MODE_OFF	= 0,
	V4L2_RECOGNITION_MODE_ON	= 1,
	V4L2_RECOGNITION_MODE_LOCK	= 2,
};

enum v4l2_recog_action {
	V4L2_RECOGNITION_ACTION_NONE	= 0,	/* only recognition */
	V4L2_RECOGNITION_ACTION_BLINK	= 1,	/* Capture on blinking */
	V4L2_RECOGNITION_ACTION_SMILE	= 2,	/* Capture on smiling */
};

enum v4l2_recog_pattern {
	V4L2_RECOG_PATTERN_FACE		= 0,	/* Face */
	V4L2_RECOG_PATTERN_HUMAN	= 1,	/* Human */
	V4L2_RECOG_PATTERN_CHAR		= 2,	/* Character */
};

struct v4l2_recog_rect {
	enum v4l2_recog_pattern p;	/* detected pattern */
	struct v4l2_rect o;		/* detected area */
	__u32 reserved[4];
};

struct v4l2_recog_data {
	__u8 detect_cnt;	/* detected object counter */
	struct v4l2_rect o;	/* detected area */
	__u32 reserved[4];
};

struct v4l2_recognition {
	enum v4l2_recog_mode mode;

	/* Which pattern to detect */
	enum v4l2_recog_pattern pattern;

	/* How many object to detect */
	__u8 obj_num;

	/* select detected object */
	__u32 detect_idx;

	/* read only :Get object coordination */
	struct v4l2_recog_data data;

	enum v4l2_recog_action action;
	__u32 reserved[4];
};

#define VIDIOC_S_RECOGNITION	_IOWR('V', 85, struct v4l2_recognition)
#define VIDIOC_G_RECOGNITION	_IOR('V', 86, struct v4l2_recognition)

/* We use this struct as the v4l2_streamparm raw_data for
 * VIDIOC_G_PARM and VIDIOC_S_PARM
 */
struct nufront_cam_parm {
	struct v4l2_captureparm capture;
	int contrast;
	int effects;
	int brightness;
	int flash_mode;
	int focus_mode;
	int iso;
	int metering;
	int saturation;
	int scene_mode;
	int sharpness;
	int white_balance;
};

#endif /* __LINUX_VIDEODEV2_SAMSUNG_H */
