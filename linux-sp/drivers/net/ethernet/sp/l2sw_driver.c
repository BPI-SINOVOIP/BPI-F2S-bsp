#include "l2sw_driver.h"
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/nvmem-consumer.h>


static const char def_mac_addr[ETHERNET_MAC_ADDR_LEN] = {0x88, 0x88, 0x88, 0x88, 0x88, 0x80};


/*********************************************************************
*
* net_device_ops
*
**********************************************************************/
#if 0
static void print_packet(struct sk_buff *skb)
{
	u8 *p = skb->data;

	printk("MAC: DA=%02x:%02x:%02x:%02x:%02x:%02x, "
		    "SA=%02x:%02x:%02x:%02x:%02x:%02x, "
		    "Len/Type=%04x, len=%d\n",
		(u32)p[0], (u32)p[1], (u32)p[2], (u32)p[3], (u32)p[4], (u32)p[5],
		(u32)p[6], (u32)p[7], (u32)p[8], (u32)p[9], (u32)p[10], (u32)p[11],
		(u32)((((u32)p[12])<<8)+p[13]),
		(int)skb->len);
}
#endif

static inline void rx_skb(struct l2sw_mac *mac, struct sk_buff *skb)
{
	mac->dev_stats.rx_packets++;
	mac->dev_stats.rx_bytes += skb->len;

	netif_rx(skb);
}

static inline void port_status_change(struct l2sw_mac *mac)
{
	u32 reg;
	struct net_device *net_dev = (struct net_device *)mac->net_dev;

	reg = read_port_ability();
	if (mac->comm->dual_nic) {
		if (!netif_carrier_ok(net_dev) && (reg & PORT_ABILITY_LINK_ST_P0)) {
			netif_carrier_on(net_dev);
			netif_start_queue(net_dev);
		}
		else if (netif_carrier_ok(net_dev) && !(reg & PORT_ABILITY_LINK_ST_P0)) {
			netif_carrier_off(net_dev);
			netif_stop_queue(net_dev);
		}

		if (mac->next_netdev) {
			struct net_device *ndev2 = mac->next_netdev;

			if (!netif_carrier_ok(ndev2) && (reg & PORT_ABILITY_LINK_ST_P1)) {
				netif_carrier_on(ndev2);
				netif_start_queue(ndev2);
			}
			else if (netif_carrier_ok(ndev2) && !(reg & PORT_ABILITY_LINK_ST_P1)) {
				netif_carrier_off(ndev2);
				netif_stop_queue(ndev2);
			}
		}
	} else {
		if (!netif_carrier_ok(net_dev) && (reg & (PORT_ABILITY_LINK_ST_P1|PORT_ABILITY_LINK_ST_P0))) {
			netif_carrier_on(net_dev);
			netif_start_queue(net_dev);
		}
		else if (netif_carrier_ok(net_dev) && !(reg & (PORT_ABILITY_LINK_ST_P1|PORT_ABILITY_LINK_ST_P0))) {
			netif_carrier_off(net_dev);
			netif_stop_queue(net_dev);
		}
	}
}


static inline void  rx_interrupt(struct l2sw_mac *mac, u32 irq_status)
{
	struct sk_buff *skb, *new_skb;
	struct skb_info *sinfo;
	volatile struct mac_desc *desc;
	volatile struct mac_desc *h_desc;
	u32 rx_pos, pkg_len;
	u32 cmd;
	u32 num, rx_count;
	s32 queue;
	struct l2sw_common *comm = mac->comm;
	int ndev2_pkt;
	struct net_device_stats *dev_stats;

	// Process high-priority queue and then low-priority queue.
	for (queue = 0; queue < RX_DESC_QUEUE_NUM; queue++) {
		rx_pos = comm->rx_pos[queue];
		rx_count = comm->rx_desc_num[queue];
		//ETH_INFO(" rx_pos = %d, rx_count = %d\n", rx_pos, rx_count);

		for (num = 0; num < rx_count; num++) {
			sinfo = comm->rx_skb_info[queue] + rx_pos;
			desc = comm->rx_desc[queue] + rx_pos;
			cmd = desc->cmd1;
			//ETH_INFO(" RX: cmd1 = %08x, cmd2 = %08x\n", cmd, desc->cmd2);

			if (cmd & OWN_BIT) {
				//ETH_INFO(" RX: is owned by NIC, rx_pos = %d, desc = %p", rx_pos, desc);
				break;
			}

			if ((comm->dual_nic) && ((cmd & PKTSP_MASK) == PKTSP_PORT1)) {
				struct l2sw_mac *mac2;

				ndev2_pkt = 1;
				mac2 = (mac->next_netdev)? netdev_priv(mac->next_netdev): NULL;
				dev_stats = (mac2)? &mac2->dev_stats: &mac->dev_stats;
			} else {
				ndev2_pkt = 0;
				dev_stats = &mac->dev_stats;
			}

			pkg_len = cmd & LEN_MASK;
			if (unlikely((cmd & ERR_CODE) || (pkg_len < 64))) {
				dev_stats->rx_length_errors++;
				dev_stats->rx_dropped++;
				goto NEXT;
			}

			if (unlikely(cmd & RX_IP_CHKSUM_BIT)) {
				//ETH_INFO(" RX IP Checksum error!\n");
				dev_stats->rx_crc_errors++;
				dev_stats->rx_dropped++;
				goto NEXT;
			}

			/* allocate an skbuff for receiving, and it's an inline function */
			new_skb = __dev_alloc_skb(comm->rx_desc_buff_size + RX_OFFSET, GFP_ATOMIC | GFP_DMA);
			if (unlikely(new_skb == NULL)) {
				dev_stats->rx_dropped++;
				goto NEXT;
			}
			new_skb->dev = mac->net_dev;

			dma_unmap_single(&mac->pdev->dev, sinfo->mapping, comm->rx_desc_buff_size, DMA_FROM_DEVICE);

			skb = sinfo->skb;
			skb->ip_summed = CHECKSUM_NONE;

			/*skb_put will judge if tail exceeds end, but __skb_put won't*/
			__skb_put(skb, (pkg_len - 4 > comm->rx_desc_buff_size)? comm->rx_desc_buff_size: pkg_len - 4);

			sinfo->mapping = dma_map_single(&mac->pdev->dev, new_skb->data, comm->rx_desc_buff_size, DMA_FROM_DEVICE);
			sinfo->skb = new_skb;
			//print_packet(skb);

#if 0// dump rx data
			ETH_INFO(" RX Dump pkg_len = %d\n", pkg_len);
			u8 * pdata = skb->data;
			int i;
			for (i = 0; i < pkg_len; i++)
			{
				printk("i = %d: data = %d\n", i, *(pdata+i));
			}
#endif

			if (ndev2_pkt) {
				struct net_device *netdev2 = mac->next_netdev;

				if (netdev2) {
					skb->protocol = eth_type_trans(skb, netdev2);
					rx_skb(netdev_priv(netdev2), skb);
				}
			} else {
				skb->protocol = eth_type_trans(skb, mac->net_dev);
				rx_skb(mac, skb);
			}

			desc->addr1 = sinfo->mapping;

NEXT:
			desc->cmd2 = (rx_pos==comm->rx_desc_num[queue]-1)? EOR_BIT|MAC_RX_LEN_MAX: MAC_RX_LEN_MAX;
			wmb();
			desc->cmd1 = (OWN_BIT | (comm->rx_desc_buff_size & LEN_MASK));

			NEXT_RX(queue, rx_pos);

			// If there are packets in high-priority queue, stop processing low-priority queue.
			if ((queue == 1) && ((h_desc->cmd1 & OWN_BIT) == 0)) {
				break;
			}
		}

		comm->rx_pos[queue] = rx_pos;

		// Save pointer to last rx descriptor of high-priority queue.
		if (queue == 0) {
			h_desc = comm->rx_desc[queue] + rx_pos;
		}
	}
}

#ifndef INTERRUPT_IMMEDIATELY
static void rx_do_tasklet(unsigned long data)
{
	struct l2sw_mac *mac = (struct l2sw_mac *) data;

	rx_interrupt(mac, mac->comm->int_status);
	//write_sw_int_status0((MAC_INT_RX) & mac->comm->int_status);
}
#endif

#ifdef RX_POLLING
static int rx_poll(struct napi_struct *napi, int budget)
{
	struct l2sw_mac *mac = container_of(napi, struct l2sw_mac, napi);

	rx_interrupt(mac, mac->comm->int_status);
	napi_complete(napi);

	return 0;
}
#endif

static inline void tx_interrupt(struct l2sw_mac *mac)
{
	u32 tx_done_pos;
	u32 cmd;
	struct skb_info *skbinfo;
	struct l2sw_mac *smac;
	struct l2sw_common *comm = mac->comm;

	tx_done_pos = comm->tx_done_pos;
	//ETH_INFO(" tx_done_pos = %d\n", tx_done_pos);
	while ((tx_done_pos != comm->tx_pos) || (comm->tx_desc_full == 1)) {
		cmd = comm->tx_desc[tx_done_pos].cmd1;
		if (cmd & OWN_BIT) {
			break;
		}
		//ETH_INFO(" tx_done_pos = %d\n", tx_done_pos);
		//ETH_INFO(" TX2: cmd1 = %08x, cmd2 = %08x\n", cmd, comm->tx_desc[tx_done_pos].cmd2);

		skbinfo = &comm->tx_temp_skb_info[tx_done_pos];
		if (unlikely(skbinfo->skb == NULL)) {
			ETH_ERR(" skb is null!\n");
		}

		smac = mac;
		if ((mac->next_netdev) && ((cmd & TO_VLAN_MASK) == TO_VLAN_GROUP1)) {
			smac = netdev_priv(mac->next_netdev);
		}

		if (unlikely(cmd & (ERR_CODE))) {
			//ETH_ERR(" TX Error = %x\n", cmd);

			smac->dev_stats.tx_errors++;
#if 0
			if (status & OWC_BIT) {
				smac->dev_stats.tx_window_errors++;
			}

			if (cmd & BUR_BIT) {
				ETH_ERR(" TX aborted error\n");
				smac->dev_stats.tx_aborted_errors++;
			}
			if (cmd & LNKF_BIT) {
				ETH_ERR(" TX link failure\n");
				smac->dev_stats.tx_carrier_errors++;
			}
			if (cmd & TWDE_BIT){
				ETH_ERR(" TX watchdog timer expired!\n");
			}
			if (cmd & TBE_MASK){
				ETH_ERR(" TX descriptor bit error!\n");
			}
#endif
		}
		else {
#if 0
			smac->dev_stats.collisions += (cmd & CC_MASK) >> 16;
#endif
			smac->dev_stats.tx_packets++;
			smac->dev_stats.tx_bytes += skbinfo->len;
		}

		dma_unmap_single(&mac->pdev->dev, skbinfo->mapping, skbinfo->len, DMA_TO_DEVICE);
		skbinfo->mapping = 0;
		dev_kfree_skb_irq(skbinfo->skb);
		skbinfo->skb = NULL;

		NEXT_TX(tx_done_pos);
		if (comm->tx_desc_full == 1) {
			comm->tx_desc_full = 0;
		}
	}

	comm->tx_done_pos = tx_done_pos;
	if (!comm->tx_desc_full) {
		if (netif_queue_stopped(mac->net_dev)) {
			netif_wake_queue(mac->net_dev);
		}

		if (mac->next_netdev) {
			if (netif_queue_stopped(mac->next_netdev)) {
				netif_wake_queue(mac->next_netdev);
			}
		}
	}
}

#ifndef INTERRUPT_IMMEDIATELY
static void tx_do_tasklet(unsigned long data)
{
	struct l2sw_mac *mac = (struct l2sw_mac *) data;

	tx_interrupt(mac);
	write_sw_int_status0(mac->comm->int_status & MAC_INT_TX);
}
#endif

static irqreturn_t ethernet_interrupt(int irq, void *dev_id)
{
	struct net_device *net_dev;
	struct l2sw_mac *mac;
	struct l2sw_common *comm;
	u32 status;

	//ETH_INFO("[%s] IN\n", __func__);
	net_dev = (struct net_device*)dev_id;
	if (unlikely(net_dev == NULL)) {
		ETH_ERR(" net_dev is null!\n");
		return -1;
	}

	mac = netdev_priv(net_dev);
	comm = mac->comm;

	spin_lock(&comm->lock);

	write_sw_int_mask0(0xffffffff); /* mask all interrupts */
	status =  read_sw_int_status0();
	//ETH_INFO(" Int Status = %08x\n", status);
	if (status == 0){
		ETH_ERR(" Interrput status is null!\n");
		goto OUT;
	}
	write_sw_int_status0(status);
	comm->int_status = status;

#ifdef RX_POLLING
	if (napi_schedule_prep(&comm->napi)) {
		__napi_schedule(&comm->napi);
	}
#else /* RX_POLLING */
	if (status & MAC_INT_RX) {
		if (unlikely(status & MAC_INT_RX_DES_ERR)) {
			ETH_ERR(" Illegal RX Descriptor!\n");
			mac->dev_stats.rx_fifo_errors++;
		}

	#ifdef INTERRUPT_IMMEDIATELY
		rx_interrupt(mac, status);
		//write_sw_int_status0(comm->int_status & MAC_INT_RX);
	#else
		tasklet_schedule(&comm->rx_tasklet);
	#endif
	}
#endif /* RX_POLLING */

	if (status & MAC_INT_TX) {
		if (unlikely(status & MAC_INT_TX_DES_ERR)) {
			ETH_ERR(" Illegal TX Descriptor Error\n");
			mac->dev_stats.tx_fifo_errors++;
			mac_soft_reset(mac);
		} else {
#ifdef INTERRUPT_IMMEDIATELY
			tx_interrupt(mac);
			write_sw_int_status0(comm->int_status & MAC_INT_TX);
#else
			tasklet_schedule(&comm->tx_tasklet);
#endif
		}
	}

	if (status & MAC_INT_PORT_ST_CHG) { /* link status changed*/
		port_status_change(mac);
	}

#if 0
	if (status & MAC_INT_RX_H_DESCF) {
		ETH_INFO(" RX High-priority Descriptor Full!\n");
	}
	if (status & MAC_INT_RX_L_DESCF) {
		ETH_INFO(" RX Low-priority Descriptor Full!\n");
	}
	if (status & MAC_INT_TX_LAN0_QUE_FULL) {
		ETH_INFO(" Lan Port 0 Queue Full!\n");
	}
	if (status & MAC_INT_TX_LAN1_QUE_FULL) {
		ETH_INFO(" Lan Port 1 Queue Full!\n");
	}
	if (status & MAC_INT_RX_SOC0_QUE_FULL) {
		ETH_INFO(" CPU Port 0 RX Queue Full!\n");
	}
	if (status & MAC_INT_TX_SOC0_PAUSE_ON) {
		ETH_INFO(" CPU Port 0 TX Pause On!\n");
	}
	if (status & MAC_INT_GLOBAL_QUE_FULL) {
		ETH_INFO(" Global Queue Full!\n");
	}
#endif

OUT:
	wmb();
	write_sw_int_mask0(MAC_INT_MASK_DEF);
	spin_unlock(&comm->lock);
	return IRQ_HANDLED;
}

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
		ETH_ERR("[%s] TX descriptor queue full when awake!\n", __func__);
		return NETDEV_TX_BUSY;
	}

	//ETH_INFO("[%s] skb->len = %d\n", __func__, skb->len);

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
	cmd2 = (tx_pos == (TX_DESC_NUM-1))? EOR_BIT|(skb->len&LEN_MASK): (skb->len&LEN_MASK);
	//ETH_INFO(" TX1: cmd1 = %08x, cmd2 = %08x\n", cmd1, cmd2);

	txdesc->addr1 = skbinfo->mapping;
	txdesc->cmd2 = cmd2;
	wmb();
	txdesc->cmd1 = cmd1;

	NEXT_TX(tx_pos);

	if (unlikely(tx_pos == comm->tx_done_pos)) {
		netif_stop_queue(net_dev);
		comm->tx_desc_full = 1;
		//ETH_INFO("[%s] TX Descriptor Queue Full!\n", __func__);
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
		ETH_ERR("[%s] Ebusy\n", __func__);
		return -EBUSY;
	}

	memcpy(net_dev->dev_addr, hwaddr->sa_data, net_dev->addr_len);

	/* Delete the old Ethernet MAC address */
	ETH_DEBUG(" HW Addr = %02x:%02x:%02x:%02x:%02x:%02x\n", mac->mac_addr[0], mac->mac_addr[1],
		mac->mac_addr[2], mac->mac_addr[3], mac->mac_addr[4], mac->mac_addr[5]);
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

	ETH_DEBUG("[%s] if = %s, cmd = 0x%04x\n", __func__, ifr->ifr_ifrn.ifrn_name, cmd);
	ETH_DEBUG(" phy_id = %d, reg_num = %d, val_in = %04x\n", data->phy_id, data->reg_num, data->val_in);

	// Check parameters' range.
	if ((cmd == SIOCGMIIREG) || (cmd == SIOCSMIIREG)) {
		if (data->phy_id > 1) {
			ETH_ERR("[%s] phy_id (= %d) excesses range!\n", __func__, (int)data->phy_id);
			return -EINVAL;
		}

		if (data->reg_num > 31) {
			ETH_ERR("[%s] reg_num (= %d) excesses range!\n", __func__, (int)data->reg_num);
			return -EINVAL;
		}
	}

	switch (cmd) {
	case SIOCGMIIPHY:
		if ((comm->dual_nic) && (mac->lan_port == 1)) {
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
		ETH_ERR("[%s] ioctl #%d has not implemented yet!\n", __func__, cmd);
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
		dev_err(_d, "OTP %s read failure: %ld", _name, PTR_ERR(c));
		return (NULL);
	}

	ret = nvmem_cell_read(c, _l);
	nvmem_cell_put(c);
	dev_dbg(_d, "%d bytes read from OTP %s", *_l, _name);

	return (ret);
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

	mac = netdev_priv(net_dev);
	mac->net_dev = net_dev;
	mac->pdev = pdev;
	mac->next_netdev = NULL;

	// Get property 'mac-addr0' or 'mac-addr1' from dts.
	otp_v = sp7021_otp_read_mac(&pdev->dev, &otp_l, m_addr_name);
	if ((otp_l < 6) || IS_ERR_OR_NULL(otp_v)) {
		ETH_INFO("OTP mac %s (len = %d) is invalid, using default!\n", m_addr_name, otp_l);
		otp_l = 0;
	} else {
		// Check if mac-address is valid or not. If not, copy from default.
		memcpy(mac->mac_addr, otp_v, 6);
		if (!is_valid_ether_addr(mac->mac_addr)) {
			ETH_INFO(" Invalid mac in OTP[%s] = %02x:%02x:%02x:%02x:%02x:%02x, use default!\n", m_addr_name,
				mac->mac_addr[0], mac->mac_addr[1], mac->mac_addr[2], mac->mac_addr[3], mac->mac_addr[4], mac->mac_addr[5]);
			otp_l = 0;
		}
	}
	if (otp_l != 6) {
		memcpy(mac->mac_addr, def_mac_addr, ETHERNET_MAC_ADDR_LEN);
		mac->mac_addr[5] += eth_no;
	}

	ETH_INFO(" HW Addr = %02x:%02x:%02x:%02x:%02x:%02x\n", mac->mac_addr[0], mac->mac_addr[1],
		mac->mac_addr[2], mac->mac_addr[3], mac->mac_addr[4], mac->mac_addr[5]);

	memcpy(net_dev->dev_addr, mac->mac_addr, ETHERNET_MAC_ADDR_LEN);

	if ((ret = register_netdev(net_dev)) != 0) {
		ETH_ERR("[%s] Failed to register net device \"%s\" (ret = %d)!\n", __func__, net_dev->name, ret);
		free_netdev(net_dev);
		*r_ndev = NULL;
		return ret;
	}
	ETH_INFO("[%s] Registered net device \"%s\" successfully.\n", __func__, net_dev->name);

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
				net_dev2->irq = comm->irq;

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
				ETH_INFO("[%s] Unregistered and freed net device \"%s\"!\n", __func__, net_dev2->name);

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
				ETH_ERR("[%s] Error: Net device \"%s\" is running!\n", __func__, net_dev2->name);
			}
		}
		mutex_unlock(&comm->store_mode);
	} else {
		ETH_ERR("[%s] Error: Unknown mode \"%c\"!\n", __func__, buf[0]);
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
		ETH_ERR("[%s] Fail to initialize mac descriptors!\n", __func__);
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
	struct resource *res;
	struct resource *r_mem;
	struct net_device *net_dev, *net_dev2;
	struct l2sw_mac *mac, *mac2;
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
	ETH_DEBUG("[%s] comm = 0x%08x\n", __func__, (int)comm);
	memset(comm, '\0', sizeof(struct l2sw_common));
	comm->pdev = pdev;
#ifdef CONFIG_DUAL_NIC
	comm->dual_nic = 1;
#endif
#ifdef CONFIG_AN_NIC_WITH_DAISY_CHAIN
	comm->sa_learning = 1;
#endif

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
		ETH_DEBUG(" r_mem->start = 0x%08x\n", r_mem->start);
		if (l2sw_reg_base_set(devm_ioremap(&pdev->dev, r_mem->start, (r_mem->end - r_mem->start + 1))) != 0){
			ETH_ERR("[%s] ioremap failed!\n", __func__);
			ret = -ENOMEM;
			goto out_free_comm;
		}
	} else {
		ETH_ERR("[%s] No MEM resource 0 found!\n", __func__);
		ret = -ENXIO;
		goto out_free_comm;
	}

	// Get memory resoruce 1 from dts.
	if ((r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1)) != NULL) {
		ETH_DEBUG(" r_mem->start = 0x%08x\n", r_mem->start);
		if (moon5_reg_base_set(devm_ioremap(&pdev->dev, r_mem->start, (r_mem->end - r_mem->start + 1))) != 0){
			ETH_ERR("[%s] ioremap failed!\n", __func__);
			ret = -ENOMEM;
			goto out_free_comm;
		}
	} else {
		ETH_ERR("[%s] No MEM resource 1 found!\n", __func__);
		ret = -ENXIO;
		goto out_free_comm;
	}

	// Get irq resource from dts.
	if ((res = platform_get_resource(pdev, IORESOURCE_IRQ, 0)) != NULL) {
		ETH_DEBUG(" res->name = \"%s\", res->start = 0x%08x, res->end = 0x%08x\n",
			res->name, res->start, res->end);
		comm->irq = res->start;
	} else {
		ETH_ERR("[%s] No IRQ resource found!\n", __func__);
		ret = -ENXIO;
		goto out_free_comm;
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

	net_dev->irq = comm->irq;
	mac = netdev_priv(net_dev);
	mac->comm = comm;
	comm->net_dev = net_dev;
	ETH_INFO("[%s] net_dev = 0x%08x, mac = 0x%08x, comm = 0x%08x\n", __func__, (int)net_dev, (int)mac, (int)mac->comm);

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

	if (comm->phy1_node) {
		ret = mdio_init(pdev, net_dev);
		if (ret) {
			ETH_ERR("[%s] Failed to initialize mdio!\n", __func__);
			goto out_unregister_dev;
		}

		ret = mac_phy_probe(net_dev);
		if (ret) {
			ETH_ERR("[%s] Failed to probe phy!\n", __func__);
			goto out_freemdio;
		}
	} else {
		ETH_ERR("[%s] Failed to get phy-handle!\n", __func__);
	}

	phy_cfg();

#ifdef RX_POLLING
	netif_napi_add(net_dev, &comm->napi, rx_poll, RX_NAPI_WEIGHT);
#endif

	// Register irq to system.
	rc = devm_request_irq(&pdev->dev, comm->irq, ethernet_interrupt, 0, net_dev->name, net_dev);
	if (rc != 0) {
		ETH_ERR("[%s] Failed to request irq #%d for \"%s\" (rc = %d)!\n", __func__,
			net_dev->irq, net_dev->name, rc);
		ret = -ENODEV;
		goto out_freemdio;
	}

#ifndef INTERRUPT_IMMEDIATELY
	comm->int_status = 0;
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

		net_dev2->irq = comm->irq;
		mac2 = netdev_priv(net_dev2);
		mac2->comm = comm;
		ETH_INFO("[%s] net_dev = 0x%08x, mac = 0x%08x, comm = 0x%08x\n", __func__, (int)net_dev2, (int)mac2, (int)mac2->comm);

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

out_unregister_dev:
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
	{ .compatible = "sunplus,sp7021-l2sw" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, sp_l2sw_of_match);

static struct platform_driver l2sw_driver = {
	.probe   = l2sw_probe,
	.remove  = l2sw_remove,
	.driver  = {
		.name  = "sp_l2sw",
		.owner = THIS_MODULE,
		.of_match_table = sp_l2sw_of_match,
#ifdef CONFIG_PM
		.pm = &l2sw_pm_ops, // not sure
#endif
	},
};



static int __init l2sw_init(void)
{
	u32 status;

	//ETH_INFO("[%s] IN\n", __func__);

	status = platform_driver_register(&l2sw_driver);

	return status;
}

static void __exit l2sw_exit(void)
{
	platform_driver_unregister(&l2sw_driver);
}


module_init(l2sw_init);
module_exit(l2sw_exit);

MODULE_LICENSE("GPL v2");

