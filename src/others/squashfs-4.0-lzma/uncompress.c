/*
 * Copyright (c) 2009  Felix Fietkau <nbd@openwrt.org>
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
 * uncompress.c
 */



#ifdef USE_LZMA
#include <LzmaLib.h>
#endif
#include <zlib.h>
#include "squashfs_fs.h"

/* compression algorithm */
int compression = ZLIB_COMPRESSION;


int uncompress_wrapper(unsigned char *dest, unsigned long *dest_len,
	const unsigned char *src, unsigned long src_len)
{
	int res;

#ifdef USE_LZMA
	if (compression == LZMA_COMPRESSION) {
		size_t slen = src_len - LZMA_PROPS_SIZE;
		res = LzmaUncompress((unsigned char *)dest, dest_len,
			(const unsigned char *) src + LZMA_PROPS_SIZE, &slen,
			(const unsigned char *) src, LZMA_PROPS_SIZE);
		switch(res) {
		case SZ_OK:
			res = Z_OK;
			break;
		case SZ_ERROR_MEM:
			res = Z_MEM_ERROR;
			break;
		}
	} else
#endif
	res = uncompress(dest, dest_len, src, src_len);
	return res;
}


