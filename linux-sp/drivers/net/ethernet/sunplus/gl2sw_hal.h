#ifndef __GL2SW_HAL_H__
#define __GL2SW_HAL_H__

#include "gl2sw_register.h"
#include "l2sw_define.h"
#include "l2sw_desc.h"


#define HWREG_W(M, N)           (l2sw_reg_base->M = N)
#define HWREG_R(M)              (l2sw_reg_base->M)


#define MDIO_RW_TIMEOUT_RETRY_NUMBERS 500


int l2sw_reg_base_set(void __iomem *baseaddr);

void mac_hw_stop(struct l2sw_mac *mac);

void mac_hw_reset(struct l2sw_mac *mac);

void mac_hw_start(struct l2sw_mac *mac);

void mac_hw_addr_set(struct l2sw_mac *mac);

void mac_hw_addr_del(struct l2sw_mac *mac);

void mac_addr_table_del_all(void);

//void mac_hw_addr_print(void);

void mac_hw_init(struct l2sw_mac *mac);

void mac_switch_mode(struct l2sw_mac *mac);

void mac_disable_port_sa_learning(void);

void mac_enable_port_sa_learning(void);

void rx_mode_set(struct net_device *net_dev);

u32 mdio_read(u32 phy_id, u16 regnum);

u32 mdio_write(u32 phy_id, u32 regnum, u16 val);

void tx_trigger(void);

void write_sw_int_mask(u32 value);

u32 read_sw_int_mask(void);

void write_sw_int_status(u32 value);

u32 read_sw_int_status(void);

u32 read_port_ability(void);

int phy_cfg(struct l2sw_mac *mac);

void l2sw_enable_port(struct l2sw_mac *mac);

void regs_print(void);


#endif
