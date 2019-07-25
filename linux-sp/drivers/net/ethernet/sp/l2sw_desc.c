#include "l2sw_desc.h"
#include "l2sw_define.h"


void rx_descs_flush(struct l2sw_common *comm)
{
	u32 i, j;
	struct mac_desc *rx_desc;
	struct skb_info *rx_skbinfo;

	for (i = 0; i < RX_DESC_QUEUE_NUM; i++) {
		rx_desc = comm->rx_desc[i];
		rx_skbinfo = comm->rx_skb_info[i];
		for (j = 0; j < comm->rx_desc_num[i]; j++) {
			rx_desc[j].addr1 = rx_skbinfo[j].mapping;
			rx_desc[j].cmd2 = (j == comm->rx_desc_num[i] - 1)? EOR_BIT|comm->rx_desc_buff_size: comm->rx_desc_buff_size;
			wmb();
			rx_desc[j].cmd1 = OWN_BIT;
		}
	}
}

void tx_descs_clean(struct l2sw_common *comm)
{
	u32 i;
	s32 buflen;

	if (comm->tx_desc == NULL) {
		return;
	}

	for (i = 0; i < TX_DESC_NUM; i++) {
		comm->tx_desc[i].cmd1 = 0;
		wmb();
		comm->tx_desc[i].cmd2 = 0;
		comm->tx_desc[i].addr1 = 0;
		comm->tx_desc[i].addr2 = 0;

		if (comm->tx_temp_skb_info[i].mapping) {
			buflen = (comm->tx_temp_skb_info[i].skb != NULL)? comm->tx_temp_skb_info[i].skb->len: MAC_TX_BUFF_SIZE;
			dma_unmap_single(&comm->pdev->dev, comm->tx_temp_skb_info[i].mapping, buflen, DMA_TO_DEVICE);
			comm->tx_temp_skb_info[i].mapping = 0;
		}

		if (comm->tx_temp_skb_info[i].skb) {
			dev_kfree_skb(comm->tx_temp_skb_info[i].skb);
			comm->tx_temp_skb_info[i].skb = NULL;
		}
	}
}

void rx_descs_clean(struct l2sw_common *comm)
{
	u32 i, j;
	struct mac_desc *rx_desc;
	struct skb_info *rx_skbinfo;

	for (i = 0; i < RX_DESC_QUEUE_NUM; i++) {
		if (comm->rx_skb_info[i] == NULL) {
			continue;
		}

		rx_desc = comm->rx_desc[i];
		rx_skbinfo = comm->rx_skb_info[i];
		for (j = 0; j < comm->rx_desc_num[i]; j++) {
			rx_desc[j].cmd1 = 0;
			wmb();
			rx_desc[j].cmd2 = 0;
			rx_desc[j].addr1 = 0;

			if (rx_skbinfo[j].skb) {
				dma_unmap_single(&comm->pdev->dev, rx_skbinfo[j].mapping, comm->rx_desc_buff_size, DMA_FROM_DEVICE);
				dev_kfree_skb(rx_skbinfo[j].skb);
				rx_skbinfo[j].skb = NULL;
				rx_skbinfo[j].mapping = 0;
			}
		}

		kfree(rx_skbinfo);
		comm->rx_skb_info[i] = NULL;
	}
}

void descs_clean(struct l2sw_common *comm)
{
	rx_descs_clean(comm);
	tx_descs_clean(comm);
}

void descs_free(struct l2sw_common *comm)
{
	u32 i;

	descs_clean(comm);
	comm->tx_desc = NULL;
	for (i = 0; i < RX_DESC_QUEUE_NUM; i++) {
		comm->rx_desc[i] = NULL;
	}

	/*  Free descriptor area  */
	if (comm->desc_base != NULL) {
		dma_free_coherent(&comm->pdev->dev, comm->desc_size, comm->desc_base, comm->desc_dma);
		comm->desc_base = NULL;
		comm->desc_dma = 0;
		comm->desc_size = 0;
	}
}

u32 tx_descs_init(struct l2sw_common *comm)
{
	memset(comm->tx_desc, '\0', sizeof(struct mac_desc) * (TX_DESC_NUM+MAC_GUARD_DESC_NUM));
	return 0;
}

u32 rx_descs_init(struct l2sw_common *comm)
{
	struct sk_buff *skb;
	u32 i, j;
	struct mac_desc *rx_desc;
	struct skb_info *rx_skbinfo;

	for (i = 0; i < RX_DESC_QUEUE_NUM; i++) {
		comm->rx_skb_info[i] = (struct skb_info*)kmalloc(sizeof(struct skb_info) * comm->rx_desc_num[i], GFP_KERNEL);
		if (comm->rx_skb_info[i] == NULL) {
			goto MEM_ALLOC_FAIL;
		}

		rx_skbinfo = comm->rx_skb_info[i];
		rx_desc = comm->rx_desc[i];
		for (j = 0; j < comm->rx_desc_num[i]; j++) {
			skb = __dev_alloc_skb(comm->rx_desc_buff_size + RX_OFFSET, GFP_ATOMIC | GFP_DMA);
			if (!skb) {
				goto MEM_ALLOC_FAIL;
			}

			skb->dev = comm->net_dev;
			skb_reserve(skb, RX_OFFSET);/* +data +tail */

			rx_skbinfo[j].skb = skb;
			rx_skbinfo[j].mapping = dma_map_single(&comm->pdev->dev, skb->data, comm->rx_desc_buff_size, DMA_FROM_DEVICE);
			rx_desc[j].addr1 = rx_skbinfo[j].mapping;
			rx_desc[j].addr2 = 0;
			rx_desc[j].cmd2 = (j == comm->rx_desc_num[i] - 1)? EOR_BIT|comm->rx_desc_buff_size: comm->rx_desc_buff_size;
			wmb();
			rx_desc[j].cmd1 = OWN_BIT;
		}
	}

	return 0;

MEM_ALLOC_FAIL:
	rx_descs_clean(comm);
	return -ENOMEM;
}

u32 descs_alloc(struct l2sw_common *comm)
{
	u32 i;
	s32 desc_size;

	/* Alloc descriptor area  */
	desc_size = (TX_DESC_NUM + MAC_GUARD_DESC_NUM) * sizeof(struct mac_desc);
	for (i = 0; i < RX_DESC_QUEUE_NUM; i++) {
		desc_size += comm->rx_desc_num[i] * sizeof(struct mac_desc);
	}

	comm->desc_base = dma_alloc_coherent(&comm->pdev->dev, desc_size, &comm->desc_dma, GFP_KERNEL);
	if (comm->desc_base == NULL) {
		return -ENOMEM;
	}
	comm->desc_size = desc_size;

	/* Setup Tx descriptor */
	comm->tx_desc = (struct mac_desc*)comm->desc_base;

	/* Setup Rx descriptor */
	comm->rx_desc[0] = &comm->tx_desc[TX_DESC_NUM + MAC_GUARD_DESC_NUM];
	for (i = 1; i < RX_DESC_QUEUE_NUM; i++) {
		comm->rx_desc[i] = comm->rx_desc[i-1] + comm->rx_desc_num[i - 1];
	}

	return 0;
}

u32 descs_init(struct l2sw_common *comm)
{
	u32 i, rc;

	// Initialize rx descriptor's data
	comm->rx_desc_num[0] = RX_QUEUE0_DESC_NUM;
#if RX_DESC_QUEUE_NUM > 1
	comm->rx_desc_num[1] = RX_QUEUE1_DESC_NUM;
#endif

	for (i = 0; i < RX_DESC_QUEUE_NUM; i++) {
		comm->rx_desc[i] = NULL;
		comm->rx_skb_info[i] = NULL;
		comm->rx_pos[i] = 0;
	}
	comm->rx_desc_buff_size = MAC_RX_LEN_MAX;

	// Initialize tx descriptor's data
	comm->tx_done_pos = 0;
	comm->tx_desc = NULL;
	comm->tx_pos = 0;
	comm->tx_desc_full = 0;
	for (i = 0; i < TX_DESC_NUM; i++) {
		comm->tx_temp_skb_info[i].skb = NULL;
	}

	// Allocate tx & rx descriptors.
	rc = descs_alloc(comm);
	if (rc) {
		ETH_ERR("[%s] Failed to allocate tx & rx descriptors!\n", __func__);
		return rc;
	}

	rc = tx_descs_init(comm);
	if (rc) {
		return rc;
	}

	return rx_descs_init(comm);
}

