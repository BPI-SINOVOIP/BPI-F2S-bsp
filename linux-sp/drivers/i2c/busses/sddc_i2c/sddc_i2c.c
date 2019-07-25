#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "sddc_i2c.h"

#define SDDC_ADDR       0x50
#define MAX_DATA_LEN    1024

//#define SDDC_I2C_DEBUG

#define SDDC_USE_I2C_MASTER0
#ifdef SDDC_USE_I2C_MASTER0
#define SDDC_USE_I2CM_NUM    0
#else
#define SDDC_USE_I2CM_NUM    1
#endif

static int sddc_i2c_major = 0;
static int sddc_i2c_minor = 0;

static struct class *sddc_i2c_class = NULL;
static struct st_sddc_dev *sddc_dev = NULL;
static struct i2c_client *g_sddc_i2c_client = NULL;


/* char devices */

static int sddc_open(struct inode *inode, struct file *filp);
static int sddc_release(struct inode *inode, struct file *filp);
static long sddc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int sddc_write(u8 addr, u8 *data, uint16_t len);
static int sddc_read(u8 addr, u8 *data, uint16_t len);
static int sddc_register(void);
static ssize_t sddc_nor_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static ssize_t sddc_nor_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);

static const struct file_operations sddc_fops = {
	.owner		= THIS_MODULE,
	.open		= sddc_open,
	.release	= sddc_release,
	.unlocked_ioctl	= sddc_ioctl,
	.read		= sddc_nor_read,
	.write		= sddc_nor_write,
};

static int sddc_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &g_sddc_dev;
	printk(KERN_INFO "open sddc\n");
	return 0;
}

static int sddc_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "release sddc\n");
	return 0;
}

static ssize_t sddc_nor_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	uint8_t reg = 0;
	i2c_param *userdata;
	uint8_t *data;
	int ret = 0;
	uint16_t len = 0;
#ifdef SDDC_I2C_DEBUG
	int i;
#endif

	printk(KERN_INFO "read ..................\n");
#if 0
	userdata = (i2c_param *)buffer;
	if (!userdata) {
		printk(KERN_ERR "user space data error!!\n");
		return -1;
	}
	printk(KERN_INFO "reg:%d\n", *(userdata->reg));
	printk(KERN_INFO "len:%d\n", *(userdata->len));

	reg = *(userdata->reg);
	len = *(userdata->len);

	data = kmalloc(MAX_DATA_LEN, GFP_KERNEL);
	if (!data) {
		printk(KERN_ERR "Failed to alloc memory.\n");
		return -ENOMEM;
	}
	memset(data, 0, MAX_DATA_LEN);

	ret = sddc_read(reg, data, len);
	memcpy(userdata->value, data, len);
#ifdef SDDC_I2C_DEBUG
	for (i = 0; i < len; i++)
		printk(KERN_INFO "data:0x%3x\n", data[i]);
#endif

	kfree(data);
#endif
	return ret;
}

static ssize_t sddc_nor_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	uint8_t reg = 0;
	i2c_param *userdata;
	uint8_t *data;
	int ret = 0;
	uint16_t len = 0;
#ifdef SDDC_I2C_DEBUG
	int i;
#endif

	printk(KERN_INFO "write ..................\n");
#if 0
	userdata = (i2c_param *)buffer;
	if (!userdata) {
		printk(KERN_ERR "user space data error!!\n");
		return -1;
	}
	printk(KERN_INFO "reg:%d\n", *(userdata->reg));
	printk(KERN_INFO "len:%d\n", *(userdata->len));

	reg = *(userdata->reg);
	len = *(userdata->len);

	data = kmalloc(MAX_DATA_LEN, GFP_KERNEL);
	if (!data) {
		printk(KERN_ERR "Failed to alloc memory.\n");
		return -ENOMEM;
	}

	memset(data, 0, MAX_DATA_LEN);
	memcpy(data, userdata->value, len);
#ifdef SDDC_I2C_DEBUG
	for (i = 0; i < len; i++)
		printk(KERN_INFO "data:0x%3x\n", data[i]);
#endif

	ret = sddc_write(reg, data, len);
	kfree(data);
#endif
	return ret;
}

static long sddc_ioctl(struct file *filp, unsigned int cmd, unsigned long __user arg)
{
	i2c_param userdata;
	unsigned char *tmp;
	int ret = 0;
#ifdef SDDC_I2C_DEBUG
	int i;
#endif

	printk(KERN_INFO "ioctl sddc\n");

	if(copy_from_user((void *)&userdata, (const void __user *)arg, sizeof(i2c_param))){
		ret = -EFAULT;
		printk(KERN_ERR "user space i2c_param error!!\n");
	}
	//printk(KERN_INFO "reg: %d\n", userdata.reg);
	//printk(KERN_INFO "len: %d\n", userdata.len);
	//printk(KERN_INFO "data addr: 0x%x\n", (unsigned int)userdata.data);

	tmp = kmalloc(MAX_DATA_LEN, GFP_KERNEL);
	if (!tmp) {
		printk(KERN_INFO "Failed to alloc memory.\n");
		return -ENOMEM;
	}
	memset(tmp, 0, MAX_DATA_LEN);

	if(copy_from_user((void *)tmp, (void *)userdata.data, userdata.len)){
		ret = -EFAULT;
		printk(KERN_ERR "user space data error!!\n");
	}
	//printk(KERN_INFO "tmp addr: 0x%x\n", (unsigned int)tmp);

	printk(KERN_INFO "ioctl cmd: %d\n", cmd);
	switch (cmd) {
		case I2C_SDDC_CMD_READ:
			printk(KERN_INFO "sddc read\n");
			ret = sddc_read(userdata.reg, tmp, (uint16_t)userdata.len);

			if(copy_to_user((void *)userdata.data, (void *)tmp, userdata.len)){
				ret = -EFAULT;
				break;
			}
			
			#ifdef SDDC_I2C_DEBUG
			for (i = 0; i < userdata.len; i++)
				printk(KERN_INFO "read data:0x%3x\n", tmp[i]);
			#endif
			break;

		case I2C_SDDC_CMD_WRITE:
			printk(KERN_INFO "sddc write\n");
			#ifdef SDDC_I2C_DEBUG
			for (i = 0; i < userdata.len; i++)
				printk(KERN_INFO "write data:0x%3x\n", tmp[i]);
			#endif
			ret = sddc_write(userdata.reg, tmp, (uint16_t)userdata.len);
			break;

		default:
			break;
	}

	kfree(tmp);
	return ret;
}

static int sddc_write(u8 addr, u8 *data, uint16_t len)
{
	struct i2c_adapter *adap = g_sddc_i2c_client->adapter;
	struct i2c_msg msg;
	int retry = 0;
	int i;
	u8 temp[512] = {0};

	temp[0] = addr;
	for (i = 0; i < len; i++)
		temp[i + 1] = data[i];

	len += 1;
	msg.addr = g_sddc_i2c_client->addr;
	msg.flags = 0;
	msg.len = len;
	msg.buf = (u8 *)temp;

	while ((1 != i2c_transfer(adap, &msg, 1)) && (retry < 10)) {
		retry++;
		printk(KERN_ERR "sddc_write fail! retry = %d\n", retry);
	}

	if (retry >= 10)
		return -1;

	return 0;
}

static int sddc_read(u8 addr, u8 *data, uint16_t len)
{
	struct i2c_adapter *adap = g_sddc_i2c_client->adapter;
	struct i2c_msg msg;
	int retry = 0;

	msg.addr = g_sddc_i2c_client->addr;
	msg.flags = 0;
	/* for test restart mode */
	/* msg.flags = 0|I2C_M_NOSTART; */
	msg.len = 1;
	msg.buf = (u8 *)&addr;

	while ((1 != i2c_transfer(adap, &msg, 1)) && (retry < 10)) {
		retry++;
		printk(KERN_ERR "sddc_read write addr fail! retry = %d\n", retry);
	}
	if (retry >= 10)
		return -1;

	/* read */
	msg.flags |= I2C_M_RD;
	msg.len = len;
	msg.buf = (u8 *)data;
	mdelay(20);

	while ((1 != i2c_transfer(adap, &msg, 1)) && (retry < 10)) {
		retry++;
		printk(KERN_ERR "sddc_read read addr fail! retry = %d\n", retry);
	}

	if (retry >= 10)
		return -1;

	return 0;
}

static int __sddc_setup_dev(struct st_sddc_dev *dev)
{
	int err;
	dev_t devno = MKDEV(sddc_i2c_major, sddc_i2c_minor);

	cdev_init(&(dev->dev), &sddc_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &sddc_fops;

	err = cdev_add(&(dev->dev), devno, 1);
	if (err)
		return err;

	return 0;
}

/* i2c driver............................. */
static const struct i2c_device_id sddc_id[] = {
	{ "sddc", 0, },
};

static int __devinit sddc_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;
	dev_t devno = MKDEV(sddc_i2c_major, sddc_i2c_minor);
	struct device *temp = NULL;

	err = alloc_chrdev_region(&devno, 0, 1, SDDC_DEVICE_NODE_NAME);
	if (err < 0) {
		printk(KERN_ERR "Failed to alloc char dev region.\n");
		goto fail;
	}
	sddc_i2c_major = MAJOR(devno);
	sddc_i2c_minor = MINOR(devno);

	sddc_dev = kmalloc(sizeof(struct st_sddc_dev), GFP_KERNEL);
	if (!sddc_dev) {
		err = -ENOMEM;
		printk(KERN_ERR "Failed to alloc sddc device.\n");
		goto unregister;
	}

	memset(sddc_dev, 0, sizeof(struct st_sddc_dev));
	err = __sddc_setup_dev(sddc_dev);
	if (err) {
		printk(KERN_ERR "Failed to setup sddc device: %d.\n", err);
		goto cleanup;
	}

	sddc_i2c_class = class_create(THIS_MODULE, SDDC_DEVICE_CLASS_NAME);
	if (IS_ERR(sddc_i2c_class)) {
		err = PTR_ERR(sddc_i2c_class);
		printk(KERN_ERR "Failed to create sddc device class.\n");
		goto destroy_cdev;
	}

	temp = device_create(sddc_i2c_class, NULL, devno, "%s", SDDC_DEVICE_FILE_NAME);
	if (IS_ERR(temp)) {
		err = PTR_ERR(temp);
		printk(KERN_ERR "Failed to create sddc device.\n");
		goto destroy_class;
	}

	printk(KERN_INFO "Succedded to initialize sddc device.\n");
	return 0;

destroy_class:
	class_destroy(sddc_i2c_class);
destroy_cdev:
	cdev_del(&(sddc_dev->dev));
cleanup:
	kfree(sddc_dev);
unregister:
	unregister_chrdev_region(MKDEV(sddc_i2c_major, sddc_i2c_minor), 1);
fail:
	return err;
}

static int __devinit sddc_i2c_remove(struct i2c_client *client)
{
	dev_t devno = MKDEV(sddc_i2c_major, sddc_i2c_minor);

	class_destroy(sddc_i2c_class);
	cdev_del(&(sddc_dev->dev));
	kfree(sddc_dev);

	unregister_chrdev_region(devno, 1);
	return 0;
}

static struct i2c_driver sddc_i2c_driver = {
	.driver = {
		.name	= "sddc",
		.owner	= THIS_MODULE,
	},
	.probe		= sddc_i2c_probe,
	.remove		= sddc_i2c_remove,
	.id_table	= sddc_id,
};

static int sddc_register(void)
{
	struct i2c_board_info info;
	struct i2c_adapter *adapter;
	struct i2c_client *client;

	memset(&info, 0, sizeof(struct i2c_board_info));
	info.addr = SDDC_ADDR;
	strlcpy(info.type, "sddc", I2C_NAME_SIZE);

	adapter = i2c_get_adapter(SDDC_USE_I2CM_NUM);	/* 0 is m0, 1 is m1*/
	if (!adapter) {
		printk(KERN_ERR "[SDDC] get adapter err!!\n");
		return -ENODEV;
	}
	printk(KERN_INFO "[SDDC] Succeeded to get adapter 0\n");

	client = i2c_new_device(adapter, &info);
	if (!client) {
		printk(KERN_ERR "Unable to add I2C device\n");
		return -ENOMEM;
	}

	g_sddc_i2c_client = client;
	//g_sddc_i2c_client->driver = &sddc_i2c_driver;

	return 0;
}

/* init the driver */
static int __init sddc_init(void)
{
	int ret = 0;

	ret = sddc_register();
	if (ret < 0)
		printk(KERN_ERR "sddc register fail\n");

	ret = i2c_add_driver(&sddc_i2c_driver);

	return ret;
}

/* cleanup the driver */
static void __exit sddc_exit(void)
{
	i2c_del_driver(&sddc_i2c_driver);
}

MODULE_AUTHOR("Sunplus");
MODULE_DESCRIPTION("Sunplus I2C DDC slave driver");
MODULE_LICENSE("GPL");

module_init(sddc_init);
module_exit(sddc_exit);
