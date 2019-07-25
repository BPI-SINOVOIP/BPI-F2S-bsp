#ifndef __REG_HDMITX_H__
#define __REG_HDMITX_H__

#include <mach/io_map.h>
#include <mach/hdmitx.h>

typedef volatile struct reg_hdmitx_s
{
	// GROUP 380 HDMI G0
	unsigned int hdmi_vendor_id;                          //0, 0x0
	unsigned int hdmi_device_id;                          //1, 0x1
	unsigned int hdmi_revision;                           //2, 0x2
	unsigned int hdmi_reserved_0003;                      //3, 0x3
	unsigned int hdmi_reserved_0004;                      //4, 0x4
	unsigned int hdmi_pwr_ctrl;                           //5, 0x5
	unsigned int hdmi_sw_reset;                           //6, 0x6
	unsigned int hdmi_system_status;                      //7, 0x7
	unsigned int hdmi_system_ctrl1;                       //8, 0x8
	unsigned int hdmi_reserved_0009;                      //9, 0x9
	unsigned int hdmi_system_ctrl2;                       //10, 0xa
	unsigned int hdmi_system_ctrl3;                       //11, 0xb
	unsigned int hdmi_system_ctrl4;                       //12, 0xc
	unsigned int hdmi_reserved_000d;                      //13, 0xd
	unsigned int hdmi_reserved_000e;                      //14, 0xe
	unsigned int hdmi_system_ctrl5;                       //15, 0xf
	unsigned int hdmi_hdcp_ctrl1;                         //16, 0x10
	unsigned int hdmi_bksv12;                             //17, 0x11
	unsigned int hdmi_bksv34;                             //18, 0x12
	unsigned int hdmi_bksv5;                              //19, 0x13
	unsigned int hdmi_mi12;                               //20, 0x14
	unsigned int hdmi_mi23;                               //21, 0x15
	unsigned int hdmi_mi56;                               //22, 0x16
	unsigned int hdmi_mi78;                               //23, 0x17
	unsigned int hdmi_aksv12;                             //24, 0x18
	unsigned int hdmi_aksv23;                             //25, 0x19
	unsigned int hdmi_aksv5;                              //26, 0x1a
	unsigned int hdmi_ri_cmp;                             //27, 0x1b
	unsigned int hdmi_rj_cmp;                             //28, 0x1c
	unsigned int hdmi_ri_cmp_set;                         //29, 0x1d
	unsigned int hdmi_framecnt;                           //30, 0x1e
	unsigned int hdmi_reserved_001f;                      //31, 0x1f

	// GROUP 381 HDMI G1
	unsigned int hdmi_autoricmp;                          //0, 0x0
	unsigned int hdmi_hdcpsts;                            //1, 0x1
	unsigned int hdmi_rxri;                               //2, 0x2
	unsigned int hdmi_rxpj;                               //3, 0x3
	unsigned int hdmi_hdcp_test;                          //4, 0x4
	unsigned int hdmi_hdcp_test_result;                   //5, 0x5
	unsigned int hdmi_apo_spdif_chnl_sts0;                //6, 0x6
	unsigned int hdmi_apo_spdif_chnl_sts1;                //7, 0x7
	unsigned int hdmi_apo_spdif_chnl_sts2;                //8, 0x8
	unsigned int hdmi_acr_config2;                        //9, 0x9
	unsigned int hdmi_asp_urg_th;                         //10, 0xa
	unsigned int hdmi_reserved_010b;                      //11, 0xb
	unsigned int hdmi_blvctrl;                            //12, 0xc
	unsigned int hdmi_blvpa0;                             //13, 0xd
	unsigned int hdmi_blvpa1;                             //14, 0xe
	unsigned int hdmi_blvpa2;                             //15, 0xf
	unsigned int hdmi_video_ctrl1;                        //16, 0x10
	unsigned int hdmi_video_in_h_sts;                     //17, 0x11
	unsigned int hdmi_video_in_v_sts;                     //18, 0x12
	unsigned int hdmi_video_dither_ctrl;                  //19, 0x13
	unsigned int hdmi_video_pat_gen1;                     //20, 0x14
	unsigned int hdmi_video_pat_gen2;                     //21, 0x15
	unsigned int hdmi_video_pat_gen3;                     //22, 0x16
	unsigned int hdmi_video_pat_gen4;                     //23, 0x17
	unsigned int hdmi_video_pat_gen5;                     //24, 0x18
	unsigned int hdmi_video_pat_gen6;                     //25, 0x19
	unsigned int hdmi_video_pat_gen7;                     //26, 0x1a
	unsigned int hdmi_video_pat_gen8;                     //27, 0x1b
	unsigned int hdmi_video_pat_gen9;                     //28, 0x1c
	unsigned int hdmi_csc_coeff1;                         //29, 0x1d
	unsigned int hdmi_csc_coeff2;                         //30, 0x1e
	unsigned int hdmi_csc_coeff3;                         //31, 0x1f

	// GROUP 382 HDMI G2
	unsigned int hdmi_csc_coeff4;                         //0, 0x0
	unsigned int hdmi_csc_coeff7;                         //1, 0x1
	unsigned int hdmi_csc_coeff8;                         //2, 0x2
	unsigned int hdmi_csc_coeff9;                         //3, 0x3
	unsigned int hdmi_csc_coeff10;                        //4, 0x4
	unsigned int hdmi_csc_coeff13;                        //5, 0x5
	unsigned int hdmi_csc_coeff14;                        //6, 0x6
	unsigned int hdmi_csc_coeff15;                        //7, 0x7
	unsigned int hdmi_csc_coeff16;                        //8, 0x8
	unsigned int hdmi_csc_coeff19;                        //9, 0x9
	unsigned int hdmi_csc_coeff20;                        //10, 0xa
	unsigned int hdmi_csc_coeff21;                        //11, 0xb
	unsigned int hdmi_video_format;                       //12, 0xc
	unsigned int hdmi_reserved_020d;                      //13, 0xd
	unsigned int hdmi_audio_sw_cts1;                      //14, 0xe
	unsigned int hdmi_audio_sw_cts2;                      //15, 0xf
	unsigned int hdmi_audio_ctrl1;                        //16, 0x10
	unsigned int hdmi_audio_ctrl2;                        //17, 0x11
	unsigned int hdmi_audio_spdif_ctrl;                   //18, 0x12
	unsigned int hdmi_audio_spdif_sw_ui;                  //19, 0x13
	unsigned int hdmi_audio_spdif_sw_3ui;                 //20, 0x14
	unsigned int hdmi_audio_spdif_hw_ui;                  //21, 0x15
	unsigned int hdmi_audio_spdif_hw_3ui;                 //22, 0x16
	unsigned int hdmi_audio_chnl_sts1;                    //23, 0x17
	unsigned int hdmi_audio_chnl_sts2;                    //24, 0x18
	unsigned int hdmi_audio_chnl_sts3;                    //25, 0x19
	unsigned int hdmi_arc_config1;                        //26, 0x1a
	unsigned int hdmi_arc_n_value1;                       //27, 0x1b
	unsigned int hdmi_arc_n_value2;                       //28, 0x1c
	unsigned int hdmi_arc_hw_cts1;                        //29, 0x1d
	unsigned int hdmi_arc_hw_cts2;                        //30, 0x1e
	unsigned int hdmi_reserved_021f;                      //31, 0x1f

	// GROUP 383 HDMI G3
	unsigned int hdmi_intr_ctrl;                          //0, 0x0
	unsigned int hdmi_intr0_unmask;                       //1, 0x1
	unsigned int hdmi_intr1_unmask;                       //2, 0x2
	unsigned int hdmi_intr2_unmask;                       //3, 0x3
	unsigned int hdmi_intr0_sts;                          //4, 0x4
	unsigned int hdmi_intr1_sts;                          //5, 0x5
	unsigned int hdmi_intr2_sts;                          //6, 0x6
	unsigned int hdmi_ddc_master_set;                     //7, 0x7
	unsigned int hdmi_ddc_slv_device_addr;                //8, 0x8
	unsigned int hdmi_ddc_slv_seg_addr;                   //9, 0x9
	unsigned int hdmi_ddc_slv_reg_offset;                 //10, 0xa
	unsigned int hdmi_ddc_data_cnt;                       //11, 0xb
	unsigned int hdmi_ddc_cmd;                            //12, 0xc
	unsigned int hdmi_ddc_sts;                            //13, 0xd
	unsigned int hdmi_ddc_data;                           //14, 0xe
	unsigned int hdmi_ddc_fifodata_cnt;                   //15, 0xf
	unsigned int hdmi_cec_config1;                        //16, 0x10
	unsigned int hdmi_cec_gpio;                           //17, 0x11
	unsigned int hdmi_cec_data_send;                      //18, 0x12
	unsigned int hdmi_cec_config2;                        //19, 0x13
	unsigned int hdmi_cec_config3;                        //20, 0x14
	unsigned int hdmi_cec_cmd;                            //21, 0x15
	unsigned int hdmi_cec_timer;                          //22, 0x16
	unsigned int hdmi_cec_timer_sts;                      //23, 0x17
	unsigned int hdmi_cec_config4;                        //24, 0x18
	unsigned int hdmi_cec_data_rcv;                       //25, 0x19
	unsigned int hdmi_cec_sts;                            //26, 0x1a
	unsigned int hdmi_cec_sts2;                           //27, 0x1b
	unsigned int hdmi_cec_sts3;                           //28, 0x1c
	unsigned int hdmi_cec_sts4;                           //29, 0x1d
	unsigned int hdmi_reserved_031e;                      //30, 0x1e
	unsigned int hdmi_reserved_031f;                      //31, 0x1f

	// GROUP 384 HDMI G4
	unsigned int hdmi_cec_send_byte12;                    //0, 0x0
	unsigned int hdmi_cec_send_byte34;                    //1, 0x1
	unsigned int hdmi_cec_send_byte56;                    //2, 0x2
	unsigned int hdmi_cec_send_byte78;                    //3, 0x3
	unsigned int hdmi_cec_send_byte910;                   //4, 0x4
	unsigned int hdmi_cec_send_byte1112;                  //5, 0x5
	unsigned int hdmi_cec_send_byte1314;                  //6, 0x6
	unsigned int hdmi_cec_send_byte1516;                  //7, 0x7
	unsigned int hdmi_cec_rcv_byte12;                     //8, 0x8
	unsigned int hdmi_cec_rcv_byte34;                     //9, 0x9
	unsigned int hdmi_cec_rcv_byte56;                     //10, 0xa
	unsigned int hdmi_cec_rcv_byte78;                     //11, 0xb
	unsigned int hdmi_cec_rcv_byte910;                    //12, 0xc
	unsigned int hdmi_cec_rcv_byte1112;                   //13, 0xd
	unsigned int hdmi_cec_rcv_byte1314;                   //14, 0xe
	unsigned int hdmi_cec_rcv_byte1516;                   //15, 0xf
	unsigned int hdmi_universal_infoframe_hb01;           //16, 0x10
	unsigned int hdmi_universal_infoframe_hb2;            //17, 0x11
	unsigned int hdmi_universal_packetbody01;             //18, 0x12
	unsigned int hdmi_universal_packetbody23;             //19, 0x13
	unsigned int hdmi_universal_packetbody45;             //20, 0x14
	unsigned int hdmi_universal_packetbody67;             //21, 0x15
	unsigned int hdmi_universal_packetbody89;             //22, 0x16
	unsigned int hdmi_universal_packetbody1011;           //23, 0x17
	unsigned int hdmi_universal_packetbody1213;           //24, 0x18
	unsigned int hdmi_universal_packetbody1415;           //25, 0x19
	unsigned int hdmi_universal_packetbody1617;           //26, 0x1a
	unsigned int hdmi_universal_packetbody1819;           //27, 0x1b
	unsigned int hdmi_universal_packetbody2021;           //28, 0x1c
	unsigned int hdmi_universal_packetbody2223;           //29, 0x1d
	unsigned int hdmi_universal_packetbody2425;           //30, 0x1e
	unsigned int hdmi_universal_packetbody2627;           //31, 0x1f

	// GROUP 385 HDMI G5
	unsigned int hdmi_infoframe_ctrl1;                    //0, 0x0
	unsigned int hdmi_infoframe_ctrl2;                    //1, 0x1
	unsigned int hdmi_infoframe_ctrl3;                    //2, 0x2
	unsigned int hdmi_infoframe_ctrl4;                    //3, 0x3
	unsigned int hdmi_infoframe_ctrl5;                    //4, 0x4
	unsigned int hdmi_infoframe_ctrl6;                    //5, 0x5
	unsigned int hdmi_avi_infoframe01;                    //6, 0x6
	unsigned int hdmi_avi_infoframe23;                    //7, 0x7
	unsigned int hdmi_avi_infoframe45;                    //8, 0x8
	unsigned int hdmi_avi_infoframe67;                    //9, 0x9
	unsigned int hdmi_avi_infoframe89;                    //10, 0xa
	unsigned int hdmi_avi_infoframe1011;                  //11, 0xb
	unsigned int hdmi_avi_infoframe1213;                  //12, 0xc
	unsigned int hdmi_audio_infoframe01;                  //13, 0xd
	unsigned int hdmi_audio_infoframe23;                  //14, 0xe
	unsigned int hdmi_audio_infoframe45;                  //15, 0xf
	unsigned int hdmi_audio_infoframe67;                  //16, 0x10
	unsigned int hdmi_audio_infoframe89;                  //17, 0x11
	unsigned int hdmi_audio_infoframe1011;                //18, 0x12
	unsigned int hdmi_general_control_packet;             //19, 0x13
	unsigned int hdmi_acp_packet_header;                  //20, 0x14
	unsigned int hdmi_acp_packet_body01;                  //21, 0x15
	unsigned int hdmi_acp_packet_body23;                  //22, 0x16
	unsigned int hdmi_acp_packet_body45;                  //23, 0x17
	unsigned int hdmi_acp_packet_body67;                  //24, 0x18
	unsigned int hdmi_acp_packet_body89;                  //25, 0x19
	unsigned int hdmi_acp_packet_body1011;                //26, 0x1a
	unsigned int hdmi_acp_packet_body1213;                //27, 0x1b
	unsigned int hdmi_acp_packet_body1415;                //28, 0x1c
	unsigned int hdmi_acp_packet_body1617;                //29, 0x1d
	unsigned int hdmi_acp_packet_body1819;                //30, 0x1e
	unsigned int hdmi_acp_packet_body2021;                //31, 0x1f

	// GROUP 386 HDMI G6
	unsigned int hdmi_acp_packet_body2223;                //0, 0x0
	unsigned int hdmi_acp_packet_body2425;                //1, 0x1
	unsigned int hdmi_acp_packet_body2627;                //2, 0x2
	unsigned int hdmi_vendor_specific_infoframe;          //3, 0x3
	unsigned int hdmi_vendor_specific_length;             //4, 0x4
	unsigned int hdmi_spd_infoframe_pb01;                 //5, 0x5
	unsigned int hdmi_spd_infoframe_pb23;                 //6, 0x6
	unsigned int hdmi_spd_infoframe_pb45;                 //7, 0x7
	unsigned int hdmi_spd_infoframe_pb67;                 //8, 0x8
	unsigned int hdmi_spd_infoframe_pb89;                 //9, 0x9
	unsigned int hdmi_spd_infoframe_pb1011;               //10, 0xa
	unsigned int hdmi_spd_infoframe_pb1213;               //11, 0xb
	unsigned int hdmi_spd_infoframe_pb1415;               //12, 0xc
	unsigned int hdmi_spd_infoframe_pb1617;               //13, 0xd
	unsigned int hdmi_spd_infoframe_pb1819;               //14, 0xe
	unsigned int hdmi_spd_infoframe_pb2021;               //15, 0xf
	unsigned int hdmi_spd_infoframe_pb2223;               //16, 0x10
	unsigned int hdmi_spd_infoframe_pb2425;               //17, 0x11
	unsigned int hdmi_isrc_packet_header;                 //18, 0x12
	unsigned int hdmi_isrc1_pb01;                         //19, 0x13
	unsigned int hdmi_isrc1_pb23;                         //20, 0x14
	unsigned int hdmi_isrc1_pb45;                         //21, 0x15
	unsigned int hdmi_isrc1_pb67;                         //22, 0x16
	unsigned int hdmi_isrc1_pb89;                         //23, 0x17
	unsigned int hdmi_isrc1_pb1011;                       //24, 0x18
	unsigned int hdmi_isrc1_pb1213;                       //25, 0x19
	unsigned int hdmi_isrc1_pb1415;                       //26, 0x1a
	unsigned int hdmi_isrc2_pb01;                         //27, 0x1b
	unsigned int hdmi_isrc2_pb23;                         //28, 0x1c
	unsigned int hdmi_isrc2_pb45;                         //29, 0x1d
	unsigned int hdmi_isrc2_pb67;                         //30, 0x1e
	unsigned int hdmi_isrc2_pb89;                         //31, 0x1f

	// GROUP 387 HDMI G7
	unsigned int hdmi_isrc2_pb1011;                       //0, 0x0
	unsigned int hdmi_isrc2_pb1213;                       //1, 0x1
	unsigned int hdmi_isrc2_pb1415;                       //2, 0x2
	unsigned int hdmi_gamut_metadata_packet_header;       //3, 0x3
	unsigned int hdmi_gamut_metadata_packet_pb01;         //4, 0x4
	unsigned int hdmi_gamut_metadata_packet_pb23;         //5, 0x5
	unsigned int hdmi_gamut_metadata_packet_pb45;         //6, 0x6
	unsigned int hdmi_gamut_metadata_packet_pb67;         //7, 0x7
	unsigned int hdmi_gamut_metadata_packet_pb89;         //8, 0x8
	unsigned int hdmi_gamut_metadata_packet_pb1011;       //9, 0x9
	unsigned int hdmi_gamut_metadata_packet_pb1213;       //10, 0xa
	unsigned int hdmi_gamut_metadata_packet_pb1415;       //11, 0xb
	unsigned int hdmi_gamut_metadata_packet_pb1617;       //12, 0xc
	unsigned int hdmi_gamut_metadata_packet_pb1819;       //13, 0xd
	unsigned int hdmi_gamut_metadata_packet_pb2021;       //14, 0xe
	unsigned int hdmi_gamut_metadata_packet_pb2223;       //15, 0xf
	unsigned int hdmi_gamut_metadata_packet_pb2425;       //16, 0x10
	unsigned int hdmi_gamut_metadata_packet_pb2627;       //17, 0x11
	unsigned int hdmi_mpeg_source_infoframe_pb01;         //18, 0x12
	unsigned int hdmi_mpeg_source_infoframe_pb23;         //19, 0x13
	unsigned int hdmi_mpeg_source_infoframe_pb45;         //20, 0x14
	unsigned int hdmi_mpeg_source_infoframe_pb67;         //21, 0x15
	unsigned int hdmi_mpeg_source_infoframe_pb89;         //22, 0x16
	unsigned int hdmi_mpeg_source_infoframe_pb10;         //23, 0x17
	unsigned int hdmi_reserved_0718;                      //24, 0x18
	unsigned int hdmi_reserved_0719;                      //25, 0x19
	unsigned int hdmi_tmdstx_ctrl1;                       //26, 0x1a
	unsigned int hdmi_tmdstx_ctrl2;                       //27, 0x1b
	unsigned int hdmi_tmdstx_ctrl3;                       //28, 0x1c
	unsigned int hdmi_tmdstx_ctrl4;                       //29, 0x1d
	unsigned int hdmi_tmdstx_ctrl5;                       //30, 0x1e
	unsigned int hdmi_reserved_071f;                      //31, 0x1f

} reg_hdmitx_t;

typedef volatile struct reg_moon0_s
{
	unsigned int stamp; // 0.0
	unsigned int clken[10]; // 0.1
	unsigned int gclken[10]; // 0.11
	unsigned int reset[10]; // 0.21
	unsigned int hw_cfg; // 0.31      
} reg_moon0_t;

typedef volatile struct reg_moon4_s
{
	unsigned int pllsp_ctl[7];  // 4.0
	unsigned int plla_ctl[5];   // 4.7
	unsigned int plle_ctl;      // 4.12
	unsigned int pllf_ctl;      // 4.13
	unsigned int plltv_ctl[3];  // 4.14
	unsigned int usbc_ctl;      // 4.17
	unsigned int uphy0_ctl[4];  // 4.18
	unsigned int uphy1_ctl[4];  // 4.22
	unsigned int pllsys;        // 4.26
	unsigned int clk_sel0;      // 4.27
	unsigned int probe_sel;     // 4.28
	unsigned int misc_ctl_0;    // 4.29
	unsigned int uphy0_sts;     // 4.30
	unsigned int otp_st;        // 4.31
} reg_moon4_t;

typedef volatile struct reg_moon5_s {
	unsigned int sft_cfg[32];
} reg_moon5_t;

typedef volatile struct reg_moon1_s {
	unsigned int sft_cfg[32];
} reg_moon1_t;

#endif // end of #ifndef __REG_HDMITX_H__