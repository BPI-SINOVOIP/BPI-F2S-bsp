/********************************************************
 * Copyright (c) 2018 by Sunplus Technology Co., Ltd.
 *    ____               __
 *   / __/_ _____  ___  / /_ _____
 *  _\ \/ // / _ \/ _ \/ / // (_-<
 * /___/\_,_/_//_/ .__/_/\_,_/___/
 *              /_/
 * Sunplus Technology Co. 19, Innovation First Road,
 * Science-Based Industrial Park, Hsin-Chu, Taiwan, R.O.C.
 * ------------------------------------------------------
 *
 * Description :  simple cbdma driver
 * ------------------------------------------------------
 * Rev  Date          Author(s)      Status & Comments
 * ======================================================
 * 0.1  2018/06/28    nz.lu          initial version
 */

#ifndef __AXI_MONITOR_H__
#define __AXI_MONITOR_H__


void axi_mon_interrupt_control_mask(int enable);

// for test
//void axi_mon_test_init();

void axi_mon_unexcept_access_test(void __iomem *axi_mon_regs, void __iomem *axi_id4_regs, void __iomem *axi_id45_regs);
void axi_mon_timeout_test(void __iomem *axi_mon_regs);
//void axi_mon_bw_test();


#endif // __AXI_MONITOR_H__
