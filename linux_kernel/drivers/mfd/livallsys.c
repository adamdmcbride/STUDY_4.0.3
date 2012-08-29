#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/major.h>
#include <linux/kdev_t.h>

struct livallsys{
	struct class *class;
	struct device *dev;
	atomic_t verinc;
};
struct livallsys *sys = NULL;

static int livallsys_init(void)
{
	if(sys) return 0;
	sys = kzalloc(sizeof(*sys),GFP_KERNEL);
	if(!sys) {
		printk(KERN_EMERG"livallsys_init error.");
		goto err_malloc;
	}
	sys->class = class_create(THIS_MODULE, "livall");
	if(!sys->class){
		printk(KERN_EMERG"livallsys_init class_create error.");
		goto err_class;
	}
	atomic_set(&sys->verinc, 1);
	return 0;
err_class:
	kfree(sys);
	sys = NULL;
	return -ENOMEM;
err_malloc:
	return -ENOMEM;
}

struct device * livallsys_device_create(const char *name)
{
	int err;
	struct device *dev;
	err = livallsys_init();
	if(err) return err;
	dev = device_create(sys->class, NULL, MKDEV(UNNAMED_MAJOR, atomic_add_return(1, &sys->verinc)), (void*)sys, name);
	if(!dev){
		printk(KERN_EMERG"livallsys_init device_create error.");
		return NULL;
	}
	return dev;
}
EXPORT_SYMBOL(livallsys_device_create);

int livallsys_create_file(struct device *dev, struct device_attribute *attr)
{
	int err;
	err = livallsys_init();
	if(err) return err;
	if(!dev) {
		if(!sys->dev) sys->dev = livallsys_device_create("state");
		if(!sys->dev) {
			return -ENOMEM;
		}
		dev = sys->dev;
	}
	return device_create_file(dev, attr);
}
EXPORT_SYMBOL(livallsys_create_file);

