/*
 * (C) Copyright 2016 Tibbo Technology
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
#include <usb.h>
#include <fs.h>
#include <malloc.h>

extern ulong load_addr;

static int dv_usb_start( void) {
 int ret = -1;
 extern char usb_started;
 if ( !usb_started && ( ret = usb_init()) < 0) return( ret);
 return( ret);  }

int tps_upd_r_usb( char *filename, ulong addr) {
 int rv = 0;

 if ( dv_usb_start() < 0) {
   printf( "Can't find USB storage or ethernet device\n");
   return( 1);  }

 // ( cmdtp, flag, argc, )
 char *argv[ 5], addr_buf[ 100];
 memset( addr_buf, 0, 100);
 memset( argv, 0, sizeof( char *)*5);
 sprintf( addr_buf, "%lx", addr);
 argv[ 0] = "tps_upd";
 argv[ 1] = "usb"; // dev
 argv[ 2] = "0"; // partition
 argv[ 3] = addr_buf;
 argv[ 4] = filename;
 rv = do_load( NULL, 0, 5, argv, FS_TYPE_ANY);
 return( rv);  }
