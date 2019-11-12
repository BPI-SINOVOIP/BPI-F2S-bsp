#ifndef __ov5647_H__
#define __ov5647_H__


#if 0
#define FUNC_DEBUG()                            printk(KERN_INFO "[ov5647] %s (L:%d)\n", __FUNCTION__, __LINE__)
#else
#define FUNC_DEBUG()
#endif
#define DBG_INFO(fmt, args ...)                 printk(KERN_INFO "[ov5647] " fmt, ## args)
#define DBG_ERR(fmt, args ...)                  printk(KERN_ERR "[ov5647] ERR: " fmt, ## args)

#define REG_NULL                                0xFFFF


/* ov5647 Registers */
#define ov5647_REG_CHIP_ID                      0x300A
#define CHIP_ID                                 0x5647

#define OV5647_REG_TIMING_X_OUTPUT_SIZE		0x3808
#define OV5647_REG_TIMING_Y_OUTPUT_SIZE		0x380A
#define OV5647_REG_ISP_CTRL3D			0x503D

#define OV5647_SW_STANDBY			0x0100
#define ov5647_MODE_SW_STANDBY                  0x0
#define ov5647_MODE_STREAMING                   BIT(0)

#define ov5647_REG_VALUE_08BIT                  1
#define ov5647_REG_VALUE_16BIT                  2
#define ov5647_REG_VALUE_24BIT                  3


#define OV5647_SW_RESET				0x0103
#define OV5640_REG_PAD_OUT 			0x300D
#define OV5647_REG_FRAME_OFF_NUMBER 		0x4202
#define OV5647_REG_MIPI_CTRL00			0x4800
#define OV5647_REG_MIPI_CTRL14			0x4814

#define MIPI_CTRL00_CLOCK_LANE_GATE		BIT(5)
#define MIPI_CTRL00_LINE_SYNC_ENABLE		BIT(4)
#define MIPI_CTRL00_BUS_IDLE			BIT(2)
#define MIPI_CTRL00_CLOCK_LANE_DISABLE		BIT(0)

#define to_ov5647(sd)                           container_of(sd, struct ov5647, subdev)


struct regval {
	u16     addr;
	u8      val;
};

struct ov5647_mode {
	u32                     width;
	u32                     height;
	const struct regval     *reg_list;
};

struct sp_subdev_sensor_data {
	int                     mode;
	u32                     fourcc;
};

struct ov5647 {
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

	const struct ov5647_mode        *cur_mode;
	struct sp_subdev_sensor_data    sensor_data;
};

#endif
