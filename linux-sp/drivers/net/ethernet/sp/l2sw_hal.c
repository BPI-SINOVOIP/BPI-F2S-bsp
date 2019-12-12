#include "l2sw_hal.h"


static struct l2sw_reg* ls2w_reg_base = NULL;
static struct moon5_reg* moon5_reg_base = NULL;


int l2sw_reg_base_set(void __iomem *baseaddr)
{
	ls2w_reg_base = (struct l2sw_reg*)baseaddr;
	ETH_INFO("[%s] ls2w_reg_base = 0x%08x\n", __func__, (int)ls2w_reg_base);

	if (ls2w_reg_base == NULL){
		return -1;
	}
	else{
		return 0;
	}
}

int moon5_reg_base_set(void __iomem *baseaddr)
{
	moon5_reg_base = (struct moon5_reg*)baseaddr;
	ETH_INFO("[%s] moon5_reg_base = 0x%08x\n", __func__, (int)moon5_reg_base);

	if (moon5_reg_base == NULL){
		return -1;
	}
	else{
		return 0;
	}
}

void mac_hw_stop(struct l2sw_mac *mac)
{
	struct l2sw_common *comm = mac->comm;
	u32 reg, disable;

	if (comm->enable == 0) {
		HWREG_W(sw_int_mask_0, 0xffffffff);
		HWREG_W(sw_int_status_0, 0xffffffff & (~MAC_INT_PORT_ST_CHG));

		reg = HWREG_R(cpu_cntl);
		HWREG_W(cpu_cntl, (0x3<<6) | reg);      // Disable cpu 0 and cpu 1.
	}

	if (comm->dual_nic) {
		disable = ((~comm->enable)&0x3) << 24;
		reg = HWREG_R(port_cntl0);
		HWREG_W(port_cntl0, disable | reg);     // Disable lan 0 and lan 1.
		wmb();
	}
}

void mac_hw_reset(struct l2sw_mac *mac)
{
}

void mac_hw_start(struct l2sw_mac *mac)
{
	struct l2sw_common *comm = mac->comm;
	u32 reg;

	//enable cpu port 0 (6) & port 0 crc padding (8)
	reg = HWREG_R(cpu_cntl);
	HWREG_W(cpu_cntl, (reg & (~(0x1<<6))) | (0x1<<8));
	wmb();

	//enable lan 0 & lan 1
	reg = HWREG_R(port_cntl0);
	HWREG_W(port_cntl0, reg & (~(comm->enable<<24)));
	wmb();

	//regs_print();
}

void mac_hw_addr_set(struct l2sw_mac *mac)
{
	u32 reg;

	HWREG_W(w_mac_15_0, mac->mac_addr[0]+(mac->mac_addr[1]<<8));
	HWREG_W(w_mac_47_16, mac->mac_addr[2]+(mac->mac_addr[3]<<8)+(mac->mac_addr[4]<<16)+(mac->mac_addr[5]<<24));
	wmb();

	HWREG_W(wt_mac_ad0, (mac->cpu_port<<10)+(mac->vlan_id<<7)+(1<<4)+0x1);  // Set aging=1
	wmb();
	do {
		reg = HWREG_R(wt_mac_ad0);
		ndelay(10);
		ETH_DEBUG(" wt_mac_ad0 = 0x%08x\n", reg);
	} while ((reg&(0x1<<1)) == 0x0);
	ETH_DEBUG(" mac_ad0 = %08x, mac_ad = %08x%04x\n", HWREG_R(wt_mac_ad0), HWREG_R(w_mac_47_16), HWREG_R(w_mac_15_0)&0xffff);

	//mac_hw_addr_print();
}

void mac_hw_addr_del(struct l2sw_mac *mac)
{
	u32 reg;

	HWREG_W(w_mac_15_0, mac->mac_addr[0]+(mac->mac_addr[1]<<8));
	HWREG_W(w_mac_47_16, mac->mac_addr[2]+(mac->mac_addr[3]<<8)+(mac->mac_addr[4]<<16)+(mac->mac_addr[5]<<24));
	wmb();

	HWREG_W(wt_mac_ad0, (0x1<<12)+(mac->vlan_id<<7)+0x1);
	wmb();
	do {
		reg = HWREG_R(wt_mac_ad0);
		ndelay(10);
		ETH_DEBUG(" wt_mac_ad0 = 0x%08x\n", reg);
	} while ((reg&(0x1<<1)) == 0x0);
	ETH_DEBUG(" mac_ad0 = %08x, mac_ad = %08x%04x\n", HWREG_R(wt_mac_ad0), HWREG_R(w_mac_47_16), HWREG_R(w_mac_15_0)&0xffff);

	//mac_hw_addr_print();
}

void mac_addr_table_del_all(void)
{
	u32 reg;

	// Wait for address table being idle.
	do {
		reg = HWREG_R(addr_tbl_srch);
		ndelay(10);
	} while (!(reg & MAC_ADDR_LOOKUP_IDLE));

	// Search address table from start.
	HWREG_W(addr_tbl_srch, HWREG_R(addr_tbl_srch) | MAC_BEGIN_SEARCH_ADDR);
	mb();
	while (1){
		do {
			reg = HWREG_R(addr_tbl_st);
			ndelay(10);
			ETH_DEBUG(" addr_tbl_st = %08x\n", reg);
		} while (!(reg & (MAC_AT_TABLE_END|MAC_AT_DATA_READY)));

		if (reg & MAC_AT_TABLE_END) {
			break;
		}

		ETH_DEBUG(" addr_tbl_st = %08x\n", reg);
		ETH_DEBUG(" @AT #%u: port=0x%01x, cpu=0x%01x, vid=%u, aging=%u, proxy=%u, mc_ingress=%u\n",
			(reg>>22)&0x3ff, (reg>>12)&0x3, (reg>>10)&0x3, (reg>>7)&0x7,
			(reg>>4)&0x7, (reg>>3)&0x1, (reg>>2)&0x1);

		// Delete all entries which are learnt from lan ports.
		if ((reg>>12)&0x3) {
			HWREG_W(w_mac_15_0, HWREG_R(MAC_ad_ser0));
			wmb();
			HWREG_W(w_mac_47_16, HWREG_R(MAC_ad_ser1));
			wmb();

			HWREG_W(wt_mac_ad0, (0x1<<12)+(reg&(0x7<<7))+0x1);
			wmb();
			do {
				reg = HWREG_R(wt_mac_ad0);
				ndelay(10);
				ETH_DEBUG(" wt_mac_ad0 = 0x%08x\n", reg);
			} while ((reg&(0x1<<1)) == 0x0);
			ETH_DEBUG(" mac_ad0 = %08x, mac_ad = %08x%04x\n", HWREG_R(wt_mac_ad0), HWREG_R(w_mac_47_16), HWREG_R(w_mac_15_0)&0xffff);
		}

		// Search next.
		wmb();
		HWREG_W(addr_tbl_srch, HWREG_R(addr_tbl_srch) | MAC_SEARCH_NEXT_ADDR);
	}
}

#if 0
void mac_hw_addr_print(void)
{
	u32 reg, regl, regh;

	// Wait for address table being idle.
	do {
		reg = HWREG_R(addr_tbl_srch);
		ndelay(10);
	} while (!(reg & MAC_ADDR_LOOKUP_IDLE));

	// Search address table from start.
	HWREG_W(addr_tbl_srch, HWREG_R(addr_tbl_srch) | MAC_BEGIN_SEARCH_ADDR);
	mb();
	while (1){
		do {
			reg = HWREG_R(addr_tbl_st);
			ndelay(10);
			//ETH_INFO(" addr_tbl_st = %08x\n", reg);
		} while (!(reg & (MAC_AT_TABLE_END|MAC_AT_DATA_READY)));

		if (reg & MAC_AT_TABLE_END) {
			break;
		}

		regl = HWREG_R(MAC_ad_ser0);
		ndelay(10);                     // delay here is necessary or you will get all 0 of MAC_ad_ser1.
		regh = HWREG_R(MAC_ad_ser1);

		//ETH_INFO(" addr_tbl_st = %08x\n", reg);
		ETH_INFO(" AT #%u: port=0x%01x, cpu=0x%01x, vid=%u, aging=%u, proxy=%u, mc_ingress=%u,"
			" HWaddr=%02x:%02x:%02x:%02x:%02x:%02x\n",
			(reg>>22)&0x3ff, (reg>>12)&0x3, (reg>>10)&0x3, (reg>>7)&0x7,
			(reg>>4)&0x7, (reg>>3)&0x1, (reg>>2)&0x1,
			regl&0xff, (regl>>8)&0xff,
			regh&0xff, (regh>>8)&0xff, (regh>>16)&0xff, (regh>>24)&0xff);

		// Search next.
		HWREG_W(addr_tbl_srch, HWREG_R(addr_tbl_srch) | MAC_SEARCH_NEXT_ADDR);
	}
}
#endif

void mac_hw_init(struct l2sw_mac *mac)
{
	struct l2sw_common *comm = mac->comm;
	u32 reg;

	reg = HWREG_R(cpu_cntl);
	HWREG_W(cpu_cntl, (0x3<<6) | reg);      // Disable cpu0 and cpu 1.
	wmb();

	/* descriptor base address */
	HWREG_W(tx_lbase_addr_0, mac->comm->desc_dma);
	HWREG_W(tx_hbase_addr_0, mac->comm->desc_dma + sizeof(struct mac_desc) * TX_DESC_NUM);
	HWREG_W(rx_hbase_addr_0, mac->comm->desc_dma + sizeof(struct mac_desc) * (TX_DESC_NUM + MAC_GUARD_DESC_NUM));
	HWREG_W(rx_lbase_addr_0, mac->comm->desc_dma + sizeof(struct mac_desc) * (TX_DESC_NUM + MAC_GUARD_DESC_NUM + RX_QUEUE0_DESC_NUM));
	wmb();

	// Threshold values
	HWREG_W(fl_cntl_th,     0x4a3a2d1d);    // Fc_rls_th=0x4a,  Fc_set_th=0x3a,  Drop_rls_th=0x2d, Drop_set_th=0x1d
	HWREG_W(cpu_fl_cntl_th, 0x6a5a1212);    // Cpu_rls_th=0x6a, Cpu_set_th=0x5a, Cpu_th=0x12,      Port_th=0x12
	HWREG_W(pri_fl_cntl,    0xf6680000);    // mtcc_lmt=0xf, Pri_th_l=6, Pri_th_h=6, weigh_8x_en=1

	// High-active LED
	reg = HWREG_R(led_port0);
	HWREG_W(led_port0, reg | (1<<28));

	/* phy address */
	reg = HWREG_R(mac_force_mode);
	HWREG_W(mac_force_mode, (reg & (~(0x1f<<16))) | ((mac->comm->phy1_addr&0x1f)<<16));
	reg = HWREG_R(mac_force_mode);
	HWREG_W(mac_force_mode, (reg & (~(0x1f<<24))) | ((mac->comm->phy2_addr&0x1f)<<24));

	//disable cpu port0 aging (12)
	//disable cpu port0 learning (14)
	//enable UC and MC packets
	reg = HWREG_R(cpu_cntl);
	HWREG_W(cpu_cntl, (reg & (~((0x1<<14)|(0x3c<<0)))) | (0x1<<12));

	mac_switch_mode(mac);

	if (!comm->dual_nic) {
		//enable lan 0 & lan 1
		reg = HWREG_R(port_cntl0);
		HWREG_W(port_cntl0, reg & (~(0x3<<24)));
		wmb();
	}

	wmb();
	HWREG_W(sw_int_mask_0, MAC_INT_MASK_DEF);
}

void mac_switch_mode(struct l2sw_mac *mac)
{
	struct l2sw_common *comm = mac->comm;
	u32 reg;

	if (comm->dual_nic) {
		mac->cpu_port = 0x1;            // soc0
		mac->lan_port = 0x1;            // forward to port 0
		mac->to_vlan = 0x1;             // vlan group: 0
		mac->vlan_id = 0x0;             // vlan group: 0

		if (mac->next_netdev) {
			struct l2sw_mac *mac2 = netdev_priv(mac->next_netdev);

			mac2->cpu_port = 0x1;   // soc0
			mac2->lan_port = 0x2;   // forward to port 1
			mac2->to_vlan = 0x2;    // vlan group: 1
			mac2->vlan_id = 0x1;    // vlan group: 1
		}

		//port 0: VLAN group 0
		//port 1: VLAN group 1
		HWREG_W(PVID_config0, (1<<4)+0);

		//VLAN group 0: cpu0+port0
		//VLAN group 1: cpu0+port1
		HWREG_W(VLAN_memset_config0, (0xa<<8)+0x9);

		//RMC forward: to cpu
		//LED: 60mS
		//BC storm prev: 31 BC
		reg = HWREG_R(sw_glb_cntl);
		HWREG_W(sw_glb_cntl, (reg & (~((0x3<<25)|(0x3<<23)|(0x3<<4)))) | (0x1<<25)|(0x1<<23)|(0x1<<4));
	} else {
		mac->cpu_port = 0x1;            // soc0
		mac->lan_port = 0x3;            // forward to port 0 and 1
		mac->to_vlan = 0x1;             // vlan group: 0
		mac->vlan_id = 0x0;             // vlan group: 0
		comm->enable = 0x3;             // enable lan 0 and 1

		//port 0: VLAN group 0
		//port 1: VLAN group 0
		HWREG_W(PVID_config0, (0<<4)+0);

		//VLAN group 0: cpu0+port1+port0
		HWREG_W(VLAN_memset_config0, (0x0<<8)+0xb);

		//RMC forward: broadcast
		//LED: 60mS
		//BC storm prev: 31 BC
		reg = HWREG_R(sw_glb_cntl);
		HWREG_W(sw_glb_cntl, (reg & (~((0x3<<25)|(0x3<<23)|(0x3<<4)))) | (0x0<<25)|(0x1<<23)|(0x1<<4));
	}
}

void mac_disable_port_sa_learning(void)
{
	u32 reg;

	// Disable lan port SA learning.
	reg = HWREG_R(port_cntl1);
	HWREG_W(port_cntl1, reg | (0x3<<8));
}

void mac_enable_port_sa_learning(void)
{
	u32 reg;

	// Disable lan port SA learning.
	reg = HWREG_R(port_cntl1);
	HWREG_W(port_cntl1, reg & (~(0x3<<8)));
}

void rx_mode_set(struct net_device *net_dev)
{
	struct l2sw_mac *mac = netdev_priv(net_dev);
	u32 mask, reg, rx_mode;

	ETH_DEBUG(" net_dev->flags = %08x\n", net_dev->flags);

	mask = (mac->lan_port<<2) | (mac->lan_port<<0);
	reg = HWREG_R(cpu_cntl);

	if (net_dev->flags & IFF_PROMISC) {     /* Set promiscuous mode */
		// Allow MC and unknown UC packets
		rx_mode = (mac->lan_port<<2) | (mac->lan_port<<0);
	} else if ((!netdev_mc_empty(net_dev) && (net_dev->flags & IFF_MULTICAST)) || (net_dev->flags & IFF_ALLMULTI)) {
		// Allow MC packets
		rx_mode = (mac->lan_port<<2);
	} else {
		// Disable MC and unknown UC packets
		rx_mode = 0;
	}

	HWREG_W(cpu_cntl, (reg & (~mask)) | ((~rx_mode) & mask));
	ETH_DEBUG(" cpu_cntl = %08x\n", HWREG_R(cpu_cntl));
}

#define MDIO_READ_CMD   0x02
#define MDIO_WRITE_CMD  0x01


static int mdio_access(u8 op_cd, u8 dev_reg_addr, u8 phy_addr, u32 wdata)
{
	u32 value, time = 0;

	HWREG_W(phy_cntl_reg0, (wdata << 16) | (op_cd << 13) | (dev_reg_addr << 8) | phy_addr);
	wmb();
	do {
		if (++time > MDIO_RW_TIMEOUT_RETRY_NUMBERS) {
			ETH_ERR(" mdio failed to operate!\n");
			time = 0;
		}

		value = HWREG_R(phy_cntl_reg1);
	} while ((value & 0x3) == 0);

	if (time == 0)
		return -1;

	return value >> 16;
}

u32 mdio_read(u32 phy_id, u16 regnum)
{
	int sdVal = 0;

	sdVal = mdio_access(MDIO_READ_CMD, regnum, phy_id, 0);
	if (sdVal < 0)
		return -EIO;

	return sdVal;
}

u32 mdio_write(u32 phy_id, u32 regnum, u16 val)
{
	int sdVal = 0;

	sdVal = mdio_access(MDIO_WRITE_CMD, regnum, phy_id, val);
	if (sdVal < 0)
		return -EIO;

	return 0;
}

inline void tx_trigger(void)
{
	HWREG_W(cpu_tx_trig, (0x1<<1));
}

inline void write_sw_int_mask0(u32 value)
{
	HWREG_W(sw_int_mask_0, value);
}

inline void write_sw_int_status0(u32 value)
{
	HWREG_W(sw_int_status_0, value);
}

inline u32 read_sw_int_status0(void)
{
	return HWREG_R(sw_int_status_0);
}

inline u32 read_port_ability(void)
{
	return HWREG_R(port_ability);
}

void l2sw_enable_port(struct l2sw_mac *mac)
{
	u32 reg;

	//set clock
	reg = MOON5REG_R(mo4_l2sw_clksw_ctl);
	MOON5REG_W(mo4_l2sw_clksw_ctl, reg | (0xf<<16) | 0xf);

	//phy address
	reg = HWREG_R(mac_force_mode);
	HWREG_W(mac_force_mode, (reg & (~(0x1f<<16))) | ((mac->comm->phy1_addr&0x1f)<<16));
	reg = HWREG_R(mac_force_mode);
	HWREG_W(mac_force_mode, (reg & (~(0x1f<<24))) | ((mac->comm->phy2_addr&0x1f)<<24));
	wmb();
}

int phy_cfg()
{
	// Bug workaround:
	// Flow-control of phy should be enabled. L2SW IP flow-control will refer
	// to the bit to decide to enable or disable flow-control.
	mdio_write(0, 4, mdio_read(0, 4) | (1<<10));
	mdio_write(1, 4, mdio_read(1, 4) | (1<<10));

	return 0;
}


#if 0
void regs_print()
{
	ETH_INFO(" sw_int_status_0     = %08x\n", HWREG_R(sw_int_status_0));
	ETH_INFO(" sw_int_mask_0       = %08x\n", HWREG_R(sw_int_mask_0));
	ETH_INFO(" fl_cntl_th          = %08x\n", HWREG_R(fl_cntl_th));
	ETH_INFO(" cpu_fl_cntl_th      = %08x\n", HWREG_R(cpu_fl_cntl_th));
	ETH_INFO(" pri_fl_cntl         = %08x\n", HWREG_R(pri_fl_cntl));
	ETH_INFO(" vlan_pri_th         = %08x\n", HWREG_R(vlan_pri_th));
	ETH_INFO(" En_tos_bus          = %08x\n", HWREG_R(En_tos_bus));
	ETH_INFO(" Tos_map0            = %08x\n", HWREG_R(Tos_map0));
	ETH_INFO(" Tos_map1            = %08x\n", HWREG_R(Tos_map1));
	ETH_INFO(" Tos_map2            = %08x\n", HWREG_R(Tos_map2));
	ETH_INFO(" Tos_map3            = %08x\n", HWREG_R(Tos_map3));
	ETH_INFO(" Tos_map4            = %08x\n", HWREG_R(Tos_map4));
	ETH_INFO(" Tos_map5            = %08x\n", HWREG_R(Tos_map5));
	ETH_INFO(" Tos_map6            = %08x\n", HWREG_R(Tos_map6));
	ETH_INFO(" Tos_map7            = %08x\n", HWREG_R(Tos_map7));
	ETH_INFO(" global_que_status   = %08x\n", HWREG_R(global_que_status));
	ETH_INFO(" addr_tbl_srch       = %08x\n", HWREG_R(addr_tbl_srch));
	ETH_INFO(" addr_tbl_st         = %08x\n", HWREG_R(addr_tbl_st));
	ETH_INFO(" MAC_ad_ser0         = %08x\n", HWREG_R(MAC_ad_ser0));
	ETH_INFO(" MAC_ad_ser1         = %08x\n", HWREG_R(MAC_ad_ser1));
	ETH_INFO(" wt_mac_ad0          = %08x\n", HWREG_R(wt_mac_ad0));
	ETH_INFO(" w_mac_15_0          = %08x\n", HWREG_R(w_mac_15_0));
	ETH_INFO(" w_mac_47_16         = %08x\n", HWREG_R(w_mac_47_16));
	ETH_INFO(" PVID_config0        = %08x\n", HWREG_R(PVID_config0));
	ETH_INFO(" PVID_config1        = %08x\n", HWREG_R(PVID_config1));
	ETH_INFO(" VLAN_memset_config0 = %08x\n", HWREG_R(VLAN_memset_config0));
	ETH_INFO(" VLAN_memset_config1 = %08x\n", HWREG_R(VLAN_memset_config1));
	ETH_INFO(" port_ability        = %08x\n", HWREG_R(port_ability));
	ETH_INFO(" port_st             = %08x\n", HWREG_R(port_st));
	ETH_INFO(" cpu_cntl            = %08x\n", HWREG_R(cpu_cntl));
	ETH_INFO(" port_cntl0          = %08x\n", HWREG_R(port_cntl0));
	ETH_INFO(" port_cntl1          = %08x\n", HWREG_R(port_cntl1));
	ETH_INFO(" port_cntl2          = %08x\n", HWREG_R(port_cntl2));
	ETH_INFO(" sw_glb_cntl         = %08x\n", HWREG_R(sw_glb_cntl));
	ETH_INFO(" l2sw_rsv1           = %08x\n", HWREG_R(l2sw_rsv1));
	ETH_INFO(" led_port0           = %08x\n", HWREG_R(led_port0));
	ETH_INFO(" led_port1           = %08x\n", HWREG_R(led_port1));
	ETH_INFO(" led_port2           = %08x\n", HWREG_R(led_port2));
	ETH_INFO(" led_port3           = %08x\n", HWREG_R(led_port3));
	ETH_INFO(" led_port4           = %08x\n", HWREG_R(led_port4));
	ETH_INFO(" watch_dog_trig_rst  = %08x\n", HWREG_R(watch_dog_trig_rst));
	ETH_INFO(" watch_dog_stop_cpu  = %08x\n", HWREG_R(watch_dog_stop_cpu));
	ETH_INFO(" phy_cntl_reg0       = %08x\n", HWREG_R(phy_cntl_reg0));
	ETH_INFO(" phy_cntl_reg1       = %08x\n", HWREG_R(phy_cntl_reg1));
	ETH_INFO(" mac_force_mode      = %08x\n", HWREG_R(mac_force_mode));
	ETH_INFO(" VLAN_group_config0  = %08x\n", HWREG_R(VLAN_group_config0));
	ETH_INFO(" VLAN_group_config1  = %08x\n", HWREG_R(VLAN_group_config1));
	ETH_INFO(" flow_ctrl_th3       = %08x\n", HWREG_R(flow_ctrl_th3));
	ETH_INFO(" queue_status_0      = %08x\n", HWREG_R(queue_status_0));
	ETH_INFO(" debug_cntl          = %08x\n", HWREG_R(debug_cntl));
	ETH_INFO(" queue_status_0      = %08x\n", HWREG_R(queue_status_0));
	ETH_INFO(" debug_cntl          = %08x\n", HWREG_R(debug_cntl));
	ETH_INFO(" l2sw_rsv2           = %08x\n", HWREG_R(l2sw_rsv2));
	ETH_INFO(" mem_test_info       = %08x\n", HWREG_R(mem_test_info));
	ETH_INFO(" sw_int_status_1     = %08x\n", HWREG_R(sw_int_status_1));
	ETH_INFO(" sw_int_mask_1       = %08x\n", HWREG_R(sw_int_mask_1));
	ETH_INFO(" cpu_tx_trig         = %08x\n", HWREG_R(cpu_tx_trig));
	ETH_INFO(" tx_hbase_addr_0     = %08x\n", HWREG_R(tx_hbase_addr_0));
	ETH_INFO(" tx_lbase_addr_0     = %08x\n", HWREG_R(tx_lbase_addr_0));
	ETH_INFO(" rx_hbase_addr_0     = %08x\n", HWREG_R(rx_hbase_addr_0));
	ETH_INFO(" rx_lbase_addr_0     = %08x\n", HWREG_R(rx_lbase_addr_0));
	ETH_INFO(" tx_hw_addr_0        = %08x\n", HWREG_R(tx_hw_addr_0));
	ETH_INFO(" tx_lw_addr_0        = %08x\n", HWREG_R(tx_lw_addr_0));
	ETH_INFO(" rx_hw_addr_0        = %08x\n", HWREG_R(rx_hw_addr_0));
	ETH_INFO(" rx_lw_addr_0        = %08x\n", HWREG_R(rx_lw_addr_0));
	ETH_INFO(" cpu_port_cntl_reg_0 = %08x\n", HWREG_R(cpu_port_cntl_reg_0));
}
#endif
