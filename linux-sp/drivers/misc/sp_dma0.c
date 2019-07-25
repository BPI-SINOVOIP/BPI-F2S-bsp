#include <linux/module.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <mach/io_map.h>
#include <asm/cacheflush.h>

//#define TRACE
#define TRACE			printk("%s:%d\n", __FUNCTION__, __LINE__)
#define MSLEEP(n)		usleep_range((n)*1000, (n)*1000)

#define DMA0_IRQS		9
#define DMA0_RUN		(1 << 31) // owner bit
#define WAIT(c)			while (c) { MSLEEP(1); }
#define DMA0_WAIT(c)	WAIT((c)->cfg1 & DMA0_RUN)

#define WORKMEM_BASE	0x9ea00000
#define WORKMEM_SIZE	SZ_512K
#define WORKMEM_VA(pa)	(sp_dma0.membase + ((pa) - WORKMEM_BASE))
#define IS_WORKMEM(a)	(((u32)(a) - WORKMEM_BASE) < WORKMEM_SIZE)

typedef volatile u32 REG;

struct dma_cfg {
	REG cfg0;	// len/src_offset/dst_offset
	REG cfg1;	// owner/dir/loops
	REG addr0;
	REG addr1;
};

struct urgent_cfg {
	REG cfg0;
	REG cfg1;
};

struct bio_cfg {
	REG addr;
	REG ctrl;
};

struct sp_dma0_reg {
	// G11~G12
	REG cfgs[2][32];
	// G13
	struct urgent_cfg ug[6];
	REG glb_ctrl;
	REG arb_wt;
	REG ch_loop_sts0;
	REG ch_loop_sts1;
	REG ch_loop_sts2;
	REG glb_sts;
	REG intr_ctrl;
};

struct sp_dma0_dev {
    struct sp_dma0_reg *reg;
	struct bio_cfg *bio;
    int irq;
	void *membase;
};

static struct sp_dma0_dev sp_dma0;

int sp_dma0_copy(u32 ch, u32 dst, u32 src, u32 size, bool wait)
{
	struct sp_dma0_reg *reg = sp_dma0.reg;
	REG *cfgs = reg->cfgs[0];
	struct dma_cfg *cfg;
	u32 dir = IS_WORKMEM(dst); // DMA0 direction 0:out 1:in
	u32 ab = (ch < 6); // 0:A<->A 1:A<->B

	BUG_ON(ch > 7);
	BUG_ON(!IS_WORKMEM(src) && !IS_WORKMEM(dst));
    BUG_ON(ab && IS_WORKMEM(src) && IS_WORKMEM(dst));
	BUG_ON(!ab && !(IS_WORKMEM(src) && IS_WORKMEM(dst)));

    //flush_cache_all();

	size = (size + 15) / 16; // DMA length: 128 bits aligned

	cfg = (struct dma_cfg *)&cfgs[ab ? (ch * 3) : (ch * 4 - 6)];
	DMA0_WAIT(cfg);

	if (ab) {
		cfg->cfg0 = size << 17;
		cfg->addr0 = dir ? dst : src;
		if (!dir) {
			sp_dma0.bio[ch].addr = dst;
			sp_dma0.bio[ch].ctrl = DMA0_RUN | (0 << 16) | size; // start bio read
		}
	} else {
		cfg->cfg0 = size << 16;
		cfg->addr0 = src;
		cfg->addr1 = dst;
	}

	cfg->cfg1 = DMA0_RUN | (dir << 30) | 1;

	if (ab && dir) {
		sp_dma0.bio[ch].addr = src;
		sp_dma0.bio[ch].ctrl = DMA0_RUN | (1 << 16) | size; // start bio write
	}

	if (wait) DMA0_WAIT(cfg); // wait dma done

	return 0;
}
EXPORT_SYMBOL(sp_dma0_copy);

void sp_dma0_wait(u32 ch)
{
	struct dma_cfg *cfg;
	cfg = (struct dma_cfg *)&sp_dma0.reg->cfgs[0][(ch < 6) ? (ch * 3) : (ch * 4 - 6)];
	DMA0_WAIT(cfg);
}
EXPORT_SYMBOL(sp_dma0_wait);

void sp_dma0_pause(u32 ch)
{
	sp_dma0.reg->glb_ctrl |= (1 << ch);
}
EXPORT_SYMBOL(sp_dma0_pause);

void sp_dma0_resume(u32 ch)
{
	sp_dma0.reg->glb_ctrl &= ~(1 << ch);
}
EXPORT_SYMBOL(sp_dma0_resume);

void sp_dma0_reset(u32 ch)
{
	u32 m = (1 << (ch + 8));
	sp_dma0.reg->glb_ctrl |= m;
	WAIT(sp_dma0.reg->glb_ctrl & m); // wait reset done
}
EXPORT_SYMBOL(sp_dma0_reset);

#ifdef CONFIG_SP_DMA0_TEST // test & example

#if 0
static void dump_buf(u8 *buf, u32 len)
{
    static char s[] = "       |       \n";
    char ss[52] = "";
    u32 i = 0, j;
    printk("buf:%p, len:%d\n", buf, len);
    while (i < len) {
        j = i & 0x0F;
        sprintf(ss + j * 3, "%02x%c", buf[i], s[j]);
        i++;
        if ((i & 0x0F) == 0) {
            printk(ss);
            ss[0] = 0;
        }
    }
    if (i & 0x0F) {
        printk(ss);
    }
}

#define RESULT \
({ \
	int ret = memcmp(src, dst, size); \
	if (ret) dump_buf(dst, size); \
	ret ? "FAIL" : "PASS"; \
})
#else
#define RESULT	(memcmp(src, dst, size) ? "FAIL" : "PASS")
#endif

#define UG_SLOP		0x4
#define UG_BD_UP	0x200
#define UG_BD_DOWN	0x0
#define UG_DATA_TH	0x2b0
#define UG_CTRL0	((1<<31) | (UG_SLOP<<20) | (UG_BD_UP<<10) | UG_BD_DOWN)

static int test_set(const char *val, const struct kernel_param *kp)
{
	u32 src_pa, dst_pa, size, sz;
	void *src, *dst;
	int i;

	// A -> A
	size   = SZ_128K;
	src_pa = WORKMEM_BASE + SZ_32K; // workmem head 32 KB maybe security
	dst_pa = src_pa + size;
	src    = WORKMEM_VA(src_pa);
	dst    = WORKMEM_VA(dst_pa);

	memset(src, 0xa5, size);
	memset(dst, 0xdd, size);

	sp_dma0_copy(6, dst_pa, src_pa, size, true);
	printk("DMA0 A:%08x -> A:%08x test: %s\n", src_pa, dst_pa, RESULT);

	// A -> B
	dst = dma_alloc_coherent(NULL, size, &dst_pa, GFP_KERNEL);
	memset(dst, 0xee, size);

    sp_dma0_copy(0, dst_pa, src_pa, size, false);
#if 0 // RESET
	sp_dma0_reset(0);
	sp_dma0_copy(0, dst_pa, src_pa, size, true);
#else // PAUSE/RESUME
	sp_dma0_pause(0);
	MSLEEP(1);
	sp_dma0_resume(0);
	sp_dma0_wait(0);
#endif
	printk("DMA0 A:%08x -> B:%08x test (pause/resume): %s\n", src_pa, dst_pa, RESULT);

	// B -> A
	memset(src, 0xff, size);

	sp_dma0_copy(1, src_pa, dst_pa, size, true);
	printk("DMA0 B:%08x -> A:%08x test: %s\n", dst_pa, src_pa, RESULT);

	// A -> B (Urgent)
	sz = SZ_16K;
	memset(dst, 0xdd, 6 * sz);

	// initial urgent config for 6 channels
	for (i = 0; i < 6; i++) {
		sp_dma0.reg->ug[i].cfg0 = UG_CTRL0;
		sp_dma0.reg->ug[i].cfg1 = UG_DATA_TH;
	}

	// start 6 channels to transfer 6*16KB data
	for (i = 0; i < 6; i++) {
		sp_dma0_copy(i, dst_pa + (i * sz), src_pa + (i * sz), sz, false);
	}
	// wait all 6 channels done
	for (i = 0; i < 6; i++) {
		sp_dma0_wait(i);
	}
    printk("DMA0 A:%08x -> B:%08x test (urgent): %s\n", src_pa, dst_pa, RESULT);

	dma_free_coherent(NULL, size, dst, dst_pa);

	return 0;
}

static const struct kernel_param_ops test_ops = {
	.set = test_set,
};
module_param_cb(test, &test_ops, NULL, 0600);
#endif

static irqreturn_t sp_dma0_isr(int irq, void *dev_id)
{
    int i = irq - sp_dma0.irq;
	if (i < 8) {
		early_printk("!DMA0 Ch#%d Job Done!\n", i);
	} else {
		early_printk("!DMA0 Address Expired Error: %08x\n", sp_dma0.reg->glb_sts);
	}

	return IRQ_HANDLED;
}
	
static int sp_dma0_probe(struct platform_device *pdev)
{
	struct sp_dma0_dev *dev = &sp_dma0;
	struct resource *res_mem, *res_irq;
	void __iomem *membase;
	int i = 0;
	int ret = 0;

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem)
		return -ENODEV;

	membase = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR(membase))
		return PTR_ERR(membase);

	res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res_irq) {
		return -ENODEV;
	}

	dev->reg = membase;
	dev->irq = res_irq->start;
	dev->bio = (struct bio_cfg *)(VA_B_REG + 261 * 4096);
	dev->membase = devm_ioremap(&pdev->dev, WORKMEM_BASE, WORKMEM_SIZE); // workmem sram
	//dev->membase = devm_memremap(&pdev->dev, WORKMEM_BASE, WORKMEM_SIZE, MEMREMAP_WB);
	//*(REG *)(VA_A_REG + 83 * 128 + 4) = 0xffffffff; // disable workmem security
	platform_set_drvdata(pdev, dev);

	while (i < DMA0_IRQS) {
		ret = devm_request_irq(&pdev->dev, dev->irq + i, sp_dma0_isr, IRQF_TRIGGER_RISING, "sp_dma0", dev);
		if (ret) {
			return -ENODEV;
		}
		i++;
	}
	TRACE;

	return ret;
}

static const struct of_device_id sp_dma0_of_match[] = {
	{ .compatible = "sunplus,sp-dma0" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_dma0_of_match);

static struct platform_driver sp_dma0_driver = {
	.probe		= sp_dma0_probe,
	.driver		= {
		.name	= "sp_dma0",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_dma0_of_match),
	},
};

module_platform_driver(sp_dma0_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus DMA0 driver");
