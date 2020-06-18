#ifndef _DT_BINDINGS_RST_SUNPLUS_I143_H
#define _DT_BINDINGS_RST_SUNPLUS_I143_H


/* mo_reset0 ~ mo_reset9 */
#define RST_SYSTEM		0x00
#define RST_IOCTL		0x03
#define RST_OTPRX		0x05
#define RST_NIC			0x06
#define RST_BR			0x07
#define RST_SPIFL		0x09
#define RST_SDCTRL0		0x0a
#define RST_PERI0		0x0b
#define RST_U54			0x0d
#define RST_UMCTL2		0x0e
#define RST_PERI1		0x0f

#define RST_DDR_PHY0	0x10
#define RST_GC520		0x11
#define RST_ACHIP		0x12
#define RST_STC0		0x14
#define RST_STC_AV0		0x15
#define RST_STC_AV1		0x16
#define RST_STC_AV2		0x17
#define RST_UA0			0x18
#define RST_UA1			0x19
#define RST_UA2			0x1a
#define RST_UA3			0x1b
#define RST_UA4			0x1c
#define RST_HWUA		0x1d
#define RST_UADMA		0x1f

#define RST_CBDMA0		0x20
//#define RST_CBDMA1		0x21
#define RST_SPI_COMBO_0	0x22
#define RST_SPI_COMBO_1	0x23
#define RST_ISP0		0x24
#define RST_ISP1		0x25
#define RST_ISPAPB0		0x27
#define RST_ISPAPB1		0x28
#define RST_U3PHY		0x29
#define RST_USBC0		0x2a
#define RST_USBC1		0x2b
#define RST_USB30C		0x2c
#define RST_UPHY0		0x2d
#define RST_UPHY1		0x2e
#define RST_UPHY2		0x2f

#define RST_I2CM0		0x30
#define RST_I2CM1		0x31
#define RST_CARD_CTL0	0x3e
#define RST_CARD_CTL1	0x3f

#define RST_CARD_CTL4	0x42
#define RST_DDFCH		0x4b
#define RST_CSIIW0		0x4c
#define RST_CSIIW1		0x4d

#define RST_HDMI_TX		0x50
#define RST_VPOST		0x55
#define RST_VPPDMA		0x56
#define RST_VSCL		0x57
#define RST_DMAPOST		0x58

#define RST_TGEN		0x60
#define RST_DMIX		0x61
#define RST_INTERRUPT	0x6f

#define RST_RGST		0x70
#define RST_RBUS_TOP	0x74
#define RST_OSD1		0x75

#define RST_MAILBOX		0x86
#define RST_DVE			0x8e
#define RST_GPOST0		0x8f

#define RST_OSD0		0x90
#define RST_GPOST1		0x91
#define RST_DISP_PWM	0x92
#define RST_UADBG		0x93
#define RST_DUMMY_MASTER	0x94
#define RST_FIO_CTL		0x95
#define RST_GL2SW		0x97
//#define RST_ICM			0x98
#define RST_AXI_GLOBAL	0x99

#define RST_MAX		      0xA0

#endif
