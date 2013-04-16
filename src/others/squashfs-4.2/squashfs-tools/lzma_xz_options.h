#ifndef LZMA_XZ_OPTIONS_H
#define LZMA_XZ_OPTIONS_H
/*
 * Copyright (c) 2011
 * Jonas Gorski <jonas.gorski@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * lzma_options.h
 */

#include <stdint.h>

#ifndef linux
#ifdef __FreeBSD__
#include <machine/endian.h>
#endif
#define __BYTE_ORDER BYTE_ORDER
#define __BIG_ENDIAN BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#else
#include <endian.h>
#endif



struct lzma_opts {
	uint32_t flags;
#define LZMA_OPT_FLT_MASK	0xffff
#define LZMA_OPT_PRE_OFF	16
#define LZMA_OPT_PRE_MASK	(0xf << LZMA_OPT_PRE_OFF)
#define LZMA_OPT_EXTREME	20	
	uint16_t bit_opts;
#define LZMA_OPT_LC_OFF		0
#define LZMA_OPT_LC_MASK	(0x7 << LZMA_OPT_LC_OFF)
#define LZMA_OPT_LP_OFF		3
#define LZMA_OPT_LP_MASK	(0x7 << LZMA_OPT_LP_OFF)
#define LZMA_OPT_PB_OFF		6
#define LZMA_OPT_PB_MASK	(0x7 << LZMA_OPT_PB_OFF)
	uint16_t fb;
	uint32_t dict_size;
};

#if __BYTE_ORDER == __BIG_ENDIAN
extern unsigned int inswap_le32(unsigned int);

#define SQUASHFS_INSWAP_LZMA_COMP_OPTS(s) { \
	(s)->flags = inswap_le32((s)->flags); \
	(s)->bit_opts = inswap_le16((s)->bit_opts); \
	(s)->fb = inswap_le16((s)->fb); \
	(s)->dict_size = inswap_le32((s)->dict_size); \
}
#else
#define SQUASHFS_INSWAP_LZMA_COMP_OPTS(s)
#endif

#define MEMLIMIT (32 * 1024 * 1024)

#define LZMA_OPT_LC_MIN		0
#define LZMA_OPT_LC_MAX		4
#define LZMA_OPT_LC_DEFAULT	3

#define LZMA_OPT_LP_MIN		0
#define LZMA_OPT_LP_MAX		4
#define LZMA_OPT_LP_DEFAULT	0

#define LZMA_OPT_PB_MIN		0
#define LZMA_OPT_PB_MAX		4
#define LZMA_OPT_PB_DEFAULT	2

#define LZMA_OPT_FB_MIN		5
#define LZMA_OPT_FB_MAX		273
#define LZMA_OPT_FB_DEFAULT	64

enum {
	LZMA_OPT_LZMA = 1,
	LZMA_OPT_XZ
};

struct lzma_xz_options {
	int preset;
	int extreme;
	int lc;
	int lp;
	int pb;
	int fb;
	int dict_size;
	int flags;
};

struct lzma_xz_options *lzma_xz_get_options(void);

int lzma_xz_options(char *argv[], int argc, int lzmaver);

int lzma_xz_options_post(int block_size, int lzmaver);

void *lzma_xz_dump_options(int block_size, int *size, int flags);

int lzma_xz_extract_options(int block_size, void *buffer, int size, int lzmaver);

void lzma_xz_usage(int lzmaver);

#endif
