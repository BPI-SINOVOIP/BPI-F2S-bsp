#ifndef __MACH_IO_MAP_H
#define __MACH_IO_MAP_H

#if defined(CONFIG_MACH_PENTAGRAM_8388_ACHIP) || defined(CONFIG_MACH_PENTAGRAM_8388_BCHIP) || \
    defined(CONFIG_MACH_PENTAGRAM_3502_ACHIP)
#include "io_map_8388.h"
#endif

#if defined(CONFIG_MACH_PENTAGRAM_I136_ACHIP) || defined(CONFIG_MACH_PENTAGRAM_I137_BCHIP)
#include "io_map_i137.h"
#endif

#if defined(CONFIG_SOC_SP7021) || defined(CONFIG_SOC_I143)
#include "io_map_sp7021.h"
#endif

#endif /* __MACH_IO_MAP_H */
