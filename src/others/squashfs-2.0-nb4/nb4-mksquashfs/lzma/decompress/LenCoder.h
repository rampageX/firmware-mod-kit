#ifndef __LENCODER_H
#define __LENCODER_H

#include "BitTreeCoder.h"


#define kNumPosStatesBitsMax 4
#define kNumPosStatesMax 16


#define kNumPosStatesBitsEncodingMax 4
#define kNumPosStatesEncodingMax 16


//#define kNumMoveBits 5

#define kNumLenBits 3
#define kNumLowSymbols (1 << kNumLenBits)

#define kNumMidBits 3
#define kNumMidSymbols (1 << kNumMidBits)

#define kNumHighBits 8

#define kNumSymbolsTotal (kNumLowSymbols + kNumMidSymbols + (1 << kNumHighBits))

typedef struct LenDecoder
{
  CBitDecoder       m_Choice;
  CBitDecoder       m_Choice2;
  CBitTreeDecoder   m_LowCoder[kNumPosStatesMax];
  CBitTreeDecoder   m_MidCoder[kNumPosStatesMax];
  CBitTreeDecoder   m_HighCoder; 
  UINT32            m_NumPosStates;
} LenDecoder;

INLINE void LenDecoderCreate(LenDecoder *lenCoder, UINT32 aNumPosStates)
  { 
    lenCoder->m_NumPosStates = aNumPosStates; 
  }

INLINE void LenDecoderInit(LenDecoder *lenCoder)
  {
    UINT32 aPosState;
    BitDecoderInit(&lenCoder->m_Choice);
    for (aPosState = 0; aPosState < lenCoder->m_NumPosStates; aPosState++)
    {
      BitTreeDecoderInit(&lenCoder->m_LowCoder[aPosState],kNumLenBits);
      BitTreeDecoderInit(&lenCoder->m_MidCoder[aPosState],kNumMidBits);
    }
    BitTreeDecoderInit(&lenCoder->m_HighCoder,kNumHighBits);
    BitDecoderInit(&lenCoder->m_Choice2);
  }

INLINE UINT32 LenDecode(ISequentialInStream *in_stream, LenDecoder *lenCoder, CRangeDecoder *aRangeDecoder, UINT32 aPosState)
  {
    if(BitDecode(in_stream, &lenCoder->m_Choice, aRangeDecoder) == 0)
      return BitTreeDecode(in_stream, &lenCoder->m_LowCoder[aPosState],aRangeDecoder);
    else
    {
      UINT32 aSymbol = kNumLowSymbols;
      if(BitDecode(in_stream, &lenCoder->m_Choice2, aRangeDecoder) == 0)
        aSymbol += BitTreeDecode(in_stream, &lenCoder->m_MidCoder[aPosState],aRangeDecoder);
      else
      {
        aSymbol += kNumMidSymbols;
        aSymbol += BitTreeDecode(in_stream, &lenCoder->m_HighCoder,aRangeDecoder);
      }
      return aSymbol;
    }
  }



#endif
