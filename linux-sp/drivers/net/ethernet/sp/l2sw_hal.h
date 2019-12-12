#ifndef __L2SW_HAL_H__
#define __L2SW_HAL_H__

#include "l2sw_register.h"
#include "l2sw_define.h"
#include "l2sw_desc.h"


#define HWREG_W(M, N)           (ls2w_reg_base->M = N)
#define HWREG_R(M)              (ls2w_reg_base->M)
#define MOON5REG_W(M, N)        (moon5_reg_base->M = N)
#define MOON5REG_R(M)           (moon5_reg_base->M)


#define MDIO_RW_TIMEOUT_RETRY_NUMBERS 500


int l2sw_reg_base_set(void __iomem *baseaddr);

int moon5_reg_base_set(void __iomem *baseaddr);

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

void write_sw_int_mask0(u32 value);

void write_sw_int_status0(u32 value);

u32 read_sw_int_status0(void);

u32 read_port_ability(void);

int phy_cfg(void);

void l2sw_enable_port(struct l2sw_mac *mac);

void regs_print(void);


#endif
