/*
 * SP7021 pinmux controller driver.
 * Copyright (C) Sunplus Tech/Tibbo Tech. 2020
 * Author: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "sppctl.h"

// function: GPIO. list of groups (pins)
const unsigned sppctlpins_G[] = {
	D(0,0), D(0,1), D(0,2), D(0,3), D(0,4), D(0,5), D(0,6), D(0,7),
	D(1,0), D(1,1), D(1,2), D(1,3), D(1,4), D(1,5), D(1,6), D(1,7),
	D(2,0), D(2,1), D(2,2), D(2,3), D(2,4), D(2,5), D(2,6), D(2,7),
	D(3,0), D(3,1), D(3,2), D(3,3), D(3,4), D(3,5), D(3,6), D(3,7),
	D(4,0), D(4,1), D(4,2), D(4,3), D(4,4), D(4,5), D(4,6), D(4,7),
	D(5,0), D(5,1), D(5,2), D(5,3), D(5,4), D(5,5), D(5,6), D(5,7),
	D(6,0), D(6,1), D(6,2), D(6,3), D(6,4), D(6,5), D(6,6), D(6,7),
	D(7,0), D(7,1), D(7,2), D(7,3), D(7,4), D(7,5), D(7,6), D(7,7),
	D(8,0), D(8,1), D(8,2), D(8,3), D(8,4), D(8,5), D(8,6), D(8,7),
	D(9,0), D(9,1), D(9,2), D(9,3), D(9,4), D(9,5), D(9,6), D(9,7),
	D(10,0), D(10,1), D(10,2), D(10,3), D(10,4), D(10,5), D(10,6), D(10,7),
	D(11,0), D(11,1), D(11,2), D(11,3), D(11,4), D(11,5), D(11,6), D(11,7),
	D(12,0), D(12,1), D(12,2),
};

#define P(x,y) PINCTRL_PIN(D(x,y),D_PIS(x,y))

const struct pinctrl_pin_desc sppctlpins_all[] = {
	// gpio and iop only
	P(0,0), P(0,1), P(0,2), P(0,3), P(0,4), P(0,5), P(0,6), P(0,7),
	// gpio, iop, muxable
	P(1,0), P(1,1), P(1,2), P(1,3), P(1,4), P(1,5), P(1,6), P(1,7),
	P(2,0), P(2,1), P(2,2), P(2,3), P(2,4), P(2,5), P(2,6), P(2,7),
	P(3,0), P(3,1), P(3,2), P(3,3), P(3,4), P(3,5), P(3,6), P(3,7),
	P(4,0), P(4,1), P(4,2), P(4,3), P(4,4), P(4,5), P(4,6), P(4,7),
	P(5,0), P(5,1), P(5,2), P(5,3), P(5,4), P(5,5), P(5,6), P(5,7),
	P(6,0), P(6,1), P(6,2), P(6,3), P(6,4), P(6,5), P(6,6), P(6,7),
	P(7,0), P(7,1), P(7,2), P(7,3), P(7,4), P(7,5), P(7,6), P(7,7),
	P(8,0), P(8,1), P(8,2), P(8,3), P(8,4), P(8,5), P(8,6), P(8,7),
	// gpio (not wired) and iop only
	P(9,0), P(9,1), P(9,2), P(9,3), P(9,4), P(9,5), P(9,6), P(9,7),
	P(10,0), P(10,1), P(10,2), P(10,3), P(10,4), P(10,5), P(10,6), P(10,7),
	P(11,0), P(11,1), P(11,2), P(11,3), P(11,4), P(11,5), P(11,6), P(11,7),
	P(12,0), P(12,1), P(12,2),
};
const size_t sppctlpins_allSZ = ARRAY_SIZE(sppctlpins_all);

// pmux groups: some pins are muxable. group = pin
const char * const sppctlpmux_list_s[] = {
	D_PIS(0,0), //D_PIS(0,1), D_PIS(0,2), D_PIS(0,3), D_PIS(0,4), D_PIS(0,5), D_PIS(0,6), D_PIS(0,7),
	D_PIS(1,0), D_PIS(1,1), D_PIS(1,2), D_PIS(1,3), D_PIS(1,4), D_PIS(1,5), D_PIS(1,6), D_PIS(1,7),
	D_PIS(2,0), D_PIS(2,1), D_PIS(2,2), D_PIS(2,3), D_PIS(2,4), D_PIS(2,5), D_PIS(2,6), D_PIS(2,7),
	D_PIS(3,0), D_PIS(3,1), D_PIS(3,2), D_PIS(3,3), D_PIS(3,4), D_PIS(3,5), D_PIS(3,6), D_PIS(3,7),
	D_PIS(4,0), D_PIS(4,1), D_PIS(4,2), D_PIS(4,3), D_PIS(4,4), D_PIS(4,5), D_PIS(4,6), D_PIS(4,7),
	D_PIS(5,0), D_PIS(5,1), D_PIS(5,2), D_PIS(5,3), D_PIS(5,4), D_PIS(5,5), D_PIS(5,6), D_PIS(5,7),
	D_PIS(6,0), D_PIS(6,1), D_PIS(6,2), D_PIS(6,3), D_PIS(6,4), D_PIS(6,5), D_PIS(6,6), D_PIS(6,7),
	D_PIS(7,0), D_PIS(7,1), D_PIS(7,2), D_PIS(7,3), D_PIS(7,4), D_PIS(7,5), D_PIS(7,6), D_PIS(7,7),
	D_PIS(8,0), D_PIS(8,1), D_PIS(8,2), D_PIS(8,3), D_PIS(8,4), D_PIS(8,5), D_PIS(8,6), D_PIS(8,7),
	// D_PIS(9,0), D_PIS(9,1), D_PIS(9,2), D_PIS(9,3), D_PIS(9,4), D_PIS(9,5), D_PIS(9,6), D_PIS(9,7),
	// D_PIS(10,0), D_PIS(10,1), D_PIS(10,2), D_PIS(10,3), D_PIS(10,4), D_PIS(10,5), D_PIS(10,6), D_PIS(10,7),
	// D_PIS(11,0), D_PIS(11,1), D_PIS(11,2), D_PIS(11,3), D_PIS(11,4), D_PIS(11,5), D_PIS(11,6), D_PIS(11,7),
	// D_PIS(12,0), D_PIS(12,1), D_PIS(12,2),
};
// gpio: is defined in gpio_inf_sp7021.c
const size_t PMUX_listSZ = sizeof(sppctlpmux_list_s)/sizeof(*(sppctlpmux_list_s));

static const unsigned pins_spif1[] = { D(10,3), D(10,4), D(10,6), D(10,7), };
static const unsigned pins_spif2[] = { D(9,4), D(9,6), D(9,7), D(10,1),};
static const sppctlgrp_t sp7021grps_spif[] = {
	EGRP("SPI_FLASH1", 1, pins_spif1),
	EGRP("SPI_FLASH2", 2, pins_spif2),
};

static const unsigned pins_spi41[] = { D(10,2), D(10,5), };
static const unsigned pins_spi42[] = { D(9,5), D(9,8), };
static const sppctlgrp_t sp7021grps_spi4[] = {
	EGRP("SPI_FLASH_4BIT1", 1, pins_spi41),
	EGRP("SPI_FLASH_4BIT2", 2, pins_spi42),
};

static const unsigned pins_snan[] = { D(9,4), D(9,5), D(9,6), D(9,7), D(10,0), D(10,1), };
static const sppctlgrp_t sp7021grps_snan[] = {
	EGRP("SPI_NAND", 1, pins_snan),
};

static const unsigned pins_emmc[] = {
	D(9,0), D(9,1), D(9,2), D(9,3), D(9,4), D(9,5),
	D(9,6), D(9,7), D(10,0), D(10,1), };
static const sppctlgrp_t sp7021grps_emmc[] = {
	EGRP("CARD0_EMMC", 1, pins_emmc),
};

static const unsigned pins_sdsd[] = { D(8,1), D(8,2), D(8,3), D(8,4), D(8,5), D(8,6), };
static const sppctlgrp_t sp7021grps_sdsd[] = {
	EGRP("SD_CARD", 1, pins_sdsd),
};

static const unsigned pins_uar0[] = { D(11,0), D(11,1), };
static const sppctlgrp_t sp7021grps_uar0[] = {
	EGRP("UA0", 1, pins_uar0),
};

static const unsigned pins_adbg1[] = { D(10,2), D(10,3), };
static const unsigned pins_adbg2[] = { D(7,1), D(7,2), };
static const sppctlgrp_t sp7021grps_adbg[] = {
	EGRP("ACHIP_DEBUG1", 1, pins_adbg1),
	EGRP("ACHIP_DEBUG2", 2, pins_adbg2),
};

static const unsigned pins_aua2axi1[] = { D(2,0), D(2,1), D(2,2), };
static const unsigned pins_aua2axi2[] = { D(1,0), D(1,1), D(1,2), };
static const sppctlgrp_t sp7021grps_au2x[] = {
	EGRP("ACHIP_UA2AXI1", 1, pins_aua2axi1),
	EGRP("ACHIP_UA2AXI2", 2, pins_aua2axi2),
};

static const unsigned pins_fpga[] = {
	D(0,2), D(0,3), D(0,4), D(0,5), D(0,6), D(0,7),
	D(1,0), D(1,1), D(1,2), D(1,3), D(1,4), D(1,5),
	D(1,6), D(1,7), D(2,0), D(2,1), D(2,2), D(2,3),
	D(2,4), D(2,5), D(2,6), D(2,7), D(3,0), D(3,1),
	D(3,2), D(3,3), D(3,4), D(3,5), D(3,6), D(3,7),
	D(4,0), D(4,1), D(4,2), D(4,3), D(4,4), D(4,5),
	D(4,6), D(4,7), D(5,0), D(5,1), D(5,2),
};
static const sppctlgrp_t sp7021grps_fpga[] = {
	EGRP("FPGA_IFX", 1, pins_fpga),
};

/* CEC pin is not used. Release it for others.
static const unsigned pins_hdmi1[] = { D(10,6), D(10,7), D(12,2), D(12,1), };
static const unsigned pins_hdmi2[] = { D(8,3), D(8,4), D(8,5), D(8,6), };
static const unsigned pins_hdmi3[] = { D(7,4), D(7,5), D(7,6), D(7,7), };
*/
static const unsigned pins_hdmi1[] = { D(10,6), D(12,2), D(12,1), };
static const unsigned pins_hdmi2[] = { D(8,3), D(8,5), D(8,6), };
static const unsigned pins_hdmi3[] = { D(7,4), D(7,6), D(7,7), };
static const sppctlgrp_t sp7021grps_hdmi[] = {
	EGRP("HDMI_TX1", 1, pins_hdmi1),
	EGRP("HDMI_TX2", 2, pins_hdmi2),
	EGRP("HDMI_TX3", 3, pins_hdmi3),
};

static const unsigned pins_eadc[] = {
	D(1,0), D(1,1), D(1,2), D(1,3), D(1,4), D(1,5), D(1,6),
};
static const sppctlgrp_t sp7021grps_eadc[] = {
	EGRP("AUD_EXT_ADC_IFX0", 1, pins_eadc),
};

static const unsigned pins_edac[] = {
	D(2,5), D(2,6), D(2,7), D(3,0), D(3,1), D(3,2), D(3,4),
};
static const sppctlgrp_t sp7021grps_edac[] = {
	EGRP("AUD_EXT_DAC_IFX0", 1, pins_edac),
};

static const unsigned pins_spdi[] = { D(2,4), };
static const sppctlgrp_t sp7021grps_spdi[] = {
	EGRP("AUD_IEC_RX0", 1, pins_spdi),
};
static const unsigned pins_spdo[] = { D(3,6), };
static const sppctlgrp_t sp7021grps_spdo[] = {
	EGRP("AUD_IEC_TX0", 1, pins_spdo),
};

static const unsigned pins_tdmt[] = { D(2,5), D(2,6), D(2,7), D(3,0), D(3,1), D(3,2), };
static const sppctlgrp_t sp7021grps_tdmt[] = {
	EGRP("TDMTX_IFX0", 1, pins_tdmt),
};

static const unsigned pins_tdmr[] = { D(1,7), D(2,0), D(2,1), D(2,2), };
static const sppctlgrp_t sp7021grps_tdmr[] = {
	EGRP("TDMRX_IFX0", 1, pins_tdmr),
};

static const unsigned pins_pdmr[] = { D(1,7), D(2,0), D(2,1), D(2,2), D(2,3), };
static const sppctlgrp_t sp7021grps_pdmr[] = {
	EGRP("PDMRX_IFX0", 1, pins_pdmr),
};

static const unsigned pins_pcmt[] = { D(3,7), D(4,0), D(4,1), D(4,2), D(4,3), D(4,4), };
static const sppctlgrp_t sp7021grps_pcmt[] = {
	EGRP("PCM_IEC_TX", 1, pins_pcmt),
};

static const unsigned pins_lcdi[] = {
	D(1,4), D(1,5),
	D(1,6), D(1,7), D(2,0), D(2,1), D(2,2), D(2,3),
	D(2,4), D(2,5), D(2,6), D(2,7), D(3,0), D(3,1),
	D(3,2), D(3,3), D(3,4), D(3,5), D(3,6), D(3,7),
	D(4,0), D(4,1), D(4,2), D(4,3), D(4,4), D(4,5),
	D(4,6), D(4,7),
};
static const sppctlgrp_t sp7021grps_lcdi[] = {
	EGRP("LCDIF", 1, pins_lcdi),
};

static const unsigned pins_dvdd[] = {
	D(7,0), D(7,1), D(7,2), D(7,3), D(7,4), D(7,5), D(7,6), D(7,7),
	D(8,0), D(8,1), D(8,2), D(8,3), D(8,4), D(8,5),
};
static const sppctlgrp_t sp7021grps_dvdd[] = {
	EGRP("DVD_DSP_DEBUG", 1, pins_dvdd),
};

static const unsigned pins_i2cd[] = { D(1,0), D(1,1), };
static const sppctlgrp_t sp7021grps_i2cd[] = {
	EGRP("I2C_DEBUG", 1, pins_i2cd),
};

static const unsigned pins_i2cs[] = { D(0,0), D(0,1), };
static const sppctlgrp_t sp7021grps_i2cs[] = {
	EGRP("I2C_SLAVE", 1, pins_i2cs),
};

static const unsigned pins_wakp[] = { D(10,5), };
static const sppctlgrp_t sp7021grps_wakp[] = {
	EGRP("WAKEUP", 1, pins_wakp),
};

static const unsigned pins_u2ax[] = { D(2,0), D(2,1), D(3,0), D(3,1), };
static const sppctlgrp_t sp7021grps_u2ax[] = {
	EGRP("UART2AXI", 1, pins_u2ax),
};

static const unsigned pins_u0ic[] = { D(0,0), D(0,1), D(0,4), D(0,5), D(1,0), D(1,1), };
static const sppctlgrp_t sp7021grps_u0ic[] = {
	EGRP("USB0_I2C", 1, pins_u0ic),
};

static const unsigned pins_u1ic[] = { D(0,2), D(0,3), D(0,6), D(0,7), D(1,2), D(1,3), };
static const sppctlgrp_t sp7021grps_u1ic[] = {
	EGRP("USB1_I2C", 1, pins_u1ic),
};

static const unsigned pins_u0ot[] = { D(11,2), };
static const sppctlgrp_t sp7021grps_u0ot[] = {
	EGRP("USB0_OTG", 1, pins_u0ot),
};

static const unsigned pins_u1ot[] = { D(11,3), };
static const sppctlgrp_t sp7021grps_u1ot[] = {
	EGRP("USB1_OTG", 1, pins_u1ot),
};

static const unsigned pins_uphd[] = {
	D(0,1), D(0,2), D(0,3), D(7,4), D(7,5), D(7,6),
	D(7,7), D(8,0), D(8,1), D(8,2), D(8,3),
	D(9,7), D(10,2), D(10,3), D(10,4),
};
static const sppctlgrp_t sp7021grps_up0d[] = {
	EGRP("UPHY0_DEBUG", 1, pins_uphd),
};
static const sppctlgrp_t sp7021grps_up1d[] = {
	EGRP("UPHY1_DEBUG", 1, pins_uphd),
};

static const unsigned pins_upex[] = {
	D(0,0), D(0,1), D(0,2), D(0,3), D(0,4), D(0,5), D(0,6), D(0,7),
	D(1,0), D(1,1), D(1,2), D(1,3), D(1,4), D(1,5), D(1,6), D(1,7),
	D(2,0), D(2,1), D(2,2), D(2,3), D(2,4), D(2,5), D(2,6), D(2,7),
	D(3,0), D(3,1), D(3,2), D(3,3), D(3,4), D(3,5), D(3,6), D(3,7),
	D(4,0), D(4,1), D(4,2), D(4,3), D(4,4), D(4,5), D(4,6), D(4,7),
	D(5,0), D(5,1), D(5,2), D(5,3), D(5,4), D(5,5), D(5,6), D(5,7),
	D(6,0), D(6,1), D(6,2), D(6,3), D(6,4), D(6,5), D(6,6), D(6,7),
	D(7,0), D(7,1), D(7,2), D(7,3), D(7,4), D(7,5), D(7,6), D(7,7),
	D(8,0), D(8,1), D(8,2), D(8,3), D(8,4), D(8,5), D(8,6), D(8,7),
	D(9,0), D(9,1), D(9,2), D(9,3), D(9,4), D(9,5), D(9,6), D(9,7),
	D(10,0), D(10,1), D(10,2), D(10,3), D(10,4), D(10,5), D(10,6), D(10,7),
};
static const sppctlgrp_t sp7021grps_upex[] = {
	EGRP("UPHY0_EXT", 1, pins_upex),
};

static const unsigned pins_prp1[] = {
	D(0,6), D(0,7),
	D(1,0), D(1,1), D(1,2), D(1,3), D(1,4), D(1,5), D(1,6), D(1,7),
	D(2,1), D(2,2), D(2,3), D(2,4), D(2,5), D(2,6), D(2,7),
	D(3,0), D(3,1), D(3,2),
};
static const unsigned pins_prp2[] = {
	D(3,4), D(3,6), D(3,7),
	D(4,0), D(4,1), D(4,2), D(4,3), D(4,4), D(4,5), D(4,6), D(4,7),
	D(5,0), D(5,1), D(5,2), D(5,3), D(5,4), D(5,5), D(5,6), D(5,7),
	D(6,4),
};
static const sppctlgrp_t sp7021grps_prbp[] = {
	EGRP("PROBE_PORT1", 1, pins_prp1),
	EGRP("PROBE_PORT2", 2, pins_prp2),
};

static const unsigned pins_anai[] = { D(0,4), D(0,5), };
static const sppctlgrp_t sp7021grps_anai[] = {
	EGRP("ANA_I2C_IF", 1, pins_anai),
};

static const unsigned pins_anat[] = {
	D(0,0), D(0,1), D(0,2), D(0,3), D(0,4), D(0,5), D(0,6), D(0,7),
	D(1,0), D(1,1), D(1,2), D(1,3), D(1,4), D(1,5), D(1,6),
	D(11,0),
};
static const sppctlgrp_t sp7021grps_anat[] = {
	EGRP("ANA_TEST_IF", 1, pins_anat),
};

func_t list_funcs[] = {
	FNCN("GPIO",            fOFF_0, 0x00, 0, 0),
	FNCN("IOP",             fOFF_0, 0x00, 0, 0),

	FNCN("L2SW_CLK_OUT",        fOFF_M, 0x00, 0, 7),
	FNCN("L2SW_MAC_SMI_MDC",    fOFF_M, 0x00, 8, 7),
	FNCN("L2SW_LED_FLASH0",     fOFF_M, 0x01, 0, 7),
	FNCN("L2SW_LED_FLASH1",     fOFF_M, 0x01, 8, 7),
	FNCN("L2SW_LED_ON0",        fOFF_M, 0x02, 0, 7),
	FNCN("L2SW_LED_ON1",        fOFF_M, 0x02, 8, 7),
	FNCN("L2SW_MAC_SMI_MDIO",   fOFF_M, 0x03, 0, 7),
	FNCN("L2SW_P0_MAC_RMII_TXEN",   fOFF_M, 0x03, 8, 7),
	FNCN("L2SW_P0_MAC_RMII_TXD0",   fOFF_M, 0x04, 0, 7),
	FNCN("L2SW_P0_MAC_RMII_TXD1",   fOFF_M, 0x04, 8, 7),
	FNCN("L2SW_P0_MAC_RMII_CRSDV",  fOFF_M, 0x05, 0, 7),
	FNCN("L2SW_P0_MAC_RMII_RXD0",   fOFF_M, 0x05, 8, 7),
	FNCN("L2SW_P0_MAC_RMII_RXD1",   fOFF_M, 0x06, 0, 7),
	FNCN("L2SW_P0_MAC_RMII_RXER",   fOFF_M, 0x06, 8, 7),
	FNCN("L2SW_P1_MAC_RMII_TXEN",   fOFF_M, 0x07, 0, 7),
	FNCN("L2SW_P1_MAC_RMII_TXD0",   fOFF_M, 0x07, 8, 7),
	FNCN("L2SW_P1_MAC_RMII_TXD1",   fOFF_M, 0x08, 0, 7),
	FNCN("L2SW_P1_MAC_RMII_CRSDV",  fOFF_M, 0x08, 8, 7),
	FNCN("L2SW_P1_MAC_RMII_RXD0",   fOFF_M, 0x09, 0, 7),
	FNCN("L2SW_P1_MAC_RMII_RXD1",   fOFF_M, 0x09, 8, 7),
	FNCN("L2SW_P1_MAC_RMII_RXER",   fOFF_M, 0x0A, 0, 7),
	FNCN("DAISY_MODE",      fOFF_M, 0x0A, 8, 7),    // mux has no effect now
	FNCN("SDIO_CLK",        fOFF_M, 0x0B, 0, 7),
	FNCN("SDIO_CMD",        fOFF_M, 0x0B, 8, 7),
	FNCN("SDIO_D0",         fOFF_M, 0x0C, 0, 7),
	FNCN("SDIO_D1",         fOFF_M, 0x0C, 8, 7),
	FNCN("SDIO_D2",         fOFF_M, 0x0D, 0, 7),
	FNCN("SDIO_D3",         fOFF_M, 0x0D, 8, 7),
	FNCN("PWM0",            fOFF_M, 0x0E, 0, 7),
	FNCN("PWM1",            fOFF_M, 0x0E, 8, 7),
	FNCN("PWM2",            fOFF_M, 0x0F, 0, 7),
	FNCN("PWM3",            fOFF_M, 0x0F, 8, 7),

	FNCN("PWM4",            fOFF_M, 0x10, 0, 7),
	FNCN("PWM5",            fOFF_M, 0x10, 8, 7),
	FNCN("PWM6",            fOFF_M, 0x11, 0, 7),
	FNCN("PWM7",            fOFF_M, 0x11, 8, 7),
	FNCN("ICM0_D",          fOFF_M, 0x12, 0, 7),    // 4x Input captures
	FNCN("ICM1_D",          fOFF_M, 0x12, 8, 7),
	FNCN("ICM2_D",          fOFF_M, 0x13, 0, 7),
	FNCN("ICM3_D",          fOFF_M, 0x13, 8, 7),
	FNCN("ICM0_CLK",        fOFF_M, 0x14, 0, 7),
	FNCN("ICM1_CLK",        fOFF_M, 0x14, 8, 7),
	FNCN("ICM2_CLK",        fOFF_M, 0x15, 0, 7),
	FNCN("ICM3_CLK",        fOFF_M, 0x15, 8, 7),
	FNCN("SPIM0_INT",       fOFF_M, 0x16, 0, 7),    // 4x SPI masters
	FNCN("SPIM0_CLK",       fOFF_M, 0x16, 8, 7),
	FNCN("SPIM0_EN",        fOFF_M, 0x17, 0, 7),
	FNCN("SPIM0_DO",        fOFF_M, 0x17, 8, 7),
	FNCN("SPIM0_DI",        fOFF_M, 0x18, 0, 7),
	FNCN("SPIM1_INT",       fOFF_M, 0x18, 8, 7),
	FNCN("SPIM1_CLK",       fOFF_M, 0x19, 0, 7),
	FNCN("SPIM1_EN",        fOFF_M, 0x19, 8, 7),
	FNCN("SPIM1_DO",        fOFF_M, 0x1A, 0, 7),
	FNCN("SPIM1_DI",        fOFF_M, 0x1A, 8, 7),
	FNCN("SPIM2_INT",       fOFF_M, 0x1B, 0, 7),
	FNCN("SPIM2_CLK",       fOFF_M, 0x1B, 8, 7),
	FNCN("SPIM2_EN",        fOFF_M, 0x1C, 0, 7),
	FNCN("SPIM2_DO",        fOFF_M, 0x1C, 8, 7),
	FNCN("SPIM2_DI",        fOFF_M, 0x1D, 0, 7),
	FNCN("SPIM3_INT",       fOFF_M, 0x1D, 8, 7),
	FNCN("SPIM3_CLK",       fOFF_M, 0x1E, 0, 7),
	FNCN("SPIM3_EN",        fOFF_M, 0x1E, 8, 7),
	FNCN("SPIM3_DO",        fOFF_M, 0x1F, 0, 7),
	FNCN("SPIM3_DI",        fOFF_M, 0x1F, 8, 7),

	FNCN("SPI0S_INT",       fOFF_M, 0x20, 0, 7),    // 4x SPI slaves
	FNCN("SPI0S_CLK",       fOFF_M, 0x20, 8, 7),
	FNCN("SPI0S_EN",        fOFF_M, 0x21, 0, 7),
	FNCN("SPI0S_DO",        fOFF_M, 0x21, 8, 7),
	FNCN("SPI0S_DI",        fOFF_M, 0x22, 0, 7),
	FNCN("SPI1S_INT",       fOFF_M, 0x22, 8, 7),
	FNCN("SPI1S_CLK",       fOFF_M, 0x23, 0, 7),
	FNCN("SPI1S_EN",        fOFF_M, 0x23, 8, 7),
	FNCN("SPI1S_DO",        fOFF_M, 0x24, 0, 7),
	FNCN("SPI1S_DI",        fOFF_M, 0x24, 8, 7),
	FNCN("SPI2S_INT",       fOFF_M, 0x25, 0, 7),
	FNCN("SPI2S_CLK",       fOFF_M, 0x25, 8, 7),
	FNCN("SPI2S_EN",        fOFF_M, 0x26, 0, 7),
	FNCN("SPI2S_DO",        fOFF_M, 0x26, 8, 7),
	FNCN("SPI2S_DI",        fOFF_M, 0x27, 0, 7),
	FNCN("SPI3S_INT",       fOFF_M, 0x27, 8, 7),
	FNCN("SPI3S_CLK",       fOFF_M, 0x28, 0, 7),
	FNCN("SPI3S_EN",        fOFF_M, 0x28, 8, 7),
	FNCN("SPI3S_DO",        fOFF_M, 0x29, 0, 7),
	FNCN("SPI3S_DI",        fOFF_M, 0x29, 8, 7),
	FNCN("I2CM0_CLK",       fOFF_M, 0x2A, 0, 7),    // 4x I2C masters
	FNCN("I2CM0_DAT",       fOFF_M, 0x2A, 8, 7),
	FNCN("I2CM1_CLK",       fOFF_M, 0x2B, 0, 7),
	FNCN("I2CM1_DAT",       fOFF_M, 0x2B, 8, 7),
	FNCN("I2CM2_CLK",       fOFF_M, 0x2C, 0, 7),
	FNCN("I2CM2_DAT",       fOFF_M, 0x2C, 8, 7),
	FNCN("I2CM3_CLK",       fOFF_M, 0x2D, 0, 7),
	FNCN("I2CM3_DAT",       fOFF_M, 0x2D, 8, 7),
	FNCN("UA1_TX",          fOFF_M, 0x2E, 0, 7),    // +4x muxable UARTS (#0 is not muxable)
	FNCN("UA1_RX",          fOFF_M, 0x2E, 8, 7),
	FNCN("UA1_CTS",         fOFF_M, 0x2F, 0, 7),
	FNCN("UA1_RTS",         fOFF_M, 0x2F, 8, 7),

	FNCN("UA2_TX",          fOFF_M, 0x30, 0, 7),
	FNCN("UA2_RX",          fOFF_M, 0x30, 8, 7),
	FNCN("UA2_CTS",         fOFF_M, 0x31, 0, 7),
	FNCN("UA2_RTS",         fOFF_M, 0x31, 8, 7),
	FNCN("UA3_TX",          fOFF_M, 0x32, 0, 7),
	FNCN("UA3_RX",          fOFF_M, 0x32, 8, 7),
	FNCN("UA3_CTS",         fOFF_M, 0x33, 0, 7),
	FNCN("UA3_RTS",         fOFF_M, 0x33, 8, 7),
	FNCN("UA4_TX",          fOFF_M, 0x34, 0, 7),
	FNCN("UA4_RX",          fOFF_M, 0x34, 8, 7),
	FNCN("UA4_CTS",         fOFF_M, 0x35, 0, 7),
	FNCN("UA4_RTS",         fOFF_M, 0x35, 8, 7),
	FNCN("TIMER0_INT",      fOFF_M, 0x36, 0, 7),    // 4x timers interrupts
	FNCN("TIMER1_INT",      fOFF_M, 0x36, 8, 7),
	FNCN("TIMER2_INT",      fOFF_M, 0x37, 0, 7),
	FNCN("TIMER3_INT",      fOFF_M, 0x37, 8, 7),
	FNCN("GPIO_INT0",       fOFF_M, 0x38, 0, 7),    // 8x GPIO interrupts
	FNCN("GPIO_INT1",       fOFF_M, 0x38, 8, 7),
	FNCN("GPIO_INT2",       fOFF_M, 0x39, 0, 7),
	FNCN("GPIO_INT3",       fOFF_M, 0x39, 8, 7),
	FNCN("GPIO_INT4",       fOFF_M, 0x3A, 0, 7),
	FNCN("GPIO_INT5",       fOFF_M, 0x3A, 8, 7),
	FNCN("GPIO_INT6",       fOFF_M, 0x3B, 0, 7),
	FNCN("GPIO_INT7",       fOFF_M, 0x3B, 8, 7),
	// offset from 0x9C000080
	FNCE("SPI_FLASH",       fOFF_G, 0x01, 0, 2, sp7021grps_spif),
	FNCE("SPI_FLASH_4BIT",  fOFF_G, 0x01, 2, 2, sp7021grps_spi4),
	FNCE("SPI_NAND",        fOFF_G, 0x01, 4, 1, sp7021grps_snan),
	FNCE("CARD0_EMMC",      fOFF_G, 0x01, 5, 1, sp7021grps_emmc),
	FNCE("SD_CARD",         fOFF_G, 0x01, 6, 1, sp7021grps_sdsd),
	FNCE("UA0",             fOFF_G, 0x01, 7, 1, sp7021grps_uar0),
	FNCE("ACHIP_DEBUG",     fOFF_G, 0x01, 8, 2, sp7021grps_adbg),
	FNCE("ACHIP_UA2AXI",    fOFF_G, 0x01,10, 2, sp7021grps_au2x),
	FNCE("FPGA_IFX",        fOFF_G, 0x01,12, 1, sp7021grps_fpga),
	FNCE("HDMI_TX",         fOFF_G, 0x01,13, 2, sp7021grps_hdmi),

	FNCE("AUD_EXT_ADC_IFX0",fOFF_G, 0x01,15, 1, sp7021grps_eadc),   // I2S audio in
	FNCE("AUD_EXT_DAC_IFX0",fOFF_G, 0x02, 0, 1, sp7021grps_edac),   // I2S audio out
	FNCE("SPDIF_RX",        fOFF_G, 0x02, 2, 1, sp7021grps_spdi),
	FNCE("SPDIF_TX",        fOFF_G, 0x02, 3, 1, sp7021grps_spdo),
	FNCE("TDMTX_IFX0",      fOFF_G, 0x02, 4, 1, sp7021grps_tdmt),
	FNCE("TDMRX_IFX0",      fOFF_G, 0x02, 5, 1, sp7021grps_tdmr),
	FNCE("PDMRX_IFX0",      fOFF_G, 0x02, 6, 1, sp7021grps_pdmr),
	FNCE("PCM_IEC_TX",      fOFF_G, 0x02, 7, 1, sp7021grps_pcmt),
	FNCE("LCDIF",           fOFF_G, 0x04, 6, 1, sp7021grps_lcdi),
	FNCE("DVD_DSP_DEBUG",   fOFF_G, 0x02, 8, 1, sp7021grps_dvdd),
	FNCE("I2C_DEBUG",       fOFF_G, 0x02, 9, 1, sp7021grps_i2cd),
	FNCE("I2C_SLAVE",       fOFF_G, 0x02,10, 1, sp7021grps_i2cs),   // I2C slave
	FNCE("WAKEUP",          fOFF_G, 0x02,11, 1, sp7021grps_wakp),
	FNCE("UART2AXI",        fOFF_G, 0x02,12, 2, sp7021grps_u2ax),
	FNCE("USB0_I2C",        fOFF_G, 0x02,14, 2, sp7021grps_u0ic),
	FNCE("USB1_I2C",        fOFF_G, 0x03, 0, 2, sp7021grps_u1ic),
	FNCE("USB0_OTG",        fOFF_G, 0x03, 2, 1, sp7021grps_u0ot),
	FNCE("USB1_OTG",        fOFF_G, 0x03, 3, 1, sp7021grps_u1ot),
	FNCE("UPHY0_DEBUG",     fOFF_G, 0x03, 4, 1, sp7021grps_up0d),
	FNCE("UPHY1_DEBUG",     fOFF_G, 0x03, 5, 1, sp7021grps_up1d),
	FNCE("UPHY0_EXT",       fOFF_G, 0x03, 6, 1, sp7021grps_upex),
	FNCE("PROBE_PORT",      fOFF_G, 0x03, 7, 2, sp7021grps_prbp),
	FNCE("ANA_I2C_IF",      fOFF_G, 0x03, 7, 2, sp7021grps_anai),
	FNCE("ANA_TEST_IF",     fOFF_G, 0x03, 7, 2, sp7021grps_anat),
};

const size_t list_funcsSZ = ARRAY_SIZE(list_funcs);
