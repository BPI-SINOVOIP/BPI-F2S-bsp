#include <linux/delay.h>
#include <linux/io.h>
#include "hal_i2c.h"

#define I2C_CLK_SOURCE_FREQ         27000  // KHz(27MHz)

#define I2C_CTL0_FREQ_MASK                  (0x7)     // 3 bit
#define I2C_CTL0_SLAVE_ADDR_MASK            (0x7F)    // 7 bit
#define I2C_CTL2_FREQ_CUSTOM_MASK           (0x7FF)   // 11 bit
#define I2C_CTL2_SCL_DELAY_MASK             (0x3)     // 2 bit
#define I2C_CTL7_RW_COUNT_MASK              (0xFFFF)  // 16 bit
#define I2C_EN0_CTL_EMPTY_THRESHOLD_MASK    (0x7)     // 3 bit
#define I2C_SG_DMA_LLI_INDEX_MASK           (0x1F)    // 5 bit


static regs_i2cm_t *pI2cMReg[I2C_MASTER_NUM];
#ifdef SUPPORT_I2C_GDMA
static regs_i2cm_gdma_t *pI2cMGdmaReg[I2C_MASTER_NUM];
#endif

#ifdef SUPPORT_I2C_GDMA
void hal_i2cm_sg_dma_go_set(unsigned int device_id)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		val = readl(&(pI2cMGdmaReg[device_id]->sg_setting));
		val |= I2C_SG_DMA_SET_DMA_GO;
		writel(val, &(pI2cMGdmaReg[device_id]->sg_setting));
	}
}
EXPORT_SYMBOL(hal_i2cm_sg_dma_go_set);

void hal_i2cm_sg_dma_rw_mode_set(unsigned int device_id, I2C_DMA_RW_Mode_e rw_mode)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		val = readl(&(pI2cMGdmaReg[device_id]->sg_dma_config));
		switch(rw_mode){
			default:
			case I2C_DMA_WRITE_MODE:
				val |= I2C_SG_DMA_CFG_DMA_MODE;
				break;

			case I2C_DMA_READ_MODE:
				val &= (~I2C_SG_DMA_CFG_DMA_MODE);
				break;
		}
		writel(val, &(pI2cMGdmaReg[device_id]->sg_dma_config));
		printk("%s(%d) sg_dma_config = 0x%x\n", __FUNCTION__, __LINE__, pI2cMGdmaReg[device_id]->sg_dma_config);
	}
}
EXPORT_SYMBOL(hal_i2cm_sg_dma_rw_mode_set);

void hal_i2cm_sg_dma_length_set(unsigned int device_id, unsigned int length)
{
	if(device_id < I2C_MASTER_NUM)
	{
		length &= (0xFFFF);  //only support 16 bit
		writel(length, &(pI2cMGdmaReg[device_id]->sg_dma_length));
	}
}
EXPORT_SYMBOL(hal_i2cm_sg_dma_length_set);

void hal_i2cm_sg_dma_addr_set(unsigned int device_id, unsigned int addr)
{
	if(device_id < I2C_MASTER_NUM)
	{
		writel(addr, &(pI2cMGdmaReg[device_id]->sg_dma_addr));
	}
}
EXPORT_SYMBOL(hal_i2cm_sg_dma_addr_set);

void hal_i2cm_sg_dma_last_lli_set(unsigned int device_id)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		val = readl(&(pI2cMGdmaReg[device_id]->sg_dma_config));
		val |= I2C_SG_DMA_CFG_LAST_LLI;
		writel(val, &(pI2cMGdmaReg[device_id]->sg_dma_config));
		printk("%s(%d) sg_dma_config = 0x%x\n", __FUNCTION__, __LINE__, pI2cMGdmaReg[device_id]->sg_dma_config);
	}
}
EXPORT_SYMBOL(hal_i2cm_sg_dma_last_lli_set);

void hal_i2cm_sg_dma_access_lli_set(unsigned int device_id, unsigned int access_index)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		access_index &= (I2C_SG_DMA_LLI_INDEX_MASK);  //only support 5 bit
		val = readl(&(pI2cMGdmaReg[device_id]->sg_dma_index));
		val &= (~I2C_SG_DMA_LLI_ACCESS_INDEX(I2C_SG_DMA_LLI_INDEX_MASK));
		val |= I2C_SG_DMA_LLI_ACCESS_INDEX(access_index);
		writel(val, &(pI2cMGdmaReg[device_id]->sg_dma_index));
	}
}
EXPORT_SYMBOL(hal_i2cm_sg_dma_access_lli_set);

void hal_i2cm_sg_dma_start_lli_set(unsigned int device_id, unsigned int start_index)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		start_index &= (I2C_SG_DMA_LLI_INDEX_MASK);   //only support 5 bit
		val = readl(&(pI2cMGdmaReg[device_id]->sg_dma_index));
		val &= (~I2C_SG_DMA_LLI_RUN_INDEX(I2C_SG_DMA_LLI_INDEX_MASK));
		val |= I2C_SG_DMA_LLI_RUN_INDEX(start_index);
		writel(val, &(pI2cMGdmaReg[device_id]->sg_dma_index));
	}
}
EXPORT_SYMBOL(hal_i2cm_sg_dma_start_lli_set);

void hal_i2cm_sg_dma_enable(unsigned int device_id)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		val = readl(&(pI2cMGdmaReg[device_id]->sg_setting));
		val |= I2C_SG_DMA_SET_DMA_ENABLE;
		writel(val, &(pI2cMGdmaReg[device_id]->sg_setting));
	}
}
EXPORT_SYMBOL(hal_i2cm_sg_dma_enable);

void hal_i2cm_dma_int_flag_clear(unsigned int device_id, unsigned int flag)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM){
		val = readl(&(pI2cMGdmaReg[device_id]->int_flag));
		val |= flag;
		writel(val, &(pI2cMGdmaReg[device_id]->int_flag));
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_int_flag_clear);

void hal_i2cm_dma_int_flag_get(unsigned int device_id, unsigned int *flag)
{
	if(device_id < I2C_MASTER_NUM)
	{
		*flag = readl(&(pI2cMGdmaReg[device_id]->int_flag));
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_int_flag_get);

void hal_i2cm_dma_int_en_set(unsigned int device_id, unsigned int dma_int)
{
	if(device_id < I2C_MASTER_NUM)
	{
		writel(dma_int, &(pI2cMGdmaReg[device_id]->int_en));
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_int_en_set);

void hal_i2cm_dma_go_set(unsigned int device_id)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		val = readl(&(pI2cMGdmaReg[device_id]->dma_config));
		val |= I2C_DMA_CFG_DMA_GO;
		writel(val, &(pI2cMGdmaReg[device_id]->dma_config));
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_go_set);

void hal_i2cm_dma_rw_mode_set(unsigned int device_id, I2C_DMA_RW_Mode_e rw_mode)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		val = readl(&(pI2cMGdmaReg[device_id]->dma_config));
		switch(rw_mode){
			default:
			case I2C_DMA_WRITE_MODE:
				val |= I2C_DMA_CFG_DMA_MODE;
				break;

			case I2C_DMA_READ_MODE:
				val &= (~I2C_DMA_CFG_DMA_MODE);
				break;
		}
		writel(val, &(pI2cMGdmaReg[device_id]->dma_config));
		//printk("%s(%d) dma_config = 0x%x\n", __FUNCTION__, __LINE__, pI2cMGdmaReg[device_id]->dma_config);
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_rw_mode_set);

void hal_i2cm_dma_length_set(unsigned int device_id, unsigned int length)
{
	if(device_id < I2C_MASTER_NUM)
	{
		length &= (0xFFFF);  //only support 16 bit
		writel(length, &(pI2cMGdmaReg[device_id]->dma_length));
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_length_set);

void hal_i2cm_dma_addr_set(unsigned int device_id, unsigned int addr)
{
	if(device_id < I2C_MASTER_NUM)
	{
		writel(addr, &(pI2cMGdmaReg[device_id]->dma_addr));
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_addr_set);

void hal_i2cm_dma_base_set(unsigned int device_id, void __iomem *membase)
{
	if (device_id < I2C_MASTER_NUM) {
		pI2cMGdmaReg[device_id] = (regs_i2cm_gdma_t *)membase;
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_base_set);

void hal_i2cm_dma_mode_disable(unsigned int device_id)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		val = readl(&(pI2cMReg[device_id]->i2cm_mode));
		val &= (~I2C_MODE_DMA_MODE);
		writel(val, &(pI2cMReg[device_id]->i2cm_mode));
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_mode_disable);

void hal_i2cm_dma_mode_enable(unsigned int device_id)
{
	unsigned int val;

	if(device_id < I2C_MASTER_NUM)
	{
		val = readl(&(pI2cMReg[device_id]->i2cm_mode));
		val |= I2C_MODE_DMA_MODE;
		writel(val, &(pI2cMReg[device_id]->i2cm_mode));
	}
}
EXPORT_SYMBOL(hal_i2cm_dma_mode_enable);
#endif


#ifdef I2C_RETEST
void hal_i2cm_scl_delay_read(unsigned int device_id, unsigned int *delay)
{
	if (device_id < I2C_MASTER_NUM) {
		*delay = readl(&(pI2cMReg[device_id]->control2));
		*delay &= I2C_CTL2_SCL_DELAY(I2C_CTL2_SCL_DELAY_MASK);
	}
}
EXPORT_SYMBOL(hal_i2cm_scl_delay_read);
#endif

void hal_i2cm_scl_delay_set(unsigned int device_id, unsigned int delay)
{
	unsigned int ctl2;

	if (device_id < I2C_MASTER_NUM) {
		ctl2 = readl(&(pI2cMReg[device_id]->control2));
		ctl2 &= (~I2C_CTL2_SCL_DELAY(I2C_CTL2_SCL_DELAY_MASK));
		ctl2 |= I2C_CTL2_SCL_DELAY(delay);

		//ctl2 |= I2C_CTL2_SDA_HALF_ENABLE;
		ctl2 &= (~(I2C_CTL2_SDA_HALF_ENABLE));

		
		writel(ctl2, &(pI2cMReg[device_id]->control2));
		//printk("hal_i2cm_scl_delay_set control2: 0x%x\n", readl(&(pI2cMReg[device_id]->control2)));
	}
}
EXPORT_SYMBOL(hal_i2cm_scl_delay_set);

void hal_i2cm_roverflow_flag_get(unsigned int device_id, unsigned int *flag)
{
	if (device_id < I2C_MASTER_NUM) {
		*flag = readl(&(pI2cMReg[device_id]->i2cm_status4));
	}
}
EXPORT_SYMBOL(hal_i2cm_roverflow_flag_get);

void hal_i2cm_rdata_flag_clear(unsigned int device_id, unsigned int flag)
{
	if (device_id < I2C_MASTER_NUM) {
		writel(flag, &(pI2cMReg[device_id]->control6));
		writel(0, &(pI2cMReg[device_id]->control6));
	}
}
EXPORT_SYMBOL(hal_i2cm_rdata_flag_clear);

void hal_i2cm_rdata_flag_get(unsigned int device_id, unsigned int *flag)
{
	if (device_id < I2C_MASTER_NUM) {
		*flag = readl(&(pI2cMReg[device_id]->i2cm_status3));
	}
}
EXPORT_SYMBOL(hal_i2cm_rdata_flag_get);

void hal_i2cm_status_clear(unsigned int device_id, unsigned int flag)
{
	unsigned int ctl1;

	if (device_id < I2C_MASTER_NUM) {
		ctl1 = readl(&(pI2cMReg[device_id]->control1));
		ctl1 |= flag;
		writel(ctl1, &(pI2cMReg[device_id]->control1));

		ctl1 = readl(&(pI2cMReg[device_id]->control1));
		ctl1 &= (~flag);
		writel(ctl1, &(pI2cMReg[device_id]->control1));
	}
}
EXPORT_SYMBOL(hal_i2cm_status_clear);

void hal_i2cm_status_get(unsigned int device_id, unsigned int *flag)
{
	if (device_id < I2C_MASTER_NUM) {
		*flag = readl(&(pI2cMReg[device_id]->interrupt));
	}
}
EXPORT_SYMBOL(hal_i2cm_status_get);

void hal_i2cm_int_en2_set(unsigned int device_id, unsigned int overflow_en)
{
	if (device_id < I2C_MASTER_NUM) {
		writel(overflow_en, &(pI2cMReg[device_id]->int_en2));
	}
}
EXPORT_SYMBOL(hal_i2cm_int_en2_set);

void hal_i2cm_int_en1_set(unsigned int device_id, unsigned int rdata_en)
{
	if (device_id < I2C_MASTER_NUM) {
		writel(rdata_en, &(pI2cMReg[device_id]->int_en1));
	}
}
EXPORT_SYMBOL(hal_i2cm_int_en1_set);

void hal_i2cm_int_en0_set(unsigned int device_id, unsigned int int0)
{
	if (device_id < I2C_MASTER_NUM) {
		writel(int0, &(pI2cMReg[device_id]->int_en0));
		//printk("hal_i2cm_int_en0_set int_en0: 0x%x\n", readl(&(pI2cMReg[device_id]->int_en0)));
	}
}
EXPORT_SYMBOL(hal_i2cm_int_en0_set);

void hal_i2cm_int_en0_disable(unsigned int device_id, unsigned int int0)
{
	unsigned int val;

	if (device_id < I2C_MASTER_NUM) {
		val = readl(&(pI2cMReg[device_id]->int_en0));
		val &= (~int0);
		writel(val, &(pI2cMReg[device_id]->int_en0));
		//printk("hal_i2cm_int_en0_disable int_en0: 0x%x\n", readl(&(pI2cMReg[device_id]->int_en0)));
	}
}
EXPORT_SYMBOL(hal_i2cm_int_en0_disable);

void hal_i2cm_int_en0_with_thershold_set(unsigned int device_id, unsigned int int0, unsigned char threshold)
{
	unsigned int val;

	if (device_id < I2C_MASTER_NUM) {
		val = (int0 | I2C_EN0_CTL_EMPTY_THRESHOLD(threshold));
		writel(val, &(pI2cMReg[device_id]->int_en0));
		//printk("hal_i2cm_int_en0_with_thershold_set int_en0: 0x%x\n", readl(&(pI2cMReg[device_id]->int_en0)));
	}
}
EXPORT_SYMBOL(hal_i2cm_int_en0_with_thershold_set);

void hal_i2cm_data_get(unsigned int device_id, unsigned int index, unsigned int *rdata)
{
	if (device_id < I2C_MASTER_NUM) {
		switch (index) {
			case 0:
				*rdata = readl(&(pI2cMReg[device_id]->data00_03));
				break;

			case 1:
				*rdata = readl(&(pI2cMReg[device_id]->data04_07));
				break;

			case 2:
				*rdata = readl(&(pI2cMReg[device_id]->data08_11));
				break;

			case 3:
				*rdata = readl(&(pI2cMReg[device_id]->data12_15));
				break;

			case 4:
				*rdata = readl(&(pI2cMReg[device_id]->data16_19));
				break;

			case 5:
				*rdata = readl(&(pI2cMReg[device_id]->data20_23));
				break;

			case 6:
				*rdata = readl(&(pI2cMReg[device_id]->data24_27));
				break;

			case 7:
				*rdata = readl(&(pI2cMReg[device_id]->data28_31));
				break;

			default:
				break;
		}
		//printk("hal_i2cm_data_get index: %d, data 0x%x\n", index, *rdata);
	}
}
EXPORT_SYMBOL(hal_i2cm_data_get);

void hal_i2cm_data_set(unsigned int device_id, unsigned int *wdata)
{
	if (device_id < I2C_MASTER_NUM) {
		writel(wdata[0], &(pI2cMReg[device_id]->data00_03));
		writel(wdata[1], &(pI2cMReg[device_id]->data04_07));
		writel(wdata[2], &(pI2cMReg[device_id]->data08_11));
		writel(wdata[3], &(pI2cMReg[device_id]->data12_15));
		writel(wdata[4], &(pI2cMReg[device_id]->data16_19));
		writel(wdata[5], &(pI2cMReg[device_id]->data20_23));
		writel(wdata[6], &(pI2cMReg[device_id]->data24_27));
		writel(wdata[7], &(pI2cMReg[device_id]->data28_31));
	}
}
EXPORT_SYMBOL(hal_i2cm_data_set);

void hal_i2cm_data0_set(unsigned int device_id, unsigned int *wdata)
{
	if (device_id < I2C_MASTER_NUM) {
		writel(*wdata, &(pI2cMReg[device_id]->data00_03));
	}
}
EXPORT_SYMBOL(hal_i2cm_data0_set);

void hal_i2cm_rw_mode_set(unsigned int device_id, I2C_RW_Mode_e rw_mode)
{
	unsigned int ctl0;

	if (device_id < I2C_MASTER_NUM) {
		ctl0 = readl(&(pI2cMReg[device_id]->control0));
		switch(rw_mode) {
			default:
			case I2C_WRITE_MODE:
				ctl0 &= (~(I2C_CTL0_PREFETCH | I2C_CTL0_RESTART_EN | I2C_CTL0_SUBADDR_EN));
				break;

			case I2C_READ_MODE:
				ctl0 &= (~(I2C_CTL0_RESTART_EN | I2C_CTL0_SUBADDR_EN));
				ctl0 |= I2C_CTL0_PREFETCH;
				break;

			case I2C_RESTART_MODE:
				ctl0 |= (I2C_CTL0_PREFETCH | I2C_CTL0_RESTART_EN | I2C_CTL0_SUBADDR_EN);
				break;
		}
		writel(ctl0, &(pI2cMReg[device_id]->control0));
	}
}
EXPORT_SYMBOL(hal_i2cm_rw_mode_set);

void hal_i2cm_manual_trigger(unsigned int device_id)
{
	unsigned int val;

	if (device_id < I2C_MASTER_NUM) {
		val = readl(&(pI2cMReg[device_id]->i2cm_mode));
		val |= I2C_MODE_MANUAL_TRIG;
		writel(val, &(pI2cMReg[device_id]->i2cm_mode));
	}
}
EXPORT_SYMBOL(hal_i2cm_manual_trigger);

void hal_i2cm_active_mode_set(unsigned int device_id, I2C_Active_Mode_e mode)
{
	unsigned int val;

	if (device_id < I2C_MASTER_NUM) {
		val = readl(&(pI2cMReg[device_id]->i2cm_mode));
		val &= (~(I2C_MODE_MANUAL_MODE | I2C_MODE_MANUAL_TRIG));
		switch (mode) {
			default:
			case I2C_TRIGGER:
				break;

			case I2C_AUTO:
				val |= I2C_MODE_MANUAL_MODE;
				break;
		}
		writel(val, &(pI2cMReg[device_id]->i2cm_mode));
	}
}
EXPORT_SYMBOL(hal_i2cm_active_mode_set);

void hal_i2cm_trans_cnt_set(unsigned int device_id, unsigned int write_cnt, unsigned int read_cnt)
{
	unsigned int t_write = write_cnt & I2C_CTL7_RW_COUNT_MASK;
	unsigned int t_read = read_cnt & I2C_CTL7_RW_COUNT_MASK;
	unsigned int ctl7;

	if (device_id < I2C_MASTER_NUM) {
		ctl7 = I2C_CTL7_WRCOUNT(t_write) | I2C_CTL7_RDCOUNT(t_read);
		writel(ctl7, &(pI2cMReg[device_id]->control7));
	}
}
EXPORT_SYMBOL(hal_i2cm_trans_cnt_set);

void hal_i2cm_slave_addr_set(unsigned int device_id, unsigned int addr)
{
	unsigned int t_addr = addr & I2C_CTL0_SLAVE_ADDR_MASK;
	unsigned int ctl0;

	if (device_id < I2C_MASTER_NUM) {
		ctl0 = readl(&(pI2cMReg[device_id]->control0));
		ctl0 &= (~I2C_CTL0_SLAVE_ADDR(I2C_CTL0_SLAVE_ADDR_MASK));
		ctl0 |= I2C_CTL0_SLAVE_ADDR(t_addr);
		writel(ctl0, &(pI2cMReg[device_id]->control0));
	}
}
EXPORT_SYMBOL(hal_i2cm_slave_addr_set);

void hal_i2cm_clock_freq_set(unsigned int device_id, unsigned int freq)
{
	unsigned int div;
	unsigned int ctl0, ctl2;

	if (device_id < I2C_MASTER_NUM) {
		div = I2C_CLK_SOURCE_FREQ / freq;
		div -= 1;
		if (0 != I2C_CLK_SOURCE_FREQ % freq) {
			div += 1;
		}

		if (div > I2C_CTL2_FREQ_CUSTOM_MASK) {
			div = I2C_CTL2_FREQ_CUSTOM_MASK;
		}

		ctl0 = readl(&(pI2cMReg[device_id]->control0));
		ctl0 &= (~I2C_CTL0_FREQ(I2C_CTL0_FREQ_MASK));
		writel(ctl0, &(pI2cMReg[device_id]->control0));

		ctl2 = readl(&(pI2cMReg[device_id]->control2));
		ctl2 &= (~I2C_CTL2_FREQ_CUSTOM(I2C_CTL2_FREQ_CUSTOM_MASK));
		ctl2 |= I2C_CTL2_FREQ_CUSTOM(div);
		writel(ctl2, &(pI2cMReg[device_id]->control2));
	}
}
EXPORT_SYMBOL(hal_i2cm_clock_freq_set);

void hal_i2cm_reset(unsigned int device_id)
{
	unsigned int ctl0;

	if (device_id < I2C_MASTER_NUM) {
		ctl0 = readl(&(pI2cMReg[device_id]->control0));
		ctl0 |= I2C_CTL0_SW_RESET;
		writel(ctl0, &(pI2cMReg[device_id]->control0));

		udelay(2);
	}
}
EXPORT_SYMBOL(hal_i2cm_reset);

void hal_i2cm_base_set(unsigned int device_id, void __iomem *membase)
{
	if (device_id < I2C_MASTER_NUM) {
		pI2cMReg[device_id] = (regs_i2cm_t *)membase;
	}
}
EXPORT_SYMBOL(hal_i2cm_base_set);
