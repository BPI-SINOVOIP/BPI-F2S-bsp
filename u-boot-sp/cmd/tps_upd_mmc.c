/*
 * (C) Copyright 2017 Tibbo Technology
 *
 * Written by: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * based on common/update.c, cmd/usb.c
 */

#include <common.h>

//#if !(defined(CONFIG_FIT) && defined(CONFIG_OF_LIBFDT))
//#error "CONFIG_FIT and CONFIG_OF_LIBFDT are required for auto-update feature"
//#endif
#if !defined(CONFIG_OF_LIBFDT)
#error "CONFIG_OF_LIBFDT are required for auto-update feature"
#endif

#include <command.h>
#include <mmc.h>
#include <fs.h>
#include <malloc.h>

extern ulong load_addr;

static struct mmc * dv_mmc_start( void) {
 struct mmc *mmc;
 if ( !( mmc = find_mmc_device( 0))) {
   printf( "no mmc device at slot %x\n", 0);
   return( NULL);  }
 if ( mmc_init( mmc)) {
   printf( "mmc init failed\n");
   return( NULL);  }
 return( mmc);  }

static inline int s2ull( const char *p, loff_t *num) {
 char *endptr;
 *num = simple_strtoull( p, &endptr, 16);
 return( *p != '\0' && *endptr == '\0');  }

int tps_upd_r_mmc( char *filename, ulong addr) {
 int rv = -1;
 if ( dv_mmc_start() == NULL) return( 1);
 // ( cmdtp, flag, argc, )
 char *argv[ 5], addr_buf[ 100];
 memset( addr_buf, 0, 100);
 sprintf( addr_buf, "%lx", addr);
 argv[ 0] = "tps_upd";
 argv[ 1] = "mmc"; // dev
 argv[ 2] = "0:1"; // partition
 argv[ 3] = addr_buf;
 argv[ 4] = filename;
 rv = do_load( NULL, 0, 5, argv, FS_TYPE_ANY);
 return( rv);  }

// block size in bytes
int dv_get_mmc_part( const uchar *_pn, disk_partition_t *_info) {
 int i, ret = -1;
 struct blk_desc *dev_desc;
 if ( ( ret = blk_get_device_by_str( "mmc", "0", &dev_desc)) < 0) {
   printf( "Can't find device %s:%d\n", "mmc", 0);
   return( ret);   }
 for ( i = 1; i <= MAX_SEARCH_PARTITIONS; i++) {
    if ( ( ret = part_get_info( dev_desc, i, _info))) continue;
    if ( strcasecmp( ( const char *)_info->name, ( const char *)_pn) != 0) continue;
    ret = i;
    break;   }
 if ( ret < 0) {
   printf( "Partitions:\n");
   for ( i = 1; i <= MAX_SEARCH_PARTITIONS; i++) printf( "%d '%s'\n", i, _info->name);
 }
 return( ret);  }

int tps_upd_w_mmc( ulong _u_data, ulong _u_size, const uchar *_to) {
 size_t w_sz = 0;
 int part_num = 0, ret = 0;
 struct mmc *mmc;
 disk_partition_t info;

 printf( "Doing upd from 0x%lX size 0x%lX at '%s' partition\n", _u_data, _u_size, _to);

 if ( ( part_num = dv_get_mmc_part( _to, &info)) < 0) {
   printf( "Can't get information about '%s' MMC partition\n", _to);
   return( -1);  }

 mmc = find_mmc_device( 0);
 if ( mmc_getwp( mmc) == 1) {
   printf( "MMC 0 is write-protected\n");
   return( -1);  }
  if ( _u_size > info.size * info.blksz) {
   printf( "Size %lX exceeds partition or device limit (%lX)\n", _u_size, info.size * info.blksz);
   return( -1);  }

 lbaint_t x = _u_size / info.blksz;
 printf( "'%s' at 0x%lX,sz:0x%lX blocks to write:%ld (%ld B/block)\n", info.name, info.start, info.size * info.blksz, x, info.blksz);
 w_sz = blk_dwrite( mmc_get_blk_desc( mmc), info.start, x, ( void *)_u_data);
 printf( "Done upd. Wrote:0x%lX /ret:%d\n", w_sz * info.blksz, ret);
// tps_leds_set( 2);
 return( ret);  }
