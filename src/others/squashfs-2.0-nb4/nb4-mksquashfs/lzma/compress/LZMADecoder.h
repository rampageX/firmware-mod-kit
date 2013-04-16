#ifndef __LZARITHMETIC_DECODER_H
#define __LZARITHMETIC_DECODER_H

#include "WindowOut.h"
#include "LZMA.h"
#include "LenCoder.h"
#include "LiteralCoder.h"

namespace NCompress {
namespace NLZMA {

typedef CMyBitDecoder<kNumMoveBitsForMainChoice> CMyBitDecoder2;

class CDecoder
{
  NStream::NWindow::COut m_OutWindowStream;
  CMyRangeDecoder m_RangeDecoder;

  CMyBitDecoder2 m_MainChoiceDecoders[kNumStates][NLength::kNumPosStatesMax];
  CMyBitDecoder2 m_MatchChoiceDecoders[kNumStates];
  CMyBitDecoder2 m_MatchRepChoiceDecoders[kNumStates];
  CMyBitDecoder2 m_MatchRep1ChoiceDecoders[kNumStates];
  CMyBitDecoder2 m_MatchRep2ChoiceDecoders[kNumStates];
  CMyBitDecoder2 m_MatchRepShortChoiceDecoders[kNumStates][NLength::kNumPosStatesMax];

  CBitTreeDecoder<kNumMoveBitsForPosSlotCoder, kNumPosSlotBits> m_PosSlotDecoder[kNumLenToPosStates];

  CReverseBitTreeDecoder2<kNumMoveBitsForPosCoders> m_PosDecoders[kNumPosModels];
  CReverseBitTreeDecoder<kNumMoveBitsForAlignCoders, kNumAlignBits> m_PosAlignDecoder;
  // CBitTreeDecoder2<kNumMoveBitsForPosCoders> m_PosDecoders[kNumPosModels];
  // CBitTreeDecoder<kNumMoveBitsForAlignCoders, kNumAlignBits> m_PosAlignDecoder;
  
  NLength::CDecoder m_LenDecoder;
  NLength::CDecoder m_RepMatchLenDecoder;

  NLiteral::CDecoder m_LiteralDecoder;

  UINT32 m_DictionarySize;
  
  UINT32 m_PosStateMask;

  HRESULT Create();

  HRESULT Init(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream);

  HRESULT Flush() {  return m_OutWindowStream.Flush(); }

  HRESULT CodeReal(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize);

public:

  CDecoder();
  
  HRESULT Code(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize);
  HRESULT ReadCoderProperties(ISequentialInStream *anInStream);

  HRESULT SetDictionarySize(UINT32 aDictionarySize);
  HRESULT SetLiteralProperties(UINT32 aLiteralPosStateBits, UINT32 aLiteralContextBits);
  HRESULT SetPosBitsProperties(UINT32 aNumPosStateBits);
};

}}

#endif
