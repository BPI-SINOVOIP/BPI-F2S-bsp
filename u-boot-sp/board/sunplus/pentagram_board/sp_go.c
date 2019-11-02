#include <common.h>
#include <command.h>
#include <image.h>
#include <mapmem.h>
#include <malloc.h>


static uint32_t sum32(uint32_t sum, uint8_t *data, uint32_t len)
{
	uint32_t val = 0, pos = 0;

	for (; (pos + 4) <= len; pos += 4)
		sum += *(uint32_t *)(data + pos);
	/*
	 * word0: 3 2 1 0
	 * word1: _ 6 5 4
	 */
	for (; len - pos; len--)
		val = (val << 8) | data[len - 1];

	sum += val;

	return sum;
}

/* Similar with original u-boot flow but use different crc calculation */
int sp_image_check_hcrc(const image_header_t *hdr)
{
	ulong hcrc;
	ulong len = image_get_header_size();
	image_header_t header;

	/* Copy header so we can blank CRC field for re-calculation */
	memmove(&header, (char *)hdr, image_get_header_size());
	image_set_hcrc(&header, 0);

	hcrc = sum32(0, (unsigned char *)&header, len);

	return (hcrc == image_get_hcrc(hdr));
}

/* Similar with original u-boot flow but use different crc calculation */
int sp_image_check_dcrc(const image_header_t *hdr)
{
	ulong data = image_get_data(hdr);
	ulong len = image_get_data_size(hdr);
	ulong dcrc = sum32(0, (unsigned char *)data, len);

	return (dcrc == image_get_dcrc(hdr));
}

/*
 * Similar with original u-boot verifiction. Only data crc is different.
 * Return NULL if failed otherwise return header address.
 */
int sp_qk_uimage_verify(ulong img_addr, int verify)
{
	image_header_t *hdr = (image_header_t *)img_addr;

	/* original uImage header's magic */
	if (!image_check_magic(hdr)) {
		puts("Bad Magic Number\n");
		return (int)NULL;
	}

	/* hcrc by quick sunplus crc */
	if (!sp_image_check_hcrc(hdr)) {
		puts("Bad Header Checksum(Simplified)\n");
		return (int)NULL;
	}

	image_print_contents(hdr);

	/* dcrc by quick sunplys crc */
	if (verify) {
		puts("   Verifying Checksum ... ");
		if (!sp_image_check_dcrc(hdr)) {
			printf("Bad Data CRC(Simplified)\n");
			return (int)NULL;
		}
		puts("OK\n");
	}

	return (int)hdr;
}

/* return 0 if failed otherwise return 1 */
int sp_image_verify(u32 kernel_addr, u32 dtb_addr)
{
	if (!sp_qk_uimage_verify(kernel_addr, 1))
		return 0;

	if (!sp_qk_uimage_verify(dtb_addr, 1))
		return 0;

	return 1;
}

__attribute__((weak))
unsigned long do_sp_go_exec(ulong (*entry)(int, char * const [], unsigned int), int argc,
			    char * const argv[], unsigned int dtb)
{
	u32 kernel_addr, dtb_addr; /* these two addr will include headers. */

	kernel_addr = simple_strtoul(argv[0], NULL, 16);
	dtb_addr = simple_strtoul(argv[1], NULL, 16);

	printf("[u-boot] kernel address 0x%08x, dtb address 0x%08x\n",
		kernel_addr, dtb_addr);

	if (!sp_image_verify(kernel_addr, dtb_addr))
		return CMD_RET_FAILURE;

	cleanup_before_linux();

	return entry (0, 0, (dtb_addr + 0x40));
}

static int do_sp_go(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong   addr, rc;
	int     rcode = 0;

	addr = simple_strtoul(argv[1], NULL, 16);
	addr += 0x40; /* 0x40 for skipping quick uImage header */

	printf ("## Starting application at 0x%08lX ...\n", addr);

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
	rc = do_sp_go_exec ((void *)addr, argc - 1, argv + 1, 0);
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

U_BOOT_CMD(
	sp_go, CONFIG_SYS_MAXARGS, 1, do_sp_go,
	"sunplus booting command",
	"sp_go - run kernel at address 'addr'\n"
	"\n"
	"sp_go [kernel addr] [dtb addr]\n"
	"\tkernel addr should include the 'qk_sp_header'\n"
	"\twhich is similar as uImage header but different crc method.\n"
	"\tdtb also should have 'qk_sp_header', althrough dtb originally\n"
	"\thas its own header.\n"
	"\n"
	"\tSo image would be like this :\n"
	"\t<kernel addr> : [qk uImage header][kernel]\n"
	"\t<dtb addr>    : [qk uImage header][dtb header][dtb]\n"
);

#ifdef SPEED_UP_SPI_NOR_CLK

#if 0
#define dev_dbg(fmt, args ...)  printf(fmt, ## args)
#else
#define dev_dbg(fmt, args ...)
#endif
#define SPI_NOR_CTRL_BASE       0x9C000B00
#define SUNPLUS_ROMTER_ID       0x0053554E
#define DEFAULT_READ_ID_CMD     0x9F

/* spi_ctrl */
#define SPI_CLK_DIV_MASK        (0x7<<16)
#define SPI_CLK_D_2             1
#define SPI_CLK_D_4             2
#define SPI_CLK_D_6             3
#define SPI_CLK_D_8             4
#define SPI_CLK_D_16            5
#define SPI_CLK_D_24            6
#define SPI_CLK_D_32            7

#define CLEAR_CUST_CMD          (~0xffff)
#define CUST_CMD(x)             (x<<8)
#define SPI_CTRL_BUSY           (1<<31)

/* spi_auto_cfg */
#define PIO_TRIGGER             (1<<21)

enum SPI_PIO_DATA_BYTE
{
	BYTE_0 = 0<<4,
	BYTE_1 = 1<<4,
	BYTE_2 = 2<<4,
	BYTE_3 = 3<<4,
	BYTE_4 = 4<<4
};

enum SPI_PIO_CMD
{
	CMD_READ = 0<<2,
	CMD_WRITE = 1<<2
};

enum SPI_PIO_ADDRESS_BYTE
{
	ADDR_0B = 0,
	ADDR_1B = 1,
	ADDR_2B = 2,
	ADDR_3B = 3
};


typedef volatile struct {
	// Group 022 : SPI_FLASH
	unsigned int  spi_ctrl;
	unsigned int  spi_timing;
	unsigned int  spi_page_addr;
	unsigned int  spi_data;
	unsigned int  spi_status;
	unsigned int  spi_auto_cfg;
	unsigned int  spi_cfg0;
	unsigned int  spi_cfg1;
	unsigned int  spi_cfg2;
	unsigned int  spi_data64;
	unsigned int  spi_buf_addr;
	unsigned int  spi_status_2;
	unsigned int  spi_err_status;
	unsigned int  spi_mem_data_addr;
	unsigned int  spi_mem_parity_addr;
	unsigned int  spi_col_addr;
	unsigned int  spi_bch;
	unsigned int  spi_intr_msk;
	unsigned int  spi_intr_sts;
	unsigned int  spi_page_size;
	unsigned int  G22_RESERVED[12];
} SPI_CTRL;

unsigned int SPI_nor_read_id(unsigned char cmd)
{
	SPI_CTRL *spi_reg = (SPI_CTRL *)map_sysmem(SPI_NOR_CTRL_BASE,0x80);
	unsigned int ctrl;
	unsigned int id;

	dev_dbg("%s\n", __FUNCTION__);

	// Setup Read JEDEC ID command.
	ctrl = spi_reg->spi_ctrl & CLEAR_CUST_CMD;
	ctrl = ctrl | CMD_READ | BYTE_3 | ADDR_0B | CUST_CMD(cmd);
	while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
		dev_dbg("wait spi_reg->spi_ctrl = 0x%08x\n", spi_reg->spi_ctrl);
	}
	spi_reg->spi_ctrl = ctrl;

	// Issue PIO command.
	spi_reg->spi_auto_cfg |= PIO_TRIGGER;
	while ((spi_reg->spi_auto_cfg & PIO_TRIGGER) != 0) {
		dev_dbg("wait PIO_TRIGGER\n");
	}
	dev_dbg("spi_reg->spi_data = 0x%08x\n", spi_reg->spi_data);
	id = spi_reg->spi_data;

	unmap_sysmem((void*)spi_reg);
	return (id&0xff0000) | ((id&0xff00)>>8) | ((id&0xff)<<8);
}

void SPI_nor_set_clk_div(int clkd)
{
	SPI_CTRL *spi_reg = (SPI_CTRL *)map_sysmem(SPI_NOR_CTRL_BASE,0x80);
	unsigned int ctrl;

	// Set clock divisor.
	ctrl = spi_reg->spi_ctrl & ~SPI_CLK_DIV_MASK;
	ctrl = ctrl | clkd << 16;
	while ((spi_reg->spi_ctrl & SPI_CTRL_BUSY) != 0) {
		dev_dbg("wait spi_reg->spi_ctrl = 0x%08x\n", spi_reg->spi_ctrl);
	}
	spi_reg->spi_ctrl = ctrl;

	unmap_sysmem((void*)spi_reg);
}

void SPI_nor_restore_cfg2(void)
{
	SPI_CTRL *spi_reg = (SPI_CTRL *)map_sysmem(SPI_NOR_CTRL_BASE,0x80);

	spi_reg->spi_cfg2 = 0x00150095; // restore default after seeting spi_ctrl

	unmap_sysmem((void*)spi_reg);
}

void SPI_nor_speed_up_clk(void)
{
	unsigned int id;

	id = SPI_nor_read_id(DEFAULT_READ_ID_CMD);
	printf("SPI:   Manufacturer id = 0x%02X, Device id = 0x%04X ", id>>16, id&0xffff);

	if ((id != SUNPLUS_ROMTER_ID) && (id != 0) && (id != 0xFFFFFF)) {
		printf("\n");
		SPI_nor_set_clk_div(SPI_CLK_D_4);
	} else {
		if (id == SUNPLUS_ROMTER_ID)
			printf("(Sunplus romter)\n");
		else
			printf("\n");
		//SPI_nor_set_clk_div(SPI_CLK_D_16);
	}

	SPI_nor_restore_cfg2();
}

#endif

#ifdef RASPBIAN_CMD

static int do_raspbian(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong   addr;
	int     size;
	char    *cmdline, *p;
	char    *delim = " \t";
	char    *bootargs;
	char    *new_bootargs;
	int     bootargs_len;
	int     init_len;
	int     found;

	if ((strcmp(argv[1], "init") == 0) && (argc == 4)) {
		addr = simple_strtoul(argv[2], NULL, 16);
		size = simple_strtoul(argv[3], NULL, 16);
		if (size >= 4096) {
			printf("Length of cmdline is too long!\n");
			return CMD_RET_FAILURE;
		}

		// Allocate memory and copy cmdline.
		cmdline = malloc(size+1);
		if (cmdline == NULL) {
			printf("Failed to alloc memory for cmdline!\n");
			return CMD_RET_FAILURE;
		}
		strncpy(cmdline, (char*)addr, size);
		cmdline[size] = 0;

		// Find token: "init="
		found = 0;
		p = strtok(cmdline, delim);
		while (p) {
			if (strncmp(p, "init=", 5) == 0) {
				found = 1;
				break;
			}
			p = strtok(NULL, delim);
		}

		if (found) {
			// Token "init=" is found. Check length of token.
			init_len = strlen(p);
			if (init_len <= 5) {
				free(cmdline);
				return CMD_RET_SUCCESS;
			}

			bootargs = env_get("bootargs");
			bootargs_len = strlen(bootargs);
			if ((init_len + bootargs_len) > (4095-1)) {
				// Length of new 'bootargs' should be no more than 4095
				printf("Length of new bootargs is too long!\n");
				free(cmdline);
				return CMD_RET_FAILURE;
			}

			// Allocate memory for new bootargs.
			new_bootargs = malloc(bootargs_len + init_len + 2);
			if (new_bootargs == NULL) {
				printf("Failed to alloc memory for new bootargs!\n");
				free(cmdline);
				return CMD_RET_FAILURE;
			}

			// Copy old bootargs.
			strcpy(new_bootargs, bootargs);
			if (bootargs_len > 0) {
				new_bootargs[bootargs_len] = ' ';
				bootargs_len++;
			}

			// Added 'init' to end of old bootargs.
			strcpy(&new_bootargs[bootargs_len], p);
			new_bootargs[bootargs_len+init_len] = 0;
			env_set("bootargs", new_bootargs);
			free(new_bootargs);
		}

		free(cmdline);
		return CMD_RET_SUCCESS;
	} else {
		return CMD_RET_USAGE;
	}
}

U_BOOT_CMD(
	raspb, 4, 1, do_raspbian,
	"Raspbian command",
	"- Raspbian command.\n"
	"\n"
	"raspb init addr size - copies 'init' setting of 'cmdline.txt' of Raspbian to 'bootargs'.\n"
	"\taddr: address where 'cmdline.txt' is loaded\n"
	"\tsize: file size of 'cmdline.txt'\n"
);

#endif
