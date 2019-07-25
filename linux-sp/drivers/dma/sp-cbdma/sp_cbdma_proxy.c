/* DMA Proxy
 *
 * This module is designed to be a small example of a DMA device driver that is
 * a client to the DMA Engine using the AXI DMA driver. It serves as a proxy for
 * kernel space DMA control to a user space application.
 *
 * A zero copy scheme is provided by allowing user space to mmap a kernel allocated
 * memory region into user space, referred to as a proxy channel interface. The
 * ioctl function is provided to start a DMA transfer which then blocks until the
 * transfer is complete. No input arguments are being used in the ioctl function.
 *
 * There is an associated user space application, dma_proxy_test.c, and dma_proxy.h
 * that work with this device driver.
 *
 * The hardware design was tested with an AXI DMA without scatter gather and
 * with the transmit channel looped back to the receive channel.
 *
 */

#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include "sp_cbdma_proxy.h"


//#define SP_CBDMA_BASIC_TEST
#if defined(SP_CBDMA_BASIC_TEST)

//#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
//#include <linux/module.h>
#include <linux/of.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/delay.h>
//#include <linux/dma-mapping.h>
#include <linux/atomic.h>

//#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/of_platform.h>
#include <linux/firmware.h>


#define DEVICE_NAME	"sunplus,sp7021-cbdma-proxy"

#define CB_DMA_REG_NAME	"cb_dma_proxy"

/* Unaligned test */
#define UNALIGNED_DROP_S	0	/* 0, 1, 2, 3 */
#define UNALIGNED_DROP_E	0	/* 0, 1, 2, 3 */
#define UNALIGNED_ADDR_S(X)	(X + UNALIGNED_DROP_S)
#define UNALIGNED_ADDR_E(X)	(X - UNALIGNED_DROP_E)

#define BUF_SIZE_DRAM		(PAGE_SIZE * 2)

#define PATTERN4TEST(X)		((((u32)(X)) << 24) | (((u32)(X)) << 16) | (((u32)(X)) << 8) | (((u32)(X)) << 0))



#define MIN(X, Y)		((X) < (Y) ? (X): (Y))

#define NUM_SG_IDX		32

struct cbdma_proxy_info_s {
	struct platform_device *pdev;
	struct miscdevice dev;
	struct mutex cbdma_lock;
	void __iomem *cbdma_regs;
	u32 sram_addr;
	u32 sram_size;
};
static struct cbdma_proxy_info_s *cbdma_proxy_info;

atomic_t isr_cnt = ATOMIC_INIT(0);

#endif // Use sp cbdma device drivers

/* Debug message definition */
#define CBDMA_PROXY_FUNC_DEBUG
//#define CBDMA_PROXY_KDBG_INFO
//#define CBDMA_PROXY_KDBG_ERR

#ifdef CBDMA_PROXY_FUNC_DEBUG
#define FUNC_DEBUG()            printk(KERN_INFO "[CBDMA_PROXY] Debug: %s(%d)\n", __FUNCTION__, __LINE__)
#else
#define FUNC_DEBUG()
#endif

#ifdef CBDMA_PROXY_KDBG_INFO
#define DBG_INFO(fmt, args ...)	printk(KERN_INFO "K_CBDMA_PROXY: " fmt, ## args)
#else
#define DBG_INFO(fmt, args ...)
#endif

#ifdef CBDMA_PROXY_KDBG_ERR
#define DBG_ERR(fmt, args ...)	printk(KERN_ERR "K_CBDMA_PROXY: " fmt, ## args)
#else
#define DBG_ERR(fmt, args ...)
#endif


#define CACHED_BUFFERS
//#define INTERNAL_TEST

/* The following module parameter controls where the allocated interface memory area is cached or not
 * such that both can be illustrated.  Add cached_buffers=1 to the command line insert of the module
 * to cause the allocated memory to be cached.
 */
static unsigned cached_buffers = 0;
module_param(cached_buffers, int, S_IRUGO);

MODULE_LICENSE("GPL");

#define DRIVER_NAME 		"dma_proxy"
#define CHANNEL_COUNT 		2
#define ERROR 			-1
#define NOT_LAST_CHANNEL 	0
#define LAST_CHANNEL 		1

/* The following data structure represents a single channel of DMA, transmit or receive in the case
 * when using AXI DMA.  It contains all the data to be maintained for the channel.
 */
struct dma_proxy_channel {
	struct dma_proxy_channel_interface *interface_p;	/* user to kernel space interface */
	dma_addr_t interface_phys_addr;

	struct device *proxy_device_p;				/* character device support */
	struct device *dma_device_p;
	dev_t dev_node;
	struct cdev cdev;
	struct class *class_p;

	struct dma_chan *channel_p;				/* dma support */
	struct completion cmp;
	dma_cookie_t cookie;
	dma_addr_t dma_handle;
	u32 direction;						/* DMA_MEM_TO_DEV or DMA_DEV_TO_MEM */
};

/* Allocate the channels for this example statically rather than dynamically for simplicity.
 */
static struct dma_proxy_channel channels[CHANNEL_COUNT];

#if defined(SP_CBDMA_BASIC_TEST)
void dump_data(u8 *ptr, u32 size)
{
	u32 i, addr_begin;
	int length;
	char buffer[256];

	FUNC_DEBUG();

	addr_begin = (u32)(ptr);
	for (i = 0; i < size; i++) {
		if ((i & 0x0F) == 0x00) {
			length = sprintf(buffer, "%08x: ", i + addr_begin);
		}
		length += sprintf(&buffer[length], "%02x ", *ptr);
		ptr++;

		if ((i & 0x0F) == 0x0F) {
			printk(KERN_INFO "%s\n", buffer);
		}
	}
	printk(KERN_INFO "\n");
}
#endif

/* Handle a callback and indicate the DMA transfer is complete to another
 * thread of control
 */
static void sync_callback(void *completion)
{
	FUNC_DEBUG();

	/* Indicate the DMA transaction completed to allow the other
	 * thread of control to finish processing
	 */
	complete(completion);
}

/* Prepare a DMA buffer to be used in a DMA transaction, submit it to the DMA engine
 * to queued and return a cookie that can be used to track that status of the
 * transaction
 */
static dma_cookie_t start_transfer(struct dma_proxy_channel *pchannel_p)
{
	enum dma_ctrl_flags flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
	struct dma_async_tx_descriptor *chan_desc;
	dma_cookie_t cookie;

	struct dma_proxy_channel_interface *interface_p = pchannel_p->interface_p;

	FUNC_DEBUG();

#if 0
	/*** The code from spi-davinci.c : Start ***/
	{
		struct dma_slave_config dma_rx_conf = {
			.direction = DMA_DEV_TO_MEM,
			.dst_addr = pchannel_p->dma_handle,
			.src_addr_width = 4,
			.src_maxburst = 1,
		};
		struct dma_slave_config dma_tx_conf = {
			.direction = DMA_MEM_TO_DEV,
			.src_addr = pchannel_p->dma_handle,
			.dst_addr_width = 4,
			.dst_maxburst = 1,
		};

		DBG_INFO("%s, %d, Configure DMA channel\n", __func__, __LINE__);
		if (pchannel_p->direction == DMA_DEV_TO_MEM)
			dmaengine_slave_config(pchannel_p->channel_p, &dma_rx_conf);
		else if (pchannel_p->direction == DMA_MEM_TO_DEV)
			dmaengine_slave_config(pchannel_p->channel_p, &dma_tx_conf);
		else
			DBG_INFO("%s, %d, errror!!\n", __func__, __LINE__);
	}
	/*** The code from spi-davinci.c : Start ***/
#endif

	/* Create a buffer (channel)  descriptor for the buffer since only a
	 * single buffer is being used for this transfer
	 */
	chan_desc = dmaengine_prep_slave_single(pchannel_p->channel_p, pchannel_p->dma_handle,
											interface_p->length,
											pchannel_p->direction,
											flags);

	/* Make sure the operation was completed successfully
	 */
	if (!chan_desc) {
		printk(KERN_ERR "dmaengine_prep_slave_single error\n");
		cookie = -EBUSY;
	} else {
		chan_desc->callback = sync_callback;
		chan_desc->callback_param = &pchannel_p->cmp;

		/* Initialize the completion for the transfer and before using it
		 * then submit the transaction to the DMA engine so that it's queued
		 * up to be processed later and get a cookie to track it's status
		 */
		init_completion(&pchannel_p->cmp);
		cookie = dmaengine_submit(chan_desc);

		/* Start the DMA transaction which was previously queued up in the DMA engine
		 */
		dma_async_issue_pending(pchannel_p->channel_p);
	}
	return cookie;
}

/* Wait for a DMA transfer that was previously submitted to the DMA engine
 * wait for it complete, timeout or have an error
 */
static void wait_for_transfer(struct dma_proxy_channel *pchannel_p)
{
	unsigned long timeout = msecs_to_jiffies(3000);
	enum dma_status status;

	FUNC_DEBUG();

	pchannel_p->interface_p->status = PROXY_BUSY;

	/* Wait for the transaction to complete, or timeout, or get an error
	 */
	timeout = wait_for_completion_timeout(&pchannel_p->cmp, timeout);
	status = dma_async_is_tx_complete(pchannel_p->channel_p, pchannel_p->cookie, NULL, NULL);

	if (timeout == 0)  {
		pchannel_p->interface_p->status  = PROXY_TIMEOUT;
		printk(KERN_ERR "DMA timed out\n");
	} else if (status != DMA_COMPLETE) {
		pchannel_p->interface_p->status = PROXY_ERROR;
		printk(KERN_ERR "DMA returned completion callback status of: %s\n",
			   status == DMA_ERROR ? "error" : "in progress");
	} else
		pchannel_p->interface_p->status = PROXY_NO_ERROR;

	if (cached_buffers) {

		/* Cached buffers need to be unmapped after the transfer is done so that the CPU
		 * can see the new data when being received. This is done in this function to
		 * allow this function to be called from another thread of control also (non-blocking).
		 */
		u32 map_direction;
		if (pchannel_p->direction == DMA_MEM_TO_DEV)
			map_direction = DMA_TO_DEVICE;
		else
			map_direction = DMA_FROM_DEVICE;

		dma_unmap_single(pchannel_p->dma_device_p, pchannel_p->dma_handle,
						 pchannel_p->interface_p->length,
						 map_direction);
	}
}

/* For debug only, print out the channel details
 */
void print_channel(struct dma_proxy_channel *pchannel_p)
{
	struct dma_proxy_channel_interface *interface_p = pchannel_p->interface_p;

	FUNC_DEBUG();

	printk("length = %d ", interface_p->length);
	if (pchannel_p->direction == DMA_MEM_TO_DEV)
		printk("tx direction ");
	else
		printk("rx direction ");
}

/* Setup the DMA transfer for the channel by taking care of any cache operations
 * and the start it.
 */
static void transfer(struct dma_proxy_channel *pchannel_p)
{
	struct dma_proxy_channel_interface *interface_p = pchannel_p->interface_p;
	u32 map_direction;

	FUNC_DEBUG();

	print_channel(pchannel_p);

	if (cached_buffers) {

		/* Cached buffers need to be handled before starting the transfer so that
		 * any cached data is pushed to memory.
		 */
		if (pchannel_p->direction == DMA_MEM_TO_DEV)
			map_direction = DMA_TO_DEVICE;
		else
			map_direction = DMA_FROM_DEVICE;
		pchannel_p->dma_handle = dma_map_single(pchannel_p->dma_device_p,
												interface_p->buffer,
												interface_p->length,
												map_direction);
	} else {

		/* The physical address of the buffer in the interface is needed for the dma transfer
		 * as the buffer may not be the first data in the interface
		 */
		u32 offset = (u32)&interface_p->buffer - (u32)interface_p;
		DBG_INFO("%s, %d, offset=0x%x, &buffer=0x%x, interface_p=0x%x\n", __func__, __LINE__,
			offset, (u32)&interface_p->buffer, (u32)interface_p);

		pchannel_p->dma_handle = (dma_addr_t)(pchannel_p->interface_phys_addr + offset);
		DBG_INFO("%s, %d, dma_handle=0x%x, &interface_phys_addr=0x%x, offset=0x%x\n", __func__, __LINE__,
			(u32)pchannel_p->dma_handle, (u32)pchannel_p->interface_phys_addr, (u32)offset);
	}

	/* Start the DMA transfer and make sure there were not any errors
	 */
	printk(KERN_INFO "Starting DMA transfers\n");
	pchannel_p->cookie = start_transfer(pchannel_p);
	DBG_INFO("%s, %d, cookie=%d\n", __func__, __LINE__, pchannel_p->cookie);

	if (dma_submit_error(pchannel_p->cookie)) {
		printk(KERN_ERR "xdma_prep_buffer error\n");
		return;
	}

	DBG_INFO("%s, %d,wait_for_transfer(pchannel_p) start\n", __func__, __LINE__);
	wait_for_transfer(pchannel_p);
	DBG_INFO("%s, %d,wait_for_transfer(pchannel_p) end\n", __func__, __LINE__);
}

/* The following functions are designed to test the driver from within the device
 * driver without any user space.
 */
#ifdef INTERNAL_TEST
#define TX_TEST		0
#define RX_TEST		1

static void test(void)
{
	int i;
	u32 *sram_ptr, *u32_src_ptr, *u32_dst_ptr;
	u32 *u32_ptr, val_u32, wrong_number;
	#if 0 // Two segments to check doubly linked list
	const int test_size = ((40 * 1024) - 256) * 2; /* last 256 bytes is reserved for COPY, access it will trigger a ISR */;
	#else
	const int test_size = (40 * 1024) - 256; /* last 256 bytes is reserved for COPY, access it will trigger a ISR */;
	#endif

	FUNC_DEBUG();

	sram_ptr = (u32 *)ioremap(cbdma_proxy_info->sram_addr, cbdma_proxy_info->sram_size);
	BUG_ON(!sram_ptr);
	DBG_INFO("%s, %d, sram_ptr=0x%x, test_size=0x%x\n", __func__, __LINE__, (int)sram_ptr, test_size);

	#if TX_TEST == 1
	DBG_INFO("%s, %d, ***TX (DMA_MEM_TO_DEV) test***\n", __func__, __LINE__);

	/* Initialize the transmit buffer with a pattern and then start
	 * the seperate thread of control to handle the transmit transfer
	 * since the functions block waiting for the transfer to complete.
	 */
	u32_ptr = (u32 *)channels[0].interface_p->buffer;
	val_u32 = (u32)(cbdma_proxy_info->sram_addr);
	printk("u32_ptr=0x%x, sram_ptr=0x%x, val_u32=0x%x, test_size=0x%x\n", (int)u32_ptr, (int)sram_ptr, val_u32, test_size);

	for (i = 0 ; i < (test_size >> 2); i++) {
		*u32_ptr = val_u32;
		u32_ptr++;
		val_u32 += 4;
	}

	DBG_INFO("%s, %d, Init buffer done @ %p\n", __func__, __LINE__, channels[0].interface_p->buffer);
	dump_data(channels[0].interface_p->buffer, 0x20);

	channels[0].interface_p->length = test_size;
	transfer(&channels[0]);


	u32_dst_ptr = sram_ptr;
	u32_src_ptr = (u32 *)(channels[0].interface_p->buffer);
	DBG_INFO("%s, %d, u32_dst_ptr=0x%x, u32_src_ptr=0x%x\n", __func__, __LINE__, (int)u32_dst_ptr, (int)u32_src_ptr);

	dump_data((u8 *)(sram_ptr), 0x20);
	wrong_number = 0;
	for (i = 0 ; (i < test_size >> 2); i++) {
		if (*u32_dst_ptr != *u32_src_ptr)
			wrong_number++;
		u32_dst_ptr++;
		u32_src_ptr++;
	}

	DBG_INFO("%s, %d, test_size:%u, wrong_number:%u in 4bytes\n", __func__, __LINE__,
		test_size>>2, wrong_number);

	if (wrong_number == 0)
		printk(KERN_INFO "Test result: Data in SRAM: OK\n\n");
	else
		printk(KERN_INFO "Test result: Data in SRAM: NG\n\n");
	#endif //#ifdef TX_TEST

	#if RX_TEST == 1
	DBG_INFO("%s, %d, ***RX (DMA_DEV_TO_MEM) test***\n", __func__, __LINE__);

	u32_ptr = sram_ptr;
	val_u32 = (u32)(cbdma_proxy_info->sram_addr);
	printk("u32_ptr=0x%x, sram_ptr=0x%x, val_u32=0x%x, test_size=0x%x\n", (int)u32_ptr, (int)sram_ptr, val_u32, test_size);
	for (i = 0 ; i < (test_size >> 2); i++) {
		*u32_ptr = val_u32;
		u32_ptr++;
		val_u32 += 4;
	}
	dump_data((u8 *)sram_ptr, 0x20);

	/* Initialize the receive buffer with zeroes so that we can be sure
	 * the transfer worked, then start the receive transfer.
	 */
	for (i = 0; i < test_size; i++)
		channels[1].interface_p->buffer[i] = 0;

	DBG_INFO("%s, %d, Init buffer done @ %p\n", __func__, __LINE__, channels[1].interface_p->buffer);
	dump_data(channels[1].interface_p->buffer, 0x20);

	channels[1].interface_p->length = test_size;
	transfer(&channels[1]);

	/* Verify the receiver buffer matches the transmit buffer to
	 * verify the transfer was good
	 */

	u32_dst_ptr = (u32 *)(channels[1].interface_p->buffer);
	u32_src_ptr = sram_ptr;
	DBG_INFO("%s, %d, u32_dst_ptr=0x%x, u32_src_ptr=0x%x\n", __func__, __LINE__, (int)u32_dst_ptr, (int)u32_src_ptr);

	dump_data(channels[1].interface_p->buffer, 0x20);
	wrong_number = 0;
	for (i = 0 ; (i < test_size >> 2); i++) {
		if (*u32_dst_ptr != *u32_src_ptr)
			wrong_number++;
		u32_dst_ptr++;
		u32_src_ptr++;
	}

	DBG_INFO("%s, %d, test_size:%u, wrong_number:%u in 4bytes\n", __func__, __LINE__,
		test_size>>2, wrong_number);

	if (wrong_number == 0)
		printk(KERN_INFO "Test result: Data in MM: OK\n\n");
	else
		printk(KERN_INFO "Test result: Data in MM: NG\n\n");
	#endif //#if RX_TEST

	iounmap(sram_ptr);
}
#endif

/* Map the memory for the channel interface into user space such that user space can
 * access it taking into account if the memory is not cached.
 */
static int mmap(struct file *file_p, struct vm_area_struct *vma)
{
	struct dma_proxy_channel *pchannel_p = (struct dma_proxy_channel *)file_p->private_data;

	FUNC_DEBUG();

	/* The virtual address to map into is good, but the page frame will not be good since
	 * user space passes a physical address of 0, so get the physical address of the buffer
	 * that was allocated and convert to a page frame number.
	 */
	if (cached_buffers) {
		if (remap_pfn_range(vma, vma->vm_start,
							virt_to_phys((void *)pchannel_p->interface_p)>>PAGE_SHIFT,
							vma->vm_end - vma->vm_start, vma->vm_page_prot))
			return -EAGAIN;
		return 0;
	} else

		/* Step 3, use the DMA utility to map the DMA memory into space so that the
		 * user space application can use it
		 */
		return dma_common_mmap(pchannel_p->dma_device_p, vma,
							   pchannel_p->interface_p, pchannel_p->interface_phys_addr,
							   vma->vm_end - vma->vm_start);
}

/* Open the device file and set up the data pointer to the proxy channel data for the
 * proxy channel such that the ioctl function can access the data structure later.
 */
static int local_open(struct inode *ino, struct file *file)
{
	FUNC_DEBUG();

	file->private_data = container_of(ino->i_cdev, struct dma_proxy_channel, cdev);

	return 0;
}

/* Close the file and there's nothing to do for it
 */
static int release(struct inode *ino, struct file *file)
{
	FUNC_DEBUG();

	return 0;
}

/* Perform I/O control to start a DMA transfer.
 */
static long ioctl(struct file *file, unsigned int unused , unsigned long arg)
{
	struct dma_proxy_channel *pchannel_p = (struct dma_proxy_channel *)file->private_data;

	FUNC_DEBUG();

	dev_dbg(pchannel_p->dma_device_p, "ioctl\n");

	/* Step 2, call the transfer function for the channel to start the DMA and wait
	 * for it to finish (blocking in the function).
	 */
	transfer(pchannel_p);

	return 0;
}
static struct file_operations dm_fops = {
	.owner    = THIS_MODULE,
	.open     = local_open,
	.release  = release,
	.unlocked_ioctl = ioctl,
	.mmap	= mmap
};


/* Initialize the driver to be a character device such that is responds to
 * file operations.
 */
static int cdevice_init(struct dma_proxy_channel *pchannel_p, char *name)
{
	int rc;
	char device_name[32] = "dma_proxy";
	static struct class *local_class_p = NULL;

	FUNC_DEBUG();

	/* Allocate a character device from the kernel for this
	 * driver
	 */
	rc = alloc_chrdev_region(&pchannel_p->dev_node, 0, 1, "dma_proxy");

	if (rc) {
		dev_err(pchannel_p->dma_device_p, "unable to get a char device number\n");
		return rc;
	}

	/* Initialize the ter device data structure before
	 * registering the character device with the kernel
	 */
	cdev_init(&pchannel_p->cdev, &dm_fops);
	pchannel_p->cdev.owner = THIS_MODULE;
	rc = cdev_add(&pchannel_p->cdev, pchannel_p->dev_node, 1);

	if (rc) {
		dev_err(pchannel_p->dma_device_p, "unable to add char device\n");
		goto init_error1;
	}

	/* Only one class in sysfs is to be created for multiple channels,
	 * create the device in sysfs which will allow the device node
	 * in /dev to be created
	 */
	if (!local_class_p) {
		local_class_p = class_create(THIS_MODULE, DRIVER_NAME);

		if (IS_ERR(pchannel_p->dma_device_p->class)) {
			dev_err(pchannel_p->dma_device_p, "unable to create class\n");
			rc = ERROR;
			goto init_error2;
		}
	}
	pchannel_p->class_p = local_class_p;

	/* Create the device node in /dev so the device is accessible
	 * as a character device
	 */
	strcat(device_name, name);
	pchannel_p->proxy_device_p = device_create(pchannel_p->class_p, NULL,
											   pchannel_p->dev_node, NULL, device_name);

	if (IS_ERR(pchannel_p->proxy_device_p)) {
		dev_err(pchannel_p->dma_device_p, "unable to create the device\n");
		goto init_error3;
	}

	return 0;

init_error3:
	class_destroy(pchannel_p->class_p);

init_error2:
	cdev_del(&pchannel_p->cdev);

init_error1:
	unregister_chrdev_region(pchannel_p->dev_node, 1);
	return rc;
}

/* Exit the character device by freeing up the resources that it created and
 * disconnecting itself from the kernel.
 */
static void cdevice_exit(struct dma_proxy_channel *pchannel_p, int last_channel)
{
	FUNC_DEBUG();

	/* Take everything down in the reverse order
	 * from how it was created for the char device
	 */
	if (pchannel_p->proxy_device_p) {
		device_destroy(pchannel_p->class_p, pchannel_p->dev_node);
		if (last_channel)
			class_destroy(pchannel_p->class_p);

		cdev_del(&pchannel_p->cdev);
		unregister_chrdev_region(pchannel_p->dev_node, 1);
	}
}

/* Create a DMA channel by getting a DMA channel from the DMA Engine and then setting
 * up the channel as a character device to allow user space control.
 */
static int create_channel(struct dma_proxy_channel *pchannel_p, char *name, u32 direction)
{
	int rc;
	dma_cap_mask_t mask;

	FUNC_DEBUG();

	/* Zero out the capability mask then initialize it for a slave channel that is
	 * private.
	 */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE | DMA_PRIVATE, mask);

	/* Request the DMA channel from the DMA engine and then use the device from
	 * the channel for the proxy channel also.
	 */
	pchannel_p->channel_p = dma_request_channel(mask, NULL, NULL);
	if (!pchannel_p->channel_p) {
		dev_err(pchannel_p->dma_device_p, "DMA channel request error\n");
		return ERROR;
	}
	pchannel_p->dma_device_p = &pchannel_p->channel_p->dev->device;

	/* Initialize the character device for the dma proxy channel
	 */
	rc = cdevice_init(pchannel_p, name);
	if (rc) {
		return rc;
	}

	pchannel_p->direction = direction;

	/* Allocate memory for the proxy channel interface for the channel as either
	 * cached or non-cache depending on input parameter. Use the managed
	 * device memory when possible but right now there's a bug that's not understood
	 * when using devm_kzalloc rather than kzalloc, so stay with kzalloc.
	 */
	if (cached_buffers) {
		pchannel_p->interface_p = (struct dma_proxy_channel_interface *)
			kzalloc(sizeof(struct dma_proxy_channel_interface),
					GFP_KERNEL);
		printk(KERN_INFO "Allocating cached memory at 0x%08X\n",
			   (unsigned int)pchannel_p->interface_p);
	} else {

		/* Step 1, set dma memory allocation for the channel so that all of memory
		 * is available for allocation, and then allocate the uncached memory (DMA)
		 * for the channel interface
		 */
		dma_set_coherent_mask(pchannel_p->proxy_device_p, 0xFFFFFFFF);
		pchannel_p->interface_p = (struct dma_proxy_channel_interface *)
			dmam_alloc_coherent(pchannel_p->proxy_device_p,
								sizeof(struct dma_proxy_channel_interface),
								&pchannel_p->interface_phys_addr, GFP_KERNEL);
		printk(KERN_INFO "Allocating uncached memory at 0x%08X\n",
			   (unsigned int)pchannel_p->interface_p);
	}
	if (!pchannel_p->interface_p) {
		dev_err(pchannel_p->dma_device_p, "DMA allocation error\n");
		return ERROR;
	}
	return 0;
}


#if defined(SP_CBDMA_BASIC_TEST)
/* Initialize the dma proxy device driver module.
 */
static int dma_proxy_init(void)
{
	int rc;

	FUNC_DEBUG();

	printk(KERN_INFO "dma_proxy module initialized\n");

	/* Create 2 channels, the first is a transmit channel
	 * the second is the receive channel.
	 */
	rc = create_channel(&channels[0], "_tx", DMA_MEM_TO_DEV);
	DBG_INFO("%s, %d, rc=%d\n", __func__, __LINE__, rc);

	if (rc) {
		return rc;
	}

	rc = create_channel(&channels[1], "_rx", DMA_DEV_TO_MEM);
	DBG_INFO("%s, %d, rc=%d\n", __func__, __LINE__, rc);

	if (rc) {
		return rc;
	}

#ifdef INTERNAL_TEST
	test();
#endif

	return 0;
}

/* Exit the dma proxy device driver module.
 */
static void dma_proxy_exit(void)
{
	int i;

	FUNC_DEBUG();

	printk(KERN_INFO "dma_proxy module exited\n");

	/* Take care of the char device infrastructure for each
	 * channel except for the last channel. Handle the last
	 * channel seperately.
	 */
	for (i = 0; i < CHANNEL_COUNT - 1; i++) {
		if (channels[i].proxy_device_p)
			cdevice_exit(&channels[i], NOT_LAST_CHANNEL);
	}
	cdevice_exit(&channels[i], LAST_CHANNEL);

	/* Take care of the DMA channels and the any buffers allocated
	 * for the DMA transfers. The DMA buffers are using managed
	 * memory such that it's automatically done.
	 */
	for (i = 0; i < CHANNEL_COUNT; i++) {
		if (channels[i].channel_p)
			dma_release_channel(channels[i].channel_p);

		if (channels[i].interface_p && cached_buffers)
			kfree((void *)channels[i].interface_p);
	}
}

static int sp_cbdma_proxy_probe(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *reg_base;
	int ret = 0;

	FUNC_DEBUG();

	/* Get a memory to store infos */
	cbdma_proxy_info = (struct cbdma_proxy_info_s *)devm_kzalloc(&pdev->dev, sizeof(struct cbdma_proxy_info_s), GFP_KERNEL);
	DBG_INFO("cbdma_proxy_info:0x%lx\n", (unsigned long)cbdma_proxy_info);
	if (cbdma_proxy_info == NULL)
	{
		printk("cbdma_proxy_info malloc fail\n");
		ret = -ENOMEM;
		goto fail_kmalloc;
	}

	/* Init */
	mutex_init(&cbdma_proxy_info->cbdma_lock);

	/* Register CBDMA device to kernel */
	cbdma_proxy_info->dev.name = "sp_cbdma_proxy";
	cbdma_proxy_info->dev.minor = MISC_DYNAMIC_MINOR;
	ret = misc_register(&cbdma_proxy_info->dev);
	DBG_INFO("ret:%u\n", ret);
	if (ret != 0) {	
		DBG_ERR("sp_cbdma_proxy device register fail\n");
		goto fail_regdev;
	}


	/* Get CBDMA register source */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, CB_DMA_REG_NAME);
	DBG_INFO("res:0x%lx\n", (unsigned long)res);
	if (res) {
		reg_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
		if (IS_ERR(reg_base)) {
			DBG_ERR("%s devm_ioremap_resource fail\n",CB_DMA_REG_NAME);
		}
	}
	DBG_INFO("reg_base 0x%x\n",(unsigned int)reg_base);

	if (((u32)(res->start)) == 0x9C000D00) {
		/* CBDMA0 */
		cbdma_proxy_info->sram_addr = 0x9E800000;
		cbdma_proxy_info->sram_size = 40 << 10;

	} else {
		/* CBDMA1 */
		cbdma_proxy_info->sram_addr = 0x9E820000;
		cbdma_proxy_info->sram_size = 4 << 10;

	}
	DBG_INFO("%s, %d, SRAM: 0x%x bytes @ 0x%x\n", __func__, __LINE__, cbdma_proxy_info->sram_size, cbdma_proxy_info->sram_addr);

	dma_proxy_init();

fail_regdev:
	mutex_destroy(&cbdma_proxy_info->cbdma_lock);
fail_kmalloc:
	return ret;
}

static int sp_cbdma_proxy_remove(struct platform_device *pdev)
{
	FUNC_DEBUG();

	dma_proxy_exit();

	return 0;
}

static const struct of_device_id sp_cbdma_proxy_of_match[] = {
	{ .compatible = "sunplus,sp7021-cbdma-proxy" },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, sp_cbdma_proxy_of_match);

static struct platform_driver sp_cbdma_proxy_driver = {
	.probe = sp_cbdma_proxy_probe,
	.remove = sp_cbdma_proxy_remove,
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sp_cbdma_proxy_of_match),
	}
};

module_platform_driver(sp_cbdma_proxy_driver);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus CBDMA Proxy Driver");
MODULE_LICENSE("GPL");

#else

/* Initialize the dma proxy device driver module.
 */
static int __init dma_proxy_init(void)
{
	int rc;

	printk(KERN_INFO "dma_proxy module initialized\n");

	/* Create 2 channels, the first is a transmit channel
	 * the second is the receive channel.
	 */
	rc = create_channel(&channels[0], "_tx", DMA_MEM_TO_DEV);

	if (rc) {
		return rc;
	}

	rc = create_channel(&channels[1], "_rx", DMA_DEV_TO_MEM);
	if (rc) {
		return rc;
	}

#ifdef INTERNAL_TEST
	test();
#endif

	return 0;
}

/* Exit the dma proxy device driver module.
 */
static void __exit dma_proxy_exit(void)
{
	int i;

	printk(KERN_INFO "dma_proxy module exited\n");

	/* Take care of the char device infrastructure for each
	 * channel except for the last channel. Handle the last
	 * channel seperately.
	 */
	for (i = 0; i < CHANNEL_COUNT - 1; i++) {
		if (channels[i].proxy_device_p)
			cdevice_exit(&channels[i], NOT_LAST_CHANNEL);
	}
	cdevice_exit(&channels[i], LAST_CHANNEL);

	/* Take care of the DMA channels and the any buffers allocated
	 * for the DMA transfers. The DMA buffers are using managed
	 * memory such that it's automatically done.
	 */
	for (i = 0; i < CHANNEL_COUNT; i++) {
		if (channels[i].channel_p)
			dma_release_channel(channels[i].channel_p);

		if (channels[i].interface_p && cached_buffers)
			kfree((void *)channels[i].interface_p);
	}
}

module_init(dma_proxy_init);
module_exit(dma_proxy_exit);
MODULE_LICENSE("GPL");
#endif
