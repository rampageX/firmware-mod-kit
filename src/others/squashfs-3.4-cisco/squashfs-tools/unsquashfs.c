/* Modifications were made by Cisco Systems, Inc. on or before Thu Feb 16 11:28:44 PST 2012 */
/*
 * Unsquash a squashfs filesystem.  This is a highly compressed read only filesystem.
 *
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@lougher.demon.co.uk>
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
 * unsquash.c
 */

#define CONFIG_SQUASHFS_1_0_COMPATIBILITY
#define CONFIG_SQUASHFS_2_0_COMPATIBILITY

#define TRUE 1
#define FALSE 0
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <zlib.h>
#include <sys/mman.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <regex.h>
#include <fnmatch.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#ifndef linux
#define __BYTE_ORDER BYTE_ORDER
#define __BIG_ENDIAN BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#else
#include <endian.h>
#endif

#include "squashfs_fs.h"
#include "read_fs.h"
#include "global.h"

#include "lzmainterface.h"
#include "LzmaDec.h"


#ifdef SQUASHFS_TRACE
#define TRACE(s, args...)		do { \
						pthread_mutex_lock(&screen_mutex); \
						if(progress_enabled) \
							printf("\n"); \
						printf("unsquashfs: "s, ## args); \
						pthread_mutex_unlock(&screen_mutex);\
					} while(0)
#else
#define TRACE(s, args...)
#endif

#define ERROR(s, args...)		do { \
						pthread_mutex_lock(&screen_mutex); \
						if(progress_enabled) \
							fprintf(stderr, "\n"); \
						fprintf(stderr, s, ## args); \
						pthread_mutex_unlock(&screen_mutex);\
					} while(0)

#define EXIT_UNSQUASH(s, args...)	do { \
						pthread_mutex_lock(&screen_mutex); \
						fprintf(stderr, "FATAL ERROR aborting: "s, ## args); \
						pthread_mutex_unlock(&screen_mutex);\
						exit(1); \
					} while(0)

#define CALCULATE_HASH(start)	(start & 0xffff)

struct hash_table_entry {
	int	start;
	int	bytes;
	struct hash_table_entry *next;
};

struct inode {
	int blocks;
	char *block_ptr;
	long long data;
	int fragment;
	int frag_bytes;
	gid_t gid;
	int inode_number;
	int mode;
	int offset;
	long long start;
	char symlink[65536];
	time_t time;
	int type;
	uid_t uid;
};

typedef struct squashfs_operations {
	struct dir *(*squashfs_opendir)(unsigned int block_start, unsigned int offset, struct inode **i);
	void (*read_fragment)(unsigned int fragment, long long *start_block, int *size);
	void (*read_fragment_table)();
	void (*read_block_list)(unsigned int *block_list, char *block_ptr, int blocks);
	struct inode *(*read_inode)(unsigned int start_block, unsigned int offset);
} squashfs_operations;

struct test {
	int mask;
	int value;
	int position;
	char mode;
};


/* Cache status struct.  Caches are used to keep
  track of memory buffers passed between different threads */
struct cache {
	int	max_buffers;
	int	count;
	int	buffer_size;
	int	wait_free;
	int	wait_pending;
	pthread_mutex_t	mutex;
	pthread_cond_t wait_for_free;
	pthread_cond_t wait_for_pending;
	struct cache_entry *free_list;
	struct cache_entry *hash_table[65536];
};

/* struct describing a cache entry passed between threads */
struct cache_entry {
	struct cache *cache;
	long long block;
	int	size;
	int	used;
	int error;
	int	pending;
	struct cache_entry *hash_next;
	struct cache_entry *hash_prev;
	struct cache_entry *free_next;
	struct cache_entry *free_prev;
	char *data;
};

/* struct describing queues used to pass data between threads */
struct queue {
	int	size;
	int	readp;
	int	writep;
	pthread_mutex_t	mutex;
	pthread_cond_t empty;
	pthread_cond_t full;
	void **data;
};

struct cache *fragment_cache, *data_cache;
struct queue *to_reader, *to_deflate, *to_writer, *from_writer;
pthread_t *thread, *deflator_thread;
pthread_mutex_t	fragment_mutex;

/* user options that control parallelisation */
int processors = -1;
/* default size of fragment buffer in Mbytes */
#define FRAGMENT_BUFFER_DEFAULT 256
/* default size of data buffer in Mbytes */
#define DATA_BUFFER_DEFAULT 256

squashfs_super_block sBlk;
squashfs_operations s_ops;

int bytes = 0, swap, file_count = 0, dir_count = 0, sym_count = 0,
	dev_count = 0, fifo_count = 0;
char *inode_table = NULL, *directory_table = NULL;
struct hash_table_entry *inode_table_hash[65536], *directory_table_hash[65536];
int fd;
squashfs_fragment_entry *fragment_table;
squashfs_fragment_entry_2 *fragment_table_2;
unsigned int *uid_table, *guid_table;
unsigned int cached_frag = SQUASHFS_INVALID_FRAG;
char *fragment_data;
char *file_data;
char *data;
unsigned int block_size;
unsigned int block_log;
int lsonly = FALSE, info = FALSE, force = FALSE, short_ls = TRUE, use_regex = FALSE;
char **created_inode;
int root_process;
int columns;
int rotate = 0;
pthread_mutex_t	screen_mutex;
pthread_cond_t progress_wait;
int progress = TRUE, progress_enabled = FALSE;
unsigned int total_blocks = 0, total_files = 0, total_inodes = 0;
unsigned int cur_blocks = 0;

int lookup_type[] = {
	0,
	S_IFDIR,
	S_IFREG,
	S_IFLNK,
	S_IFBLK,
	S_IFCHR,
	S_IFIFO,
	S_IFSOCK,
	S_IFDIR,
	S_IFREG
};

struct test table[] = {
	{ S_IFMT, S_IFSOCK, 0, 's' },
	{ S_IFMT, S_IFLNK, 0, 'l' },
	{ S_IFMT, S_IFBLK, 0, 'b' },
	{ S_IFMT, S_IFDIR, 0, 'd' },
	{ S_IFMT, S_IFCHR, 0, 'c' },
	{ S_IFMT, S_IFIFO, 0, 'p' },
	{ S_IRUSR, S_IRUSR, 1, 'r' },
	{ S_IWUSR, S_IWUSR, 2, 'w' },
	{ S_IRGRP, S_IRGRP, 4, 'r' },
	{ S_IWGRP, S_IWGRP, 5, 'w' },
	{ S_IROTH, S_IROTH, 7, 'r' },
	{ S_IWOTH, S_IWOTH, 8, 'w' },
	{ S_IXUSR | S_ISUID, S_IXUSR | S_ISUID, 3, 's' },
	{ S_IXUSR | S_ISUID, S_ISUID, 3, 'S' },
	{ S_IXUSR | S_ISUID, S_IXUSR, 3, 'x' },
	{ S_IXGRP | S_ISGID, S_IXGRP | S_ISGID, 6, 's' },
	{ S_IXGRP | S_ISGID, S_ISGID, 6, 'S' },
	{ S_IXGRP | S_ISGID, S_IXGRP, 6, 'x' },
	{ S_IXOTH | S_ISVTX, S_IXOTH | S_ISVTX, 9, 't' },
	{ S_IXOTH | S_ISVTX, S_ISVTX, 9, 'T' },
	{ S_IXOTH | S_ISVTX, S_IXOTH, 9, 'x' },
	{ 0, 0, 0, 0}
};

void progress_bar(long long current, long long max, int columns);
void update_progress_bar();

void sigwinch_handler()
{
	struct winsize winsize;

	if(ioctl(1, TIOCGWINSZ, &winsize) == -1) {
		ERROR("TIOCGWINSZ ioctl failed, defaulting to 80 columns\n");
		columns = 80;
	} else
		columns = winsize.ws_col;
}


void sigalrm_handler()
{
	rotate = (rotate + 1) % 4;
}


struct queue *queue_init(int size)
{
	struct queue *queue = malloc(sizeof(struct queue));

	if(queue == NULL)
		return NULL;

	if((queue->data = malloc(sizeof(void *) * (size + 1))) == NULL) {
		free(queue);
		return NULL;
	}

	queue->size = size + 1;
	queue->readp = queue->writep = 0;
	pthread_mutex_init(&queue->mutex, NULL);
	pthread_cond_init(&queue->empty, NULL);
	pthread_cond_init(&queue->full, NULL);

	return queue;
}


void queue_put(struct queue *queue, void *data)
{
	int nextp;

	pthread_mutex_lock(&queue->mutex);

	while((nextp = (queue->writep + 1) % queue->size) == queue->readp)
		pthread_cond_wait(&queue->full, &queue->mutex);

	queue->data[queue->writep] = data;
	queue->writep = nextp;
	pthread_cond_signal(&queue->empty);
	pthread_mutex_unlock(&queue->mutex);
}


void *queue_get(struct queue *queue)
{
	void *data;
	pthread_mutex_lock(&queue->mutex);

	while(queue->readp == queue->writep)
		pthread_cond_wait(&queue->empty, &queue->mutex);

	data = queue->data[queue->readp];
	queue->readp = (queue->readp + 1) % queue->size;
	pthread_cond_signal(&queue->full);
	pthread_mutex_unlock(&queue->mutex);

	return data;
}


/* Called with the cache mutex held */
void insert_hash_table(struct cache *cache, struct cache_entry *entry)
{
	int hash = CALCULATE_HASH(entry->block);

	entry->hash_next = cache->hash_table[hash];
	cache->hash_table[hash] = entry;
	entry->hash_prev = NULL;
	if(entry->hash_next)
		entry->hash_next->hash_prev = entry;
}


/* Called with the cache mutex held */
void remove_hash_table(struct cache *cache, struct cache_entry *entry)
{
	if(entry->hash_prev)
		entry->hash_prev->hash_next = entry->hash_next;
	else
		cache->hash_table[CALCULATE_HASH(entry->block)] = entry->hash_next;
	if(entry->hash_next)
		entry->hash_next->hash_prev = entry->hash_prev;

	entry->hash_prev = entry->hash_next = NULL;
}


/* Called with the cache mutex held */
void insert_free_list(struct cache *cache, struct cache_entry *entry)
{
	if(cache->free_list) {
		entry->free_next = cache->free_list;
		entry->free_prev = cache->free_list->free_prev;
		cache->free_list->free_prev->free_next = entry;
		cache->free_list->free_prev = entry;
	} else {
		cache->free_list = entry;
		entry->free_prev = entry->free_next = entry;
	}
}


/* Called with the cache mutex held */
void remove_free_list(struct cache *cache, struct cache_entry *entry)
{
	if(entry->free_prev == NULL && entry->free_next == NULL)
		/* not in free list */
		return;
	else if(entry->free_prev == entry && entry->free_next == entry) {
		/* only this entry in the free list */
		cache->free_list = NULL;
	} else {
		/* more than one entry in the free list */
		entry->free_next->free_prev = entry->free_prev;
		entry->free_prev->free_next = entry->free_next;
		if(cache->free_list == entry)
			cache->free_list = entry->free_next;
	}

	entry->free_prev = entry->free_next = NULL;
}


struct cache *cache_init(int buffer_size, int max_buffers)
{
	struct cache *cache = malloc(sizeof(struct cache));

	if(cache == NULL)
		return NULL;

	cache->max_buffers = max_buffers;
	cache->buffer_size = buffer_size;
	cache->count = 0;
	cache->free_list = NULL;
	memset(cache->hash_table, 0, sizeof(struct cache_entry *) * 65536);
	cache->wait_free = FALSE;
	cache->wait_pending = FALSE;
	pthread_mutex_init(&cache->mutex, NULL);
	pthread_cond_init(&cache->wait_for_free, NULL);
	pthread_cond_init(&cache->wait_for_pending, NULL);

	return cache;
}


struct cache_entry *cache_get(struct cache *cache, long long block, int size)
{
	/* Get a block out of the cache.  If the block isn't in the cache
 	 * it is added and queued to the reader() and deflate() threads for reading
 	 * off disk and decompression.  The cache grows until max_blocks is reached,
 	 * once this occurs existing discarded blocks on the free list are reused */
	int hash = CALCULATE_HASH(block);
	struct cache_entry *entry;

	pthread_mutex_lock(&cache->mutex);

	for(entry = cache->hash_table[hash]; entry; entry = entry->hash_next)
		if(entry->block == block)
			break;

	if(entry) {
		/* found the block in the cache, increment used count and
 		 * if necessary remove from free list so it won't disappear
 		 */
		entry->used ++;
		remove_free_list(cache, entry);
		pthread_mutex_unlock(&cache->mutex);
	} else {
		/* not in the cache */
		
		/* first try to allocate new block */
		if(cache->count < cache->max_buffers) {
			entry = malloc(sizeof(struct cache_entry));
			if(entry == NULL)
				goto failed;
			entry->data = malloc(cache->buffer_size);
			if(entry->data == NULL) {
				free(entry);
				goto failed;
			}
			entry->cache = cache;
			entry->free_prev = entry->free_next = NULL;
			cache->count ++;
		} else {
			/* try to get from free list */
			while(cache->free_list == NULL) {
				cache->wait_free = TRUE;
				pthread_cond_wait(&cache->wait_for_free, &cache->mutex);
			}
			entry = cache->free_list;
			remove_free_list(cache, entry);
			remove_hash_table(cache, entry);
		}

		/* initialise block and insert into the hash table */
		entry->block = block;
		entry->size = size;
		entry->used = 1;
		entry->error = FALSE;
		entry->pending = TRUE;
		insert_hash_table(cache, entry);

		/* queue to read thread to read and ultimately (via the decompress
 		 * threads) decompress the buffer
 		 */
		pthread_mutex_unlock(&cache->mutex);
		queue_put(to_reader, entry);
	}

	return entry;

failed:
	pthread_mutex_unlock(&cache->mutex);
	return NULL;
}

	
void cache_block_ready(struct cache_entry *entry, int error)
{
	/* mark cache entry as being complete, reading and (if necessary)
 	 * decompression has taken place, and the buffer is valid for use.
 	 * If an error occurs reading or decompressing, the buffer also 
 	 * becomes ready but with an error... */
	pthread_mutex_lock(&entry->cache->mutex);
	entry->pending = FALSE;
	entry->error = error;

	/* if the wait_pending flag is set, one or more threads may be waiting on
 	 * this buffer */
	if(entry->cache->wait_pending) {
		entry->cache->wait_pending = FALSE;
		pthread_cond_broadcast(&entry->cache->wait_for_pending);
	}

	pthread_mutex_unlock(&entry->cache->mutex);
}


void cache_block_wait(struct cache_entry *entry)
{
	/* wait for this cache entry to become ready, when reading and (if necessary)
 	 * decompression has taken place */
	pthread_mutex_lock(&entry->cache->mutex);

	while(entry->pending) {
		entry->cache->wait_pending = TRUE;
		pthread_cond_wait(&entry->cache->wait_for_pending, &entry->cache->mutex);
	}

	pthread_mutex_unlock(&entry->cache->mutex);
}


void cache_block_put(struct cache_entry *entry)
{
	/* finished with this cache entry, once the usage count reaches zero it
 	 * can be reused and is put onto the free list.  As it remains accessible
 	 * via the hash table it can be found getting a new lease of life before it
 	 * is reused. */
	pthread_mutex_lock(&entry->cache->mutex);

	entry->used --;
	if(entry->used == 0) {
		insert_free_list(entry->cache, entry);

		/* if the wait_free flag is set, one or more threads may be waiting on
 	 	 * this buffer */
		if(entry->cache->wait_free) {
			entry->cache->wait_free = FALSE;
			pthread_cond_broadcast(&entry->cache->wait_for_free);
		}
	}

	pthread_mutex_unlock(&entry->cache->mutex);
}


char *modestr(char *str, int mode)
{
	int i;

	strcpy(str, "----------");

	for(i = 0; table[i].mask != 0; i++) {
		if((mode & table[i].mask) == table[i].value)
			str[table[i].position] = table[i].mode;
	}

	return str;
}


#define TOTALCHARS  25
int print_filename(char *pathname, struct inode *inode)
{
	char str[11], dummy[100], dummy2[100], *userstr, *groupstr;
	int padchars;
	struct passwd *user;
	struct group *group;
	struct tm *t;

	if(short_ls) {
		printf("%s\n", pathname);
		return 1;
	}

	if((user = getpwuid(inode->uid)) == NULL) {
		sprintf(dummy, "%d", inode->uid);
		userstr = dummy;
	} else
		userstr = user->pw_name;
		 
	if((group = getgrgid(inode->gid)) == NULL) {
		sprintf(dummy2, "%d", inode->gid);
		groupstr = dummy2;
	} else
		groupstr = group->gr_name;

	printf("%s %s/%s ", modestr(str, inode->mode), userstr, groupstr);

	switch(inode->mode & S_IFMT) {
		case S_IFREG:
		case S_IFDIR:
		case S_IFSOCK:
		case S_IFIFO:
		case S_IFLNK:
			padchars = TOTALCHARS - strlen(userstr) - strlen(groupstr);

			printf("%*lld ", padchars > 0 ? padchars : 0, inode->data);
			break;
		case S_IFCHR:
		case S_IFBLK:
			padchars = TOTALCHARS - strlen(userstr) - strlen(groupstr) - 7; 

			printf("%*s%3d,%3d ", padchars > 0 ? padchars : 0, " ", (int) inode->data >> 8, (int) inode->data & 0xff);
			break;
	}

	t = localtime(&inode->time);

	printf("%d-%02d-%02d %02d:%02d %s", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, pathname);
	if((inode->mode & S_IFMT) == S_IFLNK)
		printf(" -> %s", inode->symlink);
	printf("\n");
		
	return 1;
}
	

int add_entry(struct hash_table_entry *hash_table[], int start, int bytes)
{
	int hash = CALCULATE_HASH(start);
	struct hash_table_entry *hash_table_entry;

	if((hash_table_entry = malloc(sizeof(struct hash_table_entry))) == NULL) {
		ERROR("add_hash: out of memory in malloc\n");
		return FALSE;
	}

	hash_table_entry->start = start;
	hash_table_entry->bytes = bytes;
	hash_table_entry->next = hash_table[hash];
	hash_table[hash] = hash_table_entry;

	return TRUE;
}


int lookup_entry(struct hash_table_entry *hash_table[], int start)
{
	int hash = CALCULATE_HASH(start);
	struct hash_table_entry *hash_table_entry;

	for(hash_table_entry = hash_table[hash]; hash_table_entry;
				hash_table_entry = hash_table_entry->next)
		if(hash_table_entry->start == start)
			return hash_table_entry->bytes;

	return -1;
}


int read_bytes(long long byte, int bytes, char *buff)
{
	off_t off = byte;

	TRACE("read_bytes: reading from position 0x%llx, bytes %d\n", byte, bytes);

	if(lseek(fd, off, SEEK_SET) == -1) {
		ERROR("Lseek failed because %s\n", strerror(errno));
		return FALSE;
	}

	if(read(fd, buff, bytes) == -1) {
		ERROR("Read on destination failed because %s\n", strerror(errno));
		return FALSE;
	}

	return TRUE;
}


int read_block(long long start, long long *next, char *block)
{
	unsigned short c_byte;
	int offset = 2;
	
	if(swap) {
		if(read_bytes(start, 2, block) == FALSE)
			goto failed;
		((unsigned char *) &c_byte)[1] = block[0];
		((unsigned char *) &c_byte)[0] = block[1]; 
	} else 
		if(read_bytes(start, 2, (char *)&c_byte) == FALSE)
			goto failed;

	TRACE("read_block: block @0x%llx, %d %s bytes\n", start, SQUASHFS_COMPRESSED_SIZE(c_byte), SQUASHFS_COMPRESSED(c_byte) ? "compressed" : "uncompressed");

	if(SQUASHFS_CHECK_DATA(sBlk.flags))
		offset = 3;
	if(SQUASHFS_COMPRESSED(c_byte)) {
		char buffer[SQUASHFS_METADATA_SIZE];
		int res;
		unsigned long bytes = SQUASHFS_METADATA_SIZE;

		c_byte = SQUASHFS_COMPRESSED_SIZE(c_byte);
		if(read_bytes(start + offset, c_byte, buffer) == FALSE)
			goto failed;

   		if (!is_lzma(buffer[0])) {
    		res = uncompress((unsigned char *) block, &bytes, (const unsigned char *) buffer, c_byte);

    		if(res != Z_OK) {
    			if(res == Z_MEM_ERROR)
    				ERROR("zlib::uncompress failed, not enough memory\n");
    			else if(res == Z_BUF_ERROR)
    				ERROR("zlib::uncompress failed, not enough room in output buffer\n");
    			else
    				ERROR("zlib::uncompress failed, unknown error %d\n", res);
    			goto failed;
    		}

        }else{
            /*Start:Added @2010-02-25 to suporrt lzma*/
			if((res = LzmaUncompress((unsigned char *) block, &bytes, (const unsigned char *) buffer, c_byte)) != SZ_OK)
				ERROR("LzmaUncompress: error (%d)\n", res);
            /*End:Added @2010-02-25 to suporrt lzma*/
		}
		if(next)
			*next = start + offset + c_byte;
		return bytes;
	} else {
		c_byte = SQUASHFS_COMPRESSED_SIZE(c_byte);
		if(read_bytes(start + offset, c_byte, block) == FALSE)
			goto failed;
		if(next)
			*next = start + offset + c_byte;
		return c_byte;
	}

failed:
	return FALSE;
}


int read_data_block(long long start, unsigned int size, char *block)
{
	int res;
	unsigned long bytes = block_size;
	int c_byte = SQUASHFS_COMPRESSED_SIZE_BLOCK(size);

	TRACE("read_data_block: block @0x%llx, %d %s bytes\n", start, SQUASHFS_COMPRESSED_SIZE_BLOCK(c_byte), SQUASHFS_COMPRESSED_BLOCK(c_byte) ? "compressed" : "uncompressed");

	if(SQUASHFS_COMPRESSED_BLOCK(size)) {
		if(read_bytes(start, c_byte, data) == FALSE)
			return 0;

		if (!is_lzma(data[0])) {

    		res = uncompress((unsigned char *) block, &bytes, (const unsigned char *) data, c_byte);

    		if(res != Z_OK) {
    			if(res == Z_MEM_ERROR)
    				ERROR("zlib::uncompress failed, not enough memory\n");
    			else if(res == Z_BUF_ERROR)
    				ERROR("zlib::uncompress failed, not enough room in output buffer\n");
    			else
    				ERROR("zlib::uncompress failed, unknown error %d\n", res);
    			return 0;
    		}
        }else{ 
            /*Start:Added @2010-02-25 to suporrt lzma*/
			if((res = LzmaUncompress((unsigned char *) block, &bytes, (const unsigned char *) data, c_byte)) != SZ_OK)
				ERROR("LzmaUncompress: error (%d)\n", res);
            /*End:Added @2010-02-25 to suporrt lzma*/
		}
        
		return bytes;
	} else {
		if(read_bytes(start, c_byte, block) == FALSE)
			return 0;

		return c_byte;
	}
}


void read_block_list(unsigned int *block_list, char *block_ptr, int blocks)
{
	if(swap) {
		unsigned int sblock_list[blocks];
		memcpy(sblock_list, block_ptr, blocks * sizeof(unsigned int));
		SQUASHFS_SWAP_INTS(block_list, sblock_list, blocks);
	} else
		memcpy(block_list, block_ptr, blocks * sizeof(unsigned int));
}


void read_block_list_1(unsigned int *block_list, char *block_ptr, int blocks)
{
	unsigned short block_size;
	int i;

	for(i = 0; i < blocks; i++, block_ptr += 2) {
		if(swap) {
			unsigned short sblock_size;
			memcpy(&sblock_size, block_ptr, sizeof(unsigned short));
			SQUASHFS_SWAP_SHORTS((&block_size), &sblock_size, 1);
		} else
			memcpy(&block_size, block_ptr, sizeof(unsigned short));
		block_list[i] = SQUASHFS_COMPRESSED_SIZE(block_size) | (SQUASHFS_COMPRESSED(block_size) ? 0 : SQUASHFS_COMPRESSED_BIT_BLOCK);
	}
}


void uncompress_inode_table(long long start, long long end)
{
	int size = 0, bytes = 0, res;

	while(start < end) {
		if((size - bytes < SQUASHFS_METADATA_SIZE) &&
				((inode_table = realloc(inode_table, size +=
				SQUASHFS_METADATA_SIZE)) == NULL))
			EXIT_UNSQUASH("uncompress_inode_table: out of memory in realloc\n");
		TRACE("uncompress_inode_table: reading block 0x%llx\n", start);
		add_entry(inode_table_hash, start, bytes);
		if((res = read_block(start, &start, inode_table + bytes)) == 0) {
			free(inode_table);
			EXIT_UNSQUASH("uncompress_inode_table: failed to read block\n");
		}
		bytes += res;
	}
}


int set_attributes(char *pathname, int mode, uid_t uid, gid_t guid, time_t time, unsigned int set_mode)
{
	struct utimbuf times = { time, time };

	if(utime(pathname, &times) == -1) {
		ERROR("set_attributes: failed to set time on %s, because %s\n", pathname, strerror(errno));
		return FALSE;
	}

	if(root_process) {
		if(chown(pathname, uid, guid) == -1) {
			ERROR("set_attributes: failed to change uid and gids on %s, because %s\n", pathname, strerror(errno));
			return FALSE;
		}
	} else
		mode &= ~07000;

	if((set_mode || (mode & 07000)) && chmod(pathname, (mode_t) mode) == -1) {
		ERROR("set_attributes: failed to change mode %s, because %s\n", pathname, strerror(errno));
		return FALSE;
	}

	return TRUE;
}


void read_uids_guids()
{
	if((uid_table = malloc((sBlk.no_uids + sBlk.no_guids) * sizeof(unsigned int))) == NULL)
		EXIT_UNSQUASH("read_uids_guids: failed to allocate uid/gid table\n");

	guid_table = uid_table + sBlk.no_uids;

	if(swap) {
		unsigned int suid_table[sBlk.no_uids + sBlk.no_guids];

		if(read_bytes(sBlk.uid_start, (sBlk.no_uids + sBlk.no_guids)
				* sizeof(unsigned int), (char *) suid_table) ==
				FALSE)
			EXIT_UNSQUASH("read_uids_guids: failed to read uid/gid table\n");
		SQUASHFS_SWAP_INTS(uid_table, suid_table, sBlk.no_uids + sBlk.no_guids);
	} else
		if(read_bytes(sBlk.uid_start, (sBlk.no_uids + sBlk.no_guids)
				* sizeof(unsigned int), (char *) uid_table) ==
				FALSE)
			EXIT_UNSQUASH("read_uids_guids: failed to read uid/gid table\n");
}


void read_fragment_table()
{
	int i, indexes = SQUASHFS_FRAGMENT_INDEXES(sBlk.fragments);
	squashfs_fragment_index fragment_table_index[indexes];

	TRACE("read_fragment_table: %d fragments, reading %d fragment indexes from 0x%llx\n", sBlk.fragments, indexes, sBlk.fragment_table_start);

	if(sBlk.fragments == 0)
		return;

	if((fragment_table = (squashfs_fragment_entry *)
			malloc(sBlk.fragments *
			sizeof(squashfs_fragment_entry))) == NULL)
		EXIT_UNSQUASH("read_fragment_table: failed to allocate fragment table\n");

	if(swap) {
		squashfs_fragment_index sfragment_table_index[indexes];

		read_bytes(sBlk.fragment_table_start, SQUASHFS_FRAGMENT_INDEX_BYTES(sBlk.fragments), (char *) sfragment_table_index);
		SQUASHFS_SWAP_FRAGMENT_INDEXES(fragment_table_index, sfragment_table_index, indexes);
	} else
		read_bytes(sBlk.fragment_table_start, SQUASHFS_FRAGMENT_INDEX_BYTES(sBlk.fragments), (char *) fragment_table_index);

	for(i = 0; i < indexes; i++) {
		int length = read_block(fragment_table_index[i], NULL,
		((char *) fragment_table) + (i * SQUASHFS_METADATA_SIZE));
		TRACE("Read fragment table block %d, from 0x%llx, length %d\n", i, fragment_table_index[i], length);
	}

	if(swap) {
		squashfs_fragment_entry sfragment;
		for(i = 0; i < sBlk.fragments; i++) {
			SQUASHFS_SWAP_FRAGMENT_ENTRY((&sfragment), (&fragment_table[i]));
			memcpy((char *) &fragment_table[i], (char *) &sfragment, sizeof(squashfs_fragment_entry));
		}
	}
}


void read_fragment_table_2()
{
	int i, indexes = SQUASHFS_FRAGMENT_INDEXES_2(sBlk.fragments);
	unsigned int fragment_table_index[indexes];

	TRACE("read_fragment_table: %d fragments, reading %d fragment indexes from 0x%llx\n", sBlk.fragments, indexes, sBlk.fragment_table_start);

	if(sBlk.fragments == 0)
		return;

	if((fragment_table_2 = (squashfs_fragment_entry_2 *)
			malloc(sBlk.fragments *
			sizeof(squashfs_fragment_entry))) == NULL)
		EXIT_UNSQUASH("read_fragment_table: failed to allocate fragment table\n");

	if(swap) {
		 unsigned int sfragment_table_index[indexes];

		read_bytes(sBlk.fragment_table_start, SQUASHFS_FRAGMENT_INDEX_BYTES_2(sBlk.fragments), (char *) sfragment_table_index);
		SQUASHFS_SWAP_FRAGMENT_INDEXES_2(fragment_table_index, sfragment_table_index, indexes);
	} else
		read_bytes(sBlk.fragment_table_start, SQUASHFS_FRAGMENT_INDEX_BYTES_2(sBlk.fragments), (char *) fragment_table_index);

	for(i = 0; i < indexes; i++) {
		int length = read_block(fragment_table_index[i], NULL,
		((char *) fragment_table_2) + (i * SQUASHFS_METADATA_SIZE));
		TRACE("Read fragment table block %d, from 0x%x, length %d\n", i, fragment_table_index[i], length);
	}

	if(swap) {
		squashfs_fragment_entry_2 sfragment;
		for(i = 0; i < sBlk.fragments; i++) {
			SQUASHFS_SWAP_FRAGMENT_ENTRY_2((&sfragment), (&fragment_table_2[i]));
			memcpy((char *) &fragment_table_2[i], (char *) &sfragment, sizeof(squashfs_fragment_entry_2));
		}
	}
}


void read_fragment_table_1()
{
}


void read_fragment(unsigned int fragment, long long *start_block, int *size)
{
	TRACE("read_fragment: reading fragment %d\n", fragment);

	squashfs_fragment_entry *fragment_entry = &fragment_table[fragment];
	*start_block = fragment_entry->start_block;
	*size = fragment_entry->size;
}


void read_fragment_2(unsigned int fragment, long long *start_block, int *size)
{
	TRACE("read_fragment: reading fragment %d\n", fragment);

	squashfs_fragment_entry_2 *fragment_entry = &fragment_table_2[fragment];
	*start_block = fragment_entry->start_block;
	*size = fragment_entry->size;
}


int lseek_broken = FALSE;
char *zero_data;

int write_block(int file_fd, char *buffer, int size, int hole)
{
	off_t off = hole;

	if(hole) {
		if(lseek_broken == FALSE && lseek(file_fd, off, SEEK_CUR) == -1) {
			/* failed to seek beyond end of file */
			if((zero_data = malloc(block_size)) == NULL)
				EXIT_UNSQUASH("write_block: failed to alloc zero data block\n");
			memset(zero_data, 0, block_size);
			lseek_broken = TRUE;
		}
		if(lseek_broken) {
			int blocks = (hole + block_size -1) / block_size;
			int avail_bytes, i;
			for(i = 0; i < blocks; i++, hole -= avail_bytes) {
				avail_bytes = hole > block_size ? block_size : hole;
				if(write(file_fd, zero_data, avail_bytes) < avail_bytes)
					goto failure;
			}
		}
	}

	if(write(file_fd, buffer, size) < size)
		goto failure;

	return TRUE;

failure:
	return FALSE;
}


struct file_entry {
	int offset;
	int size;
	struct cache_entry *buffer;
};


struct squashfs_file {
	int fd;
	int blocks;
	long long file_size;
	int mode;
	uid_t uid;
	gid_t gid;
	time_t time;
	char *pathname;
};


int write_file(struct inode *inode, char *pathname)
{
	unsigned int file_fd, i;
	unsigned int *block_list;
	int file_end = inode->data / block_size;
	long long start = inode->start;
	struct squashfs_file *file;

	TRACE("write_file: regular file, blocks %d\n", inode->blocks);

	if((file_fd = open(pathname, O_CREAT | O_WRONLY | (force ? O_TRUNC : 0), (mode_t) inode->mode & 0777)) == -1) {
		ERROR("write_file: failed to create file %s, because %s\n", pathname,
			strerror(errno));
		return FALSE;
	}

	if((block_list = malloc(inode->blocks * sizeof(unsigned int))) == NULL)
		EXIT_UNSQUASH("write_file: unable to malloc block list\n");

	s_ops.read_block_list(block_list, inode->block_ptr, inode->blocks);

	if((file = malloc(sizeof(struct squashfs_file))) == NULL)
		EXIT_UNSQUASH("write_file: unable to malloc file\n");

	/* the writer thread is queued a squashfs_file structure describing the
 	 * file.  If the file has one or more blocks or a fragments they are queued
 	 * separately (references to blocks in the cache). */
	file->fd = file_fd;
	file->file_size = inode->data;
	file->mode = inode->mode;
	file->gid = inode->gid;
	file->uid = inode->uid;
	file->time = inode->time;
	file->pathname = strdup(pathname);
	file->blocks = inode->blocks + (inode->frag_bytes > 0);
	queue_put(to_writer, file);

	for(i = 0; i < inode->blocks; i++) {
		int c_byte = SQUASHFS_COMPRESSED_SIZE_BLOCK(block_list[i]);
		struct file_entry *block = malloc(sizeof(struct file_entry *));

		if(block == NULL)
			EXIT_UNSQUASH("write_file: unable to malloc file\n");
		block->offset = 0;
		block->size = i == file_end ? inode->data & (block_size - 1) : block_size;
		if(block_list[i] == 0) /* sparse file */
			block->buffer = NULL;
		else {
			block->buffer = cache_get(data_cache, start, block_list[i]);
			if(block->buffer == NULL)
				EXIT_UNSQUASH("write_file: cache_get failed\n");
			start += c_byte;
		}
		queue_put(to_writer, block);
	}

	if(inode->frag_bytes) {
		int size;
		long long start;
		struct file_entry *block = malloc(sizeof(struct file_entry *));

		if(block == NULL)
			EXIT_UNSQUASH("write_file: unable to malloc file\n");
		s_ops.read_fragment(inode->fragment, &start, &size);
		block->buffer = cache_get(fragment_cache, start, size);
		if(block->buffer == NULL)
			EXIT_UNSQUASH("write_file: cache_get failed\n");
		block->offset = inode->offset;
		block->size = inode->frag_bytes;
		queue_put(to_writer, block);
	}

	free(block_list);
	return TRUE;
}


static struct inode *read_inode(unsigned int start_block, unsigned int offset)
{
	static squashfs_inode_header header;
	long long start = sBlk.inode_table_start + start_block;
	int bytes = lookup_entry(inode_table_hash, start);
	char *block_ptr = inode_table + bytes + offset;
	static struct inode i;

	if(bytes == -1)
		goto error;

	if(swap) {
		squashfs_base_inode_header sinode;
		memcpy(&sinode, block_ptr, sizeof(header.base));
		SQUASHFS_SWAP_BASE_INODE_HEADER(&header.base, &sinode, sizeof(squashfs_base_inode_header));
	} else
		memcpy(&header.base, block_ptr, sizeof(header.base));

	i.uid = (uid_t) uid_table[header.base.uid];
	i.gid = header.base.guid == SQUASHFS_GUIDS ? i.uid : (uid_t) guid_table[header.base.guid];
	i.mode = lookup_type[header.base.inode_type] | header.base.mode;
	i.type = header.base.inode_type;
	i.time = header.base.mtime;
	i.inode_number = header.base.inode_number;

	switch(header.base.inode_type) {
		case SQUASHFS_DIR_TYPE: {
			squashfs_dir_inode_header *inode = &header.dir;

			if(swap) {
				squashfs_dir_inode_header sinode;
				memcpy(&sinode, block_ptr, sizeof(header.dir));
				SQUASHFS_SWAP_DIR_INODE_HEADER(&header.dir, &sinode);
			} else
				memcpy(&header.dir, block_ptr, sizeof(header.dir));

			i.data = inode->file_size;
			i.offset = inode->offset;
			i.start = inode->start_block;
			break;
		}
		case SQUASHFS_LDIR_TYPE: {
			squashfs_ldir_inode_header *inode = &header.ldir;

			if(swap) {
				squashfs_ldir_inode_header sinode;
				memcpy(&sinode, block_ptr, sizeof(header.ldir));
				SQUASHFS_SWAP_LDIR_INODE_HEADER(&header.ldir, &sinode);
			} else
				memcpy(&header.ldir, block_ptr, sizeof(header.ldir));

			i.data = inode->file_size;
			i.offset = inode->offset;
			i.start = inode->start_block;
			break;
		}
		case SQUASHFS_FILE_TYPE: {
			squashfs_reg_inode_header *inode = &header.reg;

			if(swap) {
				squashfs_reg_inode_header sinode;
				memcpy(&sinode, block_ptr, sizeof(sinode));
				SQUASHFS_SWAP_REG_INODE_HEADER(inode, &sinode);
			} else
				memcpy(inode, block_ptr, sizeof(*inode));

			i.data = inode->file_size;
			i.frag_bytes = inode->fragment == SQUASHFS_INVALID_FRAG ?
				0 : inode->file_size % sBlk.block_size;
			i.fragment = inode->fragment;
			i.offset = inode->offset;
			i.blocks = inode->fragment == SQUASHFS_INVALID_FRAG ?
				(inode->file_size + sBlk.block_size - 1) >>
				sBlk.block_log : inode->file_size >> sBlk.block_log;
			i.start = inode->start_block;
			i.block_ptr = block_ptr + sizeof(*inode);
			break;
		}	
		case SQUASHFS_LREG_TYPE: {
			squashfs_lreg_inode_header *inode = &header.lreg;

			if(swap) {
				squashfs_lreg_inode_header sinode;
				memcpy(&sinode, block_ptr, sizeof(sinode));
				SQUASHFS_SWAP_LREG_INODE_HEADER(inode, &sinode);
			} else
				memcpy(inode, block_ptr, sizeof(*inode));

			i.data = inode->file_size;
			i.frag_bytes = inode->fragment == SQUASHFS_INVALID_FRAG ?
				0 : inode->file_size % sBlk.block_size;
			i.fragment = inode->fragment;
			i.offset = inode->offset;
			i.blocks = inode->fragment == SQUASHFS_INVALID_FRAG ?
				(inode->file_size + sBlk.block_size - 1) >>
				sBlk.block_log : inode->file_size >> sBlk.block_log;
			i.start = inode->start_block;
			i.block_ptr = block_ptr + sizeof(*inode);
			break;
		}	
		case SQUASHFS_SYMLINK_TYPE: {
			squashfs_symlink_inode_header *inodep = &header.symlink;

			if(swap) {
				squashfs_symlink_inode_header sinodep;
				memcpy(&sinodep, block_ptr, sizeof(sinodep));
				SQUASHFS_SWAP_SYMLINK_INODE_HEADER(inodep, &sinodep);
			} else
				memcpy(inodep, block_ptr, sizeof(*inodep));

			strncpy(i.symlink, block_ptr + sizeof(squashfs_symlink_inode_header), inodep->symlink_size);
			i.symlink[inodep->symlink_size] = '\0';
			i.data = inodep->symlink_size;
			break;
		}
 		case SQUASHFS_BLKDEV_TYPE:
	 	case SQUASHFS_CHRDEV_TYPE: {
			squashfs_dev_inode_header *inodep = &header.dev;

			if(swap) {
				squashfs_dev_inode_header sinodep;
				memcpy(&sinodep, block_ptr, sizeof(sinodep));
				SQUASHFS_SWAP_DEV_INODE_HEADER(inodep, &sinodep);
			} else
				memcpy(inodep, block_ptr, sizeof(*inodep));

			i.data = inodep->rdev;
			break;
			}
		case SQUASHFS_FIFO_TYPE:
		case SQUASHFS_SOCKET_TYPE:
			i.data = 0;
			break;
		default:
			ERROR("Unknown inode type %d in read_inode!\n", header.base.inode_type);
			return NULL;
	}
	return &i;

error:
	return NULL;
}


int create_inode(char *pathname, struct inode *i)
{
	TRACE("create_inode: pathname %s\n", pathname);

	if(created_inode[i->inode_number - 1]) {
		TRACE("create_inode: hard link\n");
		if(force)
			unlink(pathname);

		if(link(created_inode[i->inode_number - 1], pathname) == -1) {
			ERROR("create_inode: failed to create hardlink, because %s\n", strerror(errno));
			return FALSE;
		}

		return TRUE;
	}

	switch(i->type) {
		case SQUASHFS_FILE_TYPE:
		case SQUASHFS_LREG_TYPE:
			TRACE("create_inode: regular file, file_size %lld, blocks %d\n", i->data, i->blocks);

			if(write_file(i, pathname))
				file_count ++;
			break;
		case SQUASHFS_SYMLINK_TYPE:
			TRACE("create_inode: symlink, symlink_size %lld\n", i->data);

			if(force)
				unlink(pathname);

			if(symlink(i->symlink, pathname) == -1) {
				ERROR("create_inode: failed to create symlink %s, because %s\n", pathname,
					strerror(errno));
				break;
			}

			if(root_process) {
				if(lchown(pathname, i->uid, i->gid) == -1)
					ERROR("create_inode: failed to change uid and gids on %s, because %s\n", pathname, strerror(errno));
			}

			sym_count ++;
			break;
 		case SQUASHFS_BLKDEV_TYPE:
	 	case SQUASHFS_CHRDEV_TYPE: {
			int chrdev = i->type == SQUASHFS_CHRDEV_TYPE;
			TRACE("create_inode: dev, rdev 0x%llx\n", i->data);

			if(root_process) {
				if(force)
					unlink(pathname);

				if(mknod(pathname, chrdev ? S_IFCHR : S_IFBLK,
					makedev((i->data >> 8) & 0xff, i->data & 0xff)) == -1) {
					ERROR("create_inode: failed to create %s device %s, because %s\n",
						chrdev ? "character" : "block", pathname, strerror(errno));
					break;
				}
				set_attributes(pathname, i->mode, i->uid, i->gid, i->time, TRUE);
				dev_count ++;
			} else
				ERROR("create_inode: could not create %s device %s, because you're not superuser!\n",
					chrdev ? "character" : "block", pathname);
			break;
		}
		case SQUASHFS_FIFO_TYPE:
			TRACE("create_inode: fifo\n");

			if(force)
				unlink(pathname);

			if(mknod(pathname, S_IFIFO, 0) == -1) {
				ERROR("create_inode: failed to create fifo %s, because %s\n",
					pathname, strerror(errno));
				break;
			}
			set_attributes(pathname, i->mode, i->uid, i->gid, i->time, TRUE);
			fifo_count ++;
			break;
		case SQUASHFS_SOCKET_TYPE:
			TRACE("create_inode: socket\n");
			ERROR("create_inode: socket %s ignored\n", pathname);
			break;
		default:
			ERROR("Unknown inode type %d in create_inode_table!\n", i->type);
			return FALSE;
	}

	created_inode[i->inode_number - 1] = strdup(pathname);

	return TRUE;
}


struct inode *read_inode_2(unsigned int start_block, unsigned int offset)
{
	static squashfs_inode_header_2 header;
	long long start = sBlk.inode_table_start + start_block;
	int bytes = lookup_entry(inode_table_hash, start);
	char *block_ptr = inode_table + bytes + offset;
	static struct inode i;
	static int inode_number = 1;

	if(bytes == -1)
		goto error;

	if(swap) {
		squashfs_base_inode_header_2 sinode;
		memcpy(&sinode, block_ptr, sizeof(header.base));
		SQUASHFS_SWAP_BASE_INODE_HEADER_2(&header.base, &sinode, sizeof(squashfs_base_inode_header_2));
	} else
		memcpy(&header.base, block_ptr, sizeof(header.base));

    i.uid = (uid_t) uid_table[header.base.uid];
    i.gid = header.base.guid == SQUASHFS_GUIDS ? i.uid : (uid_t) guid_table[header.base.guid];
	i.mode = lookup_type[header.base.inode_type] | header.base.mode;
	i.type = header.base.inode_type;
	i.time = sBlk.mkfs_time;
	i.inode_number = inode_number++;

	switch(header.base.inode_type) {
		case SQUASHFS_DIR_TYPE: {
			squashfs_dir_inode_header_2 *inode = &header.dir;

			if(swap) {
				squashfs_dir_inode_header sinode;
				memcpy(&sinode, block_ptr, sizeof(header.dir));
				SQUASHFS_SWAP_DIR_INODE_HEADER_2(&header.dir, &sinode);
			} else
				memcpy(&header.dir, block_ptr, sizeof(header.dir));

			i.data = inode->file_size;
			i.offset = inode->offset;
			i.start = inode->start_block;
			i.time = inode->mtime;
			break;
		}
		case SQUASHFS_LDIR_TYPE: {
			squashfs_ldir_inode_header_2 *inode = &header.ldir;

			if(swap) {
				squashfs_ldir_inode_header sinode;
				memcpy(&sinode, block_ptr, sizeof(header.ldir));
				SQUASHFS_SWAP_LDIR_INODE_HEADER_2(&header.ldir, &sinode);
			} else
				memcpy(&header.ldir, block_ptr, sizeof(header.ldir));

			i.data = inode->file_size;
			i.offset = inode->offset;
			i.start = inode->start_block;
			i.time = inode->mtime;
			break;
		}
		case SQUASHFS_FILE_TYPE: {
			squashfs_reg_inode_header_2 *inode = &header.reg;

			if(swap) {
				squashfs_reg_inode_header_2 sinode;
				memcpy(&sinode, block_ptr, sizeof(sinode));
				SQUASHFS_SWAP_REG_INODE_HEADER_2(inode, &sinode);
			} else
				memcpy(inode, block_ptr, sizeof(*inode));

			i.data = inode->file_size;
			i.time = inode->mtime;
			i.frag_bytes = inode->fragment == SQUASHFS_INVALID_FRAG ?
				0 : inode->file_size % sBlk.block_size;
			i.fragment = inode->fragment;
			i.offset = inode->offset;
			i.blocks = inode->fragment == SQUASHFS_INVALID_FRAG ?
				(inode->file_size + sBlk.block_size - 1) >>
				sBlk.block_log : inode->file_size >> sBlk.block_log;
			i.start = inode->start_block;
			i.block_ptr = block_ptr + sizeof(*inode);
			break;
		}	
		case SQUASHFS_SYMLINK_TYPE: {
			squashfs_symlink_inode_header_2 *inodep = &header.symlink;

			if(swap) {
				squashfs_symlink_inode_header_2 sinodep;
				memcpy(&sinodep, block_ptr, sizeof(sinodep));
				SQUASHFS_SWAP_SYMLINK_INODE_HEADER_2(inodep, &sinodep);
			} else
				memcpy(inodep, block_ptr, sizeof(*inodep));

			strncpy(i.symlink, block_ptr + sizeof(squashfs_symlink_inode_header_2), inodep->symlink_size);
			i.symlink[inodep->symlink_size] = '\0';
			i.data = inodep->symlink_size;
			break;
		}
 		case SQUASHFS_BLKDEV_TYPE:
	 	case SQUASHFS_CHRDEV_TYPE: {
			squashfs_dev_inode_header_2 *inodep = &header.dev;

			if(swap) {
				squashfs_dev_inode_header_2 sinodep;
				memcpy(&sinodep, block_ptr, sizeof(sinodep));
				SQUASHFS_SWAP_DEV_INODE_HEADER_2(inodep, &sinodep);
			} else
				memcpy(inodep, block_ptr, sizeof(*inodep));

			i.data = inodep->rdev;
			break;
			}
		case SQUASHFS_FIFO_TYPE:
		case SQUASHFS_SOCKET_TYPE:
			i.data = 0;
			break;
		default:
			ERROR("Unknown inode type %d in read_inode_header_2!\n", header.base.inode_type);
			return NULL;
	}
	return &i;

error:
	return NULL;
}


struct inode *read_inode_1(unsigned int start_block, unsigned int offset)
{
	static squashfs_inode_header_1 header;
	long long start = sBlk.inode_table_start + start_block;
	int bytes = lookup_entry(inode_table_hash, start);
	char *block_ptr = inode_table + bytes + offset;
	static struct inode i;
	static int inode_number = 1;

	if(bytes == -1)
		goto error;

	if(swap) {
		squashfs_base_inode_header_1 sinode;
		memcpy(&sinode, block_ptr, sizeof(header.base));
		SQUASHFS_SWAP_BASE_INODE_HEADER_1(&header.base, &sinode, sizeof(squashfs_base_inode_header_1));
	} else
		memcpy(&header.base, block_ptr, sizeof(header.base));

    i.uid = (uid_t) uid_table[(header.base.inode_type - 1) / SQUASHFS_TYPES * 16 + header.base.uid];
	if(header.base.inode_type == SQUASHFS_IPC_TYPE) {
		squashfs_ipc_inode_header_1 *inodep = &header.ipc;

		if(swap) {
			squashfs_ipc_inode_header_1 sinodep;
			memcpy(&sinodep, block_ptr, sizeof(sinodep));
			SQUASHFS_SWAP_IPC_INODE_HEADER_1(inodep, &sinodep);
		} else
			memcpy(inodep, block_ptr, sizeof(*inodep));

		if(inodep->type == SQUASHFS_SOCKET_TYPE) {
			i.mode = S_IFSOCK | header.base.mode;
			i.type = SQUASHFS_SOCKET_TYPE;
		} else {
			i.mode = S_IFIFO | header.base.mode;
			i.type = SQUASHFS_FIFO_TYPE;
		}
		i.uid = (uid_t) uid_table[inodep->offset * 16 + inodep->uid];
	} else {
		i.mode = lookup_type[(header.base.inode_type - 1) % SQUASHFS_TYPES + 1] | header.base.mode;
		i.type = (header.base.inode_type - 1) % SQUASHFS_TYPES + 1;
	}
    i.gid = header.base.guid == 15 ? i.uid : (uid_t) guid_table[header.base.guid];
	i.time = sBlk.mkfs_time;
	i.inode_number = inode_number ++;

	switch(i.type) {
		case SQUASHFS_DIR_TYPE: {
			squashfs_dir_inode_header_1 *inode = &header.dir;

			if(swap) {
				squashfs_dir_inode_header_1 sinode;
				memcpy(&sinode, block_ptr, sizeof(header.dir));
				SQUASHFS_SWAP_DIR_INODE_HEADER_1(inode, &sinode);
			} else
			memcpy(inode, block_ptr, sizeof(header.dir));

			i.data = inode->file_size;
			i.start = inode->start_block;
			i.time = inode->mtime;
			break;
		}
		case SQUASHFS_FILE_TYPE: {
			squashfs_reg_inode_header_1 *inode = &header.reg;

			if(swap) {
				squashfs_reg_inode_header_1 sinode;
				memcpy(&sinode, block_ptr, sizeof(sinode));
				SQUASHFS_SWAP_REG_INODE_HEADER_1(inode, &sinode);
			} else
				memcpy(inode, block_ptr, sizeof(*inode));

			i.data = inode->file_size;
			i.time = inode->mtime;
			i.blocks = (inode->file_size + sBlk.block_size - 1) >>
				sBlk.block_log;
			i.start = inode->start_block;
			i.block_ptr = block_ptr + sizeof(*inode);
			i.fragment = 0;
			i.frag_bytes = 0;
			i.offset = 0;
			break;
		}	
		case SQUASHFS_SYMLINK_TYPE: {
			squashfs_symlink_inode_header_1 *inodep = &header.symlink;

			if(swap) {
				squashfs_symlink_inode_header_1 sinodep;
				memcpy(&sinodep, block_ptr, sizeof(sinodep));
				SQUASHFS_SWAP_SYMLINK_INODE_HEADER_1(inodep, &sinodep);
			} else
				memcpy(inodep, block_ptr, sizeof(*inodep));

			strncpy(i.symlink, block_ptr + sizeof(squashfs_symlink_inode_header_1), inodep->symlink_size);
			i.symlink[inodep->symlink_size] = '\0';
			i.data = inodep->symlink_size;
			break;
		}
 		case SQUASHFS_BLKDEV_TYPE:
	 	case SQUASHFS_CHRDEV_TYPE: {
			squashfs_dev_inode_header_1 *inodep = &header.dev;

			if(swap) {
				squashfs_dev_inode_header_1 sinodep;
				memcpy(&sinodep, block_ptr, sizeof(sinodep));
				SQUASHFS_SWAP_DEV_INODE_HEADER_1(inodep, &sinodep);
			} else
				memcpy(inodep, block_ptr, sizeof(*inodep));

			i.data = inodep->rdev;
			break;
			}
		case SQUASHFS_FIFO_TYPE:
		case SQUASHFS_SOCKET_TYPE: {
			i.data = 0;
			break;
			}
		default:
			ERROR("Unknown inode type %d in read_inode_header_1!\n", header.base.inode_type);
			return NULL;
	}
	return &i;

error:
	return NULL;
}


void uncompress_directory_table(long long start, long long end)
{
	int bytes = 0, size = 0, res;

	while(start < end) {
		if(size - bytes < SQUASHFS_METADATA_SIZE && (directory_table =
				realloc(directory_table, size +=
				SQUASHFS_METADATA_SIZE)) == NULL)
			EXIT_UNSQUASH("uncompress_directory_table: out of memory in realloc\n");
		TRACE("uncompress_directory_table: reading block 0x%llx\n", start);
		add_entry(directory_table_hash, start, bytes);
		if((res = read_block(start, &start, directory_table + bytes)) == 0)
			EXIT_UNSQUASH("uncompress_directory_table: failed to read block\n");
		bytes += res;
	}
}


#define DIR_ENT_SIZE	16

struct dir_ent	{
	char		name[SQUASHFS_NAME_LEN + 1];
	unsigned int	start_block;
	unsigned int	offset;
	unsigned int	type;
};

struct dir {
	int		dir_count;
	int 		cur_entry;
	unsigned int	mode;
	uid_t		uid;
	gid_t		guid;
	unsigned int	mtime;
	struct dir_ent	*dirs;
};


struct dir *squashfs_opendir(unsigned int block_start, unsigned int offset, struct inode **i)
{
	squashfs_dir_header dirh;
	char buffer[sizeof(squashfs_dir_entry) + SQUASHFS_NAME_LEN + 1];
	squashfs_dir_entry *dire = (squashfs_dir_entry *) buffer;
	long long start;
	int bytes;
	int dir_count, size;
	struct dir_ent *new_dir;
	struct dir *dir;

	TRACE("squashfs_opendir: inode start block %d, offset %d\n", block_start, offset);

	if((*i = s_ops.read_inode(block_start, offset)) == NULL) {
		ERROR("squashfs_opendir: failed to read directory inode %d\n", block_start);
		return NULL;
	}

	start = sBlk.directory_table_start + (*i)->start;
	bytes = lookup_entry(directory_table_hash, start);

	if(bytes == -1) {
		ERROR("squashfs_opendir: directory block %d not found!\n", block_start);
		return NULL;
	}

	bytes += (*i)->offset;
	size = (*i)->data + bytes - 3;

	if((dir = malloc(sizeof(struct dir))) == NULL) {
		ERROR("squashfs_opendir: malloc failed!\n");
		return NULL;
	}

	dir->dir_count = 0;
	dir->cur_entry = 0;
	dir->mode = (*i)->mode;
	dir->uid = (*i)->uid;
	dir->guid = (*i)->gid;
	dir->mtime = (*i)->time;
	dir->dirs = NULL;

	while(bytes < size) {			
		if(swap) {
			squashfs_dir_header sdirh;
			memcpy(&sdirh, directory_table + bytes, sizeof(sdirh));
			SQUASHFS_SWAP_DIR_HEADER(&dirh, &sdirh);
		} else
			memcpy(&dirh, directory_table + bytes, sizeof(dirh));
	
		dir_count = dirh.count + 1;
		TRACE("squashfs_opendir: Read directory header @ byte position %d, %d directory entries\n", bytes, dir_count);
		bytes += sizeof(dirh);

		while(dir_count--) {
			if(swap) {
				squashfs_dir_entry sdire;
				memcpy(&sdire, directory_table + bytes, sizeof(sdire));
				SQUASHFS_SWAP_DIR_ENTRY(dire, &sdire);
			} else
				memcpy(dire, directory_table + bytes, sizeof(*dire));
			bytes += sizeof(*dire);

			memcpy(dire->name, directory_table + bytes, dire->size + 1);
			dire->name[dire->size + 1] = '\0';
			TRACE("squashfs_opendir: directory entry %s, inode %d:%d, type %d\n", dire->name, dirh.start_block, dire->offset, dire->type);
			if((dir->dir_count % DIR_ENT_SIZE) == 0) {
				if((new_dir = realloc(dir->dirs, (dir->dir_count + DIR_ENT_SIZE) * sizeof(struct dir_ent))) == NULL) {
					ERROR("squashfs_opendir: realloc failed!\n");
					free(dir->dirs);
					free(dir);
					return NULL;
				}
				dir->dirs = new_dir;
			}
			strcpy(dir->dirs[dir->dir_count].name, dire->name);
			dir->dirs[dir->dir_count].start_block = dirh.start_block;
			dir->dirs[dir->dir_count].offset = dire->offset;
			dir->dirs[dir->dir_count].type = dire->type;
			dir->dir_count ++;
			bytes += dire->size + 1;
		}
	}

	return dir;
}


struct dir *squashfs_opendir_2(unsigned int block_start, unsigned int offset, struct inode **i)
{
	squashfs_dir_header_2 dirh;
	char buffer[sizeof(squashfs_dir_entry_2) + SQUASHFS_NAME_LEN + 1];
	squashfs_dir_entry_2 *dire = (squashfs_dir_entry_2 *) buffer;
	long long start;
	int bytes;
	int dir_count, size;
	struct dir_ent *new_dir;
	struct dir *dir;

	TRACE("squashfs_opendir: inode start block %d, offset %d\n", block_start, offset);

	if(((*i) = s_ops.read_inode(block_start, offset)) == NULL) {
		ERROR("squashfs_opendir: failed to read directory inode %d\n", block_start);
		return NULL;
	}

	start = sBlk.directory_table_start + (*i)->start;
	bytes = lookup_entry(directory_table_hash, start);

	if(bytes == -1) {
		ERROR("squashfs_opendir: directory block %d not found!\n", block_start);
		return NULL;
	}

	bytes += (*i)->offset;
	size = (*i)->data + bytes;

	if((dir = malloc(sizeof(struct dir))) == NULL) {
		ERROR("squashfs_opendir: malloc failed!\n");
		return NULL;
	}

	dir->dir_count = 0;
	dir->cur_entry = 0;
	dir->mode = (*i)->mode;
	dir->uid = (*i)->uid;
	dir->guid = (*i)->gid;
	dir->mtime = (*i)->time;
	dir->dirs = NULL;

	while(bytes < size) {			
		if(swap) {
			squashfs_dir_header_2 sdirh;
			memcpy(&sdirh, directory_table + bytes, sizeof(sdirh));
			SQUASHFS_SWAP_DIR_HEADER_2(&dirh, &sdirh);
		} else
			memcpy(&dirh, directory_table + bytes, sizeof(dirh));
	
		dir_count = dirh.count + 1;
		TRACE("squashfs_opendir: Read directory header @ byte position %d, %d directory entries\n", bytes, dir_count);
		bytes += sizeof(dirh);

		while(dir_count--) {
			if(swap) {
				squashfs_dir_entry_2 sdire;
				memcpy(&sdire, directory_table + bytes, sizeof(sdire));
				SQUASHFS_SWAP_DIR_ENTRY_2(dire, &sdire);
			} else
				memcpy(dire, directory_table + bytes, sizeof(*dire));
			bytes += sizeof(*dire);

			memcpy(dire->name, directory_table + bytes, dire->size + 1);
			dire->name[dire->size + 1] = '\0';
			TRACE("squashfs_opendir: directory entry %s, inode %d:%d, type %d\n", dire->name, dirh.start_block, dire->offset, dire->type);
			if((dir->dir_count % DIR_ENT_SIZE) == 0) {
				if((new_dir = realloc(dir->dirs, (dir->dir_count + DIR_ENT_SIZE) * sizeof(struct dir_ent))) == NULL) {
					ERROR("squashfs_opendir: realloc failed!\n");
					free(dir->dirs);
					free(dir);
					return NULL;
				}
				dir->dirs = new_dir;
			}
			strcpy(dir->dirs[dir->dir_count].name, dire->name);
			dir->dirs[dir->dir_count].start_block = dirh.start_block;
			dir->dirs[dir->dir_count].offset = dire->offset;
			dir->dirs[dir->dir_count].type = dire->type;
			dir->dir_count ++;
			bytes += dire->size + 1;
		}
	}

	return dir;
}


int squashfs_readdir(struct dir *dir, char **name, unsigned int *start_block,
unsigned int *offset, unsigned int *type)
{
	if(dir->cur_entry == dir->dir_count)
		return FALSE;

	*name = dir->dirs[dir->cur_entry].name;
	*start_block = dir->dirs[dir->cur_entry].start_block;
	*offset = dir->dirs[dir->cur_entry].offset;
	*type = dir->dirs[dir->cur_entry].type;
	dir->cur_entry ++;

	return TRUE;
}


void squashfs_closedir(struct dir *dir)
{
	free(dir->dirs);
	free(dir);
}


char *get_component(char *target, char *targname)
{
	while(*target == '/')
		target ++;

	while(*target != '/' && *target!= '\0')
		*targname ++ = *target ++;

	*targname = '\0';

	return target;
}


struct path_entry {
	char *name;
	regex_t *preg;
	struct pathname *paths;
};

struct pathname {
	int names;
	struct path_entry *name;
};

struct pathnames {
	int count;
	struct pathname *path[0];
};
#define PATHS_ALLOC_SIZE 10

void free_path(struct pathname *paths)
{
	int i;

	for(i = 0; i < paths->names; i++) {
		if(paths->name[i].paths)
			free_path(paths->name[i].paths);
		free(paths->name[i].name);
		if(paths->name[i].preg) {
			regfree(paths->name[i].preg);
			free(paths->name[i].preg);
		}
	}

	free(paths);
}


struct pathname *add_path(struct pathname *paths, char *target, char *alltarget)
{
	char targname[1024];
	int i, error;

	target = get_component(target, targname);

	if(paths == NULL) {
		if((paths = malloc(sizeof(struct pathname))) == NULL)
			EXIT_UNSQUASH("failed to allocate paths\n");

		paths->names = 0;
		paths->name = NULL;
	}

	for(i = 0; i < paths->names; i++)
		if(strcmp(paths->name[i].name, targname) == 0)
			break;

	if(i == paths->names) {
		/* allocate new name entry */
		paths->names ++;
		paths->name = realloc(paths->name, (i + 1) * sizeof(struct path_entry));
		paths->name[i].name = strdup(targname);
		paths->name[i].paths = NULL;
		if(use_regex) {
			paths->name[i].preg = malloc(sizeof(regex_t));
			error = regcomp(paths->name[i].preg, targname, REG_EXTENDED|REG_NOSUB);
			if(error) {
				char str[1024];

				regerror(error, paths->name[i].preg, str, 1024);
				EXIT_UNSQUASH("invalid regex %s in export %s, because %s\n", targname, alltarget, str);
			}
		} else
			paths->name[i].preg = NULL;

		if(target[0] == '\0')
			/* at leaf pathname component */
			paths->name[i].paths = NULL;
		else
			/* recurse adding child components */
			paths->name[i].paths = add_path(NULL, target, alltarget);
	} else {
		/* existing matching entry */
		if(paths->name[i].paths == NULL) {
			/* No sub-directory which means this is the leaf component of a
		   	   pre-existing extract which subsumes the extract currently
		   	   being added, in which case stop adding components */
		} else if(target[0] == '\0') {
			/* at leaf pathname component and child components exist from more
		       specific extracts, delete as they're subsumed by this extract
			*/
			free_path(paths->name[i].paths);
			paths->name[i].paths = NULL;
		} else
			/* recurse adding child components */
			add_path(paths->name[i].paths, target, alltarget);
	}

	return paths;
}


struct pathnames *init_subdir()
{
	struct pathnames *new = malloc(sizeof(struct pathnames *));
	new->count = 0;
	return new;
}


struct pathnames *add_subdir(struct pathnames *paths, struct pathname *path)
{
	if(paths->count % PATHS_ALLOC_SIZE == 0)
		paths = realloc(paths, sizeof(struct pathnames *) + (paths->count + PATHS_ALLOC_SIZE) * sizeof(struct pathname *));

	paths->path[paths->count++] = path;
	return paths;
}


void free_subdir(struct pathnames *paths)
{
	free(paths);
}


int matches(struct pathnames *paths, char *name, struct pathnames **new)
{
	int i, n;

	if(paths == NULL) {
		*new = NULL;
		return TRUE;
	}

	*new = init_subdir();

	for(n = 0; n < paths->count; n++) {
		struct pathname *path = paths->path[n];
		for(i = 0; i < path->names; i++) {
			int match = use_regex ?
				regexec(path->name[i].preg, name, (size_t) 0, NULL, 0) == 0 :
				fnmatch(path->name[i].name, name, FNM_PATHNAME|FNM_PERIOD|FNM_EXTMATCH) == 0;
			if(match && path->name[i].paths == NULL)
				/* match on a leaf component, any subdirectories will
				 * implicitly match, therefore return an empty new search set */
				goto empty_set;

			if(match)
				/* match on a non-leaf component, add any subdirectories to
				 * the new set of subdirectories to scan for this name */
				*new = add_subdir(*new, path->name[i].paths);
		}
	}

	if((*new)->count == 0) {
		/* no matching names found, delete empty search set, and return
        * FALSE */
		free_subdir(*new);
		*new = NULL;
		return FALSE;
	}

	/* one or more matches with sub-directories found (no leaf matches),
     * return new search set and return TRUE */
	return TRUE;

empty_set:
   /* found matching leaf exclude, return empty search set and return TRUE */
	free_subdir(*new);
	*new = NULL;
	return TRUE;
}


int pre_scan(char *parent_name, unsigned int start_block, unsigned int offset, struct pathnames *paths)
{
	unsigned int type;
	char *name, pathname[1024];
	struct pathnames *new;
	struct inode *i;
	struct dir *dir = s_ops.squashfs_opendir(start_block, offset, &i);

	if(dir == NULL) {
		ERROR("pre_scan: Failed to read directory %s (%x:%x)\n", parent_name, start_block, offset);
		return FALSE;
	}

	while(squashfs_readdir(dir, &name, &start_block, &offset, &type)) {
		struct inode *i;

		TRACE("pre_scan: name %s, start_block %d, offset %d, type %d\n", name, start_block, offset, type);

		if(!matches(paths, name, &new))
			continue;

		strcat(strcat(strcpy(pathname, parent_name), "/"), name);

		if(type == SQUASHFS_DIR_TYPE)
			pre_scan(parent_name, start_block, offset, new);
		else if(new == NULL) {
			if(type == SQUASHFS_FILE_TYPE || type == SQUASHFS_LREG_TYPE) {
				if((i = s_ops.read_inode(start_block, offset)) == NULL) {
					ERROR("failed to read header\n");
					continue;
				}
				if(created_inode[i->inode_number - 1] == NULL) {
					created_inode[i->inode_number - 1] = (char *) i;
					total_blocks += (i->data + (block_size - 1)) >> block_log;
				}
				total_files ++;
			}
			total_inodes ++;
		}

		free_subdir(new);
	}

	squashfs_closedir(dir);

	return TRUE;
}


int dir_scan(char *parent_name, unsigned int start_block, unsigned int offset, struct pathnames *paths)
{
	unsigned int type;
	char *name, pathname[1024];
	struct pathnames *new;
	struct inode *i;
	struct dir *dir = s_ops.squashfs_opendir(start_block, offset, &i);

	if(dir == NULL) {
		ERROR("dir_scan: Failed to read directory %s (%x:%x)\n", parent_name, start_block, offset);
		return FALSE;
	}

	if(lsonly || info)
		print_filename(parent_name, i);

	if(!lsonly && mkdir(parent_name, (mode_t) dir->mode) == -1 && (!force || errno != EEXIST)) {
		ERROR("dir_scan: failed to open directory %s, because %s\n", parent_name, strerror(errno));
		return FALSE;
	}

	while(squashfs_readdir(dir, &name, &start_block, &offset, &type)) {
		TRACE("dir_scan: name %s, start_block %d, offset %d, type %d\n", name, start_block, offset, type);


		if(!matches(paths, name, &new))
			continue;

		strcat(strcat(strcpy(pathname, parent_name), "/"), name);

		if(type == SQUASHFS_DIR_TYPE)
			dir_scan(pathname, start_block, offset, new);
		else if(new == NULL) {
			if((i = s_ops.read_inode(start_block, offset)) == NULL) {
				ERROR("failed to read header\n");
				continue;
			}

			if(lsonly || info)
				print_filename(pathname, i);

			if(!lsonly) {
				create_inode(pathname, i);
				update_progress_bar();
				}
		}

		free_subdir(new);
	}

	if(!lsonly)
		set_attributes(parent_name, dir->mode, dir->uid, dir->guid, dir->mtime, force);

	squashfs_closedir(dir);
	dir_count ++;

	return TRUE;
}


void squashfs_stat(char *source)
{
	time_t mkfs_time = (time_t) sBlk.mkfs_time;
	char *mkfs_str = ctime(&mkfs_time);

#if __BYTE_ORDER == __BIG_ENDIAN
	printf("Found a valid %s endian SQUASHFS %d:%d superblock on %s.\n", swap ? "little" : "big", sBlk.s_major, sBlk.s_minor, source);
#else
	printf("Found a valid %s endian SQUASHFS %d:%d superblock on %s.\n", swap ? "big" : "little", sBlk.s_major, sBlk.s_minor, source);
#endif
	printf("Creation or last append time %s", mkfs_str ? mkfs_str : "failed to get time\n");
	printf("Filesystem is %sexportable via NFS\n", SQUASHFS_EXPORTABLE(sBlk.flags) ? "" : "not ");

	printf("Inodes are %scompressed\n", SQUASHFS_UNCOMPRESSED_INODES(sBlk.flags) ? "un" : "");
	printf("Data is %scompressed\n", SQUASHFS_UNCOMPRESSED_DATA(sBlk.flags) ? "un" : "");
	if(sBlk.s_major > 1 && !SQUASHFS_NO_FRAGMENTS(sBlk.flags))
		printf("Fragments are %scompressed\n", SQUASHFS_UNCOMPRESSED_FRAGMENTS(sBlk.flags) ? "un" : "");
	printf("Check data is %spresent in the filesystem\n", SQUASHFS_CHECK_DATA(sBlk.flags) ? "" : "not ");
	if(sBlk.s_major > 1) {
		printf("Fragments are %spresent in the filesystem\n", SQUASHFS_NO_FRAGMENTS(sBlk.flags) ? "not " : "");
		printf("Always_use_fragments option is %sspecified\n", SQUASHFS_ALWAYS_FRAGMENTS(sBlk.flags) ? "" : "not ");
	} else
		printf("Fragments are not supported by the filesystem\n");

	if(sBlk.s_major > 1)
		printf("Duplicates are %sremoved\n", SQUASHFS_DUPLICATES(sBlk.flags) ? "" : "not ");
	else
		printf("Duplicates are removed\n");
	printf("Filesystem size %.2f Kbytes (%.2f Mbytes)\n", sBlk.bytes_used / 1024.0, sBlk.bytes_used / (1024.0 * 1024.0));
	printf("Block size %d\n", sBlk.block_size);
	if(sBlk.s_major > 1)
		printf("Number of fragments %d\n", sBlk.fragments);
	printf("Number of inodes %d\n", sBlk.inodes);
	printf("Number of uids %d\n", sBlk.no_uids);
	printf("Number of gids %d\n", sBlk.no_guids);

	TRACE("sBlk.inode_table_start 0x%llx\n", sBlk.inode_table_start);
	TRACE("sBlk.directory_table_start 0x%llx\n", sBlk.directory_table_start);
	TRACE("sBlk.uid_start 0x%llx\n", sBlk.uid_start);
	if(sBlk.s_major > 1)
		TRACE("sBlk.fragment_table_start 0x%llx\n\n", sBlk.fragment_table_start);
}


int read_super(char *source)
{
    /*Start:Changed @2010-02-25 to suporrt lzma*/
	squashfs_super_block sblk;

	read_bytes(SQUASHFS_START, sizeof(squashfs_super_block), (char *) &sBlk);

	/* Check it is a SQUASHFS superblock */
	swap = 0;
	switch (sBlk.s_magic) {
	case SQUASHFS_MAGIC:
	case SQUASHFS_MAGIC_LZMA:
		break;
	case SQUASHFS_MAGIC_SWAP:
	case SQUASHFS_MAGIC_LZMA_SWAP:
		ERROR("Reading a different endian SQUASHFS filesystem on %s\n", source);
		SQUASHFS_SWAP_SUPER_BLOCK(&sblk, &sBlk);
		memcpy(&sBlk, &sblk, sizeof(squashfs_super_block));
		swap = 1;
	default:
		ERROR("Can't find a SQUASHFS superblock on %s\n", source);
		goto failed_mount;
	}
    /*End:Changed @2010-02-25 to suporrt lzma*/


	/* Check the MAJOR & MINOR versions */
	if(sBlk.s_major == 1 || sBlk.s_major == 2) {
		sBlk.bytes_used = sBlk.bytes_used_2;
		sBlk.uid_start = sBlk.uid_start_2;
		sBlk.guid_start = sBlk.guid_start_2;
		sBlk.inode_table_start = sBlk.inode_table_start_2;
		sBlk.directory_table_start = sBlk.directory_table_start_2;
		
		if(sBlk.s_major == 1) {
			sBlk.block_size = sBlk.block_size_1;
			sBlk.fragment_table_start = sBlk.uid_start;
			s_ops.squashfs_opendir = squashfs_opendir_2;
			s_ops.read_fragment_table = read_fragment_table_1;
			s_ops.read_block_list = read_block_list_1;
			s_ops.read_inode = read_inode_1;
		} else {
			sBlk.fragment_table_start = sBlk.fragment_table_start_2;
			s_ops.squashfs_opendir = squashfs_opendir_2;
			s_ops.read_fragment = read_fragment_2;
			s_ops.read_fragment_table = read_fragment_table_2;
			s_ops.read_block_list = read_block_list;
			s_ops.read_inode = read_inode_2;
		}
	} else if(sBlk.s_major == 3 && sBlk.s_minor <= 1) {
		s_ops.squashfs_opendir = squashfs_opendir;
		s_ops.read_fragment = read_fragment;
		s_ops.read_fragment_table = read_fragment_table;
		s_ops.read_block_list = read_block_list;
		s_ops.read_inode = read_inode;
	} else {
		ERROR("Filesystem on %s is (%d:%d), ", source, sBlk.s_major, sBlk.s_minor);
		ERROR("which is a later filesystem version than I support!\n");
		goto failed_mount;
	}

	return TRUE;

failed_mount:
	return FALSE;
}


struct pathname *process_extract_files(struct pathname *path, char *filename)
{
	FILE *fd;
	char name[16384];

	if((fd = fopen(filename, "r")) == NULL)
		EXIT_UNSQUASH("Could not open %s, because %s\n", filename, strerror(errno));

	while(fscanf(fd, "%16384[^\n]\n", name) != EOF)
		path = add_path(path, name, name);

	fclose(fd);
	return path;
}
		

/* reader thread.  This thread processes read requests queued by the
 * cache_get() routine. */
void *reader(void *arg)
{
	while(1) {
		struct cache_entry *entry = queue_get(to_reader);
		int res = read_bytes(entry->block,
			SQUASHFS_COMPRESSED_SIZE_BLOCK(entry->size),
			entry->data);

		if(res && SQUASHFS_COMPRESSED_BLOCK(entry->size))
			/* queue successfully read block to the deflate thread(s)
 			 * for further processing */
			queue_put(to_deflate, entry);
		else
			/* block has either been successfully read and is uncompressed,
 			 * or an error has occurred, clear pending flag, set
 			 * error appropriately, and wake up any threads waiting on
 			 * this buffer */
			cache_block_ready(entry, !res);
	}
}


/* writer thread.  This processes file write requests queued by the
 * write_file() routine. */
void *writer(void *arg)
{
	int i;

	while(1) {
		struct squashfs_file *file = queue_get(to_writer);
		int file_fd;
		int hole = 0;
		int failed = FALSE;

		if(file == NULL) {
			queue_put(from_writer, NULL);
			continue;
		}

		TRACE("writer: regular file, blocks %d\n", file->blocks);

		file_fd = file->fd;

		for(i = 0; i < file->blocks; i++, cur_blocks ++) {
			struct file_entry *block = queue_get(to_writer);

			if(block->buffer == 0) { /* sparse file */
				hole += block->size;
				free(block);
				continue;
			}

			cache_block_wait(block->buffer);

			if(block->buffer->error)
				failed = TRUE;

			if(failed == FALSE && write_block(file_fd, block->buffer->data + block->offset, block->size, hole) == FALSE) {
				ERROR("writer: failed to write data block %d\n", i);
				failed = TRUE;
			}
			hole = 0;
			cache_block_put(block->buffer);
			free(block);
		}

		if(hole && failed == FALSE) {
			/* corner case for hole extending to end of file */
			if(lseek(file_fd, hole, SEEK_CUR) == -1) {
				/* for broken lseeks which cannot seek beyond end of
 			 	* file, write_block will do the right thing */
				hole --;
				if(write_block(file_fd, "\0", 1, hole) == FALSE) {
					ERROR("writer: failed to write sparse data block\n");
					failed = TRUE;
				}
			} else if(ftruncate(file_fd, file->file_size) == -1) {
				ERROR("writer: failed to write sparse data block\n");
				failed = TRUE;
			}
		}

		close(file_fd);
		if(failed == FALSE)
			set_attributes(file->pathname, file->mode, file->uid, file->gid, file->time, force);
		else {
			ERROR("Failed to write %s, skipping\n", file->pathname);
			unlink(file->pathname);
		}
		free(file->pathname);
		free(file);

	}
}


/* decompress thread.  This decompresses buffers queued by the read thread */
void *deflator(void *arg)
{
	char tmp[block_size];

	while(1) {
		struct cache_entry *entry = queue_get(to_deflate);
		int res;
		unsigned long bytes = block_size;

		if(!is_lzma(((unsigned char *)entry->data)[0])){

			res = uncompress((unsigned char *) tmp, &bytes, (const unsigned char *) entry->data, SQUASHFS_COMPRESSED_SIZE_BLOCK(entry->size));

    		if(res != Z_OK) {
    			if(res == Z_MEM_ERROR)
    				ERROR("zlib::uncompress failed, not enough memory\n");
    			else if(res == Z_BUF_ERROR)
    				ERROR("zlib::uncompress failed, not enough room in output buffer\n");
    			else
    				ERROR("zlib::uncompress failed, unknown error %d\n", res);
    		} else
    			memcpy(entry->data, tmp, bytes);
        }else{        	    
                /*Start:Added @2010-02-25 to suporrt lzma*/
			if((res = LzmaUncompress((unsigned char *) tmp, &bytes, (const unsigned char *) entry->data, SQUASHFS_COMPRESSED_SIZE_BLOCK(entry->size))) != SZ_OK)
				ERROR("LzmaUncompress: error (%d)\n", res);
			else
				memcpy(entry->data, tmp, bytes);
        	    /*End:Added @2010-02-25 to suporrt lzma*/

		}
		/* block has been either successfully decompressed, or an error
 		 * occurred, clear pending flag, set error appropriately and
 		 * wake up any threads waiting on this block */ 
		cache_block_ready(entry, res != Z_OK);
	}
}


void *progress_thread(void *arg)
{
	struct timeval timeval;
	struct timespec timespec;
	struct itimerval itimerval;
	struct winsize winsize;

	if(ioctl(1, TIOCGWINSZ, &winsize) == -1) {
		ERROR("TIOCGWINZ ioctl failed, defaulting to 80 columns\n");
		columns = 80;
	} else
		columns = winsize.ws_col;
	signal(SIGWINCH, sigwinch_handler);
	signal(SIGALRM, sigalrm_handler);

	itimerval.it_value.tv_sec = 0;
	itimerval.it_value.tv_usec = 250000;
	itimerval.it_interval.tv_sec = 0;
	itimerval.it_interval.tv_usec = 250000;
	setitimer(ITIMER_REAL, &itimerval, NULL);

	pthread_cond_init(&progress_wait, NULL);

	pthread_mutex_lock(&screen_mutex);
	while(1) {
		gettimeofday(&timeval, NULL);
		timespec.tv_sec = timeval.tv_sec;
		if(timeval.tv_usec + 250000 > 999999)
			timespec.tv_sec++;
		timespec.tv_nsec = ((timeval.tv_usec + 250000) % 1000000) * 1000;
		pthread_cond_timedwait(&progress_wait, &screen_mutex, &timespec);
		if(progress_enabled)
			progress_bar(sym_count + dev_count +
				fifo_count + cur_blocks, total_inodes - total_files +
				total_blocks, columns);
	}
}


void initialise_threads(int fragment_buffer_size, int data_buffer_size)
{
	int i;
	sigset_t sigmask, old_mask;
	int all_buffers_size = fragment_buffer_size + data_buffer_size;

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGQUIT);
	if(sigprocmask(SIG_BLOCK, &sigmask, &old_mask) == -1)
		EXIT_UNSQUASH("Failed to set signal mask in intialise_threads\n");

	if(processors == -1) {
#ifndef linux
		int mib[2];
		size_t len = sizeof(processors);

		mib[0] = CTL_HW;
#ifdef HW_AVAILCPU
		mib[1] = HW_AVAILCPU;
#else
		mib[1] = HW_NCPU;
#endif

		if(sysctl(mib, 2, &processors, &len, NULL, 0) == -1) {
			ERROR("Failed to get number of available processors.  Defaulting to 1\n");
			processors = 1;
		}
#else
		processors = get_nprocs();
#endif
	}

	if((thread = malloc((3 + processors) * sizeof(pthread_t))) == NULL)
		EXIT_UNSQUASH("Out of memory allocating thread descriptors\n");
	deflator_thread = &thread[3];

	to_reader = queue_init(all_buffers_size);
	to_deflate = queue_init(all_buffers_size);
	to_writer = queue_init(1000);
	from_writer = queue_init(1);
	fragment_cache = cache_init(block_size, fragment_buffer_size);
	data_cache = cache_init(block_size, data_buffer_size);
	pthread_create(&thread[0], NULL, reader, NULL);
	pthread_create(&thread[1], NULL, writer, NULL);
	pthread_create(&thread[2], NULL, progress_thread, NULL);
	pthread_mutex_init(&fragment_mutex, NULL);

	for(i = 0; i < processors; i++) {
		if(pthread_create(&deflator_thread[i], NULL, deflator, NULL) != 0 )
			EXIT_UNSQUASH("Failed to create thread\n");
	}

	printf("Parallel unsquashfs: Using %d processor%s\n", processors,
			processors == 1 ? "" : "s");

	if(sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1)
		EXIT_UNSQUASH("Failed to set signal mask in intialise_threads\n");
}


void enable_progress_bar()
{
	pthread_mutex_lock(&screen_mutex);
	progress_enabled = TRUE;
	pthread_mutex_unlock(&screen_mutex);
}


void disable_progress_bar()
{
	pthread_mutex_lock(&screen_mutex);
	progress_enabled = FALSE;
	pthread_mutex_unlock(&screen_mutex);
}


void update_progress_bar()
{
	pthread_mutex_lock(&screen_mutex);
	pthread_cond_signal(&progress_wait);
	pthread_mutex_unlock(&screen_mutex);
}


void progress_bar(long long current, long long max, int columns)
{
	char rotate_list[] = { '|', '/', '-', '\\' };
	int max_digits = floor(log10(max)) + 1;
	int used = max_digits * 2 + 11;
	int hashes = (current * (columns - used)) / max;
	int spaces = columns - used - hashes;

	if(current > max) {
		printf("%lld %lld\n", current, max);
		return;
	}

	if(columns - used < 0)
		return;

	printf("\r[");

	while (hashes --)
		putchar('=');

	putchar(rotate_list[rotate]);

	while(spaces --)
		putchar(' ');

	printf("] %*lld/%*lld", max_digits, current, max_digits, max);
	printf(" %3lld%%", current * 100 / max);
	fflush(stdout);
}


#define VERSION() \
	printf("unsquashfs version 3.4 (2008/08/26)\n");\
	printf("copyright (C) 2008 Phillip Lougher <phillip@lougher.demon.co.uk>\n\n"); \
    	printf("This program is free software; you can redistribute it and/or\n");\
	printf("modify it under the terms of the GNU General Public License\n");\
	printf("as published by the Free Software Foundation; either version 2,\n");\
	printf("or (at your option) any later version.\n\n");\
	printf("This program is distributed in the hope that it will be useful,\n");\
	printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");\
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");\
	printf("GNU General Public License for more details.\n");
int main(int argc, char *argv[])
{
	char *dest = "squashfs-root";
	int i, stat_sys = FALSE, version = FALSE;
	int n;
	struct pathnames *paths = NULL;
	struct pathname *path = NULL;
	int fragment_buffer_size = FRAGMENT_BUFFER_DEFAULT;
	int data_buffer_size = DATA_BUFFER_DEFAULT;
	char *b;
	struct winsize winsize;

	pthread_mutex_init(&screen_mutex, NULL);
	root_process = geteuid() == 0;
	if(root_process)
		umask(0);
	
	for(i = 1; i < argc; i++) {
		if(*argv[i] != '-')
			break;
		if(strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
			VERSION();
			version = TRUE;
		} else if(strcmp(argv[i], "-info") == 0 || strcmp(argv[i], "-i") == 0)
			info = TRUE;
		else if(strcmp(argv[i], "-ls") == 0 || strcmp(argv[i], "-l") == 0)
			lsonly = TRUE;
		else if(strcmp(argv[i], "-no-progress") == 0 || strcmp(argv[i], "-n") == 0)
			progress = FALSE;
		else if(strcmp(argv[i], "-dest") == 0 || strcmp(argv[i], "-d") == 0) {
			if(++i == argc) {
				fprintf(stderr, "%s: -dest missing filename\n", argv[0]);
				exit(1);
			}
			dest = argv[i];
		} else if(strcmp(argv[i], "-processors") == 0 || strcmp(argv[i], "-p") == 0) {
			if((++i == argc) || (processors = strtol(argv[i], &b, 10), *b != '\0')) {
				ERROR("%s: -processors missing or invalid processor number\n", argv[0]);
				exit(1);
			}
			if(processors < 1) {
				ERROR("%s: -processors should be 1 or larger\n", argv[0]);
				exit(1);
			}
		} else if(strcmp(argv[i], "-data-queue") == 0 || strcmp(argv[i], "-da") == 0) {
			if((++i == argc) || (data_buffer_size = strtol(argv[i], &b, 10), *b != '\0')) {
				ERROR("%s: -data-queue missing or invalid queue size\n", argv[0]);
				exit(1);
			}
			if(data_buffer_size < 1) {
				ERROR("%s: -data-queue should be 1 Mbyte or larger\n", argv[0]);
				exit(1);
			}
		} else if(strcmp(argv[i], "-frag-queue") == 0 || strcmp(argv[i], "-fr") == 0) {
			if((++i == argc) || (fragment_buffer_size = strtol(argv[i], &b, 10), *b != '\0')) {
				ERROR("%s: -frag-queue missing or invalid queue size\n", argv[0]);
				exit(1);
			}
			if(fragment_buffer_size < 1) {
				ERROR("%s: -frag-queue should be 1 Mbyte or larger\n", argv[0]);
				exit(1);
			}
		} else if(strcmp(argv[i], "-force") == 0 || strcmp(argv[i], "-f") == 0)
			force = TRUE;
		else if(strcmp(argv[i], "-stat") == 0 || strcmp(argv[i], "-s") == 0)
			stat_sys = TRUE;
		else if(strcmp(argv[i], "-lls") == 0 || strcmp(argv[i], "-ll") == 0) {
			lsonly = TRUE;
			short_ls = FALSE;
		} else if(strcmp(argv[i], "-linfo") == 0 || strcmp(argv[i], "-li") == 0) {
			info = TRUE;
			short_ls = FALSE;
		} else if(strcmp(argv[i], "-ef") == 0 || strcmp(argv[i], "-e") == 0) {
			if(++i == argc) {
				fprintf(stderr, "%s: -ef missing filename\n", argv[0]);
				exit(1);
			}
			path = process_extract_files(path, argv[i]);
		} else if(strcmp(argv[i], "-regex") == 0 || strcmp(argv[i], "-r") == 0)
			use_regex = TRUE;
		else
			goto options;
	}

	if(lsonly || info)
		progress = FALSE;

	if(i == argc) {
		if(!version) {
options:
			ERROR("SYNTAX: %s [options] filesystem [directories or files to extract]\n", argv[0]);
			ERROR("\t-v[ersion]\t\tprint version, licence and copyright information\n");
			ERROR("\t-d[est] <pathname>\tunsquash to <pathname>, default \"squashfs-root\"\n");
			ERROR("\t-n[o-progress]\t\tdon't display the progress bar\n");
			ERROR("\t-p[rocessors] <number>\tuse <number> processors.  By default will use\n\t\t\t\tnumber of processors available\n");
			ERROR("\t-i[nfo]\t\t\tprint files as they are unsquashed\n");
			ERROR("\t-li[nfo]\t\tprint files as they are unsquashed with file\n\t\t\t\tattributes (like ls -l output)\n");
			ERROR("\t-l[s]\t\t\tlist filesystem, but don't unsquash\n");
			ERROR("\t-ll[s]\t\t\tlist filesystem with file attributes (like\n\t\t\t\tls -l output), but don't unsquash\n");
			ERROR("\t-f[orce]\t\tif file already exists then overwrite\n");
			ERROR("\t-s[tat]\t\t\tdisplay filesystem superblock information\n");
			ERROR("\t-e[f] <extract file>\tlist of directories or files to extract.\n\t\t\t\tOne per line\n");
			ERROR("\t-da[ta-queue] <size>\tSet data queue to <size> Mbytes.  Default %d\n\t\t\t\tMbytes\n", DATA_BUFFER_DEFAULT);
			ERROR("\t-fr[ag-queue] <size>\tSet fagment queue to <size> Mbytes.  Default %d\n\t\t\t\tMbytes\n", FRAGMENT_BUFFER_DEFAULT);
			ERROR("\t-r[egex]\t\ttreat extract names as POSIX regular expressions\n\t\t\t\trather than use the default shell wildcard\n\t\t\t\texpansion (globbing)\n");
		}
		exit(1);
	}

	for(n = i + 1; n < argc; n++)
		path = add_path(path, argv[n], argv[n]);

	if((fd = open(argv[i], O_RDONLY)) == -1) {
		ERROR("Could not open %s, because %s\n", argv[i], strerror(errno));
		exit(1);
	}

	if(read_super(argv[i]) == FALSE)
		exit(1);

	if(stat_sys) {
		squashfs_stat(argv[i]);
		exit(0);
	}

	block_size = sBlk.block_size;
	block_log = sBlk.block_log;

	fragment_buffer_size <<= 20 - block_log;
	data_buffer_size <<= 20 - block_log;
	initialise_threads(fragment_buffer_size, data_buffer_size);

	if((fragment_data = malloc(block_size)) == NULL)
		EXIT_UNSQUASH("failed to allocate fragment_data\n");

	if((file_data = malloc(block_size)) == NULL)
		EXIT_UNSQUASH("failed to allocate file_data");

	if((data = malloc(block_size)) == NULL)
		EXIT_UNSQUASH("failed to allocate data\n");

	if((created_inode = malloc(sBlk.inodes * sizeof(char *))) == NULL)
		EXIT_UNSQUASH("failed to allocate created_inode\n");

	memset(created_inode, 0, sBlk.inodes * sizeof(char *));

	read_uids_guids();

	s_ops.read_fragment_table();

	uncompress_inode_table(sBlk.inode_table_start, sBlk.directory_table_start);

	uncompress_directory_table(sBlk.directory_table_start, sBlk.fragment_table_start);

	if(path) {
		paths = init_subdir();
		paths = add_subdir(paths, path);
	}

	pre_scan(dest, SQUASHFS_INODE_BLK(sBlk.root_inode), SQUASHFS_INODE_OFFSET(sBlk.root_inode), paths);

	memset(created_inode, 0, sBlk.inodes * sizeof(char *));

	printf("%d inodes (%d blocks) to write\n\n", total_inodes, total_inodes - total_files + total_blocks);

	if(progress)
		enable_progress_bar();

	dir_scan(dest, SQUASHFS_INODE_BLK(sBlk.root_inode), SQUASHFS_INODE_OFFSET(sBlk.root_inode), paths);

	queue_put(to_writer, NULL);
	queue_get(from_writer);

	if(progress) {
		disable_progress_bar();
		progress_bar(sym_count + dev_count + fifo_count + cur_blocks,
			total_inodes - total_files + total_blocks, columns);
	}

	if(!lsonly) {
		printf("\n");
		printf("created %d files\n", file_count);
		printf("created %d directories\n", dir_count);
		printf("created %d symlinks\n", sym_count);
		printf("created %d devices\n", dev_count);
		printf("created %d fifos\n", fifo_count);
	}

	return 0;
}
