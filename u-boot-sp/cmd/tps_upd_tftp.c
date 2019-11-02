/*
 * (C) Copyright 2016 Tibbo Technology
 *
 * Written by: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * based on common/update.c
 */

#include <common.h>

//#if !(defined(CONFIG_FIT) && defined(CONFIG_OF_LIBFDT))
//#error "CONFIG_FIT and CONFIG_OF_LIBFDT are required for auto-update feature"
//#endif

#if !defined(CONFIG_OF_LIBFDT)
#error "CONFIG_OF_LIBFDT are required for auto-update feature"
#endif

#include <command.h>
#include <net.h>
#include <malloc.h>

#define ENV_FITUPD_NR "netretry"

#ifndef CONFIG_UPDATE_TFTP_MSEC_MAX
#define CONFIG_UPDATE_TFTP_MSEC_MAX	100
#endif

#ifndef CONFIG_UPDATE_TFTP_CNT_MAX
#define CONFIG_UPDATE_TFTP_CNT_MAX	0
#endif

extern ulong load_addr;

int tps_upd_r_tftp( char *filename, ulong addr) {
 int size, rv;
// ulong saved_timeout_msecs;
// int saved_timeout_count;
 char *saved_netretry, *saved_bootfile;
 char *saved_serverip = NULL;

 rv = 0;
 saved_netretry = strdup(env_get(ENV_FITUPD_NR));
#if 0
 saved_bootfile = strdup(BootFile);
#else
 saved_bootfile = strdup(net_boot_file_name);
#endif

 /* we don't want to retry the connection if errors occur */
 env_set( ENV_FITUPD_NR, "no");

 /* download the update file */
 load_addr = addr;
#if 0
 copy_filename(BootFile, filename, sizeof(BootFile));
#else
 copy_filename(net_boot_file_name, filename, sizeof(net_boot_file_name));
#endif
 saved_serverip = strdup( env_get( "serverip"));
// printf( "SERVERIP now:%s\n", saved_serverip);
#if 0
 size = NetLoop(TFTPGET);
#else
 size = net_loop(TFTPGET);
#endif
 env_set( "serverip", saved_serverip);

 if ( size < 0) rv = 1;
 else if ( size > 0) flush_cache(addr, size);

 env_set( ENV_FITUPD_NR, saved_netretry);
 if ( saved_netretry != NULL) free(saved_netretry);
 if ( saved_bootfile != NULL) {
#if 0
   copy_filename(BootFile, saved_bootfile, sizeof(BootFile));
#else
   copy_filename(net_boot_file_name, saved_bootfile, sizeof(net_boot_file_name));
#endif
   free(saved_bootfile);
 }
 return( rv);  }
