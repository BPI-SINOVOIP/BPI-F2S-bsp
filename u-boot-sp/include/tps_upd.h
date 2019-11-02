/*
 *	Tibbo FIT update interface.
 *
 *	Copyright 2016 Dvorkin Dmitry.
 *
 *	SPDX-License-Identifier:	GPL-2.0
 *
 */

#ifndef __TPS_UPD_H__
#define __TPS_UPD_H__

// tftp read FIT
extern int tps_upd_r_tftp( char *_filename, ulong _addr);
// tftp read USB
extern int tps_upd_r_usb( char *_filename, ulong _addr);
// tftp read MMC
extern int tps_upd_r_mmc( char *_filename, ulong _addr);

// nand write
extern int tps_upd_w_nand( ulong _u_addr, ulong _u_size, const u_char *_to);
// nand write
extern int tps_upd_w_mmc( ulong _u_addr, ulong _u_size, const u_char *_to);
// flash write
extern int tps_upd_w_flash( ulong _u_addr, ulong _u_size, const u_char *_to);

/* used in other Tibbo custom functions */
extern int dv_get_part( const u_char *_pn, int *_nand_devn,
	loff_t *_off, loff_t *_size,
	loff_t *_maxsize, unsigned char *_part_num);


#endif /* __TPS_UPD_H__ */
