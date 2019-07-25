#ifndef __SP_HASH_H__
#define __SP_HASH_H__

int sp_hash_finit(void);
int sp_hash_init(void);
void sp_hash_irq(void *devid, u32 flag);

#endif //__SP_HASH_H__
