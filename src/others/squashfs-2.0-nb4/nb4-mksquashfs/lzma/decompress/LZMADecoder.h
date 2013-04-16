#ifndef __LZARITHMETIC_DECODER_H
#define __LZARITHMETIC_DECODER_H

#include "WindowOut.h"
#include "LZMA.h"
#include "LenCoder.h"
#include "LiteralCoder.h"


typedef struct LzmaDecoder
{
  CRangeDecoder m_RangeDecoder;

  CBitDecoder m_MainChoiceDecoders[kNumStates][kNumPosStatesMax];
  CBitDecoder m_MatchChoiceDecoders[kNumStates];
  CBitDecoder m_MatchRepChoiceDecoders[kNumStates];
  CBitDecoder m_MatchRep1ChoiceDecoders[kNumStates];
  CBitDecoder m_MatchRep2ChoiceDecoders[kNumStates];
  CBitDecoder m_MatchRepShortChoiceDecoders[kNumStates][kNumPosStatesMax];

  CBitTreeDecoder               m_PosSlotDecoder[kNumLenToPosStates];

  CReverseBitTreeDecoder2       m_PosDecoders[kNumPosModels];
  CReverseBitTreeDecoder        m_PosAlignDecoder;
  
  LenDecoder m_LenDecoder;
  LenDecoder m_RepMatchLenDecoder;

  LitDecoder m_LiteralDecoder;

  UINT32 m_DictionarySize;
  
  UINT32 m_PosStateMask;
} LzmaDecoder;

  HRESULT LzmaDecoderCreate(LzmaDecoder *lzmaDecoder);

  HRESULT LzmaDecoderInit(LzmaDecoder *lzmaDecoder);

//static inline  HRESULT LzmaDecoderFlush() { return OutWindowFlush(); }

  HRESULT LzmaDecoderCodeReal( 
      LzmaDecoder           *lzmaDecoder, 
//      ISequentialInStream   *in_stream, 
      UINT64                *anInSize, 
//      WindowOut             *out_window,
      UINT64                *anOutSize);


  void LzmaDecoderConstructor( LzmaDecoder *lzmaDecoder );
  
  HRESULT LzmaDecoderCode( LzmaDecoder *lzmaDecoder, UINT64 *anInSize, UINT64 *anOutSize);
  HRESULT LzmaDecoderReadCoderProperties(LzmaDecoder *lzmaDecoder );

  HRESULT LzmaDecoderSetDictionarySize(LzmaDecoder *lzmaDecoder, UINT32 aDictionarySize);
  HRESULT LzmaDecoderSetLiteralProperties(LzmaDecoder *lzmaDecoder, UINT32 aLiteralPosStateBits, UINT32 aLiteralContextBits);
  HRESULT LzmaDecoderSetPosBitsProperties(LzmaDecoder *lzmaDecoder, UINT32 aNumPosStateBits);


#endif
