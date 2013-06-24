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

#ifndef __YAFFS2UTILS_ENDIAN_CONVERT_H__
#define __YAFFS2UTILS_ENDIAN_CONVERT_H__

#include "yaffs_packedtags1.h"
#include "yaffs_packedtags2.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <libkern/OSByteOrder.h>
#else
#include <asm/byteorder.h>
#endif


#define ENDIAN_SWAP_32(x)       ((((x) & 0x000000ff) << 24) | \
				(((x) & 0x0000ff00) << 8) | \
				(((x) & 0x00ff0000) >> 8) | \
				(((x) & 0xff000000) >> 24))
#define ENDIAN_SWAP_16(x)       ((((x) & 0x00ff) << 8) | \
				(((x) & 0xff00) >> 8))

void oh_endian_convert (struct yaffs_obj_hdr *oh);
void packedtags1_endian_convert (struct yaffs_packed_tags1 *pt, unsigned reverse);
void packedtags2_tagspart_endian_convert (struct yaffs_packed_tags2 *t);
void packedtags2_eccother_endian_convert (struct yaffs_packed_tags2 *t);

#endif
