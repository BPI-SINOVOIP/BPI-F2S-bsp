/**
 * @file    sp_ipc.h
 * @brief   Declaration of Sunplus IPC Linux Driver.
 * @author  qinjian
 */
#ifndef _SP_IPC_H_
#define _SP_IPC_H_

#define TRACE
//#define MEM_DEBUG
#define RPC_DUMP
//#define IPC_USE_CBDMA

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* IPC Error Code */
#define IPC_SUCCESS			(0)
#define IPC_FAIL			(-1)
#define IPC_FAIL_NODEV		(-2)	// ipc device not exist
#define IPC_FAIL_NOMEM		(-3)	// out of memory
#define IPC_FAIL_INVALID	(-4)	// invalid arguments
#define IPC_FAIL_UNSUPPORT	(-5)	// rpc cmd not supported
#define IPC_FAIL_NOSERV		(-6)	// rpc server not running
#define IPC_FAIL_BUSY		(-7)	// rpc server busy
#define IPC_FAIL_TIMEOUT	(-8)	// rpc request timeout
#define IPC_FAIL_SERVNOTRDY (-9)    // rpc server  not ready
#define IPC_FAIL_DATANOTRDY (-10)   // rpc remote  data not ready
#define IPC_FAIL_HWTIMEOUT  (-11)   // rpc hw timeout

/* RPC Request Type */
enum {
	REQ_WAIT_REP = 0,		// Wait Response
	REQ_DEFER_REP,			// Deferred Response
	REQ_NO_REP,				// No Response
};

/* RPC Direction */
enum {
	RPC_REQUEST = 0,
	RPC_RESPONSE,
};

#define IPC_DATA_SIZE_MAX	4096
#define IPC_WRITE_TIMEOUT	500			// ms

#define SERVER_BITS			3	// highest N bits in CMD
#define SERVER_NUMS			(1 << SERVER_BITS)
#define SERVER_ID_OFFSET	(10 - SERVER_BITS)
#define IPC_SERVER_ID_MASK  ((1 << SERVER_BITS) - 1)
#define RPC_CMD_MAX			((1 << SERVER_ID_OFFSET) - 1)
#ifdef IPC_USE_CBDMA
#define RPC_DATA_REGS		18
#else
#define RPC_DATA_REGS		16
#endif
#define RPC_DATA_SIZE		(RPC_DATA_REGS * 4)
#define RPC_HEAD_SIZE		(sizeof(rpc_t) - RPC_DATA_SIZE)

#define MAILBOX_NUM			8

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

#ifndef u8
#define u8		unsigned char
#define u16		unsigned short
#define u32		unsigned int
#endif

typedef struct {
	void*		REQ_H;					// Request Handler
	u16 		DATA_LEN;				// In/Out Data Length in Bytes
	u16			CMD 	: 10;			// Command ID / Return Value
	u16 		RSV 	: 3;
	u16 		F_TYPE	: 2;			// Request Type
	u16 		F_DIR	: 1;			// REQUEST / RESPONSE
#ifndef IPC_USE_CBDMA
	void*		SEQ_ADDR;
	u32 		SEQ;
#endif
	union {
		u32		DATA[RPC_DATA_REGS];	// if DATA_LEN <= RPC_DATA_SIZE
		struct {						// if DATA_LEN >  RPC_DATA_SIZE
			void*	DATA_PTR;			// Cache Aligned
			void*	DATA_PTR_ORG;		// Backup (INTERNAL_USE)
		};
	};
} rpc_t;

typedef struct {
	u32 timeout;
}rpc_user_t;

typedef struct {
	rpc_t rpc;
	rpc_user_t user;
}rpc_new_t;

typedef struct {
	u32			TRIGGER;				// Reg00
	u32			F_RW;					// Reg01
	u32			F_OVERWRITE;			// Reg02
	u32			RSV;					// Reg03

	rpc_t		RPC;					// Reg04~23

	u32			MBOX[MAILBOX_NUM];		// Reg24~31
} ipc_t;

typedef int (*ipc_func)(void *data);
typedef void (*ipc_mbfunc)(int id, u32 data);

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifndef BIT
#define BIT(x)  (1 << (x))
#endif
#ifdef TRACE
#define print		printf
#else
#define print(...)
#endif
#define trace()		print("++++ %s(%d) ++++\n", __FUNCTION__, __LINE__)

#ifdef MEM_DEBUG
#define MALLOC(n) \
({ \
	void *_p = malloc(n); \
	printf("++++++++++ %p %d\n", _p, n); \
	_p; \
})
#define FREE(p) \
do { \
	free(p); \
	printf("---------- %p\n", p); \
} while (0)
#else
#define MALLOC	malloc
#define FREE	free
#endif

#define hex_dump(p, l) \
do { \
	static char _s[] = "       |       \n"; \
	char ss[52] = ""; \
	u8 *_p = (u8 *)(p); \
	int _l = (l); \
	int _i = 0, _j; \
	while (_i < _l) { \
		_j = _i & 0x0F; \
		sprintf(ss + _j * 3, "%02x%c", _p[_i], _s[_j]); \
		_i++; \
		if (!(_i & 0x0F)) printf(ss); \
	} \
	if (_l & 0x0F) printf(ss); \
} while (0)

#define var_dump(v) \
do { \
	printf("%s(%d) %p:\n", __FUNCTION__, __LINE__, &(v)); \
	hex_dump(&(v), sizeof(v)); \
} while (0)

#define _rpc_dump(ss, rr) \
do { \
	rpc_t *r = (rpc_t *)(rr); \
	int l; \
	printf("%s: %s %u\n", ss, (r->F_DIR)?"RES":"REQ", r->CMD); \
	l = r->DATA_LEN; \
	hex_dump(r, (l > RPC_DATA_SIZE) ? 24 : (16 + l)); \
	if (l > RPC_DATA_SIZE) hex_dump(r->DATA_PTR, l + 4); \
	printf("\n"); \
} while (0)

#ifdef RPC_DUMP
#define rpc_dump	_rpc_dump
#else
#define rpc_dump(ss, rr)
#endif


#define CACHE_MASK			(32 - 1)
#define CACHE_ALIGNED(n)	(((u32)(n) & CACHE_MASK) == 0)
#define CACHE_ALIGN(n)		(((u32)(n) + CACHE_MASK) & (~CACHE_MASK))

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

int IPC_RegFunc(int cmd, ipc_func pfIPCHandler);
int IPC_UnregFunc(int cmd);
int IPC_Init(int server_id);
void IPC_Finalize(void);

/**
 * @brief   Sunplus RPC call.
 * @param   cmd [in] request command id.
 * @param   data [in] input data buffer.
 * @param	len [in] input data length in bytes.
 * @return  success: 0 (IPC_SUCCESS), fail: error code (IPC_FAIL_XXXX)
 */
int IPC_FunctionCall(int cmd, void *data, int len);

/**
 * @brief   Sunplus RPC call.
 * @param   cmd [in] request command id.
 * @param   data [in] input data buffer.
 * @param	len [in] input data length in bytes.
 * @param	timeout [in] input rpc timeout in ms.
 * @return  success: 0 (IPC_SUCCESS), fail: error code (IPC_FAIL_XXXX)
 */
int IPC_FunctionCall_timeout(int cmd, void *data, int len, u32 timeout);

#endif /* _SP_IPC_H_ */
