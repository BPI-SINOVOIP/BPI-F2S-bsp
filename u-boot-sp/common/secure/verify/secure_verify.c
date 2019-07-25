#include <common.h>
#include "secure_verify.h"
#include "../ed25519/ed25519.h"
#include "sp_otp.h"

#define VERIFY_SIGN_MAGIC_DATA	(0x7369676E)

/***********************************
|---------------------------|
|		kernel data 		|
|---------------------------|
|---------------------------|
|	sig_flag_data(8byte)	|
|---------------------------|
|---------------------------|
|		sig data(64byte)	|
|---------------------------|
***********************************/

static volatile struct hbgpio_sunplus *otp_data = (volatile struct hbgpio_sunplus *)(HB_GPIO);

static u32 read_sb_flag(void)
{
	return ((otp_data->hb_gpio_rgst_bus32[0] >> 13) & 0x1); /* G350.0[13] */
}

void prn_dump_buffer(unsigned char *buf, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		if (i && !(i & 0xf)) {
			printf(" \n");
		}
		printf("0x%x ",buf[i]);
	}
	puts(" \n");
}

static void load_otp_pub_key(u8 in_pub[])
{
	int i;
	for (i = 0; i < 32; i++) {
		read_otp_data(i+64,(char *)&in_pub[i]);
	}
	puts("uboot  OTP pub-key:\n");
	prn_dump_buffer(in_pub,32);
}

int verify_kernel_signature(const image_header_t  *hdr)
{
	int sig_size = 64;
	int sig_flag_size = 8;
	int ret = -1;
	u8 in_pub[32] = {0};
	u8 sig_flag[8] = {0};
	u8 *data=NULL, *sig=NULL;
	int imgsize = 0;
	int tv1=0,tv2=0;
	unsigned int data_size=0;
	if(hdr == NULL)
		goto out;
	
	if ((read_sb_flag() & 0x01) == 0) {
		puts("\n ******OTP Secure Boot is OFF ,return success******\n");
		return 0;
	}
	
	/* Load public key */
	imgsize = image_get_data_size(hdr);
	
	/* load signature from image end */
	if (imgsize < sig_size) {
		puts("image size error,too small img\n");
		goto out;
	}
	puts("Verify signature...(Uboot-->Kernel)");
	
	data = ((u8 *)hdr);
	data_size = imgsize + sizeof(struct image_header);
	sig = data + data_size + sig_flag_size;

	/* Load sign flag data  */
	memcpy(sig_flag,data + data_size,4);// get sig magic data
	u32 sig_magic_data = (sig_flag[0]<<24)|(sig_flag[1]<<16)|(sig_flag[2]<<8)|(sig_flag[3]);
	printf("\n sig_magic_data=0x%x\n",sig_magic_data);

	if(sig_magic_data != VERIFY_SIGN_MAGIC_DATA)
	{
		puts("\n imgdata no sign data \n");
		goto out;
	}
	load_otp_pub_key(in_pub);
	/* verify signature */
#ifdef FOR_ZEBU_CSIM
	tv1 = get_ticks();
#else
	tv1 = get_timer(0);
#endif
	ret = !ed25519_verify(sig, data, data_size, in_pub);
	if (ret) {
		puts("\nsignature verify FAIL!!!!");
		puts("\nsignature:");
		prn_dump_buffer(sig, sig_size);
	} else {
		puts("\nsignature verify OK !!!!");
	}
#ifdef FOR_ZEBU_CSIM
	tv2 = get_ticks();
#else
	tv2 = get_timer(tv1);
#endif
	printf("\n time %dms\n",tv2);

out:
	return ret;
}

