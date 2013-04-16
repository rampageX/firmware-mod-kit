#include "BinTreeMain.h"

namespace BT_NAMESPACE {

HRESULT CMatchFinderBinTree::Create(UINT32 aSizeHistory,
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter)
{ 
  const UINT32 kAlignMask = (1 << 16) - 1;
  UINT32 aWindowReservSize = aSizeHistory / 2;
  aWindowReservSize += kAlignMask;
  aWindowReservSize &= ~(kAlignMask);

  const int kMinDictSize = (1 << 19);
  if (aWindowReservSize < kMinDictSize)
    aWindowReservSize = kMinDictSize;
  aWindowReservSize += 256;

  try 
  {
    return CInTree::Create(aSizeHistory, aKeepAddBufferBefore, aMatchMaxLen,
      aKeepAddBufferAfter, aWindowReservSize); 
  }
  catch(...)
  {
    return E_OUTOFMEMORY;
  }
}

}
