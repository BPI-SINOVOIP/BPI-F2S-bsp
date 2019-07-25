/*
 * ov9281.c - ov9281 Image Sensor Driver
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
#include "ov9281.h"

// 1280x800/ raw10/ 120fps/ 2 lane
static const struct regval ov9281_1280x800_regs[] = {
	{0x0103, 0x01}, {0x0302, 0x32}, {0x030d, 0x50}, {0x030e, 0x02},
	{0x3001, 0x00}, {0x3004, 0x00}, {0x3005, 0x00}, {0x3006, 0x04},
	{0x3011, 0x0a}, {0x3013, 0x18}, {0x301c, 0xf0}, {0x3022, 0x01},
	{0x3030, 0x10}, {0x3039, 0x32}, {0x303a, 0x00}, {0x3500, 0x00},
	{0x3501, 0x2a}, {0x3502, 0x90}, {0x3503, 0x08}, {0x3505, 0x8c},
	{0x3507, 0x03}, {0x3508, 0x00}, {0x3509, 0x10}, {0x3610, 0x80},
	{0x3611, 0xa0}, {0x3620, 0x6e}, {0x3632, 0x56}, {0x3633, 0x78},
	{0x3662, 0x05}, {0x3666, 0x00}, {0x366f, 0x5a}, {0x3680, 0x84},
	{0x3712, 0x80}, {0x372d, 0x22}, {0x3731, 0x80}, {0x3732, 0x30},
	{0x3778, 0x00}, {0x377d, 0x22}, {0x3788, 0x02}, {0x3789, 0xa4},
	{0x378a, 0x00}, {0x378b, 0x4a}, {0x3799, 0x20}, {0x3800, 0x00},
	{0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x00}, {0x3804, 0x05},
	{0x3805, 0x0f}, {0x3806, 0x03}, {0x3807, 0x2f}, {0x3808, 0x05},
	{0x3809, 0x00}, {0x380a, 0x03}, {0x380b, 0x20}, {0x380c, 0x02},
	{0x380d, 0xd8}, {0x380e, 0x03}, {0x380f, 0x8e}, {0x3810, 0x00},
	{0x3811, 0x08}, {0x3812, 0x00}, {0x3813, 0x08}, {0x3814, 0x11},
	{0x3815, 0x11}, {0x3820, 0x04}, {0x3821, 0x04}, {0x382c, 0x05},
	{0x382d, 0xb0}, {0x389d, 0x00}, {0x3881, 0x42}, {0x3882, 0x01},
	{0x3883, 0x00}, {0x3885, 0x02}, {0x38a8, 0x02}, {0x38a9, 0x80},
	{0x38b1, 0x00}, {0x38b3, 0x02}, {0x38c4, 0x00}, {0x38c5, 0xc0},
	{0x38c6, 0x04}, {0x38c7, 0x80}, {0x3920, 0xff}, {0x4003, 0x40},
	{0x4008, 0x04}, {0x4009, 0x0b}, {0x400c, 0x00}, {0x400d, 0x07},
	{0x4010, 0x40}, {0x4043, 0x40}, {0x4307, 0x30}, {0x4317, 0x00},
	{0x4501, 0x00}, {0x4507, 0x00}, {0x4509, 0x00}, {0x450a, 0x08},
	{0x4601, 0x04}, {0x470f, 0x00}, {0x4f07, 0x00}, {0x4800, 0x00},
	{0x5000, 0x9f}, {0x5001, 0x00}, {0x5e00, 0x00}, {0x5d00, 0x07},
	{0x5d01, 0x00}, {0x4f00, 0x04}, {0x4f10, 0x00}, {0x4f11, 0x98},
	{0x4f12, 0x0f}, {0x4f13, 0xc4}, {0x0100, 0x01},
	{0x3501, 0x38},
	{0x3502, 0x20},
	{0x5000, 0x87},
#if 0
	// from 120fps to 60fps
	{0x380e, 0x07}, {0x380f, 0x1c},
#endif
#if 0
	// from 120fps to 30fps
	{0x380e, 0x0e}, {0x380f, 0x38},
#endif
#if 0
	// from raw10 to raw8
	{0x0100, 0x00}, {0x3662, 0x07}, {0x4837, 0x14},  {0x4601, 0x4f},
	{0x0100, 0x01},
#endif
	{ REG_NULL, 0x00}
};

static const struct ov9281_mode supported_modes[] = {
	{
		.width = 1280,
		.height = 800,
		.reg_list = ov9281_1280x800_regs,
	},
};

/* Read registers up to 4 at a time */
static int ov9281_read_reg(struct i2c_client *client, u16 reg, unsigned int len, u32 *val)
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
	msgs[0].flags = 0;
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
static int ov9281_write_reg(struct i2c_client *client, u16 reg, u32 len, u32 val)
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

static int ov9281_write_array(struct i2c_client *client, const struct regval *regs)
{
	u32 i;
	int ret = 0;

	FUNC_DEBUG();

	for (i = 0; ((ret == 0) && (regs[i].addr != REG_NULL)); i++) {
		ret = ov9281_write_reg(client, regs[i].addr,
					OV9281_REG_VALUE_08BIT,
					regs[i].val);
	}

	return ret;
}

static int __ov9281_start_stream(struct ov9281 *ov9281)
{
	int ret;

	FUNC_DEBUG();

	ret = ov9281_write_array(ov9281->client, ov9281->cur_mode->reg_list);
	if (ret) {
		return ret;
	}

	return ov9281_write_reg(ov9281->client,
				 OV9281_REG_CTRL_MODE,
				 OV9281_REG_VALUE_08BIT,
				 OV9281_MODE_STREAMING);
}

static int __ov9281_stop_stream(struct ov9281 *ov9281)
{
	FUNC_DEBUG();

	return ov9281_write_reg(ov9281->client,
				 OV9281_REG_CTRL_MODE,
				 OV9281_REG_VALUE_08BIT,
				 OV9281_MODE_SW_STANDBY);
}

static int ov9281_s_stream(struct v4l2_subdev *sd, int on)
{
	struct ov9281 *ov9281 = to_ov9281(sd);
	int ret = 0;

	FUNC_DEBUG();

	mutex_lock(&ov9281->mutex);
	//on = !!on;
	if (on == ov9281->streaming)
		goto unlock_and_return;

	if (on) {
		ret = __ov9281_start_stream(ov9281);
		if (ret) {
			DBG_ERR("Start streaming failed while write sensor registers!\n");
			goto unlock_and_return;
		}
	} else {
		__ov9281_stop_stream(ov9281);
	}

	ov9281->streaming = on;

unlock_and_return:
	mutex_unlock(&ov9281->mutex);

	return ret;
}

static struct v4l2_subdev_video_ops ov9281_subdev_video_ops = {
	.s_stream       = ov9281_s_stream,
};
static struct v4l2_subdev_ops ov9281_subdev_ops = {
	.video          = &ov9281_subdev_video_ops,
};

static int ov9281_check_sensor_id(struct ov9281 *ov9281, struct i2c_client *client)
{
	u32 val = 0;
	int ret;

	ret = ov9281_read_reg(client, OV9281_REG_CHIP_ID,
			      OV9281_REG_VALUE_16BIT, &val);
	if ((ret != 0) || (val != CHIP_ID)) {
		DBG_ERR("Unexpected sensor (id = 0x%04x, ret = %d)!\n", val, ret);
		return -1;
	}
	DBG_INFO("Check sensor id success (id = 0x%04x).\n", val);

	return 0;
}

static int ov9281_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct ov9281 *ov9281;
	struct v4l2_subdev *sd;
	int ret;

	FUNC_DEBUG();

	ov9281 = devm_kzalloc(dev, sizeof(*ov9281), GFP_KERNEL);
	if (!ov9281) {
		DBG_ERR("Failed to allocate memory for \'ov9281\'!\n");
		return -ENOMEM;
	}

	ov9281->client = client;
	ov9281->sensor_data.mode = 0;
	ov9281->sensor_data.fourcc = V4L2_PIX_FMT_GREY;
	ov9281->cur_mode = &supported_modes[ov9281->sensor_data.mode];

	mutex_init(&ov9281->mutex);

	sd = &ov9281->subdev;
	v4l2_i2c_subdev_init(sd, client, &ov9281_subdev_ops);
	DBG_INFO("Initialized V4L2 I2C subdevice.\n");

	ret = ov9281_check_sensor_id(ov9281, client);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &ov9281_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
#endif

	ret = v4l2_async_register_subdev(sd);
	if (ret) {
		DBG_ERR("V4L2 async register subdevice failed.\n");
		goto err_clean_entity;
	}
	DBG_INFO("Registered V4L2 sub-device successfully.\n");

	v4l2_set_subdev_hostdata(sd, &ov9281->sensor_data);

	return 0;

err_clean_entity:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif

	return ret;
}

static int ov9281_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov9281 *ov9281 = to_ov9281(sd);

	FUNC_DEBUG();

	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&priv->subdev->entity);
#endif
	v4l2_ctrl_handler_free(&ov9281->ctrl_handler);
	mutex_destroy(&ov9281->mutex);

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id ov9281_of_match[] = {
	{ .compatible = "ovti,ov9281" },
	{},
};
MODULE_DEVICE_TABLE(of, ov9281_of_match);
#endif

static const struct i2c_device_id ov9281_match_id[] = {
	{ "ov9281", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, ov9281_match_id);

static struct i2c_driver ov9281_i2c_driver = {
	.probe          = ov9281_probe,
	.remove         = ov9281_remove,
	.id_table       = ov9281_match_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ov9281",
		.of_match_table = of_match_ptr(ov9281_of_match),
	},
};


static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&ov9281_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&ov9281_i2c_driver);
}

module_init(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("Sunplus ov9281 sensor driver");
MODULE_LICENSE("GPL v2");
