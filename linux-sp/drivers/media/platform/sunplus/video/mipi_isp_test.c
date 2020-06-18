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
#include "mipi_isp_test.h"

static const struct isp_test_mode supported_modes[] = {
	{
		.width = 1920,
		.height = 1080,
		.reg_list = NULL,
	},
};

static struct v4l2_subdev_video_ops isp_test_subdev_video_ops = {
	.s_stream       = NULL,
};
static struct v4l2_subdev_ops isp_test_subdev_ops = {
	.video          = &isp_test_subdev_video_ops,
};

static int isp_test_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct isp_test *isp_test;
	struct v4l2_subdev *sd;
	int ret;

	FUNC_DEBUG();

	isp_test = devm_kzalloc(dev, sizeof(*isp_test), GFP_KERNEL);
	if (!isp_test) {
		DBG_ERR("Failed to allocate memory for \'isp_test\'!\n");
		return -ENOMEM;
	}

	isp_test->client = client;
	isp_test->sensor_data.mode = 0;
	isp_test->sensor_data.fourcc = V4L2_PIX_FMT_YUYV;
	isp_test->cur_mode = &supported_modes[isp_test->sensor_data.mode];

	mutex_init(&isp_test->mutex);

	// Register a V4L2 I2C subdevice to kernel
	sd = &isp_test->subdev;
	v4l2_i2c_subdev_init(sd, client, &isp_test_subdev_ops);
	DBG_INFO("Initialized V4L2 I2C subdevice.\n");

#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	sd->internal_ops = &isp_test_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
#endif

	// Register a V4L2 subdevice to kernel
	ret = v4l2_async_register_subdev(sd);
	if (ret) {
		DBG_ERR("V4L2 async register subdevice failed.\n");
		goto err_clean_entity;
	}
	DBG_INFO("Registered V4L2 sub-device successfully.\n");

	v4l2_set_subdev_hostdata(sd, &isp_test->sensor_data);

	return 0;

err_clean_entity:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif

	return ret;
}

static int isp_test_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct isp_test *isp_test = to_isp_test(sd);

	FUNC_DEBUG();

	v4l2_async_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&priv->subdev->entity);
#endif
	v4l2_ctrl_handler_free(&isp_test->ctrl_handler);
	mutex_destroy(&isp_test->mutex);

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id isp_test_of_match[] = {
	{ .compatible = "sunplus,mipi_isp_test" },
	{},
};
MODULE_DEVICE_TABLE(of, isp_test_of_match);
#endif

static const struct i2c_device_id isp_test_match_id[] = {
	{ "mipi_isp_test", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, isp_test_match_id);

static struct i2c_driver isp_test_i2c_driver = {
	.probe          = isp_test_probe,
	.remove         = isp_test_remove,
	.id_table       = isp_test_match_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "mipi_isp_test",
		.of_match_table = of_match_ptr(isp_test_of_match),
	},
};


static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&isp_test_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&isp_test_i2c_driver);
}

module_init(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("Sunplus mipi isp test driver");
MODULE_LICENSE("GPL v2");
