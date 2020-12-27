#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/nvmem-consumer.h>
#include "l2sw_driver.h"


static const char def_mac_addr[ETHERNET_MAC_ADDR_LEN] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x80};


/*********************************************************************
*
* net_device_ops
*
**********************************************************************/
#if 0
void print_packet(struct sk_buff *skb)
{
	u8 *p = skb->data;
	int len = skb->len;
	char buf[120], *packet_t;
	u32 LenType;
	int i;

	i = snprintf(buf, sizeof(buf), " MAC: DA=%pM, SA=%pM, ", &p[0], &p[6]);
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
	ETH_INFO("%s\n", buf);
}
#endif

static int ethernet_open(struct net_device *net_dev)
{
	struct l2sw_mac *mac = netdev_priv(net_dev);

	ETH_DEBUG(" Open port = %x\n", mac->lan_port);

	/* Start phy */
	//netif_carrier_off(net_dev);
	//mac_phy_start(net_dev);

	mac->comm->enable |= mac->lan_port;

	mac_hw_start(mac);

	netif_carrier_on(net_dev);

	if (netif_carrier_ok(net_dev)) {
		ETH_DEBUG(" Open netif_start_queue.\n");
		netif_start_queue(net_dev);
	}

	return 0;
}

static int ethernet_stop(struct net_device *net_dev)
{
	struct l2sw_mac *mac = netdev_priv(net_dev);
	unsigned long flags;

	//ETH_INFO("[%s] IN\n", __func__);

	spin_lock_irqsave(&mac->comm->lock, flags);
	netif_stop_queue(net_dev);
	netif_carrier_off(net_dev);

	mac->comm->enable &= ~mac->lan_port;

	spin_unlock_irqrestore(&mac->comm->lock, flags);

	//mac_phy_stop(net_dev);
	mac_hw_stop(mac);

	return 0;
}

/* Transmit a packet (called by the kernel) */
static int ethernet_start_xmit(struct sk_buff *skb, struct net_device *net_dev)
{
	struct l2sw_mac *mac = netdev_priv(net_dev);
	struct l2sw_common *comm = mac->comm;
	u32 tx_pos;
	u32 cmd1;
	u32 cmd2;
	struct mac_desc *txdesc;
	struct skb_info *skbinfo;
	unsigned long flags;

	//ETH_INFO("[%s] IN\n", __func__);
	//print_packet(skb);

	if (unlikely(comm->tx_desc_full == 1)) { /* no desc left, wait for tx interrupt*/
		ETH_ERR(" TX descriptor queue full when xmit!\n");
		return NETDEV_TX_BUSY;
	}

	//ETH_INFO(" skb->len = %d\n", skb->len);

	/* if skb size shorter than 60, fill it with '\0' */
	if (unlikely(skb->len < ETH_ZLEN)) {
		if (skb_tailroom(skb) >= (ETH_ZLEN - skb->len)) {
			memset(__skb_put(skb, ETH_ZLEN - skb->len), '\0', ETH_ZLEN - skb->len);
		} else {
			struct sk_buff *old_skb = skb;
			skb = dev_alloc_skb(ETH_ZLEN + TX_OFFSET);
			if (skb) {
				memset(skb->data + old_skb->len, '\0', ETH_ZLEN - old_skb->len);
				memcpy(skb->data, old_skb->data, old_skb->len);
				skb_put(skb, ETH_ZLEN); /* add data to an sk_buff */
				dev_kfree_skb_irq(old_skb);
			} else {
				skb = old_skb;
			}
		}
	}

	spin_lock_irqsave(&mac->comm->lock, flags);
	tx_pos = comm->tx_pos;
	txdesc = &comm->tx_desc[tx_pos];
	skbinfo = &comm->tx_temp_skb_info[tx_pos];
	skbinfo->len = skb->len;
	skbinfo->skb = skb;
	skbinfo->mapping = dma_map_single(&mac->pdev->dev, skb->data, skb->len, DMA_TO_DEVICE);
	cmd1 = (OWN_BIT | FS_BIT | LS_BIT | (mac->to_vlan<<12)| (skb->len& LEN_MASK));
	cmd2 = skb->len & LEN_MASK;
#ifdef CONFIG_SOC_I143
	if ((eth_hdr(skb)->h_proto == htons(ETH_P_IP)) && (ip_hdr(skb)->version == IPVERSION) &&
	    ((ip_hdr(skb)->protocol == IPPROTO_TCP) || (ip_hdr(skb)->protocol == IPPROTO_UDP))) {
		// An IPv4 TCP/UDP packet.
		cmd2 |= IP_CHKSUM_APPEND | TCP_UDP_CHKSUM_APPEND;
	}
#endif
	if (tx_pos == (TX_DESC_NUM-1)) {
		cmd2 |= EOR_BIT;
	}
	//ETH_INFO(" TX1: cmd1 = %08x, cmd2 = %08x\n", cmd1, cmd2);

	txdesc->addr1 = skbinfo->mapping;
	txdesc->cmd2 = cmd2;
	wmb();
	txdesc->cmd1 = cmd1;

	NEXT_TX(tx_pos);

	if (unlikely(tx_pos == comm->tx_done_pos)) {
		netif_stop_queue(net_dev);
		comm->tx_desc_full = 1;
		//ETH_INFO(" TX Descriptor Queue Full!\n");
	}
	comm->tx_pos = tx_pos;
	wmb();

	/* trigger gmac to transmit*/
	tx_trigger();

	spin_unlock_irqrestore(&mac->comm->lock, flags);
	return NETDEV_TX_OK;
}

static void ethernet_set_rx_mode(struct net_device *net_dev)
{
	if (net_dev) {
		struct l2sw_mac *mac = netdev_priv(net_dev);
		struct l2sw_common *comm = mac->comm;
		unsigned long flags;

		spin_lock_irqsave(&comm->ioctl_lock, flags);
		rx_mode_set(net_dev);
		spin_unlock_irqrestore(&comm->ioctl_lock, flags);
	}
}

static int ethernet_set_mac_address(struct net_device *net_dev, void *addr)
{
	struct sockaddr *hwaddr = (struct sockaddr *)addr;
	struct l2sw_mac *mac = netdev_priv(net_dev);

	//ETH_INFO("[%s] IN\n", __func__);

	if (netif_running(net_dev)) {
		ETH_ERR(" Device %s is busy!\n", net_dev->name);
		return -EBUSY;
	}

	memcpy(net_dev->dev_addr, hwaddr->sa_data, net_dev->addr_len);

	/* Delete the old Ethernet MAC address */
	ETH_DEBUG(" HW Addr = %pM\n", mac->mac_addr);
	if (is_valid_ether_addr(mac->mac_addr)) {
		mac_hw_addr_del(mac);
	}

	/* Set the Ethernet MAC address */
	memcpy(mac->mac_addr, hwaddr->sa_data, net_dev->addr_len);
	mac_hw_addr_set(mac);

	return 0;
}

static int ethernet_do_ioctl(struct net_device *net_dev, struct ifreq *ifr, int cmd)
{
	struct l2sw_mac *mac = netdev_priv(net_dev);
	struct l2sw_common *comm = mac->comm;
	struct mii_ioctl_data *data = if_mii(ifr);
	unsigned long flags;

	ETH_DEBUG(" if = %s, cmd = %04x\n", ifr->ifr_ifrn.ifrn_name, cmd);
	ETH_DEBUG(" phy_id = %d, reg_num = %d, val_in = %04x\n", data->phy_id, data->reg_num, data->val_in);

	// Check parameters' range.
	if ((cmd == SIOCGMIIREG) || (cmd == SIOCSMIIREG)) {
		if (data->reg_num > 31) {
			ETH_ERR(" reg_num (= %d) excesses range!\n", (int)data->reg_num);
			return -EINVAL;
		}
	}

	switch (cmd) {
	case SIOCGMIIPHY:
		if ((comm->dual_nic) && (strcmp(ifr->ifr_ifrn.ifrn_name, "eth1") == 0)) {
			return comm->phy2_addr;
		} else {
			return comm->phy1_addr;
		}

	case SIOCGMIIREG:
		spin_lock_irqsave(&comm->ioctl_lock, flags);
		data->val_out =  mdio_read(data->phy_id, data->reg_num);
		spin_unlock_irqrestore(&comm->ioctl_lock, flags);
		ETH_DEBUG(" val_out = %04x\n", data->val_out);
		break;

	case SIOCSMIIREG:
		spin_lock_irqsave(&comm->ioctl_lock, flags);
		mdio_write(data->phy_id, data->reg_num, data->val_in);
		spin_unlock_irqrestore(&comm->ioctl_lock, flags);
		break;

	default:
		ETH_ERR(" ioctl #%d has not implemented yet!\n", cmd);
		return -EOPNOTSUPP;
	}
	return 0;
}

static int ethernet_change_mtu(struct net_device *dev, int new_mtu)
{
	return eth_change_mtu(dev, new_mtu);
}

static void ethernet_tx_timeout(struct net_device *net_dev)
{
}

static struct net_device_stats *ethernet_get_stats(struct net_device *net_dev)
{
	struct l2sw_mac *mac;

	//ETH_INFO("[%s] IN\n", __func__);
	mac = netdev_priv(net_dev);
	return &mac->dev_stats;
}


static struct net_device_ops netdev_ops = {
	.ndo_open               = ethernet_open,
	.ndo_stop               = ethernet_stop,
	.ndo_start_xmit         = ethernet_start_xmit,
	.ndo_set_rx_mode        = ethernet_set_rx_mode,
	.ndo_set_mac_address    = ethernet_set_mac_address,
	.ndo_do_ioctl           = ethernet_do_ioctl,
	.ndo_change_mtu         = ethernet_change_mtu,
	.ndo_tx_timeout         = ethernet_tx_timeout,
	.ndo_get_stats          = ethernet_get_stats,
};


char *sp7021_otp_read_mac(struct device *_d, ssize_t *_l, char *_name) {
	char *ret = NULL;
	struct nvmem_cell *c = nvmem_cell_get(_d, _name);

	if (IS_ERR_OR_NULL(c)) {
		ETH_ERR(" OTP %s read failure: %ld", _name, PTR_ERR(c));
		return (NULL);
	}

	ret = nvmem_cell_read(c, _l);
	nvmem_cell_put(c);
	ETH_DEBUG(" %zd bytes are read from OTP %s.", *_l, _name);

	return (ret);
}

static void check_mac_vendor_id_and_convert(char *mac_addr)
{
	// Byte order of MAC address of some samples are reversed.
	// Check vendor id and convert byte order if it is wrong.
	if ((mac_addr[5] == 0xFC) && (mac_addr[4] == 0x4B) && (mac_addr[3] == 0xBC) &&
		((mac_addr[0] != 0xFC) || (mac_addr[1] != 0x4B) || (mac_addr[2] != 0xBC))) {
		char tmp;
		tmp = mac_addr[0]; mac_addr[0] = mac_addr[5]; mac_addr[5] = tmp;
		tmp = mac_addr[1]; mac_addr[1] = mac_addr[4]; mac_addr[4] = tmp;
		tmp = mac_addr[2]; mac_addr[2] = mac_addr[3]; mac_addr[3] = tmp;
	}
}

/*********************************************************************
*
* platform_driver
*
**********************************************************************/
static u32 init_netdev(struct platform_device *pdev, int eth_no, struct net_device **r_ndev)
{
	u32 ret = -ENODEV;
	struct l2sw_mac *mac;
	struct net_device *net_dev;
	char *m_addr_name = (eth_no == 0)? "mac_addr0": "mac_addr1";
	ssize_t otp_l = 0;
	char *otp_v;

	//ETH_INFO("[%s] IN\n", __func__);

	/* allocate the devices, and also allocate l2sw_mac, we can get it by netdev_priv() */
	if ((net_dev = alloc_etherdev(sizeof(struct l2sw_mac))) == NULL) {
		*r_ndev = NULL;
		return -ENOMEM;
	}
	SET_NETDEV_DEV(net_dev, &pdev->dev);
	net_dev->netdev_ops = &netdev_ops;
#ifdef CONFIG_SOC_I143
	net_dev->features |= NETIF_F_IP_CSUM;
	net_dev->hw_features |= NETIF_F_IP_CSUM;
#endif

	mac = netdev_priv(net_dev);
	mac->net_dev = net_dev;
	mac->pdev = pdev;
	mac->next_netdev = NULL;

	// Get property 'mac-addr0' or 'mac-addr1' from dts.
	otp_v = sp7021_otp_read_mac(&pdev->dev, &otp_l, m_addr_name);
	if ((otp_l < 6) || IS_ERR_OR_NULL(otp_v)) {
		ETH_INFO(" OTP mac %s (len = %zd) is invalid, using default!\n", m_addr_name, otp_l);
		otp_l = 0;
	} else {
		// Check if mac-address is valid or not. If not, copy from default.
		memcpy(mac->mac_addr, otp_v, 6);

		// Byte order of Some samples are reversed. Convert byte order here.
		check_mac_vendor_id_and_convert(mac->mac_addr);

		if (!is_valid_ether_addr(mac->mac_addr)) {
			ETH_INFO(" Invalid mac in OTP[%s] = %pM, use default!\n", m_addr_name, mac->mac_addr);
			otp_l = 0;
		}
	}
	if (otp_l != 6) {
		memcpy(mac->mac_addr, def_mac_addr, ETHERNET_MAC_ADDR_LEN);
		mac->mac_addr[5] += eth_no;
	}

	ETH_INFO(" HW Addr = %pM\n", mac->mac_addr);

	memcpy(net_dev->dev_addr, mac->mac_addr, ETHERNET_MAC_ADDR_LEN);

	if ((ret = register_netdev(net_dev)) != 0) {
		ETH_ERR(" Failed to register net device \"%s\" (ret = %d)!\n", net_dev->name, ret);
		free_netdev(net_dev);
		*r_ndev = NULL;
		return ret;
	}
	ETH_INFO(" Registered net device \"%s\" successfully.\n", net_dev->name);

	*r_ndev = net_dev;
	return 0;
}

#ifdef CONFIG_DYNAMIC_MODE_SWITCHING_BY_SYSFS
static ssize_t l2sw_show_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct net_device *net_dev = dev_get_drvdata(dev);
	struct l2sw_mac *mac = netdev_priv(net_dev);

	return sprintf(buf, "%d\n", (mac->comm->dual_nic)? 1: (mac->comm->sa_learning)? 0: 2);
}

static ssize_t l2sw_store_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct net_device *net_dev = dev_get_drvdata(dev);
	struct l2sw_mac *mac = netdev_priv(net_dev);
	struct l2sw_common *comm = mac->comm;
	struct net_device *net_dev2;
	struct l2sw_mac *mac2;

	if (buf[0] == '1') {
		// Switch to dual NIC mode.
		mutex_lock(&comm->store_mode);
		if (!comm->dual_nic) {
			mac_hw_stop(mac);

			comm->dual_nic = 1;
			comm->sa_learning = 0;
			//mac_hw_addr_print();
			mac_disable_port_sa_learning();
			mac_addr_table_del_all();
			mac_switch_mode(mac);
			rx_mode_set(net_dev);
			//mac_hw_addr_print();

			init_netdev(mac->pdev, 1, &net_dev2);   // Initialize the 2nd net device (eth1)
			if (net_dev2) {
				mac->next_netdev = net_dev2;    // Pointed by previous net device.
				mac2 = netdev_priv(net_dev2);
				mac2->comm = comm;
#ifdef CONFIG_SOC_SP7021
				net_dev2->irq = comm->irq;
#else
				net_dev2->irq = comm->irq[0];
#endif

				mac_switch_mode(mac);
				rx_mode_set(net_dev2);
				mac_hw_addr_set(mac2);
				//mac_hw_addr_print();
			}

			comm->enable &= 0x1;                    // Keep lan 0, but always turn off lan 1.
			mac_hw_start(mac);
		}
		mutex_unlock(&comm->store_mode);
	} else if ((buf[0] == '0') || (buf[0] == '2')) {
		// Switch to one NIC with daisy-chain mode.
		mutex_lock(&comm->store_mode);

		if (buf[0] == '2') {
			if (comm->sa_learning == 1) {
				comm->sa_learning = 0;
				//mac_hw_addr_print();
				mac_disable_port_sa_learning();
				mac_addr_table_del_all();
				//mac_hw_addr_print();
			}
		} else {
			if (comm->sa_learning == 0) {
				comm->sa_learning = 1;
				mac_enable_port_sa_learning();
				//mac_hw_addr_print();
			}
		}

		if (comm->dual_nic) {
			struct net_device *net_dev2 = mac->next_netdev;

			if (!netif_running(net_dev2)) {
				mac_hw_stop(mac);

				mac2 = netdev_priv(net_dev2);

				// unregister and free net device.
				unregister_netdev(net_dev2);
				free_netdev(net_dev2);
				mac->next_netdev = NULL;
				ETH_INFO(" Unregistered and freed net device \"eth1\"!\n");

				comm->dual_nic = 0;
				mac_switch_mode(mac);
				rx_mode_set(net_dev);
				mac_hw_addr_del(mac2);
				//mac_hw_addr_print();

				// If eth0 is up, turn on lan 0 and 1 when switching to daisy-chain mode.
				if (comm->enable & 0x1) {
					comm->enable = 0x3;
				} else {
					comm->enable = 0;
				}

				mac_hw_start(mac);
			} else {
				ETH_ERR(" Error: Net device \"%s\" is running!\n", net_dev2->name);
			}
		}
		mutex_unlock(&comm->store_mode);
	} else {
		ETH_ERR(" Error: Unknown mode \"%c\"!\n", buf[0]);
	}

	return count;
}

static DEVICE_ATTR(mode, S_IWUSR|S_IRUGO, l2sw_show_mode, l2sw_store_mode);

static struct attribute *l2sw_sysfs_entries[] = {
	&dev_attr_mode.attr,
	NULL,
};

static struct attribute_group l2sw_attribute_group = {
	.attrs = l2sw_sysfs_entries,
};
#endif

static int soc0_open(struct l2sw_mac *mac)
{
	struct l2sw_common *comm = mac->comm;
	u32 rc;

	//ETH_INFO("[%s] IN\n", __func__);

	mac_hw_stop(mac);

#ifndef INTERRUPT_IMMEDIATELY
	//tasklet_enable(&comm->rx_tasklet);
	//tasklet_enable(&comm->tx_tasklet);
#endif

#ifdef RX_POLLING
	napi_enable(&comm->napi);
#endif

	rc = descs_init(comm);
	if (rc) {
		ETH_ERR(" Fail to initialize mac descriptors!\n");
		goto INIT_DESC_FAIL;
	}

	/*start hardware port, open interrupt, start system tx queue*/
	mac_init(mac);
	return 0;

INIT_DESC_FAIL:
	descs_free(comm);
	return rc;
}

static int soc0_stop(struct l2sw_mac *mac)
{
#ifdef RX_POLLING
	napi_disable(&mac->comm->napi);
#endif

	mac_hw_stop(mac);

	descs_free(mac->comm);
	return 0;
}

static int l2sw_probe(struct platform_device *pdev)
{
	struct l2sw_common *comm;
	struct resource *r_mem;
	struct net_device *net_dev, *net_dev2;
	struct l2sw_mac *mac, *mac2;
	u32 mode;
	int ret = 0;
	int rc;

	//ETH_INFO("[%s] IN\n", __func__);

	if (platform_get_drvdata(pdev) != NULL) {
		return -ENODEV;
	}

	// Allocate memory for l2sw 'common' area.
	comm = (struct l2sw_common*)kmalloc(sizeof(struct l2sw_common), GFP_KERNEL);
	if (comm == NULL) {
		dev_err(&pdev->dev, "Failed to allocate memory!\n");
		return -ENOMEM;
	}
	ETH_DEBUG(" comm = %px\n", comm);
	memset(comm, '\0', sizeof(struct l2sw_common));
	comm->pdev = pdev;

	/*
	 * spin_lock:         return if it obtain spin lock, or it will wait (not sleep)
	 * spin_lock_irqsave: save flags, disable interrupt, obtain spin lock
	 * spin_lock_irq:     disable interrupt, obtain spin lock, if in a interrupt, don't need to use spin_lock_irqsave
	 */
	spin_lock_init(&comm->lock);
	spin_lock_init(&comm->ioctl_lock);
	mutex_init(&comm->store_mode);

	// Get memory resoruce 0 from dts.
	if ((r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0)) != NULL) {
		ETH_DEBUG(" res->name = \"%s\", r_mem->start = %pa\n", r_mem->name, &r_mem->start);
		if (l2sw_reg_base_set(devm_ioremap(&pdev->dev, r_mem->start, (r_mem->end - r_mem->start + 1))) != 0){
			ETH_ERR(" ioremap failed!\n");
			ret = -ENOMEM;
			goto out_free_comm;
		}
	} else {
		ETH_ERR(" No MEM resource 0 found!\n");
		ret = -ENXIO;
		goto out_free_comm;
	}

#ifdef CONFIG_SOC_SP7021
	// Get memory resoruce 1 from dts.
	if ((r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1)) != NULL) {
		ETH_DEBUG(" res->name = \"%s\", r_mem->start = %pa\n", r_mem->name, &r_mem->start);
		if (moon5_reg_base_set(devm_ioremap(&pdev->dev, r_mem->start, (r_mem->end - r_mem->start + 1))) != 0){
			ETH_ERR(" ioremap failed!\n");
			ret = -ENOMEM;
			goto out_free_comm;
		}
	} else {
		ETH_ERR(" No MEM resource 1 found!\n");
		ret = -ENXIO;
		goto out_free_comm;
	}
#endif

	// Get irq resource from dts.
	if (l2sw_get_irq(pdev, comm) != 0) {
		ret = -ENXIO;
		goto out_free_comm;
	}

	// Get L2-switch mode.
	ret = of_property_read_u32(pdev->dev.of_node, "mode", &mode);
	if (ret) {
		mode = 0;
	}
	ETH_INFO(" L2 switch mode = %u\n", mode);
	if (mode == 2) {
		comm->dual_nic = 0;     // daisy-chain mode 2
		comm->sa_learning = 0;
	} else if (mode == 1) {
		comm->dual_nic = 1;     // dual NIC mode
		comm->sa_learning = 0;
	} else {
		comm->dual_nic = 0;     // daisy-chain mode
		comm->sa_learning = 1;
	}

	// Get resource of clock controller
	comm->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(comm->clk)) {
		dev_err(&pdev->dev, "Failed to retrieve clock controller!\n");
		ret = PTR_ERR(comm->clk);
		goto out_free_comm;
	}

	comm->rstc = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(comm->rstc)) {
		dev_err(&pdev->dev, "Failed to retrieve reset controller!\n");
		ret = PTR_ERR(comm->rstc);
		goto out_free_comm;
	}

	// Enable clock.
	clk_prepare_enable(comm->clk);
	udelay(1);

	ret = reset_control_assert(comm->rstc);
	udelay(1);
	ret = reset_control_deassert(comm->rstc);
	if (ret) {
		dev_err(&pdev->dev, "Failed to deassert reset line (err = %d)!\n", ret);
		ret = -ENODEV;
		goto out_free_comm;
	}
	udelay(1);

	ret = init_netdev(pdev, 0, &net_dev);   // Initialize the 1st net device (eth0)
	if (net_dev == NULL) {
		goto out_free_comm;
	}
	platform_set_drvdata(pdev, net_dev);    // Pointed by drvdata net device.

#ifdef CONFIG_SOC_SP7021
	net_dev->irq = comm->irq;
#else
	net_dev->irq = comm->irq[0];
#endif
	mac = netdev_priv(net_dev);
	mac->comm = comm;
	comm->net_dev = net_dev;
	ETH_DEBUG(" net_dev = %px, mac = %px, comm = %px\n", net_dev, mac, mac->comm);

	comm->phy1_node = of_parse_phandle(pdev->dev.of_node, "phy-handle1", 0);
	comm->phy2_node = of_parse_phandle(pdev->dev.of_node, "phy-handle2", 0);

	// Get address of phy of ethernet from dts.
	if (of_property_read_u32(comm->phy1_node, "reg", &rc) == 0) {
		comm->phy1_addr = rc;
	} else {
		comm->phy1_addr = 0;
		ETH_INFO(" Cannot get address of phy of ethernet 1! Set to 0 by default.\n");
	}

	if (of_property_read_u32(comm->phy2_node, "reg", &rc) == 0) {
		comm->phy2_addr = rc;
	} else {
		comm->phy2_addr = 1;
		ETH_INFO(" Cannot get address of phy of ethernet 2! Set to 1 by default.\n");
	}

	l2sw_enable_port(mac);

#ifndef ZEBU_XTOR
	if (comm->phy1_node) {
		ret = mdio_init(pdev, net_dev);
		if (ret) {
			ETH_ERR(" Failed to initialize mdio!\n");
			goto out_unregister_dev;
		}

		ret = mac_phy_probe(net_dev);
		if (ret) {
			ETH_ERR(" Failed to probe phy!\n");
			goto out_freemdio;
		}
	} else {
		ETH_ERR(" Failed to get phy-handle!\n");
	}

	phy_cfg(mac);
#endif

#ifdef RX_POLLING
	netif_napi_add(net_dev, &comm->napi, rx_poll, RX_NAPI_WEIGHT);
#endif

	// Register irq to system.
	if (l2sw_request_irq(pdev, comm, net_dev) != 0) {
		ret = -ENODEV;
		goto out_freemdio;
	}

#ifndef INTERRUPT_IMMEDIATELY
	tasklet_init(&comm->rx_tasklet, rx_do_tasklet, (unsigned long)mac);
	//tasklet_disable(&comm->rx_tasklet);
	tasklet_init(&comm->tx_tasklet, tx_do_tasklet, (unsigned long)mac);
	//tasklet_disable(&comm->tx_tasklet);
#endif

	soc0_open(mac);

	// Set MAC address
	mac_hw_addr_set(mac);
	//mac_hw_addr_print();

#ifdef CONFIG_DYNAMIC_MODE_SWITCHING_BY_SYSFS
	/* Add the device attributes */
	rc = sysfs_create_group(&pdev->dev.kobj, &l2sw_attribute_group);
	if (rc) {
		dev_err(&pdev->dev, "Error creating sysfs files!\n");
	}
#endif

	mac_addr_table_del_all();
	if (comm->sa_learning) {
		mac_enable_port_sa_learning();
	} else {
		mac_disable_port_sa_learning();
	}
	rx_mode_set(net_dev);
	//mac_hw_addr_print();

	if (comm->dual_nic) {
		init_netdev(pdev, 1, &net_dev2);
		if (net_dev2 == NULL) {
			goto fail_to_init_2nd_port;
		}
		mac->next_netdev = net_dev2;    // Pointed by previous net device.

#ifdef CONFIG_SOC_SP7021
		net_dev2->irq = comm->irq;
#else
		net_dev2->irq = comm->irq[0];
#endif
		mac2 = netdev_priv(net_dev2);
		mac2->comm = comm;
		ETH_DEBUG(" net_dev = %px, mac = %px, comm = %px\n", net_dev2, mac2, mac2->comm);

		mac_switch_mode(mac);
		rx_mode_set(net_dev2);
		mac_hw_addr_set(mac2);          // Set MAC address for the second net device.
		//mac_hw_addr_print();
	}

fail_to_init_2nd_port:
	return 0;

out_freemdio:
	if (comm->mii_bus) {
		mdio_remove(net_dev);
	}

#ifndef ZEBU_XTOR
out_unregister_dev:
#endif
	unregister_netdev(net_dev);

out_free_comm:
	kfree(comm);
	return ret;
}

static int l2sw_remove(struct platform_device *pdev)
{
	struct net_device *net_dev;
	struct net_device *net_dev2;
	struct l2sw_mac *mac;

	//ETH_INFO("[%s] IN\n", __func__);

	if ((net_dev = platform_get_drvdata(pdev)) == NULL) {
		return 0;
	}
	mac = netdev_priv(net_dev);

	// Unregister and free 2nd net device.
	net_dev2 = mac->next_netdev;
	if (net_dev2) {
		unregister_netdev(net_dev2);
		free_netdev(net_dev2);
	}

#ifdef CONFIG_DYNAMIC_MODE_SWITCHING_BY_SYSFS
	sysfs_remove_group(&pdev->dev.kobj, &l2sw_attribute_group);
#endif

#ifndef INTERRUPT_IMMEDIATELY
	tasklet_kill(&mac->comm->rx_tasklet);
	tasklet_kill(&mac->comm->tx_tasklet);
#endif

	mac->comm->enable = 0;
	soc0_stop(mac);

	//mac_phy_remove(net_dev);
	mdio_remove(net_dev);

	// Unregister and free 1st net device.
	unregister_netdev(net_dev);
	free_netdev(net_dev);

	clk_disable(mac->comm->clk);

	// Free 'common' area.
	kfree(mac->comm);
	return 0;
}

#ifdef CONFIG_PM
static int l2sw_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *ndev = platform_get_drvdata(pdev);

	netif_device_detach(ndev);
	return 0;
}

static int l2sw_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct net_device *ndev = platform_get_drvdata(pdev);

	netif_device_attach(ndev);
	return 0;
}

static const struct dev_pm_ops l2sw_pm_ops = {
	.suspend = l2sw_suspend,
	.resume  = l2sw_resume,
};
#endif

static const struct of_device_id sp_l2sw_of_match[] = {
#ifdef CONFIG_SOC_SP7021
	{ .compatible = "sunplus,sp7021-l2sw" },
#else
	{ .compatible = "sunplus,i143-gl2sw" },
#endif
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, sp_l2sw_of_match);

static struct platform_driver l2sw_driver = {
	.probe   = l2sw_probe,
	.remove  = l2sw_remove,
	.driver  = {
#ifdef CONFIG_SOC_SP7021
		.name  = "sp_l2sw",
#else
		.name  = "sp_gl2sw",
#endif
		.owner = THIS_MODULE,
		.of_match_table = sp_l2sw_of_match,
#ifdef CONFIG_PM
		.pm = &l2sw_pm_ops, // not sure
#endif
	},
};

module_platform_driver(l2sw_driver);

MODULE_LICENSE("GPL v2");

