#ifndef __L2SW_DRIVER_H__
#define __L2SW_DRIVER_H__

#include "l2sw_register.h"
#include "l2sw_define.h"
#include "l2sw_hal.h"
#include "l2sw_mdio.h"
#include "l2sw_mac.h"
#include "l2sw_desc.h"


#define NEXT_TX(N)              ((N) = (((N)+1) == TX_DESC_NUM)? 0: (N)+1)
#define NEXT_RX(QUEUE, N)       ((N) = (((N)+1) == mac->comm->rx_desc_num[QUEUE])? 0: (N)+1)

#define RX_NAPI_WEIGHT          64

#endif

