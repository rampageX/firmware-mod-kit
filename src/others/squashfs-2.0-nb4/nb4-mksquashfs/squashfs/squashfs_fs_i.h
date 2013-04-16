#ifndef SQUASHFS_FS_I
#define SQUASHFS_FS_I
/*
 * Squashfs
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
 * squashfs_fs_i.h
 */

typedef struct squashfs_inode_info {
	unsigned int	start_block;
	unsigned int	block_list_start;
	unsigned int	offset;
	unsigned int	fragment_start_block;
	unsigned int	fragment_size;
	unsigned int	fragment_offset;
	struct inode	vfs_inode;
	} squashfs_inode_info;
#endif
