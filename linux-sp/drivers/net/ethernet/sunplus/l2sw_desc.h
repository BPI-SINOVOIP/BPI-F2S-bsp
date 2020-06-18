#ifndef __L2SW_DESC_H__
#define __L2SW_DESC_H__

#include "l2sw_define.h"


void rx_descs_flush(struct l2sw_common *comm);

void tx_descs_clean(struct l2sw_common *comm);

void rx_descs_clean(struct l2sw_common *comm);

void descs_clean(struct l2sw_common *comm);

void descs_free(struct l2sw_common *comm);

u32 tx_descs_init(struct l2sw_common *comm);

u32 rx_descs_init(struct l2sw_common *comm);

u32 descs_alloc(struct l2sw_common *comm);

u32 descs_init(struct l2sw_common *comm);


#endif
