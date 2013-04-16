/****************************************************************************
 *
 * rg/pkg/lzma/SRC/7zip/Compress/LZMA_C/decode.c
 * 
 * Copyright (C) Jungo LTD 2004
 * 
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General 
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02111-1307, USA.
 *
 * Developed by Jungo LTD.
 * Residential Gateway Software Division
 * www.jungo.com
 * info@jungo.com
 */

/* Note: This file is included by the kernel and receives its __OS_*__
 * definition from the kernel CFLAGS using -D. This is why we don't
 * include rg_os.h.
 */

#if defined(__KERNEL__) && !defined(__OS_VXWORKS__)
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#else
#ifdef __OS_VXWORKS__
#include <kos/kos.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#include "LzmaDecode.h"

static u32 lzma_internal_size = 0;
static void *lzma_internal_data = NULL;

#ifdef __LZMA_UNCOMPRESS_KERNEL__
#define LZMA_ERR(args...) puts("LZMA Decode error\n");
#define LZMA_ALLOC(size) malloc(size)
#define LZMA_FREE free
#elif defined(__KERNEL__) && !defined(__OS_VXWORKS__)
#define LZMA_ERR(args...) printk("LZMA decode error: " args)
#define LZMA_ALLOC(size) kmalloc(size, GFP_KERNEL)
#define LZMA_FREE kfree
#else
#define LZMA_ERR(args...) fprintf(stderr, "LZMA decode error: " args)
#define LZMA_ALLOC(size) malloc(size)
#define LZMA_FREE free
#endif

int lzma_decode(void *dst, int dstlen, void *src, int srclen)
{
    u8 properties[5], prop0;
    u32 out_len = 0;
    int i, lc, lp, pb;
    int calc_internal_size, res;

    /* Compressed buffer's structure is:
     * 5 bytes properties
     * 4 bytes uncompressed length
     * rest is compressed data. */

    /* Read properties */
    memcpy(properties, src, 5);
    src += 5;

    /* Read length */
    for (i = 0; i < 4; i++)
	out_len += (u32)(((u8 *)src)[i]) << (i * 8);

    if (out_len > dstlen)
    {
	LZMA_ERR("Out buffer too small - have %d and need %d\n", dstlen,
	    out_len);
	return -1;
    }
    
    src += 4;
    srclen -= 9;
    
    prop0 = properties[0];
    if (prop0 >= 9 * 5 * 5)
    {
	LZMA_ERR("Properties Error\n");
	return -1;
    }
    for (pb = 0; prop0 >= 9 * 5; pb++, prop0 -= 9 * 5);
    for (lp = 0; prop0 >= 9; lp++, prop0 -= 9);
    lc = prop0;
   
    calc_internal_size = (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp))) *
	sizeof(CProb);
    if (calc_internal_size > lzma_internal_size)
    {
	if (lzma_internal_data)
	    LZMA_FREE(lzma_internal_data);
	if (!(lzma_internal_data = LZMA_ALLOC(calc_internal_size)))
	{
	    lzma_internal_size = 0;
	    LZMA_ERR("Error allocating internal data\n");
	    return -1;
	}
	lzma_internal_size = calc_internal_size;
    }

    res = LzmaDecode((u8 *)lzma_internal_data, lzma_internal_size, lc, lp, pb,
	(u8 *)src, srclen, (u8 *)dst, out_len, &dstlen);

    if (res)
    {
	LZMA_ERR("Decoder internal error (%d)\n", res);
	return -1;
    }

    return dstlen;
}

void lzma_decode_uninit(void)
{
   if (lzma_internal_data)
       LZMA_FREE(lzma_internal_data);
   lzma_internal_data = NULL;
   lzma_internal_size = 0;
}
