/**
 * @file    sp_ipc.c
 * @brief   Implement of Sunplus IPC Linux driver.
 * @author  qinjian
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
//#include "sp_cbdma.h"
#include <mach/sp_ipc.h>

//#define IPC_TIMEOUT_DEBUG
//#define IPC_REG_OVERWRITE
#define EXAMPLE_CODE_FOR_USER_GUIDE
#ifdef CONFIG_ARCH_ZYNQ
#define LOCAL_TEST
#endif
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* IPC Reg Group */
#ifndef CONFIG_ARCH_ZYNQ
#include <mach/io_map.h>
#define REG_GROUP(g)		VA_IOB_ADDR((g)*32*4)
#endif
#ifdef LOCAL_TEST
#define REG_GROUP_A2B		(&ipc->local)
#define REG_GROUP_B2A		(&ipc->local)
#else
#define REG_GROUP_A2B		REG_GROUP(258)
#define REG_GROUP_B2A		REG_GROUP(259)
#endif

#define IPC_LOCAL			((ipc_t *)REG_GROUP_A2B)
#define IPC_REMOTE			((ipc_t *)REG_GROUP_B2A)

#ifdef IPC_USE_CBDMA
#define IPC_CBDMA_NAME      "sp_cbdma.0"
#define CBDMA_NAND_BUFF_MAX  (19 << 10)
#define IPC_CBDMA_BUFF_MAX	(8 << 10)
#define IPC_CBDMA_BUFF_ADDR (0x9E800000 + CBDMA_NAND_BUFF_MAX + (1 << 10))
#endif
#define IPC_SEQ_LEN         (4)
/* IPC Interrupt Number */
#ifdef CONFIG_ARCH_ZYNQ
#define CA9_DIRECT_INT0		(60)
#else
#define CA9_DIRECT_INT0		(182)
#endif
#define CA9_DIRECT_INT1		(CA9_DIRECT_INT0 + 1)
#define CA9_DIRECT_INT2		(CA9_DIRECT_INT0 + 2)
#define CA9_DIRECT_INT3		(CA9_DIRECT_INT0 + 3)
#define CA9_DIRECT_INT4		(CA9_DIRECT_INT0 + 4)
#define CA9_DIRECT_INT5		(CA9_DIRECT_INT0 + 5)
#define CA9_DIRECT_INT6		(CA9_DIRECT_INT0 + 6)
#define CA9_DIRECT_INT7		(CA9_DIRECT_INT0 + 7)
#define A926_DIRECT_INT0	(CA9_DIRECT_INT0 + 8)
#define A926_DIRECT_INT1	(CA9_DIRECT_INT0 + 9)
#define A926_DIRECT_INT2	(CA9_DIRECT_INT0 + 10)
#define A926_DIRECT_INT3	(CA9_DIRECT_INT0 + 11)
#define A926_DIRECT_INT4	(CA9_DIRECT_INT0 + 12)
#define A926_DIRECT_INT5	(CA9_DIRECT_INT0 + 13)
#define A926_DIRECT_INT6	(CA9_DIRECT_INT0 + 14)
#define A926_DIRECT_INT7	(CA9_DIRECT_INT0 + 15)
#define CA9_INT				(CA9_DIRECT_INT0 + 16)
#define A926_INT			(CA9_DIRECT_INT0 + 17)

#define IRQ_RPC				CA9_INT

/* IPC Driver Defines */
#define RPC_TIMEOUT			7000	// ms
#define RPC_NO_TIMEOUT		0		// ms

#define FIFO_SIZE			16		// must be 2^n
#define FIFO_MASK			(FIFO_SIZE - 1)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define REG_ALIGN(len)      (((len) + 3)&(~3))
#define printf				printk
#define malloc(n)			kmalloc(n, GFP_KERNEL)
#define free				kfree

#define ZALLOC(n)			kzalloc(n, GFP_KERNEL)

#define MSLEEP(n)			usleep_range((n)*1000, (n)*1000)

#define WAIT_INIT(wait, t) \
{ \
	wait_t *w = (wait_t *)(wait); \
	if ((w->timeout = msecs_to_jiffies(t))) { \
		w->waked = 0; \
		init_waitqueue_head(&w->wq); \
	} else { \
		sema_init(&w->sem, 0); \
	} \
}
#define DOWN(wait) \
{ \
	wait_t *w = (wait_t *)(wait); \
	if (w->timeout) { \
		long r; \
		r = wait_event_interruptible_timeout(w->wq, w->waked, w->timeout); \
		if (r == 0) return IPC_FAIL_TIMEOUT; \
		if (r < 0) return IPC_FAIL; \
	} else { \
		if (down_killable(&w->sem)) return IPC_FAIL; \
	} \
}
#define UP(wait) \
{ \
	wait_t *w = (wait_t *)(wait); \
	if (w->timeout) { \
		w->waked = 1; \
		if (!list_empty(&w->wq.head)) wake_up_interruptible(&w->wq); \
	} else { \
		up(&w->sem); \
	} \
}

#define RESPONSE(rpc, ret) \
{ \
	(rpc)->F_DIR = RPC_RESPONSE; \
	(rpc)->CMD = ret; \
	if (rpc_fifo_put(&ipc->res_fifo, rpc) != IPC_SUCCESS) { \
		printf("RES FIFO FULL!!!\n"); \
	} \
}

#ifdef _SP_CHUNKMEM_H_
#define __PA(addr) \
({ \
	u32 t = sp_chunk_pa((void *)(addr)); \
	(t ? t : __pa(addr)); \
})
#define __VA(addr) \
({ \
	void *t = sp_chunk_va((u32)(addr)); \
	(t ? t : __va(addr)); \
})
#else
#define __PA(addr)	__pa(addr)
#define __VA(addr)	__va(addr)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct {
	u32 timeout;
	union {
		struct {
			u32	waked;
			wait_queue_head_t wq;
		};
		struct semaphore sem;
	};
} wait_t;

typedef struct {
	rpc_t	rpc;
	wait_t	wait_response;
} request_t;

typedef struct {
	rpc_t	data[FIFO_SIZE];
	u32		in;						// write pointer
	u32		out;					// read pointer
	wait_t	wait;
} rpc_fifo_t;
#ifdef IPC_USE_CBDMA
typedef struct ipc_cbdma_t {
	struct device *cbdma_device;
	u32 phy_addr;
	u32 size;
	void  *vir_addr;
	struct mutex  cbdma_lock;
}ipc_cbdma_t;
#else
typedef struct {
	u32 seq;
	struct mutex lock;
}ipc_sequense_t;
#endif

typedef struct {
	struct miscdevice dev;			// ipc device
	struct mutex write_lock;
	struct task_struct *rpc_res;	// rpc RESPONSE thread
	rpc_fifo_t	res_fifo;			// rpc RESPONSE fifo
	rpc_fifo_t	*fifo[SERVER_NUMS];	// server fifo
	u32	pid[SERVER_NUMS];			// server pid
#ifdef IPC_USE_CBDMA
	struct ipc_cbdma_t cbdma;
#else
	ipc_sequense_t seq;
#endif
#ifdef LOCAL_TEST
	ipc_t	local;
#endif
} sp_ipc_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static sp_ipc_t *ipc;

/******************************* CACHE FUNCS ******************************/

#include <asm/cacheflush.h>

void DCACHE_CLEAN(u32 pa, void *start, int size)
{
	unsigned long oldIrq;

	local_irq_save(oldIrq);
	arm_dma_ops.sync_single_for_device(NULL, pa, size, DMA_TO_DEVICE);
	local_irq_restore(oldIrq);
}

void DCACHE_INVALIDATE(u32 pa, void *start, int size)
{
	unsigned long oldIrq;

	local_irq_save(oldIrq);
	arm_dma_ops.sync_single_for_cpu(NULL, pa, size, DMA_FROM_DEVICE);
	local_irq_restore(oldIrq);
}

#ifdef CONFIG_SP_IPC_TEST // test & example
/*************************** IPC KERNEL API TEST **************************/

#ifdef _SP_CHUNKMEM_H_
#define TEST_CHUNKMEM
#endif

#define CA7_READY   (0xca700001)

static irqreturn_t mbox_isr(int irq, void *dev_id)
{
	int i = irq - CA9_DIRECT_INT0;
	u32 d = IPC_REMOTE->MBOX[i];
	early_printk("[MBOX_%d] %08x (%u)\n", i, d, d);
	return IRQ_HANDLED;
}

static int test_set(const char *val, const struct kernel_param *kp)
{
	int i, ret;
	u32 len, timeout;
	void *p;

	len = timeout = 16;
	sscanf(val, "%u %u", &len, &timeout);

	if (len < 8) {
		// test direct int
		for (i = 0; i < 8; i++)
			request_irq(CA9_DIRECT_INT0 + i, mbox_isr, IRQF_TRIGGER_RISING, "mbox", NULL);

		IPC_LOCAL->MBOX[7] = CA7_READY; // notify B
		mdelay(100); // wait B2A

		for (i = 0; i < 8; i++)
			free_irq(CA9_DIRECT_INT0 + i, NULL);
		return 0;
	}

#ifdef TEST_CHUNKMEM
	if (len > 4096) {
		printk("len error %d\n", len);
		return -EINVAL;
	}
	p = sp_chunk_malloc_nocache(0, 0, len);
#else
	p = (void *)MALLOC(len);;
#endif
	memset(p, 0x11, len);
	*(u32 *)p = 0;
	*((u32 *)p + 1) = len;
	if ((0 < timeout) && (timeout <= RPC_TIMEOUT)) {
		ret = IPC_FunctionCall(0, p, len);
	} else {
		ret = IPC_FunctionCall_timeout(0, p, len, timeout);
	}
	while(!ret && (len--)){
		if(((u8 *)p)[len] != 0) {
			printk("check error,  0x%0x, len:%0x\n", ((u8 *)p)[len], len);
			ret = IPC_FAIL;
			break;
		}
	}
	printf("RET = %d\n", ret);

#ifdef TEST_CHUNKMEM
	sp_chunk_free(p);
#else
	FREE(p);
#endif
	return 0;
}

static const struct kernel_param_ops test_ops = {
    .set = test_set,
};
module_param_cb(test, &test_ops, NULL, 0600);
#endif

/**************************************************************************/

#ifdef LOCAL_TEST
//#include <asm/hardware/gic.h>
static irqreturn_t rpc_isr(int irq, void *dev_id);

static void irq_trigger(int irq)
{
#if 0
	writel_relaxed(1 << (irq % 32), gic_base(GIC_DIST_BASE) + GIC_DIST_PENDING_SET + (irq / 32) * 4);
#else
	rpc_isr(IRQ_RPC, NULL);
#endif
}
#endif

/********************************* RPC HAL ********************************/

#ifndef IPC_USE_CBDMA
static void ipc_seq_init(ipc_sequense_t *seq) {
	seq->seq = 0;
	mutex_init(&seq->lock);
}
static void ipc_seq_finit(ipc_sequense_t *seq)
{
	seq->seq = 0;
	mutex_destroy(&seq->lock);
}
static inline u32 ipc_seq_inc(ipc_sequense_t *seq)
{
	u32 s;
	mutex_lock(&seq->lock);
	seq->seq++;
	s = seq->seq;
	mutex_unlock(&seq->lock);
	return s;
}
static inline void rpc_add_seq(rpc_t *rpc)
{
	u32 seq = ipc_seq_inc(&ipc->seq);
	u16 len = rpc->DATA_LEN;
	u32 *addr = (u32 *)((u8 *)rpc->DATA_PTR + REG_ALIGN(len));
	rpc->SEQ_ADDR = (void *)__PA(addr);
	*addr = seq;
	rpc->SEQ = seq;
}

static int rpc_check_seq(rpc_t *rpc)
{
	u32 seq = rpc->SEQ;
	u32 *addr = __VA(rpc->SEQ_ADDR);
	unsigned long timeout = jiffies + msecs_to_jiffies(10); // 10ms
	while(*addr != seq) {
		void *va = rpc->DATA_PTR;
		u32 pa = __PA(va);
		if(time_after(jiffies, timeout)) {
			printk("seq:0x%x, seq_data:%0x\n", seq, *addr);
			rpc_dump("RPC_CHECK_ERR:", rpc);
			return IPC_FAIL_DATANOTRDY;
		}
		printk("seq:0x%x, seq_data:%0x\n", seq, *addr);
		DCACHE_INVALIDATE(pa, va, CACHE_ALIGN(rpc->DATA_LEN + IPC_SEQ_LEN));
		udelay(1);
	}

	return IPC_SUCCESS;
}

#endif


/* len must 4 btyes align */
static void ipc_memcpy(void *dst, void *src, u32 len)
{
	u32 *d = (u32 *)dst;
	u32 *s = (u32 *)src;
	while(len) {
		*d++ = *s++;
		len -= 4;
	}
}

static void rpc_copy(rpc_t *dst, rpc_t *src)
{
	int len;
	trace();
	ipc_memcpy(dst, src, REG_ALIGN(RPC_HEAD_SIZE));
	if (src == &IPC_REMOTE->RPC) {	// reg read IMPORTANT: DON'T read IPC_REMOTE twice!!!
#ifdef IPC_REG_OVERWRITE
		if (IPC_REMOTE->F_OVERWRITE) {
			printk("F_OVERWRITE:%08x\n", IPC_REMOTE->F_OVERWRITE);
		}
#endif
		trace();
		len = dst->DATA_LEN;
		if (len > RPC_DATA_SIZE) {
			u32 pa = (u32) src->DATA_PTR;
			dst->DATA_PTR = __VA(pa);
			print("pa->va: %08x -> %p\n", pa, dst->DATA_PTR);
			DCACHE_INVALIDATE(pa, dst->DATA_PTR, CACHE_ALIGN(len + IPC_SEQ_LEN));
		} else {
			ipc_memcpy(dst->DATA, src->DATA, REG_ALIGN(len));
		}
	} else if (dst == &IPC_LOCAL->RPC) {	// reg write   IMPORTANT: DON'T write IPC_LOCAL twice!!!
		len = src->DATA_LEN;
		if (len > RPC_DATA_SIZE) {
			u32 pa = __PA(src->DATA_PTR);
			dst->DATA_PTR = (void *)pa;
			print("va->pa: %p -> %08x\n", src->DATA_PTR, pa);
#ifndef IPC_USE_CBDMA
			DCACHE_CLEAN(pa, src->DATA_PTR, CACHE_ALIGN(len + IPC_SEQ_LEN));
#endif
		} else {
			ipc_memcpy(dst->DATA, src->DATA, REG_ALIGN(len));
		}
	} else {		// nomal copy	
		trace();
		len = src->DATA_LEN;
		if (len > RPC_DATA_SIZE) {
			dst->DATA_PTR = src->DATA_PTR;
		} else {
			memcpy(dst->DATA, src->DATA, len);
		}
	}
}

static void rpc_read_hw(rpc_t *rpc)
{
	trace();
	rpc_copy(rpc, &IPC_REMOTE->RPC);	
	rpc_dump("RD_HW_ACHIP", rpc);
}

static void rpc_read_hw_of_926(rpc_t *rpc)
{
	rpc_copy(rpc, &IPC_LOCAL->RPC);	
	rpc_dump("RD_HW_926", rpc);
}

static int WAIT_IPC_WRITEABLE(u32 mask)
{
	int ret = IPC_SUCCESS;
	int _i = IPC_WRITE_TIMEOUT;
	while (IPC_LOCAL->F_RW & (mask)) {
		MSLEEP(1);
		if (!_i--) {
			printf("write IPC HW timeout!\n");
			ret = IPC_FAIL_HWTIMEOUT;
			break;
		}
	}
	return ret;
}

static int rpc_write_hw(rpc_t *rpc)
{
	int ret = IPC_SUCCESS;
	trace();
#ifndef IPC_USE_CBDMA
	if(rpc->DATA_LEN > RPC_DATA_SIZE) {
		rpc_add_seq(rpc);
	}
#endif
	mutex_lock(&ipc->write_lock);				// lock for concurrent write

#ifndef LOCAL_TEST
	ret = WAIT_IPC_WRITEABLE(0xFFFFF0);
	if (unlikely(ret))
		goto out;
#endif
	rpc_copy(&IPC_LOCAL->RPC, rpc);
	rpc_dump("WR_HW", rpc);
	smp_wmb();
	IPC_LOCAL->TRIGGER = 1;
#ifdef LOCAL_TEST
	irq_trigger(IRQ_RPC);
#else
out:
#endif

	mutex_unlock(&ipc->write_lock);
	return ret;
}

/******************************** RPC FIFO ********************************/

static int rpc_fifo_get(rpc_fifo_t *fifo, rpc_t *rpc)
{
	trace();
DOWN(&fifo->wait);

	if ((fifo->in - fifo->out) == 0) {				// fifo is empty
		return IPC_FAIL;
	}

	rpc_copy(rpc, &fifo->data[fifo->out & FIFO_MASK]);
	smp_wmb();
	fifo->out++;

	return IPC_SUCCESS;
}

static int rpc_fifo_put(rpc_fifo_t *fifo, rpc_t *rpc)
{
	trace();
	if ((fifo->in - fifo->out) == FIFO_SIZE) {		// fifo is full
		return IPC_FAIL;
	}
	trace();
	rpc_copy(&fifo->data[fifo->in & FIFO_MASK], rpc);
	smp_wmb();
	fifo->in++;

	UP(&fifo->wait);
	trace();
	return IPC_SUCCESS;
}

/************************** MAILBOX INTR HANDLER **************************/

// TODO:

/**************************** RPC INTR HANDLER ****************************/

static irqreturn_t rpc_isr(int irq, void *dev_id)
{
	rpc_t rpc;

	trace();
	rpc_read_hw(&rpc);

	if (rpc.F_DIR == RPC_REQUEST) {
		int sid = rpc.CMD >> SERVER_ID_OFFSET;		// server id
		rpc_fifo_t *fifo = ipc->fifo[sid];
		trace();
		if (fifo == NULL) 
		{
			print("RPC SERVER #%d NOT RUNNING!!!\n", sid);
			RESPONSE(&rpc, IPC_FAIL_NOSERV);
			trace();
		} 
		else if (rpc_fifo_put(fifo, &rpc) != IPC_SUCCESS) 
		{	// put new rpc into fifo
			print("RPC SERVER #%d FIFO FULL!!!\n", sid);
			RESPONSE(&rpc, IPC_FAIL_BUSY);
		}
	}
	else {
		request_t *req = (request_t *)rpc.REQ_H;
		if (rpc.F_TYPE != REQ_NO_REP) {
			rpc_copy(&req->rpc, &rpc);
			UP(&req->wait_response);
		}
		else {
			if (rpc.DATA_LEN > RPC_DATA_SIZE) {
				FREE(req->rpc.DATA_PTR_ORG);
			}
			FREE(req);
		}
 	}

	return IRQ_HANDLED;
}

static irqreturn_t rpc_isr_926(int irq, void *dev_id)
{
	rpc_t rpc;

	trace();
	rpc_read_hw_of_926(&rpc);
	return IRQ_HANDLED;
}


static int rpc_res_thread(void *param)
{
	WAIT_INIT(&ipc->res_fifo.wait, 0);

	while (!kthread_should_stop()) {
		rpc_t rpc;
		if (rpc_fifo_get(&ipc->res_fifo, &rpc) == IPC_SUCCESS) {
			rpc_write_hw(&rpc);
		}
	}
	return 0;
}

/**************************************************************************/

static int rpc_from_user(rpc_t *rpc, rpc_t __user *rpc_user)
{
	u32 len;
#ifdef IPC_USE_CBDMA
	dma_addr_t dst;
#endif
	copy_from_user(rpc, rpc_user, RPC_HEAD_SIZE);
	len = rpc->DATA_LEN;
	if (len > RPC_DATA_SIZE) {
		if (rpc->F_DIR == RPC_REQUEST) {
			rpc->DATA_PTR_ORG = MALLOC(len + IPC_SEQ_LEN + CACHE_MASK * 2);
			rpc->DATA_PTR = (void *)CACHE_ALIGN(rpc->DATA_PTR_ORG);
		}
		else {
			get_user(rpc->DATA_PTR, &rpc_user->DATA_PTR_ORG);	// restore DATA_PTR
		}
#ifdef IPC_USE_CBDMA
		mutex_lock(& ipc->cbdma.cbdma_lock);
		copy_from_user( ipc->cbdma.vir_addr, rpc_user->DATA_PTR, len);
		dst = dma_map_single(NULL, (void *)(rpc->DATA_PTR), len, DMA_TO_DEVICE);
		if(0 > sp_cbdma_write( ipc->cbdma.cbdma_device,
				(dma_addr_t) ipc->cbdma.phy_addr,
				dst, len)) {
			printf("cbdma error\n");
		}
		dma_unmap_single(NULL, dst, len, DMA_TO_DEVICE);
		mutex_unlock(& ipc->cbdma.cbdma_lock);
#else
		copy_from_user(rpc->DATA_PTR, rpc_user->DATA_PTR, len);
#endif

	}
	else {
		copy_from_user(rpc->DATA, rpc_user->DATA, len);
	}

	return IPC_SUCCESS;
}

static int rpc_from_user_new(rpc_t *rpc, rpc_user_t *user, rpc_new_t __user *rpc_user)
{
	rpc_from_user(rpc, &rpc_user->rpc);
	copy_from_user(user, &rpc_user->user, sizeof(rpc_user_t));
	return IPC_SUCCESS;
}
#define RET(r)	((r > 511) ? (r - 1024) : r)

static int rpc_to_user(rpc_t __user *rpc_user, rpc_t *rpc)
{
	int ret = IPC_SUCCESS;
	u32 len = rpc->DATA_LEN;
	trace();

	if (rpc->F_DIR == RPC_REQUEST) {
		if (len > RPC_DATA_SIZE) {
#ifndef IPC_USE_CBDMA
			ret = rpc_check_seq(rpc);
			if(ret) {
				return ret;
			}
#endif
			copy_to_user(rpc_user, rpc, RPC_HEAD_SIZE);
			put_user(rpc->DATA_PTR, &rpc_user->DATA_PTR_ORG);	// backup DATA_PTR
			copy_to_user(rpc_user->DATA_PTR, rpc->DATA_PTR, len);
		}
		else {
			copy_to_user(rpc_user, rpc, RPC_HEAD_SIZE + len);
		}
	}
	else {
		ret = RET(rpc->CMD);
		if (ret == IPC_SUCCESS) {
			if (len > RPC_DATA_SIZE) {
#ifndef IPC_USE_CBDMA
				ret = rpc_check_seq(rpc);
				if(ret) {
					FREE(rpc->DATA_PTR_ORG);
					return ret;
				}
#endif
				copy_to_user(rpc_user->DATA_PTR, rpc->DATA_PTR, len);
			}
			else {
				copy_to_user(rpc_user->DATA, rpc->DATA, len);
			}
		}
		if (len > RPC_DATA_SIZE) {
			FREE(rpc->DATA_PTR_ORG);
		}
	}

	return ret;
}

/****************************** RPC INTERFACE ******************************/

static int rpc_read(rpc_t __user *rpc_user)
{
	request_t *req;
	int ret = get_user(req, &rpc_user->REQ_H);
	u32 sid = (u32)req;								// server id
	trace();

	if (sid > SERVER_NUMS) {						// read deferred response
		DOWN(&req->wait_response);
		ret = rpc_to_user(rpc_user, &req->rpc);
		FREE(req);
	}
	else {											// read request
		rpc_t rpc;
		if (rpc_fifo_get(ipc->fifo[sid], &rpc) == IPC_SUCCESS) {	// get a rpc from fifo
			rpc_to_user(rpc_user, &rpc);
		}
	}

	return ret;
}

static int rpc_write(rpc_t __user *rpc_user)
{
	request_t *req = (request_t *)MALLOC(sizeof(request_t));
	rpc_t *rpc = &req->rpc;
	int ret = rpc_from_user(rpc, rpc_user);
	trace();

	if (rpc->F_DIR == RPC_REQUEST) {
		rpc->REQ_H = req;
		if (rpc->F_TYPE != REQ_NO_REP) {
			WAIT_INIT(&req->wait_response, RPC_TIMEOUT);
		}
		ret = rpc_write_hw(rpc);						// write request
		if (unlikely(ret)) {
			FREE(req);
			return ret;
		}
		if (rpc->F_TYPE == REQ_WAIT_REP) {
			DOWN(&req->wait_response);			// wait response
			ret = rpc_to_user(rpc_user, rpc);
			FREE(req);
		}
		else if (rpc->F_TYPE == REQ_DEFER_REP) {
			put_user(req, &rpc_user->REQ_H);	// return REQ_H
		}
	}
	else {
		ret = rpc_write_hw(rpc);						// write response
		FREE(req);
	}

	return ret;
}

static int rpc_write_new(rpc_new_t __user *rpc_user)
{
	request_t *req = (request_t *)MALLOC(sizeof(request_t));
	rpc_user_t user = {0};
	rpc_t *rpc = &req->rpc;
	int ret = rpc_from_user_new(rpc, &user,rpc_user);
	trace();
	if (rpc->F_DIR == RPC_REQUEST) {
		rpc->REQ_H = req;
		if (rpc->F_TYPE != REQ_NO_REP) {
		#ifdef IPC_TIMEOUT_DEBUG
			printk("timeout = %d\n", user.timeout);
		#endif
			WAIT_INIT(&req->wait_response, user.timeout);
		}
		ret = rpc_write_hw(rpc);						// write request
		if (unlikely(ret)) {
			FREE(req);
			return ret;
		}
		if (rpc->F_TYPE == REQ_WAIT_REP) {
			DOWN(&req->wait_response);			// wait response
			ret = rpc_to_user(&rpc_user->rpc, rpc);
			FREE(req);
		}
		else if (rpc->F_TYPE == REQ_DEFER_REP) {
			put_user(req, &rpc_user->rpc.REQ_H);	// return REQ_H
		}
	}
	else {
		ret = rpc_write_hw(rpc);						// write response
		FREE(req);
	}
	return ret;
}
/***************************** IPC KERNEL API *****************************/

int IPC_FunctionCall(int cmd, void *data, int len)
{
	int ret;
	request_t *req = (request_t *)MALLOC(sizeof(request_t));
	rpc_t *rpc = &req->rpc;
	void *p = NULL;						// temp buffer for cache align
#ifdef IPC_USE_CBDMA
	dma_addr_t dst;
#endif
	if (len > IPC_DATA_SIZE_MAX) {
		return IPC_FAIL_INVALID;
	}

	/* init rpc */
	rpc->REQ_H    = req;
	rpc->F_DIR    = RPC_REQUEST;
	rpc->F_TYPE   = REQ_WAIT_REP;
	rpc->CMD      = cmd;
	rpc->DATA_LEN = len;
	if (len <= RPC_DATA_SIZE) {
		memcpy(rpc->DATA, data, len);
	}
#if 0
	else if (CACHE_ALIGNED(data) && CACHE_ALIGNED(len)) {
		rpc->DATA_PTR = data;
	}
#endif
	else {								// do cache align
		p = MALLOC(len + IPC_SEQ_LEN + CACHE_MASK * 2);
		rpc->DATA_PTR = (void *)CACHE_ALIGN(p);
#ifdef IPC_USE_CBDMA
		mutex_lock(& ipc->cbdma.cbdma_lock);
		copy_from_user(ipc->cbdma.vir_addr, data, len);
		dst = dma_map_single(NULL, (void *)(rpc->DATA_PTR), len, DMA_TO_DEVICE);
		if(0 > sp_cbdma_write( ipc->cbdma.cbdma_device,
				(dma_addr_t) ipc->cbdma.phy_addr,
				dst, len)) {
			printf("cbdma error\n");
		}
		dma_unmap_single(NULL, dst, len, DMA_TO_DEVICE);
		mutex_unlock(& ipc->cbdma.cbdma_lock);
#else
		memcpy(rpc->DATA_PTR, data, len);
#endif
	}

	/* do rpc */
	rpc_dump("REQ", rpc);
	WAIT_INIT(&req->wait_response, RPC_TIMEOUT);
	rpc_write_hw(rpc);					// write request
	DOWN(&req->wait_response);			// wait response
	ret = RET(rpc->CMD);
	rpc_dump("RES", rpc);

	/* return data */
	if (ret == IPC_SUCCESS) {
		if (len <= RPC_DATA_SIZE) {
			memcpy(data, rpc->DATA, len);
		}
		else if (p) {
#ifndef IPC_USE_CBDMA
			ret = rpc_check_seq(rpc);
			if(ret) {
				FREE(p);
				FREE(req);
				return ret;
			}
#endif
			memcpy(data, rpc->DATA_PTR, len);
		}
	}
	if (p) FREE(p);						// free temp buffer
	FREE(req);
	return ret;
}
EXPORT_SYMBOL(IPC_FunctionCall);

int IPC_FunctionCall_timeout(int cmd, void *data, int len, u32 timeout)
{
	int ret;
	request_t *req = (request_t *)MALLOC(sizeof(request_t));
	rpc_t *rpc = &req->rpc;
	void *p = NULL;						// temp buffer for cache align
#ifdef IPC_USE_CBDMA
	dma_addr_t dst;
#endif
	if (len > IPC_DATA_SIZE_MAX) {
		return IPC_FAIL_INVALID;
	}
	if ((timeout < RPC_TIMEOUT) && (timeout != RPC_NO_TIMEOUT)) {
		timeout = RPC_TIMEOUT;
	}
	/* init rpc */
	rpc->REQ_H    = req;
	rpc->F_DIR    = RPC_REQUEST;
	rpc->F_TYPE   = REQ_WAIT_REP;
	rpc->CMD      = cmd;
	rpc->DATA_LEN = len;
	if (len <= RPC_DATA_SIZE) {
		memcpy(rpc->DATA, data, len);
	}
#if 0
	else if (CACHE_ALIGNED(data) && CACHE_ALIGNED(len)) {
		rpc->DATA_PTR = data;
	}
#endif
	else {								// do cache align
		p = MALLOC(len + IPC_SEQ_LEN + CACHE_MASK * 2);
		rpc->DATA_PTR = (void *)CACHE_ALIGN(p);
#ifdef IPC_USE_CBDMA
		mutex_lock(& ipc->cbdma.cbdma_lock);
		copy_from_user(ipc->cbdma.vir_addr, data, len);
		dst = dma_map_single(NULL, (void *)(rpc->DATA_PTR), len, DMA_TO_DEVICE);
		if(0 > sp_cbdma_write( ipc->cbdma.cbdma_device,
				(dma_addr_t) ipc->cbdma.phy_addr,
				dst, len)) {
			printf("cbdma error\n");
		}
		dma_unmap_single(NULL, dst, len, DMA_TO_DEVICE);
		mutex_unlock(& ipc->cbdma.cbdma_lock);
#else
		memcpy(rpc->DATA_PTR, data, len);
#endif
	}

	/* do rpc */
	rpc_dump("REQ", rpc);
	WAIT_INIT(&req->wait_response, timeout);
	rpc_write_hw(rpc);					// write request
	DOWN(&req->wait_response);			// wait response
	ret = RET(rpc->CMD);
	rpc_dump("RES", rpc);

	/* return data */
	if (ret == IPC_SUCCESS) {
		if (len <= RPC_DATA_SIZE) {
			memcpy(data, rpc->DATA, len);
		}
		else if (p) {
#ifndef IPC_USE_CBDMA
			ret = rpc_check_seq(rpc);
			if(ret) {
				FREE(p);
				FREE(req);
				return ret;
			}
#endif
			memcpy(data, rpc->DATA_PTR, len);
		}
	}
	if (p) FREE(p);						// free temp buffer

	FREE(req);
	return ret;
}
EXPORT_SYMBOL(IPC_FunctionCall_timeout);


/**************************************************************************/

static int reg_server(int sid)
{
	rpc_fifo_t *fifo = ipc->fifo[sid];
	trace();

	if (fifo) {
		struct task_struct *task;
		rcu_read_lock();
		task = find_task_by_vpid(ipc->pid[sid]);
		if (task && !task_is_stopped(task) && !(task->exit_state)) {
			rcu_read_unlock();
			return IPC_FAIL_BUSY;		// already running
		}
		rcu_read_unlock();
		memset(fifo, 0, sizeof(rpc_fifo_t));
	}
	else {
		fifo = ZALLOC(sizeof(rpc_fifo_t));
		if (fifo == NULL) {
			return IPC_FAIL_NOMEM;
		}
	}

	WAIT_INIT(&fifo->wait, 0);
	ipc->pid[sid] = current->tgid;		// new server
	ipc->fifo[sid] = fifo;

	return IPC_SUCCESS;
}

static int sp_ipc_release(struct inode *inode, struct file *file)
{
	u32 pid = current->tgid;
	int i = SERVER_NUMS;
	trace();

	while (i--) {
		if (ipc->pid[i] == pid) {		// remove server
			FREE(ipc->fifo[i]);
			ipc->fifo[i] = NULL;
			ipc->pid[i] = 0;
		}
	}
	return 0;
}

#define RET_K(r)	((r > 0) ? (r - 1024) : r)

static ssize_t sp_ipc_read(struct file *filp, char __user *buffer,
							 size_t length, loff_t *offset)
{
#ifdef EXAMPLE_CODE_FOR_USER_GUIDE
	printk("sp_ipc_read: %d\n", readl(&IPC_LOCAL->RPC));
	printk("Sunplus mailbox read\n");
	return 0;	

#else
	int ret = IPC_FAIL_INVALID;
	trace(); 
	if (length == sizeof(rpc_t)) {
		ret = rpc_read((rpc_t __user *)buffer);
	}

	return (ret ? RET_K(ret) : length);
#endif 
}

static ssize_t sp_ipc_write(struct file *filp, const char __user *buffer,
							 size_t length, loff_t *offset)
{
#ifdef EXAMPLE_CODE_FOR_USER_GUIDE
	unsigned char num[2];
	char *tmp;
    unsigned int setnum, setvalue; 
	tmp = memdup_user(buffer, length);
	num[0] = tmp[0];
   	num[1] = tmp[1];	
	writel(num[1],&IPC_LOCAL->RPC);
	printk("Sunplus mailbox write\n");	
	return 0;	
#else
	int ret = IPC_FAIL_INVALID;
	trace();

	if (length == sizeof(int)) {		// register server
		int sid;
		get_user(sid, (int __user *)buffer);
		ret = reg_server(sid);
	}
	else if (length == sizeof(rpc_t)) {
		ret = rpc_write((rpc_t __user *)buffer);
	}else if (length == sizeof(rpc_new_t)) {
		ret = rpc_write_new((rpc_new_t __user *)buffer);
	}

	return (ret ? RET_K(ret) : length);
#endif 
}

static int sp_ipc_mmap(struct file *file, struct vm_area_struct *vma)
{
#ifdef IO0_START
	if (vma->vm_pgoff >= __phys_to_pfn(IO0_START) &&
		vma->vm_pgoff <= __phys_to_pfn(IO0_START + IO0_SIZE)) {
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	}
#endif
	return io_remap_pfn_range(vma,
							  vma->vm_start,
							  vma->vm_pgoff,
							  vma->vm_end - vma->vm_start,
							  vma->vm_page_prot);
}

static struct file_operations sp_ipc_fops = {
    .owner          = THIS_MODULE,
	.release		= sp_ipc_release,
    .read           = sp_ipc_read,
	.write			= sp_ipc_write,
	.mmap			= sp_ipc_mmap,
};

/**************************************************************************/

/**
 * @brief   IPC driver probe function
 */
static int sp_ipc_probe(struct platform_device *pdev)
{
	int ret = -ENXIO;

	ipc = (sp_ipc_t *)devm_kzalloc(&pdev->dev, sizeof(sp_ipc_t), GFP_KERNEL);
	if (ipc == NULL) {
		printf("sp_ipc_t malloc fail\n");
		ret	= -ENOMEM;
		goto fail_kmalloc;
	}

	/* init */
    mutex_init(&ipc->write_lock);
	ipc->rpc_res = kthread_run(rpc_res_thread, NULL, "RpcRes");

	ret = devm_request_irq(&pdev->dev, IRQ_RPC, rpc_isr, IRQF_TRIGGER_RISING, "rpc", NULL);
	if (ret != 0) {
		printf("sp_ipc request_irq fail!\n");
		goto fail_reqirq;
	}
	#if 0
	ret = devm_request_irq(&pdev->dev, IRQ_A926, rpc_isr_926, IRQF_TRIGGER_RISING, "rpc", NULL);
	if (ret != 0) {
		printf("sp_ipc rpc_isr_926 fail!\n");
		goto fail_reqirq;
	}
    #endif 
	/* register device */
	ipc->dev.name  = "sp_ipc";
	ipc->dev.minor = MISC_DYNAMIC_MINOR;
	ipc->dev.fops  = &sp_ipc_fops;
	ret = misc_register(&ipc->dev);
	if (ret != 0) {
		printf("sp_ipc device register fail\n");
		goto fail_regdev;
	}
#ifdef IPC_USE_CBDMA
	printk("[ipc info] use cbdma \n");
	ipc->cbdma.cbdma_device = sp_cbdma_getbyname(IPC_CBDMA_NAME);
	if(!ipc->cbdma.cbdma_device) {
		printf("find cbdma device error\n");
		goto fail_findcbdmadev;
	}
	mutex_init(&ipc->cbdma.cbdma_lock);
	ipc->cbdma.phy_addr = IPC_CBDMA_BUFF_ADDR;
	ipc->cbdma.size = IPC_CBDMA_BUFF_MAX;
	ipc->cbdma.vir_addr = ioremap(ipc->cbdma.phy_addr,  ipc->cbdma.size);
#else
	ipc_seq_init(&ipc->seq);
#endif

	return 0;

    /* error rollback */
#ifdef IPC_USE_CBDMA
fail_findcbdmadev:
	misc_deregister(&ipc->dev);
#endif
fail_regdev:
fail_reqirq:
	kthread_stop(ipc->rpc_res);
    mutex_destroy(&ipc->write_lock);
fail_kmalloc:
	return ret;
}

/**
 * @brief   IPC driver exit function
 */
static int sp_ipc_remove(struct platform_device *pdev)
{
	misc_deregister(&ipc->dev);
	kthread_stop(ipc->rpc_res);
    mutex_destroy(&ipc->write_lock);
#ifdef IPC_USE_CBDMA
    mutex_destroy(&ipc->cbdma.cbdma_lock);
#else
	ipc_seq_finit(&ipc->seq);
#endif

	return 0;
}

static const struct of_device_id sp_ipc_of_match[] = {
	{ .compatible = "sunplus,sp7021-ipc" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sp_ipc_of_match);

static struct platform_driver sp_ipc_driver = {
    .probe      = sp_ipc_probe,
    .remove     = sp_ipc_remove,
    .driver     = {
        .name   = "sp_ipc",
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(sp_ipc_of_match),
    },
};

module_platform_driver(sp_ipc_driver);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Sunplus Technology");
MODULE_DESCRIPTION("Sunplus IPC Driver");
MODULE_LICENSE("GPL");
