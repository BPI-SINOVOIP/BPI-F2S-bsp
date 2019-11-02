/*
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>

#ifdef CONFIG_SP_SPINAND
extern void board_spinand_init(void);
#endif

#define Q628_REG_BASE 				(0x9c000000)
#define Q628_RF_GRP(_grp, _reg)		((((_grp)*32+(_reg))*4)+Q628_REG_BASE)
#define Q628_RF_MASK_V_CLR(_mask)	(((_mask)<<16)| 0)

struct Q628_moon0_regs{
	unsigned int stamp;
	unsigned int clken[10];
	unsigned int gclken[10];
	unsigned int reset[10];
	unsigned int hw_cfg;
};
#define Q628_MOON0_REG ((volatile struct Q628_moon0_regs *)Q628_RF_GRP(0,0))

struct Q628_moon1_regs{
	unsigned int sft_cfg[32];
};
#define Q628_MOON1_REG ((volatile struct Q628_moon1_regs *)Q628_RF_GRP(1,0))

#define Q628_MOON4_REG ((volatile struct Q628_moon1_regs *)Q628_RF_GRP(4,0))

struct Q628_pad_ctl_regs {
	unsigned int reserved[20];
	unsigned int spi_flash_sftpad_ctl;
	unsigned int spi_nd_sftpad_ctl;
};
#define Q628_PAD_CTL_REG ((volatile struct Q628_pad_ctl_regs *)Q628_RF_GRP(101,0))

enum Device_table{
	DEVICE_SPI_NAND = 0,
	DEVICE_MAX
};

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

int board_eth_init(bd_t *bis)
{
	return 0;
}

int misc_init_r(void)
{
	return 0;
}

void SetBootDev(unsigned int bootdev, unsigned int pin_x)
{
	switch(bootdev)
	{
#ifdef CONFIG_SP_SPINAND
		case DEVICE_SPI_NAND:
			/* module release reset pin */
			Q628_MOON0_REG->reset[8]=Q628_RF_MASK_V_CLR(1<<10);//spi nand 
			Q628_MOON0_REG->reset[4]=Q628_RF_MASK_V_CLR(1<<4);//bch
			/* pinmux set */
			Q628_MOON1_REG->sft_cfg[1] |= (0x00100010);
			/* nand pll level set */
			Q628_MOON4_REG->sft_cfg[27] |= (0x00040004);
			break;
#endif			
		default:
			printf("unknowm \n");
			break;
	}
}

void board_nand_init(void)
{
#ifdef CONFIG_SP_SPINAND
	SetBootDev(DEVICE_SPI_NAND,1);

	/* config soft pad */
	#ifdef CONFIG_GLB_GMNCFG_SPINAND_ENABLE_SFTPAD
	Q628_PAD_CTL_REG->spi_nd_sftpad_ctl = CONFIG_GLB_GMNCFG_SPINAND_SFTPAD_CTL;
	printf("spi_nd_sftpad_ctl:0x%08x\n", Q628_PAD_CTL_REG->spi_nd_sftpad_ctl);
	#endif

	board_spinand_init();
#endif
}

