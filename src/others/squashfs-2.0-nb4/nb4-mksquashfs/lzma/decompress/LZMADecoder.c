#include "Portable.h"
#ifdef _HOST_TOOL
#include "stdio.h"
#endif
#include "LZMADecoder.h"


//#define RETURN_E_OUTOFMEMORY_IF_FALSE(x) { if (!(x)) return E_OUTOFMEMORY; }


static UINT32    kDistStart[kDistTableSizeMax];
struct WindowOut out_window;

/*
 * BRCM modification: free all the allocated buffer by malloc
 *
 */
static void LzmaDecoderFreeBuffer(LzmaDecoder  *lzmaDecoder)
{
  int i,aPosState;
  
  //printf("free lzmaDecoder->m_LiteralDecoder\n");
  free((&lzmaDecoder->m_LiteralDecoder)->m_Coders);

  for (i = 0; i < kNumLenToPosStates; i++) {
      //printf("free lzmaDecoder->m_PosSlotDecoder\n");
      free((&lzmaDecoder->m_PosSlotDecoder[i])->m_Models);
  }
  // from LenDecoderInit(&lzmaDecoder->m_LenDecoder;
  for (aPosState = 0; aPosState < (&lzmaDecoder->m_LenDecoder)->m_NumPosStates; aPosState++) {
      //printf("free lzmaDecoder->m_PosSlotDecoder\n");
      free( (&(&lzmaDecoder->m_LenDecoder)->m_LowCoder[aPosState])->m_Models );
      //printf("free lzmaDecoder->m_PosSlotDecoder\n");
      free( (&(&lzmaDecoder->m_LenDecoder)->m_MidCoder[aPosState])->m_Models );
   }
  //printf("free lzmaDecoder->m_PosSlotDecoder\n");
  free( (&(&lzmaDecoder->m_LenDecoder)->m_HighCoder)->m_Models );


  // from LenDecoderInit(&lzmaDecoder->m_RepMatchLenDecoder);
  for (aPosState = 0; aPosState < (&lzmaDecoder->m_RepMatchLenDecoder)->m_NumPosStates; aPosState++) {
      //printf("free lzmaDecoder->m_PosSlotDecoder\n");
      free( (&(&lzmaDecoder->m_RepMatchLenDecoder)->m_LowCoder[aPosState])->m_Models );
      //printf("free lzmaDecoder->m_PosSlotDecoder\n");
      free( (&(&lzmaDecoder->m_RepMatchLenDecoder)->m_MidCoder[aPosState])->m_Models );
  }
  //printf("free lzmaDecoder->m_PosSlotDecoder\n");
  free( (&(&lzmaDecoder->m_RepMatchLenDecoder)->m_HighCoder)->m_Models );

    
  //printf("free lzmaDecoder->m_PosAlignDecoder\n");
  free((&lzmaDecoder->m_PosAlignDecoder)->m_Models);

  for(i = 0; i < kNumPosModels; i++) {
      //printf("free lzmaDecoder->m_PosDecoders\n");
      free((&lzmaDecoder->m_PosDecoders[i])->m_Models);
  }
  
}

HRESULT LzmaDecoderSetDictionarySize(
    LzmaDecoder  *lzmaDecoder, 
    UINT32        aDictionarySize)
{
  if (aDictionarySize > (1 << kDicLogSizeMax))
    return E_INVALIDARG;
  
//  UINT32 aWindowReservSize = MyMax(aDictionarySize, UINT32(1 << 21));

  if (lzmaDecoder->m_DictionarySize != aDictionarySize)
  {
    lzmaDecoder->m_DictionarySize = aDictionarySize;
  }
  return S_OK;
}

HRESULT LzmaDecoderSetLiteralProperties( 
    LzmaDecoder  *lzmaDecoder,
    UINT32        aLiteralPosStateBits, 
    UINT32        aLiteralContextBits)
{
  if (aLiteralPosStateBits > 8)
    return E_INVALIDARG;
  if (aLiteralContextBits > 8)
    return E_INVALIDARG;
  LitDecoderCreate(&lzmaDecoder->m_LiteralDecoder, aLiteralPosStateBits, aLiteralContextBits);
  return S_OK;
}

HRESULT LzmaDecoderSetPosBitsProperties(
    LzmaDecoder *lzmaDecoder, 
    UINT32       aNumPosStateBits)
{
  UINT32 aNumPosStates;
  if (aNumPosStateBits > (UINT32) kNumPosStatesBitsMax)
    return E_INVALIDARG;
  aNumPosStates = 1 << aNumPosStateBits;
  LenDecoderCreate(&lzmaDecoder->m_LenDecoder, aNumPosStates);
  LenDecoderCreate(&lzmaDecoder->m_RepMatchLenDecoder, aNumPosStates);
  lzmaDecoder->m_PosStateMask = aNumPosStates - 1;
  return S_OK;
}


void LzmaDecoderConstructor(LzmaDecoder *lzmaDecoder)
{
  lzmaDecoder->m_DictionarySize = ((UINT32)-1);
  LzmaDecoderCreate(lzmaDecoder);
}

HRESULT LzmaDecoderCreate(LzmaDecoder *lzmaDecoder)
{
  int i;
  for(i = 0; i < kNumPosModels; i++)
  {
    if (!(ReverseBitTreeDecoder2Create(&lzmaDecoder->m_PosDecoders[i],kDistDirectBits[kStartPosModelIndex + i])))
        return E_OUTOFMEMORY;;
  }
  return S_OK;
}


HRESULT LzmaDecoderInit(LzmaDecoder *lzmaDecoder)
{
  int    i;
  UINT32 j;

  RangeDecoderInit(&in_stream, &lzmaDecoder->m_RangeDecoder);

  OutWindowInit();

  for(i = 0; i < kNumStates; i++)
  {
    for (j = 0; j <= lzmaDecoder->m_PosStateMask; j++)
    {
      BitDecoderInit(&lzmaDecoder->m_MainChoiceDecoders[i][j]);
      BitDecoderInit(&lzmaDecoder->m_MatchRepShortChoiceDecoders[i][j]);
    }
    BitDecoderInit(&lzmaDecoder->m_MatchChoiceDecoders[i]);
    BitDecoderInit(&lzmaDecoder->m_MatchRepChoiceDecoders[i]);
    BitDecoderInit(&lzmaDecoder->m_MatchRep1ChoiceDecoders[i]);
    BitDecoderInit(&lzmaDecoder->m_MatchRep2ChoiceDecoders[i]);
  }
  
  LitDecoderInit(&lzmaDecoder->m_LiteralDecoder);
   
  for (i = 0; i < (int) kNumLenToPosStates; i++)
    BitTreeDecoderInit(&lzmaDecoder->m_PosSlotDecoder[i],kNumPosSlotBits);

  for(i = 0; i < kNumPosModels; i++)
    ReverseBitTreeDecoder2Init(&lzmaDecoder->m_PosDecoders[i]);
  
  LenDecoderInit(&lzmaDecoder->m_LenDecoder);
  LenDecoderInit(&lzmaDecoder->m_RepMatchLenDecoder);

  ReverseBitTreeDecoderInit(&lzmaDecoder->m_PosAlignDecoder, kNumAlignBits);
  return S_OK;

}

HRESULT LzmaDecoderCodeReal( 
    LzmaDecoder     *lzmaDecoder, 
    UINT64          *anInSize, 
    UINT64          *anOutSize)
{
  BOOL                  aPeviousIsMatch         = FALSE;
  BYTE                  aPreviousByte           = 0;
  UINT32                aRepDistances[kNumRepDistances];
  int                   i;
  UINT64                aNowPos64               = 0;
  UINT64                aSize                   = *anOutSize;
  ISequentialInStream   my_in_stream;
//  WindowOut             out_window;
  CState                aState;

  CStateInit(&aState);

  if (anOutSize == NULL)
  {
      printf("CodeReal: invalid argument %x\n", (UINT32) anOutSize );
      return E_INVALIDARG;
  }


  LzmaDecoderInit(lzmaDecoder);

  my_in_stream.data           = in_stream.data;
  my_in_stream.remainingBytes = in_stream.remainingBytes;

  for(i = 0 ; i < (int) kNumRepDistances; i++)
    aRepDistances[i] = 0;

  //while(aNowPos64 < aSize)
  while(my_in_stream.remainingBytes > 0)
  {
    UINT64 aNext = MyMin(aNowPos64 + (1 << 18), aSize);
    while(aNowPos64 < aNext)
    {
      UINT32 aPosState = (UINT32)(aNowPos64) & lzmaDecoder->m_PosStateMask;
      if (BitDecode(&my_in_stream, 
                    &lzmaDecoder->m_MainChoiceDecoders[aState][aPosState], 
                    &lzmaDecoder->m_RangeDecoder) == (UINT32) kMainChoiceLiteralIndex)
      {
        CStateUpdateChar(&aState);
        if(aPeviousIsMatch)
        {
          BYTE aMatchByte = OutWindowGetOneByte(0 - aRepDistances[0] - 1);
          aPreviousByte = LitDecodeWithMatchByte(&my_in_stream, 
                                                 &lzmaDecoder->m_LiteralDecoder, 
                                                 &lzmaDecoder->m_RangeDecoder, 
                                                 (UINT32)(aNowPos64), 
                                                 aPreviousByte, 
                                                 aMatchByte);
          aPeviousIsMatch = FALSE;
        }
        else
          aPreviousByte = LitDecodeNormal(&my_in_stream, 
                                          &lzmaDecoder->m_LiteralDecoder, 
                                          &lzmaDecoder->m_RangeDecoder, 
                                         (UINT32)(aNowPos64), 
                                          aPreviousByte);
        OutWindowPutOneByte(aPreviousByte);
        aNowPos64++;
      }
      else             
      {
        UINT32 aDistance, aLen;
        aPeviousIsMatch = TRUE;
        if(BitDecode(&my_in_stream, 
                     &lzmaDecoder->m_MatchChoiceDecoders[aState], 
                     &lzmaDecoder->m_RangeDecoder) == (UINT32) kMatchChoiceRepetitionIndex)
        {
          if(BitDecode(&my_in_stream, 
                       &lzmaDecoder->m_MatchRepChoiceDecoders[aState], 
                       &lzmaDecoder->m_RangeDecoder) == 0)
          {
            if(BitDecode(&my_in_stream, 
                         &lzmaDecoder->m_MatchRepShortChoiceDecoders[aState][aPosState], 
                         &lzmaDecoder->m_RangeDecoder) == 0)
            {
              CStateUpdateShortRep(&aState);
              aPreviousByte = OutWindowGetOneByte(0 - aRepDistances[0] - 1);
              OutWindowPutOneByte(aPreviousByte);
              aNowPos64++;
              continue;
            }
            aDistance = aRepDistances[0];
          }
          else
          {
            if(BitDecode(&my_in_stream, 
                         &lzmaDecoder->m_MatchRep1ChoiceDecoders[aState], 
                         &lzmaDecoder->m_RangeDecoder) == 0)
            {
              aDistance = aRepDistances[1];
              aRepDistances[1] = aRepDistances[0];
            }
            else 
            {
              if (BitDecode(&my_in_stream, 
                            &lzmaDecoder->m_MatchRep2ChoiceDecoders[aState], 
                            &lzmaDecoder->m_RangeDecoder) == 0)
              {
                aDistance = aRepDistances[2];
              }
              else
              {
                aDistance = aRepDistances[3];
                aRepDistances[3] = aRepDistances[2];
              }
              aRepDistances[2] = aRepDistances[1];
              aRepDistances[1] = aRepDistances[0];
            }
            aRepDistances[0] = aDistance;
          }
          aLen = LenDecode(&my_in_stream, 
                           &lzmaDecoder->m_RepMatchLenDecoder, 
                           &lzmaDecoder->m_RangeDecoder, 
                           aPosState) + kMatchMinLen;
          CStateUpdateRep(&aState);
        }
        else
        {
          UINT32 aPosSlot;
          aLen = kMatchMinLen + LenDecode(&my_in_stream, 
                                          &lzmaDecoder->m_LenDecoder, 
                                          &lzmaDecoder->m_RangeDecoder, 
                                          aPosState);
          CStateUpdateMatch(&aState);
          aPosSlot = BitTreeDecode(&my_in_stream, 
                                   &lzmaDecoder->m_PosSlotDecoder[GetLenToPosState(aLen)],
                                   &lzmaDecoder->m_RangeDecoder);
          if (aPosSlot >= (UINT32) kStartPosModelIndex)
          {
            aDistance = kDistStart[aPosSlot];
            if (aPosSlot < (UINT32) kEndPosModelIndex)
              aDistance += ReverseBitTreeDecoder2Decode(&my_in_stream, 
                                                        &lzmaDecoder->m_PosDecoders[aPosSlot - kStartPosModelIndex],
                                                        &lzmaDecoder->m_RangeDecoder);
            else
            {
              aDistance += (RangeDecodeDirectBits(&my_in_stream, 
                                                  &lzmaDecoder->m_RangeDecoder, 
                                                  kDistDirectBits[aPosSlot] - kNumAlignBits) << kNumAlignBits);
              aDistance += ReverseBitTreeDecoderDecode(&my_in_stream, 
                                                       &lzmaDecoder->m_PosAlignDecoder, 
                                                       &lzmaDecoder->m_RangeDecoder);
            }
          }
          else
            aDistance = aPosSlot;

          
          aRepDistances[3] = aRepDistances[2];
          aRepDistances[2] = aRepDistances[1];
          aRepDistances[1] = aRepDistances[0];
          
          aRepDistances[0] = aDistance;
        }
        if (aDistance >= aNowPos64)
        {
            printf("CodeReal: invalid data\n" );
            return E_INVALIDDATA;
        }
        OutWindowCopyBackBlock(aDistance, aLen);
        aNowPos64 += aLen;
        aPreviousByte = OutWindowGetOneByte(0 - 1);
      }
    }
  }
  
  //BRCM modification
  LzmaDecoderFreeBuffer(lzmaDecoder);
  
  OutWindowFlush();
  return S_OK;
}

HRESULT LzmaDecoderCode(
    LzmaDecoder *lzmaDecoder,
    UINT64 *anInSize, 
    UINT64 *anOutSize)
{

    UINT32 aStartValue = 0;
    int i;
    
    for (i = 0; i < kDistTableSizeMax; i++)
    {
        kDistStart[i] = aStartValue;
        aStartValue += (1 << kDistDirectBits[i]);
    }
    return LzmaDecoderCodeReal( 
        lzmaDecoder, 
        anInSize, 
        anOutSize);
}

HRESULT LzmaDecoderReadCoderProperties(LzmaDecoder *lzmaDecoder)
{
  UINT32 aNumPosStateBits;
  UINT32 aLiteralPosStateBits;
  UINT32 aLiteralContextBits;
  UINT32 aDictionarySize;
  BYTE   aRemainder;
  UINT32 aProcessesedSize;

  BYTE aByte;
  RETURN_IF_NOT_S_OK(InStreamRead(&aByte, 
                                  sizeof(aByte), 
                                  &aProcessesedSize));

  if (aProcessesedSize != sizeof(aByte))
    return E_INVALIDARG;

  aLiteralContextBits   = aByte % 9;
  aRemainder            = aByte / 9;
  aLiteralPosStateBits  = aRemainder % 5;
  aNumPosStateBits      = aRemainder / 5;

  RETURN_IF_NOT_S_OK(InStreamRead(&aDictionarySize, 
                                  sizeof(aDictionarySize), 
                                  &aProcessesedSize));

  if (aProcessesedSize != sizeof(aDictionarySize))
    return E_INVALIDARG;

  RETURN_IF_NOT_S_OK( LzmaDecoderSetDictionarySize(lzmaDecoder, 
                                                   aDictionarySize) );
  RETURN_IF_NOT_S_OK( LzmaDecoderSetLiteralProperties(lzmaDecoder, 
                                                      aLiteralPosStateBits, 
                                                      aLiteralContextBits) );
  RETURN_IF_NOT_S_OK( LzmaDecoderSetPosBitsProperties(lzmaDecoder, 
                                                      aNumPosStateBits) );

  return S_OK;
}

