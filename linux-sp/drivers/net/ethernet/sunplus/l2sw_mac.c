#include "l2sw_mac.h"


bool mac_init(struct l2sw_mac *mac)
{
	u32 i;

	for (i = 0; i < RX_DESC_QUEUE_NUM; i++) {
		mac->comm->rx_pos[i] = 0;
	}
	mb();

	//mac_hw_reset(mac);
	mac_hw_init(mac);
	mb();

	return 1;
}

void mac_soft_reset(struct l2sw_mac *mac)
{
	u32 i;
	struct net_device *net_dev2;

	if (netif_carrier_ok(mac->net_dev)){
		netif_carrier_off(mac->net_dev);
		netif_stop_queue(mac->net_dev);
	}

	net_dev2 = mac->next_netdev;
	if (net_dev2) {
		if (netif_carrier_ok(net_dev2)){
			netif_carrier_off(net_dev2);
			netif_stop_queue(net_dev2);
		}
	}

	mac_hw_reset(mac);
	mac_hw_stop(mac);
	mb();

	//descs_clean(mac->comm);
	rx_descs_flush(mac->comm);
	mac->comm->tx_pos = 0;
	mac->comm->tx_done_pos = 0;
	mac->comm->tx_desc_full = 0;

	for (i = 0; i < RX_DESC_QUEUE_NUM; i++) {
		mac->comm->rx_pos[i] = 0;
	}
	mb();

	mac_hw_init(mac);
	mac_hw_start(mac);
	mb();

	if (!netif_carrier_ok(mac->net_dev)){
		netif_carrier_on(mac->net_dev);
		netif_start_queue(mac->net_dev);
	}

	if (net_dev2) {
		if (!netif_carrier_ok(net_dev2)){
			netif_carrier_on(net_dev2);
			netif_start_queue(net_dev2);
		}
	}
}

