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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>
#ifdef _HAVE_LUTIMES
#include <sys/time.h>
#else
#include <utime.h>
#endif
#ifdef _HAVE_OSX_SYSLIMITS
#include <sys/syslimits.h>
#endif

#include "yaffs_packedtags1.h"
#include "yaffs_packedtags2.h"
#include "yaffs_trace.h"

#include "list.h"
#include "safe_rw.h"
#include "progress_bar.h"
#include "endian_convert.h"
#include "nand_ecclayout.h"

#include "version.h"

/*----------------------------------------------------------------------------*/

#define UNYAFFS2_OBJTABLE_SIZE	4096
#define UNYAFFS2_HARDLINK_MAX	127

#define UNYAFFS2_FLAGS_NONROOT	(1 << 0)
#define UNYAFFS2_FLAGS_SHOWBAR	(1 << 1)
#define UNYAFFS2_FLAGS_YAFFS1	(1 << 16)
#define UNYAFFS2_FLAGS_ENDIAN	(1 << 17)
#define UNYAFFS2_FLAGS_YAFFSECC	(1 << 18)
#define UNYAFFS2_FLAGS_VERBOSE	(1 << 19)

#define UNYAFFS2_ISSHOWBAR	(unyaffs2_flags & UNYAFFS2_FLAGS_SHOWBAR)
#define UNYAFFS2_ISYAFFS1	(unyaffs2_flags & UNYAFFS2_FLAGS_YAFFS1)
#define UNYAFFS2_ISENDIAN	(unyaffs2_flags & UNYAFFS2_FLAGS_ENDIAN)
#define UNYAFFS2_ISYAFFSECC	(unyaffs2_flags & UNYAFFS2_FLAGS_YAFFSECC)
#define UNYAFFS2_ISVERBOSE	(unyaffs2_flags & UNYAFFS2_FLAGS_VERBOSE)

#define UNYAFFS2_PRINTF(s, args...) \
		do { \
			if (!UNYAFFS2_ISVERBOSE && UNYAFFS2_ISSHOWBAR) { \
				unyaffs2_flags &= ~UNYAFFS2_FLAGS_SHOWBAR; \
				fprintf(stderr, "\n"); \
			} \
			fprintf(stdout, s, ##args); \
			fflush(stdout); \
		} while (0)

#define UNYAFFS2_ERROR_PRINTF(s, args...) \
		do { \
			if (!UNYAFFS2_ISVERBOSE && UNYAFFS2_ISSHOWBAR) { \
				unyaffs2_flags &= ~UNYAFFS2_FLAGS_SHOWBAR; \
				fprintf(stderr, "\n"); \
			} \
			fprintf(stderr, s, ##args); \
			fflush(stderr); \
		} while (0)

#define UNYAFFS2_HELP(s, args...) \
		UNYAFFS2_PRINTF(s, ##args)

#define UNYAFFS2_WARN(s, args...) \
		UNYAFFS2_ERROR_PRINTF(s, ##args)

#define UNYAFFS2_ERROR(s, args...) \
		UNYAFFS2_ERROR_PRINTF(s, ##args)

#ifdef _UNYAFFS2_DEBUG
#define UNYAFFS2_DEBUG(s, args...) \
		UNYAFFS2_ERROR_PRINTF("%s: " s,  __FUNCTION__, ##args)
#else
#define UNYAFFS2_DEBUG(s, args...)
#endif

#define UNYAFFS2_VERBOSE(s, args...) \
		do { \
			if (UNYAFFS2_ISVERBOSE) \
				UNYAFFS2_PRINTF(s, ##args); \
		} while (0)

#define UNYAFFS2_PROGRESS_INIT() \
		do { \
			if (!UNYAFFS2_ISVERBOSE) \
				progress_init(); \
		} while (0)

#define UNYAFFS2_PROGRESS_BAR(objs, total) \
		do { \
			if (!UNYAFFS2_ISVERBOSE) { \
				unyaffs2_flags |= UNYAFFS2_FLAGS_SHOWBAR; \
				progress_bar(objs, total); \
			} \
		} while(0)

/*----------------------------------------------------------------------------*/

typedef struct unyaffs2_file_var {
	loff_t file_size;
	off_t file_head;
} unyaffs2_file_var_t;

typedef struct unyaffs2_symlink_var {
	char *alias;
} unyaffs2_symlink_var_t;

typedef struct unyaffs2_hardlink_var {
	struct unyaffs2_obj *equiv_obj;
} unyaffs2_hardlink_var_t;

typedef struct unyaffs2_dev_var {
	unsigned rdev;
} unyaffs2_dev_var_t;

typedef union unyaffs2_file_variant {
	struct unyaffs2_file_var file;
	struct unyaffs2_symlink_var symlink;
	struct unyaffs2_hardlink_var hardlink;
	struct unyaffs2_dev_var dev;
} unyaffs2_file_variant_t;

typedef struct unyaffs2_obj {
	unsigned char valid:1;
	unsigned char extracted:1;	/* 1 when extracted. */

	off_t hdr_off;			/* header offset in the image */

	unsigned obj_id;
	unsigned parent_id;
	struct unyaffs2_obj *parent_obj;

	unsigned type;			/* file type */
	union unyaffs2_file_variant variant;

	unsigned mode;			/* file mode */

	unsigned uid;			/* owner uid */
	unsigned gid;			/* owner gid */

	unsigned atime;
	unsigned mtime;
	unsigned ctime;

	char name[NAME_MAX + 1];

	struct list_head hashlist;      /* hash table */
	struct list_head children;	/* if it is the directory */
	struct list_head siblings;	/* neighbors of fs tree */
} unyaffs2_obj_t;

typedef struct unyaffs2_fstree {
	unsigned objs;
	struct unyaffs2_obj *root;
} unyaffs2_fstree_t;

typedef struct unyaffs2_specfile {
	char *path;			/* file path */
	struct unyaffs2_obj *obj;	/* object */
	struct list_head list;		/* specified files list */
} unyaffs2_specfile_t;

#ifdef _HAVE_MMAP
typedef struct unyaffs2_mmap {
	unsigned char *addr;
	size_t size;
} unyaffs2_mmap_t;
#endif

/*----------------------------------------------------------------------------*/

static unsigned unyaffs2_chunksize = 0;
static unsigned unyaffs2_sparesize = 0;

static unsigned unyaffs2_flags = 0;

static unsigned unyaffs2_image_objs = 0;

static unsigned unyaffs2_bufsize = 0;
static unsigned char *unyaffs2_databuf = NULL;

static int unyaffs2_image_fd = -1;

static char unyaffs2_curfile[PATH_MAX + PATH_MAX] = {0};
static char unyaffs2_linkfile[PATH_MAX + PATH_MAX] = {0};

static LIST_HEAD(unyaffs2_specfile_list);	/* specfied files */

static nand_ecclayout_t *unyaffs2_ecclayout = NULL;

static struct unyaffs2_fstree unyaffs2_objtree = {0};
static struct list_head unyaffs2_objtable[UNYAFFS2_OBJTABLE_SIZE];

#ifdef _HAVE_MMAP
static struct unyaffs2_mmap unyaffs2_mmapinfo = {0};
#endif

static void
(*unyaffs2_extract_ptags) (struct yaffs_ext_tags *, unsigned char *,
			   nand_ecclayout_t *, int) = NULL;

/*----------------------------------------------------------------------------*/

static struct unyaffs2_obj *
unyaffs2_obj_alloc (void)
{
	struct unyaffs2_obj *obj;

	obj = calloc(sizeof(struct unyaffs2_obj), sizeof(unsigned char));
	if (obj == NULL)
		return NULL;

	obj->parent_obj = obj;

	INIT_LIST_HEAD(&obj->children);
	INIT_LIST_HEAD(&obj->siblings);
	INIT_LIST_HEAD(&obj->hashlist);

	return obj;
}

static void
unyaffs2_obj_free (struct unyaffs2_obj *obj)
{
	if (obj->type == YAFFS_OBJECT_TYPE_SYMLINK &&
	    obj->variant.symlink.alias != NULL)
		free(obj->variant.symlink.alias);

	list_del(&obj->children);
	list_del(&obj->siblings);
	list_del(&obj->hashlist);

	free(obj);
}

/*----------------------------------------------------------------------------*/

/*
 * hash table to look up objects
 */

static inline unsigned
unyaffs2_objtable_hash (unsigned hash)
{
	return hash % UNYAFFS2_OBJTABLE_SIZE;
}

static inline void
unyaffs2_objtable_insert (struct unyaffs2_obj *obj)
{
	unsigned n = unyaffs2_objtable_hash(obj->obj_id);
	list_add_tail(&obj->hashlist, &unyaffs2_objtable[n]);
}

static inline struct unyaffs2_obj *
unyaffs2_objtable_find (unsigned obj_id)
{
	unsigned n = unyaffs2_objtable_hash(obj_id);
	struct list_head *p;
	struct unyaffs2_obj *obj;

	list_for_each(p, &unyaffs2_objtable[n]) {
		obj = list_entry(p, unyaffs2_obj_t, hashlist);
		if (obj->obj_id == obj_id)
			return obj;
	}

	return NULL;
}

static struct unyaffs2_obj *
unyaffs2_objtable_find_alloc (unsigned obj_id)
{
	struct unyaffs2_obj *obj;

	obj = unyaffs2_objtable_find(obj_id);
	if (obj)
		return obj;

	obj = unyaffs2_obj_alloc();
	if (obj) {
		obj->obj_id = obj_id;
		unyaffs2_objtable_insert(obj);
	}

	return obj;
}

static inline void
unyaffs2_objtable_init (void)
{
	unsigned n;

	for (n = 0; n < UNYAFFS2_OBJTABLE_SIZE; n++)
		INIT_LIST_HEAD(&unyaffs2_objtable[n]);
}

static inline void
unyaffs2_objtable_exit (void)
{
	unsigned i;
	struct list_head *p, *n;
	struct unyaffs2_obj *obj;

	for (i = 0; i < UNYAFFS2_OBJTABLE_SIZE; i++) {
		list_for_each_safe(p, n, &unyaffs2_objtable[i]) {
			obj = list_entry(p, unyaffs2_obj_t, hashlist);
			unyaffs2_obj_free(obj);
		}
	}
}

/*----------------------------------------------------------------------------*/

/*
 * fs tree for objects extraction.
 */

static void
unyaffs2_objtree_cleanup (struct unyaffs2_obj *obj)
{
	struct list_head *p, *n;
	struct unyaffs2_obj *child;

	if (obj == NULL)
		return;

	list_for_each_safe(p, n, &obj->children) {
		child = list_entry(p, unyaffs2_obj_t, siblings);
		unyaffs2_objtree_cleanup(child);
	}

	unyaffs2_obj_free(obj);
}

static struct unyaffs2_fstree *
unyaffs2_objtree_init (struct unyaffs2_fstree *fst)
{
	struct unyaffs2_fstree *f = fst;

	if (f == NULL) {
		f = malloc(sizeof(struct unyaffs2_fstree));
		if (f == NULL)
			return NULL;
	}

	/* initialize */
	memset(f, 0, sizeof(struct unyaffs2_fstree));

	return f;
}

static void
unyaffs2_objtree_exit (struct unyaffs2_fstree *fst)
{
	return unyaffs2_objtree_cleanup(fst->root);
}

/*----------------------------------------------------------------------------*/

/*
 * specfied files
 */

static struct unyaffs2_specfile *
unyaffs2_specfile_find (const char *path) 
{
	struct unyaffs2_specfile *spec;
	struct list_head *p;
	size_t len;

	list_for_each(p, &unyaffs2_specfile_list) {
		spec = list_entry(p, unyaffs2_specfile_t, list);
		if (!strncmp(path, spec->path, strlen(spec->path))) {
			len = strlen(spec->path);
			if (path[len] == '\0' || path[len] == '/')
				return spec;
		}
	}

	return NULL;
}

static int
unyaffs2_specfile_insert (const char *path)
{
	char *s, *e;
	size_t len;
	struct unyaffs2_specfile *spec;

	/* skipping leading '/' */
	s = (char *)path;
	while (s[0] != '\0' && s[0] == '/')
		s++;

	/* skipping following '/' */
	e = (char *)path + strlen(path) - 1;
	while (e > s && e[0] == '/')
		e--;

	if (e <= s) {
		UNYAFFS2_DEBUG("unsupport path format: %s\n", path);
		return -1;
	}

	spec = calloc(sizeof(struct unyaffs2_specfile), sizeof(unsigned char));
	if (!spec) {
		UNYAFFS2_DEBUG("allocate obj failed for '%s': %s\n",
				path, strerror(errno));
		return -1;
	}

	len = e - s + 1;
	spec->path = calloc(len + 1, sizeof(unsigned char));
	if (spec->path == NULL) {
		UNYAFFS2_DEBUG("allocate string failed for '%s': %s\n",
				path, strerror(errno));
		free(spec);
		return -1;
	}

	memcpy(spec->path, s, len);
	spec->path[len] = '\0';

	list_add_tail(&spec->list, &unyaffs2_specfile_list);

	return 0;
}

static void
unyaffs2_specfile_exit (void)
{
	struct unyaffs2_specfile *spec;
	struct list_head *p, *n;

	list_for_each_safe(p, n, &unyaffs2_specfile_list) {
		spec = list_entry(p, unyaffs2_specfile_t, list);
		if (spec->path)
			free(spec->path);
		list_del(&spec->list);
		free(spec);
	}
}

/*----------------------------------------------------------------------------*/

static int
unyaffs2_mkdir (const char *name, const mode_t mode)
{
	char *p = NULL;
	const char *dname;

	struct stat statbuf;

	dname = strlen(name) ? name : ".";

	p = (char *)dname;
	while ((p = strchr(p, '/')) != NULL) {
		*p = '\0';
		mkdir(dname, 0755);
		*p++ = '/';
	}

	if (mkdir(dname, mode) < 0 && 
	    (stat(dname, &statbuf) < 0 || !S_ISDIR(statbuf.st_mode)))
		return -1;

	return 0;
}

static char *
unyaffs2_prefix_basestr(const char *path, const char *prefix)
{
	char *p = NULL;
	size_t len = 0;

	if ((len = strlen(prefix)) != 0 &&
	    strncmp(path, prefix, len) == 0 &&
	    (path[len] == '\0' || path[len] == '/')) {
		p = strrchr(prefix, '/');
		p = p ? (char *)path + (p - prefix) + 1: (char *)path;
	}

	return p;
}

/*----------------------------------------------------------------------------*/

static size_t
unyaffs2_spare2ptags (unsigned char *tag, unsigned char *spare, size_t bytes,
		      nand_ecclayout_t *ecclayout)
{
	unsigned i;
	size_t copied = 0;
	struct nand_oobfree *oobfree = ecclayout->oobfree;

	for (i = 0; i < 8 && copied < bytes; i++) {
		size_t size = bytes - copied;
		unsigned char *s = spare + oobfree[i].offset;

		if (size > oobfree[i].length)
			size = oobfree[i].length;

		memcpy(tag, s, size);

		copied += size;
		tag += size;
	}

	return copied;
}

static void
unyaffs2_extract_ptags1 (struct yaffs_ext_tags *t, unsigned char *pt,
			 nand_ecclayout_t *ecclayout, int ecc)
{
	struct yaffs_packed_tags1 pt1;
	nand_ecclayout_t *l = ecclayout ? ecclayout : unyaffs2_ecclayout;

	memset(&pt1, 0xff, sizeof(struct yaffs_packed_tags1));
	unyaffs2_spare2ptags((unsigned char *)&pt1, pt,
			     sizeof(struct yaffs_packed_tags1), l);

	if (UNYAFFS2_ISENDIAN)
		packedtags1_endian_convert(&pt1, 1);

	yaffs_unpack_tags1(t, &pt1);
}

static void
unyaffs2_extract_ptags2 (struct yaffs_ext_tags *t, unsigned char *s,
			 nand_ecclayout_t *ecclayout, int ecc)
{
	int result;
	enum yaffs_ecc_result ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
	struct yaffs_ecc_other tag_ecc;
	struct yaffs_packed_tags2 pt2;
	nand_ecclayout_t *l = ecclayout ? ecclayout : unyaffs2_ecclayout;

	memset(&pt2, 0xff, sizeof(struct yaffs_packed_tags2));
	unyaffs2_spare2ptags((unsigned char *)&pt2, s,
			     sizeof(struct yaffs_packed_tags2), l);

	if (pt2.t.seq_number != 0xffffffff && ecc) {
		if (UNYAFFS2_ISENDIAN)
			packedtags2_eccother_endian_convert(&pt2);

		yaffs_ecc_calc_other((unsigned char *)&pt2.t,
				sizeof(struct yaffs_packed_tags2_tags_only),
				&tag_ecc);

		result = yaffs_ecc_correct_other((unsigned char *)&pt2.t,
				sizeof(struct yaffs_packed_tags2_tags_only),
				&pt2.ecc, &tag_ecc);

		switch (result) {
		case 0:
			ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
			break;
		case 1:
			ecc_result = YAFFS_ECC_RESULT_FIXED;
			break;
		case -1:
			ecc_result = YAFFS_ECC_RESULT_UNFIXED;
			break;
		default:
			ecc_result = YAFFS_ECC_RESULT_UNKNOWN;
		}
	}

	if (UNYAFFS2_ISENDIAN)
		packedtags2_tagspart_endian_convert(&pt2);

	yaffs_unpack_tags2_tags_only(t, &pt2.t);

	t->ecc_result = ecc_result;
}

static inline int
unyaffs2_isempty (unsigned char *buf, unsigned size)
{
	while (size--) {
		if (*buf != 0xff)
			return 0;
		buf++;
	}
	return 1;
}

static inline loff_t
unyaffs2_extract_oh_size (struct yaffs_obj_hdr *oh)
{
	loff_t retval;

	if(~(oh->file_size_high))
		retval = (((loff_t) oh->file_size_high) << 32) |
			  (((loff_t) oh->file_size_low) & 0xFFFFFFFF);
	else
		retval = (loff_t) oh->file_size_low;

	return retval;
}

static int
unyaffs2_oh2obj (struct unyaffs2_obj *obj, struct yaffs_obj_hdr *oh)
{
	switch (oh->type) {
	case YAFFS_OBJECT_TYPE_FILE:
		obj->type = YAFFS_OBJECT_TYPE_FILE;
		obj->variant.file.file_size = unyaffs2_extract_oh_size(oh);
		break;
	case YAFFS_OBJECT_TYPE_SYMLINK:
		obj->type = YAFFS_OBJECT_TYPE_SYMLINK;
		obj->variant.symlink.alias = strdup(oh->alias);
		break;
	case YAFFS_OBJECT_TYPE_DIRECTORY:
		obj->type = YAFFS_OBJECT_TYPE_DIRECTORY;
		break;
	case YAFFS_OBJECT_TYPE_HARDLINK:
		obj->type = YAFFS_OBJECT_TYPE_HARDLINK;
		obj->variant.hardlink.equiv_obj =
			unyaffs2_objtable_find(oh->equiv_id);
		break;
	case YAFFS_OBJECT_TYPE_SPECIAL:
		switch (oh->yst_mode & S_IFMT) {
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
			obj->type = YAFFS_OBJECT_TYPE_UNKNOWN;
			break;
		}

		if (obj->type != YAFFS_OBJECT_TYPE_UNKNOWN)
			obj->variant.dev.rdev = oh->yst_rdev;
		break;
	default:
		obj->type = YAFFS_OBJECT_TYPE_UNKNOWN;
		return -1;
	}

	obj->parent_id = oh->parent_obj_id;
	strncpy(obj->name, oh->name, NAME_MAX);

	if (obj->type != YAFFS_OBJECT_TYPE_HARDLINK &&
	    obj->type != YAFFS_OBJECT_TYPE_UNKNOWN) {
		obj->mode = oh->yst_mode;
		obj->uid = oh->yst_uid;
		obj->gid = oh->yst_gid;
		obj->atime = oh->yst_atime;
		obj->mtime = oh->yst_mtime;
		obj->ctime = oh->yst_ctime;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/

static void
unyaffs2_format_filepath (char *path, size_t size, struct unyaffs2_obj *obj)
{
	size_t pathlen = 0;

	if (obj->obj_id == YAFFS_OBJECTID_ROOT)
		return;

	pathlen = strlen(path);
	unyaffs2_format_filepath(path, size, obj->parent_obj);

	if (pathlen < strlen(path))
		strncat(path, "/", size - strlen(path) - 1);

	strncat(path, obj->name, size - strlen(path) - 1);
}


static struct unyaffs2_obj *
unyaffs2_follow_hardlink (struct unyaffs2_obj *obj)
{
	unsigned link_counts = 0;
	struct unyaffs2_obj *equiv = obj;

	while (equiv && equiv->valid &&
	       equiv->type == YAFFS_OBJECT_TYPE_HARDLINK) {
		equiv = equiv->variant.hardlink.equiv_obj;
		if (++link_counts > UNYAFFS2_HARDLINK_MAX) {
			/* make sure everything is right */
			UNYAFFS2_DEBUG("too many links for object %d\n",
					obj->obj_id);
			break;
		}
	}

	return (equiv && equiv->valid &&
		equiv->type != YAFFS_OBJECT_TYPE_HARDLINK) ? equiv : NULL;
}

static void
unyaffs2_obj_chattr (const char *fpath, struct unyaffs2_obj *obj)
{
	/* access time */
#ifdef _HAVE_LUTIMES
	struct timeval ftime[2];

	ftime[0].tv_sec = obj->atime;
	ftime[0].tv_usec = 0;
	ftime[1].tv_sec = obj->mtime;
	ftime[1].tv_usec = 0;

	lutimes(fpath, ftime);
#else
	struct utimbuf ftime;

	ftime.actime = obj->atime;
	ftime.modtime = obj->mtime;

	utime(fpath, &ftime);
#endif

	/* owner */
	lchown(fpath, obj->uid, obj->gid);

	/* mode - no effect on symbolic link */
	if (obj->type != YAFFS_OBJECT_TYPE_SYMLINK)
		chmod(fpath, obj->mode);
}

static int
unyaffs2_objtree_chattr (struct unyaffs2_obj *obj)
{
	struct list_head *p;
	struct unyaffs2_obj *child;

	/* format the file path */
	if (unyaffs2_curfile[0] != '\0' &&
	    unyaffs2_curfile[strlen(unyaffs2_curfile) - 1] != '/')
		strncat(unyaffs2_curfile, "/",
			sizeof(unyaffs2_curfile) -
			strlen(unyaffs2_curfile) - 1);

		 strncat(unyaffs2_curfile, obj->name,
			 sizeof(unyaffs2_curfile) -
				strlen(unyaffs2_curfile) - 1);

	/* travel */
	if (obj->type == YAFFS_OBJECT_TYPE_DIRECTORY) {
		list_for_each(p, &obj->children) {
			child = list_entry(p, unyaffs2_obj_t, siblings);
			unyaffs2_objtree_chattr(child);
		}
	}

	if (obj->type != YAFFS_OBJECT_TYPE_HARDLINK)
		unyaffs2_obj_chattr(unyaffs2_curfile, obj);

	/* restore current file path */
	if (!strcmp(dirname(unyaffs2_curfile), "."))
		unyaffs2_curfile[0] = '\0';

	return 0;
}

/*----------------------------------------------------------------------------*/

static inline void
unyaffs2_scan_img_status (unsigned status)
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

	UNYAFFS2_PRINTF("\b\b\b[%c]", st);
	fflush(stdout);
}

static int
unyaffs2_scan_chunk (unsigned char *buffer, off_t offset)
{
	struct yaffs_obj_hdr oh;
	struct yaffs_ext_tags tag;
	struct unyaffs2_obj *obj;

	unyaffs2_extract_ptags(&tag, buffer + unyaffs2_chunksize, NULL, 1);
	if (tag.ecc_result == YAFFS_ECC_RESULT_UNFIXED) {
		UNYAFFS2_DEBUG("invalid page skipped @ offset %lu\n", offset);
		return 0;
	}

	/* empty page? */
	if (tag.obj_id <= YAFFS_OBJECTID_DELETED ||
	    tag.obj_id == YAFFS_OBJECTID_SUMMARY ||
	    tag.chunk_used == 0) {
		UNYAFFS2_DEBUG("unused page skipped @ offset %lu\n", offset);
		return 0;
	}

	if (tag.chunk_id == 0) {
	/* a new object */
		obj = unyaffs2_objtable_find_alloc(tag.obj_id);
		if (obj == NULL) {
			UNYAFFS2_ERROR("cannot allocate memory ");
			UNYAFFS2_ERROR("for object %u\n", tag.obj_id);
			return -1;
		}

		if (obj->valid) {
			UNYAFFS2_DEBUG("skip duplicated object %u\n",
				       tag.obj_id);
			return -1;
		}

		memcpy(&oh, unyaffs2_databuf, sizeof(struct yaffs_obj_hdr));
		if (UNYAFFS2_ISENDIAN)
			oh_endian_convert(&oh);

		/* extract oh to obj */
		unyaffs2_oh2obj(obj, &oh);
		obj->obj_id = tag.obj_id;
		obj->hdr_off = offset;
		obj->valid = 1;

		unyaffs2_image_objs++;
	}
	else if (tag.chunk_id == 1) {
	/* the first data chunk of a object */
		obj = unyaffs2_objtable_find_alloc(tag.obj_id);
		if (obj == NULL) {
			UNYAFFS2_ERROR("cannot allocate memory ");
			UNYAFFS2_ERROR("for object %u\n", tag.obj_id);
			return -1;
		}

		obj->type = YAFFS_OBJECT_TYPE_FILE;
		obj->variant.file.file_head = offset;
	}

	return 0;
}

static int
unyaffs2_scan_img (void)
{
#ifndef _HAVE_MMAP
	ssize_t reads;
#endif
	off_t offset = 0, remains = 0;

	if (unyaffs2_image_fd < 0) {
		UNYAFFS2_DEBUG("bad file descriptor.\n");
		return 0;
	}

#ifdef _HAVE_MMAP
	if (unyaffs2_mmapinfo.addr == NULL) {
		UNYAFFS2_DEBUG("NULL mmap address.\n");
		return 0;
	}
#endif

#ifdef _HAVE_MMAP
	remains = unyaffs2_mmapinfo.size;
	while (offset < unyaffs2_mmapinfo.size && remains >= unyaffs2_bufsize &&
	       memcpy(unyaffs2_databuf,
		      unyaffs2_mmapinfo.addr + offset, unyaffs2_bufsize)) {
		if (memcmp(unyaffs2_databuf, 
		    unyaffs2_mmapinfo.addr + offset, unyaffs2_bufsize)) {
#else
	remains = lseek(unyaffs2_image_fd, 0, SEEK_END);
	offset = lseek(unyaffs2_image_fd, 0, SEEK_SET);
	while (remains >= unyaffs2_bufsize &&
	       (reads = safe_read(unyaffs2_image_fd,
		unyaffs2_databuf, unyaffs2_bufsize)) != 0) {
		if (reads != unyaffs2_bufsize) {
#endif
			/* parse image failed */
			UNYAFFS2_ERROR("read image failed @ offset %lu.",
					offset);
			return -1;
		}

		if (!unyaffs2_isempty(unyaffs2_databuf, unyaffs2_bufsize))
			unyaffs2_scan_chunk(unyaffs2_databuf, offset);

		offset += unyaffs2_bufsize;
		remains -= unyaffs2_bufsize;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/

static struct unyaffs2_obj*
unyaffs2_create_fakeroot (const char *path)
{
	struct stat s;
	struct unyaffs2_obj *obj;

	if (stat(path, &s) || !S_ISDIR(s.st_mode))
		return NULL;

	obj = unyaffs2_obj_alloc();
	if (!obj)
		return NULL;

	/* update root info */
	obj->obj_id = YAFFS_OBJECTID_ROOT;
	obj->parent_id = YAFFS_OBJECTID_ROOT;
	obj->type = YAFFS_OBJECT_TYPE_DIRECTORY;

	obj->mode = s.st_mode;

	obj->uid = s.st_uid;
	obj->gid = s.st_gid;
	obj->atime = s.st_atime;
	obj->mtime = s.st_mtime;
	obj->ctime = s.st_ctime;

	obj->valid = 1;

	unyaffs2_image_objs++;

	return obj;
}

static int
unyaffs2_create_objtree (void)
{
	unsigned n, objs = 0;
	struct list_head *p;
	struct unyaffs2_obj *obj, *parent;

	unyaffs2_objtree.root = unyaffs2_objtable_find(YAFFS_OBJECTID_ROOT);

	for (n = 0; n < UNYAFFS2_OBJTABLE_SIZE; n++) {
		list_for_each(p, &unyaffs2_objtable[n]) {
			obj = list_entry(p, unyaffs2_obj_t, hashlist);

			parent = unyaffs2_objtable_find(obj->parent_id);
			if (parent && obj != parent &&
			    parent->type == YAFFS_OBJECT_TYPE_DIRECTORY) {
				obj->parent_obj = parent;
				list_add_tail(&obj->siblings,
					      &parent->children);
			}
			unyaffs2_scan_img_status(++objs);
		}
	}

	return 0;
}

static int
unyaffs2_validate_objtree (struct unyaffs2_obj *obj)
{
	struct list_head *p;
	struct unyaffs2_obj *child;

	if (obj == NULL)
		return -1;

	/* FIXME: validation? */
	unyaffs2_scan_img_status(++unyaffs2_objtree.objs);

	list_for_each(p, &obj->children) {
		child = list_entry(p, unyaffs2_obj_t, siblings);
		if (unyaffs2_validate_objtree(child) < 0)
			return -1;
	}

	return 0;
}

static int
unyaffs2_build_objtree (void)
{
	return unyaffs2_create_objtree() ||
	       unyaffs2_validate_objtree(unyaffs2_objtree.root);
}

/*----------------------------------------------------------------------------*/

#ifdef _HAVE_MMAP
static int
unyaffs2_extract_file_mmap (unsigned char *addr, size_t size, const char *fpath,
			    struct unyaffs2_obj *obj)
{
	int outfd;
	unsigned char *outaddr, *curaddr, *endaddr = addr + size;
	size_t bufsize = unyaffs2_chunksize + unyaffs2_sparesize;
	size_t fsize = obj->variant.file.file_size, remains = 0, written = 0;

	struct yaffs_ext_tags tag;

	outfd = open(fpath, O_RDWR | O_CREAT | O_TRUNC, obj->mode);
	if (outfd < 0) {
		UNYAFFS2_DEBUG("cannot create file '%s'; %s\n",
				fpath, strerror(errno));
		return -1;
	}

	if (fsize == 0)
		goto out;

	/* stretch the file */
	if (lseek(outfd, fsize - 1, SEEK_SET) < 0 ||
	    safe_write(outfd, "", 1) != 1) {
		UNYAFFS2_DEBUG("cannot stretch the file '%s': %s\n",
				fpath, strerror(errno));
		goto out;
	}

	/* now mmap */
	outaddr = mmap(NULL, fsize, PROT_WRITE | PROT_READ,
		       MAP_SHARED, outfd, 0);
	if (outaddr == MAP_FAILED) {
		UNYAFFS2_DEBUG("cannot mmap the file '%s': %s\n",
				fpath, strerror(errno));
		goto out;
	}

	if (obj->variant.file.file_head == ~(off_t)0) {
		UNYAFFS2_DEBUG("invalid head offset of file  '%s'\n", fpath);
		goto out;
	}

	addr += obj->variant.file.file_head;
	curaddr = outaddr;
	remains = fsize;

	while (addr < endaddr && remains > 0) {
		unyaffs2_extract_ptags(&tag, addr + unyaffs2_chunksize,
				       NULL, 0);

		written = remains < tag.n_bytes ? remains : tag.n_bytes;
		memcpy(curaddr, addr, written);
		if (memcmp(curaddr, addr, written)) {
			UNYAFFS2_DEBUG("copy file failed '%s': %s\n",
					fpath, strerror(errno));
			break;
		}

		remains -= written;
		curaddr += written;
		addr += bufsize;
	}

	munmap(outaddr, fsize);
out:
	close(outfd);

	return !(remains == 0);
}
#else
static int
unyaffs2_extract_file (const int fd, const char *fpath,
		       struct unyaffs2_obj *obj)
{
	int outfd;
	size_t written = 0, size = obj->variant.file.file_size;
	ssize_t w, r;

	struct yaffs_ext_tags tag;

	if (obj->type != YAFFS_OBJECT_TYPE_FILE)
		return 0;

	outfd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, obj->mode);
	if (outfd < 0) {
		UNYAFFS2_DEBUG("cannot create file '%s': %s\n",
				fpath, strerror(errno));
		return -1;
	}

	if (obj->variant.file.file_head == ~(off_t)0) {
		UNYAFFS2_DEBUG("invalid head offset of file  '%s'\n", fpath);
		goto out;
	}

	lseek(fd, obj->variant.file.file_head, SEEK_SET);

	/* read image until the size is reached */
	while (written < size &&
	       (r = safe_read(fd, unyaffs2_databuf, unyaffs2_bufsize)) != 0)
	{
		if (r != unyaffs2_bufsize) {
			UNYAFFS2_DEBUG("read image failed '%s': %s\n",
					fpath, strerror(errno));
			break;
		}

		unyaffs2_extract_ptags(&tag,
				       unyaffs2_databuf + unyaffs2_chunksize,
				       NULL, 0);

		w = safe_write(outfd, unyaffs2_databuf, tag.n_bytes);
		if (w != tag.n_bytes) {
			UNYAFFS2_DEBUG("write file failed '%s': %s",
					fpath, strerror(errno));
			break;
		}

		written += tag.n_bytes;
	}

	close(outfd);

	return !(written == size);
}
#endif

static int unyaffs2_extract_obj (const char *fpath, struct unyaffs2_obj *obj);

static int
unyaffs2_extract_hardlink (const char *fpath, struct unyaffs2_obj *obj)
{
	int retval = -1;
	char *lnkfile;

	struct unyaffs2_obj *equiv;
	union unyaffs2_file_variant *variant;

	equiv = unyaffs2_follow_hardlink(obj);
	if (equiv == NULL) {
		UNYAFFS2_DEBUG("equiv object NOT found.\n");
		return -1;
	}

	/* path of linked file */
	memset(unyaffs2_linkfile, 0, sizeof(unyaffs2_linkfile));
	unyaffs2_format_filepath(unyaffs2_linkfile,
				 sizeof(unyaffs2_linkfile), equiv);

	/* for specified files */
	lnkfile = unyaffs2_linkfile;
	if (!list_empty(&unyaffs2_specfile_list)) {
		struct unyaffs2_specfile *spec;

		spec = unyaffs2_specfile_find(unyaffs2_linkfile);
		lnkfile = (spec == NULL) ? NULL : 
			  unyaffs2_prefix_basestr(unyaffs2_linkfile,
						  spec->path);
	}
	
	/* unlink the original file always */
	unlink(fpath);

	/*
	 * equiv obj was extracted before,
	 * link them, silent on errors
	 */
	if (lnkfile && equiv->extracted)
		return link(lnkfile, fpath);

	/*
	 * FIXME: better solution!? 
	 * if the linked file was NOT existed (or extracted),
	 * try extracting the file content based on the equiv obj and
	 * swap their objects' information.
	 */
	UNYAFFS2_DEBUG("The target file is NOT extracted before ('%s'), "
		       "and a new file will be created based on "
		       "the content of target '%s'.\n",
			unyaffs2_linkfile, fpath);

	/* backup the file variant */
	memcpy(&variant, &obj->variant, sizeof(obj->variant));
	memset(&obj->variant, 0, sizeof(obj->variant));

	obj->type = equiv->type;
	obj->mode = equiv->mode;
	obj->uid = equiv->uid;
	obj->gid = equiv->gid;
	obj->atime = equiv->atime;
	obj->mtime = equiv->mtime;
	obj->ctime = equiv->ctime;

	switch (obj->type) {
	case YAFFS_OBJECT_TYPE_FILE:
	case YAFFS_OBJECT_TYPE_SYMLINK:
	case YAFFS_OBJECT_TYPE_CHR:
	case YAFFS_OBJECT_TYPE_BLK:
	case YAFFS_OBJECT_TYPE_FIFO:
	case YAFFS_OBJECT_TYPE_SOCK:
		memcpy(&obj->variant, &equiv->variant,
		       sizeof(union unyaffs2_file_variant));
		break;
	default:
		break;
	}

	retval = unyaffs2_extract_obj(fpath, obj);

	/*
	 * if the file content is NOT extracted, restore the object; 
	 * otherwise, swap two objects information.
	 */
	if (retval) {
		UNYAFFS2_DEBUG("extract file content failed, restore it\n");
		obj->type = YAFFS_OBJECT_TYPE_HARDLINK;
		memcpy(&obj->variant, &variant, sizeof(variant));
		unlink(fpath);
	}
	else {
		equiv->type = YAFFS_OBJECT_TYPE_HARDLINK;
		memset(&equiv->variant, 0, sizeof(equiv->variant));
		equiv->variant.hardlink.equiv_obj = obj;
	}

	return retval;
}

static int
unyaffs2_extract_obj (const char *fpath, struct unyaffs2_obj *obj)
{
	int retval = 0;

	switch (obj->type) {
	case YAFFS_OBJECT_TYPE_FILE:
		retval =
#ifdef _HAVE_MMAP
		unyaffs2_extract_file_mmap(unyaffs2_mmapinfo.addr,
					   unyaffs2_mmapinfo.size,
					   fpath, obj);
#else
		unyaffs2_extract_file(unyaffs2_image_fd,
				      fpath, obj);
#endif
		break;
	case YAFFS_OBJECT_TYPE_DIRECTORY:
		retval = unyaffs2_mkdir(fpath, 0755);
		break;
	case YAFFS_OBJECT_TYPE_HARDLINK:
		retval = unyaffs2_extract_hardlink(unyaffs2_curfile, obj);
		break;
	case YAFFS_OBJECT_TYPE_SYMLINK:
		retval = -1;
		if (obj->variant.symlink.alias) { 
			unlink(fpath);
			retval = symlink(obj->variant.symlink.alias, fpath);
		}
		break;
	case YAFFS_OBJECT_TYPE_CHR:
	case YAFFS_OBJECT_TYPE_BLK:
	case YAFFS_OBJECT_TYPE_FIFO:
	case YAFFS_OBJECT_TYPE_SOCK:
		retval = mknod(fpath, obj->mode,
			       obj->variant.dev.rdev);
		break;
	default:
		UNYAFFS2_DEBUG("skipped (unsupported type 0x%x).\n", obj->type);
		break;
	}

	return retval;
}

static int
unyaffs2_extract_objtree (struct unyaffs2_obj *obj)
{
	int retval = 0;
	struct list_head *p;
#ifndef _HAVE_MMAP
	ssize_t reads = 0;
#endif
	char *dstfile = unyaffs2_curfile;

	struct yaffs_ext_tags tag;
	struct yaffs_obj_hdr oh;
	struct unyaffs2_obj *child;

	enum yaffs_obj_type type = YAFFS_OBJECT_TYPE_UNKNOWN;
	static const char *type_str[] = {"????", "FILE", "SLNK", "DIR", "HLNK",
					 "CHR", "BLK", "FIFO", "SOCK"};

	if (obj == NULL) {
		if (unyaffs2_objtree.root == NULL)
			return 0;

		return unyaffs2_extract_objtree(unyaffs2_objtree.root);
	}

	if (!obj->valid) {
		UNYAFFS2_ERROR("obj %u is invalid (type: 0x%x)!!!\n",
				obj->obj_id, obj->type);
		return -1;
	}

	/* skip root object always */
	if (obj->obj_id == YAFFS_OBJECTID_ROOT) {
		type = YAFFS_OBJECT_TYPE_DIRECTORY;
		obj->extracted = 1;
		goto next;
	}

	/*
	 * extract the object content
	 */
#ifdef _HAVE_MMAP
	memcpy(unyaffs2_databuf, unyaffs2_mmapinfo.addr + obj->hdr_off,
	       unyaffs2_bufsize);

	if (memcmp(unyaffs2_databuf, unyaffs2_mmapinfo.addr + obj->hdr_off,
	    unyaffs2_bufsize)) {
#else
	lseek(unyaffs2_image_fd, obj->hdr_off, SEEK_SET);
	reads = safe_read(unyaffs2_image_fd,
			  unyaffs2_databuf, unyaffs2_bufsize);

	if (reads != unyaffs2_bufsize) {
#endif
		/* parse image failed */
		UNYAFFS2_DEBUG("read image failed @ offset %lu\n",
				obj->hdr_off);
		return -1;
	}

	memcpy(&oh, unyaffs2_databuf, sizeof(struct yaffs_obj_hdr));
	if (UNYAFFS2_ISENDIAN)
		oh_endian_convert(&oh);

	retval = unyaffs2_isempty(unyaffs2_databuf, unyaffs2_bufsize);
	unyaffs2_extract_ptags(&tag, unyaffs2_databuf + unyaffs2_chunksize,
			       NULL, 1);

	if (retval || tag.ecc_result == YAFFS_ECC_RESULT_UNFIXED ||
	    tag.chunk_used == 0 || tag.chunk_id != 0 ||
	    tag.obj_id != obj->obj_id || oh.parent_obj_id != obj->parent_id) {
		/* parse image failed */
		UNYAFFS2_DEBUG("image corrupted @ offset %lu "
			       "(is the same image)\n", obj->hdr_off);
		return -1;
	}

	/* format the file path */
	if (unyaffs2_curfile[0] != '\0' &&
	    unyaffs2_curfile[strlen(unyaffs2_curfile) - 1] != '/')
		strncat(unyaffs2_curfile, "/",
			sizeof(unyaffs2_curfile) -
			strlen(unyaffs2_curfile) - 1);

	strncat(unyaffs2_curfile, obj->name,
		 sizeof(unyaffs2_curfile) - strlen(unyaffs2_curfile) - 1);

	UNYAFFS2_VERBOSE("NOW: '%s'. ", unyaffs2_curfile);

	/* if a file(set) is specified */
	if (!list_empty(&unyaffs2_specfile_list)) {
		struct unyaffs2_specfile *spec = NULL;
		dstfile = NULL;
		spec = unyaffs2_specfile_find(unyaffs2_curfile);
		if (spec) {
			dstfile = unyaffs2_prefix_basestr(unyaffs2_curfile,
							  spec->path);
			if (!strcmp(unyaffs2_curfile, spec->path))
				spec->obj = obj;
		}
	}

	if (dstfile)
		retval = unyaffs2_extract_obj(dstfile, obj);

next:
	if (retval) {
		UNYAFFS2_ERROR("object %u: [%4s] '%s' (FAILED).\n",
				obj->obj_id, type_str[type], unyaffs2_curfile);
	}
	else {

		unyaffs2_image_objs++;

		UNYAFFS2_PROGRESS_BAR(unyaffs2_image_objs,
				      unyaffs2_objtree.objs);

		if (dstfile)
			obj->extracted = 1;

		UNYAFFS2_VERBOSE("\robject %u: [%4s] '%s'%s.\n",
				  obj->obj_id, type_str[obj->type],
				  unyaffs2_curfile,
				  obj->extracted ? "" : " (skip)");

		/* search its children if it is a directory. */
		if (obj->type == YAFFS_OBJECT_TYPE_DIRECTORY) {
			list_for_each(p, &obj->children) {
				child = list_entry(p, unyaffs2_obj_t, siblings);
				/* extract contents as more as possible */
				retval |= unyaffs2_extract_objtree(child);
			}
		}
	}

	/* restore current file path */
	if (!strcmp(dirname(unyaffs2_curfile), "."))
		unyaffs2_curfile[0] = '\0';

	return retval;
}

/*----------------------------------------------------------------------------*/

static int
unyaffs2_load_spare (const char *oobfile)
{
	int fd, retval = 0;
	ssize_t reads;

	if (oobfile == NULL)
		return 0;

	if ((fd = open(oobfile, O_RDONLY)) < 0) {
		UNYAFFS2_DEBUG("open image failed: %s\n", strerror(errno));
		return -1;
	}

	reads = safe_read(fd, &nand_oob_user, sizeof(nand_ecclayout_t));
	if (reads != sizeof(nand_ecclayout_t)) {
		UNYAFFS2_DEBUG("read image failed: %s\n", strerror(errno));
		retval = -1;
	}

	close(fd);

	return retval;
}

/*----------------------------------------------------------------------------*/

static int
unyaffs2_extract_image (const char *imgfile, const char *dirpath)
{
	int retval = -1;
	struct stat statbuf;
	struct unyaffs2_obj *root;

	unyaffs2_image_fd = open(imgfile, O_RDONLY);
	if (unyaffs2_image_fd < 0) {
		UNYAFFS2_ERROR("cannot open the image file: '%s': %s\n",
				imgfile, strerror(errno));
		return -1;
	}

	/* verify whether the input image is valid */
	if (fstat(unyaffs2_image_fd, &statbuf) < 0 ||
	    !S_ISREG(statbuf.st_mode)) {
		UNYAFFS2_ERROR("image is not a regular file: '%s'\n", imgfile);
		goto free_and_out;
	}

	if ((statbuf.st_size % (unyaffs2_chunksize + unyaffs2_sparesize)) != 0)
		UNYAFFS2_WARN("warning: image size (%lu)"
			      "is NOT a multiple of (%u + %u).\n",
			      statbuf.st_size, unyaffs2_chunksize,
			      unyaffs2_sparesize);

#if _HAVE_MMAP
	unyaffs2_mmapinfo.addr = mmap(NULL, statbuf.st_size, PROT_READ,
				      MAP_PRIVATE, unyaffs2_image_fd, 0);
	if (unyaffs2_mmapinfo.addr == MAP_FAILED) {
		UNYAFFS2_ERROR("mapping image failed: %s\n", strerror(errno));
		goto free_and_out;
	}

	unyaffs2_mmapinfo.size = statbuf.st_size;
#endif

	unyaffs2_bufsize = unyaffs2_chunksize + unyaffs2_sparesize;
	unyaffs2_databuf = (unsigned char *)malloc(unyaffs2_bufsize);
	if (unyaffs2_databuf == NULL) {
		UNYAFFS2_ERROR("cannot allocate working buffer (%u bytes): %s",
				unyaffs2_chunksize + unyaffs2_sparesize,
				strerror(errno));
		goto free_and_out;
	}

	umask(0);

	if (unyaffs2_mkdir(dirpath, 0755) < 0 || chdir(dirpath) < 0 ||
	    (root = unyaffs2_create_fakeroot(".")) == NULL) {
		UNYAFFS2_ERROR("cannot access the directory '%s' "
			       "(permission deny?)", dirpath);
		goto free_and_out;
	}

	unyaffs2_objtable_init();
	unyaffs2_objtree_init(&unyaffs2_objtree);

	unyaffs2_objtable_insert(root);

	/* stage 1: scanning image */
	UNYAFFS2_PRINTF("\n");
	UNYAFFS2_PRINTF("scanning image '%s'... [*]", imgfile);

	if (unyaffs2_scan_img() < 0)
		goto exit_and_out;

	UNYAFFS2_PRINTF("\b\b\b[done]\nscanning complete, total objects: %d\n",
			unyaffs2_image_objs);

	UNYAFFS2_PRINTF("\n");
	UNYAFFS2_PRINTF("building fs tree ... [*]");
	if (unyaffs2_build_objtree() < 0) {
		UNYAFFS2_ERROR("\nerror while building fs tree");
		goto exit_and_out;
	}

	UNYAFFS2_PRINTF("\b\b\b[done]\n");
	UNYAFFS2_PRINTF("building complete, total objects: %d\n",
			unyaffs2_objtree.objs);

	/* stage 2: extracting image */
	UNYAFFS2_PRINTF("\n");
	UNYAFFS2_PRINTF("extracting image into '%s'\n", dirpath);

	UNYAFFS2_PROGRESS_INIT();
	unyaffs2_image_objs = 0;

	/* extract objs in the obj tree */
	memset(unyaffs2_curfile, 0, sizeof(unyaffs2_curfile));
	retval = unyaffs2_extract_objtree(unyaffs2_objtree.root);

	/* modify attr for objects in the objtree */
	UNYAFFS2_PRINTF("\nmodify files attributes... [*]");

	if (!list_empty(&unyaffs2_specfile_list)) {
		struct list_head *p;
		struct unyaffs2_specfile *spec;
		list_for_each(p, &unyaffs2_specfile_list) {
			spec = list_entry(p, unyaffs2_specfile_t, list);
			if (spec->obj) {
				memset(unyaffs2_curfile, 0,
				       sizeof(unyaffs2_curfile));
				unyaffs2_objtree_chattr(spec->obj);
			}
			else {
				UNYAFFS2_ERROR("specified file was NOT found: "
					       "'%s'\n", spec->path);
			}
		}
	}
	else {
		memset(unyaffs2_curfile, 0, sizeof(unyaffs2_curfile));
		unyaffs2_objtree_chattr(unyaffs2_objtree.root);
	}

	if (!retval)
		UNYAFFS2_PRINTF("\b\b\b[done]\n");

exit_and_out:
	unyaffs2_objtree_exit(&unyaffs2_objtree);
	unyaffs2_objtable_exit();
free_and_out:
	if (unyaffs2_image_fd >= 0)
		close(unyaffs2_image_fd);
	if (unyaffs2_databuf)
		free(unyaffs2_databuf);

	return retval;
}

/*----------------------------------------------------------------------------*/

static int
unyaffs2_helper (void)
{
	UNYAFFS2_HELP("unyaffs2 %s - A utility to extract the yaffs2 image\n\n", YAFFS2UTILS_VERSION);
	UNYAFFS2_HELP("Usage: unyaffs2 [-h|--help] [-e|--endian] [-v|--verbose]\n"
		      "                [-p|--pagesize pagesize] [-s|--sparesize sparesize]\n"
		      "                [-o|--oobimg oobimage] [-f|--fileset file] [--yaffs-ecclayout]\n"
		      "                imgfile dirname\n\n");
	UNYAFFS2_HELP("Options :\n");
	UNYAFFS2_HELP("  -h                 display this help message and exit.\n");
	UNYAFFS2_HELP("  -e                 convert endian differed from local machine.\n");
	UNYAFFS2_HELP("  -v                 verbose details instead of progress bar.\n");
	UNYAFFS2_HELP("  -p pagesize        page size of target device.\n"
		      "                     (512|2048(default)|4096|(8192|16384) bytes)\n");
	UNYAFFS2_HELP("  -s sparesize       spare size of target device.\n"
		      "                     (default: pagesize/32 bytes; max: pagesize)\n");
	UNYAFFS2_HELP("  -o oobimage        load external oob image file.\n");;
	UNYAFFS2_HELP("  -f file            extract the specified file selection.\n");;
	UNYAFFS2_HELP("  --yaffs-ecclayout  use yaffs oob scheme instead of the Linux MTD default.\n");

	return -1;
}

/*----------------------------------------------------------------------------*/

int
main (int argc, char* argv[])
{
	int retval;
	char *imgfile = NULL, *dirpath = NULL, *oobfile = NULL;

	int option, option_index;
	static const char *short_options = "hvep:s:o:f:";
	static const struct option long_options[] = {
		{"pagesize",		required_argument, 	0, 'p'},
		{"sparesize",		required_argument,	0, 's'},
		{"oobimg",		required_argument, 	0, 'o'},
		{"fileset",		required_argument, 	0, 'f'},
		{"endian",		no_argument, 		0, 'e'},
		{"verbose",		no_argument,	 	0, 'v'},
		{"yaffs-ecclayout",	no_argument,	 	0, 'y'},
		{"help",		no_argument, 		0, 'h'},
		{NULL,			no_argument,		0, '\0'},
	};

	unyaffs2_chunksize = DEFAULT_CHUNKSIZE;

	while ((option = getopt_long(argc, argv, short_options,
				     long_options, &option_index)) != EOF) 
	{
		switch (option) {
		case 'p':
			unyaffs2_chunksize = strtol(optarg, NULL, 10);
			break;
		case 's':
			unyaffs2_sparesize = strtol(optarg, NULL, 10);
			break;
		case 'o':
			oobfile = optarg;
			break;
		case 'f':
			retval = unyaffs2_specfile_insert(optarg);
			if (retval) {
				UNYAFFS2_ERROR("cannot specify the file '%s' "
					       "(invalid string?)", optarg);
				unyaffs2_specfile_exit();
				return -1;
			}
			break;
		case 'e':
			unyaffs2_flags |= UNYAFFS2_FLAGS_ENDIAN;
			break;
		case 'v':
			unyaffs2_flags |= UNYAFFS2_FLAGS_VERBOSE;
			break;
		case 'y':
			unyaffs2_flags |= UNYAFFS2_FLAGS_YAFFSECC;
			break;
		case 'h':
		default:
			return unyaffs2_helper();
		}
	}

	if (argc - optind < 2)
		return unyaffs2_helper();

	imgfile = argv[optind];
	dirpath = argv[optind + 1];

	UNYAFFS2_PRINTF("unyaffs2 %s: image extracting tool for YAFFS2.\n",
			YAFFS2UTILS_VERSION);

	if (getuid() != 0) {
		unyaffs2_flags |= UNYAFFS2_FLAGS_NONROOT;
		UNYAFFS2_WARN("warning: non-root users.\n");
	}

	/* load spare image if it is existed */
	unyaffs2_ecclayout = NULL;
	if (oobfile) {
		if (unyaffs2_load_spare(oobfile) < 0) {
			UNYAFFS2_ERROR("read oob image failed.\n");
			return -1;
		}
		unyaffs2_ecclayout = &nand_oob_user;
		/* FIXME: verify for the various ecc layout */
	}

	/* validate the page size */
	unyaffs2_extract_ptags = &unyaffs2_extract_ptags2;
	switch (unyaffs2_chunksize) {
	case 512:
		unyaffs2_flags |= UNYAFFS2_FLAGS_YAFFS1;
		unyaffs2_extract_ptags = &unyaffs2_extract_ptags1;
		if (oobfile == NULL)
			unyaffs2_ecclayout = &nand_oob_16;
		break;
	case 2048:
		if (unyaffs2_ecclayout == NULL)
			unyaffs2_ecclayout = UNYAFFS2_ISYAFFSECC ?
					     &yaffs_nand_oob_64 : &nand_oob_64;
		break;
	case 4096:
	case 8192:
	case 16384:
		/* FIXME: The OOB scheme for 8192 and 16384. */
		if (unyaffs2_ecclayout == NULL)
			unyaffs2_ecclayout = UNYAFFS2_ISYAFFSECC ?
					     &yaffs_nand_oob_128 :
					     &nand_oob_128;
		break;
	default:
		UNYAFFS2_ERROR("%u bytes page size is not supported.\n",
				unyaffs2_chunksize);
		return -1;
	}

	/* spare size */
	if (!unyaffs2_sparesize)
		unyaffs2_sparesize = unyaffs2_chunksize / 32;

	if (unyaffs2_sparesize > unyaffs2_chunksize) {
		UNYAFFS2_ERROR("spare size is too large (%u).\n",
				unyaffs2_sparesize);
		return -1;
	}

	retval = unyaffs2_extract_image(imgfile, dirpath);
	if (!retval) {
		UNYAFFS2_PRINTF("\noperation complete,\n"
				"files were extracted into '%s'.\n", dirpath);
	}
	else {
		UNYAFFS2_ERROR("\n\noperation incomplete,\n"
			       "files contents may be broken!!!\n");
	}

	unyaffs2_specfile_exit();

	return retval;
}
