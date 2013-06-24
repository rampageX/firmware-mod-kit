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

#ifndef __YAFFS_UTILS_H__
#define __YAFFS_UTILS_H__

#include <sys/types.h>

#include "configs.h"

/* Definition of types */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;

#define YCHAR	char

#define yaffs_trace(...) do {} while (0)

#ifdef _HAVE_BROKEN_LOFF_T
 typedef long long	loff_t;
#endif

#endif
