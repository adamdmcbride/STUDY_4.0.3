/**************************************************************************
 +------------------------------------------------------------------------+
 | Desc.:   e-Compass Application(daemon) for 3-axis&6-axis Middleware    |
 | File:    ami_daemon.c                                                  |
 | Release note:                                                          |
 +------------------------------------------------------------------------+
 | Copyright (c) 2009 ~ 2011                                              |
 | AMIT Technology Inc.                                                   |
 | All rights reserved.                                                   |
 +------------------------------------------------------------------------+
 | This is source code in Software of AMIT 6-axis Middleware.             |
 | This software is the confidential and proprietary information of       |
 | AMIT Technology Inc.("Confidential Information")                       |
 | You shall not disclose such Confidential Information and shall         |
 | use it only in accordance with the terms of the license agreement.     |
 +------------------------------------------------------------------------+
 **************************************************************************/

#define LOG_TAG "AmitDaemon"
 
#include <cutils/log.h>
 
#include <stdio.h>
#include <linux/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <linux/input.h>
#include "ami30x.h"
#include "kxud9.h"
#include "AMI_Middle.h"

#define ms_msleep(x) 			usleep(x * 1000)
#define CONFIG_FILE_PATH                "/data/ami/AMIMW_Config.ini"
#define DAEMON_VERSION		        	"3.1.32.111"
#define VERIFIED_CODE                   0x0F1838  //for KXUD9
#define DISABLE_CALI                    0
#define ENABLE_AUTO_CALI                1
#define ENABLE_SINGLE_CALI              2
#define DISABLE_ACC_CALI                0
#define ENABLE_ACC_CALI                 1
#define RUN_LOOP_STOP			0
#define RUN_LOOP_START			1
#define TIMER_STOP			0
#define TIMER_START			1

//setting for KXUD9 gain = 819(+/-2g) and offset = 2048 => so set offset 0 in MW
#define g_sensor_1g_gain_x 	    1024 
#define g_sensor_1g_gain_y 	    1024
#define g_sensor_1g_gain_z 	    1024
#define g_sensor_offset_x       2048
#define g_sensor_offset_y       2048
#define g_sensor_offset_z       2048
//setting for AMI30X
#define m_sensor_gain_x         600
#define m_sensor_gain_y         600
#define m_sensor_gain_z         600
#define m_sensor_offset_x       2048
#define m_sensor_offset_y       2048
#define m_sensor_offset_z       2048

#define AMI_MIN_SAMPLING_TIME_MS   50
#define AMI_MAX_SAMPLING_TIME_MS    10

#define AMI_MAG_DIR		10 //placement is G
#define AMI_ACC_DIR		30 //placement is 7

#define DUMP                    LOGI

static int gfp = 0;
static int mfp = 0;

/* Prototype */
static void signal_handler( int signo );

static void StartSignal(void)
{
    DUMP("%s",__FUNCTION__);
    int controlbyte[AMI304_CB_LENGTH];

    signal(SIGINT, signal_handler);
    signal(SIGHUP , signal_handler);
    signal(SIGKILL , signal_handler);
    signal(SIGTERM , signal_handler);
    
    if(mfp != 0)
    {
    	memset(controlbyte, 0, sizeof(controlbyte));
        ioctl(mfp, AMI304DAE_IOCTL_GET_CONTROL, controlbyte);
  	controlbyte[AMI304_CB_RUN] = RUN_LOOP_START;
  	ioctl(mfp, AMI304DAE_IOCTL_SET_CONTROL, controlbyte);
    }        	
}

void signal_handler(int signo)
{
    DUMP("%s signo:%d",__FUNCTION__ , signo);	
    int controlbyte[AMI304_CB_LENGTH];
        
    if(mfp != 0)
    {
    	memset(controlbyte, 0, sizeof(controlbyte));
        ioctl(mfp, AMI304DAE_IOCTL_GET_CONTROL, controlbyte);
  	controlbyte[AMI304_CB_RUN] = RUN_LOOP_STOP;
  	ioctl(mfp, AMI304DAE_IOCTL_SET_CONTROL, controlbyte);
    }        
}

static void ExitDaemon() // Killing Daemons
{
    exit(EXIT_SUCCESS);
}

int WriteFileParameter(AMI_PARAMETER *param)
{
     FILE *fp = NULL;
     long  code = VERIFIED_CODE;
     
     fp = fopen(CONFIG_FILE_PATH, "w+");//setting file
     if(!fp)
     {                                          
         DUMP("open File failed\n");        
         return -1;
     }
     
     rewind(fp);      

     fprintf(fp, "%ld\n", code);
     fflush(fp);

     fprintf(fp,"%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%ld",
        (param->a_gain.x/2),(param->a_gain.y/2),(param->a_gain.z/2),
        (param->a_offset.x - g_sensor_offset_x),(param->a_offset.y - g_sensor_offset_y),(param->a_offset.z - g_sensor_offset_z),
        (param->m_gain.x),(param->m_gain.y),(param->m_gain.z),
        (param->m_offset.x-m_sensor_offset_x),(param->m_offset.y-m_sensor_offset_y),(param->m_offset.z-m_sensor_offset_z),
        param->usr.a_offset.x,param->usr.a_offset.y,param->usr.a_offset.z, 
        param->usr.m_offset.x,param->usr.m_offset.y,param->usr.m_offset.z,
        param->gain_cor.x,param->gain_cor.y,param->gain_cor.z,
        param->mag_radius);
     fflush(fp);
     fclose(fp);    
     
     return 0;
}

int ReadFileParameter(AMI_PARAMETER *param)
{
     FILE *fp = NULL;
     int data[21];
     signed long ldata[1];
     long code = VERIFIED_CODE;
     int fscanfRet;
     
     fp = fopen(CONFIG_FILE_PATH, "r");//Config file
     if(!fp)
     {
         DUMP("open File failed\n");        
         return -1;
     }
     
     rewind(fp);

     fscanf(fp,"%ld\n",&ldata[0]);
     if( ldata[0]!= code ) 
     {
         fclose(fp);
         return -2;
     }

     fscanfRet = fscanf(fp,"%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%6d %6d %6d\n%ld",
            &data[0],&data[1],&data[2],
            &data[3],&data[4],&data[5],
            &data[6],&data[7],&data[8],
            &data[9],&data[10],&data[11],
            &data[12],&data[13],&data[14], 
            &data[15],&data[16],&data[17],
            &data[18],&data[19],&data[20],
            &ldata[0]);
     fclose(fp);
     
    if (fscanfRet != 22)    // the number of read data  = 22
    {
        DUMP("Setting file format error!");
        return -3;
    }
     
     param->a_gain.x = data[0]*2;
     param->a_gain.y = data[1]*2;
     param->a_gain.z = data[2]*2;
     param->a_offset.x = data[3] + g_sensor_offset_x;
     param->a_offset.y = data[4] + g_sensor_offset_y;
     param->a_offset.z = data[5] + g_sensor_offset_z;
     param->m_gain.x = data[6];
     param->m_gain.y = data[7];
     param->m_gain.z = data[8];
     param->m_offset.x = data[9]  + m_sensor_offset_x;
     param->m_offset.y = data[10] + m_sensor_offset_y;
     param->m_offset.z = data[11] + m_sensor_offset_z;
     param->usr.a_offset.x = data[12];
     param->usr.a_offset.y = data[13];
     param->usr.a_offset.z = data[14];
     param->usr.m_offset.x = data[15];
     param->usr.m_offset.y = data[16];
     param->usr.m_offset.z = data[17];
     param->gain_cor.x = data[18];
     param->gain_cor.y = data[19];
     param->gain_cor.z = data[20];
     param->mag_radius = ldata[0];

     return 0;
}

void MagCalibrationEnd(ami_uint8 result, ami_uint8 cont, AMI_PARAMETER *parm)
{
    int controlbyte[AMI304_CB_LENGTH];
    int res = 0;
    
    if(cont == 0x01)
    {
        return;
    }   
    
    if(0x00 == result)
    {
        if(mfp != 0)
        {
            res = ioctl(mfp, AMI304DAE_IOCTL_GET_CONTROL, controlbyte);
            if(res != -1)
            {
                if(controlbyte[AMI304_CB_MAGCALI] == ENABLE_SINGLE_CALI)
                {
                    controlbyte[AMI304_CB_MAGCALI] = DISABLE_CALI;
                    res = ioctl(mfp, AMI304DAE_IOCTL_SET_CONTROL, controlbyte);
                    if(res != -1)
            	    {
            	    	DUMP("Mag Calibratin: Fail to set control byte!!!");
            	    	return;
            	    }                    
                }           
            }
        }
        WriteFileParameter(parm);
        DUMP("Mag Calibration Success!\n");
    }
    else if(0x01 == result)
    {
        DUMP("Mag Calibration Timeout!\n");
    }
    else
    {
        DUMP("Mag Calibration Error!\n");       
    }
}

#ifndef AMI_3AXIS 
void AccCalibratoinEnd(ami_uint8 result, ami_uint8 cont, AMI_PARAMETER *parm)
{
    int controlbyte[AMI304_CB_LENGTH];
    int res = 0;
    if(cont == 0x01)
    {
        return;
    }   

    if(0x00 == result)
    {
        if(mfp != 0)
        {
            res = ioctl(mfp, AMI304DAE_IOCTL_GET_CONTROL, controlbyte);
            if(res != -1)
            {           
                controlbyte[AMI304_CB_ACCCALI] = DISABLE_ACC_CALI;
                res = ioctl(mfp, AMI304DAE_IOCTL_SET_CONTROL, controlbyte);
                if(res != -1)
            	{
            	  DUMP("Acc Calibratin: Fail to set control byte!!!");
            	  return;
            	}                
            }
        }
        WriteFileParameter(parm);
        DUMP("AccCalibration Success!\n");
    }
    else if(0x01 == result)
    {
        DUMP("AccCalibration Timeout!\n");
    }
    else
    {
        DUMP("AccCalibration Error!\n");        
    }
}
#endif

int AccSensorReadRawData(int fd,int* ax,int* ay,int* az)
{
    int     ret=-1;
    char    buf[64];
    int     a_fd=fd;
    int     x, y, z;
    
    if(ax && ay && az)
        (*ax)=(*ay)=(*az)=0;
    x = y = z = 0;

    if(a_fd >= 0 )
    {       
        ret = ioctl(a_fd, KXUD9_IOCTL_GET_COUNT, buf);//for KXUD9		

		if( ret < 0 )
		{
		   DUMP("Read from accelerometer is error, read cnt(%d)\n", ret);
		}
		else
		{
            /* fjt: the middle ware requires:
             * when x axis of kxtj9(not pad) points to zenith, x value is +g (for kxtj9: +1024)
             * when x axis of kxtj9(not pad) points to earth heart, x value is -g (for kxtj9: -1024)
             */
			sscanf(buf, "%x %x %x", &x, &y, &z);        //for KXUD9
			*ax = -y;
			*ay = x;
			*az = -z;
			//DUMP("gsensor raw x=%d, y=%d z=%d\n", *ax,*ay,*az);
			ret=0;
		}
	}

    return ret; 
}

int MagSensorReadRawData(int fd, int* mx, int* my, int* mz)
{
    int     ret=-1;
    int     m_fd=fd;
    int     imx, imy, imz;
    
    if(mx && my && mz)
        (*mx)=(*my)=(*mz)=0;
    imx = imy = imz = 0;

    if( m_fd >= 0 )
    {
#ifdef DBG_MODE	
        //DBG_MODE must use this method to get sensor data, or you can select one of both to get
        char buf[64];
        ret = ioctl(m_fd, AMI304DAE_IOCTL_GET_SENSORDATA, buf);
        if(ret == -1 )
        {
            DUMP("Read from MI-Sensor is error, read cnt(%d)\n", ret);
        }
        else
        {
            sscanf(buf, "%x %x %x", &imx, &imy, &imz);   
            if (imx>32768)  imx -= 65536;//check negative value
            if (imy>32768)  imy -= 65536;//check negative value
            if (imz>32768)  imz -= 65536;//check negative value               
            *mx = imx;
            *my = imy;
            *mz = imz;
            ret=0;
        }
#else
	struct  input_event event[3];
	ret = read(m_fd, &event, sizeof(event));
	if( ret < (int)sizeof(event) ) {
	    DUMP("Read from MI-Sensor is error, read cnt(%d)\n", ret);
	}
	else {
	    imx = event[0].value; 	imy = event[1].value;	imz = event[2].value;
            if (imx>32768)  imx -= 65536;//check negative value
            if (imy>32768)  imy -= 65536;//check negative value
            if (imz>32768)  imz -= 65536;//check negative value      		
            *mx = imx;
            *my = imy;
            *mz = imz;
	    ret=0;
	}
#endif		
    }

    return ret;
}   

void Set_Sleep(int sleep_time_ms)
{    
    if(sleep_time_ms > 0)	
        ms_msleep(sleep_time_ms);    
}

void Set_Middleware_IIRStrength(int sleep_time_ms)
{
    int Check = 0;

#ifdef AMI_3AXIS
    Check = AMI_SetIIRStrength(0,0); 
#else
    if(sleep_time_ms <= 5)
    {
        Check = AMI_SetIIRStrength(15,5);  // 0~100
    }
    else if(sleep_time_ms == 60)
    {
        Check = AMI_SetIIRStrength(5,5);  
    }
    else if(sleep_time_ms == 200)
    {
        Check = AMI_SetIIRStrength(1,5);    
    }
    else//sleep_time_ms == 20 and other
    {
        Check = AMI_SetIIRStrength(10,5);  // 0~100
    }	
    if(Check != 0)
	DUMP("The IIR filter parameter is invalid");	
#endif
}

void Set_Middleware_TimeOut(int sleep_time_ms)
{
    int Check = 0;

    if(sleep_time_ms <= 5)
    {        
        Check = AMI_SetTimeout(4000);   // 300~30000
    }
    else if(sleep_time_ms == 60)
    {        
        Check = AMI_SetTimeout(333);  
    }
    else if(sleep_time_ms == 200)
    {        
        Check = AMI_SetTimeout(300); 
    }
    else//sleep_time_ms == 20 and other
    {        
        Check = AMI_SetTimeout(1000);
    }

    if(Check != 0)
	DUMP("The timeout parameter is invalid!");	
    else 	
    	Set_Middleware_IIRStrength(sleep_time_ms);	
}

void AMIDAE_Report()
{    
   if(mfp > 0) 
     ioctl(mfp, AMI304DAE_IOCTL_SET_REPORT);
   else
     DUMP("mfp < 0, do not report \n");	 
}

int Init_Middleware(void)
{
    AMI_PARAMETER initParam;
    char cRet = 0;
  
    DUMP("Enter Init_Middleware.\n");
 
    initParam.gain_para_XZ = AMI30x_AXIS_INTERFERENCE;
    initParam.gain_para_XY = AMI30x_AXIS_INTERFERENCE;
    initParam.gain_para_YZ = AMI30x_AXIS_INTERFERENCE;
    initParam.gain_para_YX = AMI30x_AXIS_INTERFERENCE;
    initParam.gain_para_ZY = AMI30x_AXIS_INTERFERENCE;
    initParam.gain_para_ZX = AMI30x_AXIS_INTERFERENCE;      
    
    initParam.Mdir = AMI_MAG_DIR;            
    initParam.Gdir = AMI_ACC_DIR;  
    initParam.sensor_type = MODE2;       
    
    // read sensor calibration value
    if( ReadFileParameter(&initParam) < 0 ) //read config file, if no,use default
    {            
        //It's setting for AMI304
        initParam.a_gain.x = g_sensor_1g_gain_x * 2; 
        initParam.a_gain.y = g_sensor_1g_gain_y * 2;        
        initParam.a_gain.z = g_sensor_1g_gain_z * 2;
        //for middleware use g-offset = 2048        
        initParam.a_offset.x = 2048;    
        initParam.a_offset.y = 2048;    
        initParam.a_offset.z = 2048;
        initParam.m_gain.x = m_sensor_gain_x;       
        initParam.m_gain.y = m_sensor_gain_y;       
        initParam.m_gain.z = m_sensor_gain_z;
        //for middleware use m-offset = 2048
        initParam.m_offset.x = 2048;    
        initParam.m_offset.y = 2048;    
        initParam.m_offset.z = 2048;
        
        initParam.usr.a_offset.x = 0;  
        initParam.usr.a_offset.y = 0;  
        initParam.usr.a_offset.z = 0;
        initParam.usr.m_offset.x = 0;  
        initParam.usr.m_offset.y = 0;  
        initParam.usr.m_offset.z = 0;
          
        initParam.gain_cor.x = 100;
        initParam.gain_cor.y = 100;
        initParam.gain_cor.z = 100;
              
        initParam.mag_radius = 0; 
           
        WriteFileParameter(&initParam); //write out to default config file
    } 
        
    cRet = AMI_InitializeSensor(&initParam); 
    if( cRet != 0 ) //return value is not "0", please contact with prolific
    {
        DUMP("Internal Error!error code:%d, please contact with vendor!", cRet);
        goto End_Init_Middleware;
    }
    
#ifdef AMI_3AXIS
    cRet = AMI_SetIIRStrength(0,0);             
#else           
    cRet = AMI_SetIIRStrength(10,5);   // 0~100
#endif	
    if (cRet != 0) {
        DUMP("The IIR filter parameter is invalid");
	goto End_Init_Middleware;
    }
        
    cRet = AMI_SetTimeout(350);    // 300~30000
    if (cRet != 0) {
        DUMP("The timeout parameter is invalid!");
	goto End_Init_Middleware;
    }
    
    DUMP("Start to Auto-Calibration!!!");
    cRet = AMI_CalibrationStart(MagCalibrationEnd);
    if (cRet != 0) {
        DUMP("AMI_CalibrationStart Error!");   	
	goto End_Init_Middleware;
    }

    StartSignal();

    DUMP("Leave Init_Middleware.\n");  
End_Init_Middleware:	
    return cRet;
}

int main()
{
    int control[AMI304_CB_LENGTH];
    int mx,my,mz,ax,ay,az;  
    int res = 0;    
    AMI_SENSORVALUE val;
    AMI_MOTION info;    
    int iYaw, iRoll, iPitch, iStatus;
    int nav_mx,nav_my,nav_mz,nav_ax,nav_ay,nav_az;
    int posturebuf[4] = {0};
    int calidatabuf[7] = {0};
    int mag_cali_flag = ENABLE_AUTO_CALI;
    int check_error;   

    int  timer_flag = TIMER_STOP;   
    static struct timespec Begin_Time,Final_Time;
    double TotalComputeMS;    
    int    TimeToSleepMS = 0;
    int    control_set_timeout  =  0;   
    struct itimerval run_timer; 

    DUMP("AMI30X+KXUD9 e-Compass Application(daemon), Version:%s\n\n", DAEMON_VERSION); 
    DUMP("Copyright (c) 2010 all rights preserved by AMI-TW Technology Inc.\n");
    DUMP("This daemon with middleware build version =%d\n", (int) AMI_GetMWBuildNo() );

    gfp = open("/dev/kxud9", O_RDWR);
    if (gfp==-1) {
       DUMP("Cannot open kxud9!!!\n");
       return -1;
    }

    mfp = open("/dev/ami304daemon", O_RDWR);
    if (mfp==-1) {
       close(gfp);
       DUMP("Cannot open ami304daemon!!!\n");
       return -1;
    }    

    if( Init_Middleware() )
    {
	DUMP("Init_Middleware Fail!!!\n");
        return -2;  
    }
  
    do
    {
        res = ioctl(mfp, AMI304DAE_IOCTL_GET_CONTROL, control);
        if (res < 0)
        {
            control[AMI304_CB_LOOPDELAY] = AMI30x_DEFAULT_POLLING_TIME;
            control[AMI304_CB_RUN] = RUN_LOOP_START;
            control[AMI304_CB_ACTIVESENSORS] = 0;
			DUMP("Fail to AMI304DAE_IOCTL_GET_CONTROL\n"); 
        }
        
#ifdef DBG_MODE
	clock_gettime(CLOCK_MONOTONIC,&Begin_Time);	                                         
		// reset sampling rate                                                                   
        if(control_set_timeout != control[AMI304_CB_LOOPDELAY])
        {
            Set_Middleware_TimeOut(control[AMI304_CB_LOOPDELAY]);
            control_set_timeout = control[AMI304_CB_LOOPDELAY];
	    timer_flag = TIMER_STOP;                                                                  
        }
                	
        mx = my = mz = ax = ay = az = 0;
        MagSensorReadRawData(mfp, &mx, &my, &mz);   
#ifndef AMI_3AXIS        
        AccSensorReadRawData(gfp, &ax, &ay, &az);	
#endif        
        DUMP("ax=%d, ay=%d, az=%d, mx=%d, my=%d, mz=%d\n", ax, ay, az, mx, my, mz);
#else        
            //no sensor
        if( control[AMI304_CB_ACTIVESENSORS] == 0 ) 
        {
	     if(TIMER_STOP != timer_flag)
	     {
	     	// stop timer
	     	run_timer.it_interval.tv_usec =  0;  //control_set_timeout
	     	run_timer.it_interval.tv_sec  =  0;
	     	run_timer.it_value.tv_usec    =  0;  //control_set_timeout
	     	run_timer.it_value.tv_sec     =  0;	
	       
	     	if(setitimer(ITIMER_REAL, &run_timer, NULL)<0)
	     	{
	     	   DUMP("Set timer error!!!\n");			
	     	}	
	     	// reset delay time
	     	control_set_timeout = 300; 	
	     	timer_flag = TIMER_STOP;						
	     }				            	
            ms_msleep(100);
            continue;       
        }  

        clock_gettime(CLOCK_MONOTONIC,&Begin_Time);	                                         
	// reset sampling rate                                                                   
        if(control_set_timeout != control[AMI304_CB_LOOPDELAY])
        {
            Set_Middleware_TimeOut(control[AMI304_CB_LOOPDELAY]);
            control_set_timeout = control[AMI304_CB_LOOPDELAY];
	    timer_flag = TIMER_STOP;                                                                  
        }
        
        mx = my = mz = ax = ay = az = 0;
        //orientation, magnetic_field
        if( (control[AMI304_CB_ACTIVESENSORS] & AMIT_BIT_ORIENTATION) || (control[AMI304_CB_ACTIVESENSORS] & AMIT_BIT_MAGNETIC_FIELD) )
        {         
            MagSensorReadRawData(mfp, &mx, &my, &mz);
        }              
#ifndef AMI_3AXIS            
        //orientation, accelerometer
        if( (control[AMI304_CB_ACTIVESENSORS] & AMIT_BIT_ORIENTATION) || (control[AMI304_CB_ACTIVESENSORS] & AMIT_BIT_ACCELEROMETER)
            || (control[AMI304_CB_ACTIVESENSORS] & AMIT_BIT_PEDOMETER) )
        {
            AccSensorReadRawData(gfp, &ax, &ay, &az);
        }   
#endif			
#endif	//DBG_MODE

        if(mag_cali_flag != control[AMI304_CB_MAGCALI])
        {
            if(control[AMI304_CB_MAGCALI] == ENABLE_AUTO_CALI || control[AMI304_CB_MAGCALI] == ENABLE_SINGLE_CALI)
                AMI_CalibrationStart(MagCalibrationEnd);
            else
                AMI_CalibrationCancel();
               
            mag_cali_flag = control[AMI304_CB_MAGCALI];                 
        }

#ifndef AMI_3AXIS        
        //for acc calibration
        if(control[AMI304_CB_ACCCALI] == ENABLE_ACC_CALI)
        {
            AMI_AccCalibrationStart(AccCalibratoinEnd);
        } 
#endif
        
        mx += m_sensor_offset_x;
        my += m_sensor_offset_y;
        mz += m_sensor_offset_z;	
		
        ax += g_sensor_offset_x; 
        ay += g_sensor_offset_y; 
        az += g_sensor_offset_z;  

        val.mx = mx;  
        val.my = my;  
        val.mz = mz;  
        val.ax = ax;  
        val.ay = ay;  
        val.az = az;        
#ifdef DBG_MODE
        DUMP("val.ax=%d, ay=%d, az=%d, mx=%d, my=%d, mz=%d\n", val.ax, val.ay, val.az, val.mx, val.my, val.mz); 
#endif
        check_error = AMI_InputSensorValue(val);
        if(check_error != 0)
        {
            DUMP("AMI_InputSensorValue Error!!! \n");
            continue;       
        }
        
        AMI_GetMotionInfoLatest(&info);  
        
        iStatus = (int) AMI_CheckMagneticInterference();
        nav_mx = nav_my = nav_mz = nav_ax = nav_ay = nav_az = 0;
        nav_mx = (int) -(info.mag_mGauss.x*AMI304_POSTURE_ACCURACY_RATE);
        nav_my = (int) -(info.mag_mGauss.y*AMI304_POSTURE_ACCURACY_RATE);
        nav_mz = (int) -(info.mag_mGauss.z*AMI304_POSTURE_ACCURACY_RATE);      		
#ifndef AMI_3AXIS		
        iRoll =   (int)(CONVBIT2DEG(info.Android_roll)*AMI304_POSTURE_ACCURACY_RATE);
        iYaw =    (int)(CONVBIT2DEG(info.Android_yaw)*AMI304_POSTURE_ACCURACY_RATE);
        iPitch =  (int)(CONVBIT2DEG(info.Android_pitch)*AMI304_POSTURE_ACCURACY_RATE);
	nav_ax =  (int)(info.acc_mg.x*AMI304_POSTURE_ACCURACY_RATE);
        nav_ay =  (int)(info.acc_mg.y*AMI304_POSTURE_ACCURACY_RATE);
        nav_az =  (int)(info.acc_mg.z*AMI304_POSTURE_ACCURACY_RATE);
#ifdef DBG_MODE
        DUMP("Pos Roll=%d, Pitch=%d, Yaw=%d, Status=%d\n", iRoll, iPitch, iYaw, iStatus);   
        DUMP("nav_mx=%d, nav_my=%d, nav_mz=%d, nav_ax=%d, nav_ay=%d, nav_az=%d\n", nav_mx, nav_my, nav_mz, nav_ax, nav_ay, nav_az);      
#endif        
        posturebuf[0] = iYaw;
        posturebuf[1] = iPitch;
        posturebuf[2] = iRoll;
        posturebuf[3] = iStatus;
        res = ioctl(mfp, AMI304DAE_IOCTL_SET_POSTURE, posturebuf);
        
        if( res < 0 )
        {  
	    DUMP("Fail to AMI304DAE_IOCTL_SET_POSTURE\n");
            posturebuf[0] = posturebuf[1] = posturebuf[2] = posturebuf[3] = 0;
            res = ioctl(mfp, AMI304DAE_IOCTL_SET_POSTURE, posturebuf);     
        }
#endif

        calidatabuf[0] = nav_mx;
        calidatabuf[1] = nav_my; 
        calidatabuf[2] = nav_mz;
        calidatabuf[3] = nav_ax;
        calidatabuf[4] = nav_ay;
        calidatabuf[5] = nav_az;
        calidatabuf[6] = iStatus;            
        res = ioctl(mfp, AMI304DAE_IOCTL_SET_CALIDATA, calidatabuf);        
        if( res < 0 )
        {
	    DUMP("Fail to AMI304DAE_IOCTL_SET_CALIDATA\n");
            calidatabuf[0] = calidatabuf[1] = calidatabuf[2] = calidatabuf[3] = calidatabuf[4] = calidatabuf[5] = calidatabuf[6] = 0;
            res = ioctl(mfp, AMI304DAE_IOCTL_SET_CALIDATA, calidatabuf);      
        }                               

	// start timer at this time => avoid report data not ready 
	if(TIMER_STOP == timer_flag)
	{
	    run_timer.it_interval.tv_usec = control_set_timeout*1000;  //control_set_timeout * 1000
	    run_timer.it_interval.tv_sec  =  0;
	    run_timer.it_value.tv_usec    = control_set_timeout*1000;  //control_set_timeout * 1000
	    run_timer.it_value.tv_sec     =  0;
	    			
	    signal(SIGALRM,AMIDAE_Report);
	    			
	    if(setitimer(ITIMER_REAL, &run_timer, NULL)<0)
	    {
	    	DUMP("Set timer error!!!\n");			
	    }
	    else
	    {
	    	timer_flag = TIMER_START;
	    }				
	}		
        
	clock_gettime(CLOCK_MONOTONIC,&Final_Time);
	TotalComputeMS = ( Final_Time.tv_sec - Begin_Time.tv_sec) * 1000
		+ (double)(Final_Time.tv_nsec-Begin_Time.tv_nsec)/(double)1000000;	
	//set sleep time
	if(control_set_timeout <= AMI_MAX_SAMPLING_TIME_MS)  //max sampling rate : 100Hz
		TimeToSleepMS = AMI_MAX_SAMPLING_TIME_MS - (int)(TotalComputeMS+1.0);
	else if(control_set_timeout >= AMI_MIN_SAMPLING_TIME_MS)		
		TimeToSleepMS = AMI_MIN_SAMPLING_TIME_MS - (int)(TotalComputeMS+1.0);  //min sampling rate : 5Hz
	else		
		TimeToSleepMS = control_set_timeout - (int)(TotalComputeMS+1.0);			
	Set_Sleep(TimeToSleepMS); 
	 
    } while (RUN_LOOP_START==control[AMI304_CB_RUN]);   

    DUMP("Job finished!!\n");
    
    close(gfp);
    close(mfp);
    ExitDaemon(); 
    
    return 0;
}
