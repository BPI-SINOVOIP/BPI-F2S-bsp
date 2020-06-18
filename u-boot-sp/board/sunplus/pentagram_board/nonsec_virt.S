/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * smp_waitloop code for board/sunplus/pentagram_board
 *
 * Copyright (c) 2020 Sunplus
 */

#include <config.h>
#include <linux/linkage.h>

.arch_extension sec
.arch_extension virt

	.pushsection ._secure.text, "ax"

#ifdef CONFIG_SMP_PEN_ADDR
ENTRY(smp_waitloop)
	mrc	p15, 0, r1, c0, c0, 5
	ands	r1, r1, #3
	ldr	r2, =CONFIG_SMP_PEN_ADDR	@ load start address
	sub	r2, r2, r1, LSL #2	@ A_START_POS_A_VIEW - (core# * 4)
1:
	wfe
	ldr	r1, [r2]
#ifdef CONFIG_PEN_ADDR_BIG_ENDIAN
	rev	r1, r1
#endif
	cmp	r0, r1			@ make sure we dont execute this code
	beq	1b				@ again (due to a spurious wakeup)
	mov	r0, r1
	b	_do_nonsec_entry
ENDPROC(smp_waitloop)
#endif

	.popsection
