#ifndef __MACH_CLK_H
#define __MACH_CLK_H

#if defined(CONFIG_MACH_PENTAGRAM_I136_ACHIP) || defined(CONFIG_MACH_PENTAGRAM_I137_BCHIP)
#define CLK_A_PLLCLK            2000000000      /* 2GHz */
#define CLK_B_PLLSYS            202500000       /* 202.5MHz */
#endif

#if defined(CONFIG_MACH_PENTAGRAM_SP7021_ACHIP) || defined(CONFIG_MACH_PENTAGRAM_SP7021_BCHIP)
#define CLK_B_PLLSYS            202500000       /* 202.5MHz */
#endif

#endif /* __MACH_CLK_H */
