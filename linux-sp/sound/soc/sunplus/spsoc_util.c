/*
 * ALSA SoC AUD3502 UTILITY MACRO
 *
 * Author:	 <@sunplus.com>
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include "spsoc_util.h"

//#define SYNCHRONIZE_IO		__asm__ __volatile__ ("dmb" : : : "memory")
#define SYNCHRONIZE_IO		__asm__ __volatile__ ("" : : : "memory")

#if 0
UINT32 HWREG_R(UINT32 reg_name)
{
	AUD_INFO("%s IN\n", __func__);
	return 0;
}

void HWREG_W(UINT32 reg_name, UINT32 val)
{
	AUD_INFO("%s IN\n", __func__);
}

void delay_ms(UINT32 ms_count)
{
	AUD_INFO("%s IN\n", __func__);
}

void ADCP_INITIAL_BUFFER(UINT32 chan, UINT32 Fn)
{
	AUD_INFO("%s IN\n", __func__);
}

void ADCP_INITIAL_COEFF_INDEX(UINT32 chan, UINT32 Fn, UINT32 coeff_index, UINT32 ena_inc)
{
	AUD_INFO("%s IN\n", __func__);
}
void ADCP_SET_MODE(UINT32 up_ratio, UINT32 echo_data_mode, UINT32 dw_ratio )

{
	AUD_INFO("%s IN\n", __func__);
}

#else
void delay_ms(UINT32 ms_count)
{
#if 0
	/* can not be used in interrupt (very important)*/
	msleep(ms_count);
#else
	/* cpu may buzy wait */
	mdelay(ms_count);
#endif
}

UINT32 HWREG_R(UINT32 reg_name)
{
	UINT32 rdata = 0, addr = 0;
	UINT32 group = 0, reg = 0;

	reg = reg_name % 100;
	group = (reg_name - reg)/100;

	addr = (REG_BASEADDR);
	addr = addr + group*32*4 + reg*4;
	addr = (unsigned int)ioremap(addr, 4);
	rdata = (*(volatile unsigned int *)(addr));
	SYNCHRONIZE_IO;

	iounmap((volatile unsigned int *)(addr));
	//printk(KERN_INFO "reg read addr :: 0x%x\n", addr);
	return rdata;
}

void HWREG_W(UINT32 reg_name, UINT32 val)
{
	UINT32 addr = 0x0;
	UINT32 group = 0, reg = 0;

	reg = reg_name % 100;
	group = (reg_name - reg)/100;

	addr = (REG_BASEADDR);
	addr = addr + group*32*4 + reg*4;
	addr = (unsigned int)ioremap(addr, 4);
	(*(volatile unsigned int *)(addr)) = val;
	//SYNCHRONIZE_IO;
	iounmap((volatile unsigned int *)(addr));
	//xil_printf("reg write 0x%x, val=0x%x\n\r", addr, val);
	return;
}

#endif

