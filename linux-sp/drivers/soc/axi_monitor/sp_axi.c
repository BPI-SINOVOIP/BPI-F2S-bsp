#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/firmware.h>
#include <asm/irq.h>

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include "sp_axi.h"
#include "axi_ioctl.h"

#include <dt-bindings/memory/sp-q628-mem.h> 
#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/reset.h>

/*AXI Monitor Validation*/
#include "axi_monitor.h"
#include "hal_axi_monitor_reg.h"
#include "hal_axi_monitor_sub_reg.h"
#include "regmap_q628.h"
#include "reg_axi.h"

//#define IOP_REG_NAME        "iop"

//#define C_CHIP
#define P_CHIP

#ifdef C_CHIP
/*for AXI monitor*/
#define AXI_MONITOR_REG_NAME      "axi_mon"
#define AXI_IP_00_REG_NAME		  "axi_0"
#define AXI_IP_01_REG_NAME		  "axi_1" 
#define AXI_IP_02_REG_NAME		  "axi_2"
#define AXI_IP_03_REG_NAME		  "axi_3" 	
#define AXI_IP_04_REG_NAME        "axi_4"
#define AXI_IP_05_REG_NAME        "axi_5"
#define AXI_IP_06_REG_NAME	      "axi_6"
#define AXI_IP_07_REG_NAME		  "axi_7"
#define AXI_IP_08_REG_NAME		  "axi_8" 	
#define AXI_IP_09_REG_NAME        "axi_9"
#define AXI_IP_10_REG_NAME        "axi_10"
#define AXI_CBDMA_REG_NAME	      "axi_cbdma"
#define DEVICE_NAME			"sunplus,sp7021-axi"

/*Device ID*/
#define CA7_M0		0
#define CSDBG_M1	1
#define CSETR_M2    2
#define DMA0_MA		3
#define DSPC_MA     4
#define DSPD_MA		5
#define IO_CTL_MA	6
#define UART2AXI_MA	7
#define MEM_SL 		8
#define IO_CTL_SL	9
#define RB_SL		10
#define valid_id 	1
#define invalid_id 	0

typedef struct {
	struct miscdevice dev;			// iop device
	struct mutex write_lock;
	//void __iomem *iop_regs;	
	//void __iomem *moon0_regs;
	//void __iomem *qctl_regs;
	//void __iomem *pmc_regs;
	//void __iomem *rtc_regs;
	/*for AXI monitor*/
	void __iomem *axi_mon_regs;
	void __iomem *axi_id0_regs;
	void __iomem *axi_id1_regs;
	void __iomem *axi_id2_regs;
	void __iomem *axi_id3_regs;
	void __iomem *axi_id4_regs;
	void __iomem *axi_id5_regs;
	void __iomem *axi_id6_regs;
	void __iomem *axi_id7_regs;
	void __iomem *axi_id8_regs;
	void __iomem *axi_id9_regs;
	void __iomem *axi_id10_regs;
	void __iomem *axi_cbdma_regs;
	void __iomem *current_id_regs;	
	int irq;
} sp_axi_t;
static sp_axi_t *axi_monitor;
#endif

#ifdef P_CHIP
/*for AXI monitor*/
#define AXI_MONITOR_REG_NAME      "axi_mon"
#define AXI_IP_04_REG_NAME		  "axi_4"
#define AXI_IP_05_REG_NAME		  "axi_5" 
#define AXI_IP_12_REG_NAME		  "axi_12"
#define AXI_IP_21_REG_NAME		  "axi_21" 	
#define AXI_IP_22_REG_NAME        "axi_22"
#define AXI_IP_27_REG_NAME        "axi_27"
#define AXI_IP_28_REG_NAME	      "axi_28"
#define AXI_IP_31_REG_NAME		  "axi_31"
#define AXI_IP_32_REG_NAME		  "axi_32" 	
#define AXI_IP_33_REG_NAME        "axi_33"
#define AXI_IP_34_REG_NAME        "axi_34"
#define AXI_IP_41_REG_NAME	      "axi_41"
#define AXI_IP_42_REG_NAME		  "axi_42"
#define AXI_IP_43_REG_NAME		  "axi_43" 	
#define AXI_IP_45_REG_NAME        "axi_45"
#define AXI_IP_46_REG_NAME        "axi_46"
#define AXI_IP_47_REG_NAME	      "axi_47"
#define AXI_IP_49_REG_NAME	      "axi_49"
#define AXI_CBDMA_REG_NAME	      "axi_cbdma"
#define DEVICE_NAME			"sunplus,sp7021-axi"

/*Device ID*/
#define CBDMA0_MB	4
#define CBDMA1_MB	5
#define UART2AXI_MA 12
#define NBS_MA		21
#define SPI_NOR_MA  22
#define BIO0_MA		27
#define BIO1_MA		28
#define I2C2CBUS_CB	31
#define CARD0_MA	32
#define CARD1_MA	33
#define CARD4_MA	34
#define BR_CB		41
#define SPI_ND_SL	42
#define SPI_NOR_SL	43
#define CBDMA0_CB	45
#define CBDMA1_CB	46
#define BIO_SL      47
#define SD0_SL		49
#define valid_id 	1
#define invalid_id 	0

typedef struct {
	struct miscdevice dev;			// iop device
	struct mutex write_lock;
	//void __iomem *iop_regs;	
	//void __iomem *moon0_regs;
	//void __iomem *qctl_regs;
	//void __iomem *pmc_regs;
	//void __iomem *rtc_regs;
	/*for AXI monitor*/
	void __iomem *axi_mon_regs;
	void __iomem *axi_id4_regs;
	void __iomem *axi_id5_regs;
	void __iomem *axi_id12_regs;
	void __iomem *axi_id21_regs;
	void __iomem *axi_id22_regs;
	void __iomem *axi_id27_regs;
	void __iomem *axi_id28_regs;
	void __iomem *axi_id31_regs;
	void __iomem *axi_id32_regs;
	void __iomem *axi_id33_regs;
	void __iomem *axi_id34_regs;
	void __iomem *axi_id41_regs;
	void __iomem *axi_id42_regs;
	void __iomem *axi_id43_regs;
	void __iomem *axi_id45_regs;
	void __iomem *axi_id46_regs;
	void __iomem *axi_id47_regs;
	void __iomem *axi_id49_regs;	
	void __iomem *axi_cbdma_regs;
	void __iomem *current_id_regs;	
	int irq;
} sp_axi_t;
static sp_axi_t *axi_monitor;
#endif



unsigned char AxiDeviceID;
struct sunplus_axi {
	struct clk *axiclk;
	struct reset_control *rstc;
};

struct sunplus_axi sp_axi;


#define CBDMA0_SRAM_ADDRESS (0x9E800000) // 40KB
#define CBDMA1_SRAM_ADDRESS (0x9E820000) // 4KB
#define CBDMA_TEST_SOURCE      ((void *) 0x9EA00000)
#define CBDMA_TEST_DESTINATION ((void *) 0x9EA01000)
//#define CBDMA_TEST_SOURCE      ((void *) 0x00000000)
//#define CBDMA_TEST_DESTINATION ((void *) 0x00020000)
//#define CBDMA_TEST_SIZE        (128 << 10)
//#define CBDMA_TEST_SIZE        (8 << 20)
#define CBDMA_TEST_SIZE       0x1000
void cbdma_memcpy(void __iomem *axi_cbdma_regs, int id, void *dst, void *src, unsigned length)
{	
	printk("[CBDMA:%d]: Copy %d KB from 0x%08x to 0x%08x\n", id, length>>10, (unsigned) src, (unsigned)dst);	
	regs_axi_cbdma_t *axi_cbdma = (regs_axi_cbdma_t *)axi_cbdma_regs;	
	//volatile struct cbdma_regs *cbdma;	
	//if (id)	
	//	cbdma = CBDMA1_REG;	
	//else	
	//	cbdma = CBDMA0_REG;	
	// clear all int status		
	//cbdma->int_status = 0x7f;	
	writel(0x7f,&axi_cbdma->int_flag);	
	// set copy mode		
	//cbdma->config = 0x00030003;	
	writel(0x00030003,&axi_cbdma->config);	
	// set write data length	
	//cbdma->dma_length = length;	
	writel(length,&axi_cbdma->length);	
	// set write start address	
	//cbdma->src_adr = (unsigned) src;	
	writel( (unsigned) src,&axi_cbdma->src_adr);	
	// set write end address	
	//cbdma->des_adr = (unsigned) dst;	
	writel( (unsigned) dst,&axi_cbdma->des_adr);
}

void cbdma_kick_go(void __iomem *axi_cbdma_regs, int id)
{	
	printk("[CBDMA:%d]: Start\n", id);	
	regs_axi_cbdma_t *axi_cbdma = (regs_axi_cbdma_t *)axi_cbdma_regs;	
	//volatile struct cbdma_regs *cbdma;	
	//if (id)	
	//	cbdma = CBDMA1_REG;	
	//else	
	//	cbdma = CBDMA0_REG;	
	//cbdma->config |= 0x00000100;	
	writel(0x00030103,&axi_cbdma->config);
}

void cbdma_test(void __iomem *axi_cbdma_regs)
{	
	regs_axi_cbdma_t *axi_cbdma = (regs_axi_cbdma_t *)axi_cbdma_regs;	
	//create_sequential_pattern(CBDMA_TEST_SOURCE, CBDMA_TEST_SIZE);	
	//dcache_disable();	
	// cbdma 0 test	
	//read data from main memory then write to other space of main memory	
	cbdma_memcpy(axi_monitor->axi_cbdma_regs, 0, CBDMA_TEST_DESTINATION, CBDMA_TEST_SOURCE, CBDMA_TEST_SIZE);
	// polling
	#if 1
	//printk("[DBG] cbdma_get_interrupt_status(0) : 0x%08x\n", cbdma_get_interrupt_status(0));	
	//cbdma_enable_interrupt(iop->axi_cbdma_regs, 0, 0);	
	cbdma_kick_go(axi_monitor->axi_cbdma_regs, 0);	
	readl(&axi_cbdma->int_flag);		
	printk("[DBG] cbdma_get_interrupt_status(0) : 0x%08x\n", readl(&axi_cbdma->int_flag));	
	printk("[DBG] cbdma_get_interrupt_status(0) : 0x%08x\n", readl(&axi_cbdma->int_flag));	
	printk("[DBG] cbdma_get_interrupt_status(0) : 0x%08x\n", readl(&axi_cbdma->int_flag));	
	printk("[DBG] cbdma_get_interrupt_status(0) : 0x%08x\n", readl(&axi_cbdma->int_flag));	
	while(readl(&axi_cbdma->int_flag) & 0x1 == 0)	
	{		
		printk(".");	
	}		
	printk("\n");	
	printk("[DBG] cbdma_get_interrupt_status(0) : 0x%08x\n", readl(&axi_cbdma->int_flag));	
	//cbdma_clear_interrupt_status(0);
	#else	
	g_cbmda_finished = 0;	
	cbdma_interrupt_control_mask(0, 1);
	cbdma_kick_go(0);    
	printf("g_cbmda_transfer\n");	
	while(g_cbmda_finished == 0);	
	printf("g_cbmda_finished\n");
	#endif	
	//check_sequential_pattern(CBDMA_TEST_DESTINATION, CBDMA_TEST_SIZE);	
	//dcache_enable();	
	printk("CBDMA test finished.\n");
}

void Get_Monitor_Event(void __iomem *axi_id_regs)
{
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;
	
	printk("current_id_regs=%p\n",axi_id_regs);	
	printk("axi_id ip monitor: 0x%X\n", readl(&axi_id->sub_ip_monitor));
	printk("axi_id event infomation: 0x%X\n", readl(&axi_id->sub_event));	
}

void Event_Monitor_Clear(void __iomem *axi_mon_regs, void __iomem *axi_id_regs)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;	
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;
	writel(0x0001,&axi->axi_control);	
	writel(0x00000000,&axi_id->sub_ip_monitor);
}


void Get_current_id(unsigned char device_id)
{
#if 1
#ifdef C_CHIP
	switch (device_id) {
		case CA7_M0:
			printk("CA7_M0\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id0_regs;
			break;
		case CSDBG_M1:			
			printk("CSDBG_M1\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id1_regs;
			break;
		case CSETR_M2:
			printk("CSETR_M2\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id2_regs;
			break;
		case DMA0_MA:
			printk("DMA0_MA\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id3_regs;
			break;
		case DSPC_MA:			
			printk("DSPC_MA\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id4_regs;
			break;
		case DSPD_MA:			
			printk("DSPD_MA\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id5_regs;
			break;
		case IO_CTL_MA:
			printk("IO_CTL_MA\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id6_regs;;
			break;
		case UART2AXI_MA:			
			printk("UART2AXI_MA\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id7_regs;
			break;
		case MEM_SL:			
			printk("MEM_SL\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id8_regs;
			break;
		case IO_CTL_SL:			
			printk("IO_CTL_SL\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id9_regs;
			break;
		case RB_SL:			
			printk("RB_SL\n"); 
			axi_monitor->current_id_regs = axi_monitor->axi_id10_regs;
			break;
	}
#endif 

#ifdef P_CHIP
		switch (device_id) {
			case CBDMA0_MB:
				printk("CBDMA0_MB\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id4_regs;
				break;
			case CBDMA1_MB: 		
				printk("CBDMA1_MB\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id5_regs;
				break;
			case UART2AXI_MA:
				printk("UART2AXI_MA\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id12_regs;
				break;
			case NBS_MA:
				printk("NBS_MA\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id21_regs;
				break;
			case SPI_NOR_MA:			
				printk("SPI_NOR_MA\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id22_regs;
				break;
			case BIO0_MA:			
				printk("BIO0_MA\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id27_regs;
				break;
			case BIO1_MA:
				printk("BIO1_MA\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id28_regs;;
				break;
			case I2C2CBUS_CB:			
				printk("I2C2CBUS_CB\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id31_regs;
				break;
			case CARD0_MA:			
				printk("CARD0_MA\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id32_regs;
				break;
			case CARD1_MA:			
				printk("CARD1_MA\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id33_regs;
				break;
			case CARD4_MA:			
				printk("CARD4_MA\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id34_regs;
				break;
			case BR_CB:
				printk("BR_CB\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id41_regs;
				break;
			case SPI_ND_SL:
				printk("SPI_ND_SL\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id42_regs;
				break;
			case SPI_NOR_SL:
				printk("SPI_NOR_SL\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id43_regs;
				break;		
			case CBDMA0_CB:
				printk("CBDMA0_CB\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id45_regs;
				break;
			case CBDMA1_CB:
				printk("CBDMA1_CB\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id46_regs;
				break;
			case BIO_SL:
				printk("BIO_SL\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id47_regs;
				break;
			case SD0_SL:
				printk("SD0_SL\n"); 
				axi_monitor->current_id_regs = axi_monitor->axi_id49_regs;
				break;
		}
#endif 

	#endif 
}

static int Check_current_id(unsigned char device_id)
{
#ifdef C_CHIP
	switch (device_id) {
		case CA7_M0:			
		case CSDBG_M1:						
		case CSETR_M2:
		case DMA0_MA:			
		case DSPC_MA:				
		case DSPD_MA:					
		case IO_CTL_MA:			
		case UART2AXI_MA:			
		case MEM_SL:				
		case IO_CTL_SL:				
		case RB_SL:				
			printk("valid_id");
			return valid_id;
		default:			
			printk("invalid_id");
			return invalid_id;
	}	
#endif 

#ifdef P_CHIP
	switch (device_id) {
		case CBDMA0_MB:			
		case CBDMA1_MB:						
		case UART2AXI_MA:
		case NBS_MA:			
		case SPI_NOR_MA:				
		case BIO0_MA:					
		case BIO1_MA:			
		case I2C2CBUS_CB:			
		case CARD0_MA:				
		case CARD1_MA:				
		case CARD4_MA:				
		case BR_CB:		
		case SPI_ND_SL:		
		case SPI_NOR_SL:			
		case CBDMA0_CB:			
		case CBDMA1_CB:			
		case BIO_SL:			
		case SD0_SL:
			printk("valid_id");
			return valid_id;
		default:			
			printk("invalid_id");
			return invalid_id;
	}	
#endif

}


static irqreturn_t axi_irq_handler(int irq, void *data)
{
	printk("axi_monitor_irq");
	printk("AXI device_id=%d \n",AxiDeviceID);	 
	Get_current_id(AxiDeviceID); 
	Get_Monitor_Event(axi_monitor->current_id_regs);
	Event_Monitor_Clear(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs);
	return IRQ_HANDLED;
}



void axi_mon_special_data(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;
	printk("axi=0x%p \n",axi); 
	printk("axi_id=0x%p \n",axi_id); 

	writel(data,&axi->axi_special_data);

	//bit8:latency_mon_start = 1;bit4=bw_mon_start = 1; bit0=event_clear = 1	
	writel(0x00000111,&axi->axi_control);
	
	//bit20: timeout=0, bit16:unexpect_r_access=0, bit12:unexpect_w_access=0, 
	//bit8: special_r_data=1, bit4: special_w_data=1, bit0: monitor enable=1.
	writel(0x00000111,&axi_id->sub_ip_monitor);
}

void axi_mon_unexcept_access_sAddr(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;
	printk("axi=0x%p \n",axi); 
	printk("axi_id=0x%p \n",axi_id); 

	writel((data>>16),&axi->axi_valid_start_add);
	printk("unexpect_access_sAddr=0x%x \n",(data>>16)); 
	writel((data&0xFFFF),&axi->axi_valid_end_add);
	printk("unexpect_access_eAddr=0x%x \n",(data&0xFFFF)); 
	//bit8:latency_mon_start = 1;bit4=bw_mon_start = 1; bit0=event_clear = 1	
	writel(0x00000111,&axi->axi_control);
	//unexpect access
	//bit20: timeout=0, bit16:unexpect_r_access=1, bit12:unexpect_w_access=1, 
	//bit8: special_r_data=0, bit4: special_w_data=0, bit0: monitor enable=1.
	//writel(0x00011001,&axi_id->sub_ip_monitor);	
	writel(0x00011001,&axi_id->sub_ip_monitor);	
}

void axi_mon_unexcept_access_eAddr(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;
	
	printk("axi_mon_regs=%p\n",axi_mon_regs);	
	writel(data,&axi->axi_valid_end_add);
	//bit8:latency_mon_start = 1;bit4=bw_mon_start = 1; bit0=event_clear = 1	
	writel(0x00000111,&axi->axi_control);
	//unexpect access
	//bit20: timeout=0, bit16:unexpect_r_access=1, bit12:unexpect_w_access=1, 
	//bit8: special_r_data=0, bit4: special_w_data=0, bit0: monitor enable=1.
	writel(0x00011001,&axi_id->sub_ip_monitor);	
}

void axi_mon_timeout(void __iomem *axi_mon_regs, void __iomem *axi_id_regs, unsigned int data)
{
	regs_axi_t *axi = (regs_axi_t *)axi_mon_regs;
	regs_submonitor_t *axi_id = (regs_submonitor_t *)axi_id_regs;
	printk("axi_mon_regs=0x%p \n",axi); 
	printk("axi_id=0x%p \n",axi_id); 

	// about 4.95ns, configure Timeout cycle	
	writel(data,&axi->axi_time_out);
		
	// about 83ms
	//BW update period = 0x4, BW Monitor Start=1
	writel(0x00004010,&axi->axi_control);

	//timeout
	//bit20: timeout=1, bit16:unexpect_r_access=0, bit12:unexpect_w_access=0, 
	//bit8: special_r_data=0, bit4: special_w_data=0, bit0: monitor enable=1.
	writel(0x00100001,&axi_id->sub_ip_monitor);	
}


static ssize_t axi_show_device_id(struct device *dev, struct device_attribute *attr, char *buf)
{   
	ssize_t len = 0;
    printk("axi_show_device_id\n");
	return len;
}

static ssize_t axi_store_device_id(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	printk("axi_store_device_id\n");	
	AxiDeviceID = simple_strtol(buf, NULL, 10);	//Get device_id     
	if(Check_current_id(AxiDeviceID) == valid_id)
		printk("AXI device_id=%d \n",AxiDeviceID); 	 
	else		
		printk("INVALID DEVICE ID\n"); 
	return ret;
}


static ssize_t axi_show_special_data(struct device *dev, struct device_attribute *attr, char *buf)
{   
	ssize_t len = 0;
    printk("axi_show_special_data\n");
	return len;
}

static ssize_t axi_store_special_data(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;    
	unsigned int special_data; 
	printk("axi_store_special_data\n");	
	special_data = simple_strtol(buf, NULL, 0);	//Get special_data  
	printk("AXI device_id=%d \n",AxiDeviceID); 	 
	printk("special_data=0x%x \n",special_data); 
	if(Check_current_id(AxiDeviceID) == valid_id)
	{
	 	Get_current_id(AxiDeviceID);	
		axi_mon_special_data(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs,special_data); 
	}
	else		
		printk("INVALID DEVICE ID\n"); 
	
	return ret;
}

static ssize_t axi_show_unexpect_access(struct device *dev, struct device_attribute *attr, char *buf)
{   
	ssize_t len = 0;
    printk("axi_show_unexpect_access\n");
	return len;
}

static ssize_t axi_store_unexpect_access(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;  	
	unsigned int unexpect_access; 
	printk("axi_store_unexpect_access\n");	
	unexpect_access = simple_strtol(buf, NULL, 0);	//Get unexpect_access
	printk("AXI device_id=%d \n",AxiDeviceID); 	
	if(Check_current_id(AxiDeviceID) == valid_id)
	{
	 	Get_current_id(AxiDeviceID);	
		axi_mon_unexcept_access_sAddr(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs,unexpect_access);
	}
	else		
		printk("INVALID DEVICE ID\n"); 
    
	return ret;
}

#if 0
static ssize_t axi_show_unexpect_access_eAddr(struct device *dev, struct device_attribute *attr, char *buf)
{   
	ssize_t len = 0;
    printk("axi_show_unexpect_access__eAddr\n");
	return len;
}

static ssize_t axi_store_unexpect_access_eAddr(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;  	
	unsigned int unexpect_access; 
	printk("axi_store_unexpect_access_eAddr\n");	
	unexpect_access = simple_strtol(buf, NULL, 0);	//Get unexpect_access
	printk("AXI device_id=%d \n",AxiDeviceID); 	 
	printk("unexpect_access_eAddr=0x%x \n",unexpect_access); 	 
	axi_monitor->current_id_regs = Get_current_id(AxiDeviceID);	
	axi_mon_unexcept_access_eAddr(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs,unexpect_access);
	return ret;
}
#endif 

static ssize_t axi_show_time_out(struct device *dev, struct device_attribute *attr, char *buf)
{   
	ssize_t len = 0;
    printk("axi_show_time_out\n");
	return len;
}

static ssize_t axi_store_time_out(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;
	unsigned int Timeout_cycle; 
	printk("axi_store_time_out\n");	
	Timeout_cycle = simple_strtol(buf, NULL, 0);	//Get Timeout cycle    
	printk("AXI device_id=%d \n",AxiDeviceID); 	 
	printk("Timeout_cycle=0x%x \n",Timeout_cycle); 	
	if(Check_current_id(AxiDeviceID) == valid_id)
	{
	 	Get_current_id(AxiDeviceID);	
		axi_mon_timeout(axi_monitor->axi_mon_regs, axi_monitor->current_id_regs,Timeout_cycle);
	}
	else		
		printk("INVALID DEVICE ID\n"); 
	
	return ret;
}

static ssize_t axi_show_cbdma_test(struct device *dev, struct device_attribute *attr, char *buf)
{   
	ssize_t len = 0;
    printk("axi_show_cbdma_test\n");
	return len;
}

static ssize_t axi_store_cbdma_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned char ret = count;	
	cbdma_test((axi_monitor->axi_cbdma_regs));
    printk("axi_store_cbdma_test\n");	
	return ret;
}


static DEVICE_ATTR(device_id, S_IWUSR|S_IRUGO, axi_show_device_id, axi_store_device_id);
static DEVICE_ATTR(special_data, S_IWUSR|S_IRUGO, axi_show_special_data, axi_store_special_data);
static DEVICE_ATTR(unexpect_access, S_IWUSR|S_IRUGO, axi_show_unexpect_access, axi_store_unexpect_access);
//static DEVICE_ATTR(unexpect_access_eAddr, S_IWUSR|S_IRUGO, axi_show_unexpect_access_eAddr, axi_store_unexpect_access_eAddr);
static DEVICE_ATTR(time_out, S_IWUSR|S_IRUGO, axi_show_time_out, axi_store_time_out);
static DEVICE_ATTR(cbdma_test, S_IWUSR|S_IRUGO, axi_show_cbdma_test, axi_store_cbdma_test);

static struct attribute *axi_sysfs_entries[] = {
	&dev_attr_device_id.attr,
	&dev_attr_special_data.attr,
	&dev_attr_unexpect_access.attr,
	//&dev_attr_unexpect_access_eAddr.attr,
	&dev_attr_time_out.attr,
	&dev_attr_cbdma_test.attr,
	NULL,
};

static struct attribute_group axi_attribute_group = {
	.attrs = axi_sysfs_entries,
};


/*AXI Monitor Validation end*/
/* ---------------------------------------------------------------------------------------------- */
//#define AXI_FUNC_DEBUG
//#define AXI_KDBG_INFO
//#define AXI_KDBG_ERR

#ifdef AXI_FUNC_DEBUG
	#define FUNC_DEBUG()    printk(KERN_INFO "[AXI] Debug: %s(%d)\n", __FUNCTION__, __LINE__)
#else
	#define FUNC_DEBUG()
#endif

#ifdef AXI_KDBG_INFO
#define DBG_INFO(fmt, args ...)	printk(KERN_INFO "K_AXI: " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef AXI_KDBG_ERR
#define DBG_ERR(fmt, args ...)	printk(KERN_ERR "K_AXI: " fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**************************************************************************
*                         G L O B A L    D A T A                         *
**************************************************************************/

#if 0
#define CODE_SIZE	4096
unsigned char SourceCode[CODE_SIZE];
#endif 

static int sp_axi_open(struct inode *inode, struct file *pfile)
{
	printk("Sunplus AXI module open\n");
	
	return 0;
}

static int sp_axi_release(struct inode *inode, struct file *pfile)
{
	printk("Sunplus AXI module release\n");
	return 0;
}

static long sp_axi_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
    return ret;
}

static struct file_operations sp_axi_fops = {
	.owner          	= THIS_MODULE,
	.open           = sp_axi_open,
	.release        = sp_axi_release,
	.unlocked_ioctl = sp_axi_ioctl,
};

#if 0
static int _sp_axi_get_irq(struct platform_device *pdev, sp_axi_t *pstSpIOPInfo)
{
	int irq;

	FUNC_DEBUG();

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		DBG_ERR("[AXI] get irq number fail, irq = %d\n", irq);
		return -ENODEV;
	}

	pstSpIOPInfo->irq = irq;
	return IOP_SUCCESS;
}
#endif 

static int _sp_axi_get_register_base(struct platform_device *pdev, unsigned int *membase, const char *res_name)
{
	struct resource *r;
	void __iomem *p;

	FUNC_DEBUG();
	DBG_INFO("[AXI] register name  : %s!!\n", res_name);

	r = platform_get_resource_byname(pdev, IORESOURCE_MEM, res_name);
	if(r == NULL) {
		DBG_INFO("[AXI] platform_get_resource_byname fail\n");
		return -ENODEV;
	}

	p = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(p)) {
		DBG_ERR("[AXI] ioremap fail\n");
		return PTR_ERR(p);
	}

	DBG_INFO("[AXI ioremap addr : 0x%x!!\n", (unsigned int)p);
	*membase = (unsigned int)p;

	return IOP_SUCCESS;
}

static int _sp_axi_get_resources(struct platform_device *pdev, sp_axi_t *pstSpIOPInfo)
{
	int ret;
	unsigned int membase = 0;

	FUNC_DEBUG();
#ifdef C_CHIP
		/*for AXI monitor*/
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_MONITOR_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_mon_regs = (void __iomem *)membase;
		}	
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_00_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id0_regs = (void __iomem *)membase;
		}	
		
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_01_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id1_regs = (void __iomem *)membase;
		}	
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_02_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id2_regs = (void __iomem *)membase;
		}	
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_03_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id3_regs = (void __iomem *)membase;
		}	
		
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_04_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id4_regs = (void __iomem *)membase;
		}	
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_05_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id5_regs = (void __iomem *)membase;
		}	
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_06_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id6_regs = (void __iomem *)membase;
		}	
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_07_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id7_regs = (void __iomem *)membase;
		}	
	
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_08_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id8_regs = (void __iomem *)membase;
		}	
	
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_09_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id9_regs = (void __iomem *)membase;
		}	
	
		
		ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_10_REG_NAME);
		if (ret) {
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		} else {
			pstSpIOPInfo->axi_id10_regs = (void __iomem *)membase;
		}	

		ret = _sp_axi_get_register_base(pdev, &membase, AXI_CBDMA_REG_NAME);	
		if (ret) {		
			DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);		
			return ret;	
		} else {		
			pstSpIOPInfo->axi_cbdma_regs = (void __iomem *)membase;	
		}	
#endif 	
#ifdef P_CHIP
			/*for AXI monitor*/
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_MONITOR_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_mon_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_04_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id4_regs = (void __iomem *)membase;
			}	
			
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_05_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id5_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_12_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id12_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_21_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id21_regs = (void __iomem *)membase;
			}	
			
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_22_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id22_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_27_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id27_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_28_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id28_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_31_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id31_regs = (void __iomem *)membase;
			}	
		
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_32_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id32_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_33_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id33_regs = (void __iomem *)membase;
			}	
		
			
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_34_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id34_regs = (void __iomem *)membase;
			}
		 
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_41_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id41_regs = (void __iomem *)membase;
			}	
		
			
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_42_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id42_regs = (void __iomem *)membase;
			}	
			
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_43_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id43_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_45_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id45_regs = (void __iomem *)membase;
			}	
			
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_46_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id46_regs = (void __iomem *)membase;
			}	
			
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_47_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id47_regs = (void __iomem *)membase;
			}	
		
			ret = _sp_axi_get_register_base(pdev, &membase, AXI_IP_49_REG_NAME);
			if (ret) {
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);
				return ret;
			} else {
				pstSpIOPInfo->axi_id49_regs = (void __iomem *)membase;
			}	

			ret = _sp_axi_get_register_base(pdev, &membase, AXI_CBDMA_REG_NAME);	
			if (ret) {		
				DBG_ERR("[AXI] %s (%d) ret = %d\n", __FUNCTION__, __LINE__, ret);		
				return ret;	
			} else {		
				pstSpIOPInfo->axi_cbdma_regs = (void __iomem *)membase;	
			}	
#endif 	
	return IOP_SUCCESS;
}


static int sp_axi_start(sp_axi_t *iopbase)
{

	FUNC_DEBUG();
	return IOP_SUCCESS;
}

#if 0
static int sp_axi_suspend(sp_axi_t *iopbase)
{
	DBG_ERR("sp_axi_suspend\n");
	//early_printk("[MBOX_%d] %08x (%u)\n", i, d, d);
	early_printk("sp_axi_suspend\n");
	FUNC_DEBUG();
	return IOP_SUCCESS;
}
#endif

static int sp_axi_platform_driver_probe(struct platform_device *pdev)
{
	int ret = -ENXIO;	
	int err, irq;
	AxiDeviceID = 0;
	DBG_ERR("sp_axi_platform_driver_probe\n");
	FUNC_DEBUG();
	axi_monitor = (sp_axi_t *)devm_kzalloc(&pdev->dev, sizeof(sp_axi_t), GFP_KERNEL);
	if (axi_monitor == NULL) {
		printk("sp_iop_t malloc fail\n");
		ret	= -ENOMEM;
		goto fail_kmalloc;
	}
	/* init */
		mutex_init(&axi_monitor->write_lock);
	/* register device */
	axi_monitor->dev.name  = "sp_axi";
	axi_monitor->dev.minor = MISC_DYNAMIC_MINOR;
	axi_monitor->dev.fops  = &sp_axi_fops;
	ret = misc_register(&axi_monitor->dev);
	if (ret != 0) {
		printk("sp_iop device register fail\n");
		goto fail_regdev;
	}

	ret = _sp_axi_get_resources(pdev, axi_monitor);

#if 1
	ret = sp_axi_start(axi_monitor);
	if (ret != 0) {
		DBG_ERR("[IOP] sp iop init err=%d\n", ret);
		return ret;
	}
#endif 

	// request irq
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		printk("platform_get_irq failed\n");
		return -ENODEV;
	}

	err = devm_request_irq(&pdev->dev, irq, axi_irq_handler, IRQF_TRIGGER_HIGH, "axi irq", pdev);
	if (err) {
		printk("devm_request_irq failed: %d\n", err);
		return err;
	}

	err = sysfs_create_group(&pdev->dev.kobj, &axi_attribute_group);	
	if (err) {		
		dev_err(&pdev->dev, "Error creating sysfs files!\n");	
	}

	/* clk*/
   	DBG_INFO("Enable AXI clock\n");
   	sp_axi.axiclk = devm_clk_get(&pdev->dev,NULL);
   	DBG_INFO("sp_axi->clk = %x\n",sp_axi.axiclk);
   	if(IS_ERR(sp_axi.axiclk)) {
	   dev_err(&pdev->dev, "devm_clk_get fail\n");
   	}
   	ret = clk_prepare_enable(sp_axi.axiclk);


   	/* reset*/
   	DBG_INFO("Enable AXI reset function\n");    
   	sp_axi.rstc = devm_reset_control_get(&pdev->dev, NULL);
   	DBG_INFO( "sp_axi->rstc : 0x%x \n",sp_axi.rstc);
   	if (IS_ERR(sp_axi.rstc)) {
	   ret = PTR_ERR(sp_axi.rstc);
	   dev_err(&pdev->dev, "SPI failed to retrieve reset controller: %d\n", ret);
	   goto free_clk;
   	}
   	ret = reset_control_deassert(sp_axi.rstc);
   	if (ret)
	   goto free_clk;

   

fail_regdev:
	mutex_destroy(&axi_monitor->write_lock);
fail_kmalloc:
	return ret;
free_clk:
	clk_disable_unprepare(sp_axi.axiclk);	
	return ret;

}

static int sp_axi_platform_driver_remove(struct platform_device *pdev)
{
	//struct sunplus_axi *axi = platform_get_drvdata(pdev);
	FUNC_DEBUG();
	reset_control_assert(sp_axi.rstc);
	//rtc_device_unregister(axi);
	return 0;
}

static int sp_axi_platform_driver_suspend(struct platform_device *pdev, pm_message_t state)
{    
	FUNC_DEBUG();
	return 0;
}

static int sp_axi_platform_driver_resume(struct platform_device *pdev)
{
	FUNC_DEBUG();
	return 0;
}

static const struct of_device_id sp_axi_of_match[] = {
	{ .compatible = "sunplus,sp7021-axi" },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, sp_axi_of_match);

static struct platform_driver sp_axi_platform_driver = {
	.probe		= sp_axi_platform_driver_probe,
	.remove		= sp_axi_platform_driver_remove,
	.suspend	= sp_axi_platform_driver_suspend,
	.resume		= sp_axi_platform_driver_resume,
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_axi_of_match),
	}
};



module_platform_driver(sp_axi_platform_driver);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus AXI Driver");
MODULE_LICENSE("GPL");


