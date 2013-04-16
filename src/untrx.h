/***************************************************************************
 *            untrx.h
 *
 *  Thu Aug 24 19:58:55 2006
 *  Copyright  2006  Jeremy Collake
 *  jeremy@bitsum.com
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _UNTRX_H
#define _UNTRX_H

/*
#ifdef __cplusplus
extern "C"
{
#endif
*/
	
/************************************************************
	Various stuff	
************************************************************/	
	
/* bit size specific type fixes for msvc and other systems */
#ifndef __int8_t_defined
#ifdef __int32
	#typedef u_int32_t unsigned __int32
#else	
	#warning "u_int32_t may be undefined. please define as 32-bit type."
	//#define u_int32_t unsigned long	 /* for 32-bit builds .. */
#endif
#endif


/* stuff for quick OS X compatibility by Jeremy Collake */
#ifndef bswap_32
#define bswap_32 flip_endian
#endif

#ifndef bswap_16
#define bswap_16 flip_endian16

short flip_endian16(short n)
{
	return (n>>8) | (n<<8);
}
#endif

// always flip, regardless of endianness of machine
u_int32_t flip_endian(u_int32_t nValue)
{
	// my crappy endian switch
	u_int32_t nR;
	u_int32_t nByte1=(nValue&0xff000000)>>24;
	u_int32_t nByte2=(nValue&0x00ff0000)>>16;
	u_int32_t nByte3=(nValue&0x0000ff00)>>8;
	u_int32_t nByte4=nValue&0x0ff;
	nR=nByte4<<24;
	nR|=(nByte3<<16);
	nR|=(nByte2<<8);
	nR|=nByte1;
	return nR;
}

#if __BYTE_ORDER == __BIG_ENDIAN
#define STORE32_LE(X)		bswap_32(X)
#define READ32_LE(X)        bswap_32(X)
#define STORE16_LE(X)		bswap_16(X)
#define READ16_LE(X)        bswap_16(X)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define STORE32_LE(X)		(X)
#define READ32_LE(X)        (X)
#define STORE16_LE(X)        (X)
#define READ16_LE(X)        (X)
#else
#error unkown endianness!
#endif

/**********************************************************************/
/* from trxhdr.h */

#define TRX_MAGIC	0x30524448	/* "HDR0" */

typedef struct _trx_header {
	u_int32_t magic;			/* "HDR0" */
	u_int32_t len;			/* Length of file including header */
	u_int32_t crc32;			/* 32-bit CRC from flag_version to end of file */
	u_int32_t flag_version;	/* 0:15 flags, 16:31 version */
	u_int32_t offsets[3];	/* Offsets of partitions from start of header */
} trx_header;

/**********************************************************************/

#define HDR0_SIZE (sizeof(trx_header))

// size of header added by addpattern
#define U2ND_HEADER_SIZE 32	

/* segment stuff */
#define SEGMENT_BASE_NAME "trx_segment"	
typedef enum _SEGMENT_TYPE
{
	SEGMENT_TYPE_UNTYPED,
	SEGMENT_TYPE_SQUASHFS_2_0,
	SEGMENT_TYPE_SQUASHFS_2_1,
	SEGMENT_TYPE_SQUASHFS_2_x,
	SEGMENT_TYPE_SQUASHFS_3_0,
	SEGMENT_TYPE_SQUASHFS_3_1,
	SEGMENT_TYPE_SQUASHFS_3_2,
	SEGMENT_TYPE_SQUASHFS_3_x,
	SEGMENT_TYPE_SQUASHFS_OTHER,
	SEGMENT_TYPE_CRAMFS_x_x,
	SEGMENT_TYPE_CRAMFS_OTHER	
} SEGMENT_TYPE, *PSEGMENT_TYPE;
	
/************************************************************
	squashfs v3.0 stuff (hopefully applies to other versions too)
************************************************************/

typedef long long		squashfs_inode_t;
	
typedef struct squashfs_super_block squashfs_super_block;
typedef squashfs_inode_t squashfs_inode;

#define SQUASHFS_MAJOR			3
#define SQUASHFS_MINOR			0
#define SQUASHFS_MAGIC			0x73717368
#define SQUASHFS_MAGIC_SWAP		0x68737173

/* jc: for dd-wrt build as of 09/10/06
   note: thanks a bunch Brainslayer <sarcasm> */
#define SQUASHFS_MAGIC_ALT		0x74717368
#define SQUASHFS_MAGIC_ALT_SWAP	0x68737174

struct squashfs_super_block {
	unsigned int		s_magic;
	unsigned int		inodes;
	unsigned int		bytes_used_2;
	unsigned int		uid_start_2;
	unsigned int		guid_start_2;
	unsigned int		inode_table_start_2;
	unsigned int		directory_table_start_2;
	unsigned int		s_major:16;
	unsigned int		s_minor:16;
	unsigned int		block_size_1:16;
	unsigned int		block_log:16;
	unsigned int		flags:8;
	unsigned int		no_uids:8;
	unsigned int		no_guids:8;
	unsigned int		mkfs_time /* time of filesystem creation */;
	squashfs_inode_t	root_inode;
	unsigned int		block_size;
	unsigned int		fragments;
	unsigned int		fragment_table_start_2;
	long long		bytes_used;
	long long		uid_start;
	long long		guid_start;
	long long		inode_table_start;
	long long		directory_table_start;
	long long		fragment_table_start;
	long long		unused;
} __attribute__ ((packed));	

#define SQUASHFS_MAJOR			3
#define SQUASHFS_MINOR			0
#define SQUASHFS_MAGIC			0x73717368
#define SQUASHFS_MAGIC_SWAP		0x68737173

// jc: for dd-wrt build as of 09/10/06
#define SQUASHFS_MAGIC_ALT		0x74717368
#define SQUASHFS_MAGIC_ALT_SWAP	0x68737174

/************************************************************
	cramfs v? stuff
************************************************************/
#define CRAMFS_MAGIC 0x28cd3d45
#define CRAMFS_MAGIC_SWAP 0x453dcd28 

struct cramfs_super {
	u_int32_t magic;		/* 0x28cd3d45 - random number */
	u_int32_t size;		/* Not used.  mkcramfs currently
                                   writes a constant 1<<16 here. */
	u_int32_t flags;		/* 0 */
	u_int32_t future;		/* 0 */
	u_int8_t signature[16];	/* "Compressed ROMFS" */
	u_int8_t fsid[16];		/* random number */
	u_int8_t name[16];		/* user-defined name */
	//struct cramfs_inode root;	/* Root inode data */
};

/*
#ifdef __cplusplus
}
#endif
*/

#endif /* _UNTRX_H */
