/*
 * Sunplus CBDMA test driver
 *
 * Copyright (C) 2018 Sunplus Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/atomic.h>

#include <mach/io_map.h>

struct cbdma_reg {
	volatile unsigned int hw_ver;
	volatile unsigned int config;
	volatile unsigned int length;
	volatile unsigned int src_adr;
	volatile unsigned int des_adr;
	volatile unsigned int int_flag;
	volatile unsigned int int_en;
	volatile unsigned int memset_val;
	volatile unsigned int sdram_size_config;
	volatile unsigned int illegle_record;
	volatile unsigned int sg_idx;
	volatile unsigned int sg_cfg;
	volatile unsigned int sg_length;
	volatile unsigned int sg_src_adr;
	volatile unsigned int sg_des_adr;
	volatile unsigned int sg_memset_val;
	volatile unsigned int sg_en_go;
	volatile unsigned int sg_lp_mode;
	volatile unsigned int sg_lp_sram_start;
	volatile unsigned int sg_lp_sram_size;
	volatile unsigned int sg_chk_mode;
	volatile unsigned int sg_chk_sum;
	volatile unsigned int sg_chk_xor;
	volatile unsigned int rsv_23_31[9];
};

#define QACCEPT_B_ACCEPT_CTL0				(0)
#define QACCEPT_B_NOT_ACCEPT_CTL0		(1<<0)
#define QACTIVE_NOT_ACTIVE_CTL0			(0)
#define QACTIVE_ACTIVE_CTL0				(1<<1)
#define QDENY_NOT_DENY_CTL0				(0)
#define QDENY_DENY_CTL0					(1<<2)
#define QREQ_B_REQ_CTL0					(0)
#define QREQ_B_NOT_REQ_CTL0				(1<<3)
#define QRST_B_RST_CTL0					(0)
#define QRST_B_NOT_RST_CTL0				(1<<4)
#define QCLKEN_DISABLE_CTL0				(0)
#define QCLKEN_ENABLE_CTL0					(1<<5)

#define QREQ_B_MASK_CTL0					(1<<19)
#define QRST_B_MASK_CTL0					(1<<20)
#define QCLKEN_MASK_CTL0					(1<<21)

#define QACCEPT_B_ACCEPT_CTL1				(0)
#define QACCEPT_B_NOT_ACCEPT_CTL1		(1<<8)
#define QACTIVE_NOT_ACTIVE_CTL1			(0)
#define QACTIVE_ACTIVE_CTL1				(1<<9)
#define QDENY_NOT_DENY_CTL1				(0)
#define QDENY_DENY_CTL1					(1<<10)
#define QREQ_B_REQ_CTL1					(0)
#define QREQ_B_NOT_REQ_CTL1				(1<<11)
#define QRST_B_RST_CTL1					(0)
#define QRST_B_NOT_RST_CTL1				(1<<12)
#define QCLKEN_DISABLE_CTL1				(0)
#define QCLKEN_ENABLE_CTL1					(1<<13)

#define QREQ_B_MASK_CTL1					(1<<27)
#define QRST_B_MASK_CTL1					(1<<28)
#define QCLKEN_MASK_CTL1					(1<<29)

#define QCHANNEL_START_CTL0				0x00380038 //(QCLKEN_MASK_CTL0|QRST_B_MASK_CTL0|QREQ_B_MASK_CTL0|QCLKEN_ENABLE_CTL0|QRST_B_NOT_RST_CTL0|QREQ_B_NOT_REQ_CTL0)
#define QCHANNEL_START_CTL1				0x38003800 //(QCLKEN_MASK_CTL1|QRST_B_MASK_CTL1|QREQ_B_MASK_CTL1|QCLKEN_ENABLE_CTL1|QRST_B_NOT_RST_CTL1|QREQ_B_NOT_REQ_CTL1)

#define Q_RNU_CTL0							(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_NOT_DENY_CTL0|QREQ_B_NOT_REQ_CTL0)
#define Q_REQUEST_CTL0						(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_NOT_DENY_CTL0|QREQ_B_REQ_CTL0)
#define Q_STOP_CTL0							(QACCEPT_B_ACCEPT_CTL0|QDENY_NOT_DENY_CTL0|QREQ_B_REQ_CTL0)
#define Q_EXIT_CTL0							(QACCEPT_B_ACCEPT_CTL0|QDENY_NOT_DENY_CTL0|QREQ_B_NOT_REQ_CTL0)
#define Q_DENIED_CTL0						(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_REQ_CTL0)
#define Q_CONTINUE_CTL0					(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0)


#define Q_RNU_CTL1							(QACCEPT_B_NOT_ACCEPT_CTL1|QDENY_NOT_DENY_CTL1|QREQ_B_NOT_REQ_CTL1)
#define Q_REQUEST_CTL1						(QACCEPT_B_NOT_ACCEPT_CTL1|QDENY_NOT_DENY_CTL1|QREQ_B_REQ_CTL1)
#define Q_STOP_CTL1							(QACCEPT_B_ACCEPT_CTL1|QDENY_NOT_DENY_CTL1|QREQ_B_REQ_CTL1)
#define Q_EXIT_CTL1							(QACCEPT_B_ACCEPT_CTL1|QDENY_NOT_DENY_CTL1|QREQ_B_NOT_REQ_CTL1)
#define Q_DENIED_CTL1						(QACCEPT_B_NOT_ACCEPT_CTL1|QDENY_DENY_CTL1|QREQ_B_REQ_CTL1)
#define Q_CONTINUE_CTL1					(QACCEPT_B_NOT_ACCEPT_CTL1|QDENY_DENY_CTL1|QREQ_B_NOT_REQ_CTL1)





/* Unaligned test */
#define UNALIGNED_DROP_S	0	/* 0, 1, 2, 3 */
#define UNALIGNED_DROP_E	0	/* 0, 1, 2, 3 */
#define UNALIGNED_ADDR_S(X)	(X + UNALIGNED_DROP_S)
#define UNALIGNED_ADDR_E(X)	(X - UNALIGNED_DROP_E)


#define NUM_CBDMA		2
#define BUF_SIZE_DRAM		(PAGE_SIZE * 2)

#define PATTERN4TEST(X)		((((u32)(X)) << 24) | (((u32)(X)) << 16) | (((u32)(X)) << 8) | (((u32)(X)) << 0))

#define CBDMA_CONFIG_DEFAULT	0x00030000
#define CBDMA_CONFIG_GO		(0x01 << 8)
#define CBDMA_CONFIG_MEMSET	(0x00 << 0)
#define CBDMA_CONFIG_WR		(0x01 << 0)
#define CBDMA_CONFIG_RD		(0x02 << 0)
#define CBDMA_CONFIG_CP		(0x03 << 0)

#define CBDMA_INT_FLAG_DONE	(1 << 0)

#define CBDMA_SG_CFG_NOT_LAST	(0x00 << 2)
#define CBDMA_SG_CFG_LAST	(0x01 << 2)
#define CBDMA_SG_CFG_MEMSET	(0x00 << 0)
#define CBDMA_SG_CFG_WR		(0x01 << 0)
#define CBDMA_SG_CFG_RD		(0x02 << 0)
#define CBDMA_SG_CFG_CP		(0x03 << 0)
#define CBDMA_SG_EN_GO_EN	(0x01 << 31)
#define CBDMA_SG_EN_GO_GO	(0x01 << 0)
#define CBDMA_SG_LP_MODE_LP	(0x01 << 0)
#define CBDMA_SG_LP_SZ_1KB	(0 << 0)
#define CBDMA_SG_LP_SZ_2KB	(1 << 0)
#define CBDMA_SG_LP_SZ_4KB	(2 << 0)
#define CBDMA_SG_LP_SZ_8KB	(3 << 0)
#define CBDMA_SG_LP_SZ_16KB	(4 << 0)
#define CBDMA_SG_LP_SZ_32KB	(5 << 0)
#define CBDMA_SG_LP_SZ_64KB	(6 << 0)

#define NUM_SG_IDX		32

struct cbdma_sg_lli {
	u32 sg_cfg;
	u32 sg_length;
	u32 sg_src_adr;
	u32 sg_des_adr;
	u32 sg_memset_val;
	u32 sg_lp_mode;
};

#define MIN(X, Y)		((X) < (Y) ? (X): (Y))

struct cbdma_info_s {
	char name[32];
	struct platform_device *pdev;
	char irq_name[32];
	int irq;
	volatile struct cbdma_reg *cbdma_ptr;
	u32 sram_addr;
	u32 sram_size;
	void *buf_va;
	dma_addr_t dma_handle;
};
static struct cbdma_info_s cbdma_info[NUM_CBDMA];

static struct task_struct *thread_ptr = NULL;
atomic_t isr_cnt = ATOMIC_INIT(0);

void dump_data(u8 *ptr, u32 size)
{
	u32 i, addr_begin;
	int length;
	char buffer[256];

	addr_begin = (u32)(ptr);
	for (i = 0; i < size; i++) {
		if ((i & 0x0F) == 0x00) {
			length = sprintf(buffer, "%08x: ", i + addr_begin);
		}
		length += sprintf(&buffer[length], "%02x ", *ptr);
		ptr++;

		if ((i & 0x0F) == 0x0F) {
			printk(KERN_INFO "%s\n", buffer);
		}
	}
	printk(KERN_INFO "\n");
}

static void sp_cbdma_tst_basic(void *data)
{
	int i, j, val;
	u32 *u32_ptr, expected_u32, val_u32, test_size;
	u32 *sram_ptr;

	printk(KERN_INFO "%s(), start\n", __func__);

	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);
			printk(KERN_INFO "MEMSET test\n");
			
			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;
			cbdma_info[i].cbdma_ptr->length = BUF_SIZE_DRAM - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			cbdma_info[i].cbdma_ptr->src_adr = UNALIGNED_ADDR_S((u32)(cbdma_info[i].dma_handle));
			cbdma_info[i].cbdma_ptr->des_adr = UNALIGNED_ADDR_S((u32)(cbdma_info[i].dma_handle));
			cbdma_info[i].cbdma_ptr->memset_val = PATTERN4TEST(i ^ 0xaa);
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_MEMSET;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}
			dump_data(cbdma_info[i].buf_va, 0x20);
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
			dump_data(((u8 *)cbdma_info[i].buf_va + BUF_SIZE_DRAM - 0x20), 0x20);
#endif
			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			expected_u32 = PATTERN4TEST(i ^ 0xaa);
			for (j = 0 ; j < (BUF_SIZE_DRAM >> 2); j++) {
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
				if ((j == 0) || ((j + 1) >= (BUF_SIZE_DRAM >> 2))) {
					u32_ptr++;
					continue;
				}
#endif
				BUG_ON(*u32_ptr != expected_u32);
				u32_ptr++;
			}
			
			printk(KERN_INFO "MEMSET test: OK\n\n");
			printk(KERN_INFO "SRAM r/w test\n");

			sram_ptr = (u32 *)ioremap(cbdma_info[i].sram_addr, cbdma_info[i].sram_size);
			BUG_ON(!sram_ptr);
			u32_ptr = sram_ptr;
			val_u32 = (u32)(cbdma_info[i].sram_addr);
			test_size = cbdma_info[i].sram_size - 256; /* last 256 bytes is reserved for COPY, access it will trigger a ISR */
			for (j = 0 ; j < (test_size >> 2); j++) {
				*u32_ptr = val_u32;
				u32_ptr++;
				val_u32 += 4;
			}
			dump_data((u8 *)sram_ptr, 0x20);
			u32_ptr = sram_ptr;
			val_u32 = (u32)(cbdma_info[i].sram_addr);
			for (j = 0 ; j < (test_size >> 2); j++) {
				BUG_ON(*u32_ptr != val_u32);
				u32_ptr++;
				val_u32 += 4;
			}

			printk(KERN_INFO "SRAM r/w test: OK\n\n");
			printk(KERN_INFO "R/W test\n");
			
			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			val_u32 = (u32)(u32_ptr);
			test_size = MIN(cbdma_info[i].sram_size, BUF_SIZE_DRAM) >> 1;
			for (j = 0 ; j < (test_size >> 2); j++) {
				/* Fill (test_size) bytes of data to DRAM */
				*u32_ptr = val_u32;
				u32_ptr++;
				val_u32 += 4;
			}
			dump_data(cbdma_info[i].buf_va, 0x20);

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;
			cbdma_info[i].cbdma_ptr->length = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			cbdma_info[i].cbdma_ptr->des_adr = 0;
			cbdma_info[i].cbdma_ptr->src_adr = UNALIGNED_ADDR_S((u32)(cbdma_info[i].dma_handle));
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_RD;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}
				break;
			}

			dump_data((u8 *)(sram_ptr), 0x20);
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) == 0)
			u32_ptr = sram_ptr;
			val_u32 = (u32)(cbdma_info[i].buf_va);
			for (j = 0 ; j < (test_size >> 2); j++) {
				/* Compare (test_size) bytes of data in DRAM */
				BUG_ON(*u32_ptr != val_u32);
				u32_ptr++;
				val_u32 += 4;
			}
			printk(KERN_INFO "Data in SRAM: OK\n");
#endif
			iounmap(sram_ptr);

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;
			cbdma_info[i].cbdma_ptr->length = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) == 0)
			cbdma_info[i].cbdma_ptr->des_adr = ((u32)(cbdma_info[i].dma_handle)) + test_size;
#else
			cbdma_info[i].cbdma_ptr->des_adr = ((u32)(cbdma_info[i].dma_handle)) + test_size - (4 - UNALIGNED_DROP_S);
#endif
			cbdma_info[i].cbdma_ptr->src_adr = 0;
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_WR;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}
			dump_data(cbdma_info[i].buf_va + test_size, 0x20);
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
			dump_data((u8 *)((u32)(cbdma_info[i].buf_va) + test_size * 2 - 0x20), 0x20);
#endif

			u32_ptr = (u32 *)(cbdma_info[i].buf_va + test_size);
			val_u32 = (u32)(cbdma_info[i].buf_va);
			for (j = 0 ; j < (test_size >> 2); j++) {
				/* Compare (test_size) bytes of data in DRAM */
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
				if (j == 0) {
					val_u32 += 4;
				} else if ((j + 2) >= (test_size >> 2)) {
					break;
				}
#endif
				BUG_ON(*u32_ptr != val_u32);
				u32_ptr++;
				val_u32 += 4;
			}
			printk(KERN_INFO "R/W test: OK\n\n");
			printk(KERN_INFO "COPY test\n");
			
			test_size = BUF_SIZE_DRAM >> 1;
			u32_ptr = (u32 *)(cbdma_info[i].buf_va + test_size);
			val_u32 = (u32)(u32_ptr);
			for (j = 0 ; (j < test_size >> 2); j++) {
				*u32_ptr = cpu_to_be32(val_u32);
				u32_ptr++;
				val_u32 += 4;
			}
			dump_data(cbdma_info[i].buf_va + test_size, 0x20);

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;
			cbdma_info[i].cbdma_ptr->length = test_size - UNALIGNED_DROP_S;
			cbdma_info[i].cbdma_ptr->src_adr = UNALIGNED_ADDR_S((u32)(cbdma_info[i].dma_handle) + test_size);
			cbdma_info[i].cbdma_ptr->des_adr = UNALIGNED_ADDR_S((u32)(cbdma_info[i].dma_handle));
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_CP;
			wmb();

			while (1) {
				if (cbdma_info[i].cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}
			dump_data(cbdma_info[i].buf_va, 0x20);

			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			expected_u32 = (u32)(cbdma_info[i].buf_va) + test_size;
			for (j = 0 ; (j < test_size >> 2); j++) {
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
				if (j != 0) {
					BUG_ON(*u32_ptr != cpu_to_be32(expected_u32));
				}
#else
				BUG_ON(*u32_ptr != cpu_to_be32(expected_u32));
#endif
				u32_ptr++;
				expected_u32 += 4;
			}
			printk(KERN_INFO "COPY test: OK\n\n");
		}
	}

	printk(KERN_INFO "%s(), end\n", __func__);
}


static void sp_cbdma_sg_lli(u32 sg_idx, volatile struct cbdma_reg *cbdma_ptr, struct cbdma_sg_lli *cbdma_sg_lli_ptr)
{
	if (cbdma_sg_lli_ptr->sg_lp_mode) {
		cbdma_ptr->sg_lp_mode = 0x00000001;
		cbdma_ptr->sg_lp_sram_start = 0x00000000;
		cbdma_ptr->sg_lp_sram_size = CBDMA_SG_LP_SZ_4KB;
	} else {
		cbdma_ptr->sg_lp_mode = 0x00000000;
	}

	cbdma_ptr->sg_en_go	 = CBDMA_SG_EN_GO_EN;
	cbdma_ptr->sg_idx	 = sg_idx;
	cbdma_ptr->sg_length	 = cbdma_sg_lli_ptr->sg_length;
	cbdma_ptr->sg_src_adr	 = cbdma_sg_lli_ptr->sg_src_adr;
	cbdma_ptr->sg_des_adr	 = cbdma_sg_lli_ptr->sg_des_adr;
	cbdma_ptr->sg_memset_val = cbdma_sg_lli_ptr->sg_memset_val;
	cbdma_ptr->sg_cfg	 = cbdma_sg_lli_ptr->sg_cfg;
}

static void sp_cbdma_tst_sg_memset_00(void *data)
{
	int i, j, val;
	u32 sg_idx, test_size, expected_u32;
	u32 *u32_ptr;
	struct cbdma_sg_lli sg_lli;

	printk(KERN_INFO "%s(), start\n", __func__);

	memset(&sg_lli, 0, sizeof(sg_lli));
	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;

			test_size = 1 << 10;

			/* 1st of LLI */
			sg_idx = 0;
			sg_lli.sg_length	= test_size;
			sg_lli.sg_src_adr	= (u32)(cbdma_info[i].dma_handle);
			sg_lli.sg_des_adr	= (u32)(cbdma_info[i].dma_handle);
			sg_lli.sg_memset_val	= PATTERN4TEST(0x5A);
			sg_lli.sg_cfg		= CBDMA_SG_CFG_NOT_LAST | CBDMA_SG_CFG_MEMSET;
			sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);

			/* 2nd of LLI, last one */
			sg_idx++;
			sg_lli.sg_length	= test_size;
			sg_lli.sg_src_adr	= (u32)(cbdma_info[i].dma_handle) + test_size * sg_idx;
			sg_lli.sg_des_adr	= (u32)(cbdma_info[i].dma_handle) + test_size * sg_idx;
			sg_lli.sg_memset_val	= PATTERN4TEST(0xA5);
			sg_lli.sg_cfg		= CBDMA_SG_CFG_LAST | CBDMA_SG_CFG_MEMSET;
			sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);

			cbdma_info[i].cbdma_ptr->sg_idx = 0;	/* Start from index-0 */
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->sg_en_go = CBDMA_SG_EN_GO_EN | CBDMA_SG_EN_GO_GO;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->sg_en_go & CBDMA_SG_EN_GO_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}

			/* Verification of the 1st of LLI */
			dump_data(cbdma_info[i].buf_va, 0x20);
			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			expected_u32 = PATTERN4TEST(0x5A);
			for (j = 0 ; j < (test_size >> 2); j++) {
				BUG_ON(*u32_ptr != expected_u32);
				u32_ptr++;
			}

			/* Verification of the 2nd of LLI */
			dump_data(cbdma_info[i].buf_va + test_size, 0x20);
			u32_ptr = (u32 *)(cbdma_info[i].buf_va + test_size);
			expected_u32 = PATTERN4TEST(0xA5);
			for (j = 0 ; j < (test_size >> 2); j++) {
				BUG_ON(*u32_ptr != expected_u32);
				u32_ptr++;
			}

		}
	}
	printk(KERN_INFO "%s(), end\n", __func__);
}

static void sp_cbdma_tst_sg_memset_01(void *data)
{
	int i, j, k, val;
	u32 sg_idx, test_size, expected_u32;
	u32 *u32_ptr;
	struct cbdma_sg_lli sg_lli;

	printk(KERN_INFO "%s(), start\n", __func__);

	memset(&sg_lli, 0, sizeof(sg_lli));
	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;

			test_size = BUF_SIZE_DRAM / NUM_SG_IDX;

			for (j = 0; j < NUM_SG_IDX; j++) {
				sg_idx = j;
				sg_lli.sg_length	= test_size;
				sg_lli.sg_src_adr	= (u32)(cbdma_info[i].dma_handle) + j * test_size;
				sg_lli.sg_des_adr	= (u32)(cbdma_info[i].dma_handle) + j * test_size;
				sg_lli.sg_memset_val	= PATTERN4TEST((((i << 4) | j) ^ 0x5A) & 0xFF);
				sg_lli.sg_cfg		= (j != (NUM_SG_IDX - 1)) ?
							  (CBDMA_SG_CFG_NOT_LAST | CBDMA_SG_CFG_MEMSET) :
							  (CBDMA_SG_CFG_LAST | CBDMA_SG_CFG_MEMSET);
				sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);
			}

			cbdma_info[i].cbdma_ptr->sg_idx = 0;	/* Start from index-0 */
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->sg_en_go = CBDMA_SG_EN_GO_EN | CBDMA_SG_EN_GO_GO;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->sg_en_go & CBDMA_SG_EN_GO_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}

			/* Verification */
			for (j = 0; j < NUM_SG_IDX; j++) {
				dump_data(cbdma_info[i].buf_va + j * test_size, 0x20);
				u32_ptr = (u32 *)((u32)(cbdma_info[i].buf_va) + j * test_size);
				expected_u32 = PATTERN4TEST((((i << 4) | j) ^ 0x5A) & 0xFF);
				for (k = 0 ; k < (test_size >> 2); k++) {
					BUG_ON(*u32_ptr != expected_u32);
					u32_ptr++;
				}
			}
		}
	}
	printk(KERN_INFO "%s(), end\n", __func__);
}

static void sp_cbdma_tst_sg_rw_00(void *data)
{
	int i, j, k, val;
	u32 sg_idx, test_size, expected_u32, val_u32;
	u32 *u32_ptr;
	struct cbdma_sg_lli sg_lli;
	const u32 sg_idx_used = 4;

	printk(KERN_INFO "%s(), start\n", __func__);

	memset(&sg_lli, 0, sizeof(sg_lli));
	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);
			test_size = MIN(BUF_SIZE_DRAM, cbdma_info[i].sram_size) / sg_idx_used / 2;

			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			val_u32 = (u32)(u32_ptr);
			expected_u32 = val_u32;
			for (k = 0; k < sg_idx_used; k++) {
				for (j = 0; j < (test_size >> 2); j++) {
					/* Fill (test_size) bytes of data to DRAM */
					*u32_ptr = val_u32;
					u32_ptr++;
					val_u32 += 4;
				}
			}

			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			for (k = 0; k < sg_idx_used; k++) {
				dump_data((u8 *)u32_ptr, 0x20);
				u32_ptr += (test_size >> 2);
			}

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;

			for (j = 0; j < sg_idx_used; j++) {
				sg_idx = j;
				sg_lli.sg_length	= test_size;
				sg_lli.sg_src_adr	= (u32)(cbdma_info[i].dma_handle) + j * test_size;
				sg_lli.sg_des_adr	= j * test_size;
				sg_lli.sg_cfg		= (j != (sg_idx_used - 1)) ?
							  (CBDMA_SG_CFG_NOT_LAST | CBDMA_SG_CFG_RD) :
							  (CBDMA_SG_CFG_LAST | CBDMA_SG_CFG_RD);
				sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);
			}

			cbdma_info[i].cbdma_ptr->sg_idx = 0;	/* Start from index-0 */
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->sg_en_go = CBDMA_SG_EN_GO_EN | CBDMA_SG_EN_GO_GO;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->sg_en_go & CBDMA_SG_EN_GO_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;

			for (j = 0; j < sg_idx_used; j++) {
				sg_idx = j;
				sg_lli.sg_length	= test_size;
				sg_lli.sg_src_adr	= j * test_size;
				sg_lli.sg_des_adr	= (u32)(cbdma_info[i].dma_handle) + (j + sg_idx_used) * test_size;
				sg_lli.sg_cfg		= (j != (sg_idx_used - 1)) ?
							  (CBDMA_SG_CFG_NOT_LAST | CBDMA_SG_CFG_WR) :
							  (CBDMA_SG_CFG_LAST | CBDMA_SG_CFG_WR);
				sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);
			}

			cbdma_info[i].cbdma_ptr->sg_idx = 0;	/* Start from index-0 */
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->sg_en_go = CBDMA_SG_EN_GO_EN | CBDMA_SG_EN_GO_GO;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->sg_en_go & CBDMA_SG_EN_GO_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}

			u32_ptr = (u32 *)((u32)(cbdma_info[i].buf_va) + sg_idx_used * test_size);
			for (k = 0; k < sg_idx_used; k++) {
				dump_data((u8 *)u32_ptr, 0x20);
				for (j = 0; j < (test_size >> 2); j++) {
					BUG_ON(*u32_ptr != expected_u32);
					u32_ptr++;
					expected_u32 += 4;
				}
			}
		}
	}
	printk(KERN_INFO "%s(), end\n", __func__);
}

static void sp_cbdma_tst_sg_cp_00(void *data)
{
	int i, j, val;
	u32 sg_idx, test_size, expected_u32, val_u32;
	u32 *u32_ptr;
	struct cbdma_sg_lli sg_lli;

	printk(KERN_INFO "%s(), start\n", __func__);

	memset(&sg_lli, 0, sizeof(sg_lli));
	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);
			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			val_u32 = (u32)(u32_ptr);
			expected_u32 = val_u32;
			test_size = BUF_SIZE_DRAM >> 1;
			for (j = 0 ; j < (test_size >> 2); j++) {
				/* Fill (test_size) bytes of data to DRAM */
				*u32_ptr = val_u32;
				u32_ptr++;
				val_u32 += 4;
			}
			dump_data(cbdma_info[i].buf_va, 0x20);

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;

			/* 1st of LLI, last one */
			sg_idx = 0;
			sg_lli.sg_length	= test_size;
			sg_lli.sg_src_adr	= (u32)(cbdma_info[i].dma_handle);
			sg_lli.sg_des_adr	= (u32)(cbdma_info[i].dma_handle) + test_size;
			sg_lli.sg_cfg		= CBDMA_SG_CFG_LAST | CBDMA_SG_CFG_CP;

			sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);

			cbdma_info[i].cbdma_ptr->sg_idx = 0;	/* Start from index-0 */
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->sg_en_go = CBDMA_SG_EN_GO_EN | CBDMA_SG_EN_GO_GO;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->sg_en_go & CBDMA_SG_EN_GO_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}
				break;
			}

			/* Verification of the 1st of LLI */
			dump_data(cbdma_info[i].buf_va + test_size, 0x20);
			u32_ptr = (u32 *)((u32)(cbdma_info[i].buf_va) + test_size);
			for (j = 0 ; j < (test_size >> 2); j++) {
				BUG_ON(*u32_ptr != expected_u32);
				u32_ptr++;
				expected_u32 += 4;
			}
		}
	}
	printk(KERN_INFO "%s(), end\n", __func__);
}

static void sp_cbdma_tst_sg_cp_01(void *data)
{
	int i, j, k, val;
	u32 sg_idx, test_size, expected_u32, val_u32;
	u32 *u32_ptr;
	struct cbdma_sg_lli sg_lli;
	const u32 sg_idx_used = 8;

	printk(KERN_INFO "%s(), start\n", __func__);

	memset(&sg_lli, 0, sizeof(sg_lli));
	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);
			test_size = BUF_SIZE_DRAM / sg_idx_used / 2;

			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			val_u32 = (u32)(u32_ptr);
			expected_u32 = val_u32;
			for (k = 0; k < sg_idx_used; k++) {
				for (j = 0; j < (test_size >> 2); j++) {
					/* Fill (test_size) bytes of data to DRAM */
					*u32_ptr = val_u32;
					u32_ptr++;
					val_u32 += 4;
				}
			}

			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			for (k = 0; k < sg_idx_used; k++) {
				dump_data((u8 *)u32_ptr, 0x20);
				u32_ptr += (test_size >> 2);
			}

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;

			for (j = 0; j < sg_idx_used; j++) {
				sg_idx = j;
				sg_lli.sg_length	= test_size;
				sg_lli.sg_src_adr	= (u32)(cbdma_info[i].dma_handle) + j * test_size;
				sg_lli.sg_des_adr	= (u32)(cbdma_info[i].dma_handle) + (j + sg_idx_used) * test_size;
				sg_lli.sg_cfg		= (j != (sg_idx_used - 1)) ?
							  (CBDMA_SG_CFG_NOT_LAST | CBDMA_SG_CFG_CP) :
							  (CBDMA_SG_CFG_LAST | CBDMA_SG_CFG_CP);
				sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);
			}

			cbdma_info[i].cbdma_ptr->sg_idx = 0;	/* Start from index-0 */
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->sg_en_go = CBDMA_SG_EN_GO_EN | CBDMA_SG_EN_GO_GO;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->sg_en_go & CBDMA_SG_EN_GO_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}

			u32_ptr = (u32 *)((u32)(cbdma_info[i].buf_va) + sg_idx_used * test_size);
			for (k = 0; k < sg_idx_used; k++) {
				dump_data((u8 *)u32_ptr, 0x20);
				for (j = 0; j < (test_size >> 2); j++) {
					BUG_ON(*u32_ptr != expected_u32);
					u32_ptr++;
					expected_u32 += 4;
				}
			}
		}
	}
	printk(KERN_INFO "%s(), end\n", __func__);
}

static void sp_cbdma_tst_sg_lp_cp_00(void *data)
{
	int i, j, val;
	u32 sg_idx, test_size, expected_u32, val_u32;
	u32 *u32_ptr;
	struct cbdma_sg_lli sg_lli;

	printk(KERN_INFO "%s(), start\n", __func__);

	memset(&sg_lli, 0, sizeof(sg_lli));
	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);
			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			val_u32 = (u32)(u32_ptr);
			expected_u32 = val_u32;
			test_size = BUF_SIZE_DRAM >> 1;
			for (j = 0 ; j < (test_size >> 2); j++) {
				/* Fill (test_size) bytes of data to DRAM */
				*u32_ptr = val_u32;
				u32_ptr++;
				val_u32 += 4;
			}
			dump_data(cbdma_info[i].buf_va, 0x20);

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;

			/* 1st of LLI, last one */
			sg_idx = 0;
			sg_lli.sg_length	= test_size;
			sg_lli.sg_src_adr	= (u32)(cbdma_info[i].dma_handle);
			sg_lli.sg_des_adr	= (u32)(cbdma_info[i].dma_handle) + test_size;
			sg_lli.sg_cfg		= CBDMA_SG_CFG_LAST | CBDMA_SG_CFG_CP;
			sg_lli.sg_lp_mode	= 1;

			sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);

			cbdma_info[i].cbdma_ptr->sg_idx = 0;	/* Start from index-0 */
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->sg_en_go = CBDMA_SG_EN_GO_EN | CBDMA_SG_EN_GO_GO;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->sg_en_go & CBDMA_SG_EN_GO_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}
				break;
			}

			/* Verification of the 1st of LLI */
			dump_data(cbdma_info[i].buf_va + test_size, 0x20);
			u32_ptr = (u32 *)((u32)(cbdma_info[i].buf_va) + test_size);
			for (j = 0 ; j < (test_size >> 2); j++) {
				BUG_ON(*u32_ptr != expected_u32);
				u32_ptr++;
				expected_u32 += 4;
			}
		}
	}
	printk(KERN_INFO "%s(), end\n", __func__);
}

static void sp_cbdma_tst_sg_lp_cp_01(void *data)
{
	int i, j, k, val;
	u32 sg_idx, test_size, expected_u32, val_u32;
	u32 *u32_ptr;
	struct cbdma_sg_lli sg_lli;
	const u32 sg_idx_used = 8;

	printk(KERN_INFO "%s(), start\n", __func__);

	memset(&sg_lli, 0, sizeof(sg_lli));
	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);
			test_size = BUF_SIZE_DRAM / sg_idx_used / 2;

			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			val_u32 = (u32)(u32_ptr);
			expected_u32 = val_u32;
			for (k = 0; k < sg_idx_used; k++) {
				for (j = 0; j < (test_size >> 2); j++) {
					/* Fill (test_size) bytes of data to DRAM */
					*u32_ptr = val_u32;
					u32_ptr++;
					val_u32 += 4;
				}
			}

			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			for (k = 0; k < sg_idx_used; k++) {
				dump_data((u8 *)u32_ptr, 0x20);
				u32_ptr += (test_size >> 2);
			}

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;

			for (j = 0; j < sg_idx_used; j++) {
				sg_idx = j;
				sg_lli.sg_length	= test_size;
				sg_lli.sg_src_adr	= (u32)(cbdma_info[i].dma_handle) + j * test_size;
				sg_lli.sg_des_adr	= (u32)(cbdma_info[i].dma_handle) + (j + sg_idx_used) * test_size;
				sg_lli.sg_cfg		= (j != (sg_idx_used - 1)) ?
							  (CBDMA_SG_CFG_NOT_LAST | CBDMA_SG_CFG_CP) :
							  (CBDMA_SG_CFG_LAST | CBDMA_SG_CFG_CP);
				sg_lli.sg_lp_mode	= 1;
				sp_cbdma_sg_lli(sg_idx, cbdma_info[i].cbdma_ptr, &sg_lli);
			}

			cbdma_info[i].cbdma_ptr->sg_idx = 0;	/* Start from index-0 */
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->sg_en_go = CBDMA_SG_EN_GO_EN | CBDMA_SG_EN_GO_GO;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->sg_en_go & CBDMA_SG_EN_GO_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}

			u32_ptr = (u32 *)((u32)(cbdma_info[i].buf_va) + sg_idx_used * test_size);
			for (k = 0; k < sg_idx_used; k++) {
				dump_data((u8 *)u32_ptr, 0x20);
				for (j = 0; j < (test_size >> 2); j++) {
					BUG_ON(*u32_ptr != expected_u32);
					u32_ptr++;
					expected_u32 += 4;
				}
			}
		}
	}
	printk(KERN_INFO "%s(), end\n", __func__);
}

static int sp_cbdma_tst_thread(void *data)
{
	int i;

	msleep(100);	/* let console be available */
	printk(KERN_INFO "%s, %d\n", __func__, __LINE__);

	sp_cbdma_tst_basic(data);	
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
	return 0;
#endif
	sp_cbdma_tst_sg_memset_00(data);
	sp_cbdma_tst_sg_memset_01(data);
	sp_cbdma_tst_sg_rw_00(data);
	sp_cbdma_tst_sg_cp_00(data);
	sp_cbdma_tst_sg_cp_01(data);
	sp_cbdma_tst_sg_lp_cp_00(data);
	sp_cbdma_tst_sg_lp_cp_01(data);

	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].buf_va) {
			dma_free_coherent(&(cbdma_info[i].pdev->dev), BUF_SIZE_DRAM, cbdma_info[i].buf_va, cbdma_info[i].dma_handle);
		}
	}

	return 0;
}



static void sp_qtest_cbdma_basic(void *data)
{
	int i, j, val;
	u32 *u32_ptr, expected_u32, val_u32, test_size;
	u32 *sram_ptr;

	unsigned int reg;
while(1)
{

	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].sram_size) {
			//printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info[i].name);
			printk(KERN_INFO "%s(), start num=%x\n", __func__,i);

			//printk(KERN_INFO "MEMSET test\n");
			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;
			cbdma_info[i].cbdma_ptr->length = BUF_SIZE_DRAM - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			cbdma_info[i].cbdma_ptr->src_adr = UNALIGNED_ADDR_S((u32)(cbdma_info[i].dma_handle));
			cbdma_info[i].cbdma_ptr->des_adr = UNALIGNED_ADDR_S((u32)(cbdma_info[i].dma_handle));
			cbdma_info[i].cbdma_ptr->memset_val = PATTERN4TEST(i ^ 0xaa);
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_MEMSET;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */					
					//printk(KERN_INFO "MEMSET test: OK\n\n");
					msleep(500);	/* let console be available */		
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}
#if 0
	//printk(KERN_INFO "MEMSET test: OK\n\n");
	reg = readl((void __iomem *)(B_SYSTEM_BASE + 32*4*8+ 4*9));
	if(reg==0xdd)
	{
		printk(KERN_INFO "%s() clk disable QCBDMA=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));
		//writel(0x20200000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
		writel(0x08080000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));


		while(1)
		{
			printk(KERN_INFO "%s() clk run->stop QCBDMA0=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));

			reg=readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
			if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_STOP_CTL0)
			{
				writel(0x00200000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
				printk(KERN_INFO "%s() QCBDMA0=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));				
				break;
			}
			else
			{
				writel(0x00080000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
			}
			msleep(100);
		}
		while(1)
		{
			printk(KERN_INFO "%s() clk run->stop QCBDMA1=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));
			if((reg&(QACCEPT_B_NOT_ACCEPT_CTL1|QDENY_DENY_CTL1|QREQ_B_NOT_REQ_CTL1))==Q_STOP_CTL1)
			{
				writel(0x20000000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
				printk(KERN_INFO "%s() QCBDMA1=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));				
				break;
			}
			else
			{
				writel(0x08000000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
			}
			msleep(100);
		}
		printk(KERN_INFO "%s() QCBDMA state=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));

	}
#endif
			//printk(KERN_INFO "R/W test\n");
			u32_ptr = (u32 *)(cbdma_info[i].buf_va);
			val_u32 = (u32)(u32_ptr);
			test_size = MIN(cbdma_info[i].sram_size, BUF_SIZE_DRAM) >> 1;
			for (j = 0 ; j < (test_size >> 2); j++) {
				/* Fill (test_size) bytes of data to DRAM */
				*u32_ptr = val_u32;
				u32_ptr++;
				val_u32 += 4;
			}

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;
			cbdma_info[i].cbdma_ptr->length = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			cbdma_info[i].cbdma_ptr->des_adr = 0;
			cbdma_info[i].cbdma_ptr->src_adr = UNALIGNED_ADDR_S((u32)(cbdma_info[i].dma_handle));
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_RD;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */					
					msleep(500);	/* let console be available */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}
				break;
			}

			iounmap(sram_ptr);

			val = atomic_read(&isr_cnt);
			cbdma_info[i].cbdma_ptr->int_en = 0;
			cbdma_info[i].cbdma_ptr->length = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			cbdma_info[i].cbdma_ptr->des_adr = ((u32)(cbdma_info[i].dma_handle)) + test_size - (4 - UNALIGNED_DROP_S);
			cbdma_info[i].cbdma_ptr->src_adr = 0;
			cbdma_info[i].cbdma_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_info[i].cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_WR;
			wmb();
			while (1) {
				if (cbdma_info[i].cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					#if 0
					//printk(KERN_INFO "%s() clk disable QCBDMA=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));
					reg = readl((void __iomem *)(B_SYSTEM_BASE + 32*4*8+ 4*9));
					//printk(KERN_INFO "%s() clk disable QCBDMA=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));

					if(reg==0xee)
					{
						//printk(KERN_INFO "%s() clk disable QCBDMA=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));					
						//printk(KERN_INFO "%s() clk disable BASE2=%x QAdr=%x\n", __func__,B_SYSTEM_BASE,(B_SYSTEM_BASE + 32*4*30+ 4*2));
						writel(0x20200000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
						//printk(KERN_INFO "%s() sleep2\n", __func__);
					}
					#endif
					msleep(500);	/* let console be available */

					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}

			u32_ptr = (u32 *)(cbdma_info[i].buf_va + test_size);
			val_u32 = (u32)(cbdma_info[i].buf_va);

			//printk(KERN_INFO "R/W test: OK\n\n");

		}
	}

}
	printk(KERN_INFO "%s(), end\n", __func__);

}


static void sp_qtest_cbdma_sram_rw(void *data)
{
	int i=0, j=0, jj=0,val;
	u32 *sram_ptr0,*sram_ptr1;

	unsigned int reg;

	while(1)
	{			
			sram_ptr0 = (u32 *)ioremap(cbdma_info[0].sram_addr, 40*1024);
			for(i=0;i<20;i++)
			{
				*(sram_ptr0+i)=0x11;
				if(jj==5)
				{
					printk(KERN_INFO "sp_qtest_cbdma_0 sram0=%x i=%x\n",sram_ptr0,i);
					jj=0;
				}
				jj++;
				msleep(500);
				if(*(sram_ptr0+i)!=0x11)
				{
					printk(KERN_INFO "sp_qtest_cbdma_0 sram err data=%x \n",*(sram_ptr0+i));
				}
				else
				{
					if(j==15)
					{
						printk(KERN_INFO "sp_qtest_cbdma_0 sram0  data=%x \n",*(sram_ptr0+i));
						j=0;
					}
					j++;
				}

			}
			
			//#else
			//sram_ptr0 = (u32 *)ioremap(cbdma_info[0].sram_addr, 40*1024);
			sram_ptr1 = (u32 *)ioremap(cbdma_info[1].sram_addr, 4*1024);
			
			for(i=0;i<20;i++)
			{
				*(sram_ptr1+i)=0x11;
				if(jj==5)
				{
					printk(KERN_INFO "sp_qtest_cbdma_sram_rw sram1=%x i=%x\n",sram_ptr1,i);
					jj=0;
				}
				jj++;
				msleep(500);
				if(*(sram_ptr1+i)!=0x11)
				{
					printk(KERN_INFO "sp_qtest_cbdma_sram_rw sram1 err data=%x \n",*(sram_ptr1+i));
				}
				else
				{
					if(j==15)
					{
						printk(KERN_INFO "sp_qtest_cbdma_sram_rw sram1  data=%x \n",*(sram_ptr1+i));
						j=0;
					}
					j++;
				}

			}
			

			reg=readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));//0x9C000F08, Power Domain Group 2
			printk(KERN_INFO "%s() Power Domain=%x\n", __func__,reg);

			reg = readl((void __iomem *)(B_SYSTEM_BASE + 32*4*8+ 4*9));//0x9C000424, iop_data1			
			printk(KERN_INFO "iop_data1=%x \n",reg);
			if(reg==0xdd)
			{
				#if 1
				printk(KERN_INFO "%s() clk disable QCBDMA=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));
				//writel(0x28280000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
				writel(0x00080000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));


				while(1)
				{
					//printk(KERN_INFO "%s() clk run->stop QCBDMA0=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));

					reg=readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));//0x9C000F08, Power Domain Group 2
					printk(KERN_INFO "%s() Power Domain Group 2 =%x\n", __func__,reg);

					if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_STOP_CTL0)
					{					
						printk(KERN_INFO "%s() Q_STOP_CTL0\n", __func__); 		
						//writel(0x00200000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
						writel(0x00010000, (void __iomem *)(B_SYSTEM_BASE + 32*4*0+ 4*3));				
						printk(KERN_INFO "%s() QCBDMA0=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));				
						printk(KERN_INFO "%s() QCBDMA0 clk=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*0+ 4*3)));				

						for(i=0;i<5;i++)
						{
							*(sram_ptr0+i)=0x11;
							printk(KERN_INFO "Leon access sram0=%x i=%x\n",sram_ptr0,i);
							//msleep(500);
							if(*(sram_ptr0+i)!=0x11)
							{
								printk(KERN_INFO "Leon access  sram0 err data=%x \n",*(sram_ptr0+i));
							}
							else
							{
								if(j==3)
								{
									printk(KERN_INFO "Leon access  sram0  data=%x \n",*(sram_ptr0+i));
									j=0;
								}
								j++;
							}

						}

						writel(0xaa, (void __iomem *)(B_SYSTEM_BASE + 32*4*8+ 4*9));

						printk(KERN_INFO "%s() QCBDMA0=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));				

						printk(KERN_INFO "%s() QCBDMA0 clk=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*0+ 4*3)));				
						
						break;
					}
					else if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_DENIED_CTL0)
					{
						printk(KERN_INFO "%s() QCBDMA0 Q_DENIED_CTL0\n", __func__);		
						writel(0x00080008, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));//0x9C000F08, Power Domain Group 2, 
					}
					else if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_CONTINUE_CTL0)
					{
						printk(KERN_INFO "%s() QCBDMA0 Q_CONTINUE_CTL0\n", __func__);		
					}
					else if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_RNU_CTL0)
					{
						printk(KERN_INFO "%s() QCBDMA0 Q_RNU_CTL0\n", __func__);	
						break;
					}
					else if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_REQUEST_CTL0)
					{
						printk(KERN_INFO "%s() QCBDMA0 Q_REQUEST_CTL0\n", __func__);		
					}
					//else
					//{
					//	writel(0x00080000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));//0x9C000F08, Power Domain Group 2, 
					//}
					msleep(500);
				}
				#endif
				#if 1
				writel(0x08000000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));

				while(1)
				{
				
					printk(KERN_INFO "%s() clk run->stop QCBDMA1=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));
					reg=readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
					if((reg&(QACCEPT_B_NOT_ACCEPT_CTL1|QDENY_DENY_CTL1|QREQ_B_NOT_REQ_CTL1))==Q_STOP_CTL1)
					{
						//writel(0x20000000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
						writel(0x00020000, (void __iomem *)(B_SYSTEM_BASE + 32*4*0+ 4*3));
						printk(KERN_INFO "%s() QCBDMA1=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));				
						printk(KERN_INFO "%s() QCBDMA1 clk=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*0+ 4*3)));				

						for(i=0;i<5;i++)
						{
							*(sram_ptr1+i)=0x11;
							printk(KERN_INFO "Leon access sram1=%x sram1cnt=%x\n",sram_ptr1,i);
							//msleep(500);
							if(*(sram_ptr1+i)!=0x11)
							{
								printk(KERN_INFO "Leon access  sram1 err data=%x \n",*(sram_ptr1+i));
							}
							else
							{
								if(j==3)
								{
									printk(KERN_INFO "Leon access  cmp sram1  data=%x \n",*(sram_ptr1+i));
									j=0;
								}
								j++;
							}

						}

						writel(0xaa, (void __iomem *)(B_SYSTEM_BASE + 32*4*8+ 4*9));

						printk(KERN_INFO "%s() QCBDMA1=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));				

						printk(KERN_INFO "%s() QCBDMA1 clk=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*0+ 4*3)));		
						
						break;
					}
					else if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_DENIED_CTL0)
					{
						printk(KERN_INFO "%s() QCBDMA1 Q_DENIED_CTL0\n", __func__);		
						writel(0x00080008, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));//0x9C000F08, Power Domain Group 2, 
					}
					else if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_CONTINUE_CTL0)
					{
						printk(KERN_INFO "%s() QCBDMA1 Q_CONTINUE_CTL0\n", __func__);		
					}
					else if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_RNU_CTL0)
					{
						printk(KERN_INFO "%s() QCBDMA1 Q_RNU_CTL0\n", __func__);	
						break;
					}
					else if((reg&(QACCEPT_B_NOT_ACCEPT_CTL0|QDENY_DENY_CTL0|QREQ_B_NOT_REQ_CTL0))==Q_REQUEST_CTL0)
					{
						printk(KERN_INFO "%s() QCBDMA1 Q_REQUEST_CTL0\n", __func__);		
					}
					//else
					//{
					//	writel(0x08000000, (void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2));
					//}
					msleep(500);
				}
				#endif
				//writel(0xaa, (void __iomem *)(B_SYSTEM_BASE + 32*4*8+ 4*9));
				printk(KERN_INFO "%s() QCBDMA state=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));

			}
			//msleep(500);
			//printk(KERN_INFO "%s() QCBDMA state=%x\n", __func__,readl((void __iomem *)(B_SYSTEM_BASE + 32*4*30+ 4*2)));






	}

	printk(KERN_INFO "%s(), end\n", __func__);

}



static int sp_qtest_cbdma_thread(void *data)
{
	int i;

	msleep(100);	/* let console be available */
	printk(KERN_INFO "%s, %d\n", __func__, __LINE__);

	//sp_qtest_cbdma_basic(data);	
	sp_qtest_cbdma_sram_rw(data);	

	for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info[i].buf_va) {
			dma_free_coherent(&(cbdma_info[i].pdev->dev), BUF_SIZE_DRAM, cbdma_info[i].buf_va, cbdma_info[i].dma_handle);
		}
	}

	return 0;
}

static const struct platform_device_id sp_qtest_devtypes[] = {
	{
		.name = "sp_qtest_cbdma",
	}, {
		/* sentinel */
	}
};

static const struct of_device_id sp_qtest_dt_ids[] = {
	{
		.compatible = "sunplus,sp_qtest_cbdma",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, sp_qtest_dt_ids);

static irqreturn_t sp_cbdma_tst_irq(int irq, void *args)
{
	struct cbdma_info_s *ptr;
	u32 int_flag;
	unsigned long flags;

	local_irq_save(flags);
	atomic_inc(&isr_cnt);

	ptr = (struct cbdma_info_s *)(args);
	int_flag = ptr->cbdma_ptr->int_flag;
	
	//printk(KERN_INFO  "%s, %d, %s, int_flag: 0x%x, isr_cnt: %d\n", __func__, __LINE__, ptr->irq_name, int_flag, atomic_read(&isr_cnt));
	BUG_ON(int_flag != CBDMA_INT_FLAG_DONE);

	ptr->cbdma_ptr->int_flag = int_flag;

	local_irq_restore(flags);

	return IRQ_HANDLED;
}

static int sp_qtest_cbdma_probe(struct platform_device *pdev)
{
	static int idx_cbdma = 0;
	struct resource *res_mem;
	const struct of_device_id *match;
	void __iomem *membase;
	int ret, num_irq;

	if (idx_cbdma >= NUM_CBDMA) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	printk(KERN_INFO "%s, %d\n", __func__, __LINE__);

	if (idx_cbdma == 0) {
		memset(&cbdma_info, 0, sizeof(cbdma_info));
	}

	if (pdev->dev.of_node) {
		match = of_match_node(sp_qtest_dt_ids, pdev->dev.of_node);
		if (match == NULL) {
			printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
			return -ENODEV;
		}
		num_irq = of_irq_count(pdev->dev.of_node);
		if (num_irq != 1) {
			printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		}
	}

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res_mem)) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return PTR_ERR(res_mem);
	}

	membase = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR(membase)) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return PTR_ERR(membase);
	}

	cbdma_info[idx_cbdma].pdev = pdev;
	cbdma_info[idx_cbdma].cbdma_ptr = (volatile struct cbdma_reg *)(membase);
	cbdma_info[idx_cbdma].cbdma_ptr->int_flag = ~0;	/* clear all interrupt flags */
	cbdma_info[idx_cbdma].irq = platform_get_irq(pdev, 0);
	if (cbdma_info[idx_cbdma].irq < 0) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return -ENODEV;
	}
	sprintf(cbdma_info[idx_cbdma].name, "CBDMA_%x", (u32)(res_mem->start));
	sprintf(cbdma_info[idx_cbdma].irq_name, "irq_%x", (u32)(res_mem->start));
	ret = request_irq(cbdma_info[idx_cbdma].irq, sp_cbdma_tst_irq, 0, cbdma_info[idx_cbdma].irq_name, &cbdma_info[idx_cbdma]);
	if (ret) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return ret;
	}
	printk(KERN_INFO "%s, %d, irq: %d, %s\n", __func__, __LINE__, cbdma_info[idx_cbdma].irq, cbdma_info[idx_cbdma].irq_name);
	if (((u32)(res_mem->start)) == 0x9C000D00) {
		/* CBDMA0 */
		cbdma_info[idx_cbdma].sram_addr = 0x9E800000;
		cbdma_info[idx_cbdma].sram_size = 40 << 10;

	} else {
		/* CBDMA1 */
		cbdma_info[idx_cbdma].sram_addr = 0x9E820000;
		cbdma_info[idx_cbdma].sram_size = 4 << 10;

	}
	printk(KERN_INFO "%s, %d, SRAM: 0x%x bytes @ 0x%x\n", __func__, __LINE__, cbdma_info[idx_cbdma].sram_size, cbdma_info[idx_cbdma].sram_addr);

	/* Allocate uncached memory for test */
	cbdma_info[idx_cbdma].buf_va = dma_alloc_coherent(&(pdev->dev), BUF_SIZE_DRAM, &(cbdma_info[idx_cbdma].dma_handle), GFP_KERNEL);
	if (cbdma_info[idx_cbdma].buf_va == NULL) {
		printk(KERN_INFO "%s, %d, Can't allocation buffer for %s\n", __func__, __LINE__, cbdma_info[idx_cbdma].name);
		/* Skip error handling here */
		ret = -ENOMEM;
	}
	printk(KERN_INFO "DMA buffer for %s, VA: 0x%p, PA: 0x%x\n", cbdma_info[idx_cbdma].name, cbdma_info[idx_cbdma].buf_va, (u32)(cbdma_info[idx_cbdma].dma_handle));

	if (thread_ptr == NULL) {
		printk(KERN_INFO "Start a thread for test ...\n");
		thread_ptr = kthread_run(sp_qtest_cbdma_thread, cbdma_info, "sp_qtest_cbdma_thread");
		//thread_ptr = kthread_run(sp_cbdma_tst_thread, cbdma_info, "sp_cbdma_tst_thread");
		
	}

	idx_cbdma++;
	return 0;

}


static int sp_qtest_rw_cbdma_probe(struct platform_device *pdev)
{
	static int idx_cbdma = 0;
	struct resource *res_mem;
	const struct of_device_id *match;
	void __iomem *membase;
	int ret, num_irq;

	if (idx_cbdma >= NUM_CBDMA) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	printk(KERN_INFO "%s, %d\n", __func__, __LINE__);

	if (idx_cbdma == 0) {
		memset(&cbdma_info, 0, sizeof(cbdma_info));
	}

	if (pdev->dev.of_node) {
		match = of_match_node(sp_qtest_dt_ids, pdev->dev.of_node);
		if (match == NULL) {
			printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
			return -ENODEV;
		}
		num_irq = of_irq_count(pdev->dev.of_node);
		if (num_irq != 1) {
			printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		}
	}

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res_mem)) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return PTR_ERR(res_mem);
	}

	membase = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR(membase)) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return PTR_ERR(membase);
	}

	cbdma_info[idx_cbdma].pdev = pdev;
	cbdma_info[idx_cbdma].cbdma_ptr = (volatile struct cbdma_reg *)(membase);
	cbdma_info[idx_cbdma].cbdma_ptr->int_flag = ~0;	/* clear all interrupt flags */
	cbdma_info[idx_cbdma].irq = platform_get_irq(pdev, 0);
	if (cbdma_info[idx_cbdma].irq < 0) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return -ENODEV;
	}
	sprintf(cbdma_info[idx_cbdma].name, "CBDMA_%x", (u32)(res_mem->start));
	sprintf(cbdma_info[idx_cbdma].irq_name, "irq_%x", (u32)(res_mem->start));

	//ret = request_irq(cbdma_info[idx_cbdma].irq, sp_cbdma_tst_irq, 0, cbdma_info[idx_cbdma].irq_name, &cbdma_info[idx_cbdma]);

	if (ret) {
		printk(KERN_ERR "Error: %s, %d\n", __func__, __LINE__);
		return ret;
	}
	printk(KERN_INFO "%s, %d, irq: %d, %s\n", __func__, __LINE__, cbdma_info[idx_cbdma].irq, cbdma_info[idx_cbdma].irq_name);
	if (((u32)(res_mem->start)) == 0x9C000D00) {
		/* CBDMA0 */
		cbdma_info[idx_cbdma].sram_addr = 0x9E800000;
		cbdma_info[idx_cbdma].sram_size = 40 << 10;

	} else {
		/* CBDMA1 */
		cbdma_info[idx_cbdma].sram_addr = 0x9E820000;
		cbdma_info[idx_cbdma].sram_size = 4 << 10;

	}
	printk(KERN_INFO "%s, %d, SRAM: 0x%x bytes @ 0x%x\n", __func__, __LINE__, cbdma_info[idx_cbdma].sram_size, cbdma_info[idx_cbdma].sram_addr);

	/* Allocate uncached memory for test */
	cbdma_info[idx_cbdma].buf_va = dma_alloc_coherent(&(pdev->dev), BUF_SIZE_DRAM, &(cbdma_info[idx_cbdma].dma_handle), GFP_KERNEL);
	if (cbdma_info[idx_cbdma].buf_va == NULL) {
		printk(KERN_INFO "%s, %d, Can't allocation buffer for %s\n", __func__, __LINE__, cbdma_info[idx_cbdma].name);
		/* Skip error handling here */
		ret = -ENOMEM;
	}
	printk(KERN_INFO "DMA buffer for %s, VA: 0x%p, PA: 0x%x\n", cbdma_info[idx_cbdma].name, cbdma_info[idx_cbdma].buf_va, (u32)(cbdma_info[idx_cbdma].dma_handle));

	if (thread_ptr == NULL) {
		printk(KERN_INFO "Start a thread for test ...\n");
		thread_ptr = kthread_run(sp_qtest_cbdma_thread, cbdma_info, "sp_qtest_cbdma_thread");
		//thread_ptr = kthread_run(sp_cbdma_tst_thread, cbdma_info, "sp_cbdma_tst_thread");
		
	}

	idx_cbdma++;
	return 0;

}


static struct platform_driver qtest_driver = {
	.driver		= {
		.name		= "sp_qtest_cbdma",
		.of_match_table	= of_match_ptr(sp_qtest_dt_ids),
		
	},
	.id_table	= sp_qtest_devtypes,
	.probe		= sp_qtest_rw_cbdma_probe,

};

module_platform_driver(qtest_driver);

MODULE_DESCRIPTION("Qtest CBDMA driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sp_q_tst");
