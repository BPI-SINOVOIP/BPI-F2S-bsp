/*
 * veye290.c - veye290 Image Sensor Driver
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
#include "veye290.h"

/*
 *
 */
static const struct regval veye327_global_regs[] = {
	{REG_NULL, 0x00},
};

/*
 * max_framerate 30fps
 * mipi_datarate per lane 594Mbps
 */
static struct regval veye290_1920x1080_regs [] = {
	{ REG_NULL, 0x00}
};

static const struct veye290_mode supported_modes[] = {
	{
		.width = 1920,
		.height = 1080,
		.reg_list = veye290_1920x1080_regs,
	},
};

/* Read registers up to 4 at a time */
static int veye290_read_reg(struct i2c_client *client, u16 reg, unsigned int len, u32 *val)
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
static int veye290_write_reg(struct i2c_client *client, u16 reg, u32 len, u32 val)
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

static int veye290_write_array(struct i2c_client *client, const struct regval *regs)
{
	u32 i;
	int ret = 0;

	FUNC_DEBUG();

	for (i = 0; ((ret == 0) && (regs[i].addr != REG_NULL)); i++) {
		ret = veye290_write_reg(client, regs[i].addr,
					veye290_REG_VALUE_08BIT,
					regs[i].val);
	}

	return ret;
}

static int __veye290_start_stream(struct veye290 *veye290)
{
	int ret;

	FUNC_DEBUG();

	ret = veye290_write_array(veye290->client, veye290->cur_mode->reg_list);
	if (ret) {
		return ret;
	}

	return veye290_write_reg(veye290->client,
				 veye290_REG_CTRL_MODE,
				 veye290_REG_VALUE_08BIT,
				 veye290_MODE_STREAMING);
}

static int __veye290_stop_stream(struct veye290 *veye290)
{
	FUNC_DEBUG();

	return veye290_write_reg(veye290->client,
				 veye290_REG_CTRL_MODE,
				 veye290_REG_VALUE_08BIT,
				 veye290_MODE_SW_STANDBY);
}

static int veye290_s_stream(struct v4l2_subdev *sd, int on)
{
	struct veye290 *veye290 = to_veye290(sd);
	int ret = 0;

	FUNC_DEBUG();

	mutex_lock(&veye290->mutex);
	//on = !!on;
	if (on == veye290->streaming)
		goto unlock_and_return;

	if (on) {
		ret = __veye290_start_stream(veye290);
		if (ret) {
			DBG_ERR("Start streaming failed while write sensor registers!\n");
			goto unlock_and_return;
		}
	} else {
		__veye290_stop_stream(veye290);
	}

	veye290->streaming = on;

unlock_and_return:
	mutex_unlock(&veye290->mutex);

	return ret;
}

static struct v4l2_subdev_video_ops veye290_subdev_video_ops = {
	.s_stream       = veye290_s_stream,
};
static struct v4l2_subdev_ops veye290_subdev_ops = {
	.video          = &veye290_subdev_video_ops,
};

static int veye290_check_sensor_id(struct veye290 *veye290, struct i2c_client *client)
{
	u32 val = 0;
	int ret;

	ret = veye290_read_reg(client, veye290_REG_CHIP_ID,
			      veye290_REG_VALUE_08BIT, &val);
	if ((ret != 0) || (val != CHIP_ID)) {
		DBG_ERR("Unexpected sensor (id = 0x%04x, ret = %d)!\n", val, ret);
		return -1;
	}
	DBG_INFO("Check sensor id success (id = 0x%04x).\n", val);

	return 0;
}

static int veye290_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct veye290 *veye290;
	struct v4l2_subdev *sd;
	int ret;

	FUNC_DEBUG();

	veye290 = devm_kzalloc(dev, sizeof(*veye290), GFP_KERNEL);
	if (!veye290) {
		DBG_ERR("Failed to allocate memory for \'veye290\'!\n");
		return -ENOMEM;
	}

	veye290->client = client;

	veye290->sensor_data.mode = 0;	//CONFIG_IMX290_1920x1080
	veye290->sensor_data.fourcc = V4L2_PIX_FMT_UYVY;
	veye290->cur_mode = &supported_modes[veye290->sensor_data.mode];

	mutex_init(&veye290->mutex);

	sd = &veye290->subdev;
	v4l2_i2c_subdev_init(sd, client, &veye290_subdev_ops);
	DBG_INFO("Initialized V4L2 I2C subdevice.\n");

	ret = veye290_check_sensor_id(veye290, client);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &veye290_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
#endif

	// Let IMX290 go to standby mode
	veye290_write_reg(veye290->client, veye290_REG_CTRL_MODE,
				 veye290_REG_VALUE_08BIT, veye290_MODE_SW_STANDBY);

	ret = v4l2_async_register_subdev(sd);
	if (ret) {
		DBG_ERR("V4L2 async register subdevice failed.\n");
		goto err_clean_entity;
	}
	DBG_INFO("Registered V4L2 sub-device successfully.\n");

	v4l2_set_subdev_hostdata(sd, &veye290->sensor_data);

	return 0;

err_clean_entity:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif

	return ret;
}

static int veye290_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct veye290 *veye290 = to_veye290(sd);

	FUNC_DEBUG();

	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&priv->subdev->entity);
#endif
	v4l2_ctrl_handler_free(&veye290->ctrl_handler);
	mutex_destroy(&veye290->mutex);

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id veye290_of_match[] = {
	{ .compatible = "sony,veye290" },
	{},
};
MODULE_DEVICE_TABLE(of, veye290_of_match);
#endif

static const struct i2c_device_id veye290_match_id[] = {
	{ "veye290", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, veye290_match_id);

static struct i2c_driver veye290_i2c_driver = {
	.probe          = veye290_probe,
	.remove         = veye290_remove,
	.id_table       = veye290_match_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "veye290",
		.of_match_table = of_match_ptr(veye290_of_match),
	},
};

module_i2c_driver(veye290_i2c_driver);

MODULE_DESCRIPTION("Sunplus veye290 sensor driver");
MODULE_LICENSE("GPL v2");
