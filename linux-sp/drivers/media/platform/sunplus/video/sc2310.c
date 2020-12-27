/*
 * sc2310.c - sc2310 Image Sensor Driver
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/sysfs.h>
#include <media/media-entity.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include <linux/pinctrl/consumer.h>
#include "sc2310.h"

//sensor struct
//#define MAX_SENSOR_REG_LEN		(400) // This causes a warning. warning: the frame size of 2416 bytes is larger than 2048 bytes.
#define MAX_SENSOR_REG_LEN		(200)

static const u8 SC2310_INIT_FILE[] = {
	#include "sc2310_SensorInit.txt"
};

// struct sc2310_data
// PS. By different sensor type, adr or dat could be one or two bytes.
//
// ex.
// 0xFE, 0x00, 0xC8, 0x00, 0x00, //DELAY= 200ms
// 0x00, 0x30, 0x1A, 0x10, 0xD8, //sensor addr=0x301a, sensor data=0x10d8
typedef struct sc2310_data
{
	u8 type;    // 0xFE:do delay in ms , 0x00:sensor register 
	u16 adr;
	u8 dat;
} sc2310_data_t;

typedef struct sc2310_init_file
{
	u16 count;
	sc2310_data_t data[MAX_SENSOR_REG_LEN];
} sc2310_init_file_t;

/* sensor frame rate initialization */
/* sensor 0 */
#define SENSOR_FRAME_RATE   0

static unsigned char SF_SENSOR_FRAME_RATE[8][2][4] = {
	{        /*  op    a[0]  a[1]  d         30fps*/
		{        0x00, 0x32, 0x0C, 0x04,        },
		{        0x00, 0x32, 0x0d, 0x4C,        },
	},
	{        /*  op    a[0]  a[1]  d         25fps*/
		{        0x00, 0x32, 0x0C, 0x05,        },
		{        0x00, 0x32, 0x0d, 0x28,        },
	},
	{        /*  op    a[0]  a[1]  d         20fps*/
		{        0x00, 0x32, 0x0C, 0x06,        },
		{        0x00, 0x32, 0x0d, 0x72,        },
	},
	{        /*  op    a[0]  a[1]  d         15fps*/
		{        0x00, 0x32, 0x0C, 0x08,        },
		{        0x00, 0x32, 0x0d, 0x98,        },
	},
	{        /*  op    a[0]  a[1]  d         10fps*/
		{        0x00, 0x32, 0x0C, 0x0C,        },
		{        0x00, 0x32, 0x0d, 0xE4,        },
	},
	{        /*  op    a[0]  a[1]  d         5fps*/
		{        0x00, 0x32, 0x0C, 0x19,        },
		{        0x00, 0x32, 0x0d, 0xC8,        },
	},
	{        /*  op    a[0]  a[1]  d         3fps*/
		{        0x00, 0x32, 0x0C, 0x2A,        },
		{        0x00, 0x32, 0x0d, 0xF8,        },
	},
	{        /*  op    a[0]  a[1]  d         1fps*/
		{        0x00, 0x32, 0x0C, 0x80,        },
		{        0x00, 0x32, 0x0d, 0xE8,        },
	}
};

// 1280x800/ raw10/ 120fps/ 2 lane
static const struct regval sc2310_1280x800_regs[] = {
	{ REG_NULL, 0x00}
};

static const struct sc2310_mode supported_modes[] = {
	{
		.width = 1920,
		.height = 1080,
		.reg_list = sc2310_1280x800_regs,
	},
};

/* Read registers up to 4 at a time */
static int sc2310_read_reg(struct i2c_client *client, u16 reg, unsigned int len, u32 *val)
{
	struct i2c_msg msgs[2];
	u8 *data_be_p;
	__be32 data_be = 0;
	__be16 reg_addr_be = cpu_to_be16(reg);
	int ret;

	if ((len > 4) || !len)
		return -EINVAL;

	data_be_p = (u8 *)&data_be;

	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = I2C_M_NOSTART;
	msgs[0].len = 2;
	msgs[0].buf = (u8 *)&reg_addr_be;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_be_p[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));

	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = be32_to_cpu(data_be);
	return 0;
}

/* Write registers up to 4 at a time */
static int sc2310_write_reg(struct i2c_client *client, u16 reg, u32 len, u32 val)
{
	u32 buf_i, val_i;
	u8 buf[6];
	u8 *val_p;
	__be32 val_be;

	if (len > 4)
		return -EINVAL;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	val_be = cpu_to_be32(val);
	val_p = (u8 *)&val_be;
	buf_i = 2;
	val_i = 4 - len;

	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	if (i2c_master_send(client, buf, len + 2) != len + 2)
		return -EIO;

	return 0;
}

/*
	@sc2310_init
*/
void sc2310_init(struct i2c_client *client)
{
	sc2310_init_file_t read_file;
	int i, total;
	u8 data[4];

	FUNC_DEBUG();

 	// conver table data to sc2310_init_file_t struct
	total = ARRAY_SIZE(SC2310_INIT_FILE);
	data[0] = SC2310_INIT_FILE[0];
	data[1] = SC2310_INIT_FILE[1];
	data[2] = SC2310_INIT_FILE[2];
	data[3] = SC2310_INIT_FILE[3];
	read_file.count = (data[0]<<8)|data[1];

	DBG_INFO("%s, total=%d\n", __FUNCTION__, total);
	DBG_INFO("%s, count=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n",
			__FUNCTION__, read_file.count, data[0], data[1], data[2], data[3]);
	for (i = 2; i < total; i=i+4)
	{
		data[0] = SC2310_INIT_FILE[i];
		data[1] = SC2310_INIT_FILE[i+1];
		data[2] = SC2310_INIT_FILE[i+2];
		data[3] = SC2310_INIT_FILE[i+3];
		read_file.data[(i-2)/4].type = data[0];
		read_file.data[(i-2)/4].adr  = (data[1]<<8)|data[2];
		read_file.data[(i-2)/4].dat  = data[3];
	}

	DBG_INFO("%s, %d, count=%d\n", __FUNCTION__, __LINE__, read_file.count);
	for (i = 0; i < read_file.count; i++)
	{
		if (read_file.data[i].type == 0xFE)
		{
			udelay(read_file.data[i].adr*1000);
		}
		else if (read_file.data[i].type == 0x00)
		{
			sc2310_write_reg(client, (unsigned long)read_file.data[i].adr,
				SC2310_REG_VALUE_08BIT, read_file.data[i].dat);
		}
	}

	/* set seneor frame rate */
	/* In SensorInit.txt, the default is 30fps. So skip setting frame rate if  SENSOR_FRAME_RATE = 0*/
	if (SENSOR_FRAME_RATE) {
		read_file.count = 2;
		for (i = 0; i < read_file.count; i++)
		{
			data[0] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][0];
			data[1] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][1];
			data[2] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][2];
			data[3] = SF_SENSOR_FRAME_RATE[SENSOR_FRAME_RATE][i][3];
			read_file.data[i].type = data[0];
			read_file.data[i].adr  = (data[1]<<8)|data[2];
			read_file.data[i].dat  = data[3];
			DBG_INFO("%s, i=%d, data[0]=0x%02x, data[1]=0x%02x, data[2]=0x%02x, data[3]=0x%02x\n",
				__FUNCTION__, i, data[0], data[1], data[2], data[3]);
		}

		DBG_INFO("%s, %d, count=%d\n", __FUNCTION__, __LINE__, read_file.count);
		for (i = 0; i < read_file.count; i++)
		{
			DBG_INFO("%s, %d, type=0x%02x, adr=0x%04x, dat=0x%04x\n", __FUNCTION__, __LINE__,
				read_file.data[i].type, read_file.data[i].adr, read_file.data[i].dat);

			if (read_file.data[i].type == 0xFE)
			{
				udelay(read_file.data[i].adr*1000);
			}
			else if (read_file.data[i].type == 0x00)
			{
				sc2310_write_reg(client, (unsigned long)read_file.data[i].adr,
					SC2310_REG_VALUE_08BIT, read_file.data[i].dat);
			}
		}
	}

	DBG_INFO("%s end\n", __FUNCTION__);
}

static int __sc2310_start_stream(struct sc2310 *sc2310)
{
	FUNC_DEBUG();

	sc2310_init(sc2310->client);

	return 0;
}

static int __sc2310_stop_stream(struct sc2310 *sc2310)
{
	FUNC_DEBUG();
/*
	return sc2310_write_reg(sc2310->client,
				 SC2310_REG_CTRL_MODE,
				 SC2310_REG_VALUE_08BIT,
				 SC2310_MODE_SW_STANDBY);*/
	return 0;
}

static int sc2310_s_stream(struct v4l2_subdev *sd, int on)
{
	struct sc2310 *sc2310 = to_sc2310(sd);
	int ret = 0;

	FUNC_DEBUG();

	mutex_lock(&sc2310->mutex);
	//on = !!on;
	if (on == sc2310->streaming)
		goto unlock_and_return;

	if (on) {
		ret = __sc2310_start_stream(sc2310);
		if (ret) {
			DBG_ERR("Start streaming failed while write sensor registers!\n");
			goto unlock_and_return;
		}
	} else {
		__sc2310_stop_stream(sc2310);
	}

	sc2310->streaming = on;

unlock_and_return:
	mutex_unlock(&sc2310->mutex);

	return ret;
}

static struct v4l2_subdev_video_ops sc2310_subdev_video_ops = {
	.s_stream       = sc2310_s_stream,
};
static struct v4l2_subdev_ops sc2310_subdev_ops = {
	.video          = &sc2310_subdev_video_ops,
};

static int sc2310_check_sensor_id(struct sc2310 *sc2310, struct i2c_client *client)
{
	u32 val = 0;
	int ret;

	ret = sc2310_read_reg(client, SC2310_REG_CHIP_ID, SC2310_REG_VALUE_16BIT, &val);

	if ((ret != 0) || (val != CHIP_ID)) {
		DBG_ERR("Unexpected sensor (id = 0x%04x, ret = %d)!\n", val, ret);
		return -1;
	}
	DBG_INFO("Check sensor id success (id = 0x%04x).\n", val);

	return 0;
}

static int sc2310_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct sc2310 *sc2310;
	struct v4l2_subdev *sd;
	int ret;

	DBG_INFO("SC2310 driver is probed\n");

	sc2310 = devm_kzalloc(dev, sizeof(*sc2310), GFP_KERNEL);
	if (!sc2310) {
		DBG_ERR("Failed to allocate memory for \'sc2310\'!\n");
		return -ENOMEM;
	}

	sc2310->client = client;
	sc2310->sensor_data.mode = 0;
	sc2310->sensor_data.fourcc = V4L2_PIX_FMT_UYVY;
	sc2310->cur_mode = &supported_modes[sc2310->sensor_data.mode];

	mutex_init(&sc2310->mutex);

	sd = &sc2310->subdev;
	v4l2_i2c_subdev_init(sd, client, &sc2310_subdev_ops);
	DBG_INFO("Initialized V4L2 I2C subdevice.\n");

	ret = sc2310_check_sensor_id(sc2310, client);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &sc2310_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
#endif

	ret = v4l2_async_register_subdev(sd);
	if (ret) {
		DBG_ERR("V4L2 async register subdevice failed.\n");
		goto err_clean_entity;
	}
	DBG_INFO("Registered V4L2 sub-device successfully.\n");

	v4l2_set_subdev_hostdata(sd, &sc2310->sensor_data);

	DBG_INFO("SC2310 driver is installed!\n");
	return 0;

err_clean_entity:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif

	return ret;
}

static int sc2310_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sc2310 *sc2310 = to_sc2310(sd);

	DBG_INFO("SC2310 driver is removing\n");

	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&priv->subdev->entity);
#endif
	v4l2_ctrl_handler_free(&sc2310->ctrl_handler);
	mutex_destroy(&sc2310->mutex);

	DBG_INFO("SC2310 driver is removed\n");
	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id sc2310_of_match[] = {
	{ .compatible = "SunplusIT,sc2310" },
	{},
};
MODULE_DEVICE_TABLE(of, sc2310_of_match);
#endif

static const struct i2c_device_id sc2310_match_id[] = {
	{ "sc2310", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, sc2310_match_id);

static struct i2c_driver sc2310_i2c_driver = {
	.probe          = sc2310_probe,
	.remove         = sc2310_remove,
	.id_table       = sc2310_match_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "sc2310",
		.of_match_table = of_match_ptr(sc2310_of_match),
	},
};

module_i2c_driver(sc2310_i2c_driver);

MODULE_DESCRIPTION("Sunplus sc2310 sensor driver");
MODULE_LICENSE("GPL v2");
