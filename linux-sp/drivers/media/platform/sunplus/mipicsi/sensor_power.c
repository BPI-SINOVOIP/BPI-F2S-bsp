#include <linux/io.h>
#include <linux/delay.h>
#include "isp_api.h"
#include "sensor_power.h"


void powerSensorOn_RAM(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 data;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	/* enable sensor mclk and i2c sck */
	writeb(0x00, &(regs->reg[0x2781]));
	writeb(0x08, &(regs->reg[0x2785]));

	data = readb(&(regs->reg[0x2042])) | 0x03;
	writeb(data, &(regs->reg[0x2042]));     /* xgpio[8,9] output enable */
	data = readb(&(regs->reg[0x2044])) | 0x03;
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[8,9] output high - power up */

	mdelay(1);

	//writeb(0xFD, &(regs->reg[0x2042]));
	//writeb(0xFD, &(regs->reg[0x2044]));
	data = readb(&(regs->reg[0x2044])) & (~0x1);
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[8] output low - reset */

	mdelay(1);

	data = readb(&(regs->reg[0x2044])) | 0x03;
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[8,9] output high - power up */

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}

void powerSensorDown_RAM(struct mipi_isp_info *isp_info)
{
	struct mipi_isp_reg *regs = (struct mipi_isp_reg *)((u64)isp_info->mipi_isp_regs - ISP_BASE_ADDRESS);
	u8 data;

	ISPAPI_LOGD("%s start\n", __FUNCTION__);

	/* disable sensor mclk and i2c sck */
	//writeb(0x48, &(regs->reg[0x2781]));
	writeb(0x00, &(regs->reg[0x2785]));
	udelay(6);                              /* delay 128 extclk = 6 us */

	/* xgpio[8) - 0: reset, 1: normal */
	data = readb(&(regs->reg[0x2042])) | 0x01;
	writeb(data, &(regs->reg[0x2042]));     /* xgpio[8] output enable */
	data = readb(&(regs->reg[0x2044])) & (~0x1);
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[8] output low - reset */

	/* xgpio[9) - 0: power down, 1: power up */
	data = readb(&(regs->reg[0x2042])) | 0x02;
	writeb(data, &(regs->reg[0x2042]));     /* xgpio[9] output enable */
	data = readb(&(regs->reg[0x2044])) & 0xFD;
	writeb(data, &(regs->reg[0x2044]));     /* xgpio[9] output low - power down */

	ISPAPI_LOGD("%s end\n", __FUNCTION__);
}
