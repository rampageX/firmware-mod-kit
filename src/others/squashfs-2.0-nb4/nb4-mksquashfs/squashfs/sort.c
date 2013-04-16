/*
 * Create a squashfs filesystem.  This is a highly compressed read only filesystem.
 *
 * Copyright (c) 2002, 2003, 2004 Phillip Lougher <plougher@users.sourceforge.net>
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
 * sort.c
 */

#define TRUE 1
#define FALSE 0

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <squashfs_fs.h>

#ifdef SQUASHFS_TRACE
#define TRACE(s, args...)		printf("mksquashfs: "s, ## args)
#else
#define TRACE(s, args...)
#endif

#define INFO(s, args...)		if(!silent) printf("mksquashfs: "s, ## args)
#define ERROR(s, args...)		fprintf(stderr, s, ## args)
#define EXIT_MKSQUASHFS()		exit(1)
#define BAD_ERROR(s, args...)		{\
					fprintf(stderr, "FATAL ERROR:" s, ##args);\
					EXIT_MKSQUASHFS();\
					}

struct sort_info {
	dev_t			st_dev;
	ino_t			st_ino;
	int			priority;
	struct sort_info	*next;
};

struct sort_info *sort_info_list[65536];

struct priority_entry {
	char *filename;
	long long size;
	ino_t st_ino;
	dev_t st_dev;
	struct priority_entry *next;
};

struct priority_entry *priority_list[65536];

struct sorted_inode_entry {
	squashfs_inode inode;
	ino_t	st_ino;
	dev_t	st_dev;
	struct sorted_inode_entry *next;
};

struct sorted_inode_entry *sorted_inode_list[65536];

extern int silent;
extern int excluded(char *filename, struct stat *buf);
extern squashfs_inode write_file(char *filename, long long size, int *c_size);


int add_to_sorted_inode_list(squashfs_inode inode, dev_t st_dev, ino_t st_ino)
{
	int hash = st_ino & 0xffff;
	struct sorted_inode_entry *new_sorted_inode_entry;

	if((new_sorted_inode_entry = malloc(sizeof(struct sorted_inode_entry))) == NULL) {
		ERROR("Out of memory allocating sorted inode entry\n");
		return FALSE;
	}

	new_sorted_inode_entry->inode = inode;
	new_sorted_inode_entry->st_ino = st_ino;
	new_sorted_inode_entry->st_dev = st_dev;
	new_sorted_inode_entry->next = sorted_inode_list[hash];
	sorted_inode_list[hash] = new_sorted_inode_entry;

	return TRUE;
}

	
squashfs_inode get_sorted_inode(struct stat *buf)
{
	int hash = buf->st_ino & 0xffff;
	struct sorted_inode_entry *sorted_inode_entry;

	for(sorted_inode_entry = sorted_inode_list[hash]; sorted_inode_entry; sorted_inode_entry = sorted_inode_entry->next)
		if(buf->st_ino == sorted_inode_entry->st_ino && buf->st_dev == sorted_inode_entry->st_dev)
			break;

	if(sorted_inode_entry)
		return sorted_inode_entry->inode;
	else
		return (squashfs_inode) 0;
}


int add_priority_list(char *filename, struct stat *buf, int priority)
{
	struct priority_entry *new_priority_entry;

	priority += 32768;
	if((new_priority_entry = malloc(sizeof(struct priority_entry))) == NULL) {
		ERROR("Out of memory allocating priority entry\n");
		return FALSE;
	}

	new_priority_entry->filename = strdup(filename);
	new_priority_entry->size = buf->st_size;
	new_priority_entry->st_dev = buf->st_dev;
	new_priority_entry->st_ino = buf->st_ino;
	new_priority_entry->next = priority_list[priority];
	priority_list[priority] = new_priority_entry;
	return TRUE;
}


int get_priority(char *filename, struct stat *buf, int priority)
{
	int hash = buf->st_ino & 0xffff;
	struct sort_info *s;

	for(s = sort_info_list[hash]; s; s = s->next)
		if((s->st_dev == buf->st_dev) && (s->st_ino == buf->st_ino)) {
			TRACE("returning priority %d (%s)\n", s->priority, filename);
			return s->priority;
		}
	TRACE("returning priority %d (%s)\n", priority, filename);
	return priority;
}


#define ADD_ENTRY(buf, priority) {\
	int hash = buf.st_ino & 0xffff;\
	struct sort_info *s;\
	if((s = malloc(sizeof(struct sort_info))) == NULL) {\
		ERROR("Out of memory allocating sort list entry\n");\
		return FALSE;\
	}\
	s->st_dev = buf.st_dev;\
	s->st_ino = buf.st_ino;\
	s->priority = priority;\
	s->next = sort_info_list[hash];\
	sort_info_list[hash] = s;\
	}
int add_sort_list(char *path, int priority, int source, char *source_path[])
{
	int i;
	char buffer[4096], filename[4096];
	struct stat buf;

	TRACE("add_sort_list: filename %s, priority %d\n", path, priority);
	if(strlen(path) > 1 && strcmp(path + strlen(path) - 2, "/*") == 0)
		path[strlen(path) - 2] = '\0';

	TRACE("add_sort_list: filename %s, priority %d\n", path, priority);
	if(path[0] == '/' || strncmp(path, "./", 2) == 0 || strncmp(path, "../", 3) == 0) {
		if(lstat(path, &buf) == -1) {
			sprintf(buffer, "Cannot stat sort_list dir/file %s, ignoring", path);
			perror(buffer);
			return TRUE;
		}
		TRACE("adding filename %s, priority %d, st_dev %Lx, st_ino %Lx\n", path, priority, buf.st_dev, buf.st_ino);
		ADD_ENTRY(buf, priority);
		return TRUE;
	}

	for(i = 0; i < source; i++) {
		strcat(strcat(strcpy(filename, source_path[i]), "/"), path);
		if(lstat(filename, &buf) == -1) {
			if(!(errno == ENOENT || errno == ENOTDIR)) {
				sprintf(buffer, "Cannot stat sort_list dir/file %s, ignoring", filename);
				perror(buffer);
			}
			continue;
		}
		ADD_ENTRY(buf, priority);
	}
	return TRUE;
}


void generate_file_priorities(char *pathname, int priority, struct stat *buf)
{
	char filename[8192];
	DIR *linuxdir;
	struct dirent *d_name;
	
	priority = get_priority(pathname, buf, priority);

	if((linuxdir = opendir(pathname)) == NULL) {
		ERROR("Could not open %s, skipping...\n", pathname);
		return;
	}
	
	while((d_name = readdir(linuxdir)) != NULL) {
		if(strcmp(d_name->d_name, ".") == 0 || strcmp(d_name->d_name, "..") == 0)
			continue;
		strcat(strcat(strcpy(filename, pathname), "/"), d_name->d_name);

		if(lstat(filename, buf) == -1) {
			char buffer[8192];
			sprintf(buffer, "Cannot stat dir/file %s, ignoring", filename);
			perror(buffer);
			continue;
		}

		if(excluded(filename, buf))
			continue;

		switch(buf->st_mode & S_IFMT) {
			case S_IFREG:
				add_priority_list(filename, buf, get_priority(filename, buf, priority));
				break;
			case S_IFDIR:
				generate_file_priorities(filename, priority, buf);
				break;
		}
	}

	closedir(linuxdir);
}


int read_sort_file(char *filename, int source, char *source_path[])
{
	FILE *fd;
	char sort_filename[16385];
	int priority;

	if((fd = fopen(filename, "r")) == NULL) {
		perror("Could not open sort_list file...");
		return FALSE;
	}
	while(fscanf(fd, "%s %d", sort_filename, &priority) != EOF)
		if(priority >= -32768 && priority <= 32767)
			add_sort_list(sort_filename, priority, source, source_path);
		else
			ERROR("Sort file %s, priority %d outside range of -32767:32768 - skipping...\n", sort_filename, priority);
	fclose(fd);
	return TRUE;
}


void sort_files_and_write(int source, char *source_path[])
{
	struct stat buf;
	int i, c_size;
	struct priority_entry *entry;
	squashfs_inode inode;

	for(i = 0; i < source; i++) {
		if(lstat(source_path[i], &buf) == -1) {
			char buffer[8192];
			sprintf(buffer, "Cannot stat dir/file %s, ignoring", source_path[i]);
			perror(buffer);
			continue;
		}

		if(excluded(source_path[i], &buf))
			continue;

		switch(buf.st_mode & S_IFMT) {
			case S_IFREG:
				add_priority_list(source_path[i], &buf, get_priority(source_path[i], &buf, 0));
				break;
			case S_IFDIR:
				generate_file_priorities(source_path[i], 0, &buf);
				break;
		}
	}

	for(i = 0; i < 65536; i++)
		for(entry = priority_list[i]; entry; entry = entry->next) {
			TRACE("%d: %s\n", i - 32768, entry->filename);
			inode = write_file(entry->filename, entry->size, &c_size);
			INFO("file %s, size %d bytes, inode 0x%Lx\n", entry->filename, c_size, inode);
			INFO("\t%.2f%% of uncompressed file size (%Ld bytes)\n", ((float) c_size / entry->size) * 100.0, entry->size);
			add_to_sorted_inode_list(inode, entry->st_dev, entry->st_ino);
		}
}
