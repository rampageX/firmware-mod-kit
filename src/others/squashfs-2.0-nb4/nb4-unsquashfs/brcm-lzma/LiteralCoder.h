#ifndef __LITERALCODER_H
#define __LITERALCODER_H

#include "AriBitCoder.h"
#include "RCDefs.h"

//BRCM modification start
#ifdef _HOST_TOOL
#include "stdio.h"
#include "malloc.h" 
#endif

#ifdef _CFE_
#include "lib_malloc.h"
#include "lib_printf.h"
#define malloc(x) KMALLOC(x, 0)
#define free(x) KFREE(x)
#endif

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/slab.h>
#define printf  printk
//#define malloc(x)  kmalloc(x,GFP_KERNEL)
#define malloc(x)  vmalloc(x)
#define free(x) vfree(x)
#endif
//BRCM modification end

//#define kNumMoveBits 5

typedef struct LitDecoder2
{
  CBitDecoder m_Decoders[3][1 << 8];
} LitDecoder2;


INLINE void LitDecoder2Init(LitDecoder2 *litDecoder2)
  {
    int i, j;
    for (i = 0; i < 3; i++)
      for (j = 1; j < (1 << 8); j++)
        BitDecoderInit(&litDecoder2->m_Decoders[i][j]);
  }

INLINE BYTE LitDecoder2DecodeNormal(ISequentialInStream *in_stream, LitDecoder2 *litDecoder2, CRangeDecoder *aRangeDecoder)
  {
    UINT32 aSymbol = 1;
    UINT32 aRange = aRangeDecoder->m_Range;
    UINT32 aCode = aRangeDecoder->m_Code;        
    do
    {
      RC_GETBIT(kNumMoveBits, litDecoder2->m_Decoders[0][aSymbol], aSymbol)
    }
    while (aSymbol < 0x100);
    aRangeDecoder->m_Range = aRange;
    aRangeDecoder->m_Code = aCode;
    return aSymbol;
  }

INLINE BYTE LitDecoder2DecodeWithMatchByte(ISequentialInStream *in_stream, LitDecoder2 *litDecoder2, CRangeDecoder *aRangeDecoder, BYTE aMatchByte)
  {
    UINT32 aSymbol = 1;
    UINT32 aRange = aRangeDecoder->m_Range;
    UINT32 aCode = aRangeDecoder->m_Code;        
    do
    {
      UINT32 aBit;
      UINT32 aMatchBit = (aMatchByte >> 7) & 1;
      aMatchByte <<= 1;
      RC_GETBIT2(kNumMoveBits, litDecoder2->m_Decoders[1 + aMatchBit][aSymbol], aSymbol, 
          aBit = 0, aBit = 1)
      if (aMatchBit != aBit)
      {
        while (aSymbol < 0x100)
        {
          RC_GETBIT(kNumMoveBits, litDecoder2->m_Decoders[0][aSymbol], aSymbol)
        }
        break;
      }
    }
    while (aSymbol < 0x100);
    aRangeDecoder->m_Range = aRange;
    aRangeDecoder->m_Code = aCode;
    return aSymbol;
  }


typedef struct LitDecoder
{
  LitDecoder2 *m_Coders;
  UINT32 m_NumPrevBits;
  UINT32 m_NumPosBits;
  UINT32 m_PosMask;
} LitDecoder;


//  LitDecoder(): m_Coders(0) {}
//  ~LitDecoder()  { Free(); }

/*
INLINE void LitDecoderFree(LitDecoder *litDecoder)
  { 
    free( (char *) litDecoder->m_Coders );
    litDecoder->m_Coders = 0;
  }
*/

INLINE void LitDecoderCreate(LitDecoder *litDecoder, UINT32 aNumPosBits, UINT32 aNumPrevBits)
  {
//    LitDecoderFree(litDecoder);
    UINT32 aNumStates;
    litDecoder->m_NumPosBits = aNumPosBits;
    litDecoder->m_PosMask = (1 << aNumPosBits) - 1;
    litDecoder->m_NumPrevBits = aNumPrevBits;
    aNumStates = 1 << (aNumPrevBits + aNumPosBits);
    litDecoder->m_Coders = (LitDecoder2*) malloc( sizeof( LitDecoder2 ) * aNumStates );
    //printf("malloc in LitDecoderCreate=%d\n",sizeof( LitDecoder2 ) * aNumStates);
    if (litDecoder->m_Coders == 0)
        printf( "Error allocating memory for LitDecoder m_Coders!\n" );
  }

INLINE void LitDecoderInit(LitDecoder *litDecoder)
  {
    UINT32 i;
    UINT32 aNumStates = 1 << (litDecoder->m_NumPrevBits + litDecoder->m_NumPosBits);
    for (i = 0; i < aNumStates; i++)
      LitDecoder2Init(&litDecoder->m_Coders[i]);
  }

INLINE UINT32 LitDecoderGetState(LitDecoder *litDecoder, UINT32 aPos, BYTE aPrevByte)
  { 
    return ((aPos & litDecoder->m_PosMask) << litDecoder->m_NumPrevBits) + (aPrevByte >> (8 - litDecoder->m_NumPrevBits)); 
  }

INLINE BYTE LitDecodeNormal(ISequentialInStream *in_stream, LitDecoder *litDecoder, CRangeDecoder *aRangeDecoder, UINT32 aPos, BYTE aPrevByte)
  { 
    return LitDecoder2DecodeNormal(in_stream, &litDecoder->m_Coders[LitDecoderGetState(litDecoder, aPos, aPrevByte)], aRangeDecoder); 
  }

INLINE BYTE LitDecodeWithMatchByte(ISequentialInStream *in_stream, LitDecoder *litDecoder, CRangeDecoder *aRangeDecoder, UINT32 aPos, BYTE aPrevByte, BYTE aMatchByte)
  { 
      return LitDecoder2DecodeWithMatchByte(in_stream, &litDecoder->m_Coders[LitDecoderGetState(litDecoder, aPos, aPrevByte)], aRangeDecoder, aMatchByte); 
  }

#endif
