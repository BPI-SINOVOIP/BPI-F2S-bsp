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

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/of_platform.h>
#include <linux/firmware.h>


#define DEVICE_NAME	"sunplus,sp7021-cbdma"

#define CB_DMA_REG_NAME	"cb_dma"

struct sp_cbdma_reg {
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

#define SP_CBDMA_BASIC_TEST
#if defined(SP_CBDMA_BASIC_TEST)
/* Unaligned test */
#define UNALIGNED_DROP_S	0	/* 0, 1, 2, 3 */
#define UNALIGNED_DROP_E	0	/* 0, 1, 2, 3 */
#define UNALIGNED_ADDR_S(X)	(X + UNALIGNED_DROP_S)
#define UNALIGNED_ADDR_E(X)	(X - UNALIGNED_DROP_E)

#define BUF_SIZE_DRAM		(PAGE_SIZE * 2)

#define PATTERN4TEST(X)		((((u32)(X)) << 24) | (((u32)(X)) << 16) | (((u32)(X)) << 8) | (((u32)(X)) << 0))
#endif

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

#define MIN(X, Y)		((X) < (Y) ? (X): (Y))

#define NUM_SG_IDX		32

static volatile struct sp_cbdma_reg *cbdma_reg_ptr = NULL;

struct cbdma_info_s {
	char name[32];
	struct platform_device *pdev;
	char irq_name[32];
	int irq;
	struct miscdevice dev;
	struct mutex cbdma_lock;
	void __iomem *cbdma_regs;
	//volatile struct sp_cbdma_reg *cbdma_regs;
	u32 sram_addr;
	u32 sram_size;
	void *buf_va;
	dma_addr_t dma_handle;
};
static struct cbdma_info_s *cbdma_info;

atomic_t isr_cnt = ATOMIC_INIT(0);

#if defined(SP_CBDMA_BASIC_TEST)
static struct task_struct *thread_ptr = NULL;
#endif

#define CBDMA_FUNC_DEBUG
#define CBDMA_KDBG_INFO
#define CBDMA_KDBG_ERR

#ifdef CBDMA_FUNC_DEBUG
	#define FUNC_DEBUG()    printk(KERN_INFO "[CBDMA] Debug: %s(%d)\n", __FUNCTION__, __LINE__)
#else
	#define FUNC_DEBUG()
#endif

#ifdef CBDMA_KDBG_INFO
#define DBG_INFO(fmt, args ...)	printk(KERN_INFO "K_CBDMA: " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef CBDMA_KDBG_ERR
#define DBG_ERR(fmt, args ...)	printk(KERN_ERR "K_CBDMA: " fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif


void dump_data(u8 *ptr, u32 size)
{
	u32 i, addr_begin;
	int length;
	char buffer[256];

	FUNC_DEBUG();

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

static int sp_cbdma_memset(void __iomem *cbdma_regs, int des_adr, int src_adr, int length, int pattern)
{
	struct sp_cbdma_reg *cbdma_reg_ptr;
	int val;

	FUNC_DEBUG();

	cbdma_reg_ptr = (struct sp_cbdma_reg *)cbdma_regs;

	val = atomic_read(&isr_cnt);
	writel(0, &cbdma_reg_ptr->int_en);
	writel(length, &cbdma_reg_ptr->length);
	writel(src_adr, &cbdma_reg_ptr->src_adr);
	writel(des_adr, &cbdma_reg_ptr->des_adr);
	writel(pattern, &cbdma_reg_ptr->memset_val);
	writel(~0, &cbdma_reg_ptr->int_en); /* Enable all interrupts */

	DBG_INFO("length=0x%x, src_adr=0x%x des_adr=0x%x, memset_val=0x%x\n",readl(&cbdma_reg_ptr->length),
		readl(&cbdma_reg_ptr->src_adr), readl(&cbdma_reg_ptr->des_adr), readl(&cbdma_reg_ptr->memset_val));

	wmb();
	writel(CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_MEMSET, &cbdma_reg_ptr->config);
	wmb();
	while (1) {
		if (readl(&cbdma_reg_ptr->config) & CBDMA_CONFIG_GO) {
			/* Still running */
			continue;
		}
		if (atomic_read(&isr_cnt) == val) {
			/* ISR not served */
			continue;
		}

		break;
	}

	return 0;
}

static int sp_cbdma_read(void __iomem *cbdma_regs, int des_adr, int src_adr, int length)
{
	struct sp_cbdma_reg *cbdma_reg_ptr;
	int val;

	FUNC_DEBUG();

	cbdma_reg_ptr = (struct sp_cbdma_reg *)cbdma_regs;

	val = atomic_read(&isr_cnt);
	writel(0, &cbdma_reg_ptr->int_en);
	writel(length, &cbdma_reg_ptr->length);
	writel(des_adr, &cbdma_reg_ptr->des_adr);
	writel(src_adr, &cbdma_reg_ptr->src_adr);
	writel(~0, &cbdma_reg_ptr->int_en); /* Enable all interrupts */

	DBG_INFO("length=0x%x, src_adr=0x%x des_adr=0x%x\n", readl(&cbdma_reg_ptr->length), readl(&cbdma_reg_ptr->src_adr), readl(&cbdma_reg_ptr->des_adr));

	wmb();
	writel(CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_RD, &cbdma_reg_ptr->config);
	wmb();
	while (1) {
		if (readl(&cbdma_reg_ptr->config) & CBDMA_CONFIG_GO) {
			/* Still running */
			continue;
		}
		if (atomic_read(&isr_cnt) == val) {
			/* ISR not served */
			continue;
		}
		break;
	}

	return 0;
}

static int sp_cbdma_write(void __iomem *cbdma_regs, int des_adr, int src_adr, int length)
{
	struct sp_cbdma_reg *cbdma_reg_ptr;
	int val;

	FUNC_DEBUG();

	cbdma_reg_ptr = (struct sp_cbdma_reg *)cbdma_regs;

	val = atomic_read(&isr_cnt);
	writel(0, &cbdma_reg_ptr->int_en);
	writel(length, &cbdma_reg_ptr->length);
	writel(des_adr, &cbdma_reg_ptr->des_adr);
	writel(src_adr, &cbdma_reg_ptr->src_adr);
	writel(~0, &cbdma_reg_ptr->int_en);	/* Enable all interrupts */

	DBG_INFO("length=0x%x, src_adr=0x%x des_adr=0x%x\n", readl(&cbdma_reg_ptr->length), readl(&cbdma_reg_ptr->src_adr), readl(&cbdma_reg_ptr->des_adr));

	wmb();
	writel(CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_WR, &cbdma_reg_ptr->config);
	wmb();
	while (1) {
		if (readl(&cbdma_reg_ptr->config) & CBDMA_CONFIG_GO) {
			/* Still running */
			continue;
		}
		if (atomic_read(&isr_cnt) == val) {
			/* ISR not served */
			continue;
		}

		break;
	}

	return 0;
}

static int sp_cbdma_copy(void __iomem *cbdma_regs, int des_adr, int src_adr, int length)
{
	struct sp_cbdma_reg *cbdma_reg_ptr;
	int val;

	FUNC_DEBUG();

	cbdma_reg_ptr = (struct sp_cbdma_reg *)cbdma_regs;

	val = atomic_read(&isr_cnt);
	writel(0, &cbdma_reg_ptr->int_en);
	writel(length, &cbdma_reg_ptr->length);
	writel(src_adr, &cbdma_reg_ptr->src_adr);
	writel(des_adr, &cbdma_reg_ptr->des_adr);
	writel(~0, &cbdma_reg_ptr->int_en); /* Enable all interrupts */

	DBG_INFO("length=0x%x, src_adr=0x%x des_adr=0x%x\n", readl(&cbdma_reg_ptr->length), readl(&cbdma_reg_ptr->src_adr), readl(&cbdma_reg_ptr->des_adr));

	wmb();
	writel(CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_CP, &cbdma_reg_ptr->config);
	wmb();

	while (1) {
		if (readl(&cbdma_reg_ptr->config) & CBDMA_CONFIG_GO) {
			/* Still running */
			continue;
		}
		if (atomic_read(&isr_cnt) == val) {
			/* ISR not served */
			continue;
		}

		break;
	}

	return 0;
}

#if defined(SP_CBDMA_BASIC_TEST)
static void sp_cbdma_tst_basic(void *data)
{
	struct sp_cbdma_reg *cbdma_reg_ptr;
	int i, j, val;
	u32 *u32_ptr, expected_u32, val_u32, test_size;
	u32 *sram_ptr;

	FUNC_DEBUG();

	cbdma_reg_ptr = (struct sp_cbdma_reg *)cbdma_info->cbdma_regs;

	//for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info->sram_size) {
			printk(KERN_INFO "Test for %s ------------------------\n", cbdma_info->name);
			printk(KERN_INFO "MEMSET test\n");

#if 1 // Replace it with sp_cbdma_memset() function
			val = BUF_SIZE_DRAM - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			sp_cbdma_memset(cbdma_info->cbdma_regs, UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle)), UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle)), val, PATTERN4TEST(i ^ 0xaa));
#else
			val = atomic_read(&isr_cnt);
			cbdma_reg_ptr->int_en = 0;
			cbdma_reg_ptr->length = BUF_SIZE_DRAM - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			cbdma_reg_ptr->src_adr = UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle));
			cbdma_reg_ptr->des_adr = UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle));
			cbdma_reg_ptr->memset_val = PATTERN4TEST(i ^ 0xaa);
			cbdma_reg_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_reg_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_MEMSET;
			wmb();
			while (1) {
				if (cbdma_reg_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}
#endif
			dump_data(cbdma_info->buf_va, 0x20);
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
			dump_data(((u8 *)cbdma_info->buf_va + BUF_SIZE_DRAM - 0x20), 0x20);
#endif
			u32_ptr = (u32 *)(cbdma_info->buf_va);
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

			sram_ptr = (u32 *)ioremap(cbdma_info->sram_addr, cbdma_info->sram_size);
			BUG_ON(!sram_ptr);
			u32_ptr = sram_ptr;
			val_u32 = (u32)(cbdma_info->sram_addr);
			test_size = cbdma_info->sram_size - 256; /* last 256 bytes is reserved for COPY, access it will trigger a ISR */

			printk("u32_ptr=0x%x, sram_ptr=0x%x, val_u32=0x%x, test_size=0x%x\n", (int)u32_ptr, (int)sram_ptr, val_u32, test_size);
			dump_data((u8 *)sram_ptr, 0x20);
			for (j = 0 ; j < (test_size >> 2); j++) {
				*u32_ptr = val_u32;
				u32_ptr++;
				val_u32 += 4;
			}
			dump_data((u8 *)sram_ptr, 0x20);
			u32_ptr = sram_ptr;
			val_u32 = (u32)(cbdma_info->sram_addr);
			for (j = 0 ; j < (test_size >> 2); j++) {
				BUG_ON(*u32_ptr != val_u32);
				u32_ptr++;
				val_u32 += 4;
			}

			printk(KERN_INFO "SRAM r/w test: OK\n\n");
			printk(KERN_INFO "R/W test\n");
			
			u32_ptr = (u32 *)(cbdma_info->buf_va);
			val_u32 = (u32)(u32_ptr);
			test_size = MIN(cbdma_info->sram_size, BUF_SIZE_DRAM) >> 1;

			printk("u32_ptr=0x%x, sram_ptr=0x%x, val_u32=0x%x, test_size=0x%x\n", (int)u32_ptr, (int)sram_ptr, val_u32, test_size);
			dump_data(cbdma_info->buf_va, 0x20);
			for (j = 0 ; j < (test_size >> 2); j++) {
				/* Fill (test_size) bytes of data to DRAM */
				*u32_ptr = val_u32;
				u32_ptr++;
				val_u32 += 4;
			}
			dump_data(cbdma_info->buf_va, 0x20);

#if 1 // Replace it with sp_cbdma_read() function
			val = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			sp_cbdma_read(cbdma_info->cbdma_regs, 0, cbdma_info->dma_handle, val);
#else
			val = atomic_read(&isr_cnt);
			cbdma_reg_ptr->int_en = 0;
			cbdma_reg_ptr->length = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
			cbdma_reg_ptr->des_adr = 0;
			cbdma_reg_ptr->src_adr = UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle));
			cbdma_reg_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_reg_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_RD;
			wmb();
			while (1) {
				if (cbdma_reg_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}
				break;
			}
#endif

			dump_data((u8 *)(sram_ptr), 0x20);
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) == 0)
			u32_ptr = sram_ptr;
			val_u32 = (u32)(cbdma_info->buf_va);
			for (j = 0 ; j < (test_size >> 2); j++) {
				/* Compare (test_size) bytes of data in DRAM */
				BUG_ON(*u32_ptr != val_u32);
				u32_ptr++;
				val_u32 += 4;
			}
			printk(KERN_INFO "Data in SRAM: OK\n");
#endif
			iounmap(sram_ptr);

#if 1 // Replace it with sp_cbdma_write() function
			val = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) == 0)
			sp_cbdma_write(cbdma_info->cbdma_regs, (((u32)(cbdma_info->dma_handle)) + test_size), 0, val);
#else
			sp_cbdma_write(cbdma_info->cbdma_regs, (((u32)(cbdma_info->dma_handle)) + test_size - (4 - UNALIGNED_DROP_S)), 0, val);
#endif
#else

			val = atomic_read(&isr_cnt);
			cbdma_reg_ptr->int_en = 0;
			cbdma_reg_ptr->length = test_size - UNALIGNED_DROP_S - UNALIGNED_DROP_E;
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) == 0)
			cbdma_reg_ptr->des_adr = ((u32)(cbdma_info->dma_handle)) + test_size;
#else
			cbdma_reg_ptr->des_adr = ((u32)(cbdma_info->dma_handle)) + test_size - (4 - UNALIGNED_DROP_S);
#endif
			cbdma_reg_ptr->src_adr = 0;
			cbdma_reg_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_reg_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_WR;
			wmb();
			while (1) {
				if (cbdma_reg_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}
#endif

			dump_data(cbdma_info->buf_va + test_size, 0x20);
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
			dump_data((u8 *)((u32)(cbdma_info->buf_va) + test_size * 2 - 0x20), 0x20);
#endif

			u32_ptr = (u32 *)(cbdma_info->buf_va + test_size);
			val_u32 = (u32)(cbdma_info->buf_va);
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
			u32_ptr = (u32 *)(cbdma_info->buf_va + test_size);
			val_u32 = (u32)(u32_ptr);
			for (j = 0 ; (j < test_size >> 2); j++) {
				*u32_ptr = cpu_to_be32(val_u32);
				u32_ptr++;
				val_u32 += 4;
			}
			dump_data(cbdma_info->buf_va + test_size, 0x20);

#if 1 // Replace it with sp_cbdma_copy() function
			val = test_size - UNALIGNED_DROP_S;
			sp_cbdma_copy(cbdma_info->cbdma_regs, UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle)), UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle) + test_size), val);
#else
			val = atomic_read(&isr_cnt);
			cbdma_reg_ptr->int_en = 0;
			cbdma_reg_ptr->length = test_size - UNALIGNED_DROP_S;
			cbdma_reg_ptr->src_adr = UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle) + test_size);
			cbdma_reg_ptr->des_adr = UNALIGNED_ADDR_S((u32)(cbdma_info->dma_handle));
			cbdma_reg_ptr->int_en = ~0;	/* Enable all interrupts */
			wmb();
			cbdma_reg_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_CP;
			wmb();

			while (1) {
				if (cbdma_reg_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				if (atomic_read(&isr_cnt) == val) {
					/* ISR not served */
					continue;
				}

				break;
			}
#endif
			dump_data(cbdma_info->buf_va, 0x20);

			u32_ptr = (u32 *)(cbdma_info->buf_va);
			expected_u32 = (u32)(cbdma_info->buf_va) + test_size;
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
	//}

	printk(KERN_INFO "%s(), end\n", __func__);
}

static int sp_cbdma_tst_thread(void *data)
{
	//int i;

	FUNC_DEBUG();

	msleep(100);	/* let console be available */
	DBG_INFO("%s, %d\n", __func__, __LINE__);

	sp_cbdma_tst_basic(data);	
#if ((UNALIGNED_DROP_S | UNALIGNED_DROP_E) != 0)
	return 0;
#endif
#if 0
	sp_cbdma_tst_sg_memset_00(data);
	sp_cbdma_tst_sg_memset_01(data);
	sp_cbdma_tst_sg_rw_00(data);
	sp_cbdma_tst_sg_cp_00(data);
	sp_cbdma_tst_sg_cp_01(data);
	sp_cbdma_tst_sg_lp_cp_00(data);
	sp_cbdma_tst_sg_lp_cp_01(data);
#endif
	//for (i = 0; i < NUM_CBDMA; i++) {
		if (cbdma_info->buf_va) {
			dma_free_coherent(&(cbdma_info->pdev->dev), BUF_SIZE_DRAM, cbdma_info->buf_va, cbdma_info->dma_handle);
		}
	//}

	return 0;
}
#endif // #if defined(SP_CBDMA_BASIC_TEST)

static irqreturn_t sp_cbdma_tst_irq(int irq, void *args)
{
	struct cbdma_info_s *ptr;
	struct sp_cbdma_reg *cbdma_reg_ptr;
	u32 int_flag;
	unsigned long flags;

	FUNC_DEBUG();

	local_irq_save(flags);
	atomic_inc(&isr_cnt);

	ptr = (struct cbdma_info_s *)(args);
	cbdma_reg_ptr = (struct sp_cbdma_reg *)ptr->cbdma_regs;
	int_flag = readl(&cbdma_reg_ptr->int_flag);

	DBG_INFO("%s, %d, %s, int_flag: 0x%x, isr_cnt: %d\n", __func__, __LINE__, ptr->irq_name, int_flag, atomic_read(&isr_cnt));
	BUG_ON(int_flag != CBDMA_INT_FLAG_DONE);

	writel(int_flag,&cbdma_reg_ptr->int_flag);

	local_irq_restore(flags);

	return IRQ_HANDLED;
}

static int sp_cbdma_probe(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *reg_base;
	long reg_data;
	int ret = 0;

	FUNC_DEBUG();

	/* Get a memory to store infos */
	cbdma_info = (struct cbdma_info_s *)devm_kzalloc(&pdev->dev, sizeof(struct cbdma_info_s), GFP_KERNEL);
	DBG_INFO("cbdma_info:0x%lx\n", (unsigned long)cbdma_info);
	if (cbdma_info == NULL)
	{
		printk("cbdma_info malloc fail\n");
		ret = -ENOMEM;
		goto fail_kmalloc;
	}

	/* Init */
	mutex_init(&cbdma_info->cbdma_lock);

	/* Register CBDMA device to kernel */
	cbdma_info->dev.name = "sp_cbdma";
	cbdma_info->dev.minor = MISC_DYNAMIC_MINOR;
	ret = misc_register(&cbdma_info->dev);
	DBG_INFO("ret:%u\n", ret);
	if (ret != 0) {	
		DBG_ERR("sp_cbdma device register fail\n");
		goto fail_regdev;
	}

	/* Get CBDMA register source */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, CB_DMA_REG_NAME);
	DBG_INFO("res:0x%lx\n", (unsigned long)res);
	if (res) {
		reg_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (IS_ERR(reg_base)) {
			DBG_ERR("%s devm_ioremap_resource fail\n",CB_DMA_REG_NAME);
		}
	}
	DBG_INFO("reg_base 0x%x\n",(unsigned int)reg_base);

	cbdma_info->pdev = pdev;
    cbdma_info->cbdma_regs = (void __iomem *)(reg_base);
	cbdma_reg_ptr = (volatile struct sp_cbdma_reg *)(cbdma_info->cbdma_regs);
	cbdma_info->irq = platform_get_irq(pdev, 0);
	if (cbdma_info->irq < 0) {
		DBG_ERR("Error: %s, %d\n", __func__, __LINE__);
		return -ENODEV;
	}
	sprintf(cbdma_info->name, "CBDMA_%x", (u32)(res->start));
	sprintf(cbdma_info->irq_name, "irq_%x", (u32)(res->start));
	ret = request_irq(cbdma_info->irq, sp_cbdma_tst_irq, 0, cbdma_info->irq_name, cbdma_info);
	if (ret) {
		DBG_ERR("Error: %s, %d\n", __func__, __LINE__);
		return ret;
	}
	DBG_INFO("%s, %d, irq: %d, %s\n", __func__, __LINE__, cbdma_info->irq, cbdma_info->irq_name);
	if (((u32)(res->start)) == 0x9C000D00) {
		/* CBDMA0 */
		cbdma_info->sram_addr = 0x9E800000;
		cbdma_info->sram_size = 40 << 10;

	} else {
		/* CBDMA1 */
		cbdma_info->sram_addr = 0x9E820000;
		cbdma_info->sram_size = 4 << 10;

	}
	DBG_INFO("%s, %d, SRAM: 0x%x bytes @ 0x%x\n", __func__, __LINE__, cbdma_info->sram_size, cbdma_info->sram_addr);

	reg_data = readl(&cbdma_reg_ptr->hw_ver);
	DBG_INFO("hw_ver:0x%lx\n", reg_data);

#if defined(SP_CBDMA_BASIC_TEST)
	/* Allocate uncached memory for test */
	cbdma_info->buf_va = dma_alloc_coherent(&(pdev->dev), BUF_SIZE_DRAM, &(cbdma_info->dma_handle), GFP_KERNEL);
	DBG_INFO("buf_va=0x%p, BUF_SIZE_DRAM=%lu\n", cbdma_info->buf_va, BUF_SIZE_DRAM);
	if (cbdma_info->buf_va == NULL) {
		DBG_ERR("%s, %d, Can't allocation buffer for cbdma_info\n", __func__, __LINE__);
		/* Skip error handling here */
		ret = -ENOMEM;
	}
	printk(KERN_INFO "DMA buffer for cbdma_info, VA: 0x%p, PA: 0x%x\n", cbdma_info->buf_va, (u32)(cbdma_info->dma_handle));

	/* Test CBDMA transfer */
	if (thread_ptr == NULL) {
		DBG_INFO("Start a thread for test ...\n");
		thread_ptr = kthread_run(sp_cbdma_tst_thread, cbdma_info, "sp_cbdma_tst_thread");
	}
#endif

fail_regdev:
	mutex_destroy(&cbdma_info->cbdma_lock);
fail_kmalloc:
	return ret;
}

static int sp_cbdma_remove(struct platform_device *pdev)
{
	FUNC_DEBUG();
	return 0;
}

static const struct of_device_id sp_cbdma_of_match[] = {
	{ .compatible = "sunplus,sp7021-cbdma" },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, sp_cbdma_of_match);

static struct platform_driver sp_cbdma_driver = {
	.probe = sp_cbdma_probe,
	.remove = sp_cbdma_remove,
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_cbdma_of_match),
	}
};

module_platform_driver(sp_cbdma_driver);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus CBDMA Driver");
MODULE_LICENSE("GPL");
