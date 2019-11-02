/*
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>

// #define FOR_ZEBU_CSIM

#define SIZE_SKIP_TEST		(16 << 20)
#define SIZE_TEST		(CONFIG_SYS_SDRAM_SIZE - SIZE_SKIP_TEST)

struct cbdma_reg {
	volatile unsigned int hw_ver;
	volatile unsigned int config;
	volatile unsigned int length;
	volatile unsigned int src_adr;
	volatile unsigned int des_adr;
	volatile unsigned int int_flag;
	volatile unsigned int int_en;
	volatile unsigned int memset_val;
	volatile unsigned int sdram_size_config;
	volatile unsigned int illegle_record;
	volatile unsigned int sg_idx;
	volatile unsigned int sg_cfg;
	volatile unsigned int sg_length;
	volatile unsigned int sg_src_adr;
	volatile unsigned int sg_des_adr;
	volatile unsigned int sg_memset_val;
	volatile unsigned int sg_en_go;
	volatile unsigned int sg_lp_mode;
	volatile unsigned int sg_lp_sram_start;
	volatile unsigned int sg_lp_sram_size;
	volatile unsigned int sg_chk_mode;
	volatile unsigned int sg_chk_sum;
	volatile unsigned int sg_chk_xor;
	volatile unsigned int rsv_23_31[9];
};

#define CBDMA0_REG_BASE		0x9C000D00

#define PATTERN4TEST(X)		((((u32)(X)) << 24) | (((u32)(X)) << 16) | (((u32)(X)) << 8) | (((u32)(X)) << 0))

#define CBDMA_CONFIG_DEFAULT	0x00030000
#define CBDMA_CONFIG_GO		(0x01 << 8)
#define CBDMA_CONFIG_MEMSET	(0x00 << 0)
#define CBDMA_CONFIG_WR		(0x01 << 0)
#define CBDMA_CONFIG_RD		(0x02 << 0)
#define CBDMA_CONFIG_CP		(0x03 << 0)

#define CBDMA_INT_FLAG_DONE	(1 << 0)

#ifndef wmb
// #define wmb()		asm volatile (""   : : : "memory")
#define wmb()		dmb()
#endif

static int dram_tst_simple_rw(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned int addr, data;
	unsigned int *ptr;
	unsigned int loop_cnt;

	printf("Test area: 0x00000000 - 0x%x\n", (SIZE_TEST - 1));

	for (loop_cnt = 0; ; loop_cnt++) {
		printf("\n---------- loop_cnt: %u ----------\n", loop_cnt);
		ptr = (unsigned int *)(0x00000000);
		for (addr = 0; addr < SIZE_TEST;) {
			data = (loop_cnt & 0x00000001) ? (~addr) : addr;
			*ptr = data;
			ptr++;
			addr += 4;

			if ((addr & (SIZE_SKIP_TEST - 1)) == 0) {
				printf("%u MB written.\n", (addr >> 20));
			}
		}
		ptr = (unsigned int *)(0x00000000);
		for (addr = 0; addr < SIZE_TEST;) {
			data = (loop_cnt & 0x00000001) ? (~addr) : addr;
			if (*ptr != data) {
				printf("Error @ 0x%x, 0x%x != 0x%x (expected)\n", addr, *ptr, data);
				return -1;
			}
			ptr++;
			addr += 4;

			if ((addr & (SIZE_SKIP_TEST - 1)) == 0) {
				printf("%u MB Verified.\n", (addr >> 20));
			}
		}
	}

	return 0;
}

#ifdef FOR_ZEBU_CSIM
#define MAX_SIZE_CBDMA		((8 << 20) - 1)		/* Zebu has only 64 MB memory */
#else
#define MAX_SIZE_CBDMA		((1 << 25) - 1)		/* max. 25 bits */
#endif

#define ADDR_DST		(SIZE_TEST - (MAX_SIZE_CBDMA + 1))
#define ALIGN_1M_ROUND_DOWN(X)	((X >> 20) << 20)

#ifdef FOR_ZEBU_CSIM
#define LOOP_CNT		1
#else
#define LOOP_CNT		100
#endif

#ifdef FOR_ZEBU_CSIM
extern unsigned long long get_ticks(void);	/* sp_timer.c */
#endif
static int dram_tst_cbdma(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	volatile struct cbdma_reg *cbdma_ptr = (volatile struct cbdma_reg *)(CBDMA0_REG_BASE);
	const unsigned int test_length = ALIGN_1M_ROUND_DOWN(MAX_SIZE_CBDMA);
	ulong exe_time;
	int i, j;
	uint64_t bw;
	char cmd[256];
	int ret;
#ifdef FOR_ZEBU_CSIM
	unsigned long long tick[2];
	unsigned long long exe_usec;
#endif

	for (i = 0; i < 10; i++) {
		invalidate_dcache_range(0, (MAX_SIZE_CBDMA + 1));

		cbdma_ptr->int_en = 0;
		cbdma_ptr->length = test_length;
		cbdma_ptr->src_adr = 0;
		cbdma_ptr->des_adr = 0;
		cbdma_ptr->memset_val = PATTERN4TEST(i);
		wmb();
#ifdef FOR_ZEBU_CSIM
		tick[0] = get_ticks();
#else
		exe_time = get_timer(0);
#endif
		for (j = 0; j < LOOP_CNT; j++) {
			cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_MEMSET;
			wmb();
			while (1) {
				if (cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				break;
			}
		}
#ifdef FOR_ZEBU_CSIM
		tick[1] = get_ticks();
		exe_usec = tick[1] - tick[0];
		bw = ((uint64_t)(test_length)) / ((uint64_t)(exe_usec));
		bw *= 1000 * 1000 * LOOP_CNT;
		printf("CBDMA MEMSET, size: %u bytes, %d times, in %u usec, => %u bytes/sec\n", test_length, LOOP_CNT, (unsigned int)(exe_usec), (unsigned int)(bw));
#else
		exe_time = get_timer(exe_time);
		bw = ((uint64_t)(test_length)) / ((uint64_t)(exe_time));
		bw *= 1000 * LOOP_CNT;
		printf("CBDMA MEMSET, size: %u bytes, %d times, in %u msec, => %u bytes/sec\n", test_length, LOOP_CNT, (unsigned int)(exe_time), (unsigned int)(bw));
#endif

		run_command("md.b 0x0000 0x20", 0);
		printf("\n");

		invalidate_dcache_range(ADDR_DST, ADDR_DST + (MAX_SIZE_CBDMA + 1));

		cbdma_ptr->int_en = 0;
		cbdma_ptr->length = test_length;
		cbdma_ptr->src_adr = 0;
		cbdma_ptr->des_adr = ADDR_DST;
		wmb();
#ifdef FOR_ZEBU_CSIM
		tick[0] = get_ticks();
#else
		exe_time = get_timer(0);
#endif
		for (j = 0; j < LOOP_CNT; j++) {
			cbdma_ptr->config = CBDMA_CONFIG_DEFAULT | CBDMA_CONFIG_GO | CBDMA_CONFIG_CP;
			wmb();
			while (1) {
				if (cbdma_ptr->config & CBDMA_CONFIG_GO) {
					/* Still running */
					continue;
				}
				break;
			}
		}
#ifdef FOR_ZEBU_CSIM
		tick[1] = get_ticks();
		exe_usec = tick[1] - tick[0];
		bw = ((uint64_t)(test_length)) / ((uint64_t)(exe_usec));
		bw *= 1000 * 1000 * LOOP_CNT;
		printf("CBDMA MEMCPY, size: %u bytes, %d times, in %u usec, => %u bytes/sec\n", test_length, LOOP_CNT, (unsigned int)(exe_usec), (unsigned int)(bw));
#else
		exe_time = get_timer(exe_time);
		bw = ((uint64_t)(test_length)) / ((uint64_t)(exe_time));
		bw *= 1000 * LOOP_CNT;
		printf("CBDMA MEMCPY, size: %u bytes, %d times, in %u msec, => %u bytes/sec\n", test_length, LOOP_CNT, (unsigned int)(exe_time), (unsigned int)(bw));
#endif

		sprintf(cmd, "md.b 0x%x 0x20", ADDR_DST);
		run_command(cmd, 0);

#ifndef FOR_ZEBU_CSIM
		sprintf(cmd, "cmp.l 0x00000000 0x%x 0x%x", ADDR_DST, (test_length >> 2));
		ret = run_command(cmd, 0);
		if (ret) {
			return ret;
		}
#endif
		printf("\n");
#ifdef FOR_ZEBU_CSIM
		break;
#endif
	}

	return 0;
}

static int do_dram_tst(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;

	run_command("bdinfo && meminfo", 0);

	ret = dram_tst_cbdma(cmdtp, flag, argc, argv);
	if (ret) {
		return ret;
	}
#ifdef FOR_ZEBU_CSIM
	return 0;
#endif
	ret = dram_tst_simple_rw(cmdtp, flag, argc, argv);
	if (ret) {
		return ret;
	}

	return 0;
}

U_BOOT_CMD(
	dram_tst,	CONFIG_SYS_MAXARGS,	1,	do_dram_tst,
	"Memory r/w test.",
	"Memory r/w test.\n"
	"dram_tst command ...\n"
);

