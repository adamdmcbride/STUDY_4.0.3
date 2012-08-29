/* 
 * drivers/input/touchscreen/ft5x0x_ts.c
 *
 * FocalTech ft5x0x TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *    note: only support mulititouch    Wenfs 2010-10-01
 */

#define CONFIG_FTS_CUSTOME_ENV
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include "ft5x0x_i2c_ts.h"
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <mach/extend.h>


/* -------------- global variable definition -----------*/

#define	KEYFLAG_REPORTED	0x00000001
#define	KEYFLAG_DETECTED	0x00000002
static struct FTS_BUTTON_INFO tsp_keycodes_WGJ_1024[] = {
	{KEY_BACK, 90, 110},	//100
	{KEY_HOME, 40, 60},	//47/57
	{KEY_MENU,  0, 10},	//0
	{0, 0, 0},
};
static struct FTS_BUTTON_INFO tsp_keycodes_GBGD[] = {
	{KEY_BACK, 681, 720},	//700
	{KEY_HOME, 721, 760},	//740
	{KEY_MENU, 761, 800},	//780
	{0, 0, 0},
};
static struct FTS_BUTTON_INFO tsp_keycodes_WGJ_800[] = {
	{KEY_BACK, 61, 101},	//70. 014=100
	{KEY_HOME, 30, 61},	//47/57, 0x14=60
	{KEY_MENU,  0, 10},	//0
	{0, 0, 0},
};

static uint32_t tsp_keystatus[CFG_NUMOFKEYS];

extern void axp_touchscreen(int on);

/***********************************************************************
    [function]: 
		           callback:              read data from ctpm by i2c interface;
    [parameters]:
			    buffer[in]:            data buffer;
			    length[in]:           the length of the data buffer;
    [return]:
			    FTS_TRUE:            success;
			    FTS_FALSE:           fail;
************************************************************************/
static bool i2c_read_interface(struct i2c_client *client, u8* pbt_buf, int dw_lenth)
{
    int ret;
    
    ret=i2c_master_recv(client, pbt_buf, dw_lenth);

    if(ret<=0)
    {
        printk("[TSP]i2c_read_interface error\n");
        return FTS_FALSE;
    }
  
    return FTS_TRUE;
}



/***********************************************************************
    [function]: 
		           callback:               write data to ctpm by i2c interface;
    [parameters]:
			    buffer[in]:             data buffer;
			    length[in]:            the length of the data buffer;
    [return]:
			    FTS_TRUE:            success;
			    FTS_FALSE:           fail;
************************************************************************/
static bool  i2c_write_interface(struct i2c_client *client, u8* pbt_buf, int dw_lenth)
{
    int ret;
    ret=i2c_master_send(client, pbt_buf, dw_lenth);
    if(ret<=0)
    {
        printk("[TSP]i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);
        return FTS_FALSE;
    }

    return FTS_TRUE;
}



/***********************************************************************
    [function]: 
		           callback:                 read register value ftom ctpm by i2c interface;
    [parameters]:
                        reg_name[in]:         the register which you want to read;
			    rx_buf[in]:              data buffer which is used to store register value;
			    rx_length[in]:          the length of the data buffer;
    [return]:
			    FTS_TRUE:              success;
			    FTS_FALSE:             fail;
************************************************************************/
static bool fts_register_read(struct i2c_client *client, u8 reg_name, u8* rx_buf, int rx_length)
{
	u8 read_cmd[2]= {0};
	u8 cmd_len 	= 0;

	read_cmd[0] = reg_name;
	cmd_len = 1;	

	/*send register addr*/
	if(!i2c_write_interface(client, &read_cmd[0], cmd_len))
	{
		return FTS_FALSE;
	}

	/*call the read callback function to get the register value*/		
	if(!i2c_read_interface(client, rx_buf, rx_length))
	{
		return FTS_FALSE;
	}
	return FTS_TRUE;
}




/***********************************************************************
    [function]: 
		           callback:                read register value ftom ctpm by i2c interface;
    [parameters]:
                        reg_name[in]:         the register which you want to write;
			    tx_buf[in]:              buffer which is contained of the writing value;
    [return]:
			    FTS_TRUE:              success;
			    FTS_FALSE:             fail;
************************************************************************/
static bool fts_register_write(struct i2c_client *client, u8 reg_name, u8* tx_buf)
{
	u8 write_cmd[2] = {0};

	write_cmd[0] = reg_name;
	write_cmd[1] = *tx_buf;

	/*call the write callback function*/
	return i2c_write_interface(client, write_cmd, 2);
}

/***********************************************************************
    [function]: 
		           callback:                 read touch  data ftom ctpm by i2c interface;
    [parameters]:
			    rxdata[in]:              data buffer which is used to store touch data;
			    length[in]:              the length of the data buffer;
    [return]:
			    FTS_TRUE:              success;
			    FTS_FALSE:             fail;
************************************************************************/
static int fts_i2c_rxdata(struct i2c_client *client, u8 *rxdata, int length)
{
      int ret;
      struct i2c_msg msg;

	
      msg.addr = client->addr;
      msg.flags = 0;
      msg.len = 1;
      msg.buf = rxdata;
      ret = i2c_transfer(client->adapter, &msg, 1);
	  
	if (ret < 0)
		pr_err("msg %s i2c write error: %d\n", __func__, ret);
		
      msg.addr = client->addr;
      msg.flags = I2C_M_RD;
      msg.len = length;
      msg.buf = rxdata;
      ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
		pr_err("msg %s i2c write error: %d\n", __func__, ret);
		
	return ret;
}





/***********************************************************************
    [function]: 
		           callback:                send data to ctpm by i2c interface;
    [parameters]:
			    txdata[in]:              data buffer which is used to send data;
			    length[in]:              the length of the data buffer;
    [return]:
			    FTS_TRUE:              success;
			    FTS_FALSE:             fail;
************************************************************************/
static int fts_i2c_txdata(struct i2c_client *client, u8 *txdata, int length)
{
	int ret;

	struct i2c_msg msg;

      msg.addr = client->addr;
      msg.flags = 0;
      msg.len = length;
      msg.buf = txdata;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}




/***********************************************************************
    [function]: 
		           callback:            gather the finger information and calculate the X,Y
		                                   coordinate then report them to the input system;
    [parameters]:
                         null;
    [return]:
                         null;
************************************************************************/
static int fts_adjust_button(struct FTS_TS_DATA_T *ft5x0x_ts, int x, int y, int *val)
{
	switch(ft5x0x_ts->vendor) {
	case VENDOR_WGJ_1024:
	default:
		if(x > ft5x0x_ts->xres) {
			*val = y;
			return 1;
		}
		break;
	case VENDOR_GBGD:
		if(y > ft5x0x_ts->yres) {
			*val = x;
			return 1;
		}
		break;
	}
	return 0;
}

static void fts_adjust_pointer(struct FTS_TS_DATA_T *ft5x0x_ts, REPORT_FINGER_INFO_T *touch_info, int count)
{
	int i;
	if(ft5x0x_ts->reverse == 0) return ;
	for(i=0; i<count; i++,touch_info++) {
		if(ft5x0x_ts->reverse & REVERSE_XY) {
			short t =  touch_info->i2_x;
			touch_info->i2_x = touch_info->i2_y;
			touch_info->i2_x = t;
		}
		if(ft5x0x_ts->reverse& REVERSE_X) {
			touch_info->i2_x = ft5x0x_ts->xres - touch_info->i2_x;
		}
		if(ft5x0x_ts->reverse & REVERSE_Y) {
			touch_info->i2_y = ft5x0x_ts->yres - touch_info->i2_y;
		}
	}
}

static int fts_read_data(struct FTS_TS_DATA_T *ft5x0x_ts)
{
	enum{
		XH = 0, XL = 1,
		YH = 2, YL = 3,
		PRESSURE = 4,
	};
	struct FTS_TS_DATA_T *data = i2c_get_clientdata(ft5x0x_ts->client);
	u8 buf[32] = {0};
	u8 *val=NULL;

	int i,j;
	int touch_point_num = 0;	//检测到的点数，包括key
	int touch_event=0;		//触摸屏的点数
	int button_val;
	REPORT_FINGER_INFO_T touch_info[CFG_MAX_POINT_NUM];
	struct FTS_BUTTON_INFO *button ;
	static bool sTSReported = false;

	fts_register_read(ft5x0x_ts->client, 0, buf, 0x1E);
	touch_point_num = buf[2]%0xf;
	//printk(KERN_EMERG"touch_point_num=%d\n", touch_point_num);
	val = &buf[3];
	for(i=0; i<touch_point_num; i++,val+=6) {
		int eventflag = val[XH]>>6;
		int id = val[YH]>>4;
		int x,y;
		int pressure;
		x = val[XH]&0xf; x = (x<<8)|val[XL];
		y=val[YH]&0xf; y = (y<<8)|val[YL];
		pressure = val[PRESSURE]&0x3f;
		//printk(KERN_EMERG"x=%d, y=%d\n", x, y);
		if(fts_adjust_button(ft5x0x_ts, x, y, &button_val)) {
			j = 0;
			for(j=0,button=ft5x0x_ts->button;button && button->keycode; j++,button++) {
				if(button_val>=button->ymin && button_val<button->ymax) {
					tsp_keystatus[j] |= KEYFLAG_DETECTED;
					break;
				}
			}
		} else {
			touch_info[touch_event].flag = eventflag;
			touch_info[touch_event].ui2_id = id;
			touch_info[touch_event].i2_x = x;
			touch_info[touch_event].i2_y = y;
			touch_info[touch_event].u2_pressure = 255;//pressure;
			touch_event ++;
			//printk("flag=%x, id=%d, x=%d, y=%d, pressure=%d\n", touch_info[i].flag, touch_info[i].ui2_id, touch_info[i].i2_x, touch_info[i].i2_y, touch_info[i].u2_pressure);
		}
	}
	// 先判断 key 是否需要发送
	for(j=0,button=ft5x0x_ts->button; button&&button->keycode; j++,button++) {
		//printk("key %d flag=%x\n", j, tsp_keystatus[j]);
		//if(tsp_keystatus[j] == 0) continue;
		if(tsp_keystatus[j] & KEYFLAG_REPORTED) {
			// 已经report了，判断是否已经弹起
			if(!(tsp_keystatus[j] & KEYFLAG_DETECTED)) {
				// report key up
				input_report_key(data->input_dev, button->keycode, 0);
				input_sync(data->input_dev);
				tsp_keystatus[j] = 0;
				//printk("report key %d up\n", button->keycode);
			}
		} else {
			if(tsp_keystatus[j] & KEYFLAG_DETECTED) {
				input_report_key(data->input_dev, button->keycode, 1);
				input_sync(data->input_dev);
				tsp_keystatus[j] |= KEYFLAG_REPORTED;
				//printk("report key %d down\n", button->keycode);
			}
		}
		tsp_keystatus[j] &= ~KEYFLAG_DETECTED;
	}
	// 发送touch
	if(touch_event == 0) {
		if(sTSReported) {
			//input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			//input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 0);
			input_report_abs(data->input_dev, ABS_PRESSURE, 0);
			input_report_key(data->input_dev, BTN_TOUCH, 0);
			input_mt_sync(data->input_dev);
			//printk("report all touch up\n");
		}
		sTSReported = false;
	} else {
		fts_adjust_pointer(ft5x0x_ts, touch_info, touch_event);
		for(i=0; i<touch_event; i++) {
			//if(touch_info[i].flag != 0) continue;
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, touch_info[i].i2_x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, touch_info[i].i2_y);
			//input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 255);
			//input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 255);
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, touch_info[i].ui2_id);			
			input_report_abs(data->input_dev, ABS_PRESSURE, touch_info[i].u2_pressure);
			input_report_key(data->input_dev, BTN_TOUCH, 1);
			input_mt_sync(data->input_dev);
			//printk(KERN_EMERG"report: flag=%x, id=%d, x=%d, y=%d, pressure=%d\n", touch_info[i].flag, touch_info[i].ui2_id, touch_info[i].i2_x, touch_info[i].i2_y, touch_info[i].u2_pressure);
			sTSReported = true;
		}
	}
	input_sync(data->input_dev);
	return 0;
}



static void fts_work_func(struct work_struct *work)
{
    //printk("\n---work func ---\n");
    struct FTS_TS_DATA_T *ft5x0x_ts = container_of(work,
		    struct FTS_TS_DATA_T,
		    pen_event_work);
    fts_read_data(ft5x0x_ts);
    enable_irq(ft5x0x_ts->client->irq);
}




static irqreturn_t fts_ts_irq(int irq, void *dev_id)
{
    struct FTS_TS_DATA_T *ft5x0x_ts = dev_id;
    if (!work_pending(&ft5x0x_ts->pen_event_work)) {
    	disable_irq_nosync(irq);
        queue_work(ft5x0x_ts->ts_workqueue, &ft5x0x_ts->pen_event_work);
    }

    return IRQ_HANDLED;
}



/***********************************************************************
[function]: 
                      callback:         send a command to ctpm.
[parameters]:
			  btcmd[in]:       command code;
			  btPara1[in]:     parameter 1;    
			  btPara2[in]:     parameter 2;    
			  btPara3[in]:     parameter 3;    
                      num[in]:         the valid input parameter numbers, 
                                           if only command code needed and no 
                                           parameters followed,then the num is 1;    
[return]:
			  FTS_TRUE:      success;
			  FTS_FALSE:     io fail;
************************************************************************/
static bool cmd_write(struct i2c_client *client, u8 btcmd,u8 btPara1,u8 btPara2,u8 btPara3,u8 num)
{
    u8 write_cmd[4] = {0};

    write_cmd[0] = btcmd;
    write_cmd[1] = btPara1;
    write_cmd[2] = btPara2;
    write_cmd[3] = btPara3;
    return i2c_write_interface(client, write_cmd, num);
}




/***********************************************************************
[function]: 
                      callback:         write a byte data  to ctpm;
[parameters]:
			  buffer[in]:       write buffer;
			  length[in]:      the size of write data;    
[return]:
			  FTS_TRUE:      success;
			  FTS_FALSE:     io fail;
************************************************************************/
static bool byte_write(struct i2c_client *client, u8* buffer, int length)
{
    
    return i2c_write_interface(client, buffer, length);
}




/***********************************************************************
[function]: 
                      callback:         read a byte data  from ctpm;
[parameters]:
			  buffer[in]:       read buffer;
			  length[in]:      the size of read data;    
[return]:
			  FTS_TRUE:      success;
			  FTS_FALSE:     io fail;
************************************************************************/
static bool byte_read(struct i2c_client *client, u8* buffer, int length)
{
    return i2c_read_interface(client, buffer, length);
}





#define    FTS_PACKET_LENGTH        128

static unsigned char CTPM_FW[]=
{
//#include "ft_app.i"
};



/***********************************************************************
[function]: 
                        callback:          burn the FW to ctpm.
[parameters]:
			    pbt_buf[in]:     point to Head+FW ;
			    dw_lenth[in]:   the length of the FW + 6(the Head length);    
[return]:
			    ERR_OK:          no error;
			    ERR_MODE:      fail to switch to UPDATE mode;
			    ERR_READID:   read id fail;
			    ERR_ERASE:     erase chip fail;
			    ERR_STATUS:   status error;
			    ERR_ECC:        ecc error.
************************************************************************/
E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(struct i2c_client *client, u8* pbt_buf, int dw_lenth)
{
    u8  cmd,reg_val[2] = {0};
    u8  packet_buf[FTS_PACKET_LENGTH + 6];
    u8  auc_i2c_write_buf[10];
    u8  bt_ecc;
	
    int  j,temp,lenght,i_ret,packet_number, i = 0;
    int  i_is_new_protocol = 0;
	

    /******write 0xaa to register 0xfc******/
    cmd=0xaa;
    fts_register_write(client, 0xfc,&cmd);
    mdelay(50);
	
     /******write 0x55 to register 0xfc******/
    cmd=0x55;
    fts_register_write(client, 0xfc,&cmd);
    printk("[TSP] Step 1: Reset CTPM test\n");
   
    mdelay(10);   


    /*******Step 2:Enter upgrade mode ****/
    printk("\n[TSP] Step 2:enter new update mode\n");
    auc_i2c_write_buf[0] = 0x55;
    auc_i2c_write_buf[1] = 0xaa;
    do
    {
        i ++;
        i_ret = fts_i2c_txdata(client, auc_i2c_write_buf, 2);
        mdelay(5);
    }while(i_ret <= 0 && i < 10 );

    if (i > 1)
    {
        i_is_new_protocol = 1;
    }

    /********Step 3:check READ-ID********/        
    cmd_write(client, 0x90,0x00,0x00,0x00,4);
    byte_read(client, reg_val,2);
    if (reg_val[0] == 0x79 && reg_val[1] == 0x3)
    {
        printk("[TSP] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
    }
    else
    {
        return ERR_READID;
        //i_is_new_protocol = 1;
    }
    

     /*********Step 4:erase app**********/
    if (i_is_new_protocol)
    {
        cmd_write(client, 0x61,0x00,0x00,0x00,1);
    }
    else
    {
        cmd_write(client, 0x60,0x00,0x00,0x00,1);
    }
    mdelay(1500);
    printk("[TSP] Step 4: erase. \n");



    /*Step 5:write firmware(FW) to ctpm flash*/
    bt_ecc = 0;
    printk("[TSP] Step 5: start upgrade. \n");
    dw_lenth = dw_lenth - 8;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    for (j=0;j<packet_number;j++)
    {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(lenght>>8);
        packet_buf[5] = (FTS_BYTE)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++)
        {
            packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }
        
        byte_write(client, &packet_buf[0],FTS_PACKET_LENGTH + 6);
        mdelay(FTS_PACKET_LENGTH/6 + 1);
        if ((j * FTS_PACKET_LENGTH % 1024) == 0)
        {
              printk("[TSP] upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
    {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;

        for (i=0;i<temp;i++)
        {
            packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }

        byte_write(client, &packet_buf[0],temp+6);    
        mdelay(20);
    }

    /***********send the last six byte**********/
    for (i = 0; i<6; i++)
    {
        temp = 0x6ffa + i;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        temp =1;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;
        packet_buf[6] = pbt_buf[ dw_lenth + i]; 
        bt_ecc ^= packet_buf[6];

        byte_write(client, &packet_buf[0],7);  
        mdelay(20);
    }

    /********send the opration head************/
    cmd_write(client, 0xcc,0x00,0x00,0x00,1);
    byte_read(client, reg_val,1);
    printk("[TSP] Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc)
    {
        return ERR_ECC;
    }

    /*******Step 7: reset the new FW**********/
    cmd_write(client, 0x07,0x00,0x00,0x00,1);

    return ERR_OK;
}




int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client)
{
   u8*     pbt_buf = FTS_NULL;
   int i_ret;
    
   pbt_buf = CTPM_FW;
   i_ret =  fts_ctpm_fw_upgrade(client, pbt_buf,sizeof(CTPM_FW));
   
   return i_ret;
}

unsigned char fts_ctpm_get_upg_ver(void)
{
    unsigned int ui_sz;
	
    ui_sz = sizeof(CTPM_FW);
    if (ui_sz > 2)
    {
        return CTPM_FW[ui_sz - 2];
    }
    else
        return 0xff; 
 
}

static void fts_check_vendor(struct FTS_TS_DATA_T *ft5x0x_ts)
{
	struct i2c_client *client = ft5x0x_ts->client;
	unsigned char reg_value;
	unsigned char reg_version;
	unsigned char reg_vendor;
	unsigned char reg_thres;
	int cnt_detect;
	reg_vendor = 0;
	cnt_detect = 10;
	while(!fts_register_read(client, FT5X0X_REG_VENRODID, &reg_vendor,1) && (cnt_detect>0)) {
		printk(KERN_EMERG"[TSP] Read vendor error ...\n");
		msleep(20);
		cnt_detect --;
	}
	fts_register_read(client, FT5X0X_REG_FIRMID, &reg_version,1);
	printk("[TSP] firmware version = 0x%2x\n", reg_version);
	fts_register_read(client, FT5X0X_REG_REPORT_RATE, &reg_value,1);
	printk("[TSP]firmware report rate = %dHz\n", reg_value*10);
	fts_register_read(client, FT5X0X_REG_THRES, &reg_thres,1);
	printk("[TSP]firmware threshold = %d\n", reg_thres * 4);
	fts_register_read(client, FT5X0X_REG_NOISE_MODE, &reg_value,1);
	printk("[TSP]nosie mode = 0x%2x\n", reg_value);
	printk("[TSP]vendor = 0x%2x, reg_thres=%x\n", reg_vendor, reg_thres);
	switch(reg_vendor) {
	case 0x79:
	default:
		ft5x0x_ts->max_touch_num = 5;
		switch(reg_thres) {
		case 0x11:
		case 0x14:
			ft5x0x_ts->vendor = VENDOR_WGJ_800;
			ft5x0x_ts->xres = 800;
			ft5x0x_ts->yres = 480;
			ft5x0x_ts->reverse = 0;
			ft5x0x_ts->button = tsp_keycodes_WGJ_800;
			break;
		default:
			ft5x0x_ts->vendor = VENDOR_WGJ_1024;
			ft5x0x_ts->xres = 1024;
			ft5x0x_ts->yres = 600;
			ft5x0x_ts->reverse = 0;
			ft5x0x_ts->button = tsp_keycodes_WGJ_1024;
		}
		break;
	case 0x9f:
		ft5x0x_ts->max_touch_num = 5;
		ft5x0x_ts->vendor = VENDOR_GBGD;
		ft5x0x_ts->xres = 800;
		ft5x0x_ts->yres = 480;
		ft5x0x_ts->reverse = 0;
		ft5x0x_ts->button = tsp_keycodes_GBGD;
		break;
	}
	//printk(KERN_EMERG"vendor=%x, VENDOR=%d\n", reg_value, ft5x0x_ts->vendor);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void fts_ts_early_suspend(struct early_suspend *h)
{
	struct FTS_TS_DATA_T *ft5x0x_ts = 
		container_of(h, struct FTS_TS_DATA_T, early_suspend);
	struct touch_panel_platform_data* tP_platform_data = (struct touch_panel_platform_data*)ft5x0x_ts->client->dev.platform_data;
	int reset_gpio = irq_to_gpio(tP_platform_data->irq_reset);

	disable_irq_nosync(ft5x0x_ts->client->irq);
	
	gpio_request(reset_gpio, "TP_RESET");
	gpio_direction_output(reset_gpio,  0);
	gpio_free(reset_gpio);
	
	axp_touchscreen(0);
	printk(KERN_WARNING "[%d:%s]\n", __LINE__, __FUNCTION__);
}

static void fts_ts_late_resume(struct early_suspend *h)
{
	struct FTS_TS_DATA_T *ft5x0x_ts = 
		container_of(h, struct FTS_TS_DATA_T, early_suspend);
	struct touch_panel_platform_data* tP_platform_data = (struct touch_panel_platform_data*)ft5x0x_ts->client->dev.platform_data;
	int reset_gpio = irq_to_gpio(tP_platform_data->irq_reset);

	axp_touchscreen(1);
	
	gpio_request(reset_gpio, "TP_RESET");
	gpio_direction_output(reset_gpio,  1);
	gpio_free(reset_gpio);
	
	enable_irq(ft5x0x_ts->client->irq);
	printk(KERN_WARNING "[%d:%s]\n", __LINE__, __FUNCTION__);
}
#endif


static int __devinit fts_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct FTS_TS_DATA_T *ft5x0x_ts;
	struct FTS_BUTTON_INFO *button;
	struct input_dev *input_dev;
	struct touch_panel_platform_data* tP_platform_data = (struct touch_panel_platform_data*)client->dev.platform_data;
	int reset_gpio = irq_to_gpio(tP_platform_data->irq_reset);
	int irq_gpio = irq_to_gpio(client->irq);
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	// reset
	err = gpio_request(reset_gpio, "TP_RESET");
	if(err < 0){
		dev_err(&client->dev, "Failed to request GPIO:%d, ERRNO:%d\n", reset_gpio,err);
		goto exit_check_functionality_failed;
	}
	gpio_direction_output(reset_gpio,  0);
	mdelay(50);
	gpio_direction_output(reset_gpio,  1);
	gpio_free(reset_gpio);
	// set irq gpio to input
	err = gpio_request(irq_gpio, "TS_INT");
	if(err < 0){
		dev_err(&client->dev, "Failed to request irq GPIO:%d, ERRNO:%d\n", reset_gpio,err);
		goto exit_check_functionality_failed;
	}
	//gpio_direction_output(irq_gpio, 0);
	//msleep(5);
	gpio_direction_input(irq_gpio);
	gpio_free(irq_gpio);

	ft5x0x_ts = kzalloc(sizeof(*ft5x0x_ts), GFP_KERNEL);
	if (!ft5x0x_ts)    {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}
	ft5x0x_ts->client = client;
	i2c_set_clientdata(client, ft5x0x_ts);

	INIT_WORK(&ft5x0x_ts->pen_event_work, fts_work_func);

	ft5x0x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft5x0x_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}


	/***wait CTP to bootup normally***/
	msleep(200); 
	fts_check_vendor(ft5x0x_ts);

#if 0
	if (fts_ctpm_get_upg_ver() != reg_version)  //upgrade touch screen firmware if not the same with the firmware in host flashs
	{
		printk("[TSP] start upgrade new verison 0x%2x\n", fts_ctpm_get_upg_ver());
		msleep(200);
		err =  fts_ctpm_fw_upgrade_with_i_file(client);
		if (err == 0)
		{
			printk("[TSP] ugrade successfuly.\n");
			msleep(300);
			fts_read_reg(FT5X0X_REG_FIRMID, &reg_value);
			printk("FTS_DBG from old version 0x%2x to new version = 0x%2x\n", reg_version, reg_value);
		}
		else
		{
			printk("[TSP]  ugrade fail err=%d, line = %d.\n",
			err, __LINE__);
		}
		msleep(4000);
	}
#endif
	err = request_irq(client->irq, fts_ts_irq, IRQF_TRIGGER_FALLING, client->name, ft5x0x_ts);
	if (err < 0) {
		dev_err(&client->dev, "ft5x0x_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	disable_irq(client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	ft5x0x_ts->input_dev = input_dev;

	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	input_dev->absbit[0] =  BIT_MASK(ABS_MT_TRACKING_ID) |
		BIT_MASK(ABS_MT_POSITION_X) | BIT_MASK(ABS_MT_POSITION_Y)|
		BIT(ABS_PRESSURE);

	/*
		android 4.0 new feature, kernel should set device properties and quirks 
	*/
	input_dev->propbit[0] = BIT(INPUT_PROP_DIRECT);
	for(button=ft5x0x_ts->button; button&&button->keycode; button++) {
		input_set_capability(input_dev, EV_KEY, button->keycode);
	}
	memset(tsp_keystatus, 0x00, sizeof(tsp_keystatus));

	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	//input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	//input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ft5x0x_ts->xres, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ft5x0x_ts->yres, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, ft5x0x_ts->max_touch_num, 0, 0);


	input_dev->name        = "touchscreen";
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"fts_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	ft5x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	ft5x0x_ts->early_suspend.suspend = fts_ts_early_suspend;
	ft5x0x_ts->early_suspend.resume = fts_ts_late_resume;
	register_early_suspend(&ft5x0x_ts->early_suspend);
#endif

	enable_irq(client->irq);    
	printk("[TSP] file(%s), function (%s), irq=%d-- end\n", __FILE__, __FUNCTION__, client->irq);
	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(client->irq, ft5x0x_ts);
exit_irq_request_failed:
	cancel_work_sync(&ft5x0x_ts->pen_event_work);
	destroy_workqueue(ft5x0x_ts->ts_workqueue);
exit_create_singlethread:
	printk("[TSP] ==singlethread error =\n");
	i2c_set_clientdata(client, NULL);
	kfree(ft5x0x_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}



static int __devexit fts_ts_remove(struct i2c_client *client)
{
    struct FTS_TS_DATA_T *ft5x0x_ts;
    
    ft5x0x_ts = (struct FTS_TS_DATA_T *)i2c_get_clientdata(client);
    free_irq(ft5x0x_ts->client->irq, ft5x0x_ts);
    input_unregister_device(ft5x0x_ts->input_dev);
    kfree(ft5x0x_ts);
    cancel_work_sync(&ft5x0x_ts->pen_event_work);
    destroy_workqueue(ft5x0x_ts->ts_workqueue);
    i2c_set_clientdata(client, NULL);
    return 0;
}

static int fts_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int fts_ts_resume(struct i2c_client *client)
{
	return 0;
}


static const struct i2c_device_id ft5x0x_ts_id[] = {
    { FT5X0X_NAME, 0 },{ }
};


MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);

static struct i2c_driver fts_ts_driver = {
    .probe        = fts_ts_probe,
    .remove        = __devexit_p(fts_ts_remove),
    .suspend      = fts_ts_suspend,
    .resume       = fts_ts_resume,
    .id_table    = ft5x0x_ts_id,
    .driver    = {
        .name = FT5X0X_NAME,
    },
};

static int __init fts_ts_init(void)
{
    return i2c_add_driver(&fts_ts_driver);
}


static void __exit fts_ts_exit(void)
{
    i2c_del_driver(&fts_ts_driver);
}



module_init(fts_ts_init);
module_exit(fts_ts_exit);

MODULE_AUTHOR("<duxx@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");
