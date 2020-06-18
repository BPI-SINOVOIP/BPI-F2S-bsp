.globl lowlevel_init

lowlevel_init:
	/* do nothing */
	mov     pc, lr


#ifdef	CONFIG_OF_PRIOR_STAGE
.global save_boot_params
.section .data
.align 16
.global prior_stage_fdt_address
prior_stage_fdt_address:
	.word 0x0
.section .text	
save_boot_params:
	ldr r1,=prior_stage_fdt_address
	str r0,[r1]   /* save dtb addr to prior_stage_fdt_address*/
	/* Returns */
	b	save_boot_params_ret

#endif	
