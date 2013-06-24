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

#ifndef __YAFFS2UTILS_LIST_H__
#define __YAFFS2UTILS_LIST_H__

#include <stddef.h>

typedef struct list_head {
	struct list_head *prev;
	struct list_head *next;
} list_head_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void
INIT_LIST_HEAD (list_head_t *entry)
{
	entry->prev = entry->next = entry;
}

static inline void
__list_add (list_head_t *new,
	    list_head_t *prev,
	    list_head_t *next)
{
	new->prev = prev;
	new->next = next;
	next->prev = new;
	prev->next = new;
}

static inline void
list_add (list_head_t *new, list_head_t *head)
{
	__list_add(new, head, head->next);
}

static inline void
list_add_tail (list_head_t *new, list_head_t *head)
{
	__list_add(new, head->prev, head);
}

static inline void
__list_del (list_head_t *prev, list_head_t *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void
list_del (list_head_t *entry)
{
	__list_del(entry->prev, entry->next);
	
}

static inline void
list_del_init (list_head_t *entry)
{
	list_del(entry);
	INIT_LIST_HEAD(entry);
}

static inline int
list_empty(const struct list_head *head)
{
	return head->next == head;
}

#define container_of(ptr, type, member) \
	({const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != head; pos = pos->next)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != head; \
		pos = n, n = pos->next)

#endif

