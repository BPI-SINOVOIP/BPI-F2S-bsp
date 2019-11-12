/*
 * imx219.c - imx219 Image Sensor Driver
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
#include "imx219.h"

static const struct regval imx219_3280x2464_regs[] = {
	// 3280x2464/ raw10/ 15fps/ 2 lane
	{0x30EB, 0x05},	{0x30EB, 0x0C},	{0x300A, 0xFF},	{0x300B, 0xFF},
	{0x30EB, 0x05},	{0x30EB, 0x09},	{0x0114, 0x01},	{0x0128, 0x00},
	{0x012A, 0x18},	{0x012B, 0x00},	{0x0160, 0x09},	{0x0161, 0xC8},
	{0x0162, 0x0D},	{0x0163, 0x78},	{0x0164, 0x00},	{0x0165, 0x00},
	{0x0166, 0x0C},	{0x0167, 0xCF},	{0x0168, 0x00},	{0x0169, 0x00},
	{0x016A, 0x09},	{0x016B, 0x9F},	{0x016C, 0x0C},	{0x016D, 0xD0},
	{0x016E, 0x09},	{0x016F, 0xA0},	{0x0170, 0x01},	{0x0171, 0x01},
	{0x0172, 0x03},	{0x0174, 0x00},	{0x0175, 0x00},	{0x018C, 0x0A},
	{0x018D, 0x0A},	{0x0301, 0x05},	{0x0303, 0x01},	{0x0304, 0x03},
	{0x0305, 0x03},	{0x0306, 0x00},	{0x0307, 0x2B},	{0x0309, 0x0A},
	{0x030B, 0x01},	{0x030C, 0x00},	{0x030D, 0x55},	{0x455E, 0x00},
	{0x471E, 0x4B},	{0x4767, 0x0F},	{0x4750, 0x14},	{0x4540, 0x00},
	{0x47B4, 0x14},	{0x4713, 0x30},	{0x478B, 0x10},	{0x478F, 0x10},
	{0x4797, 0x0E},	{0x479B, 0x0E}, 
	{0x0100, 0x01},	{ REG_NULL, 0x00}
#if 0
{0x0160, 0x04},	{0x0161, 0x59}, {0x0164, 0x02},	{0x0165, 0xA8},
{0x0166, 0x0A},	{0x0167, 0x27},	{0x0168, 0x02},	{0x0169, 0xB4},
{0x016A, 0x06},	{0x016B, 0xEB},	{0x016C, 0x07},	{0x016D, 0x80},
{0x016E, 0x04},	{0x016F, 0x38},	
{0x0172, xxx},
{0x0307, 0x39},
{0x030D, 0x72}
{0x4793, 0x10},
#endif
};

static struct regval imx219_1920x1080_regs [] = {
	// 1920x1080/ raw10/ 48fps/ 2 lane
	{0x30EB, 0x05},	{0x30EB, 0x0C},	{0x300A, 0xFF},	{0x300B, 0xFF},
	{0x30EB, 0x05},	{0x30EB, 0x09},	{0x0114, 0x01},	{0x0128, 0x00},
	{0x012A, 0x18},	{0x012B, 0x00},	{0x0160, 0x04},	{0x0161, 0x59},
	{0x0162, 0x0D},	{0x0163, 0x78},	{0x0164, 0x02},	{0x0165, 0xA8},
	{0x0166, 0x0A},	{0x0167, 0x27},	{0x0168, 0x02},	{0x0169, 0xB4},
	{0x016A, 0x06},	{0x016B, 0xEB},	{0x016C, 0x07},	{0x016D, 0x80},
	{0x016E, 0x04},	{0x016F, 0x38},	{0x0170, 0x01},	{0x0171, 0x01},
	{0x0172, 0x03},	{0x0174, 0x00},	{0x0175, 0x00},	{0x018C, 0x0A},
	{0x018D, 0x0A}, {0x0301, 0x05},	{0x0303, 0x01},	{0x0304, 0x03},
	{0x0305, 0x03},	{0x0306, 0x00},	{0x0307, 0x39},	{0x0309, 0x0A},
	{0x030B, 0x01},	{0x030C, 0x00},	{0x030D, 0x72},	{0x455E, 0x00},
	{0x471E, 0x4B},	{0x4767, 0x0F},	{0x4750, 0x14},	{0x4540, 0x00},
	{0x47B4, 0x14},	{0x4713, 0x30},	{0x478B, 0x10},	{0x478F, 0x10},
	{0x4793, 0x10},	{0x4797, 0x0E},	{0x479B, 0x0E},
	{0x0100, 0x01},	{ REG_NULL, 0x00}
};

static const struct imx219_mode supported_modes[] = {
	{
		.width = 3280,
		.height = 2464,
		.reg_list = imx219_3280x2464_regs,
	},	
	{
		.width = 1920,
		.height = 1080,
		.reg_list = imx219_1920x1080_regs,
	},
};

/* Read registers up to 4 at a time */
static int imx219_read_reg(struct i2c_client *client, u16 reg, unsigned int len, u32 *val)
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
static int imx219_write_reg(struct i2c_client *client, u16 reg, u32 len, u32 val)
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

static int imx219_write_array(struct i2c_client *client, const struct regval *regs)
{
	u32 i;
	int ret = 0;

	FUNC_DEBUG();

	for (i = 0; ((ret == 0) && (regs[i].addr != REG_NULL)); i++) {
		ret = imx219_write_reg(client, regs[i].addr,
					imx219_REG_VALUE_08BIT,
					regs[i].val);
	}

	return ret;
}

static int __imx219_start_stream(struct imx219 *imx219)
{
	int ret;

	FUNC_DEBUG();

	ret = imx219_write_array(imx219->client, imx219->cur_mode->reg_list);
	if (ret) {
		return ret;
	}

	return imx219_write_reg(imx219->client,
				 imx219_REG_CTRL_MODE,
				 imx219_REG_VALUE_08BIT,
				 imx219_MODE_STREAMING);
}

static int __imx219_stop_stream(struct imx219 *imx219)
{
	FUNC_DEBUG();

	return imx219_write_reg(imx219->client,
				 imx219_REG_CTRL_MODE,
				 imx219_REG_VALUE_08BIT,
				 imx219_MODE_SW_STANDBY);
}

static int imx219_s_stream(struct v4l2_subdev *sd, int on)
{
	struct imx219 *imx219 = to_imx219(sd);
	int ret = 0;

	FUNC_DEBUG();

	mutex_lock(&imx219->mutex);
	//on = !!on;
	if (on == imx219->streaming)
		goto unlock_and_return;

	if (on) {
		ret = __imx219_start_stream(imx219);
		if (ret) {
			DBG_ERR("Start streaming failed while write sensor registers!\n");
			goto unlock_and_return;
		}
	} else {
		__imx219_stop_stream(imx219);
	}

	imx219->streaming = on;

unlock_and_return:
	mutex_unlock(&imx219->mutex);

	return ret;
}

static struct v4l2_subdev_video_ops imx219_subdev_video_ops = {
	.s_stream       = imx219_s_stream,
};
static struct v4l2_subdev_ops imx219_subdev_ops = {
	.video          = &imx219_subdev_video_ops,
};

static int imx219_check_sensor_id(struct imx219 *imx219, struct i2c_client *client)
{
	u32 val = 0;
	int ret;

	ret = imx219_read_reg(client, imx219_REG_CHIP_ID,
			      imx219_REG_VALUE_16BIT, &val);
	if ((ret != 0) || (val != CHIP_ID)) {
		DBG_ERR("Unexpected sensor (id = 0x%04x, ret = %d)!\n", val, ret);
		return -1;
	}
	DBG_INFO("Check sensor id success (id = 0x%04x).\n", val);

	return 0;
}

static int imx219_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct imx219 *imx219;
	struct v4l2_subdev *sd;
	int ret;

	FUNC_DEBUG();

	imx219 = devm_kzalloc(dev, sizeof(*imx219), GFP_KERNEL);
	if (!imx219) {
		DBG_ERR("Failed to allocate memory for \'imx219\'!\n");
		return -ENOMEM;
	}

	imx219->client = client;

#ifdef CONFIG_IMX219_3280x2464
	imx219->sensor_data.mode = 0;
#else 
	imx219->sensor_data.mode = 1;	//CONFIG_IMX219_1920x1080
#endif

	imx219->sensor_data.fourcc = V4L2_PIX_FMT_SRGGB8;
	imx219->cur_mode = &supported_modes[imx219->sensor_data.mode];

	mutex_init(&imx219->mutex);

	sd = &imx219->subdev;
	v4l2_i2c_subdev_init(sd, client, &imx219_subdev_ops);
	DBG_INFO("Initialized V4L2 I2C subdevice.\n");

	ret = imx219_check_sensor_id(imx219, client);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &imx219_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
#endif

	ret = v4l2_async_register_subdev(sd);
	if (ret) {
		DBG_ERR("V4L2 async register subdevice failed.\n");
		goto err_clean_entity;
	}
	DBG_INFO("Registered V4L2 sub-device successfully.\n");

	v4l2_set_subdev_hostdata(sd, &imx219->sensor_data);

	return 0;

err_clean_entity:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif

	return ret;
}

static int imx219_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct imx219 *imx219 = to_imx219(sd);

	FUNC_DEBUG();

	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&priv->subdev->entity);
#endif
	v4l2_ctrl_handler_free(&imx219->ctrl_handler);
	mutex_destroy(&imx219->mutex);

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id imx219_of_match[] = {
	{ .compatible = "sony,imx219" },
	{},
};
MODULE_DEVICE_TABLE(of, imx219_of_match);
#endif

static const struct i2c_device_id imx219_match_id[] = {
	{ "imx219", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, imx219_match_id);

static struct i2c_driver imx219_i2c_driver = {
	.probe          = imx219_probe,
	.remove         = imx219_remove,
	.id_table       = imx219_match_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "imx219",
		.of_match_table = of_match_ptr(imx219_of_match),
	},
};


static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&imx219_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&imx219_i2c_driver);
}

module_init(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("Sunplus imx219 sensor driver");
MODULE_LICENSE("GPL v2");
