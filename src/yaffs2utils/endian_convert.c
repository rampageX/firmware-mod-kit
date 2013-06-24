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
 
#include <stdio.h>
#include <string.h>

#include "endian_convert.h"

/*-------------------------------------------------------------------------*/

void 
oh_endian_convert (struct yaffs_obj_hdr *oh)
{
	oh->type = ENDIAN_SWAP_32(oh->type); // GCC makes enums 32 bits.
	oh->parent_obj_id = ENDIAN_SWAP_32(oh->parent_obj_id); // int
	// __u16 - Not used, but done for completeness.
	oh->sum_no_longer_used = ENDIAN_SWAP_16(oh->sum_no_longer_used);
	oh->yst_mode = ENDIAN_SWAP_32(oh->yst_mode);

#ifdef CONFIG_YAFFS_WINCE 
	/* 
	 * WinCE doesn't implement this, but we need to just in case. 
	 * In fact, WinCE would be *THE* place where this would be an issue!
	 */
	oh->not_for_wince[0] = ENDIAN_SWAP_32(oh->not_for_wince[0]);
	oh->not_for_wince[1] = ENDIAN_SWAP_32(oh->not_for_wince[1]);
	oh->not_for_wince[2] = ENDIAN_SWAP_32(oh->not_for_wince[2]);
	oh->not_for_wince[3] = ENDIAN_SWAP_32(oh->not_for_wince[3]);
	oh->not_for_wince[4] = ENDIAN_SWAP_32(oh->not_for_wince[4]);
#else
	// Regular POSIX.
	oh->yst_uid = ENDIAN_SWAP_32(oh->yst_uid);
	oh->yst_gid = ENDIAN_SWAP_32(oh->yst_gid);
	oh->yst_atime = ENDIAN_SWAP_32(oh->yst_atime);
	oh->yst_mtime = ENDIAN_SWAP_32(oh->yst_mtime);
	oh->yst_ctime = ENDIAN_SWAP_32(oh->yst_ctime);
#endif

	oh->file_size_low = ENDIAN_SWAP_32(oh->file_size_low);
	oh->equiv_id = ENDIAN_SWAP_32(oh->equiv_id);
	oh->yst_rdev = ENDIAN_SWAP_32(oh->yst_rdev);

	oh->win_ctime[0] = ENDIAN_SWAP_32(oh->win_ctime[0]);
	oh->win_ctime[1] = ENDIAN_SWAP_32(oh->win_ctime[1]);
	oh->win_atime[0] = ENDIAN_SWAP_32(oh->win_atime[0]);
	oh->win_atime[1] = ENDIAN_SWAP_32(oh->win_atime[1]);
	oh->win_mtime[0] = ENDIAN_SWAP_32(oh->win_mtime[0]);
	oh->win_mtime[1] = ENDIAN_SWAP_32(oh->win_mtime[1]);

	oh->inband_shadowed_obj_id = ENDIAN_SWAP_32(oh->inband_shadowed_obj_id);
	oh->inband_is_shrink = ENDIAN_SWAP_32(oh->inband_is_shrink);
	oh->file_size_high = ENDIAN_SWAP_32(oh->file_size_high);
	oh->reserved[0] = ENDIAN_SWAP_32(oh->reserved[0]);
	oh->shadows_obj = ENDIAN_SWAP_32(oh->shadows_obj);
	oh->is_shrink = ENDIAN_SWAP_32(oh->is_shrink);
}

/*-------------------------------------------------------------------------*/

void
packedtags1_endian_convert (struct yaffs_packed_tags1 *pt, unsigned reverse)
{
	union yaffs_tags_union *tags = (union yaffs_tags_union *)pt;
	union yaffs_tags_union temp;
	unsigned char *pb = tags->as_bytes;
	unsigned char *tb = temp.as_bytes;

	memset(&temp, 0, sizeof(union yaffs_tags_union));

	/* I really hate these */
	if (!reverse) {
#if defined(__LITTLE_ENDIAN_BITFIELD)
		tb[0] = ((pb[2] & 0x0f) << 4) |
			((pb[1] & 0xf0) >> 4);
		tb[1] = ((pb[1] & 0x0f) << 4) |
			((pb[0] & 0xf0) >> 4);
		tb[2] = ((pb[0] & 0x0f) << 4) |
			((pb[2] & 0x30) >> 2) |
			((pb[3] & 0xc0) >> 6);
		tb[3] = ((pb[3] & 0x3f) << 2) |
			((pb[2] & 0xc0) >> 6);
		tb[4] = ((pb[6] & 0x03) << 6) |
			((pb[5] & 0xfc) >> 2);
		tb[5] = ((pb[5] & 0x03) << 6) |
			((pb[4] & 0xfc) >> 2);
		tb[6] = ((pb[4] & 0x03) << 6) |
			(pb[7] & 0x3f);
		tb[7] = (pb[6] & 0xfc) |
			((pb[7] & 0x40) >> 5) |
			((pb[7] & 0x80) >> 7);
#elif defined(__BIG_ENDIAN_BITFIELD)
		tb[0] = ((pb[2] & 0xf0) >> 4) |
			((pb[1] & 0x0f) << 4);
		tb[1] = ((pb[1] & 0xf0) >> 4) |
			((pb[0] & 0x0f) << 4);
		tb[2] = ((pb[0] & 0xf0) >> 4) |
			((pb[2] & 0x0c) << 2) |
			((pb[3] & 0x03) << 6);
		tb[3] = ((pb[3] & 0xfc) >> 2) |
			((pb[2] & 0x03) << 6);
		tb[4] = ((pb[6] & 0xc0) >> 6) |
			((pb[5] & 0x3f) << 2);
		tb[5] = ((pb[5] & 0xc0) >> 6) |
			((pb[4] & 0x3f) << 2);
		tb[6] = ((pb[4] & 0xc0) >> 6) |
			(pb[7] & 0xfc);
		tb[7] = (pb[6] & 0x3f) |
			((pb[7] & 0x02) << 5) |
			((pb[7] & 0x01) << 7);
#endif
	}
	else {
#if defined(__LITTLE_ENDIAN_BITFIELD)
		tb[0] = ((pb[2] & 0xf0) >> 4) |
			((pb[1] & 0x0f) << 4);
		tb[1] = ((pb[1] & 0xf0) >> 4) |
			((pb[0] & 0x0f) << 4);
		tb[2] = ((pb[0] & 0xf0) >> 4) |
			((pb[2] & 0x0C) << 2) |
			((pb[3] & 0x03) << 6);
		tb[3] = ((pb[3] & 0xfc) >> 2) |
			((pb[2] & 0x03) << 6);
		tb[4] = ((pb[6] & 0xc0) >> 6) |
			((pb[5] & 0x3f) << 2);
		tb[5] = ((pb[5] & 0xc0) >> 6) |
			((pb[4] & 0x3f) << 2);
		tb[6] = ((pb[4] & 0xc0) >> 6) |
			(pb[7] & 0xfc);
		tb[7] = (pb[6] & 0x3f) |
			((pb[7] & 0x02) << 5) |
			((pb[7] & 0x01) << 7);
#elif defined(__BIG_ENDIAN_BITFIELD)
		tb[0] = ((pb[2] & 0x0f) << 4) |
			((pb[1] & 0xf0) >> 4);
		tb[1] = ((pb[1] & 0x0f) << 4) |
			((pb[0] & 0xf0) >> 4);
		tb[2] = ((pb[0] & 0x0f) << 4) |
			((pb[2] & 0x30) >> 2) |
			((pb[3] & 0xc0) >> 6);
		tb[3] = ((pb[3] & 0x3f) << 2) |
			((pb[2] & 0xc0) >> 6);
		tb[4] = ((pb[6] & 0x03) << 6) |
			((pb[5] & 0xfc) >> 2);
		tb[5] = ((pb[5] & 0x03) << 6) |
			((pb[4] & 0xfc) >> 2);
		tb[6] = ((pb[4] & 0x03) << 6) |
			(pb[7] & 0x3f);
		tb[7] = (pb[6] & 0xfc) |
			((pb[7] & 0x40) >> 5) |
			((pb[7] & 0x80) >> 7);
#endif
	}

	// Now copy it back.
	pb[0] = tb[0];
	pb[1] = tb[1];
	pb[2] = tb[2];
	pb[3] = tb[3];
	pb[4] = tb[4];
	pb[5] = tb[5];
	pb[6] = tb[6];
	pb[7] = tb[7];
}

/*-------------------------------------------------------------------------*/

void
packedtags2_tagspart_endian_convert (struct yaffs_packed_tags2 *t)
{
	struct yaffs_packed_tags2_tags_only *tp = &t->t;

	tp->seq_number = ENDIAN_SWAP_32(tp->seq_number);
	tp->obj_id = ENDIAN_SWAP_32(tp->obj_id);
	tp->chunk_id = ENDIAN_SWAP_32(tp->chunk_id);
	tp->n_bytes = ENDIAN_SWAP_32(tp->n_bytes);	
}

void 
packedtags2_eccother_endian_convert (struct yaffs_packed_tags2 *t)
{
	struct yaffs_ecc_other *e = &t->ecc;

	e->line_parity = ENDIAN_SWAP_32(e->line_parity);
	e->line_parity_prime = ENDIAN_SWAP_32(e->line_parity_prime);
}
