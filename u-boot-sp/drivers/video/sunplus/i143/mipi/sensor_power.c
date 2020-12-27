#include <common.h>

#include "isp_api.h"
#include "sensor_power.h"


void powerSensorOn_RAM(u8 isp)
{
	/* ISP0 path */
	if ((isp == 0) || (isp == 2))
	{
		ISPAPB_LOGI("%s, ISP0 path\n", __FUNCTION__);

		/* enable sensor mclk and i2c sck */
		ISPAPB0_REG8(0x2781) = 0x00;
		ISPAPB0_REG8(0x2785) = 0x08;

		ISPAPB0_REG8(0x2042) |= 0x03;       /* xgpio[8,9] output enable */
		ISPAPB0_REG8(0x2044) |= 0x03;       /* xgpio[8,9] output high - power up */

		mdelay(1);

		//ISPAPB0_REG8(0x2042) &= 0xFD;
		//ISPAPB0_REG8(0x2044) &= 0xFD;
		ISPAPB0_REG8(0x2044) &= (~0x1);     /* xgpio[8] output low - reset */

		mdelay(1);

		ISPAPB0_REG8(0x2044) |= 0x03;		/* xgpio[8,9] output high - power up */
	}

	/* ISP1 path */
	if ((isp == 1) || (isp == 2))
	{
		ISPAPB_LOGI("%s, ISP1 path\n", __FUNCTION__);

		/* enable sensor mclk and i2c sck */
		ISPAPB1_REG8(0x2781) = 0x00;
		ISPAPB1_REG8(0x2785) = 0x08;

		ISPAPB1_REG8(0x2042) |= 0x03;		/* xgpio[8,9] output enable */
		ISPAPB1_REG8(0x2044) |= 0x03;		/* xgpio[8,9] output high - power up */

		mdelay(1);

		//ISPAPB1_REG8(0x2042) &= 0xFD;
		//ISPAPB1_REG8(0x2044) &= 0xFD;
		ISPAPB1_REG8(0x2044) &= (~0x1); 	/* xgpio[8] output low - reset */

		mdelay(1);

		ISPAPB1_REG8(0x2044) |= 0x03;		/* xgpio[8,9] output high - power up */
	}
}

void powerSensorDown_RAM(u8 isp)
{
	/* ISP0 path */
	if ((isp == 0) || (isp == 2))
	{
		ISPAPB_LOGI("%s, ISP0 path\n", __FUNCTION__);

		/* disable sensor mclk and i2c sck */
		//ISPAPB0_REG8(0x2781) = 0x48;
		ISPAPB0_REG8(0x2785) = 0x00;
		udelay(6);                          /* delay 128 extclk = 6 us */

		/* xgpio[8) - 0: reset, 1: normal */
		ISPAPB0_REG8(0x2042) |= 0x01;       /* xgpio[8] output enable */
		ISPAPB0_REG8(0x2044) &= (~0x1);     /* xgpio[8] output low - reset */

		/* xgpio[9) - 0: power down, 1: power up */
		ISPAPB0_REG8(0x2042) |= 0x02;       /* xgpio[9] output enable */
		ISPAPB0_REG8(0x2044) &= 0xFD;       /* xgpio[9] output low - power down */
	}

	/* ISP1 path */
	if ((isp == 1) || (isp == 2))
	{
		ISPAPB_LOGI("%s, ISP1 path\n", __FUNCTION__);

		/* disable sensor mclk and i2c sck */
		//ISPAPB1_REG8(0x2781) = 0x48;
		ISPAPB1_REG8(0x2785) = 0x00;
		udelay(6);							/* delay 128 extclk = 6 us */

		/* xgpio[8) - 0: reset, 1: normal */
		ISPAPB1_REG8(0x2042) |= 0x01;		/* xgpio[8] output enable */
		ISPAPB1_REG8(0x2044) &= (~0x1); 	/* xgpio[8] output low - reset */

		/* xgpio[9) - 0: power down, 1: power up */
		ISPAPB1_REG8(0x2042) |= 0x02;		/* xgpio[9] output enable */
		ISPAPB1_REG8(0x2044) &= 0xFD;		/* xgpio[9] output low - power down */
	}
}
