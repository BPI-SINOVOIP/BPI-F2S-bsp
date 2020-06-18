#ifndef __L2SW_REGISTER_H__
#define __L2SW_REGISTER_H__

#include "l2sw_define.h"


//=================================================================================================
/*
 * TYPE: RegisterFile_L2SW
 */
struct l2sw_reg {
	u32 sw_int_status_0;
	u32 sw_int_mask_0;
	u32 fl_cntl_th;
	u32 cpu_fl_cntl_th;
	u32 pri_fl_cntl;
	u32 vlan_pri_th;
	u32 En_tos_bus;
	u32 Tos_map0;
	u32 Tos_map1;
	u32 Tos_map2;
	u32 Tos_map3;
	u32 Tos_map4;
	u32 Tos_map5;
	u32 Tos_map6;
	u32 Tos_map7;
	u32 global_que_status;
	u32 addr_tbl_srch;
	u32 addr_tbl_st;
	u32 MAC_ad_ser0;
	u32 MAC_ad_ser1;
	u32 wt_mac_ad0;
	u32 w_mac_15_0;
	u32 w_mac_47_16;
	u32 PVID_config0;
	u32 PVID_config1;
	u32 VLAN_memset_config0;
	u32 VLAN_memset_config1;
	u32 port_ability;
	u32 port_st;
	u32 cpu_cntl;
	u32 port_cntl0;
	u32 port_cntl1;
	u32 port_cntl2;
	u32 sw_glb_cntl;
	u32 l2sw_rsv1;
	u32 led_port0;
	u32 led_port1;
	u32 led_port2;
	u32 led_port3;
	u32 led_port4;
	u32 watch_dog_trig_rst;
	u32 watch_dog_stop_cpu;
	u32 phy_cntl_reg0;
	u32 phy_cntl_reg1;
	u32 mac_force_mode;
	u32 VLAN_group_config0;
	u32 VLAN_group_config1;
	u32 flow_ctrl_th3;
	u32 queue_status_0;
	u32 debug_cntl;
	u32 l2sw_rsv2;
	u32 mem_test_info;
	u32 sw_int_status_1;
	u32 sw_int_mask_1;
	u32 l2sw_rsv3[76];
	u32 cpu_tx_trig;
	u32 tx_hbase_addr_0;
	u32 tx_lbase_addr_0;
	u32 rx_hbase_addr_0;
	u32 rx_lbase_addr_0;
	u32 tx_hw_addr_0;
	u32 tx_lw_addr_0;
	u32 rx_hw_addr_0;
	u32 rx_lw_addr_0;
	u32 cpu_port_cntl_reg_0;
	u32 tx_hbase_addr_1;
	u32 tx_lbase_addr_1;
	u32 rx_hbase_addr_1;
	u32 rx_lbase_addr_1;
	u32 tx_hw_addr_1;
	u32 tx_lw_addr_1;
	u32 rx_hw_addr_1;
	u32 rx_lw_addr_1;
	u32 cpu_port_cntl_reg_1;
};

//=================================================================================================
/*
 * TYPE: RegisterFile_MOON5
 */
struct moon5_reg {
	u32 mo5_thermal_ctl_0;
	u32 mo5_thermal_ctl_1;
	u32 mo4_thermal_ctl_2;
	u32 mo4_thermal_ctl_3;
	u32 mo4_tmds_l2sw_ctl;
	u32 mo4_l2sw_clksw_ctl;
};

#endif

