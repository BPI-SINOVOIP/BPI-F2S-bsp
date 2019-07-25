#ifndef __SP_AES_H__
#define __SP_AES_H__

int sp_aes_finit(void);
int sp_aes_init(void);
void sp_aes_irq(void *devid, u32 flag);

#endif /*  */
