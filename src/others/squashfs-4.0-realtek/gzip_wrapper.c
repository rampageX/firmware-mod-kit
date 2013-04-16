/*
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
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
 * gzip_wrapper.c
 */

#include <zlib.h>
#include <stdlib.h>

int gzip_compress(void **strm, char *d, char *s, int size, int block_size,
		int *error)
{
	int res = 0;
	z_stream *stream = *strm;

	if(stream == NULL) {
		if((stream = *strm = malloc(sizeof(z_stream))) == NULL)
			goto failed;

		stream->zalloc = Z_NULL;
		stream->zfree = Z_NULL;
		stream->opaque = 0;

		if((res = deflateInit(stream, 9)) != Z_OK)
			goto failed;
	} else if((res = deflateReset(stream)) != Z_OK)
		goto failed;

	stream->next_in = (unsigned char *) s;
	stream->avail_in = size;
	stream->next_out = (unsigned char *) d;
	stream->avail_out = block_size;

	res = deflate(stream, Z_FINISH);
	if(res == Z_STREAM_END)
		/*
		 * Success, return the compressed size.
		 */
		return (int) stream->total_out;
	if(res == Z_OK)
		/*
		 * Output buffer overflow.  Return out of buffer space
		 */
		return 0;
failed:
	/*
	 * All other errors return failure, with the compressor
	 * specific error code in *error
	 */
	*error = res;
	return -1;
}


int gzip_uncompress(char *d, char *s, int size, int block_size, int *error)
{
	int res;
	unsigned long bytes = block_size;

	res = uncompress((unsigned char *) d, &bytes,
		(const unsigned char *) s, size);

	*error = res;
	return res == Z_OK ? (int) bytes : -1;
}
