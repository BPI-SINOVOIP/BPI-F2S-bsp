/********************************************************
 *    ____               __
 *   / __/_ _____  ___  / /_ _____
 *  _\ \/ // / _ \/ _ \/ / // (_-<
 * /___/\_,_/_//_/ .__/_/\_,_/___/
 *              /_/
 * Sunplus Technology Co., Ltd.19, Innovation First Road,
 * Science-Based Industrial Park, Hsin-Chu, Taiwan, R.O.C.
 * ------------------------------------------------------
 * Description  : axi_monitor register file.
 * Create Date  : 2018-05-14 12:41:56 auto generated.
 */

#ifndef __REG_AXI_MONITOR_H__
#define __REG_AXI_MONITOR_H__


typedef volatile struct reg_axi_monitor_s
{
    union {
        struct {
            u32 event_clear      :1;                            /* [W]:<0x0> */
            u32 :3;
            u32 bw_mon_start     :1;                           /* [RW]:<0x0> */
            u32 :3;
            u32 latency_mon_start:1;                           /* [RW]:<0x0> */
            u32 :3;
            u32 bw_update_period :3;                           /* [RW]:<0x0> */
            u32 :1;
            u32 device_id        :7;                            /* [R]:<0x0> */
            u32 :9;
        } bits;
        u32 reg_v;
    } global_monitor_config;                                 /* group 599.0  */

    u32 special_data_config;                                 /* group 599.1  */
    union {
        struct {
            u32 timeout_cycle:16;                              /* [RW]:<0x0> sys clock , 202MHz*/
            u32 :16;
        } bits;
        u32 reg_v;
    } timeout_cycle_config;                                  /* group 599.2  */

    union {
        struct {
            u32 valid_start_addr_msb:16;                       /* [RW]:<0x0> */
            u32 :16;
        } bits;
        u32 reg_v;
    } valid_address_start_msb;                               /* group 599.3  */

    union {
        struct {
            u32 valid_end_addr_msb:16;                         /* [RW]:<0x0> */
            u32 :16;
        } bits;
        u32 reg_v;
    } valid_address_end_msb;                                 /* group 599.4  */

    u32 reserved_0[27];

} reg_axi_monitor_t;

//#define AXI_MONITOR_REGISTER_OFFSET (0x9C000000 + 0x12B80)
//#define axi_monitor_regs ((volatile reg_axi_monitor_t *) AXI_MONITOR_REGISTER_OFFSET)

#endif // end of #ifndef __REG_AXI_MONITOR_H__