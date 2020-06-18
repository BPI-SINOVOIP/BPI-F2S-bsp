#ifndef __OV9281_H__
#define __OV9281_H__


#if 0
#define FUNC_DEBUG()                            printk(KERN_INFO "[OV9281_ISP] %s (L:%d)\n", __FUNCTION__, __LINE__)
#else
#define FUNC_DEBUG()
#endif
#define DBG_INFO(fmt, args ...)                 printk(KERN_INFO "[OV9281_ISP] " fmt, ## args)
#define DBG_ERR(fmt, args ...)                  printk(KERN_ERR "[OV9281_ISP] ERR: " fmt, ## args)

#define REG_NULL                                0xFFFF


/* OV9281 Registers */
#define OV9281_REG_CHIP_ID                      0x300A
#define CHIP_ID                                 0x9281
#define OV9281_REG_CTRL_MODE                    0x0100
#define OV9281_MODE_SW_STANDBY                  0x0
#define OV9281_MODE_STREAMING                   BIT(0)

#define OV9281_REG_VALUE_08BIT                  1
#define OV9281_REG_VALUE_16BIT                  2
#define OV9281_REG_VALUE_24BIT                  3

#define to_ov9281(sd)                           container_of(sd, struct ov9281, subdev)


struct regval {
	u16     addr;
	u8      val;
};

struct ov9281_mode {
	u32                     width;
	u32                     height;
	const struct regval     *reg_list;
};

struct sp_subdev_sensor_data {
	int                     mode;
	u32                     fourcc;
};

struct ov9281 {
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

	const struct ov9281_mode        *cur_mode;
	struct sp_subdev_sensor_data    sensor_data;
};

#endif
