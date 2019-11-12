#ifndef __L2SW_MAC_H__
#define __L2SW_MAC_H__

#include "l2sw_define.h"
#include "l2sw_hal.h"


bool mac_init(struct l2sw_mac *mac);

void mac_soft_reset(struct l2sw_mac *mac);

//calculate the empty tx descriptor number
#define TX_DESC_AVAIL(mac) \
	((mac)->tx_pos != (mac)->tx_done_pos)? \
	(((mac)->tx_done_pos < (mac)->tx_pos)? \
	(TX_DESC_NUM - ((mac)->tx_pos - (mac)->tx_done_pos)): \
	((mac)->tx_done_pos - (mac)->tx_pos)): \
	((mac)->tx_desc_full? 0: TX_DESC_NUM)


#endif
