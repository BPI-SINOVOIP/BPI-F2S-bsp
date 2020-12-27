#include <linux/delay.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/rtc.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/clk.h>   
#include <linux/reset.h> 
#include "hal_mipi_i2c.h"
#include "sp_mipi_i2c.h"

#ifdef CONFIG_PM_RUNTIME_MIPI_I2C
#include <linux/pm_runtime.h>
#endif


/* Message Definition */
//#define MIPI_I2C_FUNC_DEBUG
#define MIPI_I2C_FUNC_INFO
#define MIPI_I2C_FUNC_ERR

#ifdef MIPI_I2C_FUNC_DEBUG
	#define MIPI_I2C_DBG(fmt, args ...)     printk(KERN_INFO "[MIPI I2C] DBG: " fmt, ## args)
#else
	#define MIPI_I2C_DBG(fmt, args ...)
#endif
#ifdef MIPI_I2C_FUNC_INFO
	#define MIPI_I2C_INFO(fmt, args ...)    printk(KERN_INFO "[MIPI I2C] INFO: " fmt, ## args)
#else
	#define MIPI_I2C_INFO(fmt, args ...)
#endif
#ifdef MIPI_I2C_FUNC_ERR
	#define MIPI_I2C_ERR(fmt, args ...)     printk(KERN_ERR "[MIPI I2C] ERR: " fmt, ## args)
#else
	#define MIPI_I2C_ERR(fmt, args ...)
#endif

#define MIPI_ISP_REG_NAME    "mipi_isp"
#define MOON5_REG_NAME       "moon5"
#define DEVICE_NAME          "i143-mipi-i2c"

struct sp_mipi_i2c_info {
	void __iomem            *mipi_i2c_regs;
	struct moon5_reg        *moon5_regs;
	struct clk              *clkc_isp;
	struct clk              *clkc_ispapb;
	struct reset_control    *rstc_isp;
	struct reset_control    *rstc_ispapb;

	struct i2c_msg          *msgs;          /* messages currently handled */
	struct i2c_adapter      adap;

	unsigned int			mipi_i2c_freq;
	mipi_i2c_cmd_t          mipi_i2c_cmd;	

	// Store write message info for restart action
	unsigned char           restart_w_data[32];
	unsigned int            restart_write_cnt;
	unsigned int            restart_en;
};

typedef struct sp_mipi_i2c_info sp_mipi_i2c_info_t;

static int sp_mipi_i2c_init(unsigned int device_id, sp_mipi_i2c_info_t *sp_mipi_i2c_info)
{
	mipi_i2c_cmd_t *mipi_i2c_cmd = &sp_mipi_i2c_info->mipi_i2c_cmd;
	hal_mipi_i2c_info_t *hal_mipi_i2c_info = &mipi_i2c_cmd->dI2CInfo;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	if ((device_id < MIPI_I2C_1ST_CH) || (device_id > MIPI_I2C_MAX_TH))
	{
		MIPI_I2C_ERR("I2C device id is not correct !! device_id=%d\n", device_id);
		return MIPI_I2C_ERR_INVALID_DEVID;
	}

	MIPI_I2C_INFO("device_id=%d, mipi_isp_regs=0x%px\n", device_id, sp_mipi_i2c_info->mipi_i2c_regs);

	hal_mipi_i2c_info->isp_regs = (mipi_isp_reg_t *)(sp_mipi_i2c_info->mipi_i2c_regs - ISP_BASE_ADDRESS);
	hal_mipi_i2c_info->dev_id = device_id;

	// Reset MIPI ISP hardware
	hal_mipi_i2c_isp_reset(hal_mipi_i2c_info);
	hal_mipi_i2c_power_on(hal_mipi_i2c_info);
	hal_mipi_i2c_init(hal_mipi_i2c_info);

	// Reset MIPI I2C hardware
	hal_mipi_i2c_reset(hal_mipi_i2c_info);

	return MIPI_I2C_SUCCESS;
}

static int sp_mipi_i2c_get_register_base(struct platform_device *pdev, void **membase, const char *res_name)
{
	struct resource *r;
	void __iomem *p;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);
	MIPI_I2C_DBG("Resource name: %s\n", res_name);

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, res_name);
	if (r == NULL) {
		MIPI_I2C_ERR("platform_get_resource_byname fail!\n");
		return -ENODEV;
	}

	p = devm_ioremap(&pdev->dev, r->start, (r->end - r->start + 1));
	if (IS_ERR(p)) {
		MIPI_I2C_ERR("ioremap fail\n");
		return PTR_ERR(p);
	}

	MIPI_I2C_DBG("devm_ioremap addr: 0x%px!!\n", p);
	*membase = p;
	return MIPI_I2C_SUCCESS;
}

int sp_mipi_i2c_read(mipi_i2c_cmd_t *mipi_i2c_cmd)
{
	hal_mipi_i2c_info_t *hal_mipi_i2c_info = &mipi_i2c_cmd->dI2CInfo;
	unsigned char w_data[32] = {0};
	unsigned char status = 0;
	unsigned int read_cnt = 0;
	unsigned int write_cnt = 0;
	unsigned int current_cnt = 0;
	unsigned int receive_cnt = 0;
	unsigned int remainder_cnt = 0;
	unsigned int timeout = 0;
	int ret = MIPI_I2C_SUCCESS;
	int i = 0;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	if ((hal_mipi_i2c_info->dev_id < MIPI_I2C_1ST_CH) || (hal_mipi_i2c_info->dev_id > MIPI_I2C_MAX_TH)) {
		MIPI_I2C_ERR("I2C device id is not correct !! device_id=%d\n", hal_mipi_i2c_info->dev_id);
		return MIPI_I2C_ERR_INVALID_DEVID;
	}

	hal_mipi_i2c_status_get(hal_mipi_i2c_info, &status);
	if (status == 0x01) {
		MIPI_I2C_ERR("I2C is busy !!\n");
		return MIPI_I2C_ERR_I2C_BUSY;
	}

	write_cnt = mipi_i2c_cmd->dWrDataCnt;
	read_cnt = mipi_i2c_cmd->dRdDataCnt;

	if (mipi_i2c_cmd->dRestartEn)
	{
		if (write_cnt > 32) {
			MIPI_I2C_ERR("I2C write count is invalid !! write count=%d\n", write_cnt);
			return MIPI_I2C_ERR_INVALID_CNT;
		}
	}

	if ((read_cnt > 0xFFFF)  || (read_cnt == 0)) {
		MIPI_I2C_ERR("I2C read count is invalid !! read count=%d\n", read_cnt);
		return MIPI_I2C_ERR_INVALID_CNT;
	}

	MIPI_I2C_DBG("write_cnt=%d, read_cnt=%d\n", write_cnt, read_cnt);
	while (current_cnt < read_cnt)
	{
		remainder_cnt = read_cnt - current_cnt;
		receive_cnt = (remainder_cnt > 32)? 32 : remainder_cnt;
		//current_cnt += receive_cnt;
		MIPI_I2C_DBG("current_cnt=%d, receive_cnt=%d\n", current_cnt, receive_cnt);

		hal_mipi_i2c_clock_freq_set(hal_mipi_i2c_info, mipi_i2c_cmd->dFreq);
		hal_mipi_i2c_slave_addr_set(hal_mipi_i2c_info, mipi_i2c_cmd->dSlaveAddr);
		hal_mipi_i2c_rw_mode_set(hal_mipi_i2c_info, I2C_NOR_NEW_READ);
		hal_mipi_i2c_active_mode_set(hal_mipi_i2c_info, MIPI_I2C_2603_SYNC_IM);
		hal_mipi_i2c_trans_cnt_set(hal_mipi_i2c_info, write_cnt, receive_cnt);

		if (mipi_i2c_cmd->dRestartEn) {
			MIPI_I2C_DBG("I2C_RESTART_MODE\n");
			for (i = 0; i < write_cnt; i++) {
				w_data[i] = mipi_i2c_cmd->pWrData[i];
			}
			hal_mipi_i2c_data_set(hal_mipi_i2c_info, w_data, write_cnt);
			hal_mipi_i2c_read_trigger(hal_mipi_i2c_info, I2C_RESTART);
		} else {
			MIPI_I2C_DBG("I2C_READ_MODE\n");
			hal_mipi_i2c_read_trigger(hal_mipi_i2c_info, I2C_START);
		}		

		// We will always wait for a fraction of a second!
		timeout = 0;
		do {
			usleep_range(100, 200);
			hal_mipi_i2c_status_get(hal_mipi_i2c_info, &status);
		} while ((status == MIPI_I2C_2650_BUSY) && (timeout++ < TIMEOUT_TH));

		if (timeout >= TIMEOUT_TH) {
			MIPI_I2C_ERR("I2C write timeout!!\n");
			hal_mipi_i2c_reset(hal_mipi_i2c_info);
			return ret = MIPI_I2C_ERR_TIMEOUT_OUT;
		} else {
			MIPI_I2C_DBG("I2C write done.\n");
		}

		// Read data
		hal_mipi_i2c_data_get(hal_mipi_i2c_info, &mipi_i2c_cmd->pRdData[current_cnt], receive_cnt);
		current_cnt += receive_cnt;

		// We will always wait for a fraction of a second!
		timeout = 0;
		do {
			usleep_range(100, 200);
			hal_mipi_i2c_status_get(hal_mipi_i2c_info, &status);
		} while ((status == MIPI_I2C_2650_BUSY) && (timeout++ < TIMEOUT_TH));
		
		if (timeout >= TIMEOUT_TH) {
			MIPI_I2C_ERR("I2C read timeout!!\n");
			hal_mipi_i2c_reset(hal_mipi_i2c_info);
			return ret = MIPI_I2C_ERR_TIMEOUT_OUT;
		} else {
			MIPI_I2C_DBG("I2C read done.\n");
		}
	}

	return ret;
}

int sp_mipi_i2c_write(mipi_i2c_cmd_t *mipi_i2c_cmd)
{
	hal_mipi_i2c_info_t *hal_mipi_i2c_info = &mipi_i2c_cmd->dI2CInfo;
	unsigned char w_data[32] = {0};
	unsigned char status = 0;
	unsigned int write_cnt = 0;
	unsigned int current_cnt = 0;
	unsigned int send_cnt = 0;
	unsigned int remainder_cnt = 0;
	unsigned int timeout = 0;
	int ret = MIPI_I2C_SUCCESS;
	int i = 0;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	if ((hal_mipi_i2c_info->dev_id < MIPI_I2C_1ST_CH) || (hal_mipi_i2c_info->dev_id > MIPI_I2C_MAX_TH)) {
		MIPI_I2C_ERR("I2C device id is not correct !! device_id=%d\n", hal_mipi_i2c_info->dev_id);
		return MIPI_I2C_ERR_INVALID_DEVID;
	}

	hal_mipi_i2c_status_get(hal_mipi_i2c_info, &status);
	if (status == 0x01) {
		MIPI_I2C_ERR("I2C is busy !!\n");
		return MIPI_I2C_ERR_I2C_BUSY;
	}

	write_cnt = mipi_i2c_cmd->dWrDataCnt;

	if (write_cnt > 0xFFFF) {
		MIPI_I2C_ERR("I2C write count is invalid !! write count=%d\n", write_cnt);
		return MIPI_I2C_ERR_INVALID_CNT;
	}

	while (current_cnt < write_cnt)
	{
		remainder_cnt = write_cnt - current_cnt;
		send_cnt = (remainder_cnt > 32)? 32 : remainder_cnt;
		for(i = 0; i < send_cnt; i++){
			w_data[i] = mipi_i2c_cmd->pWrData[i];
		}
		current_cnt += send_cnt;
		MIPI_I2C_DBG("current_cnt=%d, send_cnt=%d\n", current_cnt, send_cnt);

		hal_mipi_i2c_clock_freq_set(hal_mipi_i2c_info, mipi_i2c_cmd->dFreq);
		hal_mipi_i2c_slave_addr_set(hal_mipi_i2c_info, mipi_i2c_cmd->dSlaveAddr);
		hal_mipi_i2c_rw_mode_set(hal_mipi_i2c_info, I2C_NOR_NEW_WRITE);
		hal_mipi_i2c_active_mode_set(hal_mipi_i2c_info, MIPI_I2C_2603_SYNC_IM);
		hal_mipi_i2c_trans_cnt_set(hal_mipi_i2c_info, send_cnt, 0xff);
		hal_mipi_i2c_data_set(hal_mipi_i2c_info, w_data, send_cnt);  // Write data to trigger write transaction

		// We will always wait for a fraction of a second!
		timeout = 0;
		do {
			usleep_range(100, 200);
			hal_mipi_i2c_status_get(hal_mipi_i2c_info, &status);
		} while ((status == MIPI_I2C_2650_BUSY) && (timeout++ < TIMEOUT_TH));

		if (timeout >= TIMEOUT_TH) {
			MIPI_I2C_ERR("I2C write timeout!!\n");
			ret = MIPI_I2C_ERR_TIMEOUT_OUT;
			break;
		} else {
			MIPI_I2C_DBG("I2C write done.\n");
		}
	}
	return ret;
}

static int sp_mipi_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	sp_mipi_i2c_info_t *sp_mipi_i2c_info = (sp_mipi_i2c_info_t *)adap->algo_data;
	mipi_i2c_cmd_t *mipi_i2c_cmd = &(sp_mipi_i2c_info->mipi_i2c_cmd);
	int ret = MIPI_I2C_SUCCESS;
	int i = 0, j = 0;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

#ifdef CONFIG_PM_RUNTIME_MIPI_I2C
	ret = pm_runtime_get_sync(&adap->dev);
	if (ret < 0)
		goto out;  
#endif
	if (num == 0) {
		MIPI_I2C_ERR("The message number is 0!\n");
		return -EINVAL;
	}

	memset(mipi_i2c_cmd, 0, sizeof(mipi_i2c_cmd_t));
	mipi_i2c_cmd->dI2CInfo.isp_regs = (mipi_isp_reg_t *)(sp_mipi_i2c_info->mipi_i2c_regs - ISP_BASE_ADDRESS);
	mipi_i2c_cmd->dI2CInfo.dev_id = adap->nr;
	mipi_i2c_cmd->dFreq = sp_mipi_i2c_info->mipi_i2c_freq;
	MIPI_I2C_DBG("I2C freq=%d\n", mipi_i2c_cmd->dFreq);

	// Init variables for restart action
	memset(sp_mipi_i2c_info->restart_w_data, 0, sizeof(sp_mipi_i2c_info->restart_w_data));
	sp_mipi_i2c_info->restart_write_cnt = 0;
	sp_mipi_i2c_info->restart_en = 0;

	for (i = 0; i < num; i++) {
		MIPI_I2C_DBG("i=%d, num=%d\n", i, num);

		if (msgs[i].flags & I2C_M_TEN) {
			MIPI_I2C_ERR("The I2C_M_TEN flag is enabled!\n");
			return -EINVAL;
		}

		MIPI_I2C_DBG("msgs[i].addr=0x%04x, msgs[i].flags=0x%04x, msgs[i].len=%d\n",
			msgs[i].addr, msgs[i].flags, msgs[i].len);

		mipi_i2c_cmd->dSlaveAddr = msgs[i].addr;

		if (msgs[i].flags & I2C_M_NOSTART){
			MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

			//mipi_i2c_cmd->dWrDataCnt = msgs[i].len;
			//mipi_i2c_cmd->pWrData = msgs[i].buf;
            sp_mipi_i2c_info->restart_write_cnt = msgs[i].len;
	    	for (j = 0; j < sp_mipi_i2c_info->restart_write_cnt; j++) {
		   		sp_mipi_i2c_info->restart_w_data[j] = msgs[i].buf[j];
	    	}
        	sp_mipi_i2c_info->restart_en = 1;						
			continue;
		}

		if (msgs[i].flags & I2C_M_RD) {
			MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

			mipi_i2c_cmd->dRdDataCnt = msgs[i].len;
			mipi_i2c_cmd->pRdData = msgs[i].buf;
			if (sp_mipi_i2c_info->restart_en == 1) {
		    	mipi_i2c_cmd->dWrDataCnt = sp_mipi_i2c_info->restart_write_cnt;
		    	mipi_i2c_cmd->pWrData = sp_mipi_i2c_info->restart_w_data;				
		    	MIPI_I2C_DBG("I2C_M_RD dWrDataCnt=%d\n", mipi_i2c_cmd->dWrDataCnt);
		    	MIPI_I2C_DBG("I2C_M_RD mipi_i2c_cmd->pWrData[0]=%x\n", mipi_i2c_cmd->pWrData[0]);
			    MIPI_I2C_DBG("I2C_M_RD mipi_i2c_cmd->pWrData[1]=%x\n", mipi_i2c_cmd->pWrData[1]);	
		    	sp_mipi_i2c_info->restart_en = 0;			
			    mipi_i2c_cmd->dRestartEn = 1;
	    	}
			ret = sp_mipi_i2c_read(mipi_i2c_cmd);
		} else {
			MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

			mipi_i2c_cmd->dWrDataCnt = msgs[i].len;
			mipi_i2c_cmd->pWrData = msgs[i].buf;
			ret = sp_mipi_i2c_write(mipi_i2c_cmd);
		}

		if (ret != MIPI_I2C_SUCCESS) {
			MIPI_I2C_ERR("MIPI I2C xfer failed!\n");
			return -EIO;
		}
	}

#ifdef CONFIG_PM_RUNTIME_MIPI_I2C
	   pm_runtime_put(&adap->dev);
       // pm_runtime_put_autosuspend(&adap->dev);

#endif

	return num;
	
#ifdef CONFIG_PM_RUNTIME_MIPI_I2C
	out :
		pm_runtime_mark_last_busy(&adap->dev);
		pm_runtime_put_autosuspend(&adap->dev);
	 return num;
#endif
}

static u32 sp_mipi_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm sp_mipi_i2c_algorithm = {
	.master_xfer   = sp_mipi_i2c_master_xfer,
	.functionality = sp_mipi_i2c_func,
};

static int sp_mipi_i2c_probe(struct platform_device *pdev)
{
	sp_mipi_i2c_info_t *sp_mipi_i2c_info = NULL;
	struct i2c_adapter *p_adap;
	struct device *dev = &pdev->dev;
	unsigned int temp_value;
	int device_id = 0;
	int i2c_ch = 0;
	int ret = MIPI_I2C_SUCCESS;

	MIPI_I2C_INFO("SP MIPI I2C driver is probed\n");

	if (pdev->dev.of_node) {
		pdev->id = of_alias_get_id(pdev->dev.of_node, "i2c");
		device_id = pdev->id;
		i2c_ch = device_id - MIPI_I2C_1ST_CH;
		MIPI_I2C_INFO("pdev->id=%d, MIPI I2C channel=%d\n", pdev->id, i2c_ch);
	}

	// Allocate memory for 'sp_mipi_i2c_info'.
	sp_mipi_i2c_info = kzalloc(sizeof(sp_mipi_i2c_info_t), GFP_KERNEL);
	MIPI_I2C_DBG("sp_mipi_i2c_info:%px\n", sp_mipi_i2c_info);
	if (!sp_mipi_i2c_info) {
		MIPI_I2C_ERR("Failed to allocate memory!\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	// Get 'i2c-freq' property.
	if (!of_property_read_u32(pdev->dev.of_node, "i2c-freq", &temp_value)) {
		dev_dbg(&pdev->dev,"i2c freq setting: %d\n",temp_value);
		sp_mipi_i2c_info->mipi_i2c_freq = temp_value;
	} else {
		// Set 3 as default setting
		sp_mipi_i2c_info->mipi_i2c_freq = 3;
	}
	MIPI_I2C_INFO("set i2c freq setting: %d\n", sp_mipi_i2c_info->mipi_i2c_freq);

	// Get and set 'mipi_isp' register base.
	ret = sp_mipi_i2c_get_register_base(pdev, (void**)&sp_mipi_i2c_info->mipi_i2c_regs, MIPI_ISP_REG_NAME);
	if (ret) {
		return ret;
	}	

	// Get and set 'moon5' register base.
	ret = sp_mipi_i2c_get_register_base(pdev, (void**)&sp_mipi_i2c_info->moon5_regs, MOON5_REG_NAME);
	if (ret) {
		return ret;
	}

	// Get ISP clock resource and enable it 
	sp_mipi_i2c_info->clkc_isp = devm_clk_get(dev, "clkc_isp");
	MIPI_I2C_DBG("sp_mipi_i2c_info->clkc_isp : 0x%px\n",sp_mipi_i2c_info->clkc_isp);
	if (IS_ERR(sp_mipi_i2c_info->clkc_isp)) {
		ret = PTR_ERR(sp_mipi_i2c_info->clkc_isp);
		dev_err(dev, "Failed to retrieve clock resource \'clkc_isp\': %d\n", ret);
		goto err_get_clkc_isp;
	}
	ret = clk_prepare_enable(sp_mipi_i2c_info->clkc_isp);
	MIPI_I2C_DBG("clkc_isp ret : 0x%x \n",ret);
	if (ret) {
		dev_err(dev, "Failed to enable \'clkc_isp\' clock: %d\n", ret);
		goto err_en_clkc_isp;
	}

	// Get ISP APB clock resource and enable it 
	sp_mipi_i2c_info->clkc_ispapb = devm_clk_get(dev, "clkc_ispapb");
	MIPI_I2C_DBG("sp_mipi_i2c_info->clkc_ispapb : 0x%px\n",sp_mipi_i2c_info->clkc_ispapb);
	if (IS_ERR(sp_mipi_i2c_info->clkc_ispapb)) {
		ret = PTR_ERR(sp_mipi_i2c_info->clkc_ispapb);
		dev_err(dev, "Failed to retrieve clock resource \'clkc_ispapb\': %d\n", ret);
		goto err_get_clkc_ispapb;
	}
	ret = clk_prepare_enable(sp_mipi_i2c_info->clkc_ispapb);
	MIPI_I2C_DBG("clkc_ispapb ret : 0x%x \n",ret);
	if (ret) {
		dev_err(dev, "Failed to enable \'clkc_ispapb\' clock: %d\n", ret);
		goto err_en_clkc_ispapb;
	}

	// Get ISP reset controller resource and disable it 
	sp_mipi_i2c_info->rstc_isp = devm_reset_control_get(dev, "rstc_isp");
	MIPI_I2C_DBG("sp_mipi_i2c_info->rstc_isp : 0x%px\n", sp_mipi_i2c_info->rstc_isp);
	if (IS_ERR(sp_mipi_i2c_info->rstc_isp)) {
		ret = PTR_ERR(sp_mipi_i2c_info->rstc_isp);
		dev_err(dev, "Failed to retrieve reset controller 'rstc_isp\': %d\n", ret);
		goto err_get_rstc_isp;
	}
	ret = reset_control_deassert(sp_mipi_i2c_info->rstc_isp);
	MIPI_I2C_DBG("rstc_isp ret : 0x%x \n",ret);
	if (ret) {
		dev_err(dev, "Failed to deassert 'rstc_isp' reset controller: %d\n", ret);
		goto err_deassert_rstc_isp;
	}

	// Get ISP APB reset controller resource and disable it 
	sp_mipi_i2c_info->rstc_ispapb = devm_reset_control_get(dev, "rstc_ispapb");
	MIPI_I2C_DBG("sp_mipi_i2c_info->rstc_ispapb : 0x%px\n", sp_mipi_i2c_info->rstc_ispapb);
	if (IS_ERR(sp_mipi_i2c_info->rstc_ispapb)) {
		ret = PTR_ERR(sp_mipi_i2c_info->rstc_ispapb);
		dev_err(dev, "Failed to retrieve reset controller 'rstc_ispapb\': %d\n", ret);
		goto err_get_rstc_ispapb;
	}
	ret = reset_control_deassert(sp_mipi_i2c_info->rstc_ispapb);
	MIPI_I2C_DBG("rstc_ispapb ret : 0x%x\n",ret);
	if (ret) {
		dev_err(dev, "Failed to deassert 'rstc_ispapb' reset controller: %d\n", ret);
		goto err_deassert_rstc_ispapb;
	}

	// Configure MO5_CFG0 register to set ISP APB interval mode to 4T
	temp_value = readl(&(sp_mipi_i2c_info->moon5_regs->mo5_cfg_0));
	MIPI_I2C_INFO("mo5_cfg_0: 0x%08x\n",temp_value);

	if (device_id == 4) {
		temp_value = (MO_ISPABP0_MODE_MASK<<16)|MO_ISPABP0_MODE_4T;
	} else {
		temp_value = (MO_ISPABP1_MODE_MASK<<16)|MO_ISPABP1_MODE_4T;
	}
	MIPI_I2C_DBG("mo5_cfg_0 new setting: 0x%08x\n",temp_value);
	writel(temp_value, &(sp_mipi_i2c_info->moon5_regs->mo5_cfg_0));

	temp_value = readl(&(sp_mipi_i2c_info->moon5_regs->mo5_cfg_0));
	MIPI_I2C_INFO("mo5_cfg_0: 0x%08x\n",temp_value);

	ret = sp_mipi_i2c_init(device_id, sp_mipi_i2c_info);
	if (ret != 0) {
		MIPI_I2C_ERR("mipi i2c %d init error\n", device_id);
		return ret;
	}

	// Set I2C adapter info and register it into kernel
	p_adap = &sp_mipi_i2c_info->adap;
	sprintf(p_adap->name, "%s%d", DEVICE_NAME, device_id);
	p_adap->algo = &sp_mipi_i2c_algorithm;
	p_adap->algo_data = sp_mipi_i2c_info;
	p_adap->nr = device_id;
	p_adap->class = 0;
	p_adap->retries = 5;
	p_adap->dev.parent = &pdev->dev;
	p_adap->dev.of_node = pdev->dev.of_node; 

	ret = i2c_add_numbered_adapter(p_adap);
	if (ret < 0) {
		MIPI_I2C_ERR("Adding I2C adapter %s fails\n", p_adap->name);
		goto err_add_adap;
	} else {
		MIPI_I2C_INFO("Adding I2C adapter %s is success\n", p_adap->name);
		platform_set_drvdata(pdev, sp_mipi_i2c_info);
	}

#ifdef CONFIG_PM_RUNTIME_MIPI_I2C
  pm_runtime_set_autosuspend_delay(&pdev->dev,5000);
  pm_runtime_use_autosuspend(&pdev->dev);
  pm_runtime_set_active(&pdev->dev);
  pm_runtime_enable(&pdev->dev);
#endif

	MIPI_I2C_INFO("SP MIPI I2C driver is installed!\n");
	return ret;

err_add_adap:
	reset_control_assert(sp_mipi_i2c_info->rstc_ispapb);

err_deassert_rstc_ispapb:
err_get_rstc_ispapb:
	reset_control_assert(sp_mipi_i2c_info->rstc_isp);

err_deassert_rstc_isp:
err_get_rstc_isp:
	clk_disable_unprepare(sp_mipi_i2c_info->clkc_ispapb);


err_en_clkc_ispapb:
err_get_clkc_ispapb:
	clk_disable_unprepare(sp_mipi_i2c_info->clkc_isp);


err_en_clkc_isp:
err_get_clkc_isp:
err_alloc:
	kfree(sp_mipi_i2c_info);
	return ret;
}

static int sp_mipi_i2c_remove(struct platform_device *pdev)
{
	sp_mipi_i2c_info_t *sp_mipi_i2c_info = platform_get_drvdata(pdev);
	struct i2c_adapter *p_adap = &sp_mipi_i2c_info->adap;

	MIPI_I2C_INFO("SP MIPI I2C driver is removing\n");
	
#ifdef CONFIG_PM_RUNTIME_MIPI_I2C
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
#endif

	// Delete I2C adapter
	i2c_del_adapter(p_adap);

	// Disable MIPI ISP clock and enable MIPI ISP reset controller
	if ((p_adap->nr >= MIPI_I2C_1ST_CH) && (p_adap->nr < MIPI_I2C_MAX_TH)) {
		hal_mipi_i2c_power_down(&sp_mipi_i2c_info->mipi_i2c_cmd.dI2CInfo);
		clk_disable_unprepare(sp_mipi_i2c_info->clkc_isp);
		clk_disable_unprepare(sp_mipi_i2c_info->clkc_ispapb);
		reset_control_assert(sp_mipi_i2c_info->rstc_isp);
		reset_control_assert(sp_mipi_i2c_info->rstc_ispapb);
	}

	MIPI_I2C_INFO("SP MIPI I2C driver is removed!\n");
	return 0;
}

static int sp_mipi_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	sp_mipi_i2c_info_t *sp_mipi_i2c_info = platform_get_drvdata(pdev);
	struct i2c_adapter *p_adap = &sp_mipi_i2c_info->adap;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	// Enable MIPI ISP reset controller
	if ((p_adap->nr >= MIPI_I2C_1ST_CH) && (p_adap->nr < MIPI_I2C_MAX_TH)) {
		hal_mipi_i2c_power_down(&sp_mipi_i2c_info->mipi_i2c_cmd.dI2CInfo);
		reset_control_assert(sp_mipi_i2c_info->rstc_isp);
		reset_control_assert(sp_mipi_i2c_info->rstc_ispapb);
	}

	return 0;
}

static int sp_mipi_i2c_resume(struct platform_device *pdev)
{
	sp_mipi_i2c_info_t *sp_mipi_i2c_info = platform_get_drvdata(pdev);
	struct i2c_adapter *p_adap = &sp_mipi_i2c_info->adap;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	// Enable MIPI ISP clock and disable MIPI ISP reset controller
	if ((p_adap->nr >= MIPI_I2C_1ST_CH) && (p_adap->nr < MIPI_I2C_MAX_TH)) {
		reset_control_deassert(sp_mipi_i2c_info->rstc_isp);     // release reset
		reset_control_deassert(sp_mipi_i2c_info->rstc_ispapb);  // release reset
		clk_prepare_enable(sp_mipi_i2c_info->clkc_isp);         // enable clken and disable gclken
		clk_prepare_enable(sp_mipi_i2c_info->clkc_ispapb);      // enable clken and disable gclken
		hal_mipi_i2c_power_on(&sp_mipi_i2c_info->mipi_i2c_cmd.dI2CInfo);
	}

	return 0;
}

static const struct of_device_id sp_mipi_i2c_of_match[] = {
	{ .compatible = "sunplus,i143-mipi-i2c" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_mipi_i2c_of_match);

#ifdef CONFIG_PM_RUNTIME_MIPI_I2C
static int sp_mipi_i2c_runtime_suspend(struct device *dev)
{
	sp_mipi_i2c_info_t *pstSpI2CInfo = dev_get_drvdata(dev);
	struct i2c_adapter *p_adap = &pstSpI2CInfo->adap;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	if ((p_adap->nr >= MIPI_I2C_1ST_CH) && (p_adap->nr < MIPI_I2C_MAX_TH)) {
	  reset_control_assert(pstSpI2CInfo->rstc_isp);
	  reset_control_assert(pstSpI2CInfo->rstc_ispapb);
	}

	return 0;
}

static int sp_mipi_i2c_runtime_resume(struct device *dev)
{
	sp_mipi_i2c_info_t *pstSpI2CInfo = dev_get_drvdata(dev);
	struct i2c_adapter *p_adap = &pstSpI2CInfo->adap;

	MIPI_I2C_DBG("%s, %d\n", __FUNCTION__, __LINE__);

	if ((p_adap->nr >= MIPI_I2C_1ST_CH) && (p_adap->nr < MIPI_I2C_MAX_TH)) {
	  reset_control_deassert(pstSpI2CInfo->rstc_isp);       // release reset
	  reset_control_deassert(pstSpI2CInfo->rstc_ispapb);    // release reset
	  clk_prepare_enable(pstSpI2CInfo->clkc_isp);           // enable clken and disable gclken
	  clk_prepare_enable(pstSpI2CInfo->clkc_ispapb);        // enable clken and disable gclke

	}

	return 0;
}
static const struct dev_pm_ops i143_mipi_i2c_pm_ops = {
	.runtime_suspend = sp_mipi_i2c_runtime_suspend,
	.runtime_resume  = sp_mipi_i2c_runtime_resume,
};

#define sp_mipi_i2c_pm_ops  (&i143_mipi_i2c_pm_ops)
#endif

static struct platform_driver sp_mipi_i2c_driver = {
	.probe		= sp_mipi_i2c_probe,
	.remove		= sp_mipi_i2c_remove,
	.suspend	= sp_mipi_i2c_suspend,
	.resume		= sp_mipi_i2c_resume,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= DEVICE_NAME,
		.of_match_table = sp_mipi_i2c_of_match,
#ifdef CONFIG_PM_RUNTIME_MIPI_I2C
		.pm     = sp_mipi_i2c_pm_ops,
#endif
	},
};

static int __init sp_mipi_i2c_adap_init(void)
{
	return platform_driver_register(&sp_mipi_i2c_driver);
}
module_init(sp_mipi_i2c_adap_init);

static void __exit sp_mipi_i2c_adap_exit(void)
{
	platform_driver_unregister(&sp_mipi_i2c_driver);
}
module_exit(sp_mipi_i2c_adap_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus MIPI I2C Master Driver");
