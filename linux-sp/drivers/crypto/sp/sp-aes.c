/**
 * Cryptographic API.
 * Support for OMAP AES HW acceleration.
 * Author:jz.xiang
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <crypto/internal/aead.h>
#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/scatterwalk.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/crypto.h>
#include <linux/delay.h>
#include <crypto/aes.h>
#include "sp-crypto.h"

#define WORKBUF_SIZE (AES_BLOCK_SIZE + AES_BLOCK_SIZE + AES_MAX_KEYLENGTH)	// tmp + iv + key

/* structs */
struct sp_aes_ctx {
	crypto_ctx_t base;

	u8 *tmp, *iv;
	dma_addr_t pa; // workbuf phy addr
	u32 ivlen, keylen;
};

static int sp_cra_aes_init(struct crypto_tfm *tfm, u32 mode)
{
	struct sp_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	SP_CRYPTO_TRACE();

	ctx->base.mode = mode;
	ctx->ivlen = AES_BLOCK_SIZE;

	ctx->tmp = dma_alloc_coherent(NULL, WORKBUF_SIZE, &ctx->pa, GFP_KERNEL);
	ctx->iv = ctx->tmp + AES_BLOCK_SIZE;

	return crypto_ctx_init(&ctx->base, SP_CRYPTO_AES);
}

static int sp_cra_aes_ecb_init(struct crypto_tfm *tfm)
{
	return sp_cra_aes_init(tfm, M_AES_ECB);
}

static int sp_cra_aes_cbc_init(struct crypto_tfm *tfm)
{
	return sp_cra_aes_init(tfm, M_AES_CBC);
}

static int sp_cra_aes_ctr_init(struct crypto_tfm *tfm)
{
	return sp_cra_aes_init(tfm, M_AES_CTR | (128 << 24)); // CTR M = 128
}

static void sp_cra_aes_exit(struct crypto_tfm *tfm)
{
	struct sp_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	dma_free_coherent(NULL, WORKBUF_SIZE, ctx->tmp, ctx->pa);
	crypto_ctx_exit(crypto_tfm_ctx(tfm));
}

/*
  * counter = counter  + 1
  *  counter:small endian
  */
static void ctr_inc1(u8 *counter, u32 len)
 {
     u32 n = 0;
     u8 c;

     do {
         c = counter[n];
         ++c;
         counter[n] = c;
         if (c)
             return;
         ++n;
     } while (n < len);
 }


/*
  * counter = counter  + inc
  *  counter:small endian
  */
static void ctr_inc(u8 *counter, u32 len, u32 inc)
{
	u32 c, c1;
	u32 *p = (u32 *)counter;

	c =  __le32_to_cpu(*p);
	c1 = c;
	c += inc;
	*p = __cpu_to_le32(c);

	if ((c >= inc) && (c >= c1)) {
		return;
	}

	ctr_inc1(counter + sizeof(u32), len - sizeof(u32));
}

static void reverse_iv(u8 *dst, u8* src)
{
	int i = AES_BLOCK_SIZE;
	while (i--) {
		dst[i] = *(src++);
	}
}

static void dump_sglist(struct scatterlist *sglist, int count)
{
#if 0
	int i;
	struct scatterlist *sg;

	printk("sglist: %p\n", sglist);
	for_each_sg(sglist, sg, count, i) {
		printk("\t%08x (%08x)\n", sg_dma_address(sg), sg_dma_len(sg));
	}
#endif
}

static int sp_blk_aes_set_key(struct crypto_tfm *tfm,
	const u8 *in_key, unsigned int key_len)
{
	struct sp_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	SP_CRYPTO_TRACE();
#if 1 // TODO: dirty hack code, fix me
	if (key_len & 1) {
		key_len &= ~1;
		ctx->base.mode = M_AES_CTR | (32 << 24); // GCTR M = 32;
	}
#endif
	if (key_len != AES_KEYSIZE_128 && key_len != AES_KEYSIZE_192 && key_len != AES_KEYSIZE_256) {
		SP_CRYPTO_ERR("unsupported AES key length: %d\n", key_len);
		return -EINVAL;
	}

	ctx->keylen = key_len;
	ctx->base.mode |= (key_len / 4) << 16; // AESPAR0_NK
	memcpy(ctx->iv + ctx->ivlen, in_key, key_len); // key: iv + ivlen

	return 0;
}

static int sp_blk_aes_crypt(struct blkcipher_desc *desc,
	struct scatterlist *dst, struct scatterlist *src, u32 nbytes, u32 enc)
{
#define NO_WALK		0
#define SRC_WALK	1
#define DST_WALK	2
#define BOTH_WALK	3

	u32 flag = BOTH_WALK;
	int ret = 0;
	int src_cnt, dst_cnt;
	dma_addr_t src_phy, dst_phy;
	bool ioc;
	u32 processed = 0, process = 0;
	struct scatterlist *sp;
	struct scatterlist *dp;
	struct sp_aes_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	crypto_ctx_t *ctx0 = &ctx->base;
	trb_ring_t *ring = AES_RING(ctx0->dd);
	dma_addr_t iv_phy, key_phy, tmp_phy = 0;
	u32 mode = ctx0->mode & M_MMASK;
	u32 mm = ctx0->mode | enc; // AESPAR0 (trb.para.mode)

	//printk(">>>>> %08x <<<<<\n", mm);
	//dump_stack();
	if (mode == M_AES_CTR) { // CTR mode: reverse iv byte-order for HW
		reverse_iv(ctx->iv, desc->info);
	} else {
		memcpy(ctx->iv, desc->info, ctx->ivlen);
		if (mode == M_AES_CBC && enc == M_DEC) {
			scatterwalk_map_and_copy(desc->info, src,
				nbytes - ctx->ivlen, ctx->ivlen, 0);
		}
	}

	iv_phy = ctx->pa + AES_BLOCK_SIZE;
	key_phy = iv_phy + ctx->ivlen;
	sp = src;
	dp = dst;

	src_cnt = sg_nents(src);
	dst_cnt = sg_nents(dst);

	//dump_sglist(src, src_cnt);
	if(unlikely(dma_map_sg(NULL, src, src_cnt, DMA_TO_DEVICE) <= 0)) {
		SP_CRYPTO_ERR("sp aes map src sg  fail\n");
		return -EINVAL;
	}
	dump_sglist(src, src_cnt);

	//dump_sglist(dst, dst_cnt);
	if(unlikely(dma_map_sg(NULL, dst, dst_cnt, DMA_FROM_DEVICE) <= 0)) {
		SP_CRYPTO_ERR("sp aes map dst sg  fail\n");
		dma_unmap_sg(NULL, src,  src_cnt, DMA_TO_DEVICE);
		return -EINVAL;
	}
	dump_sglist(dst, dst_cnt);

	if (mutex_lock_interruptible(&ring->lock)) {
		dma_unmap_sg(NULL, src, src_cnt, DMA_TO_DEVICE);
		dma_unmap_sg(NULL, dst, dst_cnt, DMA_FROM_DEVICE);
		return -EINTR;
	}
	//mutex_lock(&ring->lock);

	while (processed < nbytes) {
		u32 reast;
		trb_t *trb;
		if (BOTH_WALK == flag) {
			src_phy = sg_dma_address(sp);
			dst_phy = sg_dma_address(dp);
			if (sg_dma_len(sp) > sg_dma_len(dp)) {
				process = sg_dma_len(dp);
				dp = sg_next(dp);
				flag = DST_WALK;
				SP_CRYPTO_TRACE();
			} else if(sg_dma_len(sp) == sg_dma_len(dp)) {
				process = sg_dma_len(sp);
				sp = sg_next(sp);
				dp = sg_next(dp);
				flag = BOTH_WALK;
				SP_CRYPTO_TRACE();
			} else {
				process = sg_dma_len(sp);
				sp = sg_next(sp);
				flag = SRC_WALK;
				SP_CRYPTO_TRACE();
			}
		} else if (DST_WALK == flag) {
			src_phy += process;
			dst_phy = sg_dma_address(dp);
			reast = sg_dma_len(sp) - (src_phy - sg_dma_address(sp));
			if(reast > sg_dma_len(dp)) {
				process = sg_dma_len(dp);
				dp = sg_next(dp);
				flag = DST_WALK;
			} else if(reast == sg_dma_len(dp)) {
				process = sg_dma_len(dp);
				dp = sg_next(dp);
				sp = sg_next(sp);
				flag = BOTH_WALK;
			} else {
				process = reast;
				sp = sg_next(sp);
				flag = SRC_WALK;
			}
		} else if (SRC_WALK == flag) {
			src_phy = sg_dma_address(sp);
			dst_phy += process;
			reast = sg_dma_len(dp) - (dst_phy - sg_dma_address(dp));
			if(reast > sg_dma_len(sp)) {
				process = sg_dma_len(sp);
				sp = sg_next(sp);
				flag = SRC_WALK;
			} else if(reast == sg_dma_len(sp)) {
				process = sg_dma_len(sp);
				dp = sg_next(dp);
				sp = sg_next(sp);
				flag = BOTH_WALK;
			} else {
				process = reast;
				dp = sg_next(dp);
				flag = DST_WALK;
			}
		} else {
			src_phy += process;
			dst_phy += process;
		}

		process = min_t(u32, process, nbytes - processed);
		if (process < AES_BLOCK_SIZE) {
			tmp_phy = ctx->pa;
		} else if (process % AES_BLOCK_SIZE) {
			process &= ~(AES_BLOCK_SIZE - 1);
			flag = NO_WALK;
		}

		processed += process;
		ioc = (processed == nbytes) || (ring->trb_sem.count == 1);

		SP_CRYPTO_TRACE();
		if (tmp_phy)
			trb = crypto_ctx_queue(ctx0, src_phy, tmp_phy, iv_phy, key_phy, AES_BLOCK_SIZE, mm, ioc);
		else
			trb = crypto_ctx_queue(ctx0, src_phy, dst_phy, iv_phy, key_phy, process, mm, ioc);
		if (!trb) {
			ret = -EINTR;
			goto out;
		}
		iv_phy = 0; // after iv inital value, using HW auto-iv

		if (ioc) {
			SP_CRYPTO_TRACE();
			ret = crypto_ctx_exec(ctx0);
			if (ret) {
				//if (ret == -ERESTARTSYS) ret = 0;
				goto out;
			}
		}
	};
	SP_CRYPTO_TRACE();
#if 1
	BUG_ON(processed != nbytes);
#else
	if (processed != nbytes) {
		SP_CRYPTO_ERR("proccessed %d bytes != need process %d bytes\n", processed, nbytes);
		ret = -EINVAL;
	}
#endif

out:
	mutex_unlock(&ring->lock);
	dma_unmap_sg(NULL, src, src_cnt, DMA_TO_DEVICE);
 	dma_unmap_sg(NULL, dst, dst_cnt, DMA_FROM_DEVICE);

	if (tmp_phy) {
		//dump_buf(ctx->tmp, AES_BLOCK_SIZE);
		scatterwalk_map_and_copy(ctx->tmp, dst, nbytes - process, process, 1);
	}

	// update iv for return
	if (mode == M_AES_CBC && enc == M_ENC) {
		scatterwalk_map_and_copy(desc->info, dst,
			nbytes - ctx->ivlen, ctx->ivlen, 0);
	} else if (mode == M_AES_CTR) {
		ctr_inc(ctx->iv, ctx->ivlen, nbytes / AES_BLOCK_SIZE);
		reverse_iv(desc->info, ctx->iv);
	}

	return ret;
}

static int sp_blk_aes_encrypt(struct blkcipher_desc *desc,
	struct scatterlist *dst, struct scatterlist *src, unsigned int nbytes)
{
	return sp_blk_aes_crypt(desc, dst, src, nbytes, M_ENC);
}

static int sp_blk_aes_decrypt(struct blkcipher_desc *desc,
	struct scatterlist *dst, struct scatterlist *src, unsigned int nbytes)
{
	return sp_blk_aes_crypt(desc, dst, src, nbytes, M_DEC);
}

/* tell the block cipher walk routines that this is a stream cipher by
 * setting cra_blocksize to 1. Even using blkcipher_walk_virt_block
 * during encrypt/decrypt doesn't solve this problem, because it calls
 * blkcipher_walk_done under the covers, which doesn't use walk->blocksize,
 * but instead uses this tfm->blocksize. */
struct crypto_alg sp_aes_alg[] = {

	{
		.cra_name		 = "ecb(aes)",
		.cra_driver_name = "sp-aes-ecb",
		.cra_priority	 = 300,
		.cra_flags		 = CRYPTO_ALG_TYPE_BLKCIPHER,
		.cra_blocksize	 = AES_BLOCK_SIZE,
		.cra_alignmask	 = 0xf,
		.cra_ctxsize	 = sizeof(struct sp_aes_ctx),
		.cra_type		 = &crypto_blkcipher_type,
		.cra_module 	 = THIS_MODULE,
		.cra_init		 = sp_cra_aes_ecb_init,
		.cra_exit		 = sp_cra_aes_exit,
		.cra_blkcipher = {
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize 	 = AES_BLOCK_SIZE,
			.setkey 	 = sp_blk_aes_set_key,
			.encrypt	 = sp_blk_aes_encrypt,
			.decrypt	 = sp_blk_aes_decrypt,
		}
	},
	{
		.cra_name		 = "cbc(aes)",
		.cra_driver_name = "sp-aes-cbc",
		.cra_priority	 = 300,
		.cra_flags		 = CRYPTO_ALG_TYPE_BLKCIPHER,
		.cra_blocksize	 = AES_BLOCK_SIZE,
		.cra_alignmask	 = 0xf,
		.cra_ctxsize	 = sizeof(struct sp_aes_ctx),
		.cra_type		 = &crypto_blkcipher_type,
		.cra_module 	 = THIS_MODULE,
		.cra_init		 = sp_cra_aes_cbc_init,
		.cra_exit		 = sp_cra_aes_exit,
		.cra_blkcipher = {
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize 	 = AES_BLOCK_SIZE,
			.setkey 	 = sp_blk_aes_set_key,
			.encrypt	 = sp_blk_aes_encrypt,
			.decrypt	 = sp_blk_aes_decrypt,
		}
	},
	{
		.cra_name		 = "ctr(aes)",
		.cra_driver_name = "sp-aes-ctr",
		.cra_priority	 = 300,
		.cra_flags		 = CRYPTO_ALG_TYPE_BLKCIPHER,
		.cra_blocksize	 = 1, // TODO: AES_BLOCK_SIZE ???
		.cra_alignmask	 = 0xf,
		.cra_ctxsize	 = sizeof(struct sp_aes_ctx),
		.cra_type		 = &crypto_blkcipher_type,
		.cra_module 	 = THIS_MODULE,
		.cra_init		 = sp_cra_aes_ctr_init,
		.cra_exit		 = sp_cra_aes_exit,
		.cra_blkcipher = {
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize 	 = AES_BLOCK_SIZE,
			.setkey 	 = sp_blk_aes_set_key,
			.encrypt	 = sp_blk_aes_encrypt,
			.decrypt	 = sp_blk_aes_decrypt,
		}

	},
};

int sp_aes_finit(void)
{
	SP_CRYPTO_TRACE();
	return crypto_unregister_algs(sp_aes_alg, ARRAY_SIZE(sp_aes_alg));
}
EXPORT_SYMBOL(sp_aes_finit);

int sp_aes_init(void)
{
	SP_CRYPTO_TRACE();
	return  crypto_register_algs(sp_aes_alg, ARRAY_SIZE(sp_aes_alg));
}
EXPORT_SYMBOL(sp_aes_init);

void sp_aes_irq(void *devid, u32 flag)
{
	struct sp_crypto_dev *dev = devid;
	trb_ring_t *ring = AES_RING(dev);

#ifdef TRACE_WAIT_ORDER
	SP_CRYPTO_INF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s:%08x %08x %08x %d\n", __FUNCTION__,
		flag, dev->reg->AES_ER, dev->reg->AESDMA_CRCR, ring->trb_sem.count/*kfifo_len(&ring->f)*/);
#else
	SP_CRYPTO_INF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s:%08x %08x %08x\n", __FUNCTION__, flag, dev->reg->AES_ER, dev->reg->AESDMA_CRCR);
#endif
	if (flag & AES_TRB_IF) { // autodma/trb done
		trb_t *trb = ring->head;
		ring->irq_count++;
		if (!trb->cc) dump_trb(trb);
		while (trb->cc) {
			//dump_trb(trb);
			if (trb->ioc) {
				crypto_ctx_t *ctx = trb->priv;
				if (ctx->type == SP_CRYPTO_AES) {
#ifdef TRACE_WAIT_ORDER
					wait_queue_head_t *w;
					BUG_ON(!kfifo_get(&ring->f, &w));
					BUG_ON(w != &ctx->wait);
#endif
					ctx->done = true;
					wake_up(&ctx->wait);
				}
				else {
					printk("\nAES_SKIP: %08x\n", ctx->type);
					dump_trb(trb);
				}
			}
			trb = trb_put(ring);
		}
	}
#ifdef USE_ERF
	if (flag & AES_ERF_IF) { // event ring full
		SP_CRYPTO_ERR("\n!!! %08x %08x\n", dev->reg->AESDMA_ERBAR, dev->reg->AESDMA_ERDPR);
		dev->reg->AESDMA_RCSR |= AUTODMA_RCSR_ERF; // clear event ring full
	}
#endif
	SP_CRYPTO_INF("\n");
}
EXPORT_SYMBOL(sp_aes_irq);

