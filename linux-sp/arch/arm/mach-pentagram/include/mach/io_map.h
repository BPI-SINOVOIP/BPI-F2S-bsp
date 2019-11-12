#ifndef __MACH_IO_MAP_H
#define __MACH_IO_MAP_H

#include <asm/sizes.h>

#if defined(CONFIG_MACH_PENTAGRAM_8388_ACHIP) || defined(CONFIG_MACH_PENTAGRAM_8388_BCHIP) || \
    defined(CONFIG_MACH_PENTAGRAM_3502_ACHIP)
#include "io_map_8388.h"
#endif

#if defined(CONFIG_MACH_PENTAGRAM_I136_ACHIP) || defined(CONFIG_MACH_PENTAGRAM_I137_BCHIP)
#include "io_map_i137.h"
#endif

#if defined(CONFIG_MACH_PENTAGRAM_SP7021_ACHIP) || defined(CONFIG_MACH_PENTAGRAM_SP7021_BCHIP)
#include "io_map_sp7021.h"
#endif

#endif /* __MACH_IO_MAP_H */
