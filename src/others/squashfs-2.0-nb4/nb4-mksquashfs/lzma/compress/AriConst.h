#ifndef __ARICONST_H
#define __ARICONST_H

#include "AriBitCoder.h"

typedef NCompression::NArithmetic::CRangeEncoder CMyRangeEncoder;
typedef NCompression::NArithmetic::CRangeDecoder CMyRangeDecoder;

template <int aNumMoveBits> class CMyBitEncoder: 
    public NCompression::NArithmetic::CBitEncoder<aNumMoveBits> {};
template <int aNumMoveBits> class CMyBitDecoder: 
    public NCompression::NArithmetic::CBitDecoder<aNumMoveBits> {};

#endif



