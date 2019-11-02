#include "imagetool.h"
#include <image.h>
#include <u-boot/crc.h>

static image_header_t header;

static int image_check_image_types(uint8_t type)
{
	if (((type > IH_TYPE_INVALID) && (type < IH_TYPE_FLATDT)) ||
	    (type == IH_TYPE_QUICKBOOT))
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

static int image_check_params(struct image_tool_params *params)
{
	return	((params->dflag && (params->fflag || params->lflag)) ||
		(params->fflag && (params->dflag || params->lflag)) ||
		(params->lflag && (params->dflag || params->fflag)));
}

static uint32_t sum32(uint32_t sum, uint8_t *data, uint32_t len)
{
	uint32_t val = 0, pos =0;

	for (; pos + 4 <= len; pos += 4)
		sum += *(uint32_t *)(data + pos);

	// word0: 3 2 1 0
	// word1: _ 6 5 4
	for (; len - pos; len--)
		val = (val << 8) | data[len - 1];

	sum += val;
	
	return sum;
}

static int image_verify_header(unsigned char *ptr, int image_size,
			struct image_tool_params *params)
{
	uint32_t len;
	const unsigned char *data;
	uint32_t checksum;
	image_header_t header;
	image_header_t *hdr = &header;

	/*
	 * create copy of header so that we can blank out the
	 * checksum field for checking - this can't be done
	 * on the PROT_READ mapped data.
	 */
	memcpy(hdr, ptr, sizeof(image_header_t));

	if (be32_to_cpu(hdr->ih_magic) != IH_MAGIC) {
		fprintf(stderr,
			"%s: Bad Magic Number: \"%s\" is no valid image\n",
			params->cmdname, params->imagefile);
		return -FDT_ERR_BADMAGIC;
	}

	data = (const unsigned char *)hdr;
	len  = sizeof(image_header_t);

	checksum = be32_to_cpu(hdr->ih_hcrc);
	hdr->ih_hcrc = cpu_to_be32(0);	/* clear for re-calculation */

	if (sum32(0, data, len) != checksum) {
		fprintf(stderr,
			"%s: ERROR: \"%s\" has bad header checksum!\n",
			params->cmdname, params->imagefile);
		return -FDT_ERR_BADSTATE;
	}

	data = (const unsigned char *)ptr + sizeof(image_header_t);
	len  = image_size - sizeof(image_header_t) ;

	checksum = be32_to_cpu(hdr->ih_dcrc);
	if (sum32(0, data, len) != checksum) {
		fprintf(stderr,
			"%s: ERROR: \"%s\" has corrupted data!\n",
			params->cmdname, params->imagefile);
		return -FDT_ERR_BADSTRUCTURE;
	}
	return 0;
}

static void image_set_header(void *ptr, struct stat *sbuf, int ifd,
				struct image_tool_params *params)
{
	uint32_t checksum;

	image_header_t * hdr = (image_header_t *)ptr;

	checksum = sum32(0,
			(const unsigned char *)(ptr +
				sizeof(image_header_t)),
			sbuf->st_size - sizeof(image_header_t));

	/* Build new header */
	image_set_magic(hdr, IH_MAGIC);
	image_set_time(hdr, sbuf->st_mtime);
	image_set_size(hdr, sbuf->st_size - sizeof(image_header_t));
	image_set_load(hdr, params->addr);
	image_set_ep(hdr, params->ep);
	image_set_dcrc(hdr, checksum);
	image_set_os(hdr, params->os);
	image_set_arch(hdr, params->arch);
	image_set_type(hdr, params->type);
	image_set_comp(hdr, params->comp);

	image_set_name(hdr, params->imagename);

	checksum = sum32(0, (const unsigned char *)hdr,
				sizeof(image_header_t));

	image_set_hcrc(hdr, checksum);
}

static int image_save_datafile(struct image_tool_params *params,
			       ulong file_data, ulong file_len)
{
	int dfd;
	const char *datafile = params->outfile;

	dfd = open(datafile, O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
		   S_IRUSR | S_IWUSR);
	if (dfd < 0) {
		fprintf(stderr, "%s: Can't open \"%s\": %s\n",
			params->cmdname, datafile, strerror(errno));
		return -1;
	}

	if (write(dfd, (void *)file_data, file_len) != (ssize_t)file_len) {
		fprintf(stderr, "%s: Write error on \"%s\": %s\n",
			params->cmdname, datafile, strerror(errno));
		close(dfd);
		return -1;
	}

	close(dfd);

	return 0;
}

static int image_extract_datafile(void *ptr, struct image_tool_params *params)
{
	const image_header_t *hdr = (const image_header_t *)ptr;
	ulong file_data;
	ulong file_len;

	if (image_check_type(hdr, IH_TYPE_MULTI)) {
		ulong idx = params->pflag;
		ulong count;

		/* get the number of data files present in the image */
		count = image_multi_count(hdr);

		/* retrieve the "data file" at the idx position */
		image_multi_getimg(hdr, idx, &file_data, &file_len);

		if ((file_len == 0) || (idx >= count)) {
			fprintf(stderr, "%s: No such data file %ld in \"%s\"\n",
				params->cmdname, idx, params->imagefile);
			return -1;
		}
	} else {
		file_data = image_get_data(hdr);
		file_len = image_get_size(hdr);
	}

	/* save the "data file" into the file system */
	return image_save_datafile(params, file_data, file_len);
}

/*
 * quick boot image parameters
 */
U_BOOT_IMAGE_TYPE(
        qkbootimage,
        "Sunplus Quick Boot Image",
        sizeof(image_header_t),
        &header,
        image_check_params,
        image_verify_header,
        image_print_contents,
        image_set_header,
        image_extract_datafile,
        image_check_image_types,
        NULL,
        NULL
);
