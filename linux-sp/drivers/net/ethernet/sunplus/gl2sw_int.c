#include "l2sw_define.h"
#include "gl2sw_int.h"
#include "l2sw_driver.h"
#include "gl2sw_hal.h"


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

static inline void rx_skb(struct l2sw_mac *mac, struct sk_buff *skb)
{
	mac->dev_stats.rx_packets++;
	mac->dev_stats.rx_bytes += skb->len;

	netif_rx(skb);
}

static inline void  rx_interrupt(struct l2sw_mac *mac)
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
		//ETH_INFO(" queue = %d, rx_pos = %d, rx_count = %d\n", queue, rx_pos, rx_count);

		for (num = 0; num < rx_count; num++) {
			sinfo = comm->rx_skb_info[queue] + rx_pos;
			desc = comm->rx_desc[queue] + rx_pos;
			cmd = desc->cmd1;
			//ETH_INFO(" rx_pos = %d, RX: cmd1 = %08x, cmd2 = %08x\n", rx_pos, cmd, desc->cmd2);

			if (cmd & OWN_BIT) {
				//ETH_INFO(" RX: is owned by NIC, rx_pos = %d, desc = %px", rx_pos, desc);
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

			if (unlikely(cmd & (RX_TCP_UDP_CHKSUM_FAIL|RX_IP_CHKSUM_FAIL))) {
				//ETH_INFO(" RX IP or TCP/UDP Checksum error!\n");
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
			skb_set_mac_header(skb, 0);
			skb_set_network_header(skb, ETH_HLEN);

			if ((eth_hdr(skb)->h_proto == htons(ETH_P_IP)) && (ip_hdr(skb)->version == IPVERSION) &&
			    ((ip_hdr(skb)->protocol == IPPROTO_TCP) || (ip_hdr(skb)->protocol == IPPROTO_UDP))) {
				// An IPv4 TCP/UDP packet.
				skb->ip_summed = CHECKSUM_UNNECESSARY;
			} else {
				skb_checksum_none_assert(skb);
			}

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
void rx_do_tasklet(unsigned long data)
{
	struct l2sw_mac *mac = (struct l2sw_mac *) data;

	rx_interrupt(mac);
}
#endif

#ifdef RX_POLLING
int rx_poll(struct napi_struct *napi, int budget)
{
	struct l2sw_mac *mac = container_of(napi, struct l2sw_mac, napi);

	rx_interrupt(mac);
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
			if (cmd & OWC_BIT) {
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
void tx_do_tasklet(unsigned long data)
{
	struct l2sw_mac *mac = (struct l2sw_mac *) data;

	tx_interrupt(mac);
}
#endif

irqreturn_t ethernet_tx_interrupt(int irq, void *dev_id)
{
	struct net_device *net_dev;
	struct l2sw_mac *mac;
	struct l2sw_common *comm;
	u32 status;

	status =  read_sw_int_status() & MAC_INT_TX;
	//ETH_INFO(" TX Int Status = %08x\n", status);

	if (likely(status)) {
		write_sw_int_mask(read_sw_int_mask() | MAC_INT_TX);     /* mask all tx interrupts */
		write_sw_int_status(status);

		net_dev = (struct net_device*)dev_id;
		if (unlikely(net_dev == NULL)) {
			ETH_ERR(" net_dev is null!\n");
			return -1;
		}

		mac = netdev_priv(net_dev);
		comm = mac->comm;

		spin_lock(&comm->lock);

		if (status & (MAC_INT_TX_DONE_L | MAC_INT_TX_DONE_H)) {
#ifdef INTERRUPT_IMMEDIATELY
			tx_interrupt(mac);
#else
			tasklet_schedule(&comm->tx_tasklet);
#endif
		}

		if (unlikely(status & MAC_INT_TX_DES_ERR)) {
			ETH_ERR(" Illegal TX Descriptor Error\n");
			mac->dev_stats.tx_fifo_errors++;
			mac_soft_reset(mac);
		}

		if (unlikely(status & MAC_INT_RX_DES_ERR)) {
			ETH_ERR(" Illegal RX Descriptor!\n");
			mac->dev_stats.rx_fifo_errors++;
		}

		if (status & MAC_INT_PORT_ST_CHG) { /* link status changed*/
			port_status_change(mac);
		}

		if (unlikely(status & MAC_INT_MEM_TEST_DONE)) {
			ETH_ERR(" Memory Test Done!\n");
		}

		spin_unlock(&comm->lock);

#if 0
		if (status & MAC_INT_TCPUDP_CHKSUM_ERR) {
			ETH_INFO(" TX TCP/UDP Checksum Append Error!\n");
		}
		if (status & MAC_INT_IP_CHKSUM_ERR) {
			ETH_INFO(" TX IP Checksum Append Error!\n");
		}
		if (status & MAC_INT_WDOG_TIMER1_EXP) {
			ETH_INFO(" Timer1 Expired!\n");
		}
		if (status & MAC_INT_WDOG_TIMER0_EXP) {
			ETH_INFO(" Timer0 Expired!\n");
		}
		if (status & MAC_INT_INTRUDER_ALERT) {
			ETH_INFO(" Intruder Alert!\n");
		}
		if (status & MAC_INT_BC_STORM) {
			ETH_INFO(" Broadcast Storm!\n");
		}
		if (status & MAC_INT_MUST_DROP_LAN) {
			ETH_INFO(" Global Queue Exhaust!\n");
		}
		if (status & MAC_INT_GLOBAL_QUE_FULL) {
			ETH_INFO(" Global Queue Full!\n");
		}
		if (status & MAC_INT_TX_SOC_PAUSE_ON) {
			ETH_INFO(" CPU Port TX Pause On!\n");
		}
		if (status & MAC_INT_RX_SOC_QUE_FULL) {
			ETH_INFO(" CPU Port RX Queue Full!\n");
		}
		if (status & MAC_INT_TX_LAN1_QUE_FULL) {
			ETH_INFO(" Lan Port 1 Queue Full!\n");
		}
		if (status & MAC_INT_TX_LAN0_QUE_FULL) {
			ETH_INFO(" Lan Port 0 Queue Full!\n");
		}
#endif

		wmb();
		write_sw_int_mask(read_sw_int_mask() & ~MAC_INT_TX);
	} else {
		ETH_ERR(" TX Int Status is null (%08x)!\n", read_sw_int_status());
	}

	return IRQ_HANDLED;
}

irqreturn_t ethernet_rx_interrupt(int irq, void *dev_id)
{
	struct net_device *net_dev;
	struct l2sw_mac *mac;
	struct l2sw_common *comm;
	u32 status;

	status =  read_sw_int_status() & MAC_INT_RX;
	//ETH_INFO(" RX Int Status = %08x\n", status);

	if (likely(status)) {
		write_sw_int_mask(read_sw_int_mask() | MAC_INT_RX);     /* mask all rx interrupts */
		write_sw_int_status(status);

		net_dev = (struct net_device*)dev_id;
		if (unlikely(net_dev == NULL)) {
			ETH_ERR(" net_dev is null!\n");
			return -1;
		}

		mac = netdev_priv(net_dev);
		comm = mac->comm;

		spin_lock(&comm->lock);

#ifdef RX_POLLING
		if (napi_schedule_prep(&comm->napi)) {
			__napi_schedule(&comm->napi);
		}
#else /* RX_POLLING */
		if (status & (MAC_INT_RX_DONE_L | MAC_INT_RX_DONE_H |
			MAC_INT_RX_L_DESCF | MAC_INT_RX_H_DESCF)) {
	#ifdef INTERRUPT_IMMEDIATELY
			rx_interrupt(mac);
	#else
			tasklet_schedule(&comm->rx_tasklet);
	#endif
		}
#endif /* RX_POLLING */

		spin_unlock(&comm->lock);

#if 0
		if (status & MAC_INT_RX_H_DESCF) {
			ETH_INFO(" RX High-priority Descriptor Full!\n");
		}
		if (status & MAC_INT_RX_L_DESCF) {
			ETH_INFO(" RX Low-priority Descriptor Full!\n");
		}
#endif

		wmb();
		write_sw_int_mask(read_sw_int_mask() & ~MAC_INT_RX);
	} else {
		ETH_ERR(" RX Int Status is null (%08x)!\n", read_sw_int_status());
	}

	return IRQ_HANDLED;
}

int l2sw_get_irq(struct platform_device *pdev, struct l2sw_common *comm)
{
	int i, ret;

	for (i = 0; i < 4; i++) {
		// Get irq #i resource from dts.
		if ((ret = platform_get_irq(pdev, i)) > 0) {
			ETH_DEBUG(" IRQ #%d = %d\n", i, ret);
			comm->irq[i] = ret;
		} else {
			ETH_ERR(" No IRQ #%d resource found!\n", i);
			return -1;
		}
	}
	return 0;
}

int l2sw_request_irq(struct platform_device *pdev, struct l2sw_common *comm, struct net_device *net_dev)
{
	int rc;

	// Register irq #0 to system.
	rc = devm_request_irq(&pdev->dev, comm->irq[0], ethernet_tx_interrupt, 0, net_dev->name, net_dev);
	if (rc != 0) {
		ETH_ERR(" Failed to request irq #%d for \"%s\" (rc = %d)!\n",
			comm->irq[0], net_dev->name, rc);
		return rc;
	}

	// Register irq #1 to system.
	rc = devm_request_irq(&pdev->dev, comm->irq[1], ethernet_rx_interrupt, 0, net_dev->name, net_dev);
	if (rc != 0) {
		ETH_ERR(" Failed to request irq #%d for \"%s\" (rc = %d)!\n",
			comm->irq[1], net_dev->name, rc);
		return rc;
	}

	// Register irq #2 to system.
	rc = devm_request_irq(&pdev->dev, comm->irq[2], ethernet_tx_interrupt, 0, net_dev->name, net_dev);
	if (rc != 0) {
		ETH_ERR(" Failed to request irq #%d for \"%s\" (rc = %d)!\n",
			comm->irq[2], net_dev->name, rc);
		return rc;
	}

	// Register irq #3 to system.
	rc = devm_request_irq(&pdev->dev, comm->irq[3], ethernet_rx_interrupt, 0, net_dev->name, net_dev);
	if (rc != 0) {
		ETH_ERR(" Failed to request irq #%d for \"%s\" (rc = %d)!\n",
			comm->irq[3], net_dev->name, rc);
	}
	return rc;
}

