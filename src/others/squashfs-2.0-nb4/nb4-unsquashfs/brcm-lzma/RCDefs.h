#ifndef __RCDEFS_H
#define __RCDEFS_H

#include "AriBitCoder.h"

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
