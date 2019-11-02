/*
 * (C) Copyright 2016 Tibbo Technology
 *
 * Written by: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * based on common/update.c
 */

#include <common.h>

#include <malloc.h>
#include <linux/mtd/mtd.h>
#include <nand.h>
#include <jffs2/jffs2.h>

static int set_dev( int _dev) {
 struct mtd_info *mtd = get_nand_dev_by_index( _dev);
 if (  !mtd) {
   printf( "No such device\n");
   return( -1);  }
 if ( nand_curr_device == _dev) return( 0);
 printf( "Device %d: %s", _dev, mtd->name);
 printf( "... is now current device\n");
 nand_curr_device = _dev;
#ifdef CONFIG_SYS_NAND_SELECT_DEVICE
 board_nand_select_device( mtd->priv, _dev);
#endif
 return( 0);  }

static inline int s2ull( const char *p, loff_t *num) {
 char *endptr;
 *num = simple_strtoull( p, &endptr, 16);
 return( *p != '\0' && *endptr == '\0');  }

static inline int s2ul( const char *p, ulong *num) {
 char *endptr;
 *num = simple_strtoul( p, &endptr, 16);
 return( *p != '\0' && *endptr == '\0');  }

int dv_get_nand_part( const u_char *_pn, int *_nand_devn, loff_t *_off, loff_t *_size,
		loff_t *_maxsize, unsigned char *_part_num) {
#ifdef CONFIG_CMD_MTDPARTS
 struct mtd_device *dev;
 struct part_info *part;
 u8 pnum;
 int ret;

 ret = mtdparts_init();
 if ( ret) return( ret);

 ret = find_dev_and_part( ( char *)_pn, &dev, &pnum, &part);
 if ( ret) return( ret);
 if ( dev->id->type != MTD_DEV_TYPE_NAND) {
   printf( "not a NAND device\n");  return( -1);  }

 *_off = part->offset;
 *_size = part->size;
 *_maxsize = part->size;
 *_nand_devn = dev->id->num;
 *_part_num = pnum;

//printf( " %s off:%llX\n", __FUNCTION__, *_off);
 ret = set_dev( *_nand_devn);
 if ( ret) return( ret);
#else
 printf( "offset '%s' is not a number\n", _pn);
 return( -1);
#endif
 return( 0);  }


// _to is a "mtd partition name" or "0xffset"
int tps_upd_w_nand( ulong _u_data, ulong _u_size, const u_char *_to) {
 int n_devn = 0;
 struct mtd_info *nand;
 nand_erase_options_t opts;
 loff_t n_off = 0, n_sz = 0, n_maxsz = 0;
 size_t w_sz = 0;
 unsigned char part_num = 0;
 int ret = 0;

 printf( "Doing upd from 0x%lX size 0x%lX at %s\n", _u_data, _u_size, _to);

 if ( _to[ 0] == '0' && _to[ 1] == 'x' && !s2ull( ( const char *)_to, &n_off)) {
   printf( "Can't get information about '%s' NAND offset\n", _to);
   return( -1);
 } else if ( dv_get_nand_part( _to, &n_devn, &n_off, &n_sz, &n_maxsz, &part_num) != 0
 ) {
   printf( "Can't get information about '%s' NAND partition\n", _to);
   return( -1);  }
//printf( " %s off:%llX n_maxsz:%llX\n", __FUNCTION__, n_off, n_maxsz);
 nand = get_nand_dev_by_index( n_devn);
 if ( n_maxsz == 0) n_maxsz = nand->size - n_off;
//printf( " %s off:%llX n_maxsz:%llX\n", __FUNCTION__, n_off, n_maxsz);
 // check only for absolute addresses
 if ( n_sz == 0 && n_off > n_maxsz) {
   printf( "Offset (%llX) exceeds device limit(%llX)\n", n_off, n_maxsz);
   return( -1);  }
 if ( _u_size > n_maxsz) {
   printf( "Size %lX exceeds partition or device limit (%llX)\n", _u_size, n_maxsz);
   return( -1);  }

// FIXME
// tps_leds_set( part_num);
 printf( "Erase off:%llX len:%llX\n", n_off, n_maxsz);
 // do it optionally
 memset( &opts, 0, sizeof( opts));
 opts.offset = n_off;
 opts.length = n_maxsz;		// or w_sz ? - depends on
// opts.spread = 1;	// 0 == do include bad blocks into erase size
// opts.quiet = 1;
 opts.lim = n_maxsz;
 if ( ( ret = nand_erase_opts( nand, &opts))) return( ret);
 // do it optionally /

 w_sz = _u_size;
 // n_off - abs nand offset to start
 // w_sz - data size to write
 // success == 0
 ret = nand_write_skip_bad( nand, n_off, &w_sz, NULL, n_maxsz, ( u_char *)_u_data, WITH_WR_VERIFY);
 printf( "Done upd offset:0x%llX lim:0x%llX, actually wrote:0x%X /ret:%d\n", n_off, n_maxsz, w_sz, ret);
// FIXME
// tps_leds_set( 2);
 return( ret);  }
