/*
 * STMicroelectronics TCG TPM I2C Linux driver 
 * Copyright (C) 2018 STMicroelectronics
 *
 * Authors:
 *
 * Christophe Ricard <christophe-h.ricard@st.com>
 * Benoit Houyère	<benoit.houyere@st.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/init.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/acpi.h>
#include <linux/freezer.h>
#include <linux/crc-ccitt.h>

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/tpm.h>
#include "tpm.h"
#include "tpm_tis_core.h"

#define TPM_LOC_SEL						0x04
#define TPM_I2C_INTERFACE_CAPABILITY	0x30
#define TPM_I2C_DEVICE_ADDRESS			0x38
#define TPM_DATA_CSUM_ENABLE			0x40
#define TPM_DATA_CSUM					0x44
#define TPM_I2C_DID_VID					0x48
#define TPM_I2C_RID						0x4C

#define TPM_I2C_DEFAULT_GUARD_TIME	0xFA

enum tpm_tis_i2c_operation {
	TPM_I2C_NONE,
	TPM_I2C_RECV,
	TPM_I2C_SEND,
};

#define TPM_I2C_DEVADRCHANGE(x)		((0x18000000 & x) >> 27)
#define TPM_I2C_READ_READ(x)		((0x00100000 & x) >> 20)
#define TPM_I2C_READ_WRITE(x)		((0x00080000 & x) >> 19)
#define TPM_I2C_WRITE_READ(x)		((0x00040000 & x) >> 18)
#define TPM_I2C_WRITE_WRITE(x)		((0x00020000 & x) >> 17)
#define TPM_I2C_GUARD_TIME(x)		((0x0001FE00 & x) >> 9)

struct tpm_tis_i2c_phy {
	struct i2c_client *client;
	struct tpm_tis_data priv;
	u8 buf[TPM_BUFSIZE +1];
	u8 last_i2c_ops;

	struct timer_list guard_timer;
	struct mutex phy_lock;

	u8 data_csum;
	bool devadrchange;
	bool read_read;
	bool read_write;
	bool write_read;
	bool write_write;
	u8 guard_time;
};

static inline struct tpm_tis_i2c_phy *to_tpm_tis_i2c_phy(struct tpm_tis_data *data)
{
	return container_of(data, struct tpm_tis_i2c_phy, priv);
}

static int tpm_tis_i2c_ptp_register_mapper(u32 addr, u8 *i2c_reg)
{
	*i2c_reg = (u8)(0x000000ff & addr);

	switch (addr) {
	case TPM_ACCESS(0):
		*i2c_reg = TPM_LOC_SEL;
		break;
	case TPM_LOC_SEL:
		*i2c_reg = TPM_ACCESS(0);
		break;
	case TPM_DID_VID(0):
		*i2c_reg = TPM_I2C_DID_VID;
		break;
	case TPM_RID(0):
		*i2c_reg = TPM_I2C_RID;
		break;
	case TPM_INTF_CAPS(0):
		*i2c_reg = TPM_I2C_INTERFACE_CAPABILITY;
		break;
	case TPM_INT_VECTOR(0):
		return -1;
	}

	return 0;
}

#ifdef BPI
static void tpm_tis_i2c_guard_time_timeout(unsigned long data)
{
	struct tpm_tis_i2c_phy *phy = (struct tpm_tis_i2c_phy *)data;

	/* GUARD_TIME expired */
	phy->last_i2c_ops = TPM_I2C_NONE;
}
#else
static void tpm_tis_i2c_guard_time_timeout(struct timer_list *t)
{
	struct tpm_tis_i2c_phy *phy = from_timer(phy, t, guard_timer);

	/* GUARD_TIME expired */
	phy->last_i2c_ops = TPM_I2C_NONE;
}
#endif

static void tpm_tis_i2c_sleep_guard_time(struct tpm_tis_i2c_phy *phy,
					 u8 i2c_operation)
{					
	del_timer_sync(&phy->guard_timer);
	switch (i2c_operation) {
	case TPM_I2C_RECV:
		switch (phy->last_i2c_ops) {
		case TPM_I2C_RECV:
		if (phy->read_read)
			udelay(phy->guard_time);
		break;
		case TPM_I2C_SEND:
		if (phy->write_read)
			udelay(phy->guard_time);
		break;
		}
	break;
	case TPM_I2C_SEND:
		switch (phy->last_i2c_ops) {
		case TPM_I2C_RECV:
		if (phy->read_write)
			udelay(phy->guard_time);
		break;
		case TPM_I2C_SEND:
		if (phy->write_write)
			udelay(phy->guard_time);
		break;
		}
	break;
	}
	phy->last_i2c_ops = i2c_operation;

}

static int tpm_tis_i2c_read_bytes(struct tpm_tis_data *data, u32 addr,
				  u16 size, u8 *result)
{
	struct tpm_tis_i2c_phy *phy = to_tpm_tis_i2c_phy(data);
	int i=0;
	int ret = 0;
	u8 i2c_reg;
	mutex_lock(&phy->phy_lock);
	ret = tpm_tis_i2c_ptp_register_mapper(addr, &i2c_reg);
	if (ret < 0) {
		/* If we don't have any register equivalence in i2c
		 * ignore the sequence.
		 */
		ret = size;
		goto exit;
	}
	ret = -1;

	for (i = 0; (i < TPM_RETRY) && (ret < 0); i++) {
		tpm_tis_i2c_sleep_guard_time(phy, TPM_I2C_SEND);
		ret = i2c_master_send(phy->client, &i2c_reg, 1);
		mod_timer(&phy->guard_timer, phy->guard_time);
	}

	if (ret < 0)
	{
		sprintf(phy->buf, " read 1 %x\n", phy->guard_time);
		goto exit;
	}
	ret = -1;
	for (i = 0; (i < TPM_RETRY) && (ret < 0); i++) {
		tpm_tis_i2c_sleep_guard_time(phy, TPM_I2C_RECV);
		ret = i2c_master_recv(phy->client, result, size);
		mod_timer(&phy->guard_timer, phy->guard_time);
	}
	if (ret < 0)
	{
		sprintf(phy->buf, " read 2 %x\n", phy->guard_time);
		goto exit;
	}
exit:
	mutex_unlock(&phy->phy_lock);
	return ret;
}

static int tpm_tis_i2c_write_bytes(struct tpm_tis_data *data, u32 addr, 
				   u16 size, const u8 *value)
{
	struct tpm_tis_i2c_phy *phy = to_tpm_tis_i2c_phy(data);
	int ret = 0;	
	int i;
	u8 i2c_reg;

	mutex_lock(&phy->phy_lock);
	ret = tpm_tis_i2c_ptp_register_mapper(addr, &i2c_reg);
	if (ret < 0) {
		/* If we don't have any register equivalence in i2c
		 * ignore the sequence.
		 */
		ret = size;
		goto exit;
	}

	ret = -1;
	phy->buf[0] = i2c_reg;
	memcpy(phy->buf + 1, value, size);

	for (i = 0; (i < TPM_RETRY) && ((ret < 0 || ret < size + 1)); i++) {
		tpm_tis_i2c_sleep_guard_time(phy, TPM_I2C_SEND);
		ret = i2c_master_send(phy->client, phy->buf, size + 1);
		mod_timer(&phy->guard_timer, phy->guard_time);
	}
		if (ret < 0)
	{
		sprintf(phy->buf, " WRITE 1 %x\n", phy->guard_time);
		goto exit;
	}
exit:
	mutex_unlock(&phy->phy_lock);
	return ret;
}

static int tpm_tis_i2c_read16(struct tpm_tis_data *data, u32 addr, u16 *result)
{
	int rc;

	rc = data->phy_ops->read_bytes(data, addr, sizeof(u16), (u8 *)result);
	if (!rc)
		*result = le16_to_cpu(*result);
	return rc;
}

static int tpm_tis_i2c_read32(struct tpm_tis_data *data, u32 addr, u32 *result)
{
	int rc;

	rc = data->phy_ops->read_bytes(data, addr, sizeof(u32), (u8 *)result);
	if (!rc)
		*result = le32_to_cpu(*result);
	return rc;
}

static int tpm_tis_i2c_write32(struct tpm_tis_data *data, u32 addr, u32 value)
{
	value = cpu_to_le32(value);
	return data->phy_ops->write_bytes(data, addr, sizeof(u32),
					   (u8 *)&value);
}

static bool tpm_tis_i2c_check_data(struct tpm_tis_data *data, u8 *buf, size_t len)
{
	struct tpm_tis_i2c_phy *phy = to_tpm_tis_i2c_phy(data);
	u16 crc, crc_tpm;
	if (phy->data_csum==1) {
		crc = crc_ccitt(0x0000, buf, len);

		crc_tpm = tpm_tis_read16(data, TPM_DATA_CSUM, &crc_tpm);
		crc_tpm = be16_to_cpu(crc_tpm);

		return crc == crc_tpm;
	}
	return true;
}

static const struct tpm_tis_phy_ops tpm_tis = {
	.read_bytes = tpm_tis_i2c_read_bytes,
	.write_bytes = tpm_tis_i2c_write_bytes,
	.read16 = tpm_tis_i2c_read16,
	.read32 = tpm_tis_i2c_read32,
	.write32 = tpm_tis_i2c_write32,
};

static SIMPLE_DEV_PM_OPS(tpm_tis_i2c_pm, tpm_pm_suspend, tpm_tis_resume);

static ssize_t i2c_addr_show(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	if (client)
		return sprintf(buf, "0x%.2x\n", client->addr);

	return 0;
}

static ssize_t i2c_addr_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct tpm_tis_data *data = dev_get_drvdata(dev);
	struct tpm_tis_i2c_phy *phy;
	long new_addr;
	u16 cur_addr;
	int ret = 0;
	if (!data)
		goto exit;

	phy = to_tpm_tis_i2c_phy(data);
	if (!phy || !phy->client || !phy->devadrchange)
		goto exit;

	/* Base string automatically detected */
	ret = kstrtol(buf, 0, &new_addr);
	if (ret < 0)
		goto exit;

	ret = tpm_tis_i2c_write32(data, TPM_I2C_DEVICE_ADDRESS,(u32) new_addr);
	if (ret < 0)
		goto exit;

	tpm_tis_i2c_read16(data, TPM_I2C_DEVICE_ADDRESS,&cur_addr);
	if (cur_addr == new_addr) {
		phy->client->addr = new_addr & 0x00ff;
		return count;
	}

	return -EINVAL;
exit:
	return ret;
}
static DEVICE_ATTR_RW(i2c_addr);

static ssize_t csum_state_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct tpm_tis_data *data = dev_get_drvdata(dev);
	struct tpm_tis_i2c_phy *phy;

	if (!data)
		goto exit;

	phy = to_tpm_tis_i2c_phy(data);
	if (!phy || !phy->client)
		goto exit;

	return sprintf(buf, "%x\n", phy->data_csum);
exit:
	return 0;
}

static ssize_t csum_state_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct tpm_tis_data *data = dev_get_drvdata(dev);
	struct tpm_tis_i2c_phy *phy;
	long new_state;
	u8 cur_state;
	int ret = 0;

	if (!data)
		goto exit;

	phy = to_tpm_tis_i2c_phy(data);
	if (!phy || !phy->client)
		goto exit;

	ret = kstrtol(buf, 2, &new_state);
	if (ret < 0)
		goto exit;

	ret = tpm_tis_i2c_write32(data, TPM_DATA_CSUM_ENABLE, new_state);
	if (ret < 0)
		goto exit;

	tpm_tis_read_bytes(data, TPM_DATA_CSUM_ENABLE,sizeof(u16),&cur_state);
	if (new_state == cur_state) {
		phy->data_csum = cur_state;
		return count;
	}

	return -EINVAL;
exit:
	return ret;
}
static DEVICE_ATTR_RW(csum_state);

static struct attribute *tpm_tis_i2c_attrs[] = {
	&dev_attr_i2c_addr.attr,
	&dev_attr_csum_state.attr,
	NULL,
};

static struct attribute_group tpm_tis_i2c_attr_group = {
	.attrs = tpm_tis_i2c_attrs,
};

static int tpm_tis_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct tpm_tis_i2c_phy *phy;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "client not i2c capable\n");
		return -ENODEV;
	}

	phy = devm_kzalloc(&client->dev, sizeof(struct tpm_tis_i2c_phy),
			   GFP_KERNEL);

	if (!phy)
		{ dev_err(&client->dev,"%s:devm_kzalloc failed.\n",
				__func__);
		return -ENOMEM;
		}

	phy->client = client;

	mutex_init(&phy->phy_lock);

	phy->guard_time = TPM_I2C_DEFAULT_GUARD_TIME;
	
	phy->read_read = false;
	phy->read_write = false;
	phy->write_read = false;
	phy->write_write = false;

	/* initialize timer */
#ifdef BPI
	init_timer(&phy->guard_timer);
	phy->guard_timer.data = (unsigned long)phy;
	phy->guard_timer.function = tpm_tis_i2c_guard_time_timeout;
#else
	phy->guard_timer.function = tpm_tis_i2c_guard_time_timeout;
	timer_setup(&phy->guard_timer, tpm_tis_i2c_guard_time_timeout, 0);
#endif

	
	return tpm_tis_core_init(&client->dev, &phy->priv, -1, &tpm_tis,
				 NULL);


}

static int tpm_tis_i2c_remove(struct i2c_client *client)
{
	
	struct tpm_chip *data = i2c_get_clientdata(client);
	tpm_chip_unregister(data);
	return 0;
}

static const struct i2c_device_id tpm_tis_i2c_id[] = {
	{"tpm_tis_i2c", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, tpm_tis_i2c_id);

static const struct of_device_id of_tis_i2c_match[] = {
	{ .compatible = "st,st33htpm-i2c", },
	{ .compatible = "tcg,tpm_tis-i2c", },
	{}
};
MODULE_DEVICE_TABLE(of, of_tis_i2c_match);

static const struct acpi_device_id acpi_tis_i2c_match[] = {
	{"SMO0768", 0},
	{}
};
MODULE_DEVICE_TABLE(acpi, acpi_tis_i2c_match);

static struct i2c_driver tpm_tis_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "tpm_tis_i2c",
		.pm = &tpm_tis_i2c_pm,
		.of_match_table = of_match_ptr(of_tis_i2c_match),
		.acpi_match_table = ACPI_PTR(acpi_tis_i2c_match),
	},
	.probe = tpm_tis_i2c_probe,
	.remove = tpm_tis_i2c_remove,
	.id_table = tpm_tis_i2c_id,
};

module_i2c_driver(tpm_tis_i2c_driver);

MODULE_DESCRIPTION("TPM Driver for native I2C access");
MODULE_LICENSE("GPL");
