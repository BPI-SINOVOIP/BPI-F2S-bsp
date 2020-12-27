#ifndef __SP_RSA_H__
#define __SP_RSA_H__
#include <linux/mpi.h>


//#define RSA_DATA_BIGENDBIAN


#define RSA_BASE   64
#define RSA_BASE_MASK ((1 << (RSA_BASE - 2)) - 1)

#if (RSA_BASE == 32)
#define SP_RSA_LB   5
typedef s32   rsabase_t;

#define sp_rsabase_le_to_cpu __le32_to_cpu
#define sp_rsabase_be_to_cpu __be32_to_cpu

#elif (RSA_BASE == 64)
#define SP_RSA_LB   6
typedef s64   rsabase_t;

#define sp_rsabase_le_to_cpu __le64_to_cpu
#define sp_rsabase_be_to_cpu __be64_to_cpu
#endif

typedef struct rsa_para {
	__u8	*crp_p;
	__u32	crp_bytes;
}rsa_para;

#define BITS2BYTES(bits)	((bits)/(BITS_PER_BYTE))
#define BITS2LONGS(bits)	((bits)/(BITS_PER_LONG))

int sp_powm(rsa_para *res, rsa_para *base, rsa_para *exp, rsa_para *mod);
void sp_rsa_finit(void);
int sp_rsa_init(void);
void sp_rsa_irq(void *devid, u32 flag);
int mont_p2(rsa_para *mod,  rsa_para *p2);
rsabase_t mont_w(rsa_para *mod);
int sp_mpi_set_buffer(MPI a, const void *xbuffer, unsigned nbytes, int sign);
#endif //__SP_RSA_H__
