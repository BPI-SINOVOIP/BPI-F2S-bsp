#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
//#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <asm/types.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

//#include "tiserdes_setting.h"
extern void tw8809_720p_init(void);
extern void TW8824_720p_init(void);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("weiwuliu");
MODULE_DESCRIPTION("TISERDES Serdes driver");



#define DEVNUM_COUNT        1
#define DEVNUM_NAME         ("tiserdes")
#define DEVNUM_MINOR_START  0


typedef enum
{
	NOT_USE0,                        //0x00
	NOT_USE1,                        //0x01
	NOT_USE2,                        //0x02
	NOT_USE3,                        //0x03

	NOT_USE4,                        //0x03
	NOT_USE5,                        //0x03
	NOT_USE6,                        //0x03
	NOT_USE7,                        //0x03

	TI914_READ_BYTE,                 //0x04
	TI914_WRITE_BYTE,                //0x05
	TI914_READ_BYTE_SMBUS,           //0x06
	TI914_WRITE_BYTE_SMBUS,          //0x07
	TI913_READ_BYTE,                 //0x08
	TI913_WRITE_BYTE,                //0x09
	TI913_READ_BYTE_SMBUS,           //0x0a
	TI913_WRITE_BYTE_SMBUS,          //0x0b
	TW8824_READ_BYTE,                //0x0c
	TW8824_WRITE_BYTE,               //0x0d
	TI9546A_READ_BYTE,               //0x0e
	TI9546A_WRITE_BYTE,              //0x0f
	APTINA_AP0100_READ,              //0x10
	APTINA_AP0100_WRITE,             //0x11
	APTINA_AP0100_READ_BYTE,         //0x12
	APTINA_AP0100_WRITE_BYTE,        //0x13
	APTINA_AP0100_READ_BYTE_SMBUS,   //0x14
	APTINA_AP0100_WRITE_BYTE_SMBUS,  //0x15
	I2C_CMD_COUNT,                   //0x16
}i2c_cmd_mode_t;


#define CAM1_CS_GPIO_NUM    188
#define CAM2_CS_GPIO_NUM    152
#define CAM3_CS_GPIO_NUM    186
#define CAM4_CS_GPIO_NUM    176


// I2C Address (7 bit)
#define I2C_TW8824_ID           0x44
#define I2C_TW8809_ID           0x45
#define I2C_APTINA_AP0100_ID    0x48
#define I2C_TCA9546A_ID         0x70
#define I2C_TI914_ID            0x60
#define I2C_TI913_ID            0x58


//#define dbg_printk if(0) printk
#define dbg_printk printk

static dev_t tiserdes_dev_num = 0;
static struct cdev dev_cdev;
static struct class* dev_class = NULL;


struct i2c_board_info i2cInfo;
struct i2c_adapter *i2cAdapter = NULL;
struct i2c_client *i2cClient44 = NULL;  // INTERSIL_TW8824
struct i2c_client *i2cClient48 = NULL;  // APTINA_AP0100
struct i2c_client *i2cClient45 = NULL;  // INTERSIL_TW8809
struct i2c_client *i2cClient58 = NULL;  // TI_913
struct i2c_client *i2cClient59 = NULL;  // TI_913
struct i2c_client *i2cClient60 = NULL;  // TI_914
struct i2c_client *i2cClient70 = NULL;  // TI_TCA9546A


#define iic_write(client, command, value) \
	i2c_smbus_write_byte_data(client, command, value)

#define iic_read(client, command) \
	i2c_smbus_read_byte_data(client, command)


typedef enum
{
	CAMALL,
	CAM1,
	CAM2,
	CAM3,
	CAM4,
}camera_ch_t;


typedef struct _ioctl_data_t
{
	unsigned char data[2];
	unsigned char addr[2];
}ioctl_data_t;

static int __init init_i2c(void)
{

	memset(&i2cInfo, 0, sizeof(struct i2c_board_info));
	strlcpy(i2cInfo.type, "tiserdes", I2C_NAME_SIZE);

	i2cAdapter = i2c_get_adapter(0);
	if( i2cAdapter == NULL ) {
		printk(KERN_ERR "[%s:%d] i2c_get_adpter failed!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	i2cInfo.addr = I2C_TCA9546A_ID;
	i2cClient70 = i2c_new_device(i2cAdapter, &i2cInfo);
	if( i2cClient70 == NULL ) {
		printk(KERN_ERR "[%s:%d] i2c_new_device #28 failed!\n", __FUNCTION__, __LINE__);
		i2c_del_adapter(i2cAdapter);
		i2cAdapter = NULL;
		return -3;
	}

	i2cInfo.addr = I2C_TI914_ID;
	i2cClient60 = i2c_new_device(i2cAdapter, &i2cInfo);
	if( i2cClient60 == NULL ) {
		printk(KERN_ERR "[%s:%d] i2c_new_device #28 failed!\n", __FUNCTION__, __LINE__);
		i2c_del_adapter(i2cAdapter);
		i2cAdapter = NULL;
		return -3;
	}

	i2cInfo.addr = I2C_TI913_ID;
	i2cClient58 = i2c_new_device(i2cAdapter, &i2cInfo);
	if( i2cClient58 == NULL ) {
		printk(KERN_ERR "[%s:%d] i2c_new_device #28 failed!\n", __FUNCTION__, __LINE__);
		i2c_del_adapter(i2cAdapter);
		i2cAdapter = NULL;
		return -3;
	}

	i2cInfo.addr = I2C_APTINA_AP0100_ID;
	i2cClient48 = i2c_new_device(i2cAdapter, &i2cInfo);
	if( i2cClient48 == NULL ) {
		printk(KERN_ERR "[%s:%d] i2c_new_device #28 failed!\n", __FUNCTION__, __LINE__);
		i2c_del_adapter(i2cAdapter);
		i2cAdapter = NULL;
		return -3;
	}

	i2cInfo.addr = I2C_TW8809_ID;
	i2cClient45 = i2c_new_device(i2cAdapter, &i2cInfo);
	if( i2cClient45 == NULL ) {
		printk(KERN_ERR "[%s:%d] i2c_new_device #28 failed!\n", __FUNCTION__, __LINE__);
		i2c_del_adapter(i2cAdapter);
		i2cAdapter = NULL;
		return -3;
	}

	i2cInfo.addr = I2C_TW8824_ID;
	i2cClient44 = i2c_new_device(i2cAdapter, &i2cInfo);
	if( i2cClient44 == NULL ) {
		printk(KERN_ERR "[%s:%d] i2c_new_device #28 failed!\n", __FUNCTION__, __LINE__);
		i2c_del_adapter(i2cAdapter);
		i2cAdapter = NULL;
		return -3;
	}

	printk("[%s:%d] Initial i2c success!\n", __FUNCTION__, __LINE__);

	return 0;
}


static int i2c_read_byte_data(const struct i2c_client *client, u8 addr, u8* data)
{
	struct i2c_msg msg;

	msg.addr = client->addr;

	// write
	//msg.flags = I2C_M_NOSTART;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = &addr;
	i2c_transfer(client->adapter, &msg, 1);

	// read 
	//msg.flags |= I2C_M_RD;
	msg.flags = I2C_M_RD;
	msg.len = 1;
	msg.buf = data;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}


/*
* clock : I2C clock , unit = kHZ, 
	max 511kHZ, min 13.5kHZ(1~13)
	if clock=0, default clock will be used
*/
static int i2c_read_byte_data_clock(const struct i2c_client *client, u8 addr, u8* data, u16 clock)
{
	struct i2c_msg msg;

	if(clock > 511)
		clock = 511;
	clock = (clock >>1) & 0xff;
	msg.addr = (clock<<8)|(client->addr);
	printk("\nmsg.addr = 0x%04x \n", msg.addr);

	// write
	//msg.flags = I2C_M_NOSTART;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = &addr;
	i2c_transfer(client->adapter, &msg, 1);

	// read 
	//msg.flags |= I2C_M_RD;
	msg.flags = I2C_M_RD;
	msg.len = 1;
	msg.buf = data;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}


static int i2c_write_byte_data(const struct i2c_client *client, u8 addr, u8 data)
{
	struct i2c_msg msg;
	u8 buf[2] = {addr, data};

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}


static int i2c_write_byte_data_clock(const struct i2c_client *client, u8 addr, u8 data, u16 clock)
{
	struct i2c_msg msg;
	u8 buf[2] = {addr, data};

	if(clock > 511)
		clock = 511;
	clock = (clock >>1) & 0xff;
	msg.addr = (clock<<8)|(client->addr);
	printk("\nmsg.addr = 0x%04x \n", msg.addr);

	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}

static int aptina_ap0100_read(const struct i2c_client *client, u8* addr, u8* data)
{
	struct i2c_msg msg;

	msg.addr = client->addr;

	// write
	msg.flags = 0;
	//msg.flags = 0|I2C_M_NOSTART;	//for test restart mode
	msg.len = 2;
	msg.buf = addr;
	i2c_transfer(client->adapter, &msg, 1);

	// read
	//msg.flags |= I2C_M_RD;
	msg.flags = I2C_M_RD;
	msg.len = 2;
	msg.buf = data;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}

static int aptina_ap0100_read_clock(const struct i2c_client *client, u8* addr, u8* data, u16 clock)
{
	struct i2c_msg msg;

	if(clock > 511)
		clock = 511;
	clock = (clock >>1) & 0xff;
	msg.addr = (clock<<8)|(client->addr);
	printk("\nmsg.addr = 0x%04x \n", msg.addr);

	// write
	msg.flags = 0;
	//msg.flags = 0|I2C_M_NOSTART;  //for test restart mode
	msg.len = 2;
	msg.buf = addr;
	i2c_transfer(client->adapter, &msg, 1);

	// read 
	//msg.flags |= I2C_M_RD;
	msg.flags = I2C_M_RD;
	msg.len = 2;
	msg.buf = data;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
} 


static int aptina_ap0100_read_byte(const struct i2c_client *client, u8* addr, u8* data)
{
	struct i2c_msg msg;

	msg.addr = client->addr;

	// write
	msg.flags = 0;
	//msg.flags = 0|I2C_M_NOSTART;  //for test restart mode
	msg.len = 2;
	msg.buf = addr;
	i2c_transfer(client->adapter, &msg, 1);

	// read
	//msg.flags |= I2C_M_RD;
	msg.flags = I2C_M_RD;
	msg.len = 1;
	msg.buf = data;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}

static int aptina_ap0100_read_byte_clock(const struct i2c_client *client, u8* addr, u8* data, u16 clock)
{
	struct i2c_msg msg;

	if(clock > 511)
		clock = 511;
	clock = (clock >>1) & 0xff;
	msg.addr = (clock<<8)|(client->addr);
	printk("\nmsg.addr = 0x%04x \n", msg.addr);

	// write
	msg.flags = 0;
	//msg.flags = 0|I2C_M_NOSTART;  //for test restart mode
	msg.len = 2;
	msg.buf = addr;
	i2c_transfer(client->adapter, &msg, 1);

	// read
	//msg.flags |= I2C_M_RD;
	msg.flags = I2C_M_RD;
	msg.len = 1;
	msg.buf = data;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}

static int aptina_ap0100_write(const struct i2c_client *client, u8* addr, u8* data)
{
	struct i2c_msg msg;
	u8 buf[4] = {addr[0], addr[1], data[0], data[1]};

	msg.addr = client->addr;

	// write
	msg.flags = 0;
	msg.len = 4;
	msg.buf = buf;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}

static int aptina_ap0100_write_clock(const struct i2c_client *client, u8* addr, u8* data, u16 clock)
{
	struct i2c_msg msg;
	u8 buf[4] = {addr[0], addr[1], data[0], data[1]};

	if(clock > 511)
		clock = 511;
	clock = (clock >>1) & 0xff;
	msg.addr = (clock<<8)|(client->addr);
	printk("\nmsg.addr = 0x%04x \n", msg.addr);

	// write
	msg.flags = 0;
	msg.len = 4;
	msg.buf = buf;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}

static int aptina_ap0100_write_byte(const struct i2c_client *client, u8* addr, u8* data)
{
	struct i2c_msg msg;
	u8 buf[4] = {addr[0], addr[1], data[0], data[1]};

	msg.addr = client->addr;

	// write
	msg.flags = 0;
	msg.len = 3;
	msg.buf = buf;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}

static int aptina_ap0100_write_byte_clock(const struct i2c_client *client, u8* addr, u8* data, u16 clock)
{
	struct i2c_msg msg;
	u8 buf[4] = {addr[0], addr[1], data[0], data[1]};

	if(clock > 511)
		clock = 511;
	clock = (clock >>1) & 0xff;
	msg.addr = (clock<<8)|(client->addr);
	printk("\nmsg.addr = 0x%04x \n", msg.addr);

	// write
	msg.flags = 0;
	msg.len = 3;
	msg.buf = buf;
	i2c_transfer(client->adapter, &msg, 1);

	return 0;
}

static void camera_i2c_channel_sel(camera_ch_t channel)
{
	switch(channel) {
		case CAM1:
			i2c_smbus_write_byte(i2cClient70, 0x01);
			break;

		case CAM2:
			i2c_smbus_write_byte(i2cClient70, 0x01<<1);
			break;

		case CAM3:
			i2c_smbus_write_byte(i2cClient70, 0x01<<2);
			break;

		case CAM4:
			i2c_smbus_write_byte(i2cClient70, 0x01<<3);
			break;

		default:
			//dbg_printk("[%s] Invalid Camera Channel\n", __FUNCTION__);
			printk("[%s] Invalid Camera Channel, camera channel = %d\n", __FUNCTION__, channel);
			break;
	}
}

static void init_ti914(void)
{
	camera_ch_t current_cam_ch;	
	for(current_cam_ch = CAM1; current_cam_ch <= CAM4; current_cam_ch++){
		camera_i2c_channel_sel(current_cam_ch);
		i2c_smbus_write_byte_data(i2cClient60, 0x07, 0xb0);  // SER Alias
		//i2c_smbus_write_byte_data(i2cClient60, 0x03, 0x0d);
		//i2c_smbus_write_byte_data(i2cClient60, 0x21, 0x85);
		//i2c_smbus_write_byte_data(i2cClient60, 0x07, 0xb0);
		i2c_smbus_write_byte_data(i2cClient60, 0x08, 0x90);  // Slave ID
		i2c_smbus_write_byte_data(i2cClient60, 0x10, 0x90);  // Slave Alias

		i2c_smbus_write_byte_data(i2cClient60, 0x40, 0x0c);  // SCL high time 0.6us
		i2c_smbus_write_byte_data(i2cClient60, 0x41, 0x1a);  // SCL low time 1.3us
	}
}

static void init_ti913(void)
{
	camera_ch_t current_cam_ch;	
	for(current_cam_ch = CAM1; current_cam_ch <= CAM4; current_cam_ch++){
		camera_i2c_channel_sel(current_cam_ch);
		i2c_smbus_write_byte_data(i2cClient58, 0x11, 0x0c);  // SCL high time 0.6us
		i2c_smbus_write_byte_data(i2cClient58, 0x12, 0x1a);  // SCL low time 1.3us
	}
}

/*
static void init_tw8809(void)
{
	camera_ch_t current_cam_ch;
	for(current_cam_ch = CAM1; current_cam_ch <= CAM4; current_cam_ch++){
		camera_i2c_channel_sel(current_cam_ch);
		tw8809_720p_init();
	}
}
*/

static void init_tw8824(void)
{
	camera_ch_t current_cam_ch;
	for(current_cam_ch = CAM1; current_cam_ch <= CAM4; current_cam_ch++){
		camera_i2c_channel_sel(current_cam_ch);
		TW8824_720p_init();
	}
}

static long tiserdes_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	static ioctl_data_t i2c_cmd_data;
	static camera_ch_t cam_num;
	camera_ch_t current_cam_ch;

	//dbg_printk("[%s] get cmd: 0x%x\n", __FUNCTION__, cmd);
	printk("\n[%s] get cmd: 0x%x\n", __FUNCTION__, cmd);

	if( (cmd == TI9546A_WRITE_BYTE) || (cmd == TI9546A_READ_BYTE) ) {
		switch(cmd) {
			case TI9546A_READ_BYTE:
				i2c_cmd_data.data[0] = i2c_smbus_read_byte(i2cClient70);

					//printk("i2c_cmd_data.data[0] = %d \n", i2c_cmd_data.data[0]);

				switch(i2c_cmd_data.data[0]) {
				case 0x00:
					cam_num = CAMALL;
					break;

				case 0x01:
					cam_num = CAM1;
					break;

				case 0x02:
					cam_num = CAM2;
					break;

				case 0x04:
					cam_num = CAM3;
					break;

				case 0x08:
					cam_num = CAM4;
					break;

				default:
					cam_num = CAMALL;
					printk("[%s] Invalid Camera Channel, camera channel = %d\n", __FUNCTION__, i2c_cmd_data.data[0]);
					printk("Input channel should be 0x01(CAM1), 0x02(CAM2), 0x04(CAM3), 0x08(CAM4)\n");
					break;
				}

				copy_to_user((void __user *)arg, &cam_num, sizeof(camera_ch_t));
				//printk("2.1 Camera num = %d \n", cam_num);
				break;

			case TI9546A_WRITE_BYTE:
				if ( copy_from_user(&cam_num, (const void __user *)arg, sizeof(camera_ch_t)) ) {
					printk(KERN_WARNING "[%s] copy from user fail!!\n", __FUNCTION__);
					return -1;
				}
				//printk("1. Camera num = %d \n", cam_num);

				switch(cam_num) {
				case CAMALL:
					i2c_cmd_data.data[0] = 0;
					break;

				case CAM1:
					i2c_cmd_data.data[0] = 1;
					break;

				case CAM2:
					i2c_cmd_data.data[0] = 2;
					break;

				case CAM3:
					i2c_cmd_data.data[0] = 4;
					break;

				case CAM4:
					i2c_cmd_data.data[0] = 8;
					break;

				default:
					i2c_cmd_data.data[0] = 0;
					printk("[%s] Invalid Camera Channel, camera channel = %d\n", __FUNCTION__, i2c_cmd_data.data[0]);
					printk("Input channel should be 0x01(CAM1), 0x02(CAM2), 0x04(CAM3), 0x08(CAM4)\n");
					break;
				}

				i2c_smbus_write_byte(i2cClient70, i2c_cmd_data.data[0]);

				//printk("2.2 Camera num = %d \n", cam_num);
				break;
		}
		dbg_printk("[%s] DONE!\n", __FUNCTION__);
		return ret;
	}

	if ( copy_from_user( &i2c_cmd_data, (const void __user *)arg, sizeof(ioctl_data_t)) ) {
		printk(KERN_WARNING "[%s] copy from user fail!!\n", __FUNCTION__);
		return -1;
	}
	printk("1. Camera num = %d \n", cam_num);

	if(cam_num == CAMALL) {
		for(current_cam_ch = CAM1; current_cam_ch <= CAM4; current_cam_ch++){
			//dbg_printk("Set Camera %d \n", current_cam_ch);
			printk("All camera set in Cemera %d \n", current_cam_ch);

			camera_i2c_channel_sel(current_cam_ch);

			switch(cmd) {
				case TI914_READ_BYTE:
					i2c_cmd_data.data[0] = i2c_smbus_read_byte_data(i2cClient60, i2c_cmd_data.addr[0]);
					copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
					dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);
					break;

				case TI914_WRITE_BYTE:
					i2c_smbus_write_byte_data(i2cClient60, i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
					break;

				case TW8824_READ_BYTE:
					i2c_cmd_data.data[0] = i2c_smbus_read_byte_data(i2cClient44, i2c_cmd_data.addr[0]);
					copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
					dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);
					break;

				case TW8824_WRITE_BYTE:
					i2c_smbus_write_byte_data(i2cClient44, i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
					//printk("addr = 0x%02x, data = 0x%02x\n", i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
					break;

				default:
					dbg_printk("[%s] Invalid Cmd\n", __FUNCTION__);
					break;
			}
		}
	}
	else {  // Set One Camera

		camera_i2c_channel_sel(cam_num);

/*
	APTINA_AP0100_READ,             //0x0c
	APTINA_AP0100_WRITE,            //0x0d
	APTINA_AP0100_READ_BYTE,        //0x0e
	APTINA_AP0100_WRITE_BYTE,       //0x0f
	APTINA_AP0100_READ_BYTE_SMBUS,  //0x10
	APTINA_AP0100_WRITE_BYTE_SMBUS, //0x11
*/

		switch (cmd) {
			case TI914_READ_BYTE:
				//ret = i2c_read_byte_data(i2cClient60, i2c_cmd_data.addr[0], i2c_cmd_data.data);
				ret = i2c_read_byte_data_clock(i2cClient60, i2c_cmd_data.addr[0], i2c_cmd_data.data, 10);
				copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
				dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);
				break;

			case TI914_WRITE_BYTE:
				//ret = i2c_write_byte_data(i2cClient60, i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
				ret = i2c_write_byte_data_clock(i2cClient60, i2c_cmd_data.addr[0], i2c_cmd_data.data[0], 10);
				break;

			case TI914_READ_BYTE_SMBUS:
				i2c_cmd_data.data[0] = i2c_smbus_read_byte_data(i2cClient60, i2c_cmd_data.addr[0]);
				copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
				dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);
				break;

			case TI914_WRITE_BYTE_SMBUS:
				i2c_smbus_write_byte_data(i2cClient60, i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
				break;

			case TI913_READ_BYTE:
				//ret = i2c_read_byte_data(i2cClient58, i2c_cmd_data.addr[0], i2c_cmd_data.data );
				ret = i2c_read_byte_data_clock(i2cClient58, i2c_cmd_data.addr[0], i2c_cmd_data.data, 10);
				copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
				dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);

				printk("kernel read 0x%02x\n", i2c_cmd_data.data[0]);
				break;

			case TI913_WRITE_BYTE:
				//ret = i2c_write_byte_data(i2cClient58, i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
				ret = i2c_write_byte_data_clock(i2cClient58, i2c_cmd_data.addr[0], i2c_cmd_data.data[0], 10);
				break;

			case TI913_READ_BYTE_SMBUS:
				i2c_cmd_data.data[0] = i2c_smbus_read_byte_data(i2cClient58, i2c_cmd_data.addr[0]);
				copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
				dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);

				printk("kernel read 0x%02x\n", i2c_cmd_data.data[0]);
				break;

			case TI913_WRITE_BYTE_SMBUS:
				i2c_smbus_write_byte_data(i2cClient58, i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
				break;

			case TW8824_READ_BYTE:
				i2c_cmd_data.data[0] = i2c_smbus_read_byte_data(i2cClient44, i2c_cmd_data.addr[0]);
				copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
				dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);
				break;

			case TW8824_WRITE_BYTE:
				i2c_smbus_write_byte_data(i2cClient44, i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
				break;

			case APTINA_AP0100_READ:
				//ret = aptina_ap0100_read(i2cClient48, i2c_cmd_data.addr, i2c_cmd_data.data);
				printk("\n APTINA_AP0100_READ 0x%02x 0x%02x ", i2c_cmd_data.addr[0], i2c_cmd_data.addr[1]);

				ret = aptina_ap0100_read_clock(i2cClient48, i2c_cmd_data.addr, i2c_cmd_data.data, 10);
				copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
				dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);
				break;	

			case APTINA_AP0100_WRITE:
				//ret = aptina_ap0100_write(i2cClient48, i2c_cmd_data.addr, i2c_cmd_data.data);
				ret = aptina_ap0100_write_clock(i2cClient48, i2c_cmd_data.addr, i2c_cmd_data.data, 10);
				break;

			case APTINA_AP0100_READ_BYTE:
				//ret = aptina_ap0100_read_byte(i2cClient48, i2c_cmd_data.addr, i2c_cmd_data.data);
				ret = aptina_ap0100_read_byte_clock(i2cClient48, i2c_cmd_data.addr, i2c_cmd_data.data, 10);
				copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
				dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);
				break;

			case APTINA_AP0100_WRITE_BYTE:
				//ret = aptina_ap0100_write_byte(i2cClient48, i2c_cmd_data.addr, i2c_cmd_data.data);
				ret = aptina_ap0100_write_byte_clock(i2cClient48, i2c_cmd_data.addr, i2c_cmd_data.data, 10);
				break;

			case APTINA_AP0100_READ_BYTE_SMBUS:
				i2c_cmd_data.data[0] = i2c_smbus_read_byte_data(i2cClient48, i2c_cmd_data.addr[0]);
				copy_to_user((void __user *)arg, i2c_cmd_data.data, sizeof(i2c_cmd_data.data));
				dbg_printk("0x%2x\n", i2c_cmd_data.data[0]);
				break;

			case APTINA_AP0100_WRITE_BYTE_SMBUS:
				i2c_smbus_write_byte_data(i2cClient48, i2c_cmd_data.addr[0], i2c_cmd_data.data[0]);
				break;

			default:
				dbg_printk("[%s] Invalid Cmd\n", __FUNCTION__);
				break;
		}
	}
	dbg_printk("[%s] DONE!\n", __FUNCTION__);
	return ret;
}

static const struct file_operations tiserdes_fops =
{
	.open           = NULL,
	.release        = NULL,
	.unlocked_ioctl = tiserdes_ioctl,
};

static int __init tiserdes_dev_init(void)
{
	int ret = 0;
	printk("insert TISERDES driver!\n");

	// Request a device number from kernel

	ret = alloc_chrdev_region( &tiserdes_dev_num, DEVNUM_MINOR_START, DEVNUM_COUNT, DEVNUM_NAME );
	if (ret) {
		printk(KERN_WARNING "%s : could not allocate device\n", __func__);
		return ret;
	}
	else {
		printk("%s : registered with major number:%i, minor number:%i\n",
		__func__, MAJOR(tiserdes_dev_num), MINOR(tiserdes_dev_num));
	}

	// Initialize device parameters
	cdev_init( &dev_cdev, &tiserdes_fops);
	dev_cdev.owner = THIS_MODULE;
	dev_cdev.ops = &tiserdes_fops;
	ret = cdev_add( &dev_cdev, tiserdes_dev_num, DEVNUM_COUNT );

	// sys class related
	dev_class = class_create(THIS_MODULE, "tiserdes");
	if (IS_ERR(dev_class)) {
		printk(KERN_WARNING "class_simple_create fail %s\n", "tiserdes_class");
		return -1;
	}
	printk("class_create %s\n", "tiserdes_class");

	device_create( dev_class, NULL, tiserdes_dev_num,NULL ,DEVNUM_NAME );
	printk("device_create %s\n", DEVNUM_NAME);

	init_i2c();
	//init_ti914();  //mark
	//init_ti913();  //mark
	//init_tw8809();
	//init_tw8824();  //mark

	//camera_i2c_channel_sel(CAM1);  //mark

	return 0;
}

static void __exit tiserdes_dev_exit(void)
{
	printk("exit TISERDES driver!\n");

	device_destroy(dev_class, tiserdes_dev_num);
	class_destroy(dev_class);

	cdev_del( &dev_cdev );
	unregister_chrdev_region( tiserdes_dev_num, DEVNUM_COUNT );
}

module_init(tiserdes_dev_init);
module_exit(tiserdes_dev_exit);
