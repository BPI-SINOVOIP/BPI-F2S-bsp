/*----------------------------------------------------------------------------*
 *					INCLUDE DECLARATIONS
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mach/hdmitx.h>
#include <sys/ioctl.h>

/*----------------------------------------------------------------------------*
 *					MACRO DECLARATIONS
 *---------------------------------------------------------------------------*/
#define MSG(fmt, args...) printf("[%s:%d] "fmt, __FILE__, __LINE__, ##args)
#define CHECK_(x, msg, errid) if (!(x)) {MSG("%s, %s", msg, #x);}
#define CHECK(x) CHECK_(x, "Check Failed", -1)

/*----------------------------------------------------------------------------*
 *					DATA TYPES
 *---------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *					GLOBAL VARIABLES
 *---------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *					EXTERNAL DECLARATIONS
 *---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
 *					FUNCTION DECLARATIONS
 *---------------------------------------------------------------------------*/

int main(int argc, const char **argv)
{
	int err = 0, hdmitx_fp;
	int value = 0;
	enum hdmitx_timing timing;
	enum hdmitx_color_depth depth;
	unsigned char enable, rx_ready;
	printf("hdmitx test\n");

	hdmitx_fp = open("/dev/hdmitx", O_RDWR);

	if (hdmitx_fp) {

		if (argc > 1) {

			if (strcmp(argv[1], "timing") == 0)	{

				if (argc > 2) {
					value = atoi(argv[2]);
					if ((value >= HDMITX_TIMING_480P)
						&& (value < HDMITX_TIMING_MAX)) {
						timing = (enum hdmitx_timing) value;
					} else {
						timing = HDMITX_TIMING_480P;
					}

					CHECK(ioctl(hdmitx_fp, HDMITXIO_SET_TIMING, &timing) != 0);
				}

			} else if (strcmp(argv[1], "depth") == 0) {

				if (argc > 2) {
					value = atoi(argv[2]);
					if ((value >= HDMITX_COLOR_DEPTH_24BITS)
						&& (value < HDMITX_COLOR_DEPTH_MAX)) {
						depth = (enum hdmitx_color_depth) value;
					} else {
						depth = HDMITX_COLOR_DEPTH_24BITS;
					}

					CHECK(ioctl(hdmitx_fp, HDMITXIO_SET_COLOR_DEPTH, &depth) != 0);
				}
			} else if (strcmp(argv[1], "display") == 0) {

				if (argc > 2) {
					value = atoi(argv[2]);
					if (value) {
						enable = 1;
					} else {
						enable = 0;
					}

					CHECK(ioctl(hdmitx_fp, HDMITXIO_DISPLAY, &enable) != 0);
				}
			} else if (strcmp(argv[1], "ptg") == 0) {

				if (argc > 2) {
					value = atoi(argv[2]);
					if (value) {
						enable = 1;
					} else {
						enable = 0;
					}

					CHECK(ioctl(hdmitx_fp, HDMITXIO_PTG, &enable) != 0);
				}
			} else if (strcmp(argv[1], "info") == 0) {

				CHECK(ioctl(hdmitx_fp, HDMITXIO_GET_TIMING, &timing) != 0);
				CHECK(ioctl(hdmitx_fp, HDMITXIO_GET_COLOR_DEPTH, &depth) != 0);
				CHECK(ioctl(hdmitx_fp, HDMITXIO_GET_RX_READY, &rx_ready) != 0);

				printf("[HDMITX][INFO]\n");
				printf("==============\n");
				printf("- timing:   %d\n", timing);
				printf("- depth:    %d\n", depth);
				printf("- rx ready: %d\n", rx_ready);
				printf("==============\n\n");
			}
		}

		close(hdmitx_fp);
	} else {
		MSG("Open /dev/hdmitx failed\n");
		err = -1;
	}

	return err;
}