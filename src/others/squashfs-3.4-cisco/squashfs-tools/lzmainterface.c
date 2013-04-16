/* Modifications were made by Cisco Systems, Inc. on or before Thu Feb 16 11:28:44 PST 2012 */
/*
 * Squashfs -lzma for Linux
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "LzmaDec.h"
#include "LzmaEnc.h"
#include "lzmainterface.h"


static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

int LzmaCompress(char *in_data, int in_size, char *out_data, int out_size, unsigned long *total_out)
{
	CLzmaEncProps encProps;
	size_t headerSize = LZMA_PROPS_SIZE;
	int ret;
	SizeT outProcess;

	LzmaEncProps_Init(&encProps);
	encProps.algo = 1;
	outProcess = out_size - LZMA_PROPS_SIZE;
	ret = LzmaEncode(out_data+LZMA_PROPS_SIZE, &outProcess, in_data, in_size, &encProps, out_data, 
						&headerSize, 0, NULL, &g_Alloc, &g_Alloc);
	*total_out = outProcess + LZMA_PROPS_SIZE;
	return ret;
}

int LzmaUncompress(char *dst, unsigned long * dstlen, char *src, int srclen)
{
	int res;
	SizeT inSizePure;
	ELzmaStatus status;

	if (srclen < LZMA_PROPS_SIZE)
	{
		memcpy(dst, src, srclen);
		return srclen;
	}
	inSizePure = srclen - LZMA_PROPS_SIZE;
	res = LzmaDecode(dst, dstlen, src + LZMA_PROPS_SIZE, &inSizePure,
	                 src, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &g_Alloc);
	srclen = inSizePure ;

	if ((res == SZ_OK) ||
		((res == SZ_ERROR_INPUT_EOF) && (srclen == inSizePure)))
		res = 0;
	if (res != SZ_OK)
		printf("In LzmaUncompress: return error (%d)\n", res);
	return res;
}


