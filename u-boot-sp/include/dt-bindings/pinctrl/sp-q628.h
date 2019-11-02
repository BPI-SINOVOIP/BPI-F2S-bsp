/*
 * Header providing constants for sp7021 pinctrl bindings.
 *
 * Copyright (C) 2019 hammer hsieh <hammer.hsieh@sunplus.com>
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 at the following locations:
 *
 */

#ifndef _DT_BINDINGS_PINMUX_Q628_H
#define _DT_BINDINGS_PINMUX_Q628_H

/* For pinmux select */
#define	G_MX_UNDEFINED   0 
#define	G_MX00_GPIO_P0_0 0 
#define	G_MX01_GPIO_P0_1 0 
#define	G_MX02_GPIO_P0_2 0 
#define	G_MX03_GPIO_P0_3 0 
#define	G_MX04_GPIO_P0_4 0 
#define	G_MX05_GPIO_P0_5 0 
#define	G_MX06_GPIO_P0_6 0 
#define	G_MX07_GPIO_P0_7 0 
#define	G_MX08_GPIO_P1_0 1 
#define	G_MX09_GPIO_P1_1 2 
#define	G_MX10_GPIO_P1_2 3 
#define	G_MX11_GPIO_P1_3 4 
#define	G_MX12_GPIO_P1_4 5 
#define	G_MX13_GPIO_P1_5 6 
#define	G_MX14_GPIO_P1_6 7 
#define	G_MX15_GPIO_P1_7 8 
#define	G_MX16_GPIO_P2_0 9 
#define	G_MX17_GPIO_P2_1 10
#define	G_MX18_GPIO_P2_2 11
#define	G_MX19_GPIO_P2_3 12
#define	G_MX20_GPIO_P2_4 13
#define	G_MX21_GPIO_P2_5 14
#define	G_MX22_GPIO_P2_6 15
#define	G_MX23_GPIO_P2_7 16
#define	G_MX24_GPIO_P3_0 17
#define	G_MX25_GPIO_P3_1 18
#define	G_MX26_GPIO_P3_2 19
#define	G_MX27_GPIO_P3_3 20
#define	G_MX28_GPIO_P3_4 21
#define	G_MX29_GPIO_P3_5 22
#define	G_MX30_GPIO_P3_6 23
#define	G_MX31_GPIO_P3_7 24
#define	G_MX32_GPIO_P4_0 25
#define	G_MX33_GPIO_P4_1 26
#define	G_MX34_GPIO_P4_2 27
#define	G_MX35_GPIO_P4_3 28
#define	G_MX36_GPIO_P4_4 29
#define	G_MX37_GPIO_P4_5 30
#define	G_MX38_GPIO_P4_6 31
#define	G_MX39_GPIO_P4_7 32
#define	G_MX40_GPIO_P5_0 33
#define	G_MX41_GPIO_P5_1 34
#define	G_MX42_GPIO_P5_2 35
#define	G_MX43_GPIO_P5_3 36
#define	G_MX44_GPIO_P5_4 37
#define	G_MX45_GPIO_P5_5 38
#define	G_MX46_GPIO_P5_6 39
#define	G_MX47_GPIO_P5_7 40
#define	G_MX48_GPIO_P6_0 41
#define	G_MX49_GPIO_P6_1 42
#define	G_MX50_GPIO_P6_2 43
#define	G_MX51_GPIO_P6_3 44
#define	G_MX52_GPIO_P6_4 45
#define	G_MX53_GPIO_P6_5 46
#define	G_MX54_GPIO_P6_6 47
#define	G_MX55_GPIO_P6_7 48
#define	G_MX56_GPIO_P7_0 49
#define	G_MX57_GPIO_P7_1 50
#define	G_MX58_GPIO_P7_2 51
#define	G_MX59_GPIO_P7_3 52
#define	G_MX60_GPIO_P7_4 53
#define	G_MX61_GPIO_P7_5 54
#define	G_MX62_GPIO_P7_6 55
#define	G_MX63_GPIO_P7_7 56
#define	G_MX64_GPIO_P8_0 57
#define	G_MX65_GPIO_P8_1 58
#define	G_MX66_GPIO_P8_2 59
#define	G_MX67_GPIO_P8_3 60
#define	G_MX68_GPIO_P8_4 61
#define	G_MX69_GPIO_P8_5 62
#define	G_MX70_GPIO_P8_6 63
#define	G_MX71_GPIO_P8_7 64

		//G2
#define	PMX_L2SW_CLK_OUT			0x02004000			// (0-64, 0)
#define	PMX_L2SW_MAC_SMI_MDC		0x02004008			// (0-64, 8)	
#define	PMX_L2SW_LED_FLASH0			0x02014000			// (0-64, 0)	
#define	PMX_L2SW_LED_FLASH1			0x02014008			// (0-64, 8)
#define	PMX_L2SW_LED_ON0			0x02024000			// (0-64, 0)
#define	PMX_L2SW_LED_ON1			0x02024008			// (0-64, 8)
#define	PMX_L2SW_MAC_SMI_MDIO		0x02034000			// (0-64, 0)
#define	PMX_L2SW_P0_MAC_RMII_TXEN   0x02034008			// (0-64, 8)
#define	PMX_L2SW_P0_MAC_RMII_TXD0   0x02044000			// (0-64, 0)	
#define	PMX_L2SW_P0_MAC_RMII_TXD1   0x02044008			// (0-64, 8)
#define	PMX_L2SW_P0_MAC_RMII_CRSDV  0x02054000			// (0-64, 0)	
#define	PMX_L2SW_P0_MAC_RMII_RXD0   0x02054008			// (0-64, 8)
#define	PMX_L2SW_P0_MAC_RMII_RXD1   0x02064000			// (0-64, 0)	
#define	PMX_L2SW_P0_MAC_RMII_RXER   0x02064008			// (0-64, 8)
#define	PMX_L2SW_P1_MAC_RMII_TXEN   0x02074000			// (0-64, 0)	
#define	PMX_L2SW_P1_MAC_RMII_TXD0   0x02074008			// (0-64, 8)
#define	PMX_L2SW_P1_MAC_RMII_TXD1   0x02084000			// (0-64, 0)	
#define	PMX_L2SW_P1_MAC_RMII_CRSDV  0x02084008			// (0-64, 8)
#define	PMX_L2SW_P1_MAC_RMII_RXD0   0x02094000			// (0-64, 0)	
#define	PMX_L2SW_P1_MAC_RMII_RXD1   0x02094008			// (0-64, 8)
#define	PMX_L2SW_P1_MAC_RMII_RXER   0x020a4000			// (0-64, 0)
#define	PMX_DAISY_MODE				0x020a4008			// (0-64, 8)
#define	PMX_SDIO_CLK				0x020b4000			// (0-64, 0)
#define	PMX_SDIO_CMD				0x020b4008			// (0-64, 8)
#define	PMX_SDIO_D0					0x020c4000			// (0-64, 0)
#define	PMX_SDIO_D1     0x020c4008			// (0-64, 8)
#define	PMX_SDIO_D2     0x020d4000			// (0-64, 0)
#define	PMX_SDIO_D3     0x020d4008			// (0-64, 8)
#define	PMX_PWM0        0x020e4000			// (0-64, 0)
#define	PMX_PWM1        0x020e4008			// (0-64, 8)
#define	PMX_PWM2        0x020f4000			// (0-64, 0)
#define	PMX_PWM3        0x020f4008			// (0-64, 8)
#define	PMX_PWM4        0x02104000			// (0-64, 0)
#define	PMX_PWM5        0x02104008			// (0-64, 8)
#define	PMX_PWM6        0x02114000			// (0-64, 0)
#define	PMX_PWM7        0x02114008			// (0-64, 8)
#define	PMX_ICM0_D      0x02124000			// (0-64, 0)
#define	PMX_ICM1_D      0x02124008			// (0-64, 8)
#define	PMX_ICM2_D      0x02134000			// (0-64, 0)
#define	PMX_ICM3_D      0x02134008			// (0-64, 8)
#define	PMX_ICM0_CLK    0x02144000			// (0-64, 0)
#define	PMX_ICM1_CLK    0x02144008			// (0-64, 8)
#define	PMX_ICM2_CLK    0x02154000			// (0-64, 0)
#define	PMX_ICM3_CLK    0x02154008			// (0-64, 8)
#define	PMX_SPIM0_INT   0x02164000			// (0-64, 0)
#define	PMX_SPIM0_CLK   0x02164008			// (0-64, 8)
#define	PMX_SPIM0_CEN   0x02174000			// (0-64, 0)
#define	PMX_SPIM0_DO    0x02174008			// (0-64, 8)
#define	PMX_SPIM0_DI    0x02184000			// (0-64, 0)
#define	PMX_SPIM1_INT   0x02184008			// (0-64, 8)
#define	PMX_SPIM1_CLK   0x02194000			// (0-64, 0)
#define	PMX_SPIM1_CEN   0x02194008			// (0-64, 8)
#define	PMX_SPIM1_DO    0x021a4000			// (0-64, 0)
#define	PMX_SPIM1_DI    0x021a4008			// (0-64, 8)
#define	PMX_SPIM2_INT   0x021b4000			// (0-64, 0)
#define	PMX_SPIM2_CLK   0x021b4008			// (0-64, 8)
#define	PMX_SPIM2_CEN   0x021c4000			// (0-64, 0)
#define	PMX_SPIM2_DO    0x021c4008			// (0-64, 8)
#define	PMX_SPIM2_DI    0x021d4000			// (0-64, 0)
#define	PMX_SPIM3_INT   0x021d4008			// (0-64, 8)
#define	PMX_SPIM3_CLK   0x021e4000			// (0-64, 0)
#define	PMX_SPIM3_CEN   0x021e4008			// (0-64, 8)
#define	PMX_SPIM3_DO    0x021f4000			// (0-64, 0)
#define	PMX_SPIM3_DI    0x021f4008			// (0-64, 8)
			//G3
#define	PMX_SPI0S_INT    0x03004000			// (0-64, 0)
#define	PMX_SPI0S_CLK    0x03004008			// (0-64, 8)
#define	PMX_SPI0S_EN     0x03014000			// (0-64, 0)
#define	PMX_SPI0S_DO     0x03014008			// (0-64, 8)
#define	PMX_SPI0S_DI     0x03024000			// (0-64, 0)
#define	PMX_SPI1S_INT    0x03024008			// (0-64, 8)
#define	PMX_SPI1S_CLK    0x03034000			// (0-64, 0)
#define	PMX_SPI1S_EN     0x03034008			// (0-64, 8)
#define	PMX_SPI1S_DO     0x03044000			// (0-64, 0)
#define	PMX_SPI1S_DI     0x03044008			// (0-64, 8)
#define	PMX_SPI2S_INT    0x03054000			// (0-64, 0)
#define	PMX_SPI2S_CLK    0x03054008			// (0-64, 8)
#define	PMX_SPI2S_EN     0x03064000			// (0-64, 0)
#define	PMX_SPI2S_DO     0x03064008			// (0-64, 8)
#define	PMX_SPI2S_DI     0x03074000			// (0-64, 0)	
#define	PMX_SPI3S_INT    0x03074008			// (0-64, 8)
#define	PMX_SPI3S_CLK    0x03084000			// (0-64, 0)
#define	PMX_SPI3S_EN     0x03084008			// (0-64, 8)
#define	PMX_SPI3S_DO     0x03094000			// (0-64, 0)
#define	PMX_SPI3S_DI     0x03094008			// (0-64, 8)
#define	PMX_I2CM0_CK     0x030a4000			// (0-64, 0)
#define	PMX_I2CM0_DAT    0x030a4008			// (0-64, 8)
#define	PMX_I2CM1_CK     0x030b4000			// (0-64, 0)	
#define	PMX_I2CM1_DAT    0x030b4008			// (0-64, 8)
#define	PMX_I2CM2_CK     0x030c4000			// (0-64, 0)		
#define	PMX_I2CM2_D      0x030c4008			// (0-64, 8)
#define	PMX_I2CM3_CK     0x030d4000			// (0-64, 0)		
#define	PMX_I2CM3_D      0x030d4008			// (0-64, 8)
#define	PMX_UA1_TX       0x030e4000			// (0-64, 0)		
#define	PMX_UA1_RX       0x030e4008			// (0-64, 8)
#define	PMX_UA1_CTS      0x030f4000			// (0-64, 0)	
#define	PMX_UA1_RTS      0x030f4008			// (0-64, 8)
#define	PMX_UA2_TX       0x03104000			// (0-64, 0)	
#define	PMX_UA2_RX       0x03104008			// (0-64, 8)
#define	PMX_UA2_CTS      0x03114000			// (0-64, 0)	
#define	PMX_UA2_RTS      0x03114008			// (0-64, 8)
#define	PMX_UA3_TX       0x03124000			// (0-64, 0)	
#define	PMX_UA3_RX       0x03124008			// (0-64, 8)
#define	PMX_UA3_CTS      0x03134000			// (0-64, 0)	
#define	PMX_UA3_RTS      0x03134008			// (0-64, 8)
#define	PMX_UA4_TX       0x03144000			// (0-64, 0)	
#define	PMX_UA4_RX       0x03144008			// (0-64, 8)
#define	PMX_UA4_CTS      0x03154000			// (0-64, 0)	
#define	PMX_UA4_RTS      0x03154008			// (0-64, 8)
#define	PMX_TIMER0_INT   0x03164000			// (0-64, 0)		
#define	PMX_TIMER1_INT   0x03164008			// (0-64, 8)
#define	PMX_TIMER2_INT   0x03174000			// (0-64, 0)	
#define	PMX_TIMER3_INT   0x03174008			// (0-64, 8)
#define	PMX_GPIO_INT0    0x03184000			// (0-64, 0)	
#define	PMX_GPIO_INT1    0x03184008			// (0-64, 8)	
#define	PMX_GPIO_INT2    0x03194000			// (0-64, 0)	
#define	PMX_GPIO_INT3    0x03194008			// (0-64, 8)	
#define	PMX_GPIO_INT4    0x031a4000			// (0-64, 0)	
#define	PMX_GPIO_INT5    0x031a4008			// (0-64, 8)	
#define	PMX_GPIO_INT6    0x031b4000			// (0-64, 0)	
#define	PMX_GPIO_INT7    0x031b4008			// (0-64, 8)

/* For gpio select */
#define	PINSEL_00   0    /*G_MX00_GPIO_P0_0*/   
#define	PINSEL_01   1    /*G_MX01_GPIO_P0_1*/
#define	PINSEL_02   2    /*G_MX02_GPIO_P0_2*/
#define	PINSEL_03   3    /*G_MX03_GPIO_P0_3*/
#define	PINSEL_04   4    /*G_MX04_GPIO_P0_4*/
#define	PINSEL_05   5    /*G_MX05_GPIO_P0_5*/
#define	PINSEL_06   6    /*G_MX06_GPIO_P0_6*/
#define	PINSEL_07   7    /*G_MX07_GPIO_P0_7*/ 
#define	PINSEL_08   8    /*G_MX08_GPIO_P1_0*/ 
#define	PINSEL_09   9    /*G_MX09_GPIO_P1_1*/ 
#define	PINSEL_10  10    /*G_MX10_GPIO_P1_2*/ 
#define	PINSEL_11  11    /*G_MX11_GPIO_P1_3*/ 
#define	PINSEL_12  12    /*G_MX12_GPIO_P1_4*/ 
#define	PINSEL_13  13    /*G_MX13_GPIO_P1_5*/ 
#define	PINSEL_14  14    /*G_MX14_GPIO_P1_6*/ 
#define	PINSEL_15  15    /*G_MX15_GPIO_P1_7*/ 
#define	PINSEL_16  16    /*G_MX16_GPIO_P2_0*/ 
#define	PINSEL_17  17    /*G_MX17_GPIO_P2_1*/ 
#define	PINSEL_18  18    /*G_MX18_GPIO_P2_2*/ 
#define	PINSEL_19  19    /*G_MX19_GPIO_P2_3*/ 
#define	PINSEL_20  20    /*G_MX20_GPIO_P2_4*/ 
#define	PINSEL_21  21    /*G_MX21_GPIO_P2_5*/ 
#define	PINSEL_22  22    /*G_MX22_GPIO_P2_6*/ 
#define	PINSEL_23  23    /*G_MX23_GPIO_P2_7*/ 
#define	PINSEL_24  24    /*G_MX24_GPIO_P3_0*/ 
#define	PINSEL_25  25    /*G_MX25_GPIO_P3_1*/ 
#define	PINSEL_26  26    /*G_MX26_GPIO_P3_2*/ 
#define	PINSEL_27  27    /*G_MX27_GPIO_P3_3*/ 
#define	PINSEL_28  28    /*G_MX28_GPIO_P3_4*/ 
#define	PINSEL_29  29    /*G_MX29_GPIO_P3_5*/ 
#define	PINSEL_30  30    /*G_MX30_GPIO_P3_6*/ 
#define	PINSEL_31  31    /*G_MX31_GPIO_P3_7*/ 
#define	PINSEL_32  32    /*G_MX32_GPIO_P4_0*/ 
#define	PINSEL_33  33    /*G_MX33_GPIO_P4_1*/ 
#define	PINSEL_34  34    /*G_MX34_GPIO_P4_2*/ 
#define	PINSEL_35  35    /*G_MX35_GPIO_P4_3*/ 
#define	PINSEL_36  36    /*G_MX36_GPIO_P4_4*/ 
#define	PINSEL_37  37    /*G_MX37_GPIO_P4_5*/ 
#define	PINSEL_38  38    /*G_MX38_GPIO_P4_6*/ 
#define	PINSEL_39  39    /*G_MX39_GPIO_P4_7*/ 
#define	PINSEL_40  40    /*G_MX40_GPIO_P5_0*/ 
#define	PINSEL_41  41    /*G_MX41_GPIO_P5_1*/ 
#define	PINSEL_42  42    /*G_MX42_GPIO_P5_2*/ 
#define	PINSEL_43  43    /*G_MX43_GPIO_P5_3*/ 
#define	PINSEL_44  44    /*G_MX44_GPIO_P5_4*/ 
#define	PINSEL_45  45    /*G_MX45_GPIO_P5_5*/ 
#define	PINSEL_46  46    /*G_MX46_GPIO_P5_6*/ 
#define	PINSEL_47  47    /*G_MX47_GPIO_P5_7*/ 
#define	PINSEL_48  48    /*G_MX48_GPIO_P6_0*/ 
#define	PINSEL_49  49    /*G_MX49_GPIO_P6_1*/ 
#define	PINSEL_50  50    /*G_MX50_GPIO_P6_2*/ 
#define	PINSEL_51  51    /*G_MX51_GPIO_P6_3*/ 
#define	PINSEL_52  52    /*G_MX52_GPIO_P6_4*/ 
#define	PINSEL_53  53    /*G_MX53_GPIO_P6_5*/ 
#define	PINSEL_54  54    /*G_MX54_GPIO_P6_6*/ 
#define	PINSEL_55  55    /*G_MX55_GPIO_P6_7*/ 
#define	PINSEL_56  56    /*G_MX56_GPIO_P7_0*/ 
#define	PINSEL_57  57    /*G_MX57_GPIO_P7_1*/ 
#define	PINSEL_58  58    /*G_MX58_GPIO_P7_2*/ 
#define	PINSEL_59  59    /*G_MX59_GPIO_P7_3*/ 
#define	PINSEL_60  60    /*G_MX60_GPIO_P7_4*/ 
#define	PINSEL_61  61    /*G_MX61_GPIO_P7_5*/ 
#define	PINSEL_62  62    /*G_MX62_GPIO_P7_6*/ 
#define	PINSEL_63  63    /*G_MX63_GPIO_P7_7*/ 
#define	PINSEL_64  64    /*G_MX64_GPIO_P8_0*/ 
#define	PINSEL_65  65    /*G_MX65_GPIO_P8_1*/ 
#define	PINSEL_66  66    /*G_MX66_GPIO_P8_2*/ 
#define	PINSEL_67  67    /*G_MX67_GPIO_P8_3*/ 
#define	PINSEL_68  68    /*G_MX68_GPIO_P8_4*/ 
#define	PINSEL_69  69    /*G_MX69_GPIO_P8_5*/ 
#define	PINSEL_70  70    /*G_MX70_GPIO_P8_6*/ 
#define	PINSEL_71  71    /*G_MX71_GPIO_P8_7*/ 

#define	GPIO_OUT		0
#define	GPIO_OUT_OD		1
#define	GPIO_IN			2

#define	GPIO_LOW		0
#define	GPIO_HIGH		1
#define	GPIO_NULL		2

#endif
