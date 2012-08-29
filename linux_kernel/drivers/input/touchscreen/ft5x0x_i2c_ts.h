#ifndef __LINUX_FT5X0X_TS_H__
#define __LINUX_FT5X0X_TS_H__

#include <linux/earlysuspend.h>

//#define CONFIG_SUPPORT_FTS_CTP_UPG

#define CFG_DBG_DUMMY_INFO_SUPPORT   1     //output touch point information
#define CFG_DBG_FUCTION_INFO_SUPPORT 0     //output fouction name
#define CFG_DBG_INPUT_EVENT                   0     //debug input event


#define CFG_MAX_POINT_NUM            0x5    //max touch points supported
#define CFG_NUMOFKEYS                    0x4    //number of touch keys

#define PRESS_MAX                 255

#define KEY_PRESS                 0x1
#define KEY_RELEASE              0x0

#define FT5X0X_NAME    "ft5x0x_ts"  

#define FTS_NULL                    0x0
#define FTS_TRUE                    0x1
#define FTS_FALSE                   0x0
#define I2C_CTPM_ADDRESS    0x70

typedef unsigned char         FTS_BYTE;    
typedef unsigned short        FTS_WORD;   
typedef unsigned int          FTS_DWRD;    
typedef unsigned char         FTS_BOOL;  

typedef enum {
	VENDOR_WGJ_1024 = 0,		//A8=79
	VENDOR_GBGD,			//A8=9f
	VENDOR_WGJ_800,			//A8=79,A6=13
}EVendorID;

#define	REVERSE_X	0x01
#define	REVERSE_Y	0x02
#define	REVERSE_XY	0x04

 typedef struct _REPORT_FINGER_INFO_T
 {
     short    flag;           /*00:PenDown;01:PenUp;10:Contack;11:Revert*/
     short   ui2_id;               /* ID information, from 0 to  CFG_MAX_POINT_NUM - 1*/
     short    u2_pressure;    /* ***pressure information, valid from 0 -63 **********/
     short    i2_x;                /*********** X coordinate, 0 - 2047 ****************/
     short    i2_y;                /* **********Y coordinate, 0 - 2047 ****************/
 } REPORT_FINGER_INFO_T;


typedef enum
{
    ERR_OK,
    ERR_MODE,
    ERR_READID,
    ERR_ERASE,
    ERR_STATUS,
    ERR_ECC,
    ERR_DL_ERASE_FAIL,
    ERR_DL_PROGRAM_FAIL,
    ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

struct FTS_BUTTON_INFO {
	int keycode;
	int ymin;
	int ymax;
};

struct FTS_TS_DATA_T {
    struct input_dev    *input_dev;
    struct work_struct     pen_event_work;
    struct workqueue_struct *ts_workqueue;
    struct early_suspend early_suspend;
    struct i2c_client *client;
    int max_touch_num;
    EVendorID vendor;
    int xres;
    int yres;
    unsigned reverse;
    struct FTS_BUTTON_INFO *button;
};


enum ft5x0x_ts_regs {
    FT5X0X_REG_THRES = 0x80,              /* Thresshold, the threshold be low, the sensitivy will be high */
    FT5X0X_REG_REPORT_RATE = 0x88,  /* **************report rate, in unit of 10Hz **************/
    FT5X0X_REG_PMODE    = 0xA5,          /* Power Consume Mode 0 -- active, 1 -- monitor, 3 -- sleep */    
    FT5X0X_REG_FIRMID   = 0xA6,          /* ***************firmware version **********************/
    FT5X0X_REG_VENRODID   = 0xA8,
    FT5X0X_REG_NOISE_MODE = 0xb2    /* to enable or disable power noise, 1 -- enable, 0 -- disable */
};

#define PMODE_ACTIVE        0x00
#define PMODE_MONITOR       0x01
#define PMODE_STANDBY       0x02
#define PMODE_HIBERNATE     0x03


#ifndef ABS_MT_TOUCH_MAJOR
#define ABS_MT_TOUCH_MAJOR    0x30    /* touching ellipse */
#define ABS_MT_TOUCH_MINOR    0x31    /* (omit if circular) */
#define ABS_MT_WIDTH_MAJOR    0x32    /* approaching ellipse */
#define ABS_MT_WIDTH_MINOR    0x33    /* (omit if circular) */
#define ABS_MT_ORIENTATION     0x34    /* Ellipse orientation */
#define ABS_MT_POSITION_X       0x35    /* Center X ellipse position */
#define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE        0x37    /* Type of touching device */
#define ABS_MT_BLOB_ID             0x38    /* Group set of pkts as blob */
#endif 


#endif

