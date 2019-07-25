#include <crypto/algapi.h>
#include <linux/cryptohash.h>
#include <crypto/internal/hash.h>
#include <crypto/hash.h>
#include <crypto/md5.h>
#include <crypto/scatterwalk.h>
#include <crypto/sha.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/crypto.h>
#include <linux/kfifo.h>
#include <linux/delay.h>

#include "sp-crypto.h"
#include "sp-hash.h"

#define SHA3_224_DIGEST_SIZE	(224 / 8)
#define SHA3_224_BLOCK_SIZE		(200 - 2 * SHA3_224_DIGEST_SIZE)
#define SHA3_256_DIGEST_SIZE	(256 / 8)
#define SHA3_256_BLOCK_SIZE		(200 - 2 * SHA3_256_DIGEST_SIZE)
#define SHA3_384_DIGEST_SIZE	(384 / 8)
#define SHA3_384_BLOCK_SIZE		(200 - 2 * SHA3_384_DIGEST_SIZE)
#define SHA3_512_DIGEST_SIZE	(512 / 8)
#define SHA3_512_BLOCK_SIZE		(200 - 2 * SHA3_512_DIGEST_SIZE)

#define SHA3_BUF_SIZE			(200)

#define GHASH_BLOCK_SIZE		(16)
#define GHASH_DIGEST_SIZE		(16)
#define GHASH_KEY_SIZE			(16)

#define WORKBUF_SIZE			(65536)

struct sp_hash_ctx {
	crypto_ctx_t base;

	// working buffer: blocks(src) + key(ghash only) + buf(digest)

	u8 *blocks;
	dma_addr_t blocks_phy;
	u32 blocks_size;
	u32 bytes; // bytes in blocks

	// ghash only
	u8 *key;
	dma_addr_t key_phy;

	u8 *buf;
	dma_addr_t buf_phy;

	u32 alg_type;
	u32 digest_len;
	u32 block_size;

	u64 byte_count;	// for MD5 padding
};

static int do_blocks(struct sp_hash_ctx *ctx, u32 len)
{
	int ret = 0;
	crypto_ctx_t *ctx0 = &ctx->base;
	trb_t *trb;

	SP_CRYPTO_TRACE();
	//dump_buf(ctx->blocks, len);

	trb = crypto_ctx_queue(ctx0,
		ctx->blocks_phy,	// src
		ctx->buf_phy,		// dst
		ctx->buf_phy,		// iv
		ctx->key_phy,		// key
		len, ctx0->mode, true);
	if (!trb) return -EINTR;

	SP_CRYPTO_TRACE();
	ret = crypto_ctx_exec(ctx0);
	//if (ret == -ERESTARTSYS) ret = 0;

	return ret;
}

static int sp_cra_init(struct crypto_tfm *tfm, u32 mode)
{
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(tfm);
	u32 bsize, len1, len2;

	SP_CRYPTO_TRACE();

	ctx->blocks = dma_alloc_coherent(NULL, WORKBUF_SIZE, &ctx->blocks_phy, GFP_KERNEL);
	if (!ctx->blocks)
		return -ENOMEM;

	ctx->base.mode = mode;
	ctx->alg_type = crypto_tfm_alg_type(tfm);
	ctx->block_size = bsize = crypto_tfm_alg_blocksize(tfm);

	switch (mode) {
	case M_MD5:
		len1 = MD5_DIGEST_SIZE;
		len2 = bsize;
		break;
	case M_GHASH:
		len1 = GHASH_DIGEST_SIZE;
		len2 = GHASH_KEY_SIZE;
		break;
	default: // M_SHA3
		len1 = SHA3_BUF_SIZE;
		len2 = bsize;
		break;
	}
	len1 = WORKBUF_SIZE - len1;
	len2 = len1 - len2;
	ctx->blocks_size = len2 / bsize * bsize;

	ctx->buf = ctx->blocks + len1;
	ctx->buf_phy = ctx->blocks_phy + len1;
	ctx->key = ctx->blocks + len2;
	ctx->key_phy = ctx->blocks_phy + len2;

	return crypto_ctx_init(&ctx->base, SP_CRYPTO_HASH);
}

static int sp_cra_md5_init(struct crypto_tfm *tfm)
{
	return sp_cra_init(tfm, M_MD5);
}

static int sp_cra_ghash_init(struct crypto_tfm *tfm)
{
	return sp_cra_init(tfm, M_GHASH);
}

static int sp_cra_sha3_224_init(struct crypto_tfm *tfm)
{
	return sp_cra_init(tfm, M_SHA3_224);
}

static int sp_cra_sha3_256_init(struct crypto_tfm *tfm)
{
	return sp_cra_init(tfm, M_SHA3_256);
}

static int sp_cra_sha3_384_init(struct crypto_tfm *tfm)
{
	return sp_cra_init(tfm, M_SHA3_384);
}

static int sp_cra_sha3_512_init(struct crypto_tfm *tfm)
{
	return sp_cra_init(tfm, M_SHA3_512);
}

static void sp_cra_exit(struct crypto_tfm *tfm)
{
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(tfm);

	SP_CRYPTO_TRACE();
	dma_free_coherent(NULL, WORKBUF_SIZE, ctx->blocks, ctx->blocks_phy);
	crypto_ctx_exit(&ctx->base);
}

static int sp_shash_export(struct shash_desc *desc, void *out)
{
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(&desc->tfm->base);

	memcpy(out, ctx->buf, ctx->digest_len);
	//printk("%s: ", __FUNCTION__); dump_buf(out, ctx->digest_len);

	return 0;
}

static int sp_shash_import(struct shash_desc *desc, const void *in)
{
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(&desc->tfm->base);

	memcpy(ctx->buf, in, ctx->digest_len);

    return 0;
}

static int sp_shash_update(struct shash_desc *desc, const u8 *data,
		  u32 len)
{
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(&desc->tfm->base);
	const u32 bsize = ctx->blocks_size;
	u32 avail = bsize - ctx->bytes; // free bytes in blocks
	int ret = 0;

	//SP_CRYPTO_TRACE();

	ctx->byte_count += len;

	if (avail >= len) {
		memcpy(ctx->blocks + ctx->bytes, data, len); // append to blocks
	} else {
		do {
			memcpy(ctx->blocks + ctx->bytes, data, avail); // fill blocks
			ret = do_blocks(ctx, bsize);
			if (ret) goto out;
			
			data += avail;
			len -= avail;

			ctx->bytes = 0;
			avail = bsize;
		} while (len > bsize);

		memcpy(ctx->blocks, data, len); // saved to blocks
	}
	ctx->bytes += len;

out:
	return ret;
}

static int sp_shash_final(struct shash_desc *desc, u8 *out)
{
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(&desc->tfm->base);
	const u32 bsize = ctx->block_size;
	u32 t = ctx->bytes % bsize;
	u8 *p = ctx->blocks + ctx->bytes;
	int padding; // padding zero bytes
	int ret = 0;

	SP_CRYPTO_TRACE();

	// padding
	switch (ctx->base.mode) {
	case M_MD5:
		padding = (bsize * 2 - t - 1 - sizeof(u64)) % bsize;
		*p++ = 0x80;
		break;
	case M_GHASH:
		padding = t ? (bsize - t) : 0;
		break;
	default: // SHA3
		padding = bsize - t - 1;
		*p++ = 0x06;
		break;
	}

	memset(p, 0, padding); // padding zero
	p += padding;

	switch (ctx->base.mode) {
	case M_MD5:
		((u32 *)p)[0] = ctx->byte_count << 3;
		((u32 *)p)[1] = ctx->byte_count >> 29;
		p += sizeof(u64);
		break;
	case M_GHASH:
		break;
	default: // SHA3
		*(p - 1) |= 0x80;
		break;
	}

	// process blocks
	//printk("%s: ", __FUNCTION__); dump_buf(ctx->block, p - ctx->block);
	ctx->base.mode |= M_FINAL;
	ret = do_blocks(ctx, p - ctx->blocks);
	ctx->base.mode &= ~M_FINAL;

	mutex_unlock(&HASH_RING(ctx->base.dd)->lock);
	if (!ret) {
		sp_shash_export(desc, out);
	}

	return ret;
}

static int sp_shash_ghash_setkey(struct crypto_shash *tfm,
			const u8 *key, unsigned int keylen)
{

	struct sp_hash_ctx *ctx = crypto_tfm_ctx(&tfm->base);

	SP_CRYPTO_TRACE();
#if 1 // TODO: dirty hack code, fix me
	if (keylen & 1) {
		// called from crypto_gcm_setkey (gcm.c)
		keylen &= ~1;
		ctx->digest_len = GHASH_DIGEST_SIZE;
	}
#endif
	//dump_stack();
	//dump_buf(key, keylen);
	if (keylen != GHASH_KEY_SIZE) {
		crypto_shash_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		SP_CRYPTO_ERR("unsupported GHASH key length: %d\n", keylen);
		return -EINVAL;
	}

	memcpy(ctx->key, key, keylen);

	return 0;
}

static int sp_shash_init(struct shash_desc *desc)
{
	struct sp_hash_ctx *ctx = crypto_tfm_ctx(&desc->tfm->base);
	crypto_ctx_t *ctx0 = &ctx->base;

	SP_CRYPTO_TRACE();
	//printk("!!! %s: %d\n", __FUNCTION__, ctx->digest_len);
	//dump_stack();
	if (!ctx->digest_len) {
		// called from crypto_create_session (CIOCGSESSION)
		ctx->digest_len = crypto_shash_alg(desc->tfm)->digestsize;
	} else	{
		// called from crypto_run (CIOCCRYPT) or gcm_hash (gcm.c)
		u32 l = (ctx0->mode & M_SHA3) ? SHA3_BUF_SIZE : ctx->digest_len;

		ctx->byte_count = 0;
		ctx->bytes = 0;

		if (ctx0->mode == M_MD5) {
			((u32 *)ctx->buf)[0] = 0x67452301;
			((u32 *)ctx->buf)[1] = 0xefcdab89;
			((u32 *)ctx->buf)[2] = 0x98badcfe;
			((u32 *)ctx->buf)[3] = 0x10325476;
		} else {
			memset(ctx->buf, 0, l);
		}

		// can't do mutex_lock in hash_update, we must lock it @ here
		SP_CRYPTO_TRACE();
		mutex_lock(&HASH_RING(ctx0->dd)->lock);
	}

	//SP_CRYPTO_INF("sp_shash_init: %08x %d %d --- %p %p %p\n", ctx->base.mode, ctx->digest_len, ctx->block_size, ctx->buf, ctx->block, ctx->base.mode == M_GHASH ? ctx->key : NULL);

	return 0;
}

static struct shash_alg sp_shash_alg[] =
{
	{
		.digestsize	=	MD5_DIGEST_SIZE,
		.init		=	sp_shash_init,
		.update		=	sp_shash_update,
		.final		=	sp_shash_final,
		.export		=	sp_shash_export,
		.import		=	sp_shash_import,
		.base		=	{
			.cra_name		=	"md5",
			.cra_driver_name = 	"sp-md5",
			.cra_flags		=	CRYPTO_ALG_TYPE_SHASH,
			.cra_blocksize	=	MD5_HMAC_BLOCK_SIZE,
			.cra_module		=	THIS_MODULE,
			.cra_priority	= 	300,
			.cra_ctxsize	= 	sizeof(struct sp_hash_ctx),
			.cra_alignmask	= 	0,
			.cra_init		= 	sp_cra_md5_init,
			.cra_exit		= 	sp_cra_exit,
		},
	},
	{
		.digestsize =	GHASH_DIGEST_SIZE,
		.init		=	sp_shash_init,
		.setkey 	=	sp_shash_ghash_setkey,
		.update 	=	sp_shash_update,
		.final		=	sp_shash_final,
		.base		=	{
			.cra_name		=	"ghash",
			.cra_driver_name =  "sp-ghash",
			.cra_flags		=	CRYPTO_ALG_TYPE_SHASH,
			.cra_blocksize	=	GHASH_BLOCK_SIZE,
			.cra_module 	=	THIS_MODULE,
			.cra_priority	= 	300,
			.cra_ctxsize	=	sizeof(struct sp_hash_ctx),
			.cra_alignmask	=	0,
			.cra_init		=	sp_cra_ghash_init,
			.cra_exit		=	sp_cra_exit,
		},
	},
	{
		.digestsize =	SHA3_224_DIGEST_SIZE,
		.init		=	sp_shash_init,
		.update 	=	sp_shash_update,
		.final		=	sp_shash_final,
		.export 	=	sp_shash_export,
		.import 	=	sp_shash_import,
		.base		=	{
			.cra_name		=	"sha3-224",
			.cra_driver_name =  "sp-sha3-224",
			.cra_flags		=	CRYPTO_ALG_TYPE_SHASH,
			.cra_blocksize	=	SHA3_224_BLOCK_SIZE,
			.cra_module 	=	THIS_MODULE,
			.cra_priority	=	300,
			.cra_ctxsize	=	sizeof(struct sp_hash_ctx),
			.cra_alignmask	=	0,
			.cra_init		=	sp_cra_sha3_224_init,
			.cra_exit		=	sp_cra_exit,
		},
	},
	{
		.digestsize =	SHA3_256_DIGEST_SIZE,
		.init		=	sp_shash_init,
		.update 	=	sp_shash_update,
		.final		=	sp_shash_final,
		.export		=	sp_shash_export,
		.import		=	sp_shash_import,
		.base		=	{
			.cra_name		=	"sha3-256",
			.cra_driver_name =  "sp-sha3-256",
			.cra_flags		=	CRYPTO_ALG_TYPE_SHASH,
			.cra_blocksize	=	SHA3_256_BLOCK_SIZE,
			.cra_module 	=	THIS_MODULE,
			.cra_priority	=	300,
			.cra_ctxsize	=	sizeof(struct sp_hash_ctx),
			.cra_alignmask	=	0,
			.cra_init		=	sp_cra_sha3_256_init,
			.cra_exit		=	sp_cra_exit,
		},
	},
	{
		.digestsize =	SHA3_384_DIGEST_SIZE,
		.init		=	sp_shash_init,
		.update 	=	sp_shash_update,
		.final		=	sp_shash_final,
		.export		=	sp_shash_export,
		.import		=	sp_shash_import,
		.base		=	{
			.cra_name		=	"sha3-384",
			.cra_driver_name =  "sp-sha3-384",
			.cra_flags		=	CRYPTO_ALG_TYPE_SHASH,
			.cra_blocksize	=	SHA3_384_BLOCK_SIZE,
			.cra_module 	=	THIS_MODULE,
			.cra_priority	=	300,
			.cra_ctxsize	=	sizeof(struct sp_hash_ctx),
			.cra_alignmask	=	0,
			.cra_init		=	sp_cra_sha3_384_init,
			.cra_exit		=	sp_cra_exit,
		},
	},
	{
		.digestsize =	SHA3_512_DIGEST_SIZE,
		.init		=	sp_shash_init,
		.update 	=	sp_shash_update,
		.final		=	sp_shash_final,
		.export		=	sp_shash_export,
		.import		=	sp_shash_import,
		.base		=	{
			.cra_name		=	"sha3-512",
			.cra_driver_name =  "sp-sha3-512",
			.cra_flags		=	CRYPTO_ALG_TYPE_SHASH,
			.cra_blocksize	=	SHA3_512_BLOCK_SIZE,
			.cra_module 	=	THIS_MODULE,
			.cra_priority	=	300,
			.cra_ctxsize	=	sizeof(struct sp_hash_ctx),
			.cra_alignmask	=	0,
			.cra_init		=	sp_cra_sha3_512_init,
			.cra_exit		=	sp_cra_exit,
		},
	},
};


int sp_hash_finit(void)
{
	SP_CRYPTO_TRACE();
	return crypto_unregister_shashes(sp_shash_alg, ARRAY_SIZE(sp_shash_alg));
}
EXPORT_SYMBOL(sp_hash_finit);


int sp_hash_init(void)
{
	SP_CRYPTO_TRACE();
	return crypto_register_shashes(sp_shash_alg, ARRAY_SIZE(sp_shash_alg));
}
EXPORT_SYMBOL(sp_hash_init);

void sp_hash_irq(void *devid, u32 flag)
{
	struct sp_crypto_dev *dev = devid;
	trb_ring_t *ring = HASH_RING(dev);

#ifdef TRACE_WAIT_ORDER
	SP_CRYPTO_INF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s:%08x %08x %08x %d\n", __FUNCTION__, flag, dev->reg->HASH_ER, dev->reg->HASHDMA_CRCR, kfifo_len(&ring->f));
#else
	SP_CRYPTO_INF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s:%08x %08x %08x\n", __FUNCTION__, flag, dev->reg->HASH_ER, dev->reg->HASHDMA_CRCR);
#endif
	if (flag & HASH_TRB_IF) { // autodma/trb done
		trb_t *trb = ring->head;
		ring->irq_count++;
		if (!trb->cc) dump_trb(trb);
		while (trb->cc) {
			//dump_trb(trb);
			if (trb->ioc) {
				crypto_ctx_t *ctx = trb->priv;
				if (ctx->type == SP_CRYPTO_HASH) {
#ifdef TRACE_WAIT_ORDER
					wait_queue_head_t *w;
					BUG_ON(!kfifo_get(&ring->f, &w));
					BUG_ON(w != &ctx->wait);
#endif
					ctx->done = true;
					if (ctx->mode & M_FINAL) wake_up(&ctx->wait);
				}
				else printk("HASH_SKIP\n");
			}
			trb = trb_put(ring);
		}
	}
#ifdef USE_ERF
	if (flag & HASH_ERF_IF) { // event ring full
		SP_CRYPTO_ERR("\n!!! %08x %08x\n", dev->reg->HASHDMA_ERBAR, dev->reg->HASHDMA_ERDPR);
		dev->reg->HASHDMA_RCSR |= AUTODMA_RCSR_ERF; // clear event ring full
	}
#endif
	SP_CRYPTO_INF("\n");
}
EXPORT_SYMBOL(sp_hash_irq);

