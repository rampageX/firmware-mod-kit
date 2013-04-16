#ifndef __COMPRESSION_RANGECODER_H
#define __COMPRESSION_RANGECODER_H

#include "IInOutStreams.h"

#define kNumTopBits 24
#define kTopValue (1 << kNumTopBits)

typedef struct CRangeDecoder
{
  UINT32  m_Range;
  UINT32  m_Code;
} CRangeDecoder;



INLINE void RangeDecoderInit(
    ISequentialInStream *in_stream, 
    CRangeDecoder *rangeDecoder)
  {
    int i;
    rangeDecoder->m_Code = 0;
    rangeDecoder->m_Range = (UINT32)(-1);
    for(i = 0; i < 5; i++)
      rangeDecoder->m_Code = (rangeDecoder->m_Code << 8) | InStreamReadByte(in_stream);
  }

INLINE UINT32 RangeDecodeDirectBits(
    ISequentialInStream *in_stream, 
    CRangeDecoder *rangeDecoder, 
    UINT32 aNumTotalBits)
  {
    UINT32 aRange = rangeDecoder->m_Range;
    UINT32 aCode = rangeDecoder->m_Code;        
    UINT32 aResult = 0;
    UINT32 i;
    for (i = aNumTotalBits; i > 0; i--)
    {
      UINT32 t;
      aRange >>= 1;
      t = (aCode - aRange) >> 31;
      aCode -= aRange & (t - 1);
      aResult = (aResult << 1) | (1 - t);

      if (aRange < kTopValue)
      {
        aCode = (aCode << 8) | InStreamReadByte(in_stream);
        aRange <<= 8; 
      }
    }
    rangeDecoder->m_Range = aRange;
    rangeDecoder->m_Code = aCode;
    return aResult;
  }

#endif
