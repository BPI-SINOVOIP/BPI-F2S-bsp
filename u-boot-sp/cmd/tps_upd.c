/*
 * (C) Copyright 2016
 * Dvorkin Dmitry, Tibbo Technology, dvorkin@tibbo.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <usb.h>
#include <tps_upd.h>
#include <linux/libfdt_env.h>

#if !defined(CONFIG_TPS_UPD_TFTP)
#error "CONFIG_TPS_UPD_TFTP required Tibbo fitupd"
#endif

//#if !defined(CONFIG_FIT)
//#error "CONFIG_FIT is required for Tibbo fitupd"
//#endif

#if !defined(CONFIG_OF_LIBFDT)
#error "CONFIG_OF_LIBFDT is required for Tibbo fitupd"
#endif

/* ITB data block attribute */
#define TIB_FITUPD_TO "tps-upd-to"
/* type for source */
#define ENV_FITUPD_P "tps_upd_p"
/* filename to load by default */
#define ENV_FITUPD_F "tps_upd_f"
#define ENV_FITUPD_A "loadaddr"

static int tps_upd_getparams( const void *fit, int noffset, 
    ulong *_data, ulong *_size, u_char **_to) {
 const void *data;
 u_char *to;
 to = ( u_char *)fdt_getprop( fit, noffset, TIB_FITUPD_TO, NULL);
 if ( to == NULL) {
   printf( "'" TIB_FITUPD_TO "' attribute is not defined in header\n");
   return( 1);  }
 if ( strlen( ( const char *)to) < 3) {
   printf( "'" TIB_FITUPD_TO "'='%s' looks wrong\n", to);
   return( 1);  }
 printf( " '" TIB_FITUPD_TO "'='%s'\n", to);
// if ( fit_image_get_load( fit, noffset, ( ulong *)_toaddr)) return( 1);
 if ( fit_image_get_data( fit, noffset, &data, ( size_t *)_size)) return( 1);
 *_data = ( ulong)data;
 *_to = ( u_char *)strdup( ( const char *)to);
 return( 0);  }

static int tps_upd_mem( ulong addr, int ( *_fp)( ulong, ulong, const u_char *)) {
 int images_noffset, ndepth, noffset;
 ulong u_data, u_size;
 u_char *u_to;
 int ret = 0;
 void *fit = ( void *)addr;

 if ( ( ret = fdt_check_header( ( void *)fit))) {
   printf( "Bad FDT header: ");
   if ( ret == -FDT_ERR_BADMAGIC) printf( "wrong magic");
   if ( ret == -FDT_ERR_BADVERSION) printf( "wrong version");
   if ( ret == -FDT_ERR_BADSTATE) printf( "wrong state");
   printf( ", aborting\n");
   return( 1);  }
 if ( !fit_check_format( ( void *)fit)) {
   printf( "Bad FIT format of the update file, aborting\n");
   return( 1);  }

 /* process updates */
 images_noffset = fdt_path_offset( fit, FIT_IMAGES_PATH);

 ndepth = 0;
 noffset = fdt_next_node( fit, images_noffset, &ndepth);
 while ( noffset >= 0 && ndepth > 0) {
   if ( ndepth != 1) goto next_node;
   printf( "Processing update '%s' :", fit_get_name( fit, noffset, NULL));
   if ( !fit_image_verify( fit, noffset)) {
     printf( "Error: invalid update hash, aborting\n");
     ret = 1;
     goto next_node;  }
   printf( "\n");
   u_to = ( u_char *)"";
   if ( tps_upd_getparams( fit, noffset, &u_data, &u_size, &u_to)) {
     printf( "Error: can't get update parameters, aborting\n");
     ret = 1;
     goto next_node;  }
#if defined(CONFIG_LCD)
   lcd_printf( "%s (%d bytes)...\n", u_to, u_size);
#endif
   // u_data - got a pointer from FIT to data block
   // u_size - got a data block size from FIT
   // u_to is the name or partition or absolute offset on device
   if ( _fp( u_data, u_size, u_to) != 0) {
     printf( "Error: can't flash update, aborting\n");
#if defined(CONFIG_LCD)
     lcd_printf( "ERR");
#endif
     ret = 1;
     goto next_node;  }
next_node:
   noffset = fdt_next_node( fit, noffset, &ndepth);
 }
 return( ret);  }


static int do_tps_upd( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
 ulong addr = 0UL;
 char *filename = "";
 int ( *cbfp)( ulong, ulong, const u_char *);

 if ( argc > 2) return CMD_RET_USAGE;
 // no argiment - try $loadaddr...
 if ( argc == 1) {
   addr = simple_strtoul( env_get( ENV_FITUPD_A), NULL, 16);
   printf( "Trying 'loadaddr' 0x%08lx for source (0)\n", addr);
 }
 while ( argc >= 2) {
   // first argiment may be 0x...
   if ( argv[ 1][ 0] == '0' && argv[ 1][ 1] == 'x') {
     addr = simple_strtoul( argv[ 1] + 2, NULL, 16);
     printf( "Forced MEM source 0x%08lx for update\n", addr);
     break;  }
   if ( strncmp( argv[ 1], "mem:", 4) == 0) {
     addr = simple_strtoul( argv[ 1] + 4, NULL, 16);
     printf( "Forced MEM source 0x%08lx for update\n", addr);
     break;  }
   addr = simple_strtoul( env_get( ENV_FITUPD_A), NULL, 16);
   printf( "Trying 'loadaddr' 0x%08lx for source (1)\n", addr);
   if ( strncmp( argv[ 1], "usb", 3) == 0) {
     if ( argv[ 1][ 3] == ':') filename = strdup( argv[ 1] + 4);
     if ( !filename) filename = "";
     if ( strlen( filename) < 1) filename = env_get( ENV_FITUPD_F);
     if ( !filename) filename = "";
     if ( strlen( filename) < 1) {
       printf( "No source option, no env variable '" ENV_FITUPD_F "' defined\n");
       return( 1);  }
#if defined(CONFIG_LCD)
     lcd_printf( "USB update: %s\n", filename);
#endif
     printf( "Forced USB source '%s' for update\n", filename);
     if ( tps_upd_r_usb( filename, addr)) {
       printf( "Can't load update file, aborting auto-update\n");
#if defined(CONFIG_LCD)
       lcd_printf( " Read Error\n");
#endif
#ifdef CONFIG_SHOW_ACTIVITY
       tps_leds_set( 0x2);
#endif
       return( 1);
     }
     break;  }
#if defined(CONFIG_MMC)
   if ( strncmp( argv[ 1], "mmc", 3) == 0) {
     if ( argv[ 1][ 3] == ':') filename = strdup( argv[ 1] + 4);
     if ( !filename) filename = "";
     if ( strlen( filename) < 1) filename = env_get( ENV_FITUPD_F);
     if ( !filename) filename = "";
     if ( strlen( filename) < 1) {
       printf( "No source option, no env variable '" ENV_FITUPD_F "' defined\n");
       return( 1);  }
#if defined(CONFIG_LCD)
     lcd_printf( "MMC update: %s\n", filename);
#endif
     printf( "Forced MMC source '%s' for update\n", filename);
     if ( tps_upd_r_mmc( filename, addr)) {
       printf( "Can't load update file, aborting auto-update\n");
#if defined(CONFIG_LCD)
       lcd_printf( " Read Error\n");
#endif
#ifdef CONFIG_SHOW_ACTIVITY
       tps_leds_set( 0x2);
#endif
       return( 1);
     }
     break;  }
#endif
   if ( strncmp( argv[ 1], "tftp", 4) == 0) {
     if ( argv[ 1][ 4] == ':') filename = strdup( argv[ 1] + 5);
     if ( !filename) filename = "";
     if ( strlen( filename) < 1) filename = env_get( ENV_FITUPD_F);
     if ( !filename) filename = "";
     if ( strlen( filename) < 1) {
       printf( "No source option, no env variable '" ENV_FITUPD_F "' defined\n");
       return( 1);  }
     printf( "Forced TFTP source '%s' for update\n", filename);
     if ( tps_upd_r_tftp( filename, addr)) {
       printf( "Can't load update file, aborting auto-update\n");
#ifdef CONFIG_SHOW_ACTIVITY
       tps_leds_set( 0x2);
#endif
       return( 1);
     }
     break;  }
   printf( "Unknown source type '%s' for update\n", argv[ 1]);
   return( 1);
   break;  }
#ifdef CONFIG_SHOW_ACTIVITY
 tps_leds_set( 0x2);
#endif
 if ( addr == 0UL) {
   printf( "Addr 0x%08lx cannot be used as the source for update\n", addr);
   return( 1);  }
 cbfp = NULL;
#ifdef CONFIG_NAND
 if ( cbfp == NULL) cbfp = &tps_upd_w_nand;
#endif
#ifdef CONFIG_MMC
 if ( cbfp == NULL) cbfp = &tps_upd_w_mmc;
#endif
 while ( argc >= 3) {
#ifdef CONFIG_NAND
   if ( strcmp( argv[ 3], "nand") == 0) cbfp = &tps_upd_w_nand;
#endif
#ifdef CONFIG_MMC
   if ( strcmp( argv[ 3], "mmc") == 0) cbfp = &tps_upd_w_mmc;
#endif
#ifdef CONFIG_CMD_FLASH
   if ( strcmp( argv[ 3], "flash") == 0) cbfp = &tps_upd_w_flash;
#endif
   break;   }
 if ( cbfp == NULL) {
   printf( "No destination device detected\n");
   return( 1);  }
 return( tps_upd_mem( addr, cbfp));  }

U_BOOT_CMD( tps_upd, 2, 0, do_tps_upd,
	"update from FIT image",
	"[0xAddr|source] [devto]\n"
	"- run update of [devto] from FIT image got from memory [0xAddr] or [source]\n"
	"  [source] is an 0xAddr or one of\n"
	"\t'mem','mem:0xAddr'\n"
	"\t'tftp','tftp:filename'\n"
	"\t'usb','usb:filename'\n"
#ifdef CONFIG_MMC
	"\t'mmc','mmc:filename'\n"
#endif
	"  [devto] is the destination device for updates. One of:\n"
#ifdef CONFIG_NAND
	"\t'nand'\n"
#endif
#ifdef CONFIG_CMD_FLASH
	"\t'flash'\n"
#endif
	" ENV variables used:\n"
	"  '" ENV_FITUPD_A "' as the memory address source if no others defined\n"
	"  '" ENV_FITUPD_P "' as the source device\n"
	"  '" ENV_FITUPD_F "' as the filename to load from source\n"
	" Attribute '" TIB_FITUPD_TO "' have to be defined in update file\n"
	"   for each data block\n"
);
