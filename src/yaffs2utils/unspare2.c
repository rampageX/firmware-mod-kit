/*
 * yaffs2utils: Utilities to make/extract a YAFFS2/YAFFS1 image.
 * Copyright (C) 2010-2011 Luen-Yung Lin <penguin.lin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * unspare2:
 *
 * A tool to dump the OOB layout from the MTD device.
 *
 * Luen-Yung Lin <penguin.lin@gmail.com>
 */

#include "configs.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#ifndef _HAVE_BROKEN_MTD_H
#include <mtd/mtd-user.h>
#else
#include "mtd-abi.h"
#endif

#include "safe_rw.h"
#include "endian_convert.h"

#include "version.h"

/*----------------------------------------------------------------------------*/

#define UNSPARE2_FLAGS_ENDIAN           0x01

#define UNSPARE2_ISENDIAN       (unspare2_flags & UNSPARE2_FLAGS_ENDIAN)

#define UNSPARE2_PRINTF(s, args...) \
		do { \
			fprintf(stdout, s, ##args); \
			fflush(stdout); \
		} while (0)

#define UNSPARE2_ERROR_PRINTF(s, args...) \
		do { \
			fprintf(stderr, s, ##args); \
			fflush(stderr); \
		} while (0)

#define UNSPARE2_HELP(s, args...) \
		UNSPARE2_PRINTF(s, ##args)

#define UNSPARE2_WARN(s, args...) \
		UNSPARE2_ERROR_PRINTF(s, ##args)

#define UNSPARE2_ERROR(s, args...) \
		UNSPARE2_ERROR_PRINTF(s, ##args)

#ifdef _UNSPARE2_DEBUG
#define UNSPARE2_DEBUG(s, args...) \
	UNSPARE2_ERROR_PRINTF("%s: " s, __FUNCTION__, ##args)
#else
#define UNSPARE2_DEBUG(s, args...)
#endif

/*----------------------------------------------------------------------------*/

static unsigned unspare2_flags = 0;

/*----------------------------------------------------------------------------*/

static void
unspare2_endian_convert (nand_ecclayout_t *oob)
{
	unsigned i, eccpos_entries, oobfree_entries;

	eccpos_entries = sizeof(oob->eccpos) / sizeof(unsigned); 
	oobfree_entries = sizeof(oob->oobfree) / sizeof(struct nand_oobfree);

	oob->eccbytes = ENDIAN_SWAP_32(oob->eccbytes);
	oob->oobavail = ENDIAN_SWAP_32(oob->oobavail);
	for (i = 0; i < eccpos_entries; i++)
		oob->eccpos[i] = ENDIAN_SWAP_32(oob->eccpos[i]);
	for (i = 0; i < oobfree_entries; i++) {
		oob->oobfree[i].offset = ENDIAN_SWAP_32(oob->oobfree[i].offset);
		oob->oobfree[i].length = ENDIAN_SWAP_32(oob->oobfree[i].length);
	}
}

/*----------------------------------------------------------------------------*/

static int
unspare2_dump (const char *devfile, const char *imgfile)
{
	int fd, retval = 0;
	ssize_t written;
	nand_ecclayout_t oob;

	/* get the ecc layout via ioctl() */
	/* FIXME: ECCGETLAYOUT is deprecated in the latest kernel. */
	memset(&oob, 0, sizeof(nand_ecclayout_t));

	if ((fd = open(devfile, O_RDWR)) < 0) {
		retval = -1;
		UNSPARE2_ERROR("cannot open the device %s\n", devfile);
	}

	if ((retval = ioctl(fd, ECCGETLAYOUT, &oob)) < 0)
		UNSPARE2_ERROR("ioctl failed\n");

	close(fd);

	if (retval)
		return retval;

	/* endian transform */
	if (UNSPARE2_ISENDIAN)
		unspare2_endian_convert(&oob);

	/* write data back to the file */
	if ((fd = open(imgfile, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
		retval = -1;
		UNSPARE2_ERROR("cannot open the image %s\n", imgfile);
	}

	written = safe_write(fd, &oob, sizeof(nand_ecclayout_t));
	if (written != sizeof(nand_ecclayout_t)) {
		retval = -1;
		UNSPARE2_ERROR("write oob info back to image failed\n");
	}

	close(fd);

	if (retval)
		unlink(imgfile);

	return retval;
}

/*----------------------------------------------------------------------------*/

static void
unspare2_helper (void)
{
	UNSPARE2_HELP("unspare2 %s - A utility to extract the OOB layout\n\n", YAFFS2UTILS_VERSION);
	UNSPARE2_HELP("Usage: unspare2 devfile imgfile\n\n");
	UNSPARE2_HELP("options:\n");
	UNSPARE2_HELP("  -h  display this help message and exit.\n");
	UNSPARE2_HELP("  -e  convert the endian differed from the local machine.\n");
}

/*----------------------------------------------------------------------------*/

int
main (int argc, char *argv[])
{
	int retval;
	char *devpath, *imgpath;

	int option, option_index;
	static const char *short_options = "he";
	static const struct option long_options[] = {
		{"help",	no_argument,	0, 'h'},
		{"endian",	no_argument,	0, 'e'},
		{NULL,		no_argument,	0, '\0'},
	};

	while ((option = getopt_long(argc, argv, short_options,
				     long_options, &option_index)) != EOF)
	{
		switch (option) {
		case 'e':
			unspare2_flags |= UNSPARE2_FLAGS_ENDIAN;
			break;
		case 'h':
			unspare2_helper();
			return 0;
		default:
			unspare2_helper();
			return -1;
		}
	}

	if (argc - optind < 2) {
		unspare2_helper();
		return -1;
	}

	devpath = argv[optind];
	imgpath = argv[optind + 1];

	UNSPARE2_PRINTF("unspare2 %s: OOB extracting tool for yaffs2utils\n",
			YAFFS2UTILS_VERSION);

	if (getuid() != 0)
		UNSPARE2_WARN("warning: non-root users\n");

	retval = unspare2_dump(devpath, imgpath);

	UNSPARE2_PRINTF("\n");
	if (!retval)
		UNSPARE2_PRINTF("OOB info for %s was saved in %s\n", devpath, imgpath);
	else
		UNSPARE2_ERROR("failed");

	return retval;
}
