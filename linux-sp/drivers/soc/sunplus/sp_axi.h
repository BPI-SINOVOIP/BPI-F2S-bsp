#ifndef __SP_IOP_H__
#define __SP_IOP_H__
#ifdef CONFIG_SOC_SP7021
#include <mach/io_map.h>
#endif
typedef enum IOP_Status_e_ {
	IOP_SUCCESS,                /* successful */
	IOP_ERR_IOP_BUSY,           /* IOP is busy */
} IOP_Status_e;


#endif /* __SP_IOP_H__ */
