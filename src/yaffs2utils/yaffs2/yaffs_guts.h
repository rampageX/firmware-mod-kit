/*
 * YAFFS: Yet another Flash File System . A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2011 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 */

#ifndef __YAFFS_GUTS_H__
#define __YAFFS_GUTS_H__

#include "yaffs_utils.h"

/*
 * Tnodes form a tree with the tnodes in "levels"
 * Levels greater than 0 hold 8 slots which point to other tnodes.
 * Those at level 0 hold 16 slots which point to chunks in NAND.
 *
 * A maximum level of 8 thust supports files of size up to:
 *
 * 2^(3*MAX_LEVEL+4)
 *
 * Thus a max level of 8 supports files with up to 2^^28 chunks which gives
 * a maximum file size of arounf 51Gbytees with 2k chunks.
 */
#define YAFFS_NTNODES_LEVEL0		16
#define YAFFS_TNODES_LEVEL0_BITS	4
#define YAFFS_TNODES_LEVEL0_MASK	0xf

#define YAFFS_NTNODES_INTERNAL		(YAFFS_NTNODES_LEVEL0 / 2)
#define YAFFS_TNODES_INTERNAL_BITS	(YAFFS_TNODES_LEVEL0_BITS - 1)
#define YAFFS_TNODES_INTERNAL_MASK	0x7
#define YAFFS_TNODES_MAX_LEVEL		8
#define YAFFS_TNODES_MAX_BITS		(YAFFS_TNODES_LEVEL0_BITS + \
					YAFFS_TNODES_INTERNAL_BITS * \
					YAFFS_TNODES_MAX_LEVEL)
#define YAFFS_MAX_CHUNK_ID		((1 << YAFFS_TNODES_MAX_BITS) - 1)

#define YAFFS_NOBJECT_BUCKETS		256

#define YAFFS_OBJECT_SPACE		0x40000
#define YAFFS_MAX_OBJECT_ID		(YAFFS_OBJECT_SPACE - 1)

#ifdef CONFIG_YAFFS_UNICODE
#define YAFFS_MAX_NAME_LENGTH		127
#define YAFFS_MAX_ALIAS_LENGTH		79
#else
#define YAFFS_MAX_NAME_LENGTH		255
#define YAFFS_MAX_ALIAS_LENGTH		159
#endif

#define YAFFS_SHORT_NAME_LENGTH		15

/* Some special object ids for pseudo objects */
#define YAFFS_OBJECTID_ROOT		1
#define YAFFS_OBJECTID_LOSTNFOUND	2
#define YAFFS_OBJECTID_UNLINKED		3
#define YAFFS_OBJECTID_DELETED		4

/* Fake object Id for summary data */
#define YAFFS_OBJECTID_SUMMARY		0x10

/* Sequence numbers are used in YAFFS2 to determine block allocation order.
 * The range is limited slightly to help distinguish bad numbers from good.
 * This also allows us to perhaps in the future use special numbers for
 * special purposes.
 * EFFFFF00 allows the allocation of 8 blocks per second (~1Mbytes) for 15 years,
 * and is a larger number than the lifetime of a 2GB device.
 */
#define YAFFS_LOWEST_SEQUENCE_NUMBER	0x00001000
#define YAFFS_HIGHEST_SEQUENCE_NUMBER	0xefffff00

/* Special sequence number for bad block that failed to be marked bad */
#define YAFFS_SEQUENCE_BAD_BLOCK	0xffff0000

/* -------------------------------------------------------------------------- */

struct yaffs_tags {
	unsigned chunk_id:20;
	unsigned serial_number:2;
	unsigned n_bytes_lsb:10;
	unsigned obj_id:18;
	unsigned ecc:12;
	unsigned n_bytes_msb:2;
};

union yaffs_tags_union {
	struct yaffs_tags as_tags;
	u8 as_bytes[8];
} __attribute__((__may_alias__));

enum yaffs_ecc_result {
	YAFFS_ECC_RESULT_UNKNOWN,
	YAFFS_ECC_RESULT_NO_ERROR,
	YAFFS_ECC_RESULT_FIXED,
	YAFFS_ECC_RESULT_UNFIXED
};

enum yaffs_obj_type {
	YAFFS_OBJECT_TYPE_UNKNOWN,
	YAFFS_OBJECT_TYPE_FILE,
	YAFFS_OBJECT_TYPE_SYMLINK,
	YAFFS_OBJECT_TYPE_DIRECTORY,
	YAFFS_OBJECT_TYPE_HARDLINK,
	YAFFS_OBJECT_TYPE_SPECIAL
};

/* Special types extension used by yaffs2utils internals */
enum yaffs_obj_type_special {
	YAFFS_OBJECT_TYPE_CHR = YAFFS_OBJECT_TYPE_SPECIAL,
	YAFFS_OBJECT_TYPE_BLK,
	YAFFS_OBJECT_TYPE_FIFO,
	YAFFS_OBJECT_TYPE_SOCK,
};

struct yaffs_ext_tags {
	unsigned chunk_used;	/* Status of the chunk: used or unused */
	unsigned obj_id;	/* If 0 this is unused */
	unsigned chunk_id;	/* If 0 this is a header, else a data chunk */
	unsigned n_bytes;	/* Only valid for data chunks */

	/* The following stuff only has meaning when we read */
	enum yaffs_ecc_result ecc_result;
	unsigned block_bad;

	/* YAFFS 1 stuff */
	unsigned is_deleted;	/* The chunk is marked deleted */
	unsigned serial_number;	/* Yaffs1 2-bit serial number */

	/* YAFFS2 stuff */
	unsigned seq_number;	/* The sequence number of this block */

	/* Extra info if this is an object header (YAFFS2 only) */

	unsigned extra_available;	/* Extra info available if not zero */
	unsigned extra_parent_id;	/* The parent object */
	unsigned extra_is_shrink;	/* Is it a shrink header? */
	unsigned extra_shadows;		/* Does this shadow another object? */

	enum yaffs_obj_type extra_obj_type;	/* What object type? */

	loff_t extra_file_size;		/* Length if it is a file */
	unsigned extra_equiv_id;	/* Equivalent object Id if it is a hard link */
};

struct yaffs_obj_hdr {
	enum yaffs_obj_type type;

	/* Apply to everything  */
	int parent_obj_id;
	u16 sum_no_longer_used;	/* checksum of name. No longer used */
	YCHAR name[YAFFS_MAX_NAME_LENGTH + 1];

	/* The following apply to all object types except for hard links */
	u32 yst_mode;		/* protection */

	u32 yst_uid;
	u32 yst_gid;
	u32 yst_atime;
	u32 yst_mtime;
	u32 yst_ctime;

	/* File size  applies to files only */
	u32 file_size_low;

	/* Equivalent object id applies to hard links only. */
	int equiv_id;

	/* Alias is for symlinks only. */
	YCHAR alias[YAFFS_MAX_ALIAS_LENGTH + 1];

	u32 yst_rdev;		/* stuff for block and char devices (major/min) */

	u32 win_ctime[2];
	u32 win_atime[2];
	u32 win_mtime[2];

	u32 inband_shadowed_obj_id;
	u32 inband_is_shrink;

	u32 file_size_high;
	u32 reserved[1];
	int shadows_obj;	/* This object header shadows the
				   specified object if > 0 */

	/* is_shrink applies to object headers written when wemake a hole. */
	u32 is_shrink;
};

#endif
