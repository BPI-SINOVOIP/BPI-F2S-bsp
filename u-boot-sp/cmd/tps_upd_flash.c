/*
 * (C) Copyright 2016 Tibbo Technology
 *
 * Written by: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * based on common/update.c
 */

#include <common.h>

#if defined(CONFIG_SYS_NO_FLASH)
#error "CONFIG_SYS_NO_FLASH defined"
#endif

#include <flash.h>
#include <malloc.h>
#include <linux/mtd/mtd.h>

static inline int s2ul( const char *p, ulong *num) {
 char *endptr;
 *num = simple_strtoul( p, &endptr, 16);
 return( *p != '\0' && *endptr == '\0');  }

static int update_flash_protect(int prot, ulong addr_first, ulong addr_last) {
 uchar *sp_info_ptr;
 ulong s;
 int i, bank, cnt;
 flash_info_t *info;
 sp_info_ptr = NULL;

 if ( prot == 0) {
	saved_prot_info =
		calloc(CONFIG_SYS_MAX_FLASH_BANKS * CONFIG_SYS_MAX_FLASH_SECT, 1);
	if (!saved_prot_info)
		return 1;
 }

 for (bank = 0; bank < CONFIG_SYS_MAX_FLASH_BANKS; ++bank) {
	cnt = 0;
	info = &flash_info[bank];
/* Nothing to do if the bank doesn't exist */
	if ( info->sector_count == 0) return( 0);
	/* Point to current bank protection information */
	sp_info_ptr = saved_prot_info + (bank * CONFIG_SYS_MAX_FLASH_SECT);
	/*
	 * Adjust addr_first or addr_last if we are on bank boundary.
	 * Address space between banks must be continuous for other
	 * flash functions (like flash_sect_erase or flash_write) to
	 * succeed. Banks must also be numbered in correct order,
	 * according to increasing addresses.
	 */
	if (addr_last > info->start[0] + info->size - 1)
		addr_last = info->start[0] + info->size - 1;
	if (addr_first < info->start[0])
		addr_first = info->start[0];
	for (i = 0; i < info->sector_count; i++) {
		/* Save current information about protected sectors */
		if (prot == 0) {
			s = info->start[i];
			if ((s >= addr_first) && (s <= addr_last))
				sp_info_ptr[i] = info->protect[i];
		}
		/* Protect/unprotect sectors */
		if (sp_info_ptr[i] == 1) {
#if defined(CONFIG_SYS_FLASH_PROTECTION)
			if (flash_real_protect(info, i, prot))
				return 1;
#else
			info->protect[i] = prot;
#endif
			cnt++;
		}
	}
	if (cnt) {
		printf("%sProtected %d sectors\n",
					prot ? "": "Un-", cnt);
	}
 }
 if ( ( prot == 1) && saved_prot_info) free( saved_prot_info);
 return( 0);  }

int tps_upd_w_flash( ulong addr_source, ulong size, const u_char *_to) {
 ulong addr_last;
 ulong addr_first;
 if ( !s2ul( _to, &addr_first)) {
   printf( "Bad flash address '%s'\n", _to);
   return( 1);  }
 addr_last = addr_first + size - 1;

 /* round last address to the sector boundary */
 if ( flash_sect_roundb( &addr_last) > 0) return( 1);
 if ( addr_first >= addr_last) {
   printf( "Error: end address exceeds addressing space\n");
   return( 1);  }
 /* remove protection on processed sectors */
 if ( update_flash_protect( 0, addr_first, addr_last) > 0) {
   printf( "Error: could not unprotect flash sectors\n");
   return( 1);  }
 printf( "Erasing 0x%08lx - 0x%08lx", addr_first, addr_last);
 if ( flash_sect_erase( addr_first, addr_last) > 0) {
   printf( "Error: could not erase flash\n");
   return( 1);  }
 printf( "Copying to flash...");
 if (flash_write( (char *)addr_source, addr_first, size) > 0) {
   printf( "Error: could not copy to flash\n");
   return( 1);  }
 printf("done\n");
 /* enable protection on processed sectors */
 if ( update_flash_protect( 1, addr_first, addr_last) > 0) {
   printf( "Error: could not protect flash sectors\n");
   return( 1);  }
 return( 0);  }
