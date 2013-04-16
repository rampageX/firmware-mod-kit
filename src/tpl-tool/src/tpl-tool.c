/*
 * TP-Link firmware image extraction and rebuilding tool.
 *
 * Copyright (c) 2012 Jonathan McGowan <jonathan@howlingwolf.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or later
 * as published by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>     	/* for unlink() */
#include <getopt.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>

#include <netinet/in.h>		/* for network / host byte order conversions */

#include "md5.h"


#define PROGRAM_NAME	"tpl-tool"
#define PROGRAM_VER		"1.2"

/*
 * During testing against non-official firmware images I discovered that
 * OpenWrt images are broken despite having a valid image header. The
 * root filesystem follows directly after the kernel and not at the
 * offset specified.
 *
 * As a result they do *NOT* extract correctly.
 *
 * It would be possible to handle this but as they have their own build
 * system which is probably better suited, and everything else seems to
 * be handled correctly I decided to simply reject OpenWrt images rather
 * than risk a bad rebuild which might pass firmware update checks.
 */

#define BAD_VENDOR		"OpenWrt"

/*
 * All of the official firmware images available from the TP-Link website
 * at this time use the values defined below.
 */

#define HEADER_VERSION		0x01000000
#define	IMAGE2_OFFSET		0x00020200
#define ROOTFS_OFFSET		0x00100000
#define FLASH_4M			0x003c0000
#define FLASH_8M			0x007c0000

#define MD5SUM_LEN	16


struct image_header {
	uint32_t	header_version;
	char		image_vendor[24];
	char		image_version[36];
	uint32_t	product_id;
	uint32_t	product_ver;
	uint32_t	padding1;
	uint8_t	image_checksum[MD5SUM_LEN];
	uint32_t	padding2;
	uint8_t	kernel_checksum[MD5SUM_LEN];
	uint32_t	padding3;
	uint32_t	kernel_loadaddr;	/* kernel load address */
	uint32_t	kernel_entrypoint;	/* kernel entry point */
	uint32_t	image_length;
	uint32_t	kernel_offset;		/* kernel data offset */
	uint32_t	kernel_length;		/* kernel data length */
	uint32_t	rootfs_offset;		/* rootfs data offset */
	uint32_t	rootfs_length;		/* rootfs data length */
	uint32_t	bootldr_offset;		/* bootloader data offset */
	uint32_t	bootldr_length;		/* bootloader data length */
	uint16_t	fw_ver_major;		/* firmware version */
	uint16_t	fw_ver_minor;
	uint16_t	fw_ver_point;
	uint8_t	padding4[354];
} __attribute__ ((packed));

struct device_info {
	uint32_t	prod_id;
	uint32_t	prod_ver;
	char		*name;
	uint32_t	flash_size;
};

struct file_info {
	char		*name;
	uint32_t	size;
};

char MD5Key[MD5SUM_LEN] = {
	0xdc, 0xd7, 0x3a, 0xa5, 0xc3, 0x95, 0x98, 0xfb,
	0xdd, 0xf9, 0xe7, 0xf4, 0x0e, 0xae, 0x47, 0x38,
};

char MD5Key_bootldr[MD5SUM_LEN] = {
	0x8c, 0xef, 0x33, 0x5b, 0xd5, 0xc5, 0xce, 0xfa,
	0xa7, 0x9c, 0x28, 0xda, 0xb2, 0xe9, 0x0f, 0x42,
};

/*
 * Adding new devices.
 *
 * Create a new entry in the structure below and fill in the appropriate
 * values for the new device.
 *
 * Name from the image filename or the TP-Link website.
 * Product id and version from the header using the -s option.
 * Flash size from the header Image Size or Image2 Size if a bootloader
 * is present. ie. 0x003c0000 is 4M and 0x007c0000 is 8M.
 */

static struct device_info devices[] = {
	/* Product id, Product version, Name, Flash size */
	{0x08010001,	1,	"TL-WA801NDv1",		FLASH_4M},
	{0x09010001,	1,	"TL-WA901NDv1",		FLASH_4M},
	{0x09010002,	1,	"TL-WA901NDv2",		FLASH_4M},
	{0x09410002,	2,	"TL-WR941NDv2",		FLASH_4M},
	{0x09410004,	1,	"TL-WR941NDv4",		FLASH_4M},
	{0x10420001,	1,	"TL-WR1042NDv1",	FLASH_8M},
	{0x10430001,	1,	"TL-WR1043NDv1",	FLASH_8M},
	{0x25430001,	1,	"TL-WR2543Nv1",		FLASH_8M},
	{0x43000001,	1,	"TL-WDR4300v1",		FLASH_8M},
	{	/* Null entry to mark end of table */	}
};


/*
 * Console output macros.
 */

#define ERROR(fmt, ...) \
	do { \
		fflush(0); \
		fprintf(stderr, "[%s] *** ERROR: " fmt "\n", PROGRAM_NAME, ## __VA_ARGS__ ); \
	} while (0)

#define WARN(fmt, ...) \
	do { \
		fprintf(stderr, "[%s] WARNING: " fmt "\n", PROGRAM_NAME, ## __VA_ARGS__ ); \
	} while (0)

#define PRINT_STR(label, str) printf("%-19s: %s\n", label, str)

#define PRINT_HEX(label, value, str) printf("%-19s: 0x%08x%s\n", label, value, str)

#define PRINT_BOTH(label, value) printf("%-19s: 0x%08x / %7u\n", label, value, value)

#define PRINT_PROD_ID(label, value, str) printf("%-19s: 0x%08x  (%s)\n", label, value, str)


static void usage(void)
{
	fprintf(stderr, "%s %s (%s)\n", PROGRAM_NAME, PROGRAM_VER, __DATE__);
	fprintf(stderr, "Copyright (C) 2012 Jonathan McGowan\n\n");
	fprintf(stderr, "Usage: %s <option> <file> [-o <file>]\n", PROGRAM_NAME);
	fprintf(stderr,
		"\n"
		"Options:\n"
		"  -s <file>    Show firmware image header information.\n"
		"  -b <file>    Rebuild firmware image.\n"
		"  -x <file>    Extract firmware image.\n"
		"  -o <file>    Specify an alternative output filename.\n"
		"\n"
		"  -h           Show this message.\n"
	);
}


static struct device_info *get_device_info(uint32_t prod_id)
{
	struct device_info *ret;
	struct device_info *device;

	ret = NULL;
	if (prod_id == 0)
		goto out;

	for (device = devices; device->name != NULL; device++) {
		if (prod_id == device->prod_id) {
			ret = device;
			break;
		}
	};

 out:
	return ret;
}

/*
 * Helper function used to simplify checksum generation/verification code.
 *
 * Selects the appropriate key value (normal/bootldr), generates an
 * MD5 hash of the data specified and returns the result of comparing
 * the new MD5 hash with the original. A flag value controls restoration
 * of the original hash value.
 */

static int checksum(char *buf, int len, int overwrite)
{
	MD5_CTX ctx;
	struct image_header *hdr;
	uint8_t old_checksum[MD5SUM_LEN];
	int ret;

	hdr = (struct image_header *)buf;
	memcpy(old_checksum, hdr->image_checksum, MD5SUM_LEN);

	if (ntohl(hdr->bootldr_length) == 0)
		memcpy(hdr->image_checksum, MD5Key, MD5SUM_LEN);
	else
		memcpy(hdr->image_checksum, MD5Key_bootldr, MD5SUM_LEN);

	MD5_Init(&ctx);
	MD5_Update(&ctx, buf, len);
	MD5_Final(hdr->image_checksum, &ctx);

	ret = memcmp(hdr->image_checksum, old_checksum, MD5SUM_LEN);

	if (!overwrite)
		memcpy(hdr->image_checksum, old_checksum, MD5SUM_LEN);

	return ret;
}


/*
 * File I/O functions.
 */

static int get_file_size(struct file_info *fdata)
{
	struct stat st;
	int ret = EXIT_FAILURE;

	if (fdata->name == NULL)
		goto out;

	ret = stat(fdata->name, &st);
	if (ret)
		goto out;

	fdata->size = st.st_size;
	ret = EXIT_SUCCESS;

 out:
	return ret;
}

static int read_to_buffer(struct file_info *fdata, char *buf)
{
	FILE *fp;
	int ret = EXIT_FAILURE;

	fp = fopen(fdata->name, "r");
	if (fp == NULL)
		goto out;

	errno = 0;
	int c = fread(buf, fdata->size, 1, fp);
	if (errno != 0)
		goto out_close;

	ret = EXIT_SUCCESS;

 out_close:
	fclose(fp);
 out:
	return ret;
}

static int write_from_buffer(struct file_info *fdata, char *buf)
{
	FILE *fp;
	int ret = EXIT_FAILURE;

	fp = fopen(fdata->name, "w");
	if (fp == NULL)
		goto out;

	errno = 0;
	fwrite(buf, fdata->size, 1, fp);
	if (errno != 0)
		goto out_flush;

	ret = EXIT_SUCCESS;

 out_flush:
	fflush(fp);
	fclose(fp);
	if (ret != EXIT_SUCCESS)
		unlink(fdata->name);
 out:
	return ret;
}


/*
 * Helper function used to output the image checksums and verification
 * status.
 */

static void print_checksum(char *label, uint8_t *chksum, int valid)
{
	int i;

	printf("%-19s:", label);
	for (i=0; i<MD5SUM_LEN; i++)
		printf(" %02x", chksum[i]);

	switch (valid) {
		case 0:
			printf("  (Invalid)\n");
			break;
		case 1:
			printf("  (Valid)\n");
			break;
		case -1:
			printf("  (Not Verified)\n");
			break;
	}
}


/*
 * Show firmware image header.
 *
 * Outputs the image header in a human readable format.
 */

static int show_image_header(char *fname_in)
{
	char *buf;
	struct file_info fdata;
	struct image_header *hdr;
	struct device_info *device;
	int ret = EXIT_FAILURE;

	fdata.name = fname_in;
	ret = get_file_size(&fdata);
	if (ret) {
		ERROR("Image file not found \"%s\".", fdata.name);
		goto out;
	}

	buf = malloc(fdata.size);
	if (!buf) {
		ERROR("Unable to allocate image buffer.");
		goto out;
	}

	ret = read_to_buffer(&fdata, buf);
	if (ret) {
		ERROR("Unable to read image file \"%s\".", fdata.name);
		goto out_free_buf;
	}

	hdr = (struct image_header *)buf;

	PRINT_STR("Filename", fdata.name);
	PRINT_BOTH("Filesize", fdata.size);
	printf("\n");

	if (ntohl(hdr->header_version) != HEADER_VERSION) {
		ERROR("Unsupported image header version (%u).", ntohl(hdr->header_version));
		goto out_free_buf;
	}

	PRINT_STR("Image Vendor", hdr->image_vendor);
	PRINT_STR("Image Version", hdr->image_version);
	PRINT_BOTH("Image Size", ntohl(hdr->image_length));

	int tmp = (checksum(buf, fdata.size, 0)) ? 0 : 1;
	print_checksum("Image Checksum", hdr->image_checksum, tmp);
	printf("\n");

	char *tmp_str = "Unknown";
	device = get_device_info(ntohl(hdr->product_id));
	if (device)
		tmp_str = device->name;
	PRINT_PROD_ID("Product Id", ntohl(hdr->product_id), tmp_str);

	tmp_str = "";
	if (device && (device->prod_ver != ntohl(hdr->product_ver)))
		tmp_str = "  (Unknown)";
	PRINT_HEX("Product Version", ntohl(hdr->product_ver), tmp_str);

	printf("%-19s: %u.%u.%u\n\n", "Firmware Version", ntohs(hdr->fw_ver_major), \
		ntohs(hdr->fw_ver_minor), ntohs(hdr->fw_ver_point));


	/* Display bootloader info and shift pointer to inner image */
	if (ntohl(hdr->bootldr_length) != 0) {
		PRINT_BOTH("Bootldr Offset", ntohl(hdr->bootldr_offset));
		PRINT_BOTH("Bootldr Length", ntohl(hdr->bootldr_length));
		printf("\n");

		hdr = (struct image_header *)(buf + IMAGE2_OFFSET);

		PRINT_BOTH("Image2 Size", ntohl(hdr->image_length));

		tmp = (checksum((char *)hdr, fdata.size - IMAGE2_OFFSET, 0)) ? 0 : 1;
		print_checksum("Image2 Checksum", hdr->image_checksum, tmp);
		printf("\n");
	}


	PRINT_BOTH("Kernel Offset", ntohl(hdr->kernel_offset));
	PRINT_BOTH("Kernel Length", ntohl(hdr->kernel_length));
	PRINT_HEX("Kernel Load Address", ntohl(hdr->kernel_loadaddr), "");
	PRINT_HEX("Kernel Entry Point", ntohl(hdr->kernel_entrypoint), "");
	print_checksum("Kernel Checksum", hdr->kernel_checksum, -1);
	printf("\n");

	PRINT_BOTH("Rootfs Offset", ntohl(hdr->rootfs_offset));
	PRINT_BOTH("Rootfs Length", ntohl(hdr->rootfs_length));


 out_free_buf:
	free(buf);
 out:
	return ret;
}


/*
 * Extract firmware image.
 *
 * Extracts the individual parts of an image to the following files.
 * 
 * <file>-header
 * <file>-bootldr  (when present)
 * <file>-kernel
 * <file>-rootfs
 * 
 * Where <file> is either the input filename or that specified with -o
 */

static int extract_image(char *fname_in, char *fname_out)
{
	char *buf;
	char *new_name;
	struct file_info fdata;
	struct image_header *hdr;
	struct device_info *device;
	int buf_offset;
	int bootldr_present = 0;
	int ret = EXIT_FAILURE;

	if (fname_out == NULL)
		fname_out = fname_in;

	fdata.name = fname_in;
	ret = get_file_size(&fdata);
	if (ret) {
		ERROR("Image file not found \"%s\".", fdata.name);
		goto out;
	}

	buf = malloc(fdata.size);
	if (!buf) {
		ERROR("Unable to allocate image buffer.");
		goto out;
	}

	ret = read_to_buffer(&fdata, buf);
	if (ret) {
		ERROR("Unable to read image file \"%s\".", fdata.name);
		goto out_free_buf;
	}

	hdr = (struct image_header *)buf;


	/* Check header version and hardware id */
	if (ntohl(hdr->header_version) != HEADER_VERSION) {
		ERROR("Unsupported image header version (%u).", ntohl(hdr->header_version));
		goto out_free_buf;
	}

	/* OpenWrt images are broken */
	if (!strncasecmp(hdr->image_vendor, BAD_VENDOR, sizeof(hdr->image_vendor))) {
		ERROR("OpenWrt images do not extract correctly.");
		goto out_free_buf;
	}

	device = get_device_info(ntohl(hdr->product_id));
	if (!device) {
		WARN("Unknown device (0x%08x).", ntohl(hdr->product_id));
		WARN("Component files may not be valid.");
	}

	/* Verify image checksum */
	if (checksum(buf, fdata.size, 0)) {
		WARN("Invalid Image Checksum.");
		WARN("Component files may not be valid.");
	}

	bootldr_present = (ntohl(hdr->bootldr_length) == 0) ? 0 : 1;

	new_name = malloc(strlen(fname_out) + 9);
	if (!new_name) {
		ERROR("Unable to allocate space for filenames.");
		goto out_free_buf;
	}
	fdata.name = new_name;


	/* Save header block */
	sprintf(fdata.name, "%s-header", fname_out);
	fdata.size = sizeof(struct image_header);
	ret = write_from_buffer(&fdata, buf);
	if (ret) {
		ERROR("Unable to create image header file \"%s\".", fdata.name);
		goto out_free_name;
	}


	/* Save bootloader image */
	if (bootldr_present) {
		fdata.size = ntohl(hdr->bootldr_length);
		sprintf(fdata.name, "%s-bootldr", fname_out);

		ret = write_from_buffer(&fdata, buf + sizeof(struct image_header));
		if (ret) {
			ERROR("Unable to create bootloader file \"%s\".", fdata.name);
			goto out_free_name;
		}
	}


	/* Move pointer to inner image and verify checksum */
	if (bootldr_present) {
		hdr = (struct image_header *)(buf + IMAGE2_OFFSET);

		if (checksum((char *)hdr, ntohl(hdr->image_length), 0)) {
			WARN("Invalid Image2 Checksum.");
			WARN("kernel/rootfs files may not be valid.");
		}
	}

	/* Save kernel image */
	buf_offset = ntohl(hdr->kernel_offset);
	if (buf_offset != sizeof(struct image_header))
		WARN("Non standard kernel offset (0x%08x).", buf_offset);

	sprintf(fdata.name, "%s-kernel", fname_out);
	fdata.size = ntohl(hdr->kernel_length);

	ret = write_from_buffer(&fdata, ((char *)hdr) + buf_offset);
	if (ret) {
		ERROR("Unable to create kernel file \"%s\".", fdata.name);
		goto out_free_name;
	}


	/* Save root filesystem image */
	buf_offset = ntohl(hdr->rootfs_offset);
	if (buf_offset != ROOTFS_OFFSET)
		WARN("Non standard rootfs offset (0x%08x).", buf_offset);

	sprintf(fdata.name, "%s-rootfs", fname_out);
	fdata.size = ntohl(hdr->rootfs_length);

	ret = write_from_buffer(&fdata, ((char *)hdr) + buf_offset);
	if (ret) {
		ERROR("Unable to create rootfs file \"%s\".", fdata.name);
		goto out_free_name;
	}

	printf("Image file \"%s\" successfully extracted.\n", fname_in);


 out_free_name:
	free(new_name);
 out_free_buf:
	free(buf);
 out:
	return ret;
}


/*
 * Rebuild firmware image.
 *
 * Rebuilds an image from the individual parts extracted with -x
 * 
 * <file>-header
 * <file>-bootldr  (when present)
 * <file>-kernel
 * <file>-rootfs
 *
 * Where <file> is the input filename and the output file either
 * <file>-new or that specified with -o
 *
 * Firmware images which include a bootloader have an 'image within image'
 * structure consisting of a 'standard' kernel/rootfs image within a
 * larger image.
 *
 * -----------------------------------------
 * |     |     |                           |
 * |     |     |    -------------------    |
 * |     |     |    |     |     |     |    |
 * |  h  |  b  |    |  h  |  k  |  r  |    |
 * |  e  |  o  |    |  e  |  e  |  o  |    |
 * |  a  |  o  |    |  a  |  r  |  o  |    |
 * |  d  |  t  |    |  d  |  n  |  t  |    |
 * |  r  |  l  |    |  e  |  e  |  f  |    |
 * |     |  d  |    |  r  |  l  |  s  |    |
 * |     |  r  |    |     |     |     |    |
 * |     |     |    -------------------    |
 * |     |     |                           |
 * -----------------------------------------
 */

static int build_image(char *fname_in, char *fname_out)
{
	char *new_name;
	char *hdr_buf;
	char *img_buf;
	struct file_info fdata;
	struct image_header *hdr;
	struct device_info *device;

	struct {
		struct image_header *hdr;
		char *kernel_buf;
		char *rootfs_buf;
	} img;

	struct {
		struct image_header *hdr;
		char *bootldr_buf;
	} img_b;

	int max_length;
	int buf_size;
	int ret = EXIT_FAILURE;


	new_name = malloc(strlen(fname_in) + 9);
	if (!new_name) {
		ERROR("Unable to allocate space for filenames.");
		goto out;
	}
	fdata.name = new_name;


	/* Read image header file */
	sprintf(fdata.name, "%s-header", fname_in);
	ret = get_file_size(&fdata);
	if (ret) {
		ERROR("Header file not found \"%s\".", fdata.name);
		goto out_free_name;
	}

	if (fdata.size != sizeof(struct image_header)) {		/* Sanity check */
		ERROR("Wrong size header file \"%s\".", fdata.name);
		goto out_free_name;
	}

	hdr_buf = malloc(sizeof(struct image_header));
	if (!hdr_buf) {
		ERROR("Unable to allocate space for image header.");
		goto out_free_name;
	}

	ret = read_to_buffer(&fdata, hdr_buf);
	if (ret) {
		ERROR("Unable to read image header file \"%s\".", fdata.name);
		goto out_free_hdr;
	}

	hdr = (struct image_header *)hdr_buf;


	/* Header sanity checks */
	if (ntohl(hdr->header_version) != HEADER_VERSION) {
		ERROR("Unsupported image header version (%u).", ntohl(hdr->header_version));
		goto out_free_hdr;
	}

	if (ntohl(hdr->bootldr_offset) != 0) {
		ERROR("Bootloader offset must be 0 (0x%08x).", ntohl(hdr->bootldr_offset));
		goto out_free_hdr;
	}

	/* OpenWrt images are broken */
	if (!strncasecmp(hdr->image_vendor, BAD_VENDOR, sizeof(hdr->image_vendor))) {
		ERROR("Rebuilding OpenWrt images is not supported.");
		goto out_free_hdr;
	}

	if (ntohl(hdr->kernel_offset) != sizeof(struct image_header))
		WARN("Non standard kernel offset (0x%08x).", ntohl(hdr->kernel_offset));

	if (ntohl(hdr->rootfs_offset) != ROOTFS_OFFSET)
		WARN("Non standard rootfs offset (0x%08x).", ntohl(hdr->rootfs_offset));


	/* Get image size from device info or header and set buffer size */
	device = get_device_info(ntohl(hdr->product_id));
	if (device) {
		buf_size = device->flash_size + IMAGE2_OFFSET;
		max_length = device->flash_size;
	}
	else {
		WARN("Unknown device (0x%08x).", ntohl(hdr->product_id));
		WARN("Using image size from header.");

		buf_size = ntohl(hdr->image_length);
		if (ntohl(hdr->bootldr_length) != 0)
			max_length = buf_size - IMAGE2_OFFSET;
		else {
			max_length = buf_size;
			buf_size += IMAGE2_OFFSET;
		}
	}


	/* Allocate buffer, set pointers and copy image header to buffer */
	img_buf = malloc(buf_size);
	if (!img_buf) {
		ERROR("Unable to allocate image buffer.");
		goto out_free_hdr;
	}

	memset(img_buf, 0xff, buf_size);

	img_b.hdr = (struct image_header *)img_buf;
	img_b.bootldr_buf = img_buf + sizeof(struct image_header);

	img.hdr = (struct image_header *)(img_buf + IMAGE2_OFFSET);
	img.kernel_buf = ((char *)img.hdr) + ntohl(hdr->kernel_offset);
	img.rootfs_buf = ((char *)img.hdr) + ntohl(hdr->rootfs_offset);

	hdr->bootldr_length = 0;
	memcpy((char *)img.hdr, (char *)hdr, sizeof(struct image_header));


	/* Read kernel image file */
	sprintf(fdata.name, "%s-kernel", fname_in);
	ret = get_file_size(&fdata);
	if (ret) {
		ERROR("Kernel file not found \"%s\".", fdata.name);
		goto out_free_img;
	}

	int tmp = img.hdr->rootfs_offset - img.hdr->kernel_offset;
	if (fdata.size > tmp) { 					/* Kernel size check */
		ERROR("Kernel image too large for image layout.");
		goto out_free_img;
	}

	ret = read_to_buffer(&fdata, img.kernel_buf);
	if (ret) {
		ERROR("Unable to read kernel file \"%s\".", fdata.name);
		goto out_free_img;
	}
	img.hdr->kernel_length = htonl(fdata.size);


	/* Read root filesystem image file */
	sprintf(fdata.name, "%s-rootfs", fname_in);
	ret = get_file_size(&fdata);
	if (ret) {
		ERROR("Rootfs file not found \"%s\".", fdata.name);
		goto out_free_img;
	}

	tmp = max_length - img.hdr->rootfs_offset;
	if (fdata.size > tmp) {						/* Rootfs size check */
		ERROR("Root filesystem too large for image layout.");
		goto out_free_img;
	}

	ret = read_to_buffer(&fdata, img.rootfs_buf);
	if (ret) {
		ERROR("Unable to read rootfs file \"%s\".", fdata.name);
		goto out_free_img;
	}
	img.hdr->rootfs_length = htonl(fdata.size);


	/* Set image length and checksum */
	img.hdr->image_length = htonl(max_length);
	checksum((char *)img.hdr, max_length, 1);


	/* read bootloader image file */
	sprintf(fdata.name, "%s-bootldr", fname_in);
	ret = get_file_size(&fdata);
	if (ret && (ntohl(hdr->bootldr_length) != 0)) {
		WARN("Bootloader length set in header and no bootloader image file was found.");
		WARN("Bootloader will be omitted from image.");
	}

	if (!ret) {
		/* Copy header from inner image */
		memcpy((char *)img_b.hdr, (char *)img.hdr, sizeof(struct image_header));

		tmp = sizeof(struct image_header);
		if ((tmp + fdata.size) > IMAGE2_OFFSET) {
			ERROR("Bootloader image too large for image layout.");
			goto out_free_img;
		}

		ret = read_to_buffer(&fdata, img_b.bootldr_buf);
		if (ret) {
			ERROR("Unable to read bootloader file \"%s\".", fdata.name);
			goto out_free_img;
		}
		img_b.hdr->bootldr_length = htonl(fdata.size);

		/* Set image length and checksum */
		img_b.hdr->image_length = htonl(buf_size);
		checksum((char *)img_b.hdr, buf_size, 1);

		/* Set image pointer and length for output */
		img.hdr = img_b.hdr;
		max_length = buf_size;
	}

	/* Save new firmware image */
	if (fname_out != NULL)
		fdata.name = fname_out;
	else
		sprintf(fdata.name, "%s-new", fname_in);

	fdata.size = max_length;
	ret = write_from_buffer(&fdata, (char *)img.hdr);
	if (ret) {
		ERROR("Unable to create image file \"%s\".", fdata.name);
		goto out_free_img;
	}

	printf("Image file \"%s\" successfully rebuilt.\n", fdata.name);

 out_free_img:
	free(img_buf);

 out_free_hdr:
	free(hdr_buf);

 out_free_name:
	free(new_name);

 out:
	return ret;
}


/*
 * Main Entry Point.
 *
 * Handles options processing and function selection.
 */

int main(int argc, char *argv[])
{
	char *fname_in;
	char *fname_out = NULL;
	int ret = EXIT_FAILURE;
	int cmd, opt;


	while ( 1 ) {
		opt = getopt(argc, argv, "b:x:s:o:h");
		if (opt == -1)
			break;

		switch (opt) {
			case 'b':
			case 'x':
				cmd = opt;
				fname_in = optarg;
				break;
			case 's':
				ret = show_image_header(optarg);
				goto out;
			case 'o':
				fname_out = optarg;
				break;
			case 'h':
			default:
				usage();
				goto out;
		}
	}

	switch (cmd) {
		case 'b':
			ret = build_image(fname_in, fname_out);
			break;
		case 'x':
			ret = extract_image(fname_in, fname_out);
			break;
		default:
			usage();
			break;
	}

 out:
	return ret;
}
