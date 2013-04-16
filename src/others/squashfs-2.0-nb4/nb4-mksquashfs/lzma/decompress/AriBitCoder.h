#ifndef __COMPRESSION_BITCODER_H
#define __COMPRESSION_BITCODER_H

#include "RangeCoder.h"

#define kNumBitModelTotalBits  11
#define kBitModelTotal (1 << kNumBitModelTotalBits)

#define kNumMoveReducingBits 2


typedef UINT32 CBitDecoder;

INLINE  void BitDecoderInit(CBitDecoder *bitDecoder)
  { 
    *bitDecoder = kBitModelTotal / 2; 
  }

#if 0
UINT32 BitDecode(ISequentialInStream *in_stream, CBitDecoder *bitDecoder, CRangeDecoder *aRangeDecoder);
#else
INLINE  UINT32 BitDecode(ISequentialInStream *in_stream, CBitDecoder *bitDecoder, CRangeDecoder *aRangeDecoder)
  {
    UINT32 aNewBound = (aRangeDecoder->m_Range >> kNumBitModelTotalBits) * (*bitDecoder);
    if (aRangeDecoder->m_Code < aNewBound)
    {
      aRangeDecoder->m_Range = aNewBound;
      *bitDecoder += (kBitModelTotal - *bitDecoder) >> kNumMoveBits;
      if (aRangeDecoder->m_Range < kTopValue)
      {
        aRangeDecoder->m_Code = (aRangeDecoder->m_Code << 8) | InStreamReadByte(in_stream);
        aRangeDecoder->m_Range <<= 8;
      }
      return 0;
    }
    else
    {
      aRangeDecoder->m_Range -= aNewBound;
      aRangeDecoder->m_Code -= aNewBound;
      *bitDecoder -= (*bitDecoder) >> kNumMoveBits;
      if (aRangeDecoder->m_Range < kTopValue)
      {
        aRangeDecoder->m_Code = (aRangeDecoder->m_Code << 8) | InStreamReadByte(in_stream);
        aRangeDecoder->m_Range <<= 8;
      }
      return 1;
    }
  }
#endif

#endif
