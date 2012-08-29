/*
 * driver/mfd/axp173.c
 *
 */
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/mfd/core.h>
#include <linux/mfd/axp173.h>
#include <mach/board-ns115.h>
#include <mach/io.h>
#include <mach/hardware.h>
#include <linux/power/ns115-battery.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/input.h>

#define	COUNT_IRQSTATUS		4
#define DEFAULT_TOTAL_COULOMBMETER	0x1cef

static uint8_t REG_IRQSTATUS[COUNT_IRQSTATUS] = {
	AXP173REG_IRQ1_STATUS,
	AXP173REG_IRQ2_STATUS,
	AXP173REG_IRQ3_STATUS,
	AXP173REG_IRQ4_STATUS,
};

extern void dwc_otg_usb_det(void);

struct axp173_t;
typedef void (*irqhandle_t)(struct axp173_t *axp173, uint8_t irqstatus);

struct axp173_t {
	struct i2c_client *client;
	struct work_struct     work;
	struct workqueue_struct *workqueue;
	struct delayed_work delayed_work;
	int bat_vol;
	int use_coulombmeter;
	int coulombmeter_total;
	int ac_status;
	int usb_status;
	unsigned int batt_level;
	unsigned int batt_full;
	unsigned int battery_irq_full;
	irqhandle_t irqhandle_func[COUNT_IRQSTATUS];
};
struct axp173_t *axp173 = NULL;

//[][0]: Percent; [][1]:Voltage
static const unsigned long s_arrPercent[][2] = {
	{100, 4150}, {90, 4030}, {80, 3910}, {70, 3850},
	{60, 3778}, {50, 3774}, {40, 3690}, {30, 3670},
	{20, 3640}, {10, 3580}, {0, 3500},
};
static int size_percent_table = sizeof(s_arrPercent)/sizeof(s_arrPercent[0]);

extern int ns115_charger_register(struct ns115_charger *hw_chg);
extern int ns115_charger_unregister(struct ns115_charger *hw_chg);
extern int ns115_battery_gauge_register(struct ns115_battery_gauge *batt_gauge);
extern int ns115_battery_gauge_unregister(struct ns115_battery_gauge *batt_gauge);
extern void ns115_charger_plug(enum ns115_charger_type type);

static int axp173_ac_status(void);
static int axp173_usb_status(void);
static int axp173_batt_full(void);

static inline int __axp173_read(struct i2c_client *client,
		u8 reg, uint8_t *val)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (uint8_t)ret;
	dev_dbg(&client->dev, "axp173: reg read  reg=%x, val=%x\n",
			reg, *val);
	return 0;
}

static inline int __axp173_write(struct i2c_client *client,
		u8 reg, uint8_t val)
{
	int ret;

	dev_dbg(&client->dev, "axp173: reg write  reg=%x, val=%x\n",
			reg, val);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
				val, reg);
		return ret;
	}

	return 0;
}

static inline int __axp173_write_mask(struct i2c_client *client,
		u8 reg, uint8_t val, uint8_t mask)
{
	int ret;
	uint8_t rdval;
	ret = __axp173_read(client, reg, &rdval);
	if(ret < 0) {
		return ret;
	}
	rdval &= ~mask;
	rdval |= (val&mask);
	return __axp173_write(client, reg, rdval);
}

void axp_touchscreen(int on)
{
	uint8_t value;
	__axp173_read(axp173->client,0x12,&value);
	if(on)
		value |= 1 << 3;
	else
		value &= (~(1 << 3));
	__axp173_write(axp173->client,0x12,value);
}
EXPORT_SYMBOL(axp_touchscreen);

static int coulombmeter_control(int on)
{
	int ret;
	if(on ==1)
		ret = __axp173_write_mask(axp173->client,0xb8,(1 << 7),(1 << 7));//enable
	else if(on == 0)
		ret = __axp173_write_mask(axp173->client,0xb8,(1 << 7),0);//disable
	return ret;
}
static int coulombmeter_stop(void)
{
	int ret;
	ret = __axp173_write_mask(axp173->client,0xb8,(1 << 6),(1 << 6));
	return ret;
}
static int coulombmeter_clear(void)
{
	int ret;
	ret = __axp173_write_mask(axp173->client,0xb8,(1 << 5),(1 << 5));
	return ret;
}

#if 0
static unsigned int charge_coulombmeter(void)
{
	unsigned int charge_coulombmeter = 0;
	static unsigned int old_charge_coulombmeter = 0xffffffff;
	uint8_t value;
	__axp173_read(axp173->client,0xb0,&value);
	charge_coulombmeter += value << 24;
	
	__axp173_read(axp173->client,0xb1,&value);	
	charge_coulombmeter += value << 16;

	__axp173_read(axp173->client,0xb2,&value);	
	charge_coulombmeter += value << 8;
	
	__axp173_read(axp173->client,0xb3,&value);
	charge_coulombmeter += value;
	
	printk(KERN_EMERG"chg = %d\n",charge_coulombmeter);
	
	if(old_charge_coulombmeter == 0xffffffff)
		old_charge_coulombmeter = charge_coulombmeter;
	if(old_charge_coulombmeter < 20)
		old_charge_coulombmeter = 20;
	if((charge_coulombmeter < old_charge_coulombmeter - 20) || (charge_coulombmeter > old_charge_coulombmeter + 20))
		charge_coulombmeter = old_charge_coulombmeter;
	old_charge_coulombmeter = charge_coulombmeter;
	return charge_coulombmeter;
}
static unsigned int discharge_coulombmeter(void)
{
	unsigned int discharge_coulombmeter = 0;
	static unsigned int old_discharge_coulombmeter = 0xffffffff;
	uint8_t value;

	__axp173_read(axp173->client,0xb4,&value);
	discharge_coulombmeter += value << 24;
	
	__axp173_read(axp173->client,0xb5,&value);	
	discharge_coulombmeter += value << 16;
	
	__axp173_read(axp173->client,0xb6,&value);	
	discharge_coulombmeter += value << 8;
	
	__axp173_read(axp173->client,0xb7,&value);
	discharge_coulombmeter += value;
	
	printk(KERN_EMERG"dis = %d\n",discharge_coulombmeter);

	if(old_discharge_coulombmeter == 0xffffffff)
		old_discharge_coulombmeter = discharge_coulombmeter;
	if(old_discharge_coulombmeter < 20)
		old_discharge_coulombmeter = 20;
	if((discharge_coulombmeter < old_discharge_coulombmeter - 20) || (discharge_coulombmeter > old_discharge_coulombmeter + 20))
		discharge_coulombmeter = old_discharge_coulombmeter;
	old_discharge_coulombmeter = discharge_coulombmeter;

	return discharge_coulombmeter;
}
#endif

static unsigned int current_coulombmeter(void)
{
	unsigned int charge_coulombmeter,discharge_coulombmeter;
	unsigned int current_coulombmeter;
	static unsigned int old_coulombmeter = 3500;
	char reg_value = 0xb0;
	uint8_t value[8];
	i2c_master_send(axp173->client,&reg_value,1);
	i2c_master_recv(axp173->client,value,8);
	charge_coulombmeter += value[0] << 24;
	charge_coulombmeter += value[1] << 16;
	charge_coulombmeter += value[2] << 8;
	charge_coulombmeter += value[3] << 0;

	discharge_coulombmeter += value[4] << 24;
	discharge_coulombmeter += value[5] << 16;	
	discharge_coulombmeter += value[6] << 8;
	discharge_coulombmeter += value[7] << 0;

	//printk(KERN_EMERG"chg = %d,dis = %d\n",charge_coulombmeter,discharge_coulombmeter);
	current_coulombmeter = charge_coulombmeter - discharge_coulombmeter;
	axp173->ac_status = axp173_ac_status();
	axp173->usb_status = axp173_usb_status();
	axp173->batt_full = axp173_batt_full();
	
	if(current_coulombmeter < 10000)
		old_coulombmeter = current_coulombmeter;	
	//printk(KERN_EMERG"current_coulombmeter = %d,old_coulombmeter = %d\n",current_coulombmeter,old_coulombmeter);
	return old_coulombmeter;
}

static void set_use_coulombmeter(void)
{
	__axp173_write(axp173->client,0x06,0xaa);
}
static int use_coulombmeter(void)
{
	uint8_t value;
	__axp173_read(axp173->client,0x06,&value);
	if(value == 0xaa)
		return 1;
	else 
		return 0;
}
static int read_coulombmeter_total(void)
{
	unsigned int total;
	uint8_t value;
	__axp173_read(axp173->client,0x07,&value);	
	total += value << 16;
	
	__axp173_read(axp173->client,0x08,&value);	
	total += value << 8;
	
	__axp173_read(axp173->client,0x09,&value);
	total += value;
	return total;
}

static void clear_coulombmeter_total(void)
{
	__axp173_write(axp173->client,0x07,0);
	__axp173_write(axp173->client,0x08,0);
	__axp173_write(axp173->client,0x09,0);
}

static void save_coulombmeter_total(unsigned int total)
{
	__axp173_write(axp173->client,0x07,(total >> 16) & 0xff);
	__axp173_write(axp173->client,0x08,(total >> 8) & 0xff);
	__axp173_write(axp173->client,0x09,total & 0xff);
	return;
}

static int axp173_battery_vol(void)
{	
	uint8_t vol;
	int voltage,ret = 0;
	ret = __axp173_read(axp173->client,0x78,&vol);
	voltage = (vol << 4);
	ret = __axp173_read(axp173->client,0x79,&vol);
	voltage += (vol & 0x0f);
	return voltage*11/10;
}

static int axp173_battery_discharging_cur(void)
{
	uint8_t cur;
	int curr;
	__axp173_read(axp173->client,0x7c,&cur);
	curr = (cur << 5);
	__axp173_read(axp173->client,0x7d,&cur);
	curr += cur;
	return curr/2;
}

static int axp173_battery_charging_cur(void)
{
	uint8_t cur;
	int curr;
	__axp173_read(axp173->client,0x7a,&cur);	
	curr = (cur << 5);
	__axp173_read(axp173->client,0x7b,&cur);
	curr += cur;
	return curr/2;

}

static int axp173_ac_status(void)
{
	uint8_t value;
	__axp173_read(axp173->client,0x0,&value);
	if(value < 0)
		return -1;
	return (value & (1 << 7))?1:0;
}

int axp173_usb_status(void)
{
	uint8_t value;

	__axp173_read(axp173->client,0x00,&value);
	if((value & (0x07 << 3)) == (0x07 << 3))
		return 1;
	return 0;
}

static int axp173_batt_full(void)
{
	uint8_t value;
	//static int battery_irq_full = 0;
	__axp173_read(axp173->client,0x01,&value);
	if(value < 0)
		return -1;

	/*__axp173_read(axp173->client,AXP173REG_IRQ2_STATUS,&irq_value);
	if(irq_value < 0)
		return -1;

	if(irq_value & (1 << 2)){	
		__axp173_write(axp173->client,AXP173REG_IRQ2_STATUS,irq_value);
		battery_irq_full = 1;
		if(axp173->use_coulombmeter){
			save_coulombmeter_total(current_coulombmeter());
		}
	}
	*/
	if((axp173->ac_status != 1) || (axp173->usb_status != 1))
		axp173->battery_irq_full = 0;
	
	if((axp173->ac_status || axp173->usb_status) && (value & (1 << 5)) && (!(value & (1 << 6))) && axp173->battery_irq_full){
		return 1;
	}
	return 0;
}

void axp173_power_off(void)
{
	printk(KERN_EMERG"%s\n",__func__);
#if 1
	if(axp173->batt_level == 0){
		coulombmeter_control(0);
		coulombmeter_clear();
		coulombmeter_control(1);
		set_use_coulombmeter();
	}
#endif
	gpio_request(93,"5v_en");
	gpio_request(13,"backlight");
	gpio_request(12,"LCD_ON");
	gpio_request(102,"LVDS");
	
	gpio_direction_output(93,0);//5V
	gpio_direction_output(13,0);//backlight
	gpio_direction_output(12,1);//LCD_ON
	gpio_direction_output(102,0);//LVDS

	gpio_free(93);
	gpio_free(13);
	gpio_free(12);
	gpio_free(102);
	__axp173_write_mask(axp173->client,0x32,1 << 7,1 << 7);
	mdelay(100);
}
EXPORT_SYMBOL(axp173_power_off);

void axp173_battery_status(struct work_struct *work)
{
	static int old_ac_status = 0,old_usb_status = 0;
	int current_ac_status,current_usb_status;
	if(axp173->use_coulombmeter){
		current_ac_status = axp173->ac_status;
		current_usb_status = axp173->usb_status;
	}else{
		current_ac_status = axp173_ac_status();
		current_usb_status = axp173_usb_status();
	}
	
	if((old_ac_status != current_ac_status) || (old_usb_status != current_usb_status)){
		if(!(current_ac_status && old_ac_status)){
			if(current_ac_status)
				ns115_charger_plug(CHG_TYPE_AC);
			else if(current_usb_status)
				ns115_charger_plug(CHG_TYPE_USB);
		}
		if((current_ac_status ==0) && (current_usb_status == 0))
			ns115_charger_unplug();
	}
	old_ac_status = current_ac_status;
	old_usb_status = current_usb_status;
	schedule_delayed_work(&axp173->delayed_work,5 * HZ);
}

static int axp173_get_stat(enum ns115_charger_state * hw_stat)
{	
	int ret = 1;
	
	if (ret){
		*hw_stat = CHG_CHARGING_STATE;
		//dev_info("%s: CHG_CHARGING_STATE", __func__);
	}else{
		*hw_stat = CHG_DONE_STATE;
		//dev_info("%s: CHG_DONE_STATE", __func__);	
	}

	return 0;
}

static int axp173_start_chg(struct ns115_charger * hw_chg, enum ns115_charging_type type)
{	
	return 1;
}

static int axp173_stop_chg(struct ns115_charger * hw_chg)
{		
	return 1;
}

static int axp173_gasgauge_get_mvolts(void)
{
	//printk(KERN_EMERG"%s\n",__func__);
	struct i2c_client *client = NULL;
	int bat_vol,bat_discharge_cur,bat_charge_cur;

	bat_vol = axp173_battery_vol();//battery voltage

	bat_discharge_cur = axp173_battery_discharging_cur();
	bat_charge_cur = axp173_battery_charging_cur();
	bat_vol = bat_vol + bat_discharge_cur * 2 / 10;
	bat_vol = bat_vol - bat_charge_cur * 2 / 10;

	if(bat_vol < 1000)
		bat_vol = 4200;

	if(bat_vol < 0)
		goto out;
	axp173->bat_vol = bat_vol;
	return (bat_vol & 0xffff)*1000;

out:
	dev_warn(&client->dev, "failed to read voltage. ret %x\n", bat_vol);
	return 0;
}


static int axp173_gasgauge_get_capacity(void)
{
	//printk(KERN_EMERG"%s\n",__func__);
	struct i2c_client *client = NULL;
	int total_coulombmeter = 0,i;
	int level = 0;
	total_coulombmeter = read_coulombmeter_total();
	if(axp173->use_coulombmeter == 0){
		if(axp173->bat_vol >= s_arrPercent[0][1]) {
			level = s_arrPercent[0][0];
		} else {
			for(i=1; i<size_percent_table; i++) {
				if((axp173->bat_vol >= s_arrPercent[i][1])) {
					level = ((s_arrPercent[i-1][0] - s_arrPercent[i][0]) * 1000 /
						(s_arrPercent[i-1][1]-s_arrPercent[i][1]) *
					(axp173->bat_vol - s_arrPercent[i][1])) / 1000 + s_arrPercent[i][0];
				break;
			}
		}
		if(level <= 0)
			level = 0;
		}
		
	}else if(axp173->use_coulombmeter == 1){
		if(total_coulombmeter == 0){
			//printk(KERN_EMERG"use default\n");
			//level = ((charge_coulombmeter() - discharge_coulombmeter())*100)/DEFAULT_TOTAL_COULOMBMETER;
			level = (current_coulombmeter()*100)/DEFAULT_TOTAL_COULOMBMETER;
		}
		else {
			//printk(KERN_EMERG"use total\n");
			//level = ((charge_coulombmeter() - discharge_coulombmeter())*100)/total_coulombmeter;
			level = (current_coulombmeter()*100)/total_coulombmeter;
		}
		
		if(axp173_batt_full())
			level = 100;
		if(level < 110 && level > 100)
			level = 100;
		if(level >= 110 || level < 0)
			level = 0;
	}
	//printk(KERN_EMERG"level = %d\n",level);
	axp173->batt_level = level;
	return level;
	
out:
	dev_warn(&client->dev, "failed to read voltage. ret %x\n", level);
	return 0;

}

int axp173_gasgauge_get_status(void)
{
	//printk(KERN_EMERG"%s\n",__func__);
	int status;
	struct i2c_client *client = NULL;
	int ret;
	int ac_in,usb_in,batt_full;
	if(axp173->use_coulombmeter){
		ac_in = axp173->ac_status;
		usb_in = axp173->usb_status;
		batt_full = axp173->batt_full;
	}
	else{
		ac_in = axp173_ac_status();
		usb_in = axp173_usb_status();
		batt_full = axp173_batt_full();
	}
	if(ac_in || usb_in)
		ret = 4;
	else 
		ret = 1;
	
	if(batt_full)
		ret = 2;
	if(ret<0)
		return ret;
	dev_vdbg(&client->dev, "FLAGS 0x%x\n", ret);
	if(ret & 1){
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	}else if(ret & 2){
		status = POWER_SUPPLY_STATUS_FULL;
	}else{
		status = POWER_SUPPLY_STATUS_CHARGING;
	}
	
	return status;
}

static void axp173_irq1_handle(struct axp173_t *axp173, uint8_t status)
{
	if(status & (1 << 2))
		dwc_otg_usb_det();
}

static void axp173_irq2_handle(struct axp173_t *axp173, uint8_t status)
{
	if((status & (1 << 2)) && axp173->use_coulombmeter){
		axp173->battery_irq_full = 1;
		save_coulombmeter_total(current_coulombmeter());
	}
}

static void axp173_irq3_handle(struct axp173_t *axp173, uint8_t status)
{
}

static void axp173_irq4_handle(struct axp173_t *axp173, uint8_t status)
{
}

static void axp173_irq_wq(struct work_struct *wk)
{
	int i;
	int ret;
	uint8_t irqstatus=0;
	struct axp173_t *axp173 = container_of(wk,
			struct axp173_t,
			work);
	struct i2c_client *i2c = axp173->client;
	for(i=0; i<sizeof(REG_IRQSTATUS)/sizeof(REG_IRQSTATUS[0]); i++) {
		irqstatus = 0;
		ret = __axp173_read(i2c, REG_IRQSTATUS[i], &irqstatus);
		printk("[%d:%s] ret=%d, irqstatus=%x\n", __LINE__, __FUNCTION__, ret, irqstatus);
		if(ret!=0  || irqstatus==0) {
			continue;
		}
		// clear irq
		__axp173_write_mask(i2c, REG_IRQSTATUS[i], irqstatus, irqstatus);
		// now, process irq
		axp173->irqhandle_func[i](axp173, irqstatus);
	}

	enable_irq(axp173->client->irq);
}

static irqreturn_t axp173_irq_handle(int irq, void *data)
{
	struct axp173_t *axp173 = (struct axp173_t *)data;
	disable_irq_nosync(irq);
	queue_work(axp173->workqueue, &axp173->work);
	return IRQ_HANDLED;
}

static int __devinit axp173_irq_init(struct axp173_t *axp173)
{
	int ret;
	struct i2c_client *i2c = axp173->client;
#if 0
	// init irq registers
	__axp173_write(i2c, AXP173REG_IRQ1_ENABLE, IRQ1_AC_INSERT|IRQ1_AC_EJECT|IRQ1_VBUS_INSERT|IRQ1_VBUS_EJECT|IRQ1_VBUS_LOW);
	__axp173_write(i2c, AXP173REG_IRQ2_ENABLE, IRQ2_CHARGING|IRQ2_CHARG_END);
	__axp173_write(i2c, AXP173REG_IRQ3_ENABLE, IRQ3_CHARGE_mALOW);
	__axp173_write(i2c, AXP173REG_IRQ4_ENABLE, IRQ4_VBUS_VALID|IRQ4_VBUS_INVALID);
	ret = request_threaded_irq(i2c->irq, NULL, axp173_irq_handle,  IRQF_TRIGGER_FALLING,
			"axp173", axp173);
	//ret = request_irq(i2c->irq, axp173_irq_handle, IRQF_TRIGGER_FALLING, i2c->name, axp173);
#else
	// close all irq, next version implement it
	__axp173_write(i2c, AXP173REG_IRQ1_ENABLE, 0);
	__axp173_write(i2c, AXP173REG_IRQ2_ENABLE, 0);
	__axp173_write(i2c, AXP173REG_IRQ3_ENABLE, 0);
	__axp173_write(i2c, AXP173REG_IRQ4_ENABLE, 0);
#endif
	ret = request_threaded_irq(i2c->irq, NULL, axp173_irq_handle,  IRQF_TRIGGER_FALLING,
			"axp173", axp173);
	//ret = request_irq(i2c->irq, axp173_irq_handle, IRQF_TRIGGER_FALLING, i2c->name, axp173);
	__axp173_write_mask(i2c,AXP173REG_IRQ1_ENABLE,(1 << 2),(1 << 2));
	__axp173_write_mask(i2c,AXP173REG_IRQ2_ENABLE,(1 << 2),(1 << 2));
	return ret;
}

static int __devinit axp173_register_init(struct axp173_t *axp173)
{
	struct i2c_client *i2c = axp173->client;
	// init LDO&DCDC power
	// 	LDO3(3.3V) for TP
	//	LDO4(2.8V) for Camera
	__axp173_write(i2c, AXP173REG_LDO4_VOL, 0x54);		//2.8V
	__axp173_write_mask(i2c, AXP173REG_LDO23_VOL, 0xA0, 0xF0);
	__axp173_write_mask(i2c, AXP173REG_LDO234_CTRL, OUTENABLE_LDO3|OUTENABLE_LDO4, OUTENABLE_LDO3|OUTENABLE_LDO4);
	__axp173_write_mask(i2c,0x82,0x7c,0x7c);//enable battery current adc
	__axp173_write_mask(i2c,0x36,0x02,0x03);
	
	return 0;
}

static struct ns115_charger axp173_hw_chg = {
	.type = CHG_TYPE_AC,
	.name = "charger_ac",
	.start_charging = axp173_start_chg,
	.stop_charging = axp173_stop_chg,
	.get_charging_stat = axp173_get_stat,
};

static struct ns115_charger axp173_usb_chg = {
	.type = CHG_TYPE_USB,
	.name = "charger_usb",
	.start_charging = axp173_start_chg,
	.stop_charging = axp173_stop_chg,
	.get_charging_stat = axp173_get_stat,
};

static struct ns115_battery_gauge axp173_gasgauge_gauge = {
	.get_battery_mvolts = axp173_gasgauge_get_mvolts,
	.get_battery_capacity = axp173_gasgauge_get_capacity,
	.get_battery_status = axp173_gasgauge_get_status,
};

static int axp173_status_register(struct axp173_t *axp173)
{
	int ret;

	ret = ns115_charger_register(&axp173_hw_chg);
	if (ret)
		return -1;
	
 	ret = ns115_charger_register(&axp173_usb_chg);
 	if (ret)
 		return -1;

	ret = ns115_battery_gauge_register(&axp173_gasgauge_gauge);
	if(ret)
		return -1;
	return 0;
}

static ssize_t axp173_state_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf,"axp173_info\n%s\naxp173_battery_voltage=%d mV\ntotal_coulombmeter=%d\ncurrent_coulombmeter=%d\n",\
		axp173->use_coulombmeter?"coulombmeter_mode":"voltage_mode",\
		axp173_gasgauge_get_mvolts()/1000,\
		read_coulombmeter_total(),\
		current_coulombmeter());
}

static ssize_t axp173_state_store(struct device *dev,struct device_attribute *attr,
		char *buf)
{
	char read_buf[256];
	memset(read_buf,0,256);
	sprintf(read_buf,"%s\n",buf);
	if(!memcmp(buf,"reset",5)){
		coulombmeter_control(0);
		coulombmeter_clear();
		coulombmeter_control(1);
		set_use_coulombmeter();
	}
	return printk(KERN_EMERG"read_buf = %s\n",read_buf);
}

static DEVICE_ATTR(axp_state, S_IRUGO | S_IWUSR, axp173_state_show, axp173_state_store);

static int __devinit axp173_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	int ret;
	struct class *axp173_class = NULL;
	struct device *axp173_dev = NULL;
	printk("[%d:%s] \n", __LINE__, __FUNCTION__);
	axp173 = kmalloc(sizeof(struct axp173_t), GFP_KERNEL);
	if(!axp173) {
		return -ENOMEM;
	}
	axp173->client = i2c;
	axp173->workqueue = create_singlethread_workqueue("wq_axp173");
	if(!axp173->workqueue) {
		ret = -1;
		goto err_create_wq;
	}
	INIT_WORK(&axp173->work, axp173_irq_wq);
	axp173->irqhandle_func[0] = axp173_irq1_handle;
	axp173->irqhandle_func[1] = axp173_irq2_handle;
	axp173->irqhandle_func[2] = axp173_irq3_handle;
	axp173->irqhandle_func[3] = axp173_irq4_handle;

	axp173->ac_status = 0;
	axp173->usb_status = 0;
	axp173->batt_level = 50;
	axp173->batt_full = 0;
	
	
	ret = axp173_register_init(axp173);
	if(ret != 0) {
		dev_err(&i2c->dev, "register init error\n");
		goto err_register_init;
	}
	ret = axp173_irq_init(axp173);
	if(ret != 0) {
		dev_err(&i2c->dev, "irq init error\n");
		goto err_irq_init;
	}

	axp173->use_coulombmeter = use_coulombmeter();
	if(axp173->use_coulombmeter==0)
		clear_coulombmeter_total();
	coulombmeter_control(0);
	axp173->use_coulombmeter = 0;
	
	ret = axp173_status_register(axp173);
	if(ret != 0){
		dev_err(&i2c->dev,"register battery status error\n");
		goto err_irq_init;
	}
	axp173_class = class_create(THIS_MODULE,"axp_state");
	axp173_dev = device_create(axp173_class,NULL,MKDEV(0,162),NULL,"axp_state");
	ret = device_create_file(axp173_dev,&dev_attr_axp_state);
	INIT_DELAYED_WORK(&axp173->delayed_work,axp173_battery_status);
	schedule_delayed_work(&axp173->delayed_work,10*HZ);
	enable_irq_wake(axp173->client->irq);
	return 0;
err_irq_init:
err_register_init:	
err_create_wq:
	kfree(axp173);
	return ret;

}

static int  __devexit axp173_i2c_remove(struct i2c_client *i2c)
{
	return 0;
}

#ifdef CONFIG_PM
static int axp173_i2c_suspend(struct i2c_client *i2c, pm_message_t state)
{
	return 0;
}


static int axp173_i2c_resume(struct i2c_client *i2c)
{
	return 0;
}

#endif


static const struct i2c_device_id axp173_i2c_id[] = {
	{"axp173", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, axp173_i2c_id);

static struct i2c_driver axp173_i2c_driver = {
	.driver = {
		.name = "axp173",
		.owner = THIS_MODULE,
	},
	.probe = axp173_i2c_probe,
	.remove = __devexit_p(axp173_i2c_remove),
#ifdef CONFIG_PM
	.suspend = axp173_i2c_suspend,
	.resume = axp173_i2c_resume,
#endif
	.id_table = axp173_i2c_id,
};


static int __init axp173_i2c_init(void)
{
	int ret = -ENODEV;
	ret = i2c_add_driver(&axp173_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);

	return ret;
}

late_initcall(axp173_i2c_init);

static void __exit axp173_i2c_exit(void)
{
	i2c_del_driver(&axp173_i2c_driver);
}

module_exit(axp173_i2c_exit);


MODULE_AUTHOR("hengai <qiang@livall.cn>");
MODULE_DESCRIPTION("AXP173 PMIC");
MODULE_LICENSE("GPL");

