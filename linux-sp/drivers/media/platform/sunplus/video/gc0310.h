#ifndef __GC0310_H__
#define __GC0310_H__


#if 0
#define FUNC_DEBUG()                            printk(KERN_INFO "[GC0310] %s (L:%d)\n", __FUNCTION__, __LINE__)
#else
#define FUNC_DEBUG()
#endif
#define DBG_INFO(fmt, args ...)                 printk(KERN_INFO "[GC0310] " fmt, ## args)
#define DBG_ERR(fmt, args ...)                  printk(KERN_ERR "[GC0310] ERR: " fmt, ## args)


/* GC0310 Registers */
#define GC0310_REG_CHIP_ID                      0xF0
#define CHIP_ID                                 0xa310
#define GC0310_REG_CTRL_MODE                    0x00

#define GC0310_REG_VALUE_08BIT                  1
#define GC0310_REG_VALUE_16BIT                  2
#define GC0310_REG_VALUE_24BIT                  3

#define to_gc0310(sd)                           container_of(sd, struct gc0310, subdev)


struct regval {
	u8      addr;
	u8      val;
};

struct gc0310_mode {
	u32                     width;
	u32                     height;
	const struct regval     *reg_list;
	u32                     reg_num;
};

struct sp_subdev_sensor_data {
	int                     mode;
	u32                     fourcc;
};

struct gc0310 {
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

	const struct gc0310_mode        *cur_mode;
	struct sp_subdev_sensor_data    sensor_data;
};

#endif
