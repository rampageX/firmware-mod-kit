// #ifndef __BINTREEMF_H
// #define __BINTREEMF_H

#include "BinTree.h"

namespace BT_NAMESPACE {

class CMatchFinderBinTree : public CInTree
{
public:
  HRESULT Create(UINT32 aSizeHistory,
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter);
};
 
}

// #endif

