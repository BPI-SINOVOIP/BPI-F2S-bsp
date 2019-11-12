#include "l2sw_mdio.h"


static int mii_read(struct mii_bus *bus, int phy_id, int regnum)
{
	return mdio_read(phy_id, regnum);
}

static int mii_write(struct mii_bus *bus, int phy_id, int regnum, u16 val)
{
	return mdio_write(phy_id, regnum, val);
}

u32 mdio_init(struct platform_device *pdev, struct net_device *net_dev)
{
	struct l2sw_mac *mac = netdev_priv(net_dev);
	struct mii_bus *mii_bus;
	struct device_node *mdio_node;
	u32 ret;

	mii_bus = mdiobus_alloc();
	if (mii_bus == NULL) {
		ETH_ERR("[%s] Failed to allocate mdio_bus memory!\n", __func__);
		return -ENOMEM;
	}

	mii_bus->name = "sp7021_mii_bus";
	mii_bus->parent = &pdev->dev;
	mii_bus->priv = mac;
	mii_bus->read = mii_read;
	mii_bus->write = mii_write;
	snprintf(mii_bus->id, MII_BUS_ID_SIZE, "%s-mii", dev_name(&pdev->dev));

	mdio_node = of_get_parent(mac->comm->phy1_node);
	ret = of_mdiobus_register(mii_bus, mdio_node);
	if (ret) {
		ETH_ERR("[%s] Failed to register mii bus (ret = %d)!\n", __func__, ret);
		mdiobus_free(mii_bus);
		return ret;
	}

	mac->comm->mii_bus = mii_bus;
	return ret;
}

void mdio_remove(struct net_device *net_dev)
{
	struct l2sw_mac *mac = netdev_priv(net_dev);

	if (mac->comm->mii_bus != NULL) {
		mdiobus_unregister(mac->comm->mii_bus);
		mdiobus_free(mac->comm->mii_bus);
		mac->comm->mii_bus = NULL;
	}
}

static void mii_linkchange(struct net_device *netdev)
{
}

int mac_phy_probe(struct net_device *netdev)
{
	struct l2sw_mac *mac = netdev_priv(netdev);
	struct phy_device *phydev;

	phydev = of_phy_connect(mac->net_dev, mac->comm->phy1_node, mii_linkchange,
				0, PHY_INTERFACE_MODE_RGMII_ID);
	if (!phydev) {
		ETH_ERR(" \"%s\" has no phy found\n", netdev->name);
		return -1;
	}

	if (mac->comm->phy2_node) {
		of_phy_connect(mac->net_dev, mac->comm->phy2_node, mii_linkchange,
				0, PHY_INTERFACE_MODE_RGMII_ID);
	}

#ifdef PHY_RUN_STATEMACHINE
	phydev->supported &= (PHY_GBIT_FEATURES | SUPPORTED_Pause |
			      SUPPORTED_Asym_Pause);

	phydev->advertising = phydev->supported;
	phydev->irq = PHY_IGNORE_INTERRUPT;
	mac->comm->phy_dev = phydev;
#endif

	return 0;
}

void mac_phy_start(struct net_device *netdev)
{
	struct l2sw_mac *mac = netdev_priv(netdev);

#ifdef PHY_RUN_STATEMACHINE
	phy_start(mac->comm->phy_dev);
#else
	u32 phyaddr;
	struct phy_device *phydev = NULL;

	/* find the first (lowest address) PHY on the current MAC's MII bus */
	for (phyaddr = 0; phyaddr < PHY_MAX_ADDR; phyaddr++) {
		if (mac->comm->mii_bus->mdio_map[phyaddr]) {
			phydev = mac->comm->mii_bus->mdio_map[phyaddr];
			break; /* break out with first one found */
		}
	}

	phydev = phy_attach(netdev, dev_name(&phydev->dev), 0, PHY_INTERFACE_MODE_GMII);
	if (IS_ERR(phydev)) {
		ETH_ERR("[%s] Failed to attach phy!\n", __func__);
		return;
	}

	phydev->supported &= (PHY_GBIT_FEATURES | SUPPORTED_Pause |
			      SUPPORTED_Asym_Pause);
	phydev->advertising = phydev->supported;
	phydev->irq = PHY_IGNORE_INTERRUPT;
	mac->comm->phy_dev = phydev;

	phy_start_aneg(phydev);
#endif
}

void mac_phy_stop(struct net_device *netdev)
{
	struct l2sw_mac *mac = netdev_priv(netdev);

	if (mac->comm->phy_dev != NULL) {
#ifdef PHY_RUN_STATEMACHINE
		phy_stop(mac->comm->phy_dev);
#else
		phy_detach(mac->comm->phy_dev);
		mac->comm->phy_dev = NULL;
#endif
	}
}

void mac_phy_remove(struct net_device *netdev)
{
	struct l2sw_mac *mac = netdev_priv(netdev);

	if (mac->comm->phy_dev != NULL) {
#ifdef PHY_RUN_STATEMACHINE
		phy_disconnect(mac->comm->phy_dev);
#endif
		mac->comm->phy_dev = NULL;
	}
}


