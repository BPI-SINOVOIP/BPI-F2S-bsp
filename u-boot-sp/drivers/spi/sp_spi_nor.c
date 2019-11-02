/*
 * (C) Copyright 2017
 * Sunplus Technology
 * Henry Liou<henry.liou@sunplus.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <dm.h>
#include <errno.h>
#include <memalign.h>
#include <asm/io.h>
#include "sp_spi_nor.h"

DECLARE_GLOBAL_DATA_PTR;

static UINT8 *cmd_buf;
static size_t cmd_len;
static volatile sp_spi_nor_regs *spi_reg;

int AV1_GetStc32(void)
{
	//TBD
	return 0;
}

static void spi_nor_io_CUST_config(UINT8 cmd_b, UINT8 addr_b, UINT8 data_b, SPI_ENHANCE enhance,UINT8 dummy)
{
	UINT32 config;

	if (enhance.enhance_en == 1) {
		config = spi_reg->spi_cfg0  & CLEAR_ENHANCE_DATA;
		if (enhance.enhance_bit == 4) {
			config &= ~(1<<18);
		} else if (enhance.enhance_bit == 8) {
			config |= (1<<18);
		}
		spi_reg->spi_cfg0 = config | ENHANCE_DATA(enhance.enhance_data);
	}

	config = 0;
	switch (cmd_b) {
	case 4:
		config |= SPI_CMD_4b | SPI_CMD_OEN_4b;
		break;
	case 2:
		config |= SPI_CMD_2b | SPI_CMD_OEN_2b;
		break;
	case 1:
		config |= SPI_CMD_1b | SPI_CMD_OEN_1b;
		break;
	case 0:
	default:
		config |= SPI_CMD_NO | SPI_CMD_OEN_NO;
		break;
	}

	switch (addr_b) {
	case 4:
		config |= SPI_ADDR_4b | SPI_ADDR_OEN_4b;
		break;
	case 2:
		config |= SPI_ADDR_2b | SPI_ADDR_OEN_2b;
		break;
	case 1:
		config |= SPI_ADDR_1b | SPI_ADDR_OEN_1b;
		break;
	case 0:
	default:
		config |= SPI_ADDR_NO | SPI_ADDR_OEN_NO;
		break;
	}

	switch (data_b) {
	case 4:
		config |= SPI_DATA_4b | SPI_DATA_OEN_4b;
		break;
	case 2:
		config |= SPI_DATA_2b | SPI_DATA_OEN_2b;
		break;
	case 1:
		config |= SPI_DATA_1b | SPI_DATA_OEN_1b | SPI_DATA_IEN_DQ1;
		break;
	case 0:
	default:
		config |= SPI_DATA_NO | SPI_DATA_OEN_NO;
		break;
	}

	switch (enhance.enhance_bit_mode) {
	case 4:
		config |= SPI_ENHANCE_4b;
		break;
	case 2:
		config |= SPI_ENHANCE_2b;
		break;
	case 1:
		config |= SPI_ENHANCE_1b;
		break;
	case 0:
	default:
		config |= SPI_ENHANCE_NO;
		break;
	}

	spi_reg->spi_cfg1 = config | SPI_DUMMY_CYC(dummy);
	spi_reg->spi_cfg2 = config | SPI_DUMMY_CYC(dummy);
}

#if (SP_SPINOR_DMA)
static void spi_readcmd_set(UINT8 cmd)
{
	SPI_ENHANCE enhance;

	enhance.enhance_en = 0;
	enhance.enhance_bit_mode = 0;
	diag_printf("%s\n",__FUNCTION__);

	switch (cmd) {
	case 0x0B:
		spi_nor_io_CUST_config(CMD_1,ADDR_1,DATA_1,enhance,DUMMY_CYCLE(8));
		break;
	case 0x3B:
		spi_nor_io_CUST_config(CMD_1,ADDR_1,DATA_2,enhance,DUMMY_CYCLE(8));
		break;
	case 0x6B:
		spi_nor_io_CUST_config(CMD_1,ADDR_1,DATA_4,enhance,DUMMY_CYCLE(8));
		break;
	case 0xBB:
		spi_nor_io_CUST_config(CMD_1,ADDR_2,DATA_2,enhance,DUMMY_CYCLE(4));
		break;
	case 0xEB:
		spi_nor_io_CUST_config(CMD_1,ADDR_4,DATA_4,enhance,DUMMY_CYCLE(6));
		break;
	case 0x32:
		spi_nor_io_CUST_config(CMD_1,ADDR_1,DATA_4,enhance,DUMMY_CYCLE(0));
		break;
	default:
		spi_nor_io_CUST_config(CMD_1,ADDR_1,DATA_1,enhance,DUMMY_CYCLE(0));
		break;
	}
}

static int spi_flash_xfer_DMAread(struct sp_spi_nor_priv *priv,UINT8 *cmd, size_t cmd_len, void *data, size_t data_len)
{
	UINT32 addr_temp = 0;
	UINT8 *data_in = data;
	UINT32 time = 0;
	UINT32 ctrl = 0;
	UINT32 autocfg = 0;
	UINT32 temp_len = 0;
	int value = 0;
	struct spinorbufdesc *desc_r = &priv->rchain;
	ulong data_end;

	diag_printf("%s\n",__FUNCTION__);
	msg_printf("DMA read :data length 0x%x cmd[0] 0x%x\n",data_len, cmd[0]);
	while ((spi_reg->spi_auto_cfg & DMA_TRIGGER) ||  (spi_reg->spi_ctrl & SPI_CTRL_BUSY)) {
		time++;
		if (time > 0x30000) {
			 msg_printf("##busy check time out: spi_auto_cfg 0x%x spi_ctrl 0x%x\n", spi_reg->spi_auto_cfg, spi_reg->spi_ctrl);
			 break;
		}
	}

	data_end = desc_r->phys + CFG_BUFF_MAX;
	msg_printf("data length %d buff size 0x%x\n",data_len, desc_r->size);

	spi_readcmd_set(cmd[0]);

	ctrl = spi_reg->spi_ctrl & (CLEAR_CUST_CMD & (~(1<<19)));
	ctrl |= (READ | BYTE_0 | ADDR_0B);

	//spi_reg->spi_page_addr = 0;
	spi_reg->spi_data = 0;
	if (cmd_len > 1) {
		addr_temp = (cmd[1] << 16) | (cmd[2] << 8) | cmd[3];
		ctrl |= ADDR_3B;
	}

	spi_reg->spi_ctrl = ctrl;
	msg_printf("data ctrl 0x%x\n",ctrl);
	spi_reg->spi_page_size = 0x100<<4;
	do {
		spi_reg->spi_page_addr = addr_temp;
		if (data_len > CFG_BUFF_MAX) {
			temp_len = CFG_BUFF_MAX;
			data_len -= CFG_BUFF_MAX;
		} else {
			temp_len = data_len;
			data_len = 0;
		}
		msg_printf("remain len  0x%x\n", data_len);

		value =  (spi_reg->spi_cfg0 & CLEAR_DATA64_LEN) |  temp_len | DATA64_EN;
		if (cmd[0] == 5)//need to check
			value |= (1<<19);
		spi_reg->spi_cfg0 = value;

		spi_reg->spi_mem_data_addr = desc_r->phys;
		msg_printf("dma addr 0x%x\n", spi_reg->spi_mem_data_addr);
		msg_printf("spi_auto_cfg  0x%x\n", spi_reg->spi_auto_cfg);

		autocfg =  (cmd[0]<<24)|(1<<20)| DMA_TRIGGER;
		value = (spi_reg->spi_auto_cfg &(~(0xff<<24))) | autocfg;

		spi_reg->spi_intr_msk = (0x2<<1);
		spi_reg->spi_intr_sts = 0x07;

		spi_reg->spi_auto_cfg = value;

		while (( spi_reg->spi_intr_sts & 0x2) == 0x0);

		spi_reg->spi_intr_sts |= 0x02;
		spi_reg->spi_intr_msk = 0;
		time = 0;
		while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY)!=0) {
			time++;
			if (time > 0x20000) {
				msg_printf("##busy check time out: spi_auto_cfg 0x%x spi_ctrl 0x%x\n", spi_reg->spi_auto_cfg, spi_reg->spi_ctrl);
				data_len = 0;
				break;
			}
		}

		/* Invalidate received data */
		invalidate_dcache_range(rounddown(desc_r->phys, ARCH_DMA_MINALIGN), roundup(data_end,ARCH_DMA_MINALIGN));
		memcpy(data_in, (void *)desc_r->phys, temp_len);
		addr_temp += CFG_BUFF_MAX;
		data_in += CFG_BUFF_MAX;
	} while (data_len != 0);

	spi_reg->spi_cfg0 &= (DATA64_DIS & (~ (1<<19)));
	spi_reg->spi_auto_cfg  &= ~autocfg;
	spi_readcmd_set(0);
	return 0;
}

static int spi_flash_xfer_DMAwrite(struct sp_spi_nor_priv *priv, UINT8 *cmd, size_t cmd_len, const void *data, size_t data_len)
{
	UINT32 temp_len = data_len;
	UINT32 addr_temp = 0;
	UINT8 *data_in = (UINT8 *)data;
	UINT32 time = 0;
	UINT32 ctrl = 0;
	UINT32 autocfg = 0;
	int value = 0;
	struct spinorbufdesc *desc_w = &priv->wchain;

	ulong data_end = desc_w->phys + roundup(CFG_BUFF_MAX, ARCH_DMA_MINALIGN);

	diag_printf("%s\n",__FUNCTION__);
	msg_printf("DMA write : wdata length %d, cmd 0x%x\n",data_len, cmd[0]);

	while ((spi_reg->spi_auto_cfg & DMA_TRIGGER) ||  (spi_reg->spi_ctrl & SPI_CTRL_BUSY)) {
		time++;
		if (time > 0x30000) {
			 msg_printf("##busy check time out: spi_auto_cfg 0x%x spi_ctrl 0x%x\n", spi_reg->spi_auto_cfg, spi_reg->spi_ctrl);
			 break;
		}
	}

	spi_readcmd_set(cmd[0]);
	ctrl = spi_reg->spi_ctrl & (CLEAR_CUST_CMD & (~(1<<19)));
	ctrl |= (WRITE | BYTE_0 | ADDR_0B | (1<<19));
	if (cmd[0] == 6)//need to check
		ctrl &= ~(1<<19);

	spi_reg->spi_page_addr = 0;
	spi_reg->spi_data = 0;
	if (cmd_len > 1) {
		addr_temp = (cmd[1] << 16) | (cmd[2] << 8) | cmd[3];
		ctrl |= ADDR_3B;
	}

	spi_reg->spi_ctrl = ctrl;
	spi_reg->spi_page_size = 0x100<<4;
	do {
		spi_reg->spi_page_addr = addr_temp;
		if (data_len > CFG_BUFF_MAX) {
			temp_len = CFG_BUFF_MAX;
			data_len -= CFG_BUFF_MAX;
		} else {
			temp_len = data_len;
			data_len = 0;
		}

		if (temp_len > 0) {
			memcpy((void *)desc_w->phys, data_in, temp_len); // copy data to dma
			/* Flush data to be sent */
			flush_dcache_range(desc_w->phys, data_end);
		}

		value =  spi_reg->spi_cfg0;
		spi_reg->spi_cfg0 = (value & CLEAR_DATA64_LEN) | temp_len;//| DATA64_EN;

		spi_reg->spi_mem_data_addr = desc_w->phys;

		autocfg = DMA_TRIGGER | (cmd[0]<<8) | (1);
		value = (spi_reg->spi_auto_cfg & (~(0xff<<8))) | autocfg;

		spi_reg->spi_intr_msk = (0x2<<1);
		spi_reg->spi_intr_sts = 0x07;

		spi_reg->spi_auto_cfg =  value;

		while (( spi_reg->spi_intr_sts & 0x2) == 0x0);

		spi_reg->spi_intr_sts |= 0x02;
		spi_reg->spi_intr_msk = 0;
		time = 0;

		while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
			time++;
			if (time > 0x30000) {
				msg_printf("##busy check time out: spi_auto_cfg 0x%x spi_ctrl 0x%x\n", spi_reg->spi_auto_cfg, spi_reg->spi_ctrl);
				data_len = 0;
				break;
			}
		}

		addr_temp += CFG_BUFF_MAX;
		data_in += CFG_BUFF_MAX;
	} while (data_len != 0);

	spi_reg->spi_cfg0 &= DATA64_DIS;
	spi_reg->spi_auto_cfg  &= ~autocfg;
	spi_readcmd_set(0);
	return 0;
}
#else
static void spi_fast_read_enable(void)
{
	SPI_ENHANCE enhance;

	enhance.enhance_en = 0;
	diag_printf("%s\n",__FUNCTION__);

	while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
		msg_printf("wait spi_reg->spi_ctrl 0x%x\n",spi_reg->spi_ctrl);
	}

	spi_reg->spi_ctrl = A_CHIP | SPI_CLK_D_16;
	spi_nor_io_CUST_config(CMD_1,ADDR_1,DATA_1,enhance,DUMMY_CYCLE(8));
}

static void spi_fast_read_disable(void)
{
	SPI_ENHANCE enhance;

	enhance.enhance_en = 0;
	diag_printf("%s\n",__FUNCTION__);

	while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
		msg_printf("wait spi_reg->spi_ctrl 0x%x\n",spi_reg->spi_ctrl);
	}

	spi_reg->spi_ctrl = A_CHIP | SPI_CLK_D_32;
	spi_nor_io_CUST_config(CMD_1,ADDR_1,DATA_1,enhance,DUMMY_CYCLE(0));
}

static UINT8 spi_nor_read_status1(void)
{
	UINT32 ctrl;

	diag_printf("%s\n",__FUNCTION__);

	ctrl = spi_reg->spi_ctrl & CLEAR_CUST_CMD;
	ctrl = ctrl | READ | BYTE_0 | ADDR_0B | CUST_CMD(0x05);
	while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
		msg_printf("wait spi_reg->spi_ctrl 0x%x\n",spi_reg->spi_ctrl);
	}

	spi_reg->spi_data = 0;
	spi_reg->spi_ctrl = ctrl;
	spi_reg->spi_auto_cfg |= PIO_TRIGGER;
	while ((spi_reg->spi_auto_cfg & PIO_TRIGGER) != 0) {
		msg_printf("wait PIO_TRIGGER\n");
	}

	msg_printf("spi_reg->spi_status 0x%x\n",spi_reg->spi_status);
	return (spi_reg->spi_status&0xff);
}

static int spi_flash_xfer_read(UINT8 *cmd, size_t cmd_len, void *data, size_t data_len)
{
	diag_printf("%s\n",__FUNCTION__);
	UINT32 total_count = data_len;
	UINT32 data_count;
	UINT32 addr_offset = 0;
	UINT32 addr_temp = 0;
	UINT8 *data_in = (UINT8 *)data;
	UINT32 data_temp = 0;
	//UINT8 addr_len = 0;
	UINT32 timeout = 0;
	UINT32 time = 0;
	UINT32 ctrl = 0;
	int fast_read = 0;

	msg_printf("data length %d\n",data_len);
	while (total_count > 0) {
		if (total_count > SPI_DATA64_MAX_LEN) {
			total_count = total_count - SPI_DATA64_MAX_LEN;
			data_count = SPI_DATA64_MAX_LEN;
		} else {
			data_count = total_count;
			total_count = 0;
		}

		while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
			msg_printf("wait spi_reg->spi_ctrl 0x%x\n",spi_reg->spi_ctrl);
		}

		ctrl = spi_reg->spi_ctrl & CLEAR_CUST_CMD;
		ctrl = ctrl | READ | BYTE_0 | ADDR_0B | CUST_CMD(cmd[0]);
		spi_reg->spi_cfg0 = (spi_reg->spi_cfg0 & CLEAR_DATA64_LEN) | data_count | DATA64_EN;
		spi_reg->spi_page_addr = 0;
		spi_reg->spi_buf_addr = DATA64_READ_ADDR(0) | DATA64_WRITE_ADDR(0);

		if (cmd_len > 1) {
			addr_temp = (cmd[1] << 16) | (cmd[2] << 8) | cmd[3];
			addr_temp = addr_temp + addr_offset * SPI_DATA64_MAX_LEN;
			spi_reg->spi_page_addr = addr_temp;
			ctrl = ctrl | ADDR_3B ;
			msg_printf("addr 0x%x\n", spi_reg->spi_page_addr);
		}

		if (cmd[0] == CMD_FAST_READ) {
			spi_fast_read_enable();
			fast_read = 1;
		}

		spi_reg->spi_data = 0;
		spi_reg->spi_ctrl = ctrl;
		spi_reg->spi_auto_cfg |= PIO_TRIGGER;
		//msg_printf("spi_reg->spi_ctrl 0x%x\n", spi_reg->spi_ctrl);
		//msg_printf("spi_reg->spi_page_addr 0x%x\n", spi_reg->spi_page_addr);
		//msg_printf("spi_reg->spi_cfg0 0x%x\n", spi_reg->spi_cfg0);

		if (cmd[0] == CMD_READ_STATUS) {
			data_in[0] = spi_reg->spi_status& 0xff;
			data_count = 0;
		}

		while (data_count > 0) {
			if ((data_count / 4) > 0) {
				time = AV1_GetStc32();
				while ((spi_reg->spi_status_2 & SPI_SRAM_ST ) == SRAM_EMPTY) {
					timeout = AV1_GetStc32();
					if ((timeout - time) > SPI_TIMEOUT) {
						msg_printf("timeout \n");
						break;
					}
				}

				data_temp = spi_reg->spi_data64;
				//msg_printf("data_temp 0x%x\n",data_temp);
				data_in[0] = data_temp & 0xff;
				data_in[1] = ((data_temp & 0xff00) >> 8);
				data_in[2] = ((data_temp & 0xff0000) >> 16);
				data_in[3] = ((data_temp & 0xff000000) >> 24);
				data_in = data_in + 4;
				data_count = data_count - 4;
			} else {
				time = AV1_GetStc32();
				while ((spi_reg->spi_status_2 & SPI_SRAM_ST ) == SRAM_EMPTY) {
					timeout = AV1_GetStc32();
					if ((timeout - time) > SPI_TIMEOUT) {
						msg_printf("timeout \n");
						break;
					}
				}

				data_temp = spi_reg->spi_data64;
				//msg_printf("data_temp 0x%x\n",data_temp);
				if (data_count%4 == 3) {
					data_in[0] = data_temp & 0xff;
					data_in[1] = ((data_temp & 0xff00) >> 8);
					data_in[2] = ((data_temp & 0xff0000) >> 16);
					data_count = data_count-3;
				} else if (data_count%4 == 2) {
					data_in[0] = data_temp & 0xff;
					data_in[1] = ((data_temp & 0xff00) >> 8);
					data_count = data_count-2;
				} else if (data_count%4 == 1) {
					data_in[0] = data_temp & 0xff;
					data_count = data_count-1;
				}
			}
		}

		addr_offset = addr_offset + 1;
		while ((spi_nor_read_status1() & 0x01) != 0) {
			msg_printf("wait DEVICE busy\n");
		}
	}

	while ((spi_reg->spi_auto_cfg & PIO_TRIGGER) != 0) {
		msg_printf("wait PIO_TRIGGER\n");
	}

	while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
		msg_printf("wait spi_reg->spi_ctrl 0x%x\n",spi_reg->spi_ctrl);
	}

	spi_reg->spi_cfg0 &= DATA64_DIS;
	if (fast_read == 1) {
		spi_fast_read_disable();
	}
	return 0;
}

static int spi_flash_xfer_write(UINT8 *cmd, size_t cmd_len, const void *data, size_t data_len)
{
	diag_printf("%s\n",__FUNCTION__);
	UINT32 total_count = data_len;
	UINT32 data_count = 0;
	UINT32 addr_offset = 0;
	UINT32 addr_temp = 0;
	UINT8 *data_in = (UINT8 *) data;
	UINT32 data_temp = 0;
	//UINT8 addr_len = 0;
	UINT32 timeout = 0;
	UINT32 time = 0;
	UINT32 ctrl = 0;

	msg_printf("data length %d\n",data_len);
	if (total_count == 0) {
		while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
			msg_printf("wait spi_reg->spi_ctrl 0x%x\n",spi_reg->spi_ctrl);
		}

		ctrl = spi_reg->spi_ctrl & CLEAR_CUST_CMD;
		ctrl = ctrl | WRITE | BYTE_0 | ADDR_0B | CUST_CMD(cmd[0]);
		spi_reg->spi_cfg0 = (spi_reg->spi_cfg0 & CLEAR_DATA64_LEN) | data_count | DATA64_EN;
		spi_reg->spi_buf_addr = DATA64_READ_ADDR(0) | DATA64_WRITE_ADDR(0);

		if (cmd_len > 1) {
			addr_temp = (cmd[1] << 16) | (cmd[2] << 8) | cmd[3];
			spi_reg->spi_page_addr = addr_temp;
			ctrl = ctrl | ADDR_3B ;
			msg_printf("addr 0x%x\n", spi_reg->spi_page_addr);
		}

		spi_reg->spi_data = 0;
		spi_reg->spi_ctrl = ctrl;
		spi_reg->spi_auto_cfg |= PIO_TRIGGER;
		//msg_printf("spi_reg->spi_ctrl 0x%x\n", spi_reg->spi_ctrl);
		//msg_printf("spi_reg->spi_page_addr 0x%x\n", spi_reg->spi_page_addr);
		//msg_printf("spi_reg->spi_cfg0 0x%x\n", spi_reg->spi_cfg0);
	}

	while (total_count > 0) {
		if (total_count > SPI_DATA64_MAX_LEN) {
			total_count = total_count - SPI_DATA64_MAX_LEN;
			data_count = SPI_DATA64_MAX_LEN;
		} else {
			data_count = total_count;
			total_count = 0;
		}

		while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
			msg_printf("wait spi_reg->spi_ctrl 0x%x\n",spi_reg->spi_ctrl);
		}

		ctrl = spi_reg->spi_ctrl & CLEAR_CUST_CMD;
		ctrl = ctrl | WRITE | BYTE_0 | ADDR_0B | CUST_CMD(cmd[0]);
		spi_reg->spi_cfg0 = (spi_reg->spi_cfg0 & CLEAR_DATA64_LEN) | data_count | DATA64_EN;
		spi_reg->spi_buf_addr = DATA64_READ_ADDR(0) | DATA64_WRITE_ADDR(0);
		if (cmd_len > 1) {
			addr_temp = (cmd[1] << 16) | (cmd[2] << 8) | cmd[3];
			addr_temp = addr_temp + addr_offset * SPI_DATA64_MAX_LEN;
			spi_reg->spi_page_addr = addr_temp;
			ctrl = ctrl | ADDR_3B ;
			msg_printf("addr 0x%x\n", spi_reg->spi_page_addr);
		}

		spi_reg->spi_data = 0;
		spi_reg->spi_ctrl = ctrl;
		spi_reg->spi_auto_cfg |= PIO_TRIGGER;
		//msg_printf("spi_reg->spi_ctrl 0x%x\n", spi_reg->spi_ctrl);
		//msg_printf("spi_reg->spi_page_addr 0x%x\n", spi_reg->spi_page_addr);
		//msg_printf("spi_reg->spi_cfg0 0x%x\n", spi_reg->spi_cfg0);

		while (data_count > 0) {
			if ((data_count / 4) > 0) {
				if ((spi_reg->spi_status_2 & SPI_SRAM_ST) == SRAM_FULL) {
					time = AV1_GetStc32();
					while ((spi_reg->spi_status_2 & SPI_SRAM_ST) != SRAM_EMPTY) {
						timeout = AV1_GetStc32();
						if ((timeout - time) > SPI_TIMEOUT) {
							msg_printf("timeout \n");
							break;
						}
					}
				}

				data_temp = (data_in[3] << 24) | (data_in[2] << 16) | (data_in[1] << 8) | data_in[0];
				spi_reg->spi_data64 = data_temp;
				data_in = data_in + 4;
				data_count = data_count - 4;
			} else {
				if ((spi_reg->spi_status_2 & SPI_SRAM_ST) == SRAM_FULL) {
					time = AV1_GetStc32();
					while ((spi_reg->spi_status_2 & SPI_SRAM_ST) != SRAM_EMPTY) {
						timeout = AV1_GetStc32();
						if ((timeout - time) > SPI_TIMEOUT) {
							msg_printf("timeout \n");
							break;
						}
					}
				}

				//data_temp = data_in[0] & 0xff;
				if ((data_count%4) == 3) {
					data_temp = (data_in[2] << 16) | (data_in[1] << 8) | data_in[0];
					data_count = data_count-3;
				} else if ((data_count%4) == 2) {
					data_temp =  (data_in[1] << 8) | data_in[0];
					data_count = data_count-2;
				} else if ((data_count%4) == 1) {
					data_temp = data_in[0];
					data_count = data_count-1;
				}
				spi_reg->spi_data64 = data_temp;
			}
		}
		addr_offset = addr_offset + 1;

		while ((spi_nor_read_status1() & 0x01) != 0) {
			msg_printf("wait DEVICE busy\n");
		}
	}

	while ((spi_reg->spi_auto_cfg & PIO_TRIGGER) != 0) {
		msg_printf("wait PIO_TRIGGER\n");
	}

	while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
		msg_printf("wait spi_reg->spi_ctrl 0x%x\n",spi_reg->spi_ctrl);
	}

	spi_reg->spi_cfg0 &= DATA64_DIS;
	return 0;
}
#endif

static int sp_spi_nor_ofdata_to_platdata(struct udevice *bus)
{
	struct sp_spi_nor_platdata *plat = bus->platdata;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(bus);

	msg_printf("%s\n",__FUNCTION__);
	plat->regs = (struct sp_spi_nor_regs *)fdtdec_get_addr(blob, node, "reg");
	plat->clock = fdtdec_get_int(blob, node, "spi-max-frequency", 50000000);
	plat->chipsel = fdtdec_get_int(blob, node, "chip-selection", 0);

	return 0;
}

static int sp_spi_nor_probe(struct udevice *bus)
{
	struct sp_spi_nor_platdata *plat = dev_get_platdata(bus);
	struct sp_spi_nor_priv *priv = dev_get_priv(bus);
#if (SP_SPINOR_DMA)
	struct spinorbufdesc *wdesc = &priv->wchain;
	struct spinorbufdesc *rdesc = &priv->rchain;
	struct spinorbufdesc *tempdesc;
	UINT32 *w_buffer_adr = (UINT32 *)&priv->w_buf[0];
	UINT32 *r_buffer_adr = (UINT32 *)&priv->r_buf[0];
#endif

	diag_printf("%s\n",__FUNCTION__);
	priv->regs = plat->regs;
	priv->clock = plat->clock;

	spi_reg = (sp_spi_nor_regs *)priv->regs;
	cmd_buf = malloc(CMD_BUF_LEN * sizeof(UINT8));

#if (SP_SPINOR_DMA)
	msg_printf("buffsddr 0x%x\n", cmd_buf);
	msg_printf("wdesc 0x%x rdesc 0x%x\n", wdesc, rdesc);
	msg_printf("w_buffer_adr 0x%x size 0x%x r_buffer_adr 0x%x\n", w_buffer_adr, CFG_BUFF_MAX, r_buffer_adr);
	flush_dcache_range((ulong)w_buffer_adr, (ulong)w_buffer_adr + CFG_BUFF_MAX);
	flush_dcache_range((ulong)r_buffer_adr, (ulong)r_buffer_adr + CFG_BUFF_MAX);

	tempdesc = wdesc;
	tempdesc->size = CFG_BUFF_MAX;
	tempdesc->phys = (dma_addr_t)w_buffer_adr;
	flush_dcache_range((ulong)(&priv->wchain), (ulong)(&priv->wchain) + sizeof(priv->wchain));

	tempdesc = rdesc;
	tempdesc->size = CFG_BUFF_MAX;
	tempdesc->phys = (dma_addr_t)r_buffer_adr;
	flush_dcache_range((ulong)(&priv->rchain),(ulong)(&priv->rchain) + sizeof(priv->rchain));
	msg_printf("wdesc->phys 0x%x,  rdesc->phys 0x%x\n", wdesc->phys, rdesc->phys);
#endif
	return 0;
}

static int sp_spi_nor_remove(struct udevice *dev)
{
	diag_printf("%s\n",__FUNCTION__);
	free(cmd_buf);
	return 0;
}

static int sp_spi_nor_claim_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct sp_spi_nor_platdata *plat =  bus->platdata;
	//set pinmux
	UINT32* grp1_sft_cfg = (UINT32 *) 0x9c000080;
	int value = 0;

	diag_printf("%s\n",__FUNCTION__);
	grp1_sft_cfg[1] = RF_MASK_V(0xf, (2 << 2) | 2);

	if (plat->chipsel == 0)
		value = A_CHIP;
	else
		value = B_CHIP;

	switch (plat->clock) {
	case 100000000:
		value |= SPI_CLK_D_2;
		break;
	case 50000000:
		value |= SPI_CLK_D_4;
		break;
	case 33000000:
		value |= SPI_CLK_D_6;
		break;
	case 25000000:
		value |= SPI_CLK_D_8;
		break;
	case 12000000:
		value |= SPI_CLK_D_16;
		break;
	case  8000000:
		value |= SPI_CLK_D_24;
		break;
	case  6000000:
	default:
		value |= SPI_CLK_D_32;
		break;
	}

#if (SP_SPINOR_DMA)
	spi_reg->spi_ctrl = value;
	//value = spi_reg->spi_timing;
	spi_reg->spi_timing = (2<<22) | (0x16<<16) | (1<<1);
	msg_printf("ctrl 0x%x spi_timing 0x%x\n",spi_reg->spi_ctrl, spi_reg->spi_timing);
#else
	spi_reg->spi_ctrl = value;//SPI_CLK_D_16 = 62M
#endif
	spi_reg->spi_cfg1 = SPI_CMD_OEN_1b | SPI_ADDR_OEN_1b | SPI_DATA_OEN_1b | SPI_CMD_1b | SPI_ADDR_1b |
			    SPI_DATA_1b | SPI_ENHANCE_NO | SPI_DUMMY_CYC(0) | SPI_DATA_IEN_DQ1;
	spi_reg->spi_auto_cfg &= ~(AUTO_RDSR_EN);

	return 0;
}

static int sp_spi_nor_release_bus(struct udevice *dev)
{
	diag_printf("%s\n",__FUNCTION__);
	return 0;
}

static int sp_spi_nor_xfer(struct udevice *dev, unsigned int bitlen,
				const void *dout, void *din, unsigned long flags)
{
#if (SP_SPINOR_DMA)
	struct udevice *bus = dev->parent;
	//struct sp_spi_nor_platdata *pdata = dev_get_platdata(bus);
	struct sp_spi_nor_priv *priv = dev_get_priv(bus);
#endif
	unsigned int len;
	int flc = 0;

	diag_printf("%s\n", __FUNCTION__);
	if (bitlen == 0)
		goto out;

	if (bitlen % 8)
		goto out;

	len = bitlen / 8;

	if (flags & SPI_XFER_BEGIN) {
		if (len > 0 && dout) {
			cmd_len = len;
			memset(cmd_buf, 0, CMD_BUF_LEN);
			memcpy(cmd_buf, dout, len);
			msg_printf("cmd %x\n", cmd_buf[0]);
			msg_printf("addr 0x%x\n", cmd_buf[1] << 16 | cmd_buf[2] << 8 | cmd_buf[3]);
			msg_printf("cmd len %d, flags %d\n", len, flags);
		}

		if (!(flags & SPI_XFER_END))
			goto out;
		else
			flc = 1;
	}

	if (!dout) {
		// read
		msg_printf("read\n");
#if (SP_SPINOR_DMA)
		if (cmd_buf[0] == 0x0B)
			cmd_buf[0] = 0xEB;

		spi_flash_xfer_DMAread(priv, cmd_buf, cmd_len, din, len);
#else
		spi_flash_xfer_read(cmd_buf, cmd_len, din, len);
#endif
	} else if ((!din) | (flags & SPI_XFER_END)) {
		// write
#if (SP_SPINOR_DMA)
		if (cmd_buf[0] == 0x06)
			goto out;
		if (cmd_buf[0] == 0x02)
			cmd_buf[0] = 0x32;

		msg_printf("write\n");
		if (flc == 1)
			spi_flash_xfer_DMAwrite(priv, cmd_buf, cmd_len, NULL, 0);
		else
			spi_flash_xfer_DMAwrite(priv, cmd_buf, cmd_len, dout, len);
#else
		msg_printf("write\n");
		if (flc == 1)
			spi_flash_xfer_write(cmd_buf, cmd_len, NULL, 0);
		else
			spi_flash_xfer_write(cmd_buf, cmd_len, dout, len);
#endif
	}

out:
	return 0;
}

static int sp_spi_nor_set_speed(struct udevice *bus, uint speed)
{
	struct sp_spi_nor_platdata *plat = bus->platdata;
	struct sp_spi_nor_priv *priv = dev_get_priv(bus);

	diag_printf("%s %d\n",__FUNCTION__, speed);

	if (speed > plat->clock)
		speed = plat->clock;

	//set spi nor clock
	priv->clock = speed;
	debug("%s: regs=%p, speed=%d\n", __func__, priv->regs, priv->clock);

	return 0;
}

static int sp_spi_nor_set_mode(struct udevice *bus, uint mode)
{
	struct sp_spi_nor_priv *priv = dev_get_priv(bus);
	//uint32_t reg;

	diag_printf("%s %d\n",__FUNCTION__, mode);

	//set spi nor mode
	debug("%s: regs=%p, mode=%d\n", __func__, priv->regs, priv->mode);

	return 0;
}

static const struct dm_spi_ops sp_spi_nor_ops = {
	.claim_bus      = sp_spi_nor_claim_bus,
	.release_bus    = sp_spi_nor_release_bus,
	.xfer           = sp_spi_nor_xfer,
	.set_speed      = sp_spi_nor_set_speed,
	.set_mode       = sp_spi_nor_set_mode,
};

static const struct udevice_id sp_spi_nor_ids[] = {
	{ .compatible = "sunplus,sunplus-q628-spi-nor" },
	{ }
};

U_BOOT_DRIVER(sp_spi_nor) = {
	.name           = "sp_spi_nor",
	.id             = UCLASS_SPI,
	.of_match       = sp_spi_nor_ids,
	.ops            = &sp_spi_nor_ops,
	.ofdata_to_platdata = sp_spi_nor_ofdata_to_platdata,
	.probe          = sp_spi_nor_probe,
	.remove         = sp_spi_nor_remove,
	.platdata_auto_alloc_size = sizeof(struct sp_spi_nor_platdata),
	.priv_auto_alloc_size     = sizeof(struct sp_spi_nor_priv),
#if (SP_SPINOR_DMA)
	.flags          = DM_FLAG_ALLOC_PRIV_DMA,
#endif
};
/*
probe flow
1.ofdata_to_platdata
2.set_speed
3.set_mode
4.claim bus
*/
