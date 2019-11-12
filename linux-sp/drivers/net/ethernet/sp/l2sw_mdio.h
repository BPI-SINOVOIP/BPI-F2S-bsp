#ifndef __L2SW_MDIO_H__
#define __L2SW_MDIO_H__

#include "l2sw_define.h"
#include "l2sw_hal.h"

#define PHY_RUN_STATEMACHINE

u32 mdio_init(struct platform_device *pdev, struct net_device *net_dev);

void mdio_remove(struct net_device *net_dev);

int mac_phy_probe(struct net_device *netdev);

void mac_phy_start(struct net_device *netdev);

void mac_phy_stop(struct net_device *netdev);

void mac_phy_remove(struct net_device *netdev);

#endif

