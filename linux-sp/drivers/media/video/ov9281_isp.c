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
#include "ov9281_isp.h"

static const struct ov9281_mode supported_modes[] = {
	{
		.width = 1280,
		.height = 720,
		.reg_list = NULL,
	},
};

static struct v4l2_subdev_video_ops ov9281_subdev_video_ops = {
	.s_stream       = NULL,
};
static struct v4l2_subdev_ops ov9281_subdev_ops = {
	.video          = &ov9281_subdev_video_ops,
};

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
	ov9281->sensor_data.fourcc = V4L2_PIX_FMT_YUYV;
	ov9281->cur_mode = &supported_modes[ov9281->sensor_data.mode];

	mutex_init(&ov9281->mutex);

	sd = &ov9281->subdev;
	v4l2_i2c_subdev_init(sd, client, &ov9281_subdev_ops);
	DBG_INFO("Initialized V4L2 I2C subdevice.\n");

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
	{ .compatible = "ovti,ov9281_isp" },
	{},
};
MODULE_DEVICE_TABLE(of, ov9281_of_match);
#endif

static const struct i2c_device_id ov9281_match_id[] = {
	{ "ov9281_isp", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, ov9281_match_id);

static struct i2c_driver ov9281_i2c_driver = {
	.probe          = ov9281_probe,
	.remove         = ov9281_remove,
	.id_table       = ov9281_match_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ov9281_isp",
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
