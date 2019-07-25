/*
 * (C) Copyright 2019
 * Author: Wells Lu, wells.lu@sunplus.com
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 * Ethernet driver for Sunplus L2SW (Q628)
 *
*/

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <fdt_support.h>
#include <linux/err.h>
#include <malloc.h>
#include <miiphy.h>
#include <net.h>
#include <asm/cache.h>
#include "sunplus_l2sw.h"


extern int read_otp_data(int addr, char *value);


static struct l2sw_reg* ls2w_reg_base = NULL;
static struct moon5_reg* moon5_reg_base = NULL;

#if 0
static void print_packet(char *p, int len)
{
	static u32 tftp_port = 0xfffff;
	char buf[120], *packet_t;
	u32 LenType;
	int i;

	i = snprintf(buf, sizeof(buf), "MAC: DA=%02x:%02x:%02x:%02x:%02x:%02x, "
		"SA=%02x:%02x:%02x:%02x:%02x:%02x, ",
		(u32)p[0], (u32)p[1], (u32)p[2], (u32)p[3], (u32)p[4], (u32)p[5],
		(u32)p[6], (u32)p[7], (u32)p[8], (u32)p[9], (u32)p[10], (u32)p[11]);
	p += 12;        // point to LenType

	LenType = (((u32)p[0])<<8) + p[1];
	if (LenType == 0x8100) {
		u32 tag = (((u32)p[2])<<8)+p[3];
		u32 type = (((u32)p[4])<<8) + p[5];

		snprintf(buf+i, sizeof(buf)-i, "TPID=%04x, Tag=%04x, LenType=%04x, len=%d (VLAN tagged packet)",
			LenType, tag, type, len);
		LenType = type;
		p += 4; // point to LenType
	} else if (LenType > 1500) {
		switch (LenType) {
		case 0x0800:
			packet_t = "IPv4"; break;
		case 0x0806:
			packet_t = "ARP"; break;
		case 0x8035:
			packet_t = "RARP"; break;
		case 0x86DD:
			packet_t = "IPv6"; break;
		default:
			packet_t = "unknown";
		}

		snprintf(buf+i, sizeof(buf)-i, "Type=%04x, len=%d (%s packet)",
			LenType, (int)len, packet_t);
	} else {
		snprintf(buf+i, sizeof(buf)-i, "Len=%04x, len=%d (802.3 packet)",
			LenType, (int)len);
	}
	p += 2;
	eth_info("%s\n", buf);

	if (LenType == 0x0800) {
		u8 proto;
		u32 src_port, dst_port;
		char *proto_s;

		proto = p[9];
		switch (proto) {
		case 1:
			proto_s = "/ICMP"; break;
		case 6:
			proto_s = "/TCP"; break;
		case 17:
			proto_s = "/UDP"; break;
		default:
			proto_s = ""; break;
		}

		i = snprintf(buf, sizeof(buf), "IP%s: SA=%d.%d.%d.%d, DA=%d.%d.%d.%d",
			proto_s,
			(u32)p[12], (u32)p[13], (u32)p[14], (u32)p[15],
			(u32)p[16], (u32)p[17], (u32)p[18], (u32)p[19]);
		p += 20;

		src_port = (((u32)p[0])<<8) + p[1];
		dst_port = (((u32)p[2])<<8) + p[3];
		if (proto == 1) {
			// An ICMP packet
			char *icmp_s;

			switch (p[0]) {
			case 0:
				icmp_s = "Echo Reply"; break;
			case 3:
				icmp_s = "Dst unreachable"; break;
			case 8:
				icmp_s = "Echo Request"; break;
			default:
				icmp_s = ""; break;
			}

			snprintf(buf+i, sizeof(buf)-i, ", %s", icmp_s);
		} else if (proto == 6) {
			// A TCP packet
			i += snprintf(buf+i, sizeof(buf)-i, ", SP=%d, DP=%d", src_port, dst_port);
			if ((dst_port == 67) || (dst_port == 68)) {
				snprintf(buf+i, sizeof(buf)-i, " (DHCP)");
			}
		} else if (proto == 17) {
			// An UDP packet
			i += snprintf(buf+i, sizeof(buf)-i, ", SP=%d, DP=%d", src_port, dst_port);
			p += 8;

			if ((dst_port == 67) || (dst_port == 68)) {
				snprintf(buf+i, sizeof(buf)-i, " (DHCP)");
			}else if ((dst_port == 69) || (src_port == tftp_port) || (dst_port == tftp_port)) {
				// A TFTP packet
				u32 op_c;
				u32 block_num;

				op_c = (((u32)p[0])<<8) + p[1];
				block_num = (((u32)p[2])<<8) + p[3];

				switch (op_c) {
				case 1:
					snprintf(buf+i, sizeof(buf)-i, ", TFTP: RRQ, %s", &p[2]);
					tftp_port = src_port;
					break;
				case 2:
					snprintf(buf+i, sizeof(buf)-i, ", TFTP: WRQ, %s", &p[2]);
					break;
				case 3:
					snprintf(buf+i, sizeof(buf)-i, ", TFTP: DAT, #%d", block_num);
					break;
				case 4:
					snprintf(buf+i, sizeof(buf)-i, ", TFTP: ACK, #%d", block_num);
					break;
				case 5:
					snprintf(buf+i, sizeof(buf)-i, ", TFTP: ERR, #%d", block_num);
					break;
				case 6:
					snprintf(buf+i, sizeof(buf)-i, ", TFTP: OAK, #%d", block_num);
					break;
				}
			}
		}

		eth_info("%s\n", buf);
	}
}
#endif

u32 mdio_read(u32 phy_id, u16 regnum)
{
	u32 value;
	ulong start;
	int timeout = CONFIG_MDIO_TIMEOUT;

	HWREG_W(phy_cntl_reg0, (1<<14) | (regnum <<8) | phy_id);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		value = HWREG_R(phy_cntl_reg1);
		if (value & (1<<1)) {
			//eth_info("mdio_read(addr=%d, reg=%d) = %04x\n", phy_id, regnum, value>>16);
			return value >> 16;
		}
		udelay(10);
	};

	return -1;
}

u32 mdio_write(u32 phy_id, u32 regnum, u16 val)
{
	ulong start;
	int timeout = CONFIG_MDIO_TIMEOUT;

	//eth_info("mdio_write(addr=%d, reg=%d, val=%04x)\n", phy_id, regnum, val);
	HWREG_W(phy_cntl_reg0, (val<<16) | (1<<13) | (regnum <<8) | phy_id);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		if (HWREG_R(phy_cntl_reg1) & (1<<0)) {
			return 0;
		}
		udelay(10);
	};

	return -1;
}

static int l2sw_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	return mdio_read(addr, reg);
}

static int l2sw_mdio_write(struct mii_dev *bus, int addr, int devad, int reg, u16 val)
{
	return mdio_write(addr, reg, val);
}

static int l2sw_mdio_init(const char *name, struct udevice *priv)
{
	struct mii_dev *bus = mdio_alloc();

	if (!bus) {
		eth_err("Failed to allocate MDIO bus\n");
		return -ENOMEM;
	}

	bus->read = l2sw_mdio_read;
	bus->write = l2sw_mdio_write;
	snprintf(bus->name, sizeof(bus->name), name);
	bus->priv = (void *)priv;

	return mdio_register(bus);
}

static int l2sw_phy_init(struct emac_eth_dev *priv, void *dev)
{
	struct phy_device *phy_dev;

	//eth_info("[%s] IN\n", __func__);

	phy_dev = phy_connect(priv->bus, priv->phy_addr0, dev, priv->interface);
	if (!phy_dev)
		return -ENODEV;

	phy_connect_dev(phy_dev, dev);

	priv->phy_dev0 = phy_dev;
	phy_config(priv->phy_dev0);

	return 0;
}

static void rx_descs_init(struct emac_eth_dev *priv)
{
	volatile char *rxbuffs = &priv->rxbuffer[0];
	volatile struct spl2sw_desc *rxdesc;
	u32 i, j;

	//eth_info("[%s] IN\n", __func__);

	// Flush all rx buffers */
	flush_dcache_range(DCACHE_ROUNDDN(rxbuffs), DCACHE_ROUNDUP(rxbuffs+RX_TOTAL_BUFSIZE));

	for (i = 0; i < CONFIG_RX_QUEUE_NUM; i++) {
		for (j = 0; j < CONFIG_RX_DESCR_NUM; j++) {
			rxdesc = &priv->rx_desc[i*CONFIG_RX_DESCR_NUM+j];
			rxdesc->addr1 = (uintptr_t)&rxbuffs[(i*CONFIG_RX_DESCR_NUM+j) * CONFIG_ETH_BUFSIZE];
			rxdesc->addr2 = 0;
			rxdesc->cmd2 = (j == (CONFIG_RX_DESCR_NUM - 1))? EOR_BIT|CONFIG_ETH_RXSIZE: CONFIG_ETH_RXSIZE;
			rxdesc->cmd1 = OWN_BIT;
		}
		priv->rx_pos = 0;
	}

	// Flush all rx descriptors.
	flush_dcache_range(DCACHE_ROUNDDN(priv->rx_desc), DCACHE_ROUNDUP((u32)priv->rx_desc+sizeof(priv->rx_desc)));
	//eth_info("RX Queue: start = %08x, end = %08x\n", priv->rx_desc, &priv->rx_desc[CONFIG_RX_DESCR_NUM]);

	// Setup base address for high- and low-priority rx queue.
	HWREG_W(rx_hbase_addr_0, (uintptr_t)&priv->rx_desc[0]);
	HWREG_W(rx_lbase_addr_0, (uintptr_t)&priv->rx_desc[CONFIG_RX_DESCR_NUM]);
}

static void tx_descs_init(struct emac_eth_dev *priv)
{
	volatile char *txbuffs = &priv->txbuffer[0];
	volatile struct spl2sw_desc *txdesc;
	u32 i;

	//eth_info("[%s] IN\n", __func__);

	memset((void*)&priv->tx_desc[0], 0, sizeof(*txdesc)*(CONFIG_TX_DESCR_NUM*CONFIG_RX_QUEUE_NUM));
	for (i = 0; i < (CONFIG_TX_DESCR_NUM*CONFIG_RX_QUEUE_NUM); i++) {
		txdesc = &priv->tx_desc[i];
		txdesc->addr1 = (uintptr_t)&txbuffs[i * CONFIG_ETH_BUFSIZE];
	}

	priv->tx_pos = 0;

	// Flush all tx descriptors.
	flush_dcache_range(DCACHE_ROUNDDN(priv->tx_desc), DCACHE_ROUNDUP((u32)priv->tx_desc+sizeof(priv->tx_desc)));
	//eth_info("TX Queue: start = %08x, end = %08x\n", priv->tx_desc, &priv->tx_desc[CONFIG_TX_DESCR_NUM]);

	// Setup base address for high- and low-priority tx queue.
	HWREG_W(tx_hbase_addr_0, (uintptr_t)&priv->tx_desc[0]);
	HWREG_W(tx_lbase_addr_0, (uintptr_t)&priv->tx_desc[CONFIG_TX_DESCR_NUM]);
}

#if 0
void mac_hw_addr_print(void)
{
	u32 reg, regl, regh;

	// Wait for address table being idle.
	do {
		reg = HWREG_R(addr_tbl_srch);
		ndelay(10);
	} while (!(reg & MAC_ADDR_LOOKUP_IDLE));

	// Search address table from start.
	HWREG_W(addr_tbl_srch, HWREG_R(addr_tbl_srch) | MAC_BEGIN_SEARCH_ADDR);
	mb();
	while (1){
		do {
			reg = HWREG_R(addr_tbl_st);
			ndelay(10);
			//eth_info("addr_tbl_st = %08x\n", reg);
		} while (!(reg & (MAC_AT_TABLE_END|MAC_AT_DATA_READY)));

		if (reg & MAC_AT_TABLE_END) {
			break;
		}

		regl = HWREG_R(MAC_ad_ser0);
		ndelay(10);                     // delay here is necessary or you will get all 0 of MAC_ad_ser1.
		regh = HWREG_R(MAC_ad_ser1);

		//eth_info("addr_tbl_st = %08x\n", reg);
		eth_info("AT #%u: port=0x%01x, cpu=0x%01x, vid=%u, aging=%u, proxy=%u, mc_ingress=%u, "
			"HWaddr=%02x:%02x:%02x:%02x:%02x:%02x\n",
			(reg>>22)&0x3ff, (reg>>12)&0x3, (reg>>10)&0x3, (reg>>7)&0x7,
			(reg>>4)&0x7, (reg>>3)&0x1, (reg>>2)&0x1,
			regl&0xff, (regl>>8)&0xff,
			regh&0xff, (regh>>8)&0xff, (regh>>16)&0xff, (regh>>24)&0xff);

		// Search next.
		HWREG_W(addr_tbl_srch, HWREG_R(addr_tbl_srch) | MAC_SEARCH_NEXT_ADDR);
	}
}
#endif

static int _l2sw_write_hwaddr(struct emac_eth_dev *priv, u8 *mac_id)
{
	u32 reg;

	//eth_info("[%s] IN\n", __func__);

	HWREG_W(w_mac_15_0, mac_id[0]+(mac_id[1]<<8));
	HWREG_W(w_mac_47_16, mac_id[2]+(mac_id[3]<<8)+(mac_id[4]<<16)+(mac_id[5]<<24));

	//eth_info("ethaddr=%02x:%02x:%02x:%02x:%02x:%02x\n", mac_id[0], mac_id[1],
	//	mac_id[2], mac_id[3], mac_id[4], mac_id[5]);

	HWREG_W(wt_mac_ad0, (1<<10)|(1<<4)|1);  // Set aging=1
	do {
		reg = HWREG_R(wt_mac_ad0);
		ndelay(10);
		//eth_info("wt_mac_ad0 = 0x%08x\n", reg);
	} while ((reg&(0x1<<1)) == 0x0);
	//eth_info("mac_ad0 = %08x, mac_ad = %08x%04x\n", HWREG_R(wt_mac_ad0), HWREG_R(w_mac_47_16), HWREG_R(w_mac_15_0)&0xffff);

	memcpy(priv->mac_addr, mac_id, ARP_HLEN);
	//mac_hw_addr_print();

	return 0;
}

static int _l2sw_remove_hwaddr(struct emac_eth_dev *priv, u8 *mac_id)
{
	u32 reg;

	//eth_info("[%s] IN\n", __func__);

	HWREG_W(w_mac_15_0, mac_id[0]+(mac_id[1]<<8));
	HWREG_W(w_mac_47_16, mac_id[2]+(mac_id[3]<<8)+(mac_id[4]<<16)+(mac_id[5]<<24));

	HWREG_W(wt_mac_ad0, (1<<12)+1);
	do {
		reg = HWREG_R(wt_mac_ad0);
		ndelay(10);
		//eth_info("wt_mac_ad0 = 0x%08x\n", reg);
	} while ((reg&(0x1<<1)) == 0x0);
	//eth_info("mac_ad0 = %08x, mac_ad = %08x%04x\n", HWREG_R(wt_mac_ad0), HWREG_R(w_mac_47_16), HWREG_R(w_mac_15_0)&0xffff);

	//mac_hw_addr_print();

	return 0;
}

static int l2sw_emac_eth_start(struct udevice *dev)
{
	u32 reg;

	//eth_info("[%s] IN\n", __func__);

	dcache_disable();

	// Enable cpu port 0 (6) & port 0 crc padding (8)
	reg = HWREG_R(cpu_cntl);
	HWREG_W(cpu_cntl, (reg & (~(0x1<<6))) | (0x1<<8));

	// Enable lan port0 & port 1
	reg = HWREG_R(port_cntl0);
	HWREG_W(port_cntl0, reg & (~(0x3<<24)));

	return 0;
}

static int l2sw_eth_write_hwaddr(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct emac_eth_dev *priv = dev_get_priv(dev);

	// Delete the old mac address.
	//eth_info("ethaddr=%02x:%02x:%02x:%02x:%02x:%02x\n", priv->mac_addr[0], priv->mac_addr[1],
	//	priv->mac_addr[2], priv->mac_addr[3], priv->mac_addr[4], priv->mac_addr[5]);
	if (is_valid_ethaddr(priv->mac_addr)) {
		_l2sw_remove_hwaddr(priv, priv->mac_addr);
	}

	// Write new mac address if it is valid.
	if (is_valid_ethaddr(pdata->enetaddr)) {
		return _l2sw_write_hwaddr(priv, pdata->enetaddr);
	} else {
		eth_err("Invalid mac address = %02x:%02x:%02x:%02x:%02x:%02x!\n",
			pdata->enetaddr[0], pdata->enetaddr[1], pdata->enetaddr[2],
			pdata->enetaddr[3], pdata->enetaddr[4], pdata->enetaddr[5]);
	}

	return -1;
}

static int l2sw_emac_eth_send(struct udevice *dev, void *packet, int len)
{
	struct emac_eth_dev *priv = dev_get_priv(dev);
	u32 tx_pos = priv->tx_pos;
	volatile struct spl2sw_desc *txdesc = &priv->tx_desc[tx_pos];
	uintptr_t desc_start, desc_end;
	uintptr_t data_start, data_end;

	// Invalidate tx descriptor.
	desc_start = DCACHE_ROUNDDN(txdesc);
	desc_end = DCACHE_ROUNDUP((u32)txdesc + sizeof(*txdesc));
	invalidate_dcache_range(desc_start, desc_end);

	if (txdesc->cmd1 & OWN_BIT) {
		//eth_info("Out of TX descriptors!\n");
		return -EPERM;
	}

	memcpy((void *)txdesc->addr1, packet, len);
	if (len < TX_BUF_MIN_SZ)
	{
		memset((char*)txdesc->addr1+len, 0, TX_BUF_MIN_SZ-len);
		len = TX_BUF_MIN_SZ;
	}

	// Flush tx buffer.
	data_start = DCACHE_ROUNDDN(txdesc->addr1);
	data_end = DCACHE_ROUNDUP(txdesc->addr1 + len);
	flush_dcache_range(data_start, data_end);

	// Invalidate tx descriptor.
	invalidate_dcache_range(desc_start, desc_end);

	// Setup command of tx descriptor.
	txdesc->cmd2 = (tx_pos==(CONFIG_TX_DESCR_NUM-1))? EOR_BIT|(len&LEN_MASK): (len&LEN_MASK);
	txdesc->cmd1 = OWN_BIT | FS_BIT | LS_BIT | (1<<12) | (len&LEN_MASK);

	// Flush tx descriptor.
	flush_dcache_range(desc_start, desc_end);

	//eth_info("TX: txdesc[%d], cmd1 = %08x, cmd2 = %08x\n", tx_pos, txdesc->cmd1, txdesc->cmd2);
	//print_packet(packet, len);

	// Move to next descriptor and wrap around if needed.
	if (++tx_pos >= CONFIG_TX_DESCR_NUM)
		tx_pos = 0;
	priv->tx_pos = tx_pos;

	// Trigger hardware to transmit.
	HWREG_W(cpu_tx_trig, (1<<0));

	return 0;
}

static int l2sw_emac_eth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct emac_eth_dev *priv = dev_get_priv(dev);
	volatile struct spl2sw_desc *rxdesc;
	u32 cmd, rx_pos;
	int length;
	int good_packet = 1;
	uintptr_t desc_start, desc_end;
	ulong data_start, data_end;
	int i;
	int status;

	// Read status and clear.
	status = HWREG_R(sw_int_status_0);
	HWREG_W(sw_int_status_0, status);
	//if (status != 0) {
	//      eth_info("status = %08x\n", status);
	//}

	// Note that we only take care of high-priority queue
	// because all packets are forwarded to it.
	rx_pos = priv->rx_pos;
	for (i = 0; i < CONFIG_RX_DESCR_NUM; i++) {
		rxdesc = &priv->rx_desc[rx_pos];

		// Invalidate rx descriptor queue.
		desc_start = DCACHE_ROUNDDN(rxdesc);
		desc_end = DCACHE_ROUNDUP((u32)rxdesc+sizeof(u32));
		invalidate_dcache_range(desc_start, desc_end);
		cmd = rxdesc->cmd1;

		// Check OWN bit.
		if (cmd & OWN_BIT) {
			if (++rx_pos >= CONFIG_RX_DESCR_NUM) {
				rx_pos = 0;
			}
			continue;
		}

		//eth_info("RX: rx_desc[%d], cmd1 = %08x, cmd2 = %08x, addr1 = %08x\n", rx_pos, cmd, rxdesc->cmd2, rxdesc->addr1);

		length = cmd & LEN_MASK;
		if (length < 0x40) {
			good_packet = 0;
			eth_err("RX: Bad Packet (runt)\n");
		}

		// Invalidate received data buffer.
		data_start = DCACHE_ROUNDDN(rxdesc->addr1);
		data_end = DCACHE_ROUNDUP(rxdesc->addr1 + length);
		invalidate_dcache_range(data_start, data_end);

		//print_packet((char*)rxdesc->addr1, length);

		// Move to next descriptor and wrap-around if needed.
		if (++rx_pos >= CONFIG_RX_DESCR_NUM) {
			rx_pos = 0;
		}
		priv->rx_pos = rx_pos;

		if (good_packet) {
			if (length > CONFIG_ETH_RXSIZE) {
				eth_err("Received packet is too big (len = %d)\n", length);
				return -EMSGSIZE;
			}
			*packetp = (uchar *)(ulong)rxdesc->addr1;
			return length;
		}
	}

	return -EAGAIN;
}

static int l2sw_eth_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct emac_eth_dev *priv = dev_get_priv(dev);
	volatile struct spl2sw_desc *rxdesc;
	uintptr_t desc_start, desc_end;
	u32 i;

	i = ((int)packet - priv->rx_desc[0].addr1) / CONFIG_ETH_BUFSIZE;
	if (i < (CONFIG_RX_DESCR_NUM * CONFIG_RX_QUEUE_NUM)) {
		rxdesc = &priv->rx_desc[i];

		// Invalidate rx descriptor.
		desc_start = DCACHE_ROUNDDN(rxdesc);
		desc_end = DCACHE_ROUNDUP((u32)rxdesc + sizeof(u32));
		invalidate_dcache_range(desc_start, desc_end);

		//eth_info("FR: rx_desc[%d], cmd1 = %08x, cmd2 = %08x, addr1 = %08x\n", i, rxdesc->cmd1, rxdesc->cmd2, rxdesc->addr1);

		// Make the current descriptor valid again.
		rxdesc->cmd1 = OWN_BIT;

		// Flush cmd1 field of descriptor.
		flush_dcache_range(desc_start, desc_end);
	}

	return 0;
}

static void l2sw_soc_stop(void)
{
	u32 reg;

	reg = HWREG_R(cpu_cntl);
	HWREG_W(cpu_cntl, reg | (1<<6));

	reg = HWREG_R(port_cntl0);
	HWREG_W(port_cntl0, reg | (3<<24));

	HWREG_W(sw_int_status_0, 0xffffffff);
}

static void l2sw_emac_eth_stop(struct udevice *dev)
{
	struct emac_eth_dev *priv = dev_get_priv(dev);

	//eth_info("[%s] IN\n", __func__);

	// Stop receiving and tranferring.
	l2sw_soc_stop();

	dcache_enable();

	phy_shutdown(priv->phy_dev0);

	//mac_hw_addr_print();
}

static void l2sw_emac_board_setup(struct emac_eth_dev *priv)
{
	u32 reg;

	// Set polarity of TX & RX
	reg = MOON5REG_R(mo4_l2sw_clksw_ctl);
	MOON5REG_W(mo4_l2sw_clksw_ctl, reg | (0xf<<16) | 0xf);

	// Set phy 0 address.
	if (priv->phy_addr0 <= 31) {
		reg = HWREG_R(mac_force_mode);
		HWREG_W(mac_force_mode, (reg & (~(0x1f<<16))) | (priv->phy_addr0<<16));
	}

	// Set phy 1 address.
	if (priv->phy_addr1 <= 31) {
		reg = HWREG_R(mac_force_mode);
		HWREG_W(mac_force_mode, (reg & (~(0x1f<<24))) | (priv->phy_addr1<<24));
	}
	//eth_info("mac_force_mode = %08x\n", HWREG_R(mac_force_mode));
}

static int l2sw_emac_eth_init(struct emac_eth_dev *priv, u8 *enetaddr)
{
	u32 reg;

	//eth_info("[%s] IN\n", __func__);

	// Stop receiving and tranferring.
	l2sw_soc_stop();

	HWREG_W(sw_int_mask_0, 0xffffffff);

	// port 0: VLAN group 0
	// port 1: VLAN group 0
	HWREG_W(PVID_config0, (0<<4)+0);

	// VLAN group 0: cpu0+port1+port0
	HWREG_W(VLAN_memset_config0, (0x0<<8)+0xb);

	// Forward all packets to high-priority rx queue.
	reg = HWREG_R(pri_fl_cntl);
	HWREG_W(pri_fl_cntl, reg | (0x3<<12));          // Packet from lan port 0 & 1 are high priority packet.

	// RMC forward: broadcast
	// LED: 60mS
	// BC storm prev: 31 BC
	reg = HWREG_R(sw_glb_cntl);
	HWREG_W(sw_glb_cntl, (reg & (~((0x3<<25)|(0x3<<23)|(0x3<<4)))) | (0x0<<25)|(0x1<<23)|(0x1<<4));

	// Disable cpu port0 aging (12)
	// Disable cpu port0 learning (14)
	// Enable UC and MC packets
	reg = HWREG_R(cpu_cntl);
	HWREG_W(cpu_cntl, (reg & (~((0x1<<14)|(0x3c<<0)))) | (0x1<<12));

	// Write mac address.
	_l2sw_write_hwaddr(priv, enetaddr);

	// Initialize rx/tx descriptor.
	rx_descs_init(priv);
	tx_descs_init(priv);

	// High-active LED
	reg = HWREG_R(led_port0);
	HWREG_W(led_port0, reg | (1<<28));

	// Start up PHY0 */
	genphy_parse_link(priv->phy_dev0);

	return 0;
}

static int l2sw_emac_eth_probe(struct udevice *dev)
{
	struct emac_eth_dev *priv = dev_get_priv(dev);
	struct eth_pdata *pdata = dev_get_platdata(dev);
	int ret;

	l2sw_emac_board_setup(priv);

	ret = l2sw_mdio_init(dev->name, dev);
	if (ret != 0) {
		eth_err("Failed to register mdio bus\n");
	}
	priv->bus = miiphy_get_dev_by_name(dev->name);

	ret = l2sw_phy_init(priv, dev);
	if (ret != 0) {
		return ret;
	}

	l2sw_emac_eth_init(dev->priv, pdata->enetaddr);

	return 0;
}

static const struct eth_ops l2sw_emac_eth_ops = {
	.start          = l2sw_emac_eth_start,
	.write_hwaddr   = l2sw_eth_write_hwaddr,
	.send           = l2sw_emac_eth_send,
	.recv           = l2sw_emac_eth_recv,
	.free_pkt       = l2sw_eth_free_pkt,
	.stop           = l2sw_emac_eth_stop,
};

static int l2sw_emac_eth_ofdata_to_platdata(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	struct emac_eth_dev *priv = dev_get_priv(dev);
	int node = dev_of_offset(dev);
	int offset = 0;
	int i;
	u8 otp_mac[ARP_HLEN];

	ls2w_reg_base = (void*)devfdt_get_addr_name(dev, "l2sw");
	pdata->iobase = (int)ls2w_reg_base;
	//eth_info("ls2w_reg_base = %08x\n", (int)ls2w_reg_base);
	if ((int)ls2w_reg_base == -1) {
		eth_err("Failed to get base address of L2SW!\n");
		return -EINVAL;
	}

	moon5_reg_base = (void*)devfdt_get_addr_name(dev, "moon5");
	//eth_info("moon5_reg_base = %08x\n", (int)moon5_reg_base);
	if ((int)moon5_reg_base == -1) {
		eth_err("Failed to get base address of MOON5!\n");
		return -EINVAL;
	}

	pdata->phy_interface = -1;
	priv->phy_addr0 = -1;
	priv->phy_addr1 = -1;

	offset = fdtdec_lookup_phandle(gd->fdt_blob, node, "phy0");
	if (offset > 0) {
		priv->phy_addr0 = fdtdec_get_int(gd->fdt_blob, offset, "reg", -1);
	}
	//eth_info("priv->phy_addr0 = %d\n", priv->phy_addr0);
	if (priv->phy_addr0 > 31) {
		eth_err("Failed to get address of phy0 or address out of range!\n");
		return -EINVAL;
	}

	offset = fdtdec_lookup_phandle(gd->fdt_blob, node, "phy1");
	if (offset > 0) {
		priv->phy_addr1 = fdtdec_get_int(gd->fdt_blob, offset, "reg", -1);
	}
	//eth_info("priv->phy_addr1 = %d\n", priv->phy_addr1);

	pdata->phy_interface = phy_get_interface_by_name("rmii");
	//eth_info("phy_interface = %d\n", pdata->phy_interface);
	if (pdata->phy_interface == -1) {
		eth_err("Invalid PHY interface!\n");
		return -EINVAL;
	}
	priv->interface = pdata->phy_interface;

	priv->otp_mac_addr = fdtdec_get_int(gd->fdt_blob, node, "mac-addr1", -1);
	//eth_info("priv->otp_mac_addr = %d\n", priv->otp_mac_addr);
	if (priv->otp_mac_addr < 128) {
		for (i = 0; i < ARP_HLEN; i++) {
			read_otp_data(priv->otp_mac_addr+i, (char*)&otp_mac[i]);
		}

		if (is_valid_ethaddr(otp_mac)) {
			memcpy(pdata->enetaddr, otp_mac, ARP_HLEN);
		} else {
			eth_err("Invalid mac address from OTP[%d:%d] = %02x:%02x:%02x:%02x:%02x:%02x!\n",
				priv->otp_mac_addr, priv->otp_mac_addr+5, otp_mac[0], otp_mac[1],
				otp_mac[2], otp_mac[3], otp_mac[4], otp_mac[5]);
		}
	} else if (priv->otp_mac_addr == -1) {
		eth_err("OTP address of mac address is not defined!\n");
	} else {
		eth_err("Invalid OTP address of mac address!\n");
	}

	return 0;
}

static const struct udevice_id l2sw_emac_eth_ids[] = {
	{.compatible = "sunplus,sunplus-q628-l2sw"},
	{ }
};

U_BOOT_DRIVER(eth_sunplus_l2sw_emac) = {
	.name                   = "eth_sunplus_l2sw_emac",
	.id                     = UCLASS_ETH,
	.of_match               = l2sw_emac_eth_ids,
	.ofdata_to_platdata     = l2sw_emac_eth_ofdata_to_platdata,
	.probe                  = l2sw_emac_eth_probe,
	.ops                    = &l2sw_emac_eth_ops,
	.priv_auto_alloc_size   = sizeof(struct emac_eth_dev),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
	.flags                  = DM_FLAG_ALLOC_PRIV_DMA,
};

