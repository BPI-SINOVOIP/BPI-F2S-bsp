/********************************************************
 *    ____               __
 *   / __/_ _____  ___  / /_ _____
 *  _\ \/ // / _ \/ _ \/ / // (_-<
 * /___/\_,_/_//_/ .__/_/\_,_/___/
 *              /_/
 * Sunplus Technology Co., Ltd.19, Innovation First Road,
 * Science-Based Industrial Park, Hsin-Chu, Taiwan, R.O.C.
 * ------------------------------------------------------
 * Description  : axi_monitor_sub register file.
 * Create Date  : 2018-05-14 12:41:56 auto generated.
 */

#ifndef __REG_AXI_MONITOR_SUB_H__
#define __REG_AXI_MONITOR_SUB_H__


typedef volatile struct reg_axi_monitor_sub_s
{
    union {
        struct {
            u32 monitor_enable      :1;                        /* [RW]:<0x0> */
            u32 :3;
            u32 monitor_special_w_data:1;                        /* [RW]:<0x0> */
            u32 :3;
			u32 monitor_special_r_data:1;                        /* [RW]:<0x0> */
            u32 :3;
            u32 monitor_ue_w_access   :1;                        /* [RW]:<0x0> */
            u32 :3;
			u32 monitor_ue_r_access   :1;                        /* [RW]:<0x0> */
            u32 :3;
            u32 monitor_timeout     :1;                        /* [RW]:<0x0> */
            u32 :11;
        } bits;
        u32 reg_v;
    } ip_monitor_control;                                    /* group 600.0  */

    union {
        struct {
            u32 incomplete_wcmd_flag    :1;                      /* [RU]:<0> */
            u32 incomplete_rcmd_flag    :1;                      /* [RU]:<0> */
            u32 :2;
            u32 special_w_data          :1;                      /* [RU]:<0> */
			u32 special_r_data          :1;                      /* [RU]:<0> */
            u32 :2;
            u32 unexpect_w_access       :1;                      /* [RU]:<0> */
            u32 unexpect_r_access       :1;                      /* [RU]:<0> */
            u32 :2;
            u32 timeout_rdata_ready_N   :1;                      /* [RU]:<0> slave ack */
            u32 timeout_wdata_valid_N   :1;                      /* [RU]:<0>  */
            u32 timout_cmd_wdata_ready_N:1;                      /* [RU]:<0> */
            u32 timout_rdata_valid_N    :1;                      /* [RW]:<0> */
            u32 master_id_record        :8;                      /* [RU]:<0> */
            u32 :8;
        } bits;
        u32 reg_v;
    } event_info_record;                                     /* group 600.1  */

    union {
        struct {
            u32 read_bw_ratio :8;                                /* [RU]:<0> */
            u32 :8;
            u32 write_bw_ratio:8;                                /* [RU]:<0> */
            u32 :8;
        } bits;
        u32 reg_v;
    } bw_info_record;                                        /* group 600.2  */

    u32 latency_info_wcmd_cnt;                               /* group 600.3  */
    u32 latency_info_wcmd_execute_cycle;                     /* group 600.4  */
    u32 latency_info_rcmd_cnt;                               /* group 600.5  */
    u32 latency_info_rcmd_execute_cycle;                     /* group 600.6  */
    u32 reserved_0[25];

} reg_axi_monitor_sub_t;

// group 600
#define AXI_MONITOR_SUB_REGISTER_OFFSET (0x9C000000 + 0x12C00)

// group 604
#define axi_mon_sub_cbdma0_regs ((volatile reg_axi_monitor_sub_t *) (AXI_MONITOR_SUB_REGISTER_OFFSET + 128*4))
// group 605
#define axi_mon_sub_cbdma1_regs ((volatile reg_axi_monitor_sub_t *) (AXI_MONITOR_SUB_REGISTER_OFFSET + 128*5))
// group 649
#define axi_mon_sub_sd0_regs ((volatile reg_axi_monitor_sub_t *) (AXI_MONITOR_SUB_REGISTER_OFFSET + 128*49))


#endif // end of #ifndef __REG_AXI_MONITOR_SUB_H__
