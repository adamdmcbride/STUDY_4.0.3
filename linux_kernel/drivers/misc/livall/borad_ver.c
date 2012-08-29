/*
   B16  B15   VER  NOTE
    0    0     0   1024x600
    1    0     2   QunChuan800x480

    hwversion= (B15)|(B16<<1)
*/
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <mach/gpio.h>
#include <mach/board-ns115.h>

int hwversion = 0;
#ifndef MODULE
static int __init hwversion_setup(char *version)
{
	if(version && *version) hwversion = simple_strtol(version, NULL, 16);
}
__setup("hwversion=", hwversion_setup);

#endif

extern int livallsys_create_file(struct device *dev, struct device_attribute *attr);

static ssize_t hwversion_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x", hwversion);
}

static ssize_t boardinfo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;
	//NAME
	size += sprintf(buf+size, "NAME:%s\n", "N751");
	// WIFI & CRYSTAL
	if(hwversion & HWVERSION_NW51) {
		size += sprintf(buf+size, "WIFI:%s\n", "NW51");
		size += sprintf(buf+size, "BT:%s\n", "0");
	} else {
		size += sprintf(buf+size, "WIFI:%s\n", "NW53");
		size += sprintf(buf+size, "BT:%s\n", "1");
	}
	if(hwversion & HWVERSION_WIFI_26M) size += sprintf(buf+size, "CRYSTAL:%s\n", "26");
	else size += sprintf(buf+size, "CRYSTAL:%s\n", "37.4");
	//DDR
	if(hwversion & HWVERSION_DDR_512M) size += sprintf(buf+size, "DDR:%s\n", "512");
	else size += sprintf(buf+size, "DDR:%s\n", "1024");
	//LVDS
	if(hwversion & HWVERSION_TC103) size += sprintf(buf+size, "LVDS:%s\n", "1");
	else size += sprintf(buf+size, "LVDS:%s\n", "0");
	return size;
}

static ssize_t resolution_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(HWVERSION_800x480 & hwversion) return sprintf(buf, "%s", "800x480");
	return sprintf(buf, "%s", "1024x600");
}

static DEVICE_ATTR(hwversion, S_IRUGO, hwversion_show, NULL);
static DEVICE_ATTR(lcdres, S_IRUGO, resolution_show, NULL);
static DEVICE_ATTR(boardinfo, S_IRUGO, boardinfo_show, NULL);

static int __init board_version_init(void)
{
	int err;
	err = livallsys_create_file(NULL, &dev_attr_hwversion);
	err = livallsys_create_file(NULL, &dev_attr_lcdres);
	err = livallsys_create_file(NULL, &dev_attr_boardinfo);
	return 0;
}

static void __exit board_version_exit(void)
{
}

MODULE_AUTHOR("hengai<halfhq@163.com>");
MODULE_DESCRIPTION("Livall HW boardversion");
MODULE_LICENSE("GPL");

late_initcall(board_version_init);
module_exit(board_version_exit);

