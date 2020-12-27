#ifndef __GL2SW_REGISTER_H__
#define __GL2SW_REGISTER_H__

#include "l2sw_define.h"


//=================================================================================================
/*
 * TYPE: RegisterFile_GL2SW
 */
struct l2sw_reg {
	u32 sw_int_status;
	u32 sw_int_mask;
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
	u32 reserved0;
	u32 VLAN_memset_config0;
	u32 VLAN_memset_config1;
	u32 port_ability;
	u32 port_st;
	u32 cpu_cntl;
	u32 port_cntl0;
	u32 port_cntl1;
	u32 port_cntl2;
	u32 sw_glb_cntl;
	u32 sw_reset;
	u32 led_port0;
	u32 led_port1;
	u32 reserved1[3];
	u32 watch_dog_trig_rst;
	u32 watch_dog_stop_cpu;
	u32 phy_cntl_reg0;
	u32 phy_cntl_reg1;
	u32 mac_force_mode0;
	u32 VLAN_group_config0;
	u32 reserved2;
	u32 flow_ctrl_th3;
	u32 queue_status_0;
	u32 debug_cntl;
	u32 debug_info;
	u32 mem_test_info;
	u32 sw_global_signal;
	u32 pause_uc_sa_sw_15_0;
	u32 pause_uc_sa_sw_47_16;
	u32 reserved3[2];
	u32 mac_force_mode1;
	u32 p0_softpad_config;
	u32 p1_softpad_config;
	u32 reserved4[70];
	u32 cpu_tx_trig;
	u32 tx_hbase_addr;
	u32 tx_lbase_addr;
	u32 rx_hbase_addr;
	u32 rx_lbase_addr;
	u32 tx_hw_addr;
	u32 tx_lw_addr;
	u32 rx_hw_addr;
	u32 rx_lw_addr;
	u32 cpu_port_cntl_reg;
	u32 desc_addr_cntl;
};

//=================================================================================================
/*
 * TYPE: RegisterFile_MOON4
 */
struct moon4_reg {
	u32 sd_softpad_cfg_0;
	u32 sd_softpad_cfg_1;
	u32 sd_softpad_cfg_2;
	u32 sd_softpad_cfg_3;
	u32 sdio_softpad_cfg_0;
	u32 sdio_softpad_cfg_1;
	u32 sdio_softpad_cfg_2;
	u32 sdio_softpad_cfg_3;
	u32 pllref_cfg;
	u32 pllsys_cfg;
	u32 pllfla_cfg;
	u32 pllgpu_cfg;
	u32 pllcpu_cfg;
	u32 pllmip_cfg;
	u32 plleth_cfg;
	u32 plltv_cfg_0;
	u32 plltv_cfg_1;
	u32 plltv_cfg_2;
	u32 plltv_cfg_3;
	u32 pllsrv_cfg_1;
	u32 xtal_cfg;
	u32 mo4_cfg_21;
};

#endif

