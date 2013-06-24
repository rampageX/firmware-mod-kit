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
 
#include "configs.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <getopt.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#ifdef _HAVE_OSX_SYSLIMITS
#include <sys/syslimits.h>
#endif

#include "yaffs_trace.h"
#include "yaffs_packedtags1.h"
#include "yaffs_packedtags2.h"

#include "list.h"
#include "safe_rw.h"
#include "progress_bar.h"
#include "endian_convert.h"
#include "nand_ecclayout.h"

#include "version.h"

/*----------------------------------------------------------------------------*/

#define MKYAFFS2_OBJTABLE_SIZE	4096

#define MKYAFFS2_FLAGS_NONROOT	(1 << 0)
#define MKYAFFS2_FLAGS_SHOWBAR	(1 << 1)
#define MKYAFFS2_FLAGS_YAFFS1	(1 << 16)
#define MKYAFFS2_FLAGS_ENDIAN	(1 << 17)
#define MKYAFFS2_FLAGS_YAFFSECC	(1 << 18)
#define MKYAFFS2_FLAGS_ALLROOT	(1 << 19)
#define MKYAFFS2_FLAGS_VERBOSE	(1 << 20)

#define MKYAFFS2_ISSHOWBAR	(mkyaffs2_flags & MKYAFFS2_FLAGS_SHOWBAR)
#define MKYAFFS2_ISYAFFS1	(mkyaffs2_flags & MKYAFFS2_FLAGS_YAFFS1)
#define MKYAFFS2_ISENDIAN	(mkyaffs2_flags & MKYAFFS2_FLAGS_ENDIAN)
#define MKYAFFS2_ISYAFFSECC	(mkyaffs2_flags & MKYAFFS2_FLAGS_YAFFSECC)
#define MKYAFFS2_ISALLROOT	(mkyaffs2_flags & MKYAFFS2_FLAGS_ALLROOT)
#define MKYAFFS2_ISVERBOSE	(mkyaffs2_flags & MKYAFFS2_FLAGS_VERBOSE)

#define MKYAFFS2_PRINTF(s, args...) \
		do { \
			if (!MKYAFFS2_ISVERBOSE && MKYAFFS2_ISSHOWBAR) { \
				mkyaffs2_flags &= ~MKYAFFS2_FLAGS_SHOWBAR; \
				fprintf(stdout, "\n"); \
			} \
			fprintf(stdout, s, ##args); \
			fflush(stdout); \
		} while (0)

#define MKYAFFS2_ERROR_PRINTF(s, args...) \
		do { \
			if (!MKYAFFS2_ISVERBOSE && MKYAFFS2_ISSHOWBAR) { \
				mkyaffs2_flags &= ~MKYAFFS2_FLAGS_SHOWBAR; \
				fprintf(stderr, "\n"); \
			} \
			fprintf(stderr, s, ##args); \
			fflush(stderr); \
		} while (0)

#define MKYAFFS2_HELP(s, args...) \
		MKYAFFS2_PRINTF(s, ##args)

#define MKYAFFS2_WARN(s, args...) \
		MKYAFFS2_ERROR_PRINTF(s, ##args)

#define MKYAFFS2_ERROR(s, args...) \
		MKYAFFS2_ERROR_PRINTF(s, ##args)

#ifdef _MKYAFFS2_DEBUG
#define MKYAFFS2_DEBUG(s, args...) \
		MKYAFFS2_ERROR_PRINTF("%s: " s, __FUNCTION__, ##args)
#else
#define MKYAFFS2_DEBUG(s, args...)
#endif

#define MKYAFFS2_VERBOSE(s, args...) \
		do { \
			if (MKYAFFS2_ISVERBOSE) \
				MKYAFFS2_PRINTF(s, ##args); \
		} while (0)

#define MKYAFFS2_PROGRESS_INIT() \
		do { \
			if (!MKYAFFS2_ISVERBOSE) \
				progress_init(); \
		} while (0)

#define MKYAFFS2_PROGRESS_BAR(objs, total) \
		do { \
			if (!MKYAFFS2_ISVERBOSE) { \
				mkyaffs2_flags |= MKYAFFS2_FLAGS_SHOWBAR; \
				progress_bar(objs, total); \
			} \
		} while(0)

/*----------------------------------------------------------------------------*/

typedef struct mkyaffs2_obj {
	dev_t dev;
	ino_t ino;

	unsigned obj_id;
	struct mkyaffs2_obj *parent_obj;

	unsigned type;

	char name[NAME_MAX + 1];

	struct list_head children;	/* for a directory */
	struct list_head siblings;	/* neighbors in the same directory */
	struct list_head hashlist;	/* hash table */
} mkyaffs2_obj_t;

typedef struct mkyaffs2_fstree {
	unsigned objs;
	struct mkyaffs2_obj *root;
} mkyaffs2_fstree_t;

/*----------------------------------------------------------------------------*/

static unsigned mkyaffs2_flags = 0;

static unsigned mkyaffs2_chunksize = 0;
static unsigned mkyaffs2_sparesize = 0;

static unsigned mkyaffs2_image_obj_id = YAFFS_NOBJECT_BUCKETS;
static unsigned mkyaffs2_image_objs = 0;
static unsigned mkyaffs2_image_pages = 0;

static int mkyaffs2_image_fd = -1;

static char mkyaffs2_curfile[PATH_MAX + PATH_MAX] = {0};

static nand_ecclayout_t *mkyaffs2_ecclayout = NULL;

static unsigned mkyaffs2_bufsize = 0;
static unsigned char *mkyaffs2_databuf = NULL;

static struct mkyaffs2_fstree mkyaffs2_objtree = {0};
static struct list_head mkyaffs2_objtable[MKYAFFS2_OBJTABLE_SIZE];

static int
(*mkyaffs2_assemble_ptags) (unsigned char *, struct yaffs_ext_tags *,
			    nand_ecclayout_t *, int) = NULL;

/*----------------------------------------------------------------------------*/

static struct mkyaffs2_obj *
mkyaffs2_obj_alloc (void)
{
	struct mkyaffs2_obj *obj;

	obj = calloc(sizeof(struct mkyaffs2_obj), sizeof(unsigned char));
	if (obj == NULL)
		return NULL;

	obj->parent_obj = obj;

	INIT_LIST_HEAD(&obj->hashlist);
	INIT_LIST_HEAD(&obj->children);
	INIT_LIST_HEAD(&obj->siblings);

	return obj;
}

static void
mkyaffs2_obj_free (struct mkyaffs2_obj *object)
{
	list_del(&object->children);
	list_del(&object->siblings);
	list_del(&object->hashlist);

	free(object);
}

/*----------------------------------------------------------------------------*/

static inline unsigned
mkyaffs2_objtable_hash (unsigned hash)
{
	return hash % MKYAFFS2_OBJTABLE_SIZE;
}

static inline void
mkyaffs2_objtable_insert (struct mkyaffs2_obj *object)
{
	unsigned n = mkyaffs2_objtable_hash(object->ino);
	list_add_tail(&object->hashlist, &mkyaffs2_objtable[n]);
}

static inline struct mkyaffs2_obj *
mkyaffs2_objtable_find (dev_t dev, ino_t ino)
{
	unsigned n = mkyaffs2_objtable_hash(ino);
	struct list_head *p;
	struct mkyaffs2_obj *obj;

	list_for_each(p, &mkyaffs2_objtable[n]) {
		obj = list_entry(p, mkyaffs2_obj_t, hashlist);
		if (obj->dev == dev && obj->ino == ino)
			return obj;
	}

	return NULL;
}

static inline void
mkyaffs2_objtable_init (void)
{
	unsigned n;

	for (n = 0; n < MKYAFFS2_OBJTABLE_SIZE; n++)
		INIT_LIST_HEAD(&mkyaffs2_objtable[n]);
}

static inline void
mkyaffs2_objtable_exit (void)
{
	unsigned i;
	struct mkyaffs2_obj *obj;
	struct list_head *p, *n;

	for (i = 0; i < MKYAFFS2_OBJTABLE_SIZE; i++) {
		list_for_each_safe(p, n, &mkyaffs2_objtable[i]) {
			obj = list_entry(p, struct mkyaffs2_obj, hashlist);
			mkyaffs2_obj_free(obj);
		}
	}
}

/*----------------------------------------------------------------------------*/

static void
mkyaffs2_objtree_cleanup (struct mkyaffs2_obj *object)
{
	struct mkyaffs2_obj *obj;
	struct list_head *p, *n;

	if (object == NULL)
		return;

	list_for_each_safe(p, n, &object->children) {
		obj = list_entry(p, mkyaffs2_obj_t, siblings);
		mkyaffs2_objtree_cleanup(obj);
	}

	mkyaffs2_obj_free(object);
}

static struct mkyaffs2_fstree *
mkyaffs2_objtree_init (struct mkyaffs2_fstree *fst)
{
	struct mkyaffs2_fstree *f = fst;

	if (f == NULL) {
		f = malloc(sizeof(struct mkyaffs2_fstree));
		if (f == NULL)
			return NULL;
	}

	/* initialize */
	memset(f, 0, sizeof(struct mkyaffs2_fstree));

	return f;
}

static struct mkyaffs2_fstree *
mkyaffs2_objtree_init2 (struct mkyaffs2_fstree *fst, struct mkyaffs2_obj *root)
{
	struct mkyaffs2_fstree *f = fst;

	f = mkyaffs2_objtree_init(f);
	if (f && root) {
		f->root = root;
		f->objs++;
	}

	return f;
}

static void
mkyaffs2_objtree_exit (struct mkyaffs2_fstree *fst)
{
	mkyaffs2_objtree_cleanup(fst->root);
}

/*----------------------------------------------------------------------------*/

static void 
mkyaffs2_packedtags1_ecc (struct yaffs_packed_tags1 *pt)
{
	unsigned char *b = ((union yaffs_tags_union *)pt)->as_bytes;
	unsigned i, j;
	unsigned ecc = 0, bit = 0;

	/* clear the ecc field */
	if (MKYAFFS2_ISYAFFS1) {
#if defined(__LITTLE_ENDIAN_BITFIELD)
		b[6] &= 0xc0;
		b[7] &= 0x03;
#elif defined(__BIG_ENDIAN_BITFIELD)
		b[6] &= 0x03;
		b[7] &= 0xc0;
#endif
	}
	else
		pt->ecc = 0;

	/* calculate ecc */
	for (i = 0; i < 8; i++) {
		for (j = 1; j & 0xff; j <<= 1) {
			bit++;
			if (b[i] & j)
				ecc ^= bit;
		}
	}

	/* write ecc back to tags */
	if (MKYAFFS2_ISENDIAN) {
#if defined(__LITTLE_ENDIAN_BITFIELD)
		b[6] |= ((ecc >> 6) & 0x3f);
		b[7] |= ((ecc & 0x3f) << 2);
#elif defined(__BIG_ENDIAN_BITFIELD)
		b[6] |= ((ecc & 0x3f) << 2);
		b[7] |= ((ecc >> 6) & 0x3f);
#endif
	}
	else
		pt->ecc = ecc;
}

static ssize_t
mkyaffs2_ptags2spare (unsigned char *spare, unsigned char *tag, size_t bytes,
		      nand_ecclayout_t *ecclayout)
{
	unsigned i;
	size_t copied = 0;
	struct nand_oobfree *oobfree = ecclayout->oobfree;

	for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES && copied < bytes; i++) {
		size_t size = bytes - copied;
		unsigned char *s = spare + oobfree[i].offset;

		if (size > oobfree[i].length)
			size = oobfree[i].length;

		memcpy(s, tag, size);

		copied += size;
		tag += size;
	}

	return copied;
}

/*----------------------------------------------------------------------------*/

static int
mkyaffs2_assemble_ptags1(unsigned char *spare, struct yaffs_ext_tags *t,
			 nand_ecclayout_t *ecclayout, int ecc)
{
	ssize_t written;
	struct yaffs_packed_tags1 pt1;
	nand_ecclayout_t *l = ecclayout ? ecclayout : mkyaffs2_ecclayout;

	memset(&pt1, 0xff, sizeof(struct yaffs_packed_tags1));
	yaffs_pack_tags1(&pt1, t);

	if (MKYAFFS2_ISENDIAN)
		packedtags1_endian_convert(&pt1, 0);

	mkyaffs2_packedtags1_ecc(&pt1);

	written = mkyaffs2_ptags2spare(spare, (unsigned char *)&pt1,
				       sizeof(struct yaffs_packed_tags1), l);

	written += sizeof(((struct yaffs_packed_tags1 *)0)->should_be_ff);

	return written < sizeof(struct yaffs_packed_tags1);
}

static int
mkyaffs2_assemble_ptags2(unsigned char *spare, struct yaffs_ext_tags *t,
			 nand_ecclayout_t *ecclayout, int ecc)
{
	ssize_t written;
	struct yaffs_packed_tags2 pt2;
	nand_ecclayout_t *l = ecclayout ? ecclayout : mkyaffs2_ecclayout;

	memset(&pt2, 0xff, sizeof(struct yaffs_packed_tags2));
	yaffs_pack_tags2_tags_only(&pt2.t, t);

	if (MKYAFFS2_ISENDIAN)
		packedtags2_tagspart_endian_convert(&pt2);

	if (ecc) {
		yaffs_ecc_calc_other((unsigned char *)&pt2.t,
				     sizeof(struct yaffs_packed_tags2_tags_only),
				     &pt2.ecc);

		if (MKYAFFS2_ISENDIAN)
			packedtags2_eccother_endian_convert(&pt2);
	}

	written = mkyaffs2_ptags2spare(spare, (unsigned char *)&pt2,
				       sizeof(struct yaffs_packed_tags2), l);

	return written != sizeof(struct yaffs_packed_tags2);
}

static int
mkyaffs2_write_chunk (unsigned obj_id, unsigned chunk_id, unsigned bytes)
{
	ssize_t written;
	unsigned char *spare = mkyaffs2_databuf + mkyaffs2_chunksize;

	struct yaffs_ext_tags tag;

	/* prepare the spare (oob) first */
	memset(&tag, 0, sizeof(struct yaffs_ext_tags));

	/* common */
	tag.obj_id = obj_id;
	tag.chunk_id = chunk_id;
	tag.n_bytes = bytes;

	/* yaffs1 only */
	tag.is_deleted = 0;
	tag.serial_number = 1;

	/* yaffs2 only */
	tag.chunk_used = 1;
	tag.seq_number = YAFFS_LOWEST_SEQUENCE_NUMBER;


	/* write the spare (oob) into the buffer */
	memset(spare, 0xff, mkyaffs2_sparesize);
	if (mkyaffs2_assemble_ptags(spare, &tag, NULL, 1)) {
		MKYAFFS2_DEBUG("tag to spare failed for obj %u chunk %u\n",
				obj_id, chunk_id);
		return -1;
	}

	/* write a whole "chunk + spare" back to the image */
	written = safe_write(mkyaffs2_image_fd,
			     mkyaffs2_databuf, mkyaffs2_bufsize);
	if (written != mkyaffs2_bufsize) {
		MKYAFFS2_DEBUG("write chunk failed for obj %u chunk %u: %s\n",
				obj_id, chunk_id, strerror(errno));
		return -1;
	}

	mkyaffs2_image_pages++;

	return 0;
}

static int 
mkyaffs2_write_oh (struct yaffs_obj_hdr *oh, struct mkyaffs2_obj *obj)
{
	if (MKYAFFS2_ISENDIAN)
 	   	oh_endian_convert(oh);

	/* copy header into the buffer */
	memset(mkyaffs2_databuf, 0xff, mkyaffs2_chunksize);
	memcpy(mkyaffs2_databuf, oh, sizeof(struct yaffs_obj_hdr));

	/* write buffer */
	return mkyaffs2_write_chunk(obj->obj_id, 0, 0xffff);
}

static int 
mkyaffs2_write_regfile (const char *fpath, struct mkyaffs2_obj *obj)
{
	int fd, retval = 0;
	unsigned chunk = 0;
	unsigned char *databuf = mkyaffs2_databuf;
	ssize_t bytes;

	fd = open(fpath, O_RDONLY);
	if (fd < 0) {
		MKYAFFS2_DEBUG("cannot open the file: '%s'\n", fpath);
		return -1;
	}

	memset(databuf, 0xff, mkyaffs2_chunksize);
	while((bytes = safe_read(fd, databuf, mkyaffs2_chunksize)) != 0) {
		if (bytes < 0) {
			MKYAFFS2_DEBUG("error while reading file '%s': %s\n",
					fpath, strerror(errno));
			retval = bytes;
			break;
		}

		/* write buffer */
		retval = mkyaffs2_write_chunk(obj->obj_id, ++chunk, bytes);
		if (retval) {
			MKYAFFS2_DEBUG("error while writing file '%s': %s\n",
					fpath, strerror(errno));
			break;
		}

		memset(databuf, 0xff, mkyaffs2_chunksize);
	}

	close(fd);

	return retval;
}

/*----------------------------------------------------------------------------*/

static inline void
mkyaffs2_scan_dir_status (unsigned status)
{
	char st = '-';

	switch (status % 4) {
	case 1:
		st = '\\';
		break;
	case 2:
		st = '|';
		break;
	case 3:
		st = '/';
		break;
	default:
		st = '-';
		break;
	}

	MKYAFFS2_PRINTF("\b\b\b[%c]", st);
	fflush(stdout);
}

static int
mkyaffs2_scan_dir (struct mkyaffs2_obj *parent)
{
	int retval = 0;
	DIR *dir;
	struct stat s;
	struct dirent *dent;
	struct mkyaffs2_obj *obj = NULL;

	dir = opendir(mkyaffs2_curfile[0] == '\0' ? "." : mkyaffs2_curfile);
	if (dir == NULL) {
		MKYAFFS2_ERROR("cannot open dir '%s': %s.",
				mkyaffs2_curfile, strerror(errno));
		return -1;
	}

	while(!retval && (dent = readdir(dir)) != NULL) {
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			continue;

		if (mkyaffs2_curfile[0] != '\0' &&
		    mkyaffs2_curfile[strlen(mkyaffs2_curfile) - 1] != '/')
			strncat(mkyaffs2_curfile, "/", 
				sizeof(mkyaffs2_curfile) -
				strlen(mkyaffs2_curfile) - 1);

		strncat(mkyaffs2_curfile, dent->d_name,
			sizeof(mkyaffs2_curfile) - strlen(mkyaffs2_curfile) - 1);

		obj = mkyaffs2_obj_alloc();
		if (obj == NULL) {
			MKYAFFS2_ERROR("allocate object failed for '%s': %sn\n",
					mkyaffs2_curfile, strerror(errno));
			return -1;
		}

		strncpy(obj->name, dent->d_name, NAME_MAX);
		obj->parent_obj = parent;
		list_add_tail(&obj->siblings, &parent->children);

		mkyaffs2_scan_dir_status(++mkyaffs2_objtree.objs);

		if (!lstat(mkyaffs2_curfile, &s) && S_ISDIR(s.st_mode))
			retval = mkyaffs2_scan_dir(obj);

		if (!strcmp(dirname(mkyaffs2_curfile), "."))
			mkyaffs2_curfile[0] = '\0';
	}

	closedir(dir);

	return retval;
}

static int
mkyaffs2_write_obj (const char *fpath, struct mkyaffs2_obj *obj)
{
	int retval = 0;
	ssize_t r;
	struct stat s;

	struct mkyaffs2_obj *equiv_obj;
	struct yaffs_obj_hdr oh;

	memset(&oh, 0xff, sizeof(struct yaffs_obj_hdr));

	retval = lstat(mkyaffs2_curfile, &s);
	if (retval) {
		MKYAFFS2_DEBUG("obtain attribute failed: %s.\n",
				strerror(errno));
		return retval;
	}

	/* update the obj */
	obj->dev = s.st_dev;
	obj->ino = s.st_ino;

	/* hardlink? */
	equiv_obj = mkyaffs2_objtable_find(obj->dev, obj->ino);
	if (equiv_obj) {
		obj->type = YAFFS_OBJECT_TYPE_HARDLINK;
		oh.equiv_id = equiv_obj->obj_id;
		goto write_obj;
	}

	switch (s.st_mode & S_IFMT) {
	case S_IFREG:
		obj->type = YAFFS_OBJECT_TYPE_FILE;
		oh.file_size_low = s.st_size & 0xFFFFFFFF;
#if __WORDSIZE == 64 || !defined __USE_FILE_OFFSET64
		oh.file_size_high = 0xffffffff;
#else
		oh.file_size_high = (s.st_size >> 32) & 0xFFFFFFFF;
#endif
		break;
	case S_IFLNK:
		obj->type = YAFFS_OBJECT_TYPE_SYMLINK;

		memset(oh.alias, 0, sizeof(oh.alias));

		r = readlink((char *)fpath, oh.alias, sizeof(oh.alias));
		if (r < 0) {
			MKYAFFS2_ERROR("read symbol link failed: %s\n",
					strerror(errno));
			return -1;
		}
		else if (r == sizeof(oh.alias)) {
			MKYAFFS2_ERROR("symbolic link is too long (max: %u)",
					(unsigned)sizeof(oh.alias) - 1);
			return -1;
		}
		break;
	case S_IFDIR:
		obj->type = YAFFS_OBJECT_TYPE_DIRECTORY;
		break;
	case S_IFCHR:
		obj->type = YAFFS_OBJECT_TYPE_CHR;
		break;
	case S_IFBLK:
		obj->type = YAFFS_OBJECT_TYPE_BLK;
		break;
	case S_IFIFO:
		obj->type = YAFFS_OBJECT_TYPE_FIFO;
		break;
	case S_IFSOCK:
		obj->type = YAFFS_OBJECT_TYPE_SOCK;
		break;
	default:
		/* skipping un-supported files silently */
		obj->type = YAFFS_OBJECT_TYPE_UNKNOWN;
		MKYAFFS2_DEBUG("skipped (unsupported file)\n");
		return 0;
	}

write_obj:
	oh.parent_obj_id = obj->parent_obj->obj_id;
	strncpy(oh.name, obj->name, YAFFS_MAX_NAME_LENGTH);
	oh.type = (obj->type > YAFFS_OBJECT_TYPE_SPECIAL) ?
			YAFFS_OBJECT_TYPE_SPECIAL : obj->type;
	
	if (oh.type != YAFFS_OBJECT_TYPE_HARDLINK) {
		if (MKYAFFS2_ISALLROOT) {
			oh.yst_uid = 0;
			oh.yst_gid = 0;
		}
		else {
			oh.yst_uid = s.st_uid;
			oh.yst_gid = s.st_gid;
		}
		oh.yst_mode = s.st_mode;
		oh.yst_atime = s.st_atime;
		oh.yst_mtime = s.st_mtime;
		oh.yst_ctime = s.st_ctime;
		oh.yst_rdev  = s.st_rdev;
	}

	obj->obj_id = ++mkyaffs2_image_obj_id;

	if (obj->obj_id > YAFFS_MAX_OBJECT_ID)
		MKYAFFS2_WARN("warning: too many files\n");

	retval = mkyaffs2_write_oh(&oh, obj);

	if (obj->type == YAFFS_OBJECT_TYPE_FILE && !retval)
		retval = mkyaffs2_write_regfile(fpath, obj);

	return retval;
}

static int
mkyaffs2_assemble_objtree (struct mkyaffs2_obj *obj)
{
	int retval = 0;
	struct stat s;

	struct list_head *p;
	struct mkyaffs2_obj *child;

	enum yaffs_obj_type type = YAFFS_OBJECT_TYPE_UNKNOWN;
	static const char *type_str[] = {"????", "FILE", "SLNK", "DIR", "HLNK",
					 "CHR", "BLK", "FIFO", "SOCK"};
	/* root object */
	if (obj == mkyaffs2_objtree.root) {
		if (stat(mkyaffs2_curfile[0] == '\0' ?
			 "." : mkyaffs2_curfile, &s) < 0 ||
		    !S_ISDIR(s.st_mode)) {
			MKYAFFS2_ERROR("ROOT is NOT a directory '%s' "
				       "(permission denied?)\n",
					mkyaffs2_curfile);
			return -1;
		}

		obj->dev = s.st_dev;
		obj->ino = s.st_ino;
		obj->obj_id = YAFFS_OBJECTID_ROOT;
		obj->type = YAFFS_OBJECT_TYPE_DIRECTORY;

		mkyaffs2_objtable_insert(obj);

		goto next;
	}

	if (!strlen(obj->name)) {
		/* it should NOT happen! */
		MKYAFFS2_DEBUG("skip obj with empty name\n");
		return 0;
	}

	/* format file path */
	if (mkyaffs2_curfile[0] != '\0' &&
	    mkyaffs2_curfile[strlen(mkyaffs2_curfile) - 1] != '/')
		strncat(mkyaffs2_curfile, "/",
			sizeof(mkyaffs2_curfile) -
			strlen(mkyaffs2_curfile) - 1);

	strncat(mkyaffs2_curfile, obj->name,
		sizeof(mkyaffs2_curfile) - strlen(mkyaffs2_curfile) - 1);

	MKYAFFS2_VERBOSE("NOW: '%s'. ", mkyaffs2_curfile);

	retval = mkyaffs2_write_obj(mkyaffs2_curfile, obj);
	if (!retval &&
	    obj->type != YAFFS_OBJECT_TYPE_HARDLINK &&
	    obj->type != YAFFS_OBJECT_TYPE_UNKNOWN)
		mkyaffs2_objtable_insert(obj);

next:
	if (retval) {
		MKYAFFS2_ERROR("object %u: [%4s] '%s' (FAILED).\n",
				obj->obj_id, type_str[type], mkyaffs2_curfile);
	}
	else {
		mkyaffs2_image_objs++;
		MKYAFFS2_PROGRESS_BAR(mkyaffs2_image_objs,
				      mkyaffs2_objtree.objs);

		MKYAFFS2_VERBOSE("\robject %u: [%4s] '%s'%s.\n",
				  obj->obj_id, type_str[obj->type], mkyaffs2_curfile,
				  obj->type == YAFFS_OBJECT_TYPE_UNKNOWN ? 
				  " (skip)" : "");

		if (obj->type == YAFFS_OBJECT_TYPE_DIRECTORY) {
			list_for_each(p, &obj->children) {
				child = list_entry(p, mkyaffs2_obj_t, siblings);
				retval = mkyaffs2_assemble_objtree(child);
				if (retval)
					break;
			}
		}
	}

	/* restore current file path */
	if (!strcmp(dirname(mkyaffs2_curfile), "."))
		mkyaffs2_curfile[0] = '\0';

	return retval;
}

/*----------------------------------------------------------------------------*/

static int
mkyaffs2_load_spare (const char *oobfile)
{
	int fd, retval = 0;
	ssize_t reads;

	if (oobfile == NULL)
		return 0;

	if ((fd = open(oobfile, O_RDONLY)) < 0) {
		MKYAFFS2_DEBUG("open oob image failed: %s\n", strerror(errno));
		return -1;
	}

	reads = safe_read(fd, &nand_oob_user, sizeof(nand_ecclayout_t));
	if (reads != sizeof(nand_ecclayout_t)) {
		MKYAFFS2_DEBUG("read oob image failed: %s\n", strerror(errno));
		retval = -1;
	}

	close(fd);

	return retval;
}


/*----------------------------------------------------------------------------*/

static int
mkyaffs2_create_image (const char *dirpath, const char *imgfile)
{
	int retval;
	struct stat statbuf;
	struct mkyaffs2_obj *root;

	if (stat(dirpath, &statbuf) < 0 && !S_ISDIR(statbuf.st_mode)) {
		MKYAFFS2_ERROR("ROOT is not a directory '%s'.\n", dirpath);
		return -1;
	}

	/* allocate root obj first */
	root = mkyaffs2_obj_alloc();
	if (root == NULL) {
		MKYAFFS2_ERROR("allocate object failed for '%s': %s.\n",
				dirpath, strerror(errno));
		return -1;
	}

	/* table initiailzation */
	mkyaffs2_objtable_init();
	mkyaffs2_objtree_init2(&mkyaffs2_objtree, root);

	/* allocate working buffer */
	mkyaffs2_bufsize = mkyaffs2_chunksize + mkyaffs2_sparesize;
	mkyaffs2_databuf = (unsigned char *)malloc(mkyaffs2_bufsize);
	if (mkyaffs2_databuf == NULL) {
		MKYAFFS2_ERROR("cannot allocate working buffer (%u bytes): %s",
				mkyaffs2_chunksize + mkyaffs2_sparesize,
				strerror(errno));
		retval = -1;
		goto exit_and_out;
	}

	mkyaffs2_image_fd = open(imgfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (mkyaffs2_image_fd < 0) {
		MKYAFFS2_ERROR("cannot open the image file: '%s'.\n", imgfile);
		retval = -1;
		goto free_and_out;
	}

	/* stage 1: scanning direcotry */
	snprintf(mkyaffs2_curfile, PATH_MAX, "%s", dirpath);
	MKYAFFS2_PRINTF("\n");
	MKYAFFS2_PRINTF("stage 1: scanning directory '%s'... [*]",
			mkyaffs2_curfile);

	retval = mkyaffs2_scan_dir(mkyaffs2_objtree.root);
	if (retval < 0)
		goto free_and_out;

	MKYAFFS2_PRINTF("\b\b\b[done]\nscanning complete, total objects: %u.\n",
			mkyaffs2_objtree.objs);

	/* stage 2: making a image */
	MKYAFFS2_PRINTF("\n");
	MKYAFFS2_PRINTF("stage 2: creating image '%s'\n", imgfile);

	MKYAFFS2_PROGRESS_INIT();

	snprintf(mkyaffs2_curfile, PATH_MAX, "%s", dirpath);
	retval = mkyaffs2_assemble_objtree(mkyaffs2_objtree.root);

free_and_out:
	if (mkyaffs2_image_fd >= 0)
		close(mkyaffs2_image_fd);
	free(mkyaffs2_databuf);
exit_and_out:
	mkyaffs2_objtree_exit(&mkyaffs2_objtree);
	mkyaffs2_objtable_exit();

	return retval;
}

/*----------------------------------------------------------------------------*/

static int
mkyaffs2_helper (void)
{
	MKYAFFS2_HELP("mkyaffs2 %s - A utility to make the yaffs2 image\n\n", YAFFS2UTILS_VERSION);
	MKYAFFS2_HELP("Usage: mkyaffs2 [-h|--help] [-e|--endian] [-v|--verbose]\n"
		      "                [-p|--pagesize pagesize] [-s|sparesize sparesize]\n"
		      "                [-o|--oobimg oobimage] [--all-root] [--yaffs-ecclayout]\n"
		      "                dirname imgfile\n\n");
	MKYAFFS2_HELP("Options:\n");
	MKYAFFS2_HELP("  -h                 display this help message and exit.\n");
	MKYAFFS2_HELP("  -e                 convert endian differed from local machine.\n");
	MKYAFFS2_HELP("  -v                 verbose details instead of progress bar.\n");
	MKYAFFS2_HELP("  -p pagesize        page size of target device.\n"
		      "                     (512|2048(default)|4096|(8192|16384) bytes)\n");
	MKYAFFS2_HELP("  -s sparesize       spare size of target device.\n"
		      "                     (default: pagesize/32 bytes; max: pagesize)\n");
	MKYAFFS2_HELP("  -o oobimage        load external oob image file.\n");
	MKYAFFS2_HELP("  --all-root         all files in the target system are owned by root.\n");
	MKYAFFS2_HELP("  --yaffs-ecclayout  use yaffs oob scheme instead of the Linux MTD default.\n");

	return -1;
}

/*----------------------------------------------------------------------------*/

int 
main (int argc, char *argv[])
{
	int retval;
	char *dirpath = NULL, *imgfile = NULL, *oobfile = NULL;
	
	int option, option_index;
	static const char *short_options = "hvep:s:o:";
	static const struct option long_options[] = {
		{"pagesize", 		required_argument, 	0, 'p'},
		{"sparesize", 		required_argument, 	0, 's'},
		{"oobimg",		required_argument,	0, 'o'},
		{"endian", 		no_argument, 		0, 'e'},
		{"verbose", 		no_argument, 		0, 'v'},
		{"all-root",		no_argument,		0, '0'},
		{"yaffs-ecclayout",	no_argument,		0, 'y'},
		{"help", 		no_argument, 		0, 'h'},
		{NULL,			no_argument,		0, '\0'},
	};

	mkyaffs2_chunksize = DEFAULT_CHUNKSIZE;

	while ((option = getopt_long(argc, argv, short_options,
				     long_options, &option_index)) != EOF) {
		switch (option) {
		case 'p':
			mkyaffs2_chunksize = strtoul(optarg, NULL, 10);
			break;
		case 's':
			mkyaffs2_sparesize = strtoul(optarg, NULL, 10);
			break;
		case 'o':
			oobfile = optarg;
			break;
		case 'e':
			mkyaffs2_flags |= MKYAFFS2_FLAGS_ENDIAN;
			break;
		case 'v':
			mkyaffs2_flags |= MKYAFFS2_FLAGS_VERBOSE;
			break;
		case 'y':
			mkyaffs2_flags |= MKYAFFS2_FLAGS_YAFFSECC;
			break;
		case '0':
			mkyaffs2_flags |= MKYAFFS2_FLAGS_ALLROOT;
			break;
		case 'h':
		default:
			return mkyaffs2_helper();
		}
	}

	if (argc - optind < 2)
		return mkyaffs2_helper();

	dirpath = argv[optind];
	imgfile = argv[optind + 1];

	MKYAFFS2_PRINTF("mkyaffs2 %s: image building tool for YAFFS2.\n",
			YAFFS2UTILS_VERSION);

	if (getuid() != 0) {
		mkyaffs2_flags |= MKYAFFS2_FLAGS_NONROOT;
		MKYAFFS2_WARN("warning: non-root users.\n");
	}

	/* load spare image if it is existed */
	mkyaffs2_ecclayout = NULL;
	if (oobfile) {
		if (mkyaffs2_load_spare(oobfile) < 0) {
			MKYAFFS2_ERROR("read oob image failed\n");
			return -1;
		}
		mkyaffs2_ecclayout = &nand_oob_user;
		/* FIXME: verify for the various ecc layout */
	}

	/* veridate the page size */
	mkyaffs2_assemble_ptags = &mkyaffs2_assemble_ptags2;
	switch (mkyaffs2_chunksize) {
	case 512:
		mkyaffs2_flags |= MKYAFFS2_FLAGS_YAFFS1;
		mkyaffs2_assemble_ptags = &mkyaffs2_assemble_ptags1;
		if (oobfile == NULL)
			mkyaffs2_ecclayout = &nand_oob_16;
		break;
	case 2048:
		if (mkyaffs2_ecclayout == NULL)
			mkyaffs2_ecclayout = MKYAFFS2_ISYAFFSECC ?
					     &yaffs_nand_oob_64 : &nand_oob_64;
		break;
	case 4096:
	case 8192:
	case 16384:
		/* FIXME: The OOB scheme for 8192 and 16384 bytes */
		if (mkyaffs2_ecclayout == NULL)
			mkyaffs2_ecclayout = MKYAFFS2_ISYAFFSECC ?
					     &yaffs_nand_oob_128 :
					     &nand_oob_128;
		break;
	default:
		MKYAFFS2_ERROR("%u bytes page size is NOT supported.\n",
				mkyaffs2_chunksize);
		return -1;
	}

	/* spare size */
	if (!mkyaffs2_sparesize)
		mkyaffs2_sparesize = mkyaffs2_chunksize / 32;

	if (mkyaffs2_sparesize > mkyaffs2_chunksize) {
		MKYAFFS2_ERROR("spare size is too large (%u).\n",
				mkyaffs2_sparesize);
		return -1;
	}

	/* verify whether the input directory is valid */
	if (strlen(dirpath) >= PATH_MAX || strlen(imgfile) >= PATH_MAX) {
		MKYAFFS2_ERROR("directory or image path is too long ");
		MKYAFFS2_ERROR("(max: %u characters).\n", PATH_MAX - 1);
		return -1;
	}


	retval = mkyaffs2_create_image(dirpath, imgfile);
	if (!retval) {
		MKYAFFS2_PRINTF("\noperation complete,\n"
				"%u objects in %u NAND pages.\n",
				mkyaffs2_image_objs, mkyaffs2_image_pages);
	}
	else {
		MKYAFFS2_ERROR("\noperation incomplete,\n"
			       "image may be broken!!!\n");
	}

	return retval;
}
