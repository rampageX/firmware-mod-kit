#include "LenCoder.h"

#ifndef __LZMA_H
#define __LZMA_H


#define kNumRepDistances 4

#define kNumStates 12

static const BYTE kLiteralNextStates[kNumStates] = {0, 0, 0, 0, 1, 2, 3, 4,  5,  6,   4, 5};
static const BYTE kMatchNextStates[kNumStates]   = {7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10};
static const BYTE kRepNextStates[kNumStates]     = {8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11};
static const BYTE kShortRepNextStates[kNumStates]= {9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11};

typedef BYTE CState;

INLINE void CStateInit(CState *m_Index)
    { *m_Index = 0; }
INLINE void CStateUpdateChar(CState *m_Index)
    { *m_Index = kLiteralNextStates[*m_Index]; }
INLINE void CStateUpdateMatch(CState *m_Index)
    { *m_Index = kMatchNextStates[*m_Index]; }
INLINE void CStateUpdateRep(CState *m_Index)
    { *m_Index = kRepNextStates[*m_Index]; }
INLINE void CStateUpdateShortRep(CState *m_Index)
    { *m_Index = kShortRepNextStates[*m_Index]; }


#define kNumPosSlotBits 6
#define kDicLogSizeMax 28
#define kDistTableSizeMax 56

//extern UINT32 kDistStart[kDistTableSizeMax];
static const BYTE kDistDirectBits[kDistTableSizeMax] = 
{
  0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,
  10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 
  20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26 
};

#define kNumLenToPosStates 4
INLINE UINT32 GetLenToPosState(UINT32 aLen)
{
  aLen -= 2;
  if (aLen < kNumLenToPosStates)
    return aLen;
  return kNumLenToPosStates - 1;
}

#define kMatchMinLen 2

#define kMatchMaxLen (kMatchMinLen + kNumSymbolsTotal - 1)

#define kNumAlignBits 4
#define kAlignTableSize 16
#define kAlignMask 15

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumPosModels 10

#define kNumFullDistances (1 << (kEndPosModelIndex / 2))


#define kMainChoiceLiteralIndex 0
#define kMainChoiceMatchIndex 1

#define kMatchChoiceDistanceIndex0
#define kMatchChoiceRepetitionIndex 1

#define kNumMoveBitsForMainChoice 5
#define kNumMoveBitsForPosCoders 5

#define kNumMoveBitsForAlignCoders 5

#define kNumMoveBitsForPosSlotCoder 5

#define kNumLitPosStatesBitsEncodingMax 4
#define kNumLitContextBitsMax 8


#endif
