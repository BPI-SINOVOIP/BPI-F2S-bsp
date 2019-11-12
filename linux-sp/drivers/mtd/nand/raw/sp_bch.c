#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/uaccess.h>
#include "sp_bch.h"

static struct sp_bch_chip __this;

/*static int get_setbits(uint32_t n)
{
	n = n - ((n >> 1) & 0x55555555);
	n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
	return (((n + (n >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

static int sp_bch_blank(dma_addr_t ecc, int len)
{
	uint8_t *oob = ioremap(ecc, len);
	int mbe = 4;
	int ret = 0;
	int i, n;

	if (oob) {
		ret = 1;
		for (i = 2, n = 0; i < len; ++i) {
			if (oob[i] != 0xff)
				n += get_setbits(oob[i] ^ 0xff);
			if (n > mbe) {
				ret = 0;
				break;
			}
		}
	}

	if (oob)
		iounmap(oob);

	return ret;
}
*/

static int sp_bch_reset(struct sp_bch_chip *chip)
{
	struct sp_bch_regs *regs = chip->regs;
	unsigned long timeout = jiffies + msecs_to_jiffies(50);

	/* reset controller */
	writel(SRR_RESET, &regs->srr);
	while (jiffies < timeout) {
		if (!(readl(&regs->srr) & SRR_RESET))
			break;
		cpu_relax();
	}
	if (jiffies >= timeout) {
		printk(KERN_WARNING "sp_bch: reset timeout\n");
		return -1;
	}

	/* reset interrupts */
	writel(~(IER_FAIL|IER_DONE), &regs->ier);
	writel(SR_DONE|SR_FAIL, &regs->sr);
	writel(ISR_BCH, &regs->isr);

	return 0;
}

static int sp_bch_wait(struct sp_bch_chip *chip)
{
	int ret = 0;
	if (!wait_event_timeout(chip->wq, !chip->busy, HZ/10)) {
		if (chip->busy == 0) {
			return 0;
		}
		ret = -ETIME;
	}
	return ret;
}

static irqreturn_t sp_bch_irq(int irq, void *dev)
{
	struct sp_bch_chip *chip = dev;
	struct sp_bch_regs *regs = chip->regs;
	u32 value;

	chip->busy = 0;

	value = readl(&regs->sr);
	writel(value, &regs->sr);

	value = readl(&regs->isr);
	writel(value, &regs->isr);

	wake_up(&chip->wq);

	return IRQ_HANDLED;
}

static int sunplus_ooblayout_free(struct mtd_info *mtd, int section,
					struct mtd_oob_region *oobregion)
{
	struct nand_chip *nand = mtd_to_nand(mtd);
	struct sp_bch_chip *chip = &__this;

	if (section >= nand->ecc.steps)
		return -ERANGE;

	oobregion->offset = section * chip->pssz;
	oobregion->length = chip->free;

	if (section == 0) {
		/* skip bad block 2 byte flag */
		oobregion->offset += 2;
		oobregion->length -= 2;
	}
	return 0;
}

static int sunplus_ooblayout_ecc(struct mtd_info *mtd, int section,
					struct mtd_oob_region *oobregion)
{
	struct sp_bch_chip *chip = &__this;

	oobregion->offset = section * chip->pssz + chip->free;
	oobregion->length = chip->pssz - chip->free;

	return 0;
}

static const struct mtd_ooblayout_ops sunplus_ooblayout_ops = {
	.ecc = sunplus_ooblayout_ecc,
	.free = sunplus_ooblayout_free,
};

int sp_bch_init(struct mtd_info *mtd, int *parity_sector_sz)
{
	struct sp_bch_chip *chip = &__this;
	struct nand_chip *nand = mtd_to_nand(mtd);
	int oobsz;
	int pgsz;
	int rsvd;  /* Reserved bytes for YAFFS2 */
	int size;  /* BCH data length per sector */
	int bits;  /* BCH strength per sector */
	int nrps;  /* BCH parity sector number */
	int pssz;  /* BCH parity sector size */
	int free;  /* BCH free bytes per sector */

	if (!mtd || !chip)
		BUG();

	chip->mtd = mtd;

	rsvd = 32;
	oobsz = mtd->oobsize;
	pgsz = mtd->writesize;

	/* 1024x60 */
	size = 1024;
	bits = 60;
	nrps = pgsz >> 10;
	chip->cr0 = CR0_CMODE_1024x60 | CR0_NBLKS(nrps);
	if (oobsz >= nrps * 128) {
		chip->cr0 |= CR0_DMODE(0) | CR0_BMODE(5);
		pssz = 128;
		free = 23;
	} else if (oobsz >= nrps * 112) {
		chip->cr0 |= CR0_DMODE(1) | CR0_BMODE(1);
		pssz = 112;
		free = 7;
	} else {
		pssz = 0;
		free = 0;
	}
	if (free * nrps >= rsvd)
		goto ecc_detected;

	/* 1024x40 */
	size = 1024;
	bits = 40;
	nrps = pgsz >> 10;
	chip->cr0 = CR0_CMODE_1024x40 | CR0_NBLKS(nrps);
	if (oobsz >= nrps * 96) {
		chip->cr0 |= CR0_DMODE(0) | CR0_BMODE(6);
		pssz = 96;
		free = 26;
	} else if (oobsz >= nrps * 80) {
		chip->cr0 |= CR0_DMODE(1) | CR0_BMODE(2);
		pssz = 80;
		free = 10;
	} else {
		pssz = 0;
		free = 0;
	}
	if (free * nrps >= rsvd)
		goto ecc_detected;

	/* 1024x24 */
	size = 1024;
	bits = 24;
	nrps = pgsz >> 10;
	chip->cr0 = CR0_CMODE_1024x24 | CR0_NBLKS(nrps);
	if (oobsz >= nrps * 64) {
		chip->cr0 |= CR0_DMODE(0) | CR0_BMODE(5);
		pssz = 64;
		free = 22;
	} else if (oobsz >= nrps * 48) {
		chip->cr0 |= CR0_DMODE(1) | CR0_BMODE(1);
		pssz = 48;
		free = 6;
	} else {
		pssz = 0;
		free = 0;
	}
	if (free * nrps >= rsvd)
		goto ecc_detected;

	/* 1024x16 */
	size = 1024;
	bits = 16;
	nrps = pgsz >> 10;
	chip->cr0 = CR0_CMODE_1024x16 | CR0_NBLKS(nrps);
	if (oobsz >= nrps * 64) {
		chip->cr0 |= CR0_DMODE(0) | CR0_BMODE(6);
		pssz = 64;
		free = 28;
	} else if (oobsz >= nrps * 48) {
		chip->cr0 |= CR0_DMODE(1) | CR0_BMODE(4);
		pssz = 48;
		free = 20;
	} else {
		pssz = 0;
		free = 0;
	}
	if (free * nrps >= rsvd)
		goto ecc_detected;

	/* 512x8 */
	size = 512;
	bits = 8;
	nrps = pgsz >> 9;
	chip->cr0 = CR0_CMODE_512x8 | CR0_NBLKS(nrps);
	if (oobsz >= nrps * 32) {
		chip->cr0 |= CR0_DMODE(0) | CR0_BMODE(4);
		pssz = 32;
		free = 18;
	} else if (oobsz >= nrps * 16) {
		chip->cr0 |= CR0_DMODE(1) | CR0_BMODE(0);
		pssz = 16;
		free = 2;
	} else {
		pssz = 0;
		free = 0;
	}
	if (free * nrps >= rsvd)
		goto ecc_detected;

	/* 512x4 */
	size = 512;
	bits = 4;
	nrps = pgsz >> 9;
	chip->cr0 = CR0_CMODE_512x4 | CR0_NBLKS(nrps);
	if (oobsz >= nrps * 32) {
		chip->cr0 |= CR0_DMODE(0) | CR0_BMODE(6);
		pssz = 32;
		free = 25;
	} else {
		chip->cr0 |= CR0_DMODE(1) | CR0_BMODE(2);
		pssz = 16;
		free = 9;
	}

ecc_detected:
	chip->pssz = pssz;
	chip->free = free;
	nand->ecc.size = size;
	nand->ecc.steps = nrps;
	nand->ecc.strength = bits;
	nand->ecc.bytes = ((12 + (size >> 9)) * bits + 7) >> 3;

	if(parity_sector_sz)
			*parity_sector_sz = pssz;

	/* sanity check */
	if (nand->ecc.steps > MTD_MAX_OOBFREE_ENTRIES_LARGE)
		BUG();

	if (free * nrps < rsvd)
		BUG();

	mtd->ooblayout = &sunplus_ooblayout_ops;

	mtd->oobavail = nand->ecc.steps*free - 2;

	mutex_lock(&chip->lock);
	sp_bch_reset(chip);
	mutex_unlock(&chip->lock);

	return 0;
}
EXPORT_SYMBOL(sp_bch_init);

/*
 * Calculate BCH ecc code
 */
int sp_bch_encode(struct mtd_info *mtd, dma_addr_t buf, dma_addr_t ecc)
{
	struct sp_bch_chip *chip = &__this;
	struct sp_bch_regs *regs = chip->regs;
	int ret;

	if (!mtd)
		BUG();

	if (chip->mtd != mtd)
		sp_bch_init(mtd, NULL);

	mutex_lock(&chip->lock);

	writel(SRR_RESET, &regs->srr);
	writel(buf, &regs->buf);
	writel(ecc, &regs->ecc);

	chip->busy = 1;
	writel(CR0_START | CR0_ENCODE | chip->cr0, &regs->cr0);
	ret = sp_bch_wait(chip);
	if (ret)
		printk(KERN_WARNING "sp_bch: encode timeout\n");

	mutex_unlock(&chip->lock);

	return ret;
}
EXPORT_SYMBOL(sp_bch_encode);

/*
 * Detect and correct bit errors
 */
int sp_bch_decode(struct mtd_info *mtd, dma_addr_t buf, dma_addr_t ecc)
{
	struct sp_bch_chip *chip = &__this;
	struct sp_bch_regs *regs = chip->regs;
	uint32_t status;
	int ret;
	if (!mtd)
		BUG();

	if (chip->mtd != mtd)
		sp_bch_init(mtd, NULL);

	mutex_lock(&chip->lock);

	writel(SRR_RESET, &regs->srr);
	writel(buf, &regs->buf);
	writel(ecc, &regs->ecc);

	chip->busy = 1;
	writel(CR0_START | CR0_DECODE | chip->cr0, &regs->cr0);
	ret = sp_bch_wait(chip);
	status = readl(&regs->sr);
	if (ret) {
		printk(KERN_WARNING "sp_bch: decode timeout\n");
	} else if (readl(&regs->fsr) != 0) {
		if((status & SR_BLANK_FF)) {
			//printk("sp_bch: decode all FF!\n");
			ret = 0;
		} else {
			printk(KERN_WARNING "sp_bch: decode failed.\n");
			mtd->ecc_stats.failed += SR_ERR_BITS(status);
			ret = -1;
		}
		sp_bch_reset(chip);
	} else {
		mtd->ecc_stats.corrected += SR_ERR_BITS(status);
	}

	mutex_unlock(&chip->lock);

	return ret;
}
EXPORT_SYMBOL(sp_bch_decode);

int sp_autobch_config(struct mtd_info *mtd, void *buf, void *ecc, int enc)
{
	struct sp_bch_chip *chip = &__this;
	struct sp_bch_regs *regs = chip->regs;
	int value;

	mutex_lock(&chip->lock);

	writel(SRR_RESET, &regs->srr);
	writel((uint32_t) buf, &regs->buf);
	writel((uint32_t) ecc, &regs->ecc);

	value = chip->cr0
		| (enc ? CR0_ENCODE : CR0_DECODE)
		| CR0_AUTOSTART;
	writel(value, &regs->cr0);

	mutex_unlock(&chip->lock);

	return 0;
}
EXPORT_SYMBOL(sp_autobch_config);


int sp_autobch_result(struct mtd_info *mtd)
{
	struct sp_bch_chip *chip = &__this;
	struct sp_bch_regs *regs = chip->regs;
	int ret = 0;
	int status;

	status = readl(&regs->sr);
	if (readl(&regs->fsr) != 0) {
		if((status & SR_BLANK_FF)) {
			//printk("decode all FF!\n");
			ret = 0;
		} else {
			printk(KERN_WARNING "sp_bch: decode failed.\n");
			mtd->ecc_stats.failed += SR_ERR_BITS(status);
			ret = -1;
		}
		sp_bch_reset(chip);
	} else {
		mtd->ecc_stats.corrected += SR_ERR_BITS(status);
	}

	return ret;
}
EXPORT_SYMBOL(sp_autobch_result);

#ifdef CONFIG_SPBCH_SUPPORT_IOCTL
static long sp_bch_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct sp_bch_chip *chip = &__this;
	struct sp_bch_regs *regs = chip->regs;
	void __user *argp = (void __user *)arg;
	unsigned long size;
	long ret = 0;
	void *buf;
	struct sp_bch_req *req;
	dma_addr_t buf_phys, ecc_phys;

	size = (cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT;
	if (cmd & IOC_IN) {
		if (!access_ok(VERIFY_READ, argp, size))
			return -EFAULT;
	}
	if (cmd & IOC_OUT) {
		if (!access_ok(VERIFY_WRITE, argp, size))
			return -EFAULT;
	}

	buf = kmalloc(sizeof(*req) + 32, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	/* make sure it's 32 bytes aligned */
	req = (void *)(((unsigned)buf + 31) & 0xffffffe0);

	switch (cmd) {
	case SP_BCH_IOC1K60ENC:
		if (copy_from_user(req, argp, sizeof(*req))) {
			ret = -EFAULT;
			break;
		}

		mutex_lock(&chip->lock);

		buf_phys = dma_map_single(chip->dev, req->buf, 1024, DMA_TO_DEVICE);
		ecc_phys = dma_map_single(chip->dev, req->ecc, 128, DMA_BIDIRECTIONAL);

		writel(buf_phys, &regs->buf);
		writel(ecc_phys, &regs->ecc);

		chip->busy = 1;
		writel(CR0_START | CR0_ENCODE | CR0_CMODE_1024x60, &regs->cr0);
		if (sp_bch_wait(chip)) {
			printk(KERN_ERR "sp_bch: 1k60 encode timeout\n");
			ret = -EFAULT;
		}

		dma_unmap_single(chip->dev, buf_phys, 1024, DMA_TO_DEVICE);
		dma_unmap_single(chip->dev, ecc_phys, 128, DMA_BIDIRECTIONAL);

		mutex_unlock(&chip->lock);

		if (copy_to_user(argp, req, sizeof(*req)))
			ret = -EFAULT;
		break;

	case SP_BCH_IOC1K60DEC:
		if (copy_from_user(req, argp, sizeof(*req))) {
			ret = -EFAULT;
			break;
		}

		mutex_lock(&chip->lock);

		buf_phys = dma_map_single(chip->dev, req->buf, 1024, DMA_BIDIRECTIONAL);
		ecc_phys = dma_map_single(chip->dev, req->ecc, 128, DMA_TO_DEVICE);

		writel(buf_phys, &regs->buf);
		writel(ecc_phys, &regs->ecc);

		chip->busy = 1;
		writel(CR0_START | CR0_DECODE | CR0_CMODE_1024x60, &regs->cr0);
		if (sp_bch_wait(chip)) {
			printk(KERN_ERR "sp_bch: 1k60 decode timeout\n");
			ret = -EFAULT;
		}

		dma_unmap_single(chip->dev, buf_phys, 1024, DMA_BIDIRECTIONAL);
		dma_unmap_single(chip->dev, ecc_phys, 128, DMA_TO_DEVICE);

		mutex_unlock(&chip->lock);

		if (copy_to_user(argp, req, sizeof(*req)))
			ret = -EFAULT;
		break;

	default:
		ret = -ENOTTY;
		break;
	}

	kfree(buf);

	return ret;
}

static struct file_operations sp_bch_fops = {
	.owner          = THIS_MODULE,
	.llseek         = no_llseek,
	.compat_ioctl   = sp_bch_ioctl,
	.unlocked_ioctl = sp_bch_ioctl,
};

static struct miscdevice sp_bch_dev = {
	.name = "sunplus,sp7021-bch",
	.fops = &sp_bch_fops,
};
#endif

static int sp_bch_probe(struct platform_device *pdev)
{
	struct sp_bch_chip *chip = &__this;
	struct resource *res_mem;
	struct resource *res_irq;
	struct clk *clk;
	int ret = 0;

	printk(KERN_INFO "%s in\n", __FUNCTION__);

	memset(chip, 0, sizeof(*chip));
	mutex_init(&chip->lock);
	init_waitqueue_head(&chip->wq);

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_mem) {
		printk(KERN_ERR "sp_bch: get memory resource fail\n");
		ret = -ENXIO;
		goto err;
	}

	chip->regs = devm_ioremap_resource(&pdev->dev, res_mem);
	if (IS_ERR(chip->regs)) {
		printk(KERN_ERR "sp_bch: memory remap fail!\n");
		ret = PTR_ERR(chip->regs);
		chip->regs = NULL;
		goto err;
	}

	res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res_irq <= 0) {
		printk(KERN_ERR "sp_bch: get irq resource fail!\n");
		ret = -ENXIO;
		goto err;
	}

	clk = devm_clk_get(&pdev->dev, NULL);
	if (!IS_ERR(clk)) {
		ret = clk_prepare(clk);
		if (ret) {
			printk(KERN_ERR "sp_bch: clk_prepare fail!\n");
			goto err;
		}
		ret = clk_enable(clk);
		if (ret) {
			printk(KERN_ERR "sp_bch: clk_enable fail!\n");
			clk_unprepare(clk);
			goto err;
		}
		chip->clk = clk;
	}

	if (sp_bch_reset(chip)) {
		printk(KERN_ERR "sp_bch: reset bch hw fail!\n");
		ret = -ENXIO;
		goto err;
	}

	ret = request_irq(res_irq->start,sp_bch_irq,IRQF_SHARED,"sp_bch",chip);
	if(ret) {
		printk(KERN_ERR"sp_bch: request IRQ(%d) fail\n",res_irq->start);
		goto err;
	}

	chip->irq = res_irq->start;
	chip->dev = &pdev->dev;

	platform_set_drvdata(pdev, chip);

	#ifdef CONFIG_SPBCH_SUPPORT_IOCTL
	misc_register(&sp_bch_dev);
	#endif

	return 0;

err:
	if (chip->clk) {
		clk_disable(chip->clk);
		clk_unprepare(chip->clk);
	}

	if (chip->irq)
		free_irq(chip->irq, chip);

	if (chip->regs)
		iounmap(chip->regs);

	mutex_destroy(&chip->lock);

	return ret;
}

static int sp_bch_remove(struct platform_device *pdev)
{
	struct sp_bch_chip *chip = platform_get_drvdata(pdev);

	#ifdef CONFIG_SPBCH_SUPPORT_IOCTL
	misc_deregister(&sp_bch_dev);
	#endif

	if (!chip)
		BUG();

	wake_up(&chip->wq);
	mutex_destroy(&chip->lock);
	platform_set_drvdata(pdev, NULL);

	if (chip->clk) {
		clk_disable(chip->clk);
		clk_unprepare(chip->clk);
	}

	if (chip->irq)
		free_irq(chip->irq, chip);

	if (chip->regs)
		iounmap(chip->regs);

	return 0;
}

int sp_bch_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

int sp_bch_resume(struct platform_device *pdev)
{
	return 0;
}

#ifndef CONFIG_SPBCH_DEV_IN_DTS
static struct resource sp_bch_res[] = {
	{
		.start = SP_BCH_REG,
		.end   = SP_BCH_REG + sizeof(struct sp_bch_regs),
		.flags = IORESOURCE_MEM,
	},
	{
		.start = SP_BCH_IRQ,
		.end   = SP_BCH_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device sp_bch_device = {
	.name  = "sunplus,sp7021-bch",
	.id    = 0,
	.num_resources = ARRAY_SIZE(sp_bch_res),
	.resource  = sp_bch_res,
};
#endif

static const struct of_device_id sp_bch_of_match[] = {
	{ .compatible = "sunplus,sp7021-bch" },
	{},
};
MODULE_DEVICE_TABLE(of, sp_bch_of_match);

static struct platform_driver sp_bch_driver = {
	.probe = sp_bch_probe,
	.remove = sp_bch_remove,
	.suspend = sp_bch_suspend,
	.resume = sp_bch_resume,
	.driver = {
		.name = "sunplus,sp_bch",
		.owner = THIS_MODULE,
		.of_match_table = sp_bch_of_match,
	},
};

static int __init sp_bch_module_init(void)
{
	#ifndef CONFIG_SPBCH_DEV_IN_DTS
	platform_device_register(&sp_bch_device);
	#endif
	platform_driver_register(&sp_bch_driver);
	return 0;
}

static void __exit sp_bch_module_exit(void)
{
	platform_driver_unregister(&sp_bch_driver);
	#ifndef CONFIG_SPBCH_DEV_IN_DTS
	platform_device_unregister(&sp_bch_device);
	#endif
}

arch_initcall(sp_bch_module_init);  //module_init(sp_bch_module_init);
module_exit(sp_bch_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sunplus BCH controller");

