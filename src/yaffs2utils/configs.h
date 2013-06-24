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

#ifndef __YAFFS2UTILS_CONFIGS_H__
#define __YAFFS2UTILS_CONFIGS_H__

#define DEFAULT_CHUNKSIZE	2048

#if defined(__linux__) || defined (__FreeBSD__) || defined(__NetBSD__) || \
    (defined(__APPLE__) && defined(__MACH__))
 #define _HAVE_LUTIMES		1
#endif

#if defined(__APPLE__) && defined(__MACH__)
 #define _HAVE_OSX_SYSLIMITS	1
 #define _HAVE_BROKEN_LOFF_T	1
#endif

#if !defined(__linux__)
 #define _HAVE_BROKEN_MTD_H	1
#endif

#endif
