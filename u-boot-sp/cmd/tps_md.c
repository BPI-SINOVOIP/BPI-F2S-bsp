/*
 * (C) Copyright 2016
 * Dvorkin Dmitry, Tibbo Technology, dvorkin@tibbo.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>

#include <asm/arch/gpio.h>

#ifdef CONFIG_CMD_TPS_MD
extern unsigned int tibbo_md_val;
#else
unsigned int tibbo_md_val;
#endif

__weak void tps_leds_set( unsigned char _mask) {  return;  }
__weak int tps_md_value( void) {  return( 0);  }

static inline int s2ul( const char *p, ulong *num) {
 char *endptr;
 *num = simple_strtoul( p, &endptr, 16);
 return( *p != '\0' && *endptr == '\0');  }

static int do_tps_led( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
 ulong val = 0;
 if ( argc > 1) s2ul( argv[ 1], &val);
 tps_leds_set( val);
 return( val);  }

static int do_tps_md( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
 int val = 0, cur = 1, verb = 0;
 val = tps_md_value();
 if ( argv[ argc - 1][ 0] == 'v') verb = 1;
 if ( argc > 1 && argv[ argc - 2][ 0] == 's') {
   val = tibbo_md_val;  cur = 0;
 }
 if ( verb) printf( "%s MD value:%d\n", ( cur ? "current" : "saved"), val);
 return( val);  }

U_BOOT_CMD( tps_md, 3, 0, do_tps_md,
	"get MD button status",
	"[current|saved] [verbose]\n"
	"- get MD button status\n"
);
U_BOOT_CMD( tps_led, 2, 0, do_tps_led,
	"set TPS LED status",
	"0x0\n"
	"- set TPS LED status (bits)\n"
);
