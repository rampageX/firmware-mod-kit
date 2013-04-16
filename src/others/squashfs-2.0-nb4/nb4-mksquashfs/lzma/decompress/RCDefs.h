#ifndef __RCDEFS_H
#define __RCDEFS_H

#include "AriBitCoder.h"

/* BRCM modification start */
#ifdef _HOST_TOOL
#include "stdio.h"
#include "stdlib.h"
#include "malloc.h"
#endif

#ifdef _CFE_
#include "lib_malloc.h"
#include "lib_printf.h"
#define malloc(x) KMALLOC(x, 0)
#define free(x)   KFREE(x)
#endif

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#define printf printk

/*
 * BRCM Note:
 *
 * Typically the LitDecoder2 object m_Coders is very large e.g. 24576 bytes,
 * and kmalloc can fail if there is no contiguous space. All other dynamic
 * memory allocations in lzma's usage are very small. If vmalloc is used
 * we would use up (2*4K + 32 + 32) bytes per invocation. When memory is
 * in short supply, e.g. during an upload we typically require over 158Kbytes
 * when 28K would have sufficed if kmalloc was used for smaller sizes.
 *
 */
INLINE void * lzma_malloc(unsigned long sz)
{
    if ( sz > 1024 )
    {
        // printf("lzma_malloc -> vmalloc( %lu )\n", sz );
        return (void*) vmalloc(sz);
    }
    else
    {
        // printf("lzma_malloc -> kmalloc( %lu )\n", sz );
        return kmalloc(sz, GFP_KERNEL);
    }
}

INLINE void lzma_free(void * blk_p)
{
        /* if kseg0 kernel cached memory | kseg1 kernel uncached memory */
    if (  ((unsigned int)blk_p <  (unsigned int) 0xC0000000)
       && ((unsigned int)blk_p >= (unsigned int) 0x80000000))
    {
        // printf("lzma_free -> kfree( 0x%08x )\n", (int) blk_p);
        kfree(blk_p);
    }
    else        /* kseg2 vmalloc | kuseg malloc */
    {
        // printf("lzma_free -> vfree( 0x%08x )\n", (int) blk_p);
        vfree(blk_p);
    }
}

#define malloc(x) lzma_malloc(x)
#define free(x)   lzma_free(x)
#endif
/* BRCM modification end */


/*
#define RC_INIT_VAR                            \
  UINT32 aRange = aRangeDecoder->m_Range;      \
  UINT32 aCode = aRangeDecoder->m_Code;        

#define RC_FLUSH_VAR                          \
  aRangeDecoder->m_Range = aRange;            \
  aRangeDecoder->m_Code = aCode;
*/


#if 1
#define RC_GETBIT2(aNumMoveBits, aProb, aModelIndex, Action0, Action1)                        \
    {UINT32 aNewBound = (aRange >> kNumBitModelTotalBits) * aProb; \
    if (aCode < aNewBound)                               \
    {                                                             \
      Action0;                                                    \
      aRange = aNewBound;                                         \
      aProb += (kBitModelTotal - aProb) >> aNumMoveBits;          \
      aModelIndex <<= 1;                                          \
    }                                                             \
    else                                                          \
    {                                                             \
      Action1;                                                    \
      aRange -= aNewBound;                                        \
      aCode -= aNewBound;                                          \
      aProb -= (aProb) >> aNumMoveBits;                           \
      aModelIndex = (aModelIndex << 1) + 1;                       \
    }}                                                             \
    if (aRange < kTopValue)               \
    {                                                              \
      aCode = (aCode << 8) | InStreamReadByte(in_stream);   \
      aRange <<= 8; }

#define RC_GETBIT(aNumMoveBits, aProb, aModelIndex) RC_GETBIT2(aNumMoveBits, aProb, aModelIndex, ; , ;)               
#endif

#endif
