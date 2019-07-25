/*
 * Sunplus Technology
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __SP_BCH_H
#define __SP_BCH_H

struct sp_bch_regs {
	uint32_t cr0;   /* 0.0 bch control register */
	uint32_t buf;   /* 0.1 bch encode/decode data start address */
	uint32_t ecc;   /* 0.2 bch encode/decode parity start address */
	uint32_t isr;   /* 0.3 bch interrupt,write 1 clear(Interrupt status)*/
	uint32_t srr;   /* 0.4 write 1 to reset bch(software clear flag)*/
	uint32_t ier;   /* 0.5 bch interrupt mask(Interrupt mask) */
	uint32_t sr;    /* 0.6 bch report status(Report status) */
	uint32_t esr;   /* 0.7 report Nth(N=0~31)sector has err bits or not */
	uint32_t fsr;   /* 0.8 report Nth(N=0~31)sector has err bits can't fix*/
	uint32_t ipn;   /* 0.9 report IP number */
	uint32_t ipd;   /* 0.10 report IP update date*/
	uint32_t cr1;   /* 0.11 control register */
};

/*
 *  macros for cr0 register
 */
#define CR0_START                  BIT(0)
#define CR0_AUTOSTART              BIT(1)
#define CR0_ENCODE                 0
#define CR0_DECODE                 BIT(4)
#define CR0_CMODE_1024x60         (0 << 8)
#define CR0_CMODE_1024x40         (1 << 8)
#define CR0_CMODE_1024x24         (2 << 8)
#define CR0_CMODE_1024x16         (3 << 8)
#define CR0_CMODE_512x8           (4 << 8)
#define CR0_CMODE_512x4           (5 << 8)
#define CR0_CMODE(n)              (((n) & 7) << 8)
#define CR0_DMODE(n)              ((n) ? BIT(11) : 0)
#define CR0_NBLKS(n)              ((((n) - 1) & 0x1f) << 16)
#define CR0_BMODE(n)              (((n) & 7) << 28)

/*
 *  macros for isr register
 */
#define ISR_BCH                   BIT(0)
#define ISR_BUSY                  BIT(4)
#define ISR_CURR_DBLK(x)          (((x) >> 8) & 0x1f)
#define ISR_CURR_CBLK(x)          (((x) >> 16) & 0x1f)

/*
 *  macros for srr register
 */
#define SRR_RESET                 BIT(0)

/*
 *  macros for ier register
 */
#define IER_DONE                  BIT(0)
#define IER_FAIL                  BIT(1)

/*
 *  macros for sr register
 */
#define SR_DONE                   BIT(0)
#define SR_FAIL                   BIT(4)
#define SR_ERR_BITS(x)            (((x) >> 8) & 0x7ff)
#define SR_BLANK_00               BIT(28)       /* data are all 0x00 */
#define SR_BLANK_FF               BIT(24)       /* data are all 0xff */

int sp_bch_init(struct mtd_info *mtd, int *parity_sector_sz);
int sp_bch_encode(struct mtd_info *mtd, void *buf, void *ecc);
int sp_bch_decode(struct mtd_info *mtd, void *buf, void *ecc);

int sp_bch_encode_1024x60(void *buf, void *ecc);
int sp_bch_decode_1024x60(void *buf, void *ecc);

int sp_autobch_config(struct mtd_info *mtd, void *buf, void *ecc, int enc);
int sp_autobch_result(struct mtd_info *mtd);


#endif /* __SP_BCH_H */

