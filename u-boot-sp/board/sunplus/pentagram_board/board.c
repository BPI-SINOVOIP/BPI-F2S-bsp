/*
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/arch/sp7021_common.h>

#ifdef CONFIG_SP_SPINAND
extern void board_spinand_init(void);
#endif

#define Q628_REG_BASE 				(0x9c000000)
#define Q628_RF_GRP(_grp, _reg)		((((_grp)*32+(_reg))*4)+Q628_REG_BASE)
#define Q628_RF_MASK_V_CLR(_mask)	(((_mask)<<16)| 0)

#define A_REG_BASE 					(0x9ec00000)
#define A_RF_GRP(_grp, _reg)		((((_grp)*32+(_reg))*4)+A_REG_BASE)

typedef volatile u32 * reg_ptr;

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
#ifdef A_SYS_COUNTER
	*(reg_ptr)A_SYS_COUNTER = 3; // enable A_SYS_COUNTER
#endif
#ifdef CONFIG_CPU_V7_HAS_NONSEC
	/* set Achip RGST,AMBA to NonSec state */
	for (int i = 502; i < 504; i++)
		for (int j = 0; j < 32; j++)
			*(reg_ptr)A_RF_GRP(i, j) = 0;
#endif

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

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_DM_VIDEO
	sp7021_video_show_board_info();
#endif
	return 0;
}
#endif

#ifdef CONFIG_ARMV7_NONSEC
//void smp_kick_all_cpus(void) {}
void smp_set_core_boot_addr(unsigned long addr, int corenr)
{
	reg_ptr cpu_boot_regs = (void *)(CONFIG_SMP_PEN_ADDR - 12);

	/* wakeup core 1~3 */
	cpu_boot_regs[0] = addr;
	cpu_boot_regs[1] = addr;
	cpu_boot_regs[2] = addr;

	__asm__ __volatile__ ("dsb ishst; sev");
}
#endif
