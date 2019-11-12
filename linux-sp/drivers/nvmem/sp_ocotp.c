/*
 * The OCOTP driver for Sunplus QAC628
 *
 * Copyright (C) 2019 Sunplus Technology Inc., All rights reseerved.
 *
 * Author: Sunplus
 *
 * This program is free software; you can redistribute is and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/version.h>

/*
 * QAC628 OTP memory contains 8 banks with 4 32-bit words. Bank 0 starts
 * at offset 0 from the base.
 *
 */
#define QAC628_OTP_NUM_BANKS            8
#define QAC628_OTP_WORDS_PER_BANK       4
#define QAC628_OTP_WORD_SIZE            sizeof(u32)
#define QAC628_OTP_SIZE                 (QAC628_OTP_NUM_BANKS * \
							QAC628_OTP_WORDS_PER_BANK * \
							QAC628_OTP_WORD_SIZE)
#define QAC628_OTP_BIT_ADDR_OF_BANK     (8 * QAC628_OTP_WORD_SIZE * \
							QAC628_OTP_WORDS_PER_BANK)

#define OTP_READ_TIMEOUT                20000

/* HB_GPIO register map */
#define ADDRESS_8_DATA                  0x20
#define ADDRESS_9_DATA                  0x24
#define ADDRESS_10_DATA                 0x28
#define ADDRESS_11_DATA                 0x2C

/* OTPRX register map */
#define OTP_PROGRAM_CONTROL             0x0C
#define PIO_MODE                        0x07

#define OTP_PROGRAM_ADDRESS             0x10
#define PROG_OTP_ADDR                   0x1FFF

#define OTP_PROGRAM_PGENB               0x20
#define PIO_PGENB                       0x01

#define OTP_PROGRAM_ENABLE              0x24
#define PIO_WR                          0x01

#define OTP_PROGRAM_VDD2P5              0x28
#define PROGRAM_OTP_DATA                0xFF00
#define PROGRAM_OTP_DATA_SHIFT          8
#define REG25_PD_MODE_SEL               0x10
#define REG25_POWER_SOURCE_SEL          0x02
#define OTP2REG_PD_N_P                  0x01

#define OTP_PROGRAM_STATE               0x2C
#define OTPRSV_CMD3                     0xE0
#define OTPRSV_CMD3_SHIFT               5
#define TSMC_OTP_STATE                  0x1F

#define OTP_CONTROL                     0x44
#define PROG_OTP_PERIOD                 0xFFE0
#define PROG_OTP_PERIOD_SHIFT           5
#define OTP_EN_R                        0x01

#define OTP_CONTROL2                    0x48
#define OTP_RD_PERIOD                   0xFF00
#define OTP_RD_PERIOD_SHIFT             8
#define OTP_READ                        0x04

#define OTP_STATUS                      0x4C
#define OTP_READ_DONE                   0x10

#define OTP_READ_ADDRESS                0x50
#define RD_OTP_ADDRESS                  0x1F

enum base_type {
	HB_GPIO,
	OTPRX,
	BASEMAX,
};

typedef struct {
	struct device *dev;
	void __iomem *base[BASEMAX];
	struct clk *clk;
	struct nvmem_config *config;
} sp_otp_data_t;

static int sp_otp_wait( void __iomem *_base)
{
	int timeout = OTP_READ_TIMEOUT;
	unsigned int status;

	do {
		udelay(10);
		if (timeout-- == 0) {
			return -ETIMEDOUT;
		}

		status = readl( _base + OTP_STATUS);
	} while((status & OTP_READ_DONE) != OTP_READ_DONE);

	return 0;
}

int sp_otp_read_real( sp_otp_data_t *_otp, int addr, char *value)
{
	unsigned int addr_data;
	unsigned int byte_shift;
	int ret = 0;

	addr_data = addr % (QAC628_OTP_WORD_SIZE * QAC628_OTP_WORDS_PER_BANK);
	addr_data = addr_data / QAC628_OTP_WORD_SIZE;

	byte_shift = addr % (QAC628_OTP_WORD_SIZE * QAC628_OTP_WORDS_PER_BANK);
	byte_shift = byte_shift % QAC628_OTP_WORD_SIZE;

	addr = addr / (QAC628_OTP_WORD_SIZE * QAC628_OTP_WORDS_PER_BANK);
	addr = addr * QAC628_OTP_BIT_ADDR_OF_BANK;

	writel(0x0, _otp->base[OTPRX] + OTP_STATUS);
	writel(addr, _otp->base[OTPRX] + OTP_READ_ADDRESS);
	writel(0x1E04, _otp->base[OTPRX] + OTP_CONTROL2);

	ret = sp_otp_wait( _otp->base[OTPRX]);
	if (ret < 0) {
		return ret;
	}

	*value = (readl( _otp->base[HB_GPIO] + ADDRESS_8_DATA + addr_data *
			QAC628_OTP_WORD_SIZE) >> (8 * byte_shift)) & 0xFF;

	return ret;
}

static int sp_ocotp_read( void *_c, unsigned int _off, void *_v, size_t _l)
{
	sp_otp_data_t *otp = _c;
	unsigned int addr;
	char *buf = _v;
	char value[ 4];
	int ret;

	dev_dbg( otp->dev, "OTP read %u bytes at %u", _l, _off);

	if (( _off >= QAC628_OTP_SIZE) || ( _l == 0) || (( _off + _l) > 128)) {
		return -EINVAL;
	}

	ret = clk_enable( otp->clk);
	if (ret) {
		return ret;
	}

	*buf = 0;
	for ( addr = _off; addr < ( _off + _l); addr++) {
		ret = sp_otp_read_real( otp, addr, value);
		if (ret < 0) {
			dev_err( otp->dev, "OTP read fail:%d at %d", ret, addr);
			goto disable_clk;
		}

		*buf++ = *value;
	}

disable_clk:
	clk_disable( otp->clk);
	dev_dbg( otp->dev, "OTP read complete");
	return ret;
}

static struct nvmem_config sp_ocotp_nvmem_config = {
	.name = "sp-ocotp",
	.read_only = true,
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
	.type = NVMEM_TYPE_OTP,
#endif
	.word_size = 1,
	.size = QAC628_OTP_SIZE,
	.stride = 1,
	.reg_read = sp_ocotp_read,
	.owner = THIS_MODULE,
};

typedef struct {
	int size;
} sp_otp_vX_t;

static const sp_otp_vX_t  sp_otp_v0 = {
	.size = QAC628_OTP_SIZE,
};

static int sp_ocotp_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	const sp_otp_vX_t *sp_otp_vX = NULL;
	struct device *dev = &( pdev->dev);
	struct nvmem_device *nvmem;
	sp_otp_data_t *otp;
	struct resource *res;
	int ret;

	match = of_match_device( dev->driver->of_match_table, dev);
	if ( match && match->data) {
		sp_otp_vX = match->data;
		// may be used to choose the parameters
	} else {
		dev_err( dev, "OTP vX does not match");
	}

	otp = devm_kzalloc( dev, sizeof( *otp), GFP_KERNEL);
	if (!otp) {
		return -ENOMEM;
	}
	otp->dev = dev;
	otp->config = &sp_ocotp_nvmem_config;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "hb_gpio");
	otp->base[HB_GPIO] = devm_ioremap_resource(dev, res);
	if (IS_ERR( otp->base[HB_GPIO])) {
		return PTR_ERR( otp->base[HB_GPIO]);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "otprx");
	otp->base[OTPRX] = devm_ioremap_resource(dev, res);
	if (IS_ERR( otp->base[OTPRX])) {
		return PTR_ERR( otp->base[OTPRX]);
	}

	otp->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR( otp->clk)) {
		return PTR_ERR( otp->clk);
	}

	ret = clk_prepare(otp->clk);
	if (ret < 0) {
		dev_err( dev, "failed to prepare clk: %d\n", ret);
		return ret;
	}
	clk_enable(otp->clk);

	sp_ocotp_nvmem_config.priv = otp;
	sp_ocotp_nvmem_config.dev = dev;

	// devm_* >= 4.15 kernel
	// nvmem = devm_nvmem_register( dev, &sp_ocotp_nvmem_config);
	nvmem = nvmem_register( &sp_ocotp_nvmem_config);
	if ( IS_ERR( nvmem)) {
		dev_err( dev, "error registering nvmem config\n");
		return PTR_ERR( nvmem);
	}

	platform_set_drvdata( pdev, nvmem);

	dev_dbg( dev, "clk:%ld banks:%d x wpb:%d x wsize:%d = %d",
		clk_get_rate( otp->clk),
		QAC628_OTP_NUM_BANKS, QAC628_OTP_WORDS_PER_BANK,
		QAC628_OTP_WORD_SIZE, QAC628_OTP_SIZE);
	dev_info( dev, "by SunPlus (C) 2019");

	return 0;
}

static int sp_ocotp_remove(struct platform_device *pdev)
{
	// disbale for devm_*
	struct nvmem_device *nvmem = platform_get_drvdata( pdev);
	return nvmem_unregister( nvmem);
}

static const struct of_device_id sp_ocotp_dt_ids[] = {
	{ .compatible = "sunplus,sp7021-ocotp", .data = &sp_otp_v0  },
	{ }
};
MODULE_DEVICE_TABLE(of, sp_ocotp_dt_ids);

static struct platform_driver sp_otp_driver = {
	.probe     = sp_ocotp_probe,
	.remove    = sp_ocotp_remove,
	.driver    = {
		.name           = "sunplus,sp7021-ocotp",
		.of_match_table = sp_ocotp_dt_ids,
	},
};
static int __init sp_otp_drv_new( void) {
	return platform_driver_register( &sp_otp_driver);
}
subsys_initcall( sp_otp_drv_new);

static void __exit sp_otp_drv_del( void) {
	platform_driver_unregister( &sp_otp_driver);
}
module_exit(sp_otp_drv_del);

MODULE_DESCRIPTION("Sunplus OCOTP driver");
MODULE_LICENSE("GPL v2");
