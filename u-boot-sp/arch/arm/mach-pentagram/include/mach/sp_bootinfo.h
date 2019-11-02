#ifndef __SP_BOOTINFO_H
#define __SP_BOOTINFO_H

#ifdef CONFIG_SYS_ENV_8388
#include <asm/arch/sp_bootinfo_8388.h>
#else /* CONFIG_SYS_ENV_SP7021_EVB (and CONFIG_SYS_ENV_ZEBU) */
#include <asm/arch/sp_bootinfo_sc7xxx.h>
#endif

#endif /* __SP_BOOTINFO_H */
