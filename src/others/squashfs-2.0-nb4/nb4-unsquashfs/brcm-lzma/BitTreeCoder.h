#ifndef __BITTREECODER_H
#define __BITTREECODER_H

#include "AriBitCoder.h"
#include "RCDefs.h"

//BRCM modification start
#ifdef _HOST_TOOL
#include "stdio.h"
#include "stdlib.h"
#include "malloc.h" 
#endif

#ifdef _CFE_
#include "lib_malloc.h"
#include "lib_printf.h"
#define malloc(x) KMALLOC(x, 0)
#endif

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#define printf printk
//#define malloc(x) kmalloc(x,GFP_KERNEL)
#define malloc(x) vmalloc(x)
#define free(x) vfree(x)
#endif
//BRCM modification end

//////////////////////////
// CBitTreeDecoder

typedef struct CBitTreeDecoder
{
  UINT32       m_NumBitLevels;
  CBitDecoder *m_Models;
} CBitTreeDecoder;

//  ~CBitTreeDecoder() { free(m_Models); }
INLINE void BitTreeDecoderInit(CBitTreeDecoder *bitTreeDecoder, UINT32 aNumBitLevels)
  {
    int i;
    bitTreeDecoder->m_NumBitLevels = aNumBitLevels;
    bitTreeDecoder->m_Models       = (CBitDecoder *)malloc( sizeof(CBitDecoder) * (1 << bitTreeDecoder->m_NumBitLevels));
    //BRCM modification
    //printf("malloc in BitTreeDecoderInit=%d\n",sizeof(CBitDecoder) * (1 << bitTreeDecoder->m_NumBitLevels));
    if (!bitTreeDecoder->m_Models) {
	    printf("Error in allocating memory for bitTreeDecoder!\n");
	    return;
    }	    
    for(i = 1; i < (1 << aNumBitLevels); i++)
      BitDecoderInit(&bitTreeDecoder->m_Models[i]);
  }
INLINE UINT32 BitTreeDecode(ISequentialInStream *in_stream, CBitTreeDecoder *bitTreeDecoder, CRangeDecoder *aRangeDecoder)
  {
    UINT32 aModelIndex = 1;
    UINT32 aRange = aRangeDecoder->m_Range;
    UINT32 aCode = aRangeDecoder->m_Code;
    UINT32 aBitIndex;
    for(aBitIndex = bitTreeDecoder->m_NumBitLevels; aBitIndex > 0; aBitIndex--)
    {
      RC_GETBIT(kNumMoveBits, bitTreeDecoder->m_Models[aModelIndex], aModelIndex)
    }
    aRangeDecoder->m_Range = aRange;
    aRangeDecoder->m_Code = aCode;
    return aModelIndex - (1 << bitTreeDecoder->m_NumBitLevels);
  }


////////////////////////////////
// CReverseBitTreeDecoder2

typedef struct CReverseBitTreeDecoder2
{
  UINT32       m_NumBitLevels;
  CBitDecoder *m_Models;
} CReverseBitTreeDecoder2;

//  CReverseBitTreeDecoder2(): m_Models(0) { }
//  ~CReverseBitTreeDecoder2() { free(m_Models); }
INLINE BOOL ReverseBitTreeDecoder2Create(CReverseBitTreeDecoder2 *reverseBitTreeDecoder2, UINT32 aNumBitLevels)
  {
    reverseBitTreeDecoder2->m_NumBitLevels = aNumBitLevels;
    reverseBitTreeDecoder2->m_Models       = (CBitDecoder *)malloc( sizeof(CBitDecoder) * (1 << reverseBitTreeDecoder2->m_NumBitLevels));
    //printf("malloc in ReverseBitTreeDecoder2Create=%d\n",sizeof(CBitDecoder) * (1 << reverseBitTreeDecoder2->m_NumBitLevels));
    if (!reverseBitTreeDecoder2->m_Models) {
	    printf("Error in allocating memory for reverseBitTreeDecoder2!\n");
	    return 0;
    }	    
    return (reverseBitTreeDecoder2->m_Models != 0);
  }
INLINE void ReverseBitTreeDecoder2Init(CReverseBitTreeDecoder2 *reverseBitTreeDecoder2)
  {
    UINT32 aNumModels = 1 << reverseBitTreeDecoder2->m_NumBitLevels;
    UINT32 i;
    for(i = 1; i < aNumModels; i++)
      BitDecoderInit(&reverseBitTreeDecoder2->m_Models[i]);
  }
INLINE UINT32 ReverseBitTreeDecoder2Decode(ISequentialInStream *in_stream, CReverseBitTreeDecoder2 *reverseBitTreeDecoder2, CRangeDecoder *aRangeDecoder)
  {
    UINT32 aModelIndex = 1;
    UINT32 aSymbol = 0;
    UINT32 aRange = aRangeDecoder->m_Range;
    UINT32 aCode = aRangeDecoder->m_Code;
    UINT32 aBitIndex;
    for(aBitIndex = 0; aBitIndex < reverseBitTreeDecoder2->m_NumBitLevels; aBitIndex++)
    {
      RC_GETBIT2(kNumMoveBits, reverseBitTreeDecoder2->m_Models[aModelIndex], aModelIndex, ; , aSymbol |= (1 << aBitIndex))
    }
    aRangeDecoder->m_Range = aRange;
    aRangeDecoder->m_Code = aCode;
    return aSymbol;
  }


////////////////////////////
// CReverseBitTreeDecoder

typedef struct CReverseBitTreeDecoder
{
  UINT32        m_NumBitLevels;
  CBitDecoder  *m_Models;
} CReverseBitTreeDecoder;

//    CReverseBitTreeDecoder(): m_Models(0) { }
//    ~CReverseBitTreeDecoder() { free(m_Models); }
INLINE void ReverseBitTreeDecoderInit(CReverseBitTreeDecoder *reverseBitTreeDecoder, UINT32 aNumBitLevels)
  {
    int i;
    reverseBitTreeDecoder->m_NumBitLevels = aNumBitLevels;
    reverseBitTreeDecoder->m_Models       = (CBitDecoder *)malloc( sizeof(CBitDecoder) * (1 << reverseBitTreeDecoder->m_NumBitLevels));
    //printf("malloc in ReverseBitTreeDecoderInit=%d\n",sizeof(CBitDecoder) * (1 << reverseBitTreeDecoder->m_NumBitLevels));
    if (!reverseBitTreeDecoder->m_Models) {
	    printf("Error in allocating memory for reverseBitTreeDecoder!\n");
	    return;
    }	    
    for(i = 1; i < (1 << reverseBitTreeDecoder->m_NumBitLevels); i++)
      BitDecoderInit(&reverseBitTreeDecoder->m_Models[i]);
  }

INLINE UINT32 ReverseBitTreeDecoderDecode(ISequentialInStream *in_stream, CReverseBitTreeDecoder *reverseBitTreeDecoder, CRangeDecoder *aRangeDecoder)
  {
    UINT32 aModelIndex = 1;
    UINT32 aSymbol = 0;
    UINT32 aRange = aRangeDecoder->m_Range;
    UINT32 aCode = aRangeDecoder->m_Code;
    UINT32 aBitIndex;
    for(aBitIndex = 0; aBitIndex < reverseBitTreeDecoder->m_NumBitLevels; aBitIndex++)
    {
      RC_GETBIT2(kNumMoveBits, reverseBitTreeDecoder->m_Models[aModelIndex], aModelIndex, ; , aSymbol |= (1 << aBitIndex))
    }
    aRangeDecoder->m_Range = aRange;
    aRangeDecoder->m_Code = aCode;
    return aSymbol;
  }



#endif
