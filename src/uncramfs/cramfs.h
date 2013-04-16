#ifndef __CRAMFS_H
#define __CRAMFS_H

#define CRAMFS_MAGIC		0x28cd3d45	/* some random number */
#define CRAMFS_SIGNATURE	"Compressed ROMFS"

/*
 * Reasonably terse representation of the inode data.
 */
struct cramfs_inode {
	u32 mode:16, uid:16;
	/* SIZE for device files is i_rdev */
	u32 size:24, gid:8;
	/* NAMELEN is the length of the file name, divided by 4 and
           rounded up.  (cramfs doesn't support hard links.) */
	/* OFFSET: For symlinks and non-empty regular files, this
	   contains the offset (divided by 4) of the file data in
	   compressed form (starting with an array of block pointers;
	   see README).  For non-empty directories it is the offset
	   (divided by 4) of the inode of the first file in that
	   directory.  For anything else, offset is zero. */
	u32 namelen:6, offset:26;
};

/*
 * Superblock information at the beginning of the FS.
 */
struct cramfs_super {
	u32 magic;		/* 0x28cd3d45 - random number */
	u32 size;		/* Not used.  mkcramfs currently
                                   writes a constant 1<<16 here. */
	u32 flags;		/* 0 */
	u32 future;		/* 0 */
	u8 signature[16];	/* "Compressed ROMFS" */
	u8 fsid[16];		/* random number */
	u8 name[16];		/* user-defined name */
	struct cramfs_inode root;	/* Root inode data */
};

/*
 * Valid values in super.flags.  Currently we refuse to mount
 * if (flags & ~CRAMFS_SUPPORTED_FLAGS).  Maybe that should be
 * changed to test super.future instead.
 */
#define CRAMFS_SUPPORTED_FLAGS (0xff)

/* Uncompression interfaces to the underlying zlib */
int cramfs_uncompress_block(void *dst, int dstlen, void *src, int srclen);
int cramfs_uncompress_init(void);
int cramfs_uncompress_exit(void);

#endif
