#ifndef _INC_SP_BBLK_H_
#define _INC_SP_BBLK_H_

#include <compiler.h>
#include <nand.h>

/* 16-byte optional boot entry */
struct OptBootEntry16
{
	uint32_t opt0_copies;
	uint32_t opt0_pg_off;
	uint32_t opt0_pg_cnt;
	uint32_t reserved;
};

#define BHDR_BEG_SIGNATURE 0x52444842  // BHDR
#define BHDR_END_SIGNATURE 0x444e4548  // HEND
struct BootProfileHeader
{
	// 0
	uint32_t    Signature;     // BHDR_BEG_SIGNATURE
	uint32_t    Length;        // 256
	uint32_t    Version;       // 0
	uint32_t    reserved12;

	// 16
	uint8_t     BchType;       // BCH method: 0=1K60, 0xff=BCH OFF
	uint8_t     addrCycle;     // NAND addressing cycle
	uint16_t    ReduntSize;    // depricated, keep it 0
	uint32_t    BlockNum;      // Total NAND block number
	uint32_t    BadBlockNum;   // not used now
	uint32_t    PagePerBlock;  // NAND Pages per Block

	// 32
	uint32_t    PageSize;       // NAND Page size
	uint32_t    ACWriteTiming;  // not used now
	uint32_t    ACReadTiming;   // not used now
	uint32_t    PlaneSelectMode;// special odd blocks read mode (bit 0: special sw flow en. bit 1 read mode en. bit 2~bit 5 plane select bit addr)

	// 48
	uint32_t    xboot_copies;   // Number of Xboot copies. Copies are consecutive.
	uint32_t    xboot_pg_off;   // page offset (usu. page offset to block 1) to Xboot
	uint32_t    xboot_pg_cnt;   // page count of one copy of Xboot
	uint32_t    reserved60;

	// 64
	uint64_t    uboot_env_off;  // u64 hint offset to uboot env partition
	uint32_t    reserved72;
	uint32_t    reserved76;

	// 80
	struct OptBootEntry16 opt_entry[10]; // optional boot entries at 80, ..., 224

	// 240
	uint32_t    reserved240;
	uint32_t    reserved244;
	uint32_t    EndSignature;   // BHDR_END_SIGNATURE
	uint32_t    CheckSum;       // TCP checksum (little endian)

	// 256
};

#define SP_BHDR_LOADED_ADDR	0x9e809600   // bhdr is loaded by romcode
#define SP_BHDR_IS_VALID()	( \
	(((struct BootProfileHeader *)SP_BHDR_LOADED_ADDR)->Signature    == BHDR_BEG_SIGNATURE) && \
	(((struct BootProfileHeader *)SP_BHDR_LOADED_ADDR)->EndSignature == BHDR_END_SIGNATURE))
#define SP_GET_BHDR_UBOOT_ENV_HINT()  (((struct BootProfileHeader *)SP_BHDR_LOADED_ADDR)->uboot_env_off)

typedef struct mtd_info nand_info_t;

uint32_t sp_nand_lookup_bdata_sect_sz(nand_info_t *nand);

int sp_nand_write_bblk(nand_info_t *nand, loff_t off, size_t *length,
		       loff_t maxsize, u_char *data, int is_hdr);

int sp_nand_read_bblk(nand_info_t *nand, loff_t off, size_t *length,
		      loff_t maxsize, u_char *data, int no_skip);

int sp_nand_compose_bhdr(nand_info_t *mtd, struct BootProfileHeader *hdr,
			 uint32_t xboot_sz);

void sp_nand_info(nand_info_t *mtd);

#endif
