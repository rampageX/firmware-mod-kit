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

#include <errno.h>
#include <unistd.h>

#include "safe_rw.h"

/*-------------------------------------------------------------------------*/

ssize_t
safe_read (int fd, void *buf, size_t count)
{
	ssize_t r;
	size_t reads = 0;

	while (reads < count &&
	       (r = read(fd, (char *)buf + reads, count - reads)) != 0)
	{
		if (r < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;

			return -1;
		}
		reads += r;
	}

	return reads;
}

ssize_t
safe_write (int fd, const void *buf, size_t count)
{
	ssize_t w;
	size_t written = 0;

	while (written < count &&
	       (w = write(fd, (char *)buf + written, count - written)) != 0)
	{
		if (w < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;

			return -1;
		}
		written += w;
	}

	return written;
}
