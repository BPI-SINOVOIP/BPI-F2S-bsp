#ifndef _SDDC_I2C_H_
#define _SDDC_I2C_H_

#include <linux/cdev.h>
#include <linux/semaphore.h>

#define SDDC_DEVICE_NODE_NAME     "sddc"
#define SDDC_DEVICE_FILE_NAME     "sddc"
#define SDDC_DEVICE_CLASS_NAME    "sddc"

#define SDDC_IOCTL_BASE       'I'
#define I2C_SDDC_CMD_READ     _IOR(SDDC_IOCTL_BASE, 1, i2c_param)
#define I2C_SDDC_CMD_WRITE    _IOW(SDDC_IOCTL_BASE, 2, i2c_param)

struct st_sddc_dev {
	struct cdev dev;
	int major;
	unsigned char mem[1024];
	struct semaphore semLock;
};

typedef struct i2c_param_s
{
	unsigned char reg;  //sub address
	unsigned char len;
	unsigned char *data;
} i2c_param;


/* global dev object */
struct st_sddc_dev g_sddc_dev;

#endif  /* _SDDC_I2C_H_ */
