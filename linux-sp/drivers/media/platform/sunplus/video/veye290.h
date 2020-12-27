#ifndef __veye290_H__
#define __veye290_H__


#if 0
#define FUNC_DEBUG()                            printk(KERN_INFO "[veye290] %s (L:%d)\n", __FUNCTION__, __LINE__)
#else
#define FUNC_DEBUG()
#endif
#define DBG_INFO(fmt, args ...)                 printk(KERN_INFO "[veye290] " fmt, ## args)
#define DBG_ERR(fmt, args ...)                  printk(KERN_ERR "[veye290] ERR: " fmt, ## args)

#define REG_NULL                                0xFFFF


/* veye290 Registers */
#define veye290_REG_CHIP_ID                      0x0001
#define CHIP_ID                                 0x06
#define veye290_REG_CTRL_MODE                    0x000B
#define veye290_MODE_SW_STANDBY                  0x0
#define veye290_MODE_STREAMING                   BIT(0)

#define veye290_REG_VALUE_08BIT                  1
#define veye290_REG_VALUE_16BIT                  2
#define veye290_REG_VALUE_24BIT                  3

#define to_veye290(sd)                           container_of(sd, struct veye290, subdev)


struct regval {
	u16     addr;
	u8      val;
};

struct veye290_mode {
	u32                     width;
	u32                     height;
	const struct regval     *reg_list;
};

struct sp_subdev_sensor_data {
	int                     mode;
	u32                     fourcc;
};

struct veye290 {
	struct i2c_client               *client;

	struct gpio_desc                *reset_gpio;
	struct gpio_desc                *pwdn_gpio;
	struct gpio_desc                *pwr_gpio;
	struct gpio_desc                *pwr_snd_gpio;
	struct pinctrl                  *pinctrl;
	struct pinctrl_state            *pins_default;
	struct pinctrl_state            *pins_sleep;

	struct v4l2_subdev              subdev;
	struct v4l2_ctrl_handler        ctrl_handler;
	struct v4l2_ctrl                *exposure;
	struct v4l2_ctrl                *anal_gain;
	struct v4l2_ctrl                *digi_gain;
	struct v4l2_ctrl                *hblank;
	struct v4l2_ctrl                *vblank;
	struct v4l2_ctrl                *test_pattern;

	struct mutex                    mutex;
	bool                            streaming;

	const struct veye290_mode        *cur_mode;
	struct sp_subdev_sensor_data    sensor_data;
};

#endif
