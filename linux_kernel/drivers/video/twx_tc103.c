///FOR MINILVDS Driver TWX_TC103

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h> 
#include <linux/i2c.h>
//#include <linux/i2c-dev.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <asm/uaccess.h>
//#include <mach/pinmux.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include "t3data.h"
//TTL in  LVDS out

#define TC103B_I2C_CLK_RATE		(400*1000)



#define TWX_TC103_I2C_NAME "twx_tc103"
extern void T3_PWMBL_CTR(int vla);

//input: 0 �ص����� 1���򿪱���
extern void TC103_BLCTROL(int Vale);

static void twx_early_suspend(struct early_suspend *h);
static void twx_late_resume(struct early_suspend *h);
	
static struct i2c_client *twx_tc103_i2c_client;
static struct i2c_client *twx_tc103_bl_i2c_client;

static unsigned char gamaSelect = 0x03;
static unsigned char dlightMode = 0;
static unsigned bl_level = 255;

static int twx_i2c_write(struct i2c_client *i2client,unsigned char *buff, unsigned len)
{
    int res = 0;
/*    struct i2c_msg msg[] = {
        {
        .addr = i2client->addr,
        .flags = 0,
        .len = len,
        .buf = buff,
        }
    };
    //printk( "tc103 addr %#x reg:%#x  value:%#x \n",twx_tc103_client->addr,buff[0],buff[1]);	
    res = i2c_transfer(i2client->adapter, msg, 1);
    if (res < 0) {
        pr_err("%s: i2c transfer failed\n", __FUNCTION__);
    }
    */
    res = i2c_master_send(i2client,buff,len);
	if(res < 0)
		printk(KERN_EMERG"twx_i2c_write fail\n");
    return res;
}

static int twx_i2c_read(struct i2c_client *i2client,unsigned char *buff, unsigned len)
{
    int res = 0;
    struct i2c_msg msgs[] = {
        {
            .addr = i2client->addr,
            .flags = 0,
            .len = 1,
            .buf = buff,
       
        },
        {
            .addr = i2client->addr,
            .flags = I2C_M_RD,
            .len = len,
            .buf = buff,
        }
    };
    res = i2c_transfer(i2client->adapter, msgs, 2);
    if (res < 0) {
        pr_err("%s: i2c transfer failed\n", __FUNCTION__);
    }

    return res;
}



static u8 twx_recv(struct i2c_client *i2client,unsigned short addr)

{

   int res = 0;
 	u8 buf[2]= {addr >> 8, addr&0xff };
	u8 data = 0x69;
    struct i2c_msg msgs[] = {
        {
            .addr = i2client->addr,
            .flags = 0,
            .len = 2,
            .buf = buf,
        },
        {
            .addr = i2client->addr,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = &data,
        }
    };
    res = i2c_transfer(i2client->adapter, msgs, 2);
	pr_err("i2c return=%d\n", res);
   if (res < 0) {
        pr_err("%s: i2c transfer failed\n", __FUNCTION__);
   }
   else
		res = data;

    return res;
}


/**
  for 
*/
void initial_dithering_table(struct i2c_client *i2client)
{
	unsigned int i,j;
	//unsigned char tAddr;
	unsigned char tData[2];
	
	for(i=0;i<0x0c;i++)
	{
		tData[0] =0x00;
                tData[1] = i;	
                twx_i2c_write(i2client,tData,2);				
		for(j=0;j<64;j++)
		{
			tData[0]=0xc0+j;
                        tData[1] = Dith_Table[i][j];	
                        twx_i2c_write(i2client,tData,2);							
		}	
	}	
    tData[0]=0x00;	
	tData[1] = 0x10;
        twx_i2c_write(i2client,tData,2);	        					
};

/*
  for dblight db light addr
*/
void initial_gamma_table(struct i2c_client *client,char vale)
{
	unsigned int i,j;
	//unsigned char tAddr;
	unsigned char tData[65];
//printk("twx_tc103_init.bl .i2c initial_gamma_table vale = %d gamaSelect=%d adr=%x\n",vale,gamaSelect,client->addr);   
	if(vale)
	{
		
            if(gamaSelect != 0)
            {
              for(i=0;i<0x04;i++)
			{
				tData[0]=0x4f;
	            tData[1] = i;	
	            twx_i2c_write(client,tData,2);                        			
				tData[0]=0xc0;
				for(j=0;j<64;j++)
				{
	                tData[1+j] = DBL_BANK_GAMMA_TABLE_DBL[i][j];		
				}	
				twx_i2c_write(client,tData,65);	
				#if 0
				msleep(1);
				printk("1111 0x4F  %d\n",i);
				
				for(j=0;j<65;j++)
					tData[j] = 0;
				
				tData[0] = 0xC0;
				twx_i2c_read(client,tData,64);
				for(j=0;j<64;j++)
					printk("%02x, ",tData[j]);

				printk("\n");
				#endif
			}
               }
               gamaSelect =0;
	}
	else 
	{
	       if(gamaSelect != 1)
              {
               for(i=0;i<0x04;i++)
				{
					tData[0] = 0x4f;	
		            tData[1] = i;	
					twx_i2c_write(client,tData,2);			
					tData[0] =0xc0;
					for(j=0;j<64;j++)
					{
		                tData[j+1] = DBL_BANK_GAMMA_TABLE[i][j];		
					}	
					twx_i2c_write(client,tData,65);		
					#if 0
					msleep(1);
					printk("222 0x4F  %d\n",i);
					
					for(j=0;j<65;j++)
						tData[j] = 0;
					
					tData[0] = 0xC0;
					twx_i2c_read(client,tData,64);
					for(j=0;j<64;j++)
						printk("%02x,",tData[j]);
					
					printk("\n");
					#endif
				}	
               }	
                gamaSelect = 1;
	}	
         			
	tData[1] = 0x04;
    tData[0] = 0x4f;
	twx_i2c_write(client,tData,2);	
        msleep(200);				
};


void IIC_Read_forT101(unsigned char addr, unsigned char reg, unsigned char *buff, int len)
{
   unsigned char tData[3];
   tData[0]=reg;
   tData[1]=buff[0];
   if(addr == 0x6e)
   {
     twx_i2c_read(twx_tc103_i2c_client,tData, 1);
   }
    else
   {
     twx_i2c_read(twx_tc103_bl_i2c_client,tData, 1);
    }
}

void IIC_Write_forT101(unsigned char addr, unsigned char reg, unsigned char *buff, int len)
{
   unsigned char tData[3];
   tData[0]=reg;
   tData[1]=buff[0];
   if(addr == 0x6e)
   {
     twx_i2c_write(twx_tc103_i2c_client,tData, 1);
   }
    else
   {
     twx_i2c_write(twx_tc103_bl_i2c_client,tData, 1);
    }
}
//input pin:  0:com0 1:com1 2:seg0 3:seg1 4:seg2 5:seg3 6:seg4 7:seg5
//	vale: 0:�ر� 1:����� 2:�����
//�ر�˵����com0 û�з���򿪡�
//output:��
void COMSEGSet(int Pin,int vale)
{
	unsigned char tData[2];
	switch(Pin)
	{
		case 0:
			if(vale==0)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x98, tData, 1);
				tData[0] |= 0x08;
				IIC_Write_forT101(T103ADDR_TCON, 0x98, tData, 1);					
			}
			else if(vale==1)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x98, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x98, tData, 1);				
			}
			else if(vale==2)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x98, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x98, tData, 1);					
			}
			else
				;
			break;
		case 1:
			if(vale==0)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x97, tData, 1);
				tData[0] |= 0x08;
				IIC_Write_forT101(T103ADDR_TCON, 0x97, tData, 1);					
			}
			else if(vale==1)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x97, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x97, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2e, tData, 1);
				tData[0] &= 0xdf;
				IIC_Write_forT101(T103ADDR_TCON, 0x2e, tData, 1);				
			}
			else if(vale==2)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x97, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x97, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2e, tData, 1);
				tData[0] |= 0x20;
				IIC_Write_forT101(T103ADDR_TCON, 0x2e, tData, 1);										
			}
			else
				;
			break;
		case 2:
			if(vale==0)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x9A, tData, 1);
				tData[0] |= 0x08;
				IIC_Write_forT101(T103ADDR_TCON, 0x9A, tData, 1);					
			}
			else if(vale==1)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x9A, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x9A, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2F, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x2F, tData, 1);				
			}
			else if(vale==2)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x9A, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x9A, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2F, tData, 1);
				tData[0] |= 0x08;
				IIC_Write_forT101(T103ADDR_TCON, 0x2F, tData, 1);										
			}
			else
				;			
			break;
		case 3:
			if(vale==0)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x99, tData, 1);
				tData[0] |= 0x08;
				IIC_Write_forT101(T103ADDR_TCON, 0x99, tData, 1);					
			}
			else if(vale==1)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x99, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x99, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2e, tData, 1);
				tData[0] &= 0xbf;
				IIC_Write_forT101(T103ADDR_TCON, 0x2e, tData, 1);				
			}
			else if(vale==2)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x99, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x99, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2e, tData, 1);
				tData[0] |= 0x40;
				IIC_Write_forT101(T103ADDR_TCON, 0x2e, tData, 1);										
			}
			else
				;				
			break;
		case 4:
			if(vale==0)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x98, tData, 1);
				tData[0] |= 0x80;
				IIC_Write_forT101(T103ADDR_TCON, 0x98, tData, 1);					
			}
			else if(vale==1)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x98, tData, 1);
				tData[0] &= 0x7f;
				IIC_Write_forT101(T103ADDR_TCON, 0x98, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2f, tData, 1);
				tData[0] &= 0xfb;
				IIC_Write_forT101(T103ADDR_TCON, 0x2f, tData, 1);				
			}
			else if(vale==2)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x98, tData, 1);
				tData[0] &= 0x7f;
				IIC_Write_forT101(T103ADDR_TCON, 0x98, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2f, tData, 1);
				tData[0] |= 0x04;
				IIC_Write_forT101(T103ADDR_TCON, 0x2f, tData, 1);										
			}
			else
				;		
			break;
		case 5:
			if(vale==0)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x94, tData, 1);
				tData[0] |= 0x08;
				IIC_Write_forT101(T103ADDR_TCON, 0x94, tData, 1);					
			}
			else if(vale==1)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x94, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x94, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2f, tData, 1);
				tData[0] &= 0xbf;
				IIC_Write_forT101(T103ADDR_TCON, 0x2f, tData, 1);				
			}
			else if(vale==2)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x94, tData, 1);
				tData[0] &= 0xf7;
				IIC_Write_forT101(T103ADDR_TCON, 0x94, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2f, tData, 1);
				tData[0] |= 0x40;
				IIC_Write_forT101(T103ADDR_TCON, 0x2f, tData, 1);										
			}
			else
				;		
			break;
		case 6:
			if(vale==0)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x99, tData, 1);
				tData[0] |= 0x80;
				IIC_Write_forT101(T103ADDR_TCON, 0x99, tData, 1);					
			}
			else if(vale==1)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x99, tData, 1);
				tData[0] &= 0x7f;
				IIC_Write_forT101(T103ADDR_TCON, 0x99, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2e, tData, 1);
				tData[0] &= 0x7f;
				IIC_Write_forT101(T103ADDR_TCON, 0x2e, tData, 1);				
			}
			else if(vale==2)
			{
				IIC_Read_forT101(T103ADDR_TCON, 0x99, tData, 1);
				tData[0] &= 0x7f;
				IIC_Write_forT101(T103ADDR_TCON, 0x99, tData, 1);
				
				IIC_Read_forT101(T103ADDR_TCON, 0x2e, tData, 1);
				tData[0] |= 0x80;
				IIC_Write_forT101(T103ADDR_TCON, 0x2e, tData, 1);										
			}
			else
				;			
			break;
			
		default:break;	
	}
}



//input: ��mode = 0;Ϊһ���ɺģʽ��valeѡ������һ��seg�źţ�(1--6::COM1;SEG1,SEG2,SEG3,SEG4,SEG5;)
//	   mode = 1;Ϊ�����ɺ������ģʽ��ֻ��һ��״̬������vale�������ã�
//	   mode = 2;Ϊ�����ɺ�ĺ���ģʽ��valeֵ��Χ��1��5��5��״̬��ѡ;
//vale = 0,Ϊ�رչ�ɺ�����������ѡ���ɺ;
//output:��
void T2_Panel3DLense(int mode,int vale)
{
	unsigned char tData[2];
	if(mode==2)
	{
		switch (vale)
		{
			case 1:
				COMSEGSet(0,1);
				COMSEGSet(1,1);
				COMSEGSet(2,2);
				COMSEGSet(3,2);
				COMSEGSet(4,2);
				COMSEGSet(5,1);
				COMSEGSet(6,1);
				break;
			case 2:
				COMSEGSet(0,1);
				COMSEGSet(1,1);
				COMSEGSet(2,1);
				COMSEGSet(3,2);
				COMSEGSet(4,2);
				COMSEGSet(5,2);
				COMSEGSet(6,1);			
				break;
			case 3:
				COMSEGSet(0,1);
				COMSEGSet(1,1);
				COMSEGSet(2,1);
				COMSEGSet(3,1);
				COMSEGSet(4,2);
				COMSEGSet(5,2);
				COMSEGSet(6,2);				
				break;
			case 4:
				COMSEGSet(0,1);
				COMSEGSet(1,1);
				COMSEGSet(2,2);
				COMSEGSet(3,1);
				COMSEGSet(4,1);
				COMSEGSet(5,2);
				COMSEGSet(6,2);				
				break;
			case 5:
				COMSEGSet(0,1);
				COMSEGSet(1,1);
				COMSEGSet(2,2);
				COMSEGSet(3,2);
				COMSEGSet(4,1);
				COMSEGSet(5,1);
				COMSEGSet(6,2);			
				break;
			default:break;	
		}		
	}
	else if(mode==1)
	{
		COMSEGSet(0,1);
		COMSEGSet(1,2);
		COMSEGSet(2,1);
		COMSEGSet(3,1);
		COMSEGSet(4,1);
		COMSEGSet(5,1);	
		COMSEGSet(6,1);
	}
	else if(mode==0)
	{
		if(vale!=0)
		{
			COMSEGSet(0,0);
			COMSEGSet(1,0);
			COMSEGSet(2,0);
			COMSEGSet(3,0);
			COMSEGSet(4,0);
			COMSEGSet(5,0);	
			COMSEGSet(6,0);			
			COMSEGSet(0,1);
			COMSEGSet(vale,2);
		}	
	}
	else
        {
  
        }


	if(vale!=0)
	{
		IIC_Read_forT101(T103ADDR_TCON, 0x2f, tData, 1);
		tData[0] |= 0x80;
		IIC_Write_forT101(T103ADDR_TCON, 0x2f, tData, 1);		
	}	
	else
	{
		IIC_Read_forT101(T103ADDR_TCON, 0x2f, tData, 1);
		tData[0] &= 0x3f;	
		IIC_Write_forT101(T103ADDR_TCON, 0x2f, tData, 1);	
	}
	
}

//input: 0 �ص����� 1���򿪱���
void TC103_BLCTROL(int Vale)
{
	unsigned char tData[3];
        tData[0]=0x40;
        tData[1]=0;
        twx_i2c_read(twx_tc103_bl_i2c_client,tData, 2);
	if(Vale==0)
	{
		tData[1] = tData[0]|0x02;	
	}
	else
	{
		tData[1] = tData[0]&0x0fd;		
	}
        tData[0]=0x40;		
        twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);
};

//��֯
void TC103_Panel3DInter(int flag)
{
	//unsigned char tAddr;
	unsigned char tData[2];
	tData[1] = 0x0;
        tData[0]=0x1b;
	if(0 == flag)
	{
		tData[1] = 0x00;
	}
	else
	{		
		tData[1] = 0x03;
	}		
        twx_i2c_write(twx_tc103_i2c_client,tData,2);
}


//����������
void TC103_Panel3DSwitchLRPic(int flag)
{
	//unsigned char tAddr;
	unsigned char tData[3];
	tData[2] = 0x0;
        tData[0]=0x48;
	if(0 == flag)
	{
        tData[1] = 0x03;
	}
	else
	{		
		tData[1] = 0x02;
		
	}
        twx_i2c_write(twx_tc103_i2c_client,tData,2);
}


//��̬����
//input: 0:�� 1: ���� 2�� ��� 3: ǿʡ��
void TC103_DBL_CTR(int vla)
{
	unsigned char tData[2];
    unsigned int TC103_DBL_TBL[4]={0x00,0xc0,0x80,0x60};
	switch(vla)
	{
		case 0:			
#if 1   //�ض�̬����ʱ����T3��gamma	                
	        tData[0] = 0x01;	
			tData[1] = 0x08;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);
	      	tData[0] = 0x4e;
			tData[1] = 0x00;
#else	//�ض�̬����ʱ������IC��gamma
	                tData[0]=0x01;	
			tData[1] = 0x00;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);
	                tData[0] = 0x4e;
			tData[1] = 0x01;
#endif			
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);		
			initial_gamma_table(twx_tc103_bl_i2c_client,0);	
            tData[0] = 0x40;
	        tData[1]=0x01;
	        twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);
			break;			
		case 1:				
			tData[1] = 0x0E;
			tData[0] = 0x01;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			tData[1] = 0x00;
			tData[0] = 0x4e;	
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
				
			tData[1] = TC103_DBL_TBL[1]/256;
			tData[0] = 0x44;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);		
			tData[1] = TC103_DBL_TBL[1]%256;
			tData[0] = 0x45;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			initial_gamma_table(twx_tc103_bl_i2c_client,1);	
                        tData[0] = 0x40;
			tData[1] = 0x00;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			break;
		case 2:				
			tData[1] = 0x0E;
			tData[0] = 0x01;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			tData[1] = 0x00;
			tData[0] = 0x4e;	
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
				
			tData[1] = TC103_DBL_TBL[2]/256;
			tData[0] = 0x44;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);		
			tData[1] = TC103_DBL_TBL[2]%256;
			tData[0] = 0x45;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			initial_gamma_table(twx_tc103_bl_i2c_client,1);	
                        tData[0] = 0x40;
			tData[1] = 0x00;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			break;		
		case 3:					
			tData[1] = 0x0E;
			tData[0] = 0x01;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			tData[1] = 0x00;
			tData[0] = 0x4e;	
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
				
			tData[1] = TC103_DBL_TBL[3]/256;
			tData[0] = 0x44;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);		
			tData[1] = TC103_DBL_TBL[3]%256;
			tData[0] = 0x45;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			initial_gamma_table(twx_tc103_bl_i2c_client,1);	
            tData[0] = 0x40;
			tData[1] = 0x00;
			twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);	
			break;					
	}	
}


//input: 0--0xff 
 void T3_PWMBL_CTR(int vla)
{
    //����ǿ�ȿ���
//unsigned int TC103_PWMBL_TBL[16]=
//{0x08,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe8,0xff};
    unsigned char tData[2];
    if(twx_tc103_bl_i2c_client)
   {
    tData[0]=0x42;
    tData[1] = 0x00;
    twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);
    tData[0]=0x43;	
    tData[1] = vla;//TC103_DBL_TBL[vla];
    twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);
//    printk( "tc103 set backlight leve %d\n",vla);    
   }
   bl_level=vla;
}
EXPORT_SYMBOL_GPL(T3_PWMBL_CTR);
//input: ���������ؽ�֯��
//	��һ�� �ڶ��� ������
//type=0:RGB	GBR    BRG
//type=1:RGB    BRG    GBR
//type=2:GBR    RGB    BRG
//type=3:GBR    BRG    RGB
//type=4:BRG    RGB    GBR
//type=5:BRG    GBR    RGB
#define RGB 0x00
#define GBR 0x01
#define BRG 0x02
void T2_Panel3DInter3(int type)
{
	unsigned char tAddr[2], tData[2];
	
	IIC_Read_forT101(T103ADDR_TCON, 0x1b, tData, 1);
	tAddr[0] = tData[0];
	tData[0] = 0x00;
	
	switch(type)
	{
		case 0:	//RGB	GBR    BRG
			tAddr[0] &= 0x3f;
			tAddr[0] |= (RGB*64);
			IIC_Write_forT101(T103ADDR_TCON, 0x1b, tAddr, 1);
			tData[0] |= GBR;
			tData[0] |= (BRG*4);
			IIC_Write_forT101(T103ADDR_TCON, 0x1c, tData, 1);
			break;
		case 1:	//RGB    BRG    GBR
			tAddr[0] &= 0x3f;
			tAddr[0] |= (RGB*64);
			IIC_Write_forT101(T103ADDR_TCON, 0x1b, tAddr, 1);
			tData[0] |= BRG;
			tData[0] |= (GBR*4);
			IIC_Write_forT101(T103ADDR_TCON, 0x1c, tData, 1);
			break;	
		case 2: //GBR    RGB    BRG
			tAddr[0] &= 0x3f;
			tAddr[0] |= (GBR*64);
			IIC_Write_forT101(T103ADDR_TCON, 0x1b, tAddr, 1);
			tData[0] |= RGB;
			tData[0] |= (BRG*4);
			IIC_Write_forT101(T103ADDR_TCON, 0x1c, tData, 1);
			break;	
		case 3://GBR    BRG    RGB
			tAddr[0] &= 0x3f;
			tAddr[0] |= (GBR*64);
			IIC_Write_forT101(T103ADDR_TCON, 0x1b, tAddr, 1);
			tData[0] |= BRG;
			tData[0] |= (RGB*4);
			IIC_Write_forT101(T103ADDR_TCON, 0x1c, tData, 1);
			break;
		case 4: //BRG    RGB    GBR
			tAddr[0] &= 0x3f;
			tAddr[0] |= (BRG*64);
			IIC_Write_forT101(T103ADDR_TCON, 0x1b, tAddr, 1);
			tData[0] |= RGB;
			tData[0] |= (GBR*4);
			IIC_Write_forT101(T103ADDR_TCON, 0x1c, tData, 1);
			break;	
		case 5://BRG    GBR    RGB
			tAddr[0] &= 0x3f;
			tAddr[0] |= (BRG*64);
			IIC_Write_forT101(T103ADDR_TCON, 0x1b, tAddr, 1);
			tData[0] |= GBR;
			tData[0] |= (RGB*4);
			IIC_Write_forT101(T103ADDR_TCON, 0x1c, tData, 1);
			break;	
		default:break;	
	}	
}

//��ż����ѡ��ͬ��RGB���ݽ�֯
//input: 
//������  ż���� ��һ��
//type=0   RGB ѡ�� RGB
//type=1   RGB ѡ�� RBG
//type=2   RGB ѡ�� GRB
//type=3   RGB ѡ�� GBR
//type=4   RGB ѡ�� BRG
//type=5   RGB ѡ�� BGR
void T2_Panel3DInter1(int ODD_type,int EVEN_type)
{
	unsigned char tAddr[2], tData[2];	
	
	IIC_Read_forT101(T103ADDR_TCON, 0x19, tData, 1);
	tAddr[0] = tData[0];
	tData[0] = 0x00;
	switch(ODD_type)
	{
		case 0://RGB
			tData[0] |= 0x02;	//B_odd select B
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);
			tAddr[0] &= 0x0f;
			tAddr[0] |= 0x40;	//G_odd select G
			tAddr[0] |= 0x00;	//R_odd select R
			IIC_Write_forT101(T103ADDR_TCON, 0x19, tAddr, 1);				
		case 1://RBG
			tData[0] |= 0x01;	//B_odd select G
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);
			tAddr[0] &= 0x0f;
			tAddr[0] |= 0x80;	//G_odd select B
			tAddr[0] |= 0x00;	//R_odd select R
			IIC_Write_forT101(T103ADDR_TCON, 0x19, tAddr, 1);		
		case 2://GRB
			tData[0] |= 0x02;	//B_odd select B
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);
			tAddr[0] &= 0x0f;
			tAddr[0] |= 0x00;	//G_odd select R
			tAddr[0] |= 0x10;	//R_odd select G
			IIC_Write_forT101(T103ADDR_TCON, 0x19, tAddr, 1);		
		case 3://GBR
			tData[0] |= 0x00;	//B_odd select R
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);
			tAddr[0] &= 0x0f;
			tAddr[0] |= 0x80;	//G_odd select B
			tAddr[0] |= 0x10;	//R_odd select G
			IIC_Write_forT101(T103ADDR_TCON, 0x19, tAddr, 1);		
		case 4://BRG
			tData[0] |= 0x01;	//B_odd select G
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);
			tAddr[0] &= 0x0f;
			tAddr[0] |= 0x00;	//G_odd select R
			tAddr[0] |= 0x20;	//R_odd select B
			IIC_Write_forT101(T103ADDR_TCON, 0x19, tAddr, 1);		
		case 5://BGR
			tData[0] |= 0x00;	//B_odd select R
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);
			tAddr[0] &= 0x0f;
			tAddr[0] |= 0x40;	//G_odd select G
			tAddr[0] |= 0x20;	//R_odd select B
			IIC_Write_forT101(T103ADDR_TCON, 0x19, tAddr, 1);		
			break;
		default:break;
	}
	IIC_Read_forT101(T103ADDR_TCON, 0x1a, tData, 1);	
	switch(EVEN_type)
	{
		case 0://RGB
			tData[0] |= 0x80;	//B_EVEN select B
			tData[0] |= 0x10;	//G_EVEN select G
			tData[0] |= 0x00;	//R_EVEN select R
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);
				
		case 1://RBG
			tData[0] |= 0x40;	//B_EVEN select G
			tData[0] |= 0x20;	//G_EVEN select B
			tData[0] |= 0x00;	//R_EVEN select R
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);		
		case 2://GRB
			tData[0] |= 0x80;	//B_EVEN select B
			tData[0] |= 0x00;	//G_EVEN select R
			tData[0] |= 0x04;	//R_EVEN select G
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);			
		case 3://GBR
			tData[0] |= 0x00;	//B_EVEN select R
			tData[0] |= 0x20;	//G_EVEN select B
			tData[0] |= 0x04;	//R_EVEN select G
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);		
		case 4://BRG
			tData[0] |= 0x40;	//B_EVEN select G
			tData[0] |= 0x00;	//G_EVEN select R
			tData[0] |= 0x08;	//R_EVEN select B
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);		
		case 5://BGR
			tData[0] |= 0x00;	//B_EVEN select R
			tData[0] |= 0x10;	//G_EVEN select G
			tData[0] |= 0x08;	//R_EVEN select B
			IIC_Write_forT101(T103ADDR_TCON, 0x1a, tData, 1);		
			break;
		default:break;		
	}	
}

//�����ؽ�֯
//���ĸ�����Ϊ����,�����R0G0B0R1G1B1R2G2B2R3G3B3����ѡ�������R0G0B0R1G1B1R2G2B2R3G3B3
//input:mode=0 Ϊ�����ҽ�֯ʱ�����ؽ�֯������Ϊû�����ҽ�֯ʱ�������ؽ�֯��
//input:�ĸ����ص��ѡ��Ĳ���
//input:�ĸ����ص㶼һ��������һ��Ϊ�ο���
//��������ҽ�֯ʱ:
//  0��left r0 �� left g0 �� left b0 (rֻ��ѡr0,gֻ��ѡg0��bֻ��ѡb0,���¶�һ��)
//  1��left r1 �� left g1 �� left b1
//  2��right r0 �� right g0 �� right b0
//  3��right r1 �� right g1 �� right b1
//����������ҽ�֯ʱ:
//  0��r0 �� g0 �� b0
//  1��r1 �� g1 �� b1
//  2��r2 �� g2 �� b2
//  3��r3 �� g3 �� b3

//����һ��������˵��:
// �����ҽ�֯ʱ��
//	ONE=0x1b= 0001 1011 ��ʾ��һ�����ص��ѡ��bit0,bit1��ʾR��ѡ��11��ʾѡ����right r1 ;
//						    bit2,bit3��ʾG��ѡ��10��ʾѡ����right g0 ;
//						    bit4,bit5��ʾB��ѡ��01��ʾѡ����left b1 ;
// ��û�����ҽ�֯ʱ��
//	ONE=0x1b= 0001 1011 ��ʾ��һ�����ص��ѡ��bit0,bit1��ʾR��ѡ��11��ʾѡ����r3 ;
//						    bit2,bit3��ʾG��ѡ��10��ʾѡ����g2 ;
//						    bit4,bit5��ʾB��ѡ��01��ʾѡ����b1 ;


//�����������ʾ����

//�������ؽ�֯��
//3D���ҽ�֯ʱ��mode = 1��ONE = 0x00000000; TWO = 0x00101010; Three = 0x00010101; Four = 0x00111111;
//				0x00		  0x2a		      0x15		 0x3f
//2D��֯ʱ��	mode = 0��ONE = 0x00000000; TWO = 0x00010101; Three = 0x00101010; Four = 0x00111111;
//				0x00		  0x15		      0x2a		 0x3f

//�����ص�G����:
//3D���ҽ�֯ʱ��mode = 1��ONE = 0x00001000; TWO = 0x00100010; Three = 0x00011101; Four = 0x00110111;
//				0x08		  0x22                0x1d		 0x37
//2D��֯ʱ��	mode = 0��ONE = 0x00000100; TWO = 0x00010001; Three = 0x00101110; Four = 0x00111011;
//				0x04		  0x11                0x2e		 0x3b

//�����ص�R����:
//3D���ҽ�֯ʱ��mode = 1��ONE = 0x00000010; TWO = 0x00101000; Three = 0x00010111; Four = 0x00111101;
//				0x02		  0x28                0x17		 0x3d
//2D��֯ʱ��	mode = 0��ONE = 0x00000001; TWO = 0x00010100; Three = 0x00101011; Four = 0x00111110;
//				0x01		  0x14                0x2b		 0x3E

//�����ص�B����:
//3D���ҽ�֯ʱ��mode = 1��ONE = 0x00100000; TWO = 0x00001010; Three = 0x00110101; Four = 0x00011111;
//				0x20		  0x0a                0x35		 0x1f
//2D��֯ʱ��	mode = 0��ONE = 0x00010000; TWO = 0x00000101; Three = 0x00111010; Four = 0x00101111;
//				0x10		  0x05                0x3a		 0x2f

void T2_Panel3DInter2(int mode,int ONE,int TWO,int Three,int Four)
{
	unsigned char tAddr, tData[2];
	unsigned char tempData=0;
	
	if(mode==0)  //�����ҽ�֯ʱ
	{
		//��һ�����ص�Rѡ��
		tAddr = ONE&0x03;
		IIC_Read_forT101(T103ADDR_TCON, 0x15, tData, 1);
		tData[0] &= 0xf8;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //r0 select left r0
				break;
			case 1:
				tData[0] |=0x01; //r0 select left r1
				break;
			case 2:
				tData[0] |=0x03; //r0 select right r0				
				break;
			case 3:
				tData[0] |=0x04; //r0 select right r2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x15, tData, 1);
		
		//��һ�����ص�Gѡ��
		tAddr = (ONE&0x0c)/4;
		IIC_Read_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tData[0] &= 0x8f;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //g0 select left g0
				break;
			case 1:
				tData[0] |=0x10; //g0 select left g1
				break;
			case 2:
				tData[0] |=0x30; //g0 select right g0				
				break;
			case 3:
				tData[0] |=0x40; //g0 select right g2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x16, tData, 1);	
		
		//��һ�����ص�Bѡ��
		tAddr = (ONE&0x30)/16;
		IIC_Read_forT101(T103ADDR_TCON, 0x18, tData, 1);
		tData[0] &= 0xfc;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //b0 select left b0
				break;
			case 1:
				tData[0] |=0x01; //b0 select left b1
				break;
			case 2:
				tData[0] |=0x03; //b0 select right b0				
				break;
			case 3:
				tData[0] |=0x04; //b0 select right b2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x18, tData, 1);
		
		//�ڶ������ص�Rѡ��
		tAddr = TWO&0x03;
		IIC_Read_forT101(T103ADDR_TCON, 0x15, tData, 1);
		tData[0] &= 0xc7;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //r1 select left r0
				break;
			case 1:
				tData[0] |=0x08; //r1 select left r1
				break;
			case 2:
				tData[0] |=0x18; //r1 select right r0				
				break;
			case 3:
				tData[0] |=0x20; //r1 select right r2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x15, tData, 1);
		
		//�ڶ������ص�Gѡ��
		tAddr = (TWO&0x0c)/4;
		IIC_Read_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tempData = tData[0];
		IIC_Read_forT101(T103ADDR_TCON, 0x17, tData, 1);
		tData[0] &= 0xfc;
		tempData &= 0x7f;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //g1 select left g0
				tempData |= 0x00;
				break;
			case 1:
				tData[0] |=0x00; //g1 select left g1
				tempData |= 0x80;
				break;
			case 2:
				tData[0] |=0x01; //g1 select right g0	
				tempData |= 0x80;			
				break;
			case 3:
				tData[0] |=0x02; //g1 select right g2
				tempData |= 0x00;
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x17, tData, 1);
		tData[0] = tempData;
		IIC_Write_forT101(T103ADDR_TCON, 0x16, tData, 1);	
		
		//�ڶ������ص�Bѡ��
		tAddr = (TWO&0x30)/16;
		IIC_Read_forT101(T103ADDR_TCON, 0x18, tData, 1);
		tData[0] &= 0xc7;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //b1 select left b0
				break;
			case 1:
				tData[0] |=0x08; //b1 select left b1
				break;
			case 2:
				tData[0] |=0x18; //b1 select right b0				
				break;
			case 3:
				tData[0] |=0x20; //b1 select right b2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x18, tData, 1);
		
		//���������ص�Rѡ��
		tAddr = (Three&0x03);
		IIC_Read_forT101(T103ADDR_TCON, 0x15, tData, 1);
		tempData = tData[0];
		IIC_Read_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tData[0] &= 0xfe;
		tempData &= 0x3f;
		switch(tAddr)
		{
			case 0:
				tData[0] |= 0x00; //r2 select left r0
				tempData |= 0x00;
				break;
			case 1:
				tData[0] |=0x00; //r2 select left r1
				tempData |= 0x40;
				break;
			case 2:
				tData[0] |=0x00; //r2 select right r0	
				tempData |= 0xc0;			
				break;
			case 3:
				tData[0] |=0x01; //r2 select right r2
				tempData |= 0x00;
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tData[0] = tempData;
		IIC_Write_forT101(T103ADDR_TCON, 0x15, tData, 1);
		
		//���������ص�Gѡ��
		tAddr = (Three&0x0c)/4;
		IIC_Read_forT101(T103ADDR_TCON, 0x17, tData, 1);
		tData[0] &= 0xe3;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //g0 select left g0
				break;
			case 1:
				tData[0] |=0x04; //g2 select left g1
				break;
			case 2:
				tData[0] |=0x0c; //g2 select right g0				
				break;
			case 3:
				tData[0] |=0x10; //g2 select right g2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x17, tData, 1);	
		
		//���������ص�Bѡ��
		tAddr = (Three&0x30)/16;
		IIC_Read_forT101(T103ADDR_TCON, 0x18, tData, 1);
		tempData = tData[0];
		IIC_Read_forT101(T103ADDR_TCON, 0x19, tData, 1);
		tData[0] &= 0xfe;
		tempData &= 0x3f;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //b2 select left b0
				tempData |= 0x00;
				break;
			case 1:
				tData[0] |=0x00; //b2 select left b1
				tempData |= 0x40;
				break;
			case 2:
				tData[0] |=0x00; //b2 select right b0	
				tempData |= 0xc0;			
				break;
			case 3:
				tData[0] |=0x01; //b2 select right b2
				tempData |= 0x00;
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x19, tData, 1);
		tData[0] = tempData;
		IIC_Write_forT101(T103ADDR_TCON, 0x18, tData, 1);	
		
		//���ĸ����ص�Rѡ��
		tAddr = (Four&0x03);
		IIC_Read_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tData[0] &= 0xf1;
		switch(tAddr)
		{
			case 0:
				tData[0] |= 0x00; //r3 select left r0
				tempData |= 0x00;
				break;
			case 1:
				tData[0] |=0x02; //r3 select left r1
				break;
			case 2:
				tData[0] |=0x06; //r3 select right r0				
				break;
			case 3:
				tData[0] |=0x08; //r3 select right r2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x16, tData, 1);
		
		//���ĸ����ص�Gѡ��
		tAddr = (Four&0x0c)/4;
		IIC_Read_forT101(T103ADDR_TCON, 0x17, tData, 1);
		tData[0] &= 0x1f;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //g3 select left g0
				break;
			case 1:
				tData[0] |=0x20; //g3 select left g1
				break;
			case 2:
				tData[0] |=0x60; //g3 select right g0				
				break;
			case 3:
				tData[0] |=0x80; //g3 select right g2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x17, tData, 1);	
		
		//���ĸ����ص�Bѡ��
		tAddr = (Four&0x30)/16;
		IIC_Read_forT101(T103ADDR_TCON, 0x19, tData, 1);
		tData[0] &= 0xf1;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //b3 select left b0
				break;
			case 1:
				tData[0] |=0x02; //b3 select left b1
				break;
			case 2:
				tData[0] |=0x06; //b3 select right b0				
				break;
			case 3:
				tData[0] |=0x08; //b3 select right b2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x19, tData, 1);								
	}
	else 	   //û�����ҽ�֯ʱ
	{
		//��һ�����ص�Rѡ��
		tAddr = ONE&0x03;
		IIC_Read_forT101(T103ADDR_TCON, 0x15, tData, 1);
		tData[0] &= 0xf8;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //r0 select left r0
				break;
			case 1:
				tData[0] |=0x01; //r0 select left r1
				break;
			case 2:
				tData[0] |=0x02; //r0 select right r0				
				break;
			case 3:
				tData[0] |=0x03; //r0 select right r2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x15, tData, 1);
		
		//��һ�����ص�Gѡ��
		tAddr = (ONE&0x0c)/4;
		IIC_Read_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tData[0] &= 0x8f;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //g0 select left g0
				break;
			case 1:
				tData[0] |=0x10; //g0 select left g1
				break;
			case 2:
				tData[0] |=0x20; //g0 select right g0				
				break;
			case 3:
				tData[0] |=0x30; //g0 select right g2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x16, tData, 1);	
		
		//��һ�����ص�Bѡ��
		tAddr = (ONE&0x30)/16;
		IIC_Read_forT101(T103ADDR_TCON, 0x18, tData, 1);
		tData[0] &= 0xfc;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //b0 select left b0
				break;
			case 1:
				tData[0] |=0x01; //b0 select left b1
				break;
			case 2:
				tData[0] |=0x02; //b0 select right b0				
				break;
			case 3:
				tData[0] |=0x03; //b0 select right b2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x18, tData, 1);
		
		//�ڶ������ص�Rѡ��
		tAddr = TWO&0x03;
		IIC_Read_forT101(T103ADDR_TCON, 0x15, tData, 1);
		tData[0] &= 0xc7;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //r1 select left r0
				break;
			case 1:
				tData[0] |=0x08; //r1 select left r1
				break;
			case 2:
				tData[0] |=0x10; //r1 select right r0				
				break;
			case 3:
				tData[0] |=0x18; //r1 select right r2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x15, tData, 1);
		
		//�ڶ������ص�Gѡ��
		tAddr = (TWO&0x0c)/4;
		IIC_Read_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tempData = tData[0];
		IIC_Read_forT101(T103ADDR_TCON, 0x17, tData, 1);
		tData[0] &= 0xfc;
		tempData &= 0x7f;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //g1 select left g0
				tempData |= 0x00;
				break;
			case 1:
				tData[0] |=0x00; //g1 select left g1
				tempData |= 0x80;
				break;
			case 2:
				tData[0] |=0x01; //g1 select right g0	
				tempData |= 0x00;			
				break;
			case 3:
				tData[0] |=0x01; //g1 select right g2
				tempData |= 0x80;
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x17, tData, 1);
		tData[0] = tempData;
		IIC_Write_forT101(T103ADDR_TCON, 0x16, tData, 1);	
		
		//�ڶ������ص�Bѡ��
		tAddr = (TWO&0x30)/16;
		IIC_Read_forT101(T103ADDR_TCON, 0x18, tData, 1);
		tData[0] &= 0xc7;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //b1 select left b0
				break;
			case 1:
				tData[0] |=0x08; //b1 select left b1
				break;
			case 2:
				tData[0] |=0x10; //b1 select right b0				
				break;
			case 3:
				tData[0] |=0x18; //b1 select right b2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x18, tData, 1);
		
		//���������ص�Rѡ��
		tAddr = (Three&0x03);
		IIC_Read_forT101(T103ADDR_TCON, 0x15, tData, 1);
		tempData = tData[0];
		IIC_Read_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tData[0] &= 0xfe;
		tempData &= 0x3f;
		switch(tAddr)
		{
			case 0:
				tData[0] |= 0x00; //r2 select left r0
				tempData |= 0x00;
				break;
			case 1:
				tData[0] |=0x00; //r2 select left r1
				tempData |= 0x40;
				break;
			case 2:
				tData[0] |=0x00; //r2 select right r0	
				tempData |= 0x80;			
				break;
			case 3:
				tData[0] |=0x00; //r2 select right r2
				tempData |= 0xc0;
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tData[0] = tempData;
		IIC_Write_forT101(T103ADDR_TCON, 0x15, tData, 1);
		
		//���������ص�Gѡ��
		tAddr = (Three&0x0c)/4;
		IIC_Read_forT101(T103ADDR_TCON, 0x17, tData, 1);
		tData[0] &= 0xe3;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //g0 select left g0
				break;
			case 1:
				tData[0] |=0x04; //g2 select left g1
				break;
			case 2:
				tData[0] |=0x08; //g2 select right g0				
				break;
			case 3:
				tData[0] |=0x0c; //g2 select right g2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x17, tData, 1);	
		
		//���������ص�Bѡ��
		tAddr = (Three&0x30)/16;
		IIC_Read_forT101(T103ADDR_TCON, 0x18, tData, 1);
		tempData = tData[0];
		IIC_Read_forT101(T103ADDR_TCON, 0x19, tData, 1);
		tData[0] &= 0xfe;
		tempData &= 0x3f;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //b2 select left b0
				tempData |= 0x00;
				break;
			case 1:
				tData[0] |=0x00; //b2 select left b1
				tempData |= 0x40;
				break;
			case 2:
				tData[0] |=0x00; //b2 select right b0	
				tempData |= 0x80;			
				break;
			case 3:
				tData[0] |=0x01; //b2 select right b2
				tempData |= 0xc0;
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x19, tData, 1);
		tData[0] = tempData;
		IIC_Write_forT101(T103ADDR_TCON, 0x18, tData, 1);	
		
		//���ĸ����ص�Rѡ��
		tAddr = (Four&0x03);
		IIC_Read_forT101(T103ADDR_TCON, 0x16, tData, 1);
		tData[0] &= 0xf1;
		switch(tAddr)
		{
			case 0:
				tData[0] |= 0x00; //r3 select left r0
				tempData |= 0x00;
				break;
			case 1:
				tData[0] |=0x02; //r3 select left r1
				break;
			case 2:
				tData[0] |=0x04; //r3 select right r0				
				break;
			case 3:
				tData[0] |=0x06; //r3 select right r2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x16, tData, 1);
		
		//���ĸ����ص�Gѡ��
		tAddr = (Four&0x0c)/4;
		IIC_Read_forT101(T103ADDR_TCON, 0x17, tData, 1);
		tData[0] &= 0x1f;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //g3 select left g0
				break;
			case 1:
				tData[0] |=0x20; //g3 select left g1
				break;
			case 2:
				tData[0] |=0x40; //g3 select right g0				
				break;
			case 3:
				tData[0] |=0x60; //g3 select right g2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x17, tData, 1);	
		
		//���ĸ����ص�Bѡ��
		tAddr = (Four&0x30)/16;
		IIC_Read_forT101(T103ADDR_TCON, 0x19, tData, 1);
		tData[0] &= 0xf1;
		switch(tAddr)
		{
			case 0:
				tData[0] |=0x00; //b3 select left b0
				break;
			case 1:
				tData[0] |=0x02; //b3 select left b1
				break;
			case 2:
				tData[0] |=0x04; //b3 select right b0				
				break;
			case 3:
				tData[0] |=0x06; //b3 select right b2
				break;
			default:break;	
		}
		IIC_Write_forT101(T103ADDR_TCON, 0x19, tData, 1);		
	}
}


//3D��֯����
//input: 0: �ر�3D��֯ 1:�������ؽ�֯�� 2:���������ؽ�֯ 3��2D�����ؽ�֯
void T3_Panel3DInter_EN(int flag)
{
	//unsigned char tAddr;
	unsigned char tData[2];
	tData[1] = 0x0;
	if(0 == flag)
	{
		tData[0] = 0x00;
		IIC_Write_forT101(T103ADDR_TCON, 0x1b, tData, 1);

	}
	else if(1 == flag)
	{		
		tData[0] = 0x03;
		IIC_Write_forT101(T103ADDR_TCON, 0x1b, tData, 1);
	}
	else if(1 == flag)
	{		
		tData[0] = 0x03;
		IIC_Write_forT101(T103ADDR_TCON, 0x1b, tData, 1);
	}	
	else	
	{
		tData[0] = 0x06;
		IIC_Write_forT101(T103ADDR_TCON, 0x1b, tData, 1);	
	}	
}

static void set_backlight_level(unsigned level)
{    
   // int set_level;
    printk(KERN_INFO "%s: %d\n", __FUNCTION__, level);	
    if(level > 15) level = 15;
    
    bl_level = level; 
}

static unsigned get_backlight_level(void)
{
    printk(KERN_DEBUG "%s: %d\n", __FUNCTION__, bl_level);
    return bl_level;
}

//input: FrequencyΪTTL �� CLK��Hz Ϊ ��õ��Ĺ�ɺCOM��Ƶ��(���磺50Hz��60Hz)��
//output:��
void SetComFrequency(int Frequency,int Hz)
{
	//unsigned char tAddr;
	unsigned char tData[2];
	int Fre;
	Fre = 512*Hz;
	Fre = Frequency/Fre;
	tData[0] = Fre/256;
	IIC_Write_forT101(T103ADDR_TCON, 0x31, tData, 1);
	tData[0] = Fre%256;
	IIC_Write_forT101(T103ADDR_TCON, 0x30, tData, 1);			
}


void T103_TCON_Initialize(struct i2c_client *client)
{
	unsigned char i, tData[3];
	
	for(i=0;i<MAX_TC103_LEN_TCON;i++)
	{
		tData[0] = TC103_INIT_TBL_TCON[i][0];
		tData[1] = TC103_INIT_TBL_TCON[i][1];
		twx_i2c_write(client,tData,2);			
	}
	printk( "twx_tc103_init.write data to tc103 \n");
    initial_dithering_table(client);
}

void T103_BL_Initialize(struct i2c_client *client)
{
    unsigned char i, tData[3];
	printk("T103_BL_Initialize i2cadr=%x\n",client->addr);
    gamaSelect = 3;	
	for(i=0;i<MAX_TC103_LEN_DBL;i++)
	{
		tData[0] = TC103_INIT_TBL_DBL[i][0];
        tData[1] = TC103_INIT_TBL_DBL[i][1];
		twx_i2c_write(client,tData,2);	
                //msleep(5);		
	}

	//initial_gamma_table(client,0);      
	dlightMode = 1;
    TC103_DBL_CTR(dlightMode);
    T3_PWMBL_CTR(bl_level);
    printk( "twx_tc103_init.bl .T103_BL_Initialize backlight level:%d \n",bl_level);
    //msleep(100);     
           
    //TC103_BLCTROL(1);    
}

void T103_BL_Initialize_Test(struct i2c_client *client)
{
	unsigned char TC103_INIT_TBL_DBL[4][2]={{0x00,0x00},{0x01,0x0d},{0x02,0x1f},{0x40,0x02},};
    unsigned char i, tData[3];
    printk( "T103_BL_Initialize_Test\n");
    gamaSelect = 3;	
	for(i=0;i<4;i++)
	{
		tData[0] = TC103_INIT_TBL_DBL[i][0];
        tData[1] = TC103_INIT_TBL_DBL[i][1];
		twx_i2c_write(client,tData,2);			
	}
	tData[0]=0x00;
	tData[1]=0x01;
	twx_i2c_write(client,tData,2);
}
static int twx_tc103_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int res = 0;
    printk( "twx_tc103_init.i2c probe begin \n");
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        pr_err("%s: functionality check failed\n", __FUNCTION__);
        res = -ENODEV;
    }
    else
    {
     twx_tc103_i2c_client = client;
     printk( "twx_tc103_init.i2c probe client get \n");
     //T103_TCON_Initialize(client);
     //tc103b_init(client);
     
    }	

	
    printk( "twx_tc103_init.i2c probe end \n");
    
    return res;
}

static int twx_tc103_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id twx_tc103_id[] = {
    { "twx_tc103", 0 },
    { }
};


static struct i2c_driver twx_tc103_i2c_driver = {
    .probe = twx_tc103_i2c_probe,
    .remove = twx_tc103_i2c_remove,
    .id_table = twx_tc103_id,
    .driver = {
    .owner	= THIS_MODULE,
    .name = "twx_tc103",
    },
};


static int twx_tc103_bl_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int res = 0;
    printk( "twx_tc103_init.bl .i2c probe begin \n");
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        pr_err("%s: functionality check failed\n", __FUNCTION__);
        res = -ENODEV;
    }
    else
    {
     twx_tc103_bl_i2c_client = client;
     //T103_BL_Initialize(client);
     printk( "twx_tc103_init.bl .i2c probe client get \n");
    }	
    printk( "twx_tc103_init.bl .i2c probe end \n");
    
    return res;
}

static int twx_tc103_bl_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct i2c_device_id twx_tc103_bl_id[] = {
    { "twx_tc103_bl", 0 },
    { }
};

static struct i2c_driver twx_tc103_bl_i2c_driver = {
    .probe = twx_tc103_bl_i2c_probe,
    .remove = twx_tc103_bl_i2c_remove,
    .id_table = twx_tc103_bl_id,
    .driver = {
    .owner	= THIS_MODULE,
    .name = "twx_tc103_bl",
    },
};

static void power_on_backlight(void)
{            

}
static int twx_tc103_resume(struct platform_device * pdev);

static int twx_tc103_probe(struct platform_device *pdev){
    int res;
	
	printk("\n\nMINI LVDS Driver Init.\n\n");
	//msleep(1000);
    if (twx_tc103_i2c_client)
    {
        res = 0;
    }
    else
    {
        res = i2c_add_driver(&twx_tc103_i2c_driver);
        if (res < 0) {
            printk("add twx_tc103 i2c driver error\n");
        }
    }    
    if (twx_tc103_bl_i2c_client)
    {
        res = 0;
    }
    else
    {
        res = i2c_add_driver(&twx_tc103_bl_i2c_driver);
        if (res < 0) {
            printk("add twx_tc103 i2c driver error\n");
        }
    }

	printk("\n\nMINI LVDS Driver Init I2C add OK.\n\n");

	//T3_PWMBL_CTR(twx_tc103_bl_client,0x08);
	//twx_tc103_resume(pdev);
    return res;
}

static int twx_tc103_remove(struct platform_device *pdev)
{
  return 0;
}

static void power_on_lcd(void)
{
    msleep(50);
}

static int twx_tc103_suspend(struct platform_device * pdev, pm_message_t mesg)
{
	return 0;
}

static int twx_tc103_resume(struct platform_device * pdev)
{
	int res = 0;


	/*
	* already in late resume, but lcd resume later than tc103, 
	* so call power_on_lcd here, and lcd_resume would call it again
	*/
	power_on_lcd();

 // T103_BL_Initialize_Test(twx_tc103_bl_i2c_client);
  if (twx_tc103_i2c_client)
  {
      T103_TCON_Initialize(twx_tc103_i2c_client);
      res = 0;
  }
  else
  {
      res = i2c_add_driver(&twx_tc103_i2c_driver);
      if (res < 0) {
          printk("add twx_tc103 i2c driver error\n");
      }
  }

  if (twx_tc103_bl_i2c_client)
  {
      T103_BL_Initialize(twx_tc103_bl_i2c_client);
      res = 0;
  }
  else
  {
      res = i2c_add_driver(&twx_tc103_bl_i2c_driver);
      if (res < 0) {
          printk("add twx_tc103 i2c driver error\n");
      }
  }
  
	return res;
}

void lvds_init(void)
{
	unsigned char disable_gamma[6] = {0x40,0x11,0x4e,0x01,0x01,0x00};
	if (twx_tc103_i2c_client)
		T103_TCON_Initialize(twx_tc103_i2c_client);
	if (twx_tc103_bl_i2c_client){
		T103_BL_Initialize(twx_tc103_bl_i2c_client);
		twx_i2c_write(twx_tc103_bl_i2c_client,&disable_gamma[0],2);
		twx_i2c_write(twx_tc103_bl_i2c_client,&disable_gamma[2],2);
		twx_i2c_write(twx_tc103_bl_i2c_client,&disable_gamma[4],2);
	}
}
EXPORT_SYMBOL(lvds_init);

int twx_tc103_reinit(void)
{
	int res = 0;
  if (twx_tc103_i2c_client)
  {
	T103_TCON_Initialize(twx_tc103_i2c_client);
  }
	return res;
}

static ssize_t control_3DInter(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    char flag_3d = buf[0] ;    
    if(flag_3d == '1') {
        flag_3d = 1;
        printk("\nLCD 3D Inter on.\n");
        TC103_Panel3DInter(1);
    }else{
        flag_3d = 0;
        TC103_Panel3DInter(0);
        printk("\nLCD 3D Inter off.\n");
    }

	return count;
}

static ssize_t control_3DLense(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    char flag_3d = buf[0] ;    
    if(flag_3d == '0') {
        flag_3d = 1;
        printk("\nLCD 3D Lense on.\n");
        T2_Panel3DLense(0,0);
    }else{
        flag_3d = 0;
        T2_Panel3DLense(0,1);
        printk("\nLCD 3D Lense off.\n");
    }

	return count;
}

static ssize_t control_LRSwitch(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    char flag_3d = buf[0] ;    
    if(flag_3d == '0') {
        TC103_Panel3DSwitchLRPic(0);
    }else{
        TC103_Panel3DSwitchLRPic(1);
    }

	return count;
}

static ssize_t control_DLightPower(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    char flag_3d = buf[0] ;    
    if(flag_3d == '0') {
        //input: 0 �ص����� 1���򿪱���
         TC103_BLCTROL(0);
    }else{
        TC103_BLCTROL(1);
    }

	return count;
}

static ssize_t control_DLight(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    char flag_3d = buf[0] ;
        
    if(flag_3d == '0') {
       if(dlightMode != 0)
       {
         TC103_BLCTROL(0);
         TC103_DBL_CTR(0); 
         //msleep(500);
         //TC103_BLCTROL(1);   
        } 
       dlightMode=0;
    }else if(flag_3d=='1'){
        if(dlightMode == 0) TC103_BLCTROL(0);
        if(dlightMode != 1) TC103_DBL_CTR(1);  
        //if(dlightMode == 0) {msleep(500);TC103_BLCTROL(1);   }
       dlightMode=1;
    }else if(flag_3d=='2'){
       if(dlightMode == 0) TC103_BLCTROL(0);
       if(dlightMode != 2) TC103_DBL_CTR(2); 
       //if(dlightMode == 0) {msleep(500);TC103_BLCTROL(1);   }   
       dlightMode=2;
    }else if(flag_3d=='3'){
       if(dlightMode == 0) TC103_BLCTROL(0);
       if(dlightMode != 3) TC103_DBL_CTR(3); 
       //if(dlightMode == 0) {msleep(500);TC103_BLCTROL(1);   }  
       dlightMode=3;
    }
    printk("tc103 set dlight to mode:%d",dlightMode);
    return count;
}

static ssize_t control_DLRegWrite(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    unsigned char tData[2]={0},ret;
    printk("reg:%x  value:%x \n",tData[0],tData[1]);
    ret=sscanf(buf,"%x %x",&tData[0],&tData[1]);   
    twx_i2c_write(twx_tc103_bl_i2c_client,tData,2);
	return count;
}

static ssize_t control_DLRegRead(struct class *class, 
			struct class_attribute *attr,	const char *buf)
{
    unsigned char tData[2],ret;
    int i;
    printk("---------read dynamic light reg -----------");
    for( i=0;i<256;i++)
    {
      if(i%16 == 0) printk("\n");
      tData[0]=i; 
      ret = twx_i2c_read(twx_tc103_bl_i2c_client,tData,2);
      printk(" %x ",tData[0]);      
    }
     printk("\n");
	return 0;
}

static ssize_t control_TCONRegWrite(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    unsigned char tData[2],ret;
    
    ret=sscanf(buf,"%x %x",&tData[0],&tData[1]);   
    twx_i2c_write(twx_tc103_i2c_client,tData,2);
	return count;
}

static ssize_t control_TCONRegRead(struct class *class, 
			struct class_attribute *attr,	const char *buf)
{
    unsigned char tData[2],ret;
    int i;
    printk("---------read tcon reg -----------");
    for(i=0;i<256;i++)
    {
      if(i%16 == 0) printk("\n");
      tData[0]=i; 
      ret = twx_i2c_read(twx_tc103_i2c_client,tData,2);
      printk(" %x ",tData[0]);      
    }
     printk("\n");
	return 0;
}


static ssize_t control_Light(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{

    int value;
    sscanf(buf,"%x",&value);  
    if(value<256) 
      T3_PWMBL_CTR(value);

	return count;
}

static struct class_attribute enable3d_class_attrs[] = {
    __ATTR(lense,  S_IRUGO | S_IWUSR, NULL,    control_3DLense), 
    __ATTR(inter, S_IRUGO | S_IWUSR, NULL,    control_3DInter), 
    __ATTR(lr_switch, S_IRUGO | S_IWUSR, NULL,    control_LRSwitch), 
    __ATTR(dlight, S_IRUGO | S_IWUSR, NULL,    control_DLight), 
    __ATTR(dlight_power, S_IRUGO | S_IWUSR, NULL,    control_DLightPower), 
    __ATTR(light, S_IRUGO | S_IWUSR, NULL,    control_Light),
    __ATTR(dlreg, S_IRUGO | S_IWUSR, control_DLRegRead,    control_DLRegWrite),
    __ATTR(tcreg, S_IRUGO | S_IWUSR, control_TCONRegRead,    control_TCONRegWrite),
    __ATTR_NULL
};

static struct class enable3d_class = {
    .name = "twx3d",
    .class_attrs = enable3d_class_attrs,
};


static struct platform_driver twx_tc103_driver = {
	.probe = twx_tc103_probe,
  .remove = twx_tc103_remove,
  .suspend = NULL,
  .resume = NULL,
	.driver = {
		.name = "twx",
	},
};

static int __init twx_tc103_init(void)
{
	int ret = 0;
    printk( "twx_tc103_init. \n");

    ret = platform_driver_register(&twx_tc103_driver);
    if (ret != 0) {
        printk(KERN_ERR "failed to register twx module, error %d\n", ret);
        return -ENODEV;
    }

    ret = class_register(&enable3d_class);
	if(ret){
		printk(" class register enable3d_class fail!\n");
	}
    return ret;
}

static void __exit twx_tc103_exit(void)
{
    pr_info("twx_tc103: exit\n");
    platform_driver_unregister(&twx_tc103_driver);
}
	
	
//arch_initcall(twx_tc103_init);
module_init(twx_tc103_init);
module_exit(twx_tc103_exit);

MODULE_AUTHOR("BESTIDEAR");
MODULE_DESCRIPTION("TTL IN MINILVDS OUT driver for TWX_TC103");
MODULE_LICENSE("GPL");



