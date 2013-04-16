#include "Portable.h"
#include "WindowIn.h"

namespace BT_NAMESPACE {

typedef UINT32 CIndex;
const UINT32 kMaxValForNormalize = (UINT32(1) << 31) - 1;

struct CPair
{
  CIndex Left;
  CIndex Right;
};

class CInTree: public NStream::NWindow::CIn
{
  UINT32 m_HistorySize;
  UINT32 m_MatchMaxLen;

  CIndex *m_Hash;
  
  #ifdef HASH_ARRAY_2
  CIndex *m_Hash2;
  #ifdef HASH_ARRAY_3
  CIndex *m_Hash3;
  #endif
  #endif
  
  CPair *m_Son;
  CPair *m_Base;

  UINT32 m_CutValue;

  void NormalizeLinks(CIndex *anArray, UINT32 aNumItems, UINT32 aSubValue);
  void Normalize();
  void FreeMemory();

protected:
  virtual void AfterMoveBlock();
public:
  CInTree();
  ~CInTree();
  HRESULT Create(UINT32 aSizeHistory, UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter, UINT32 _dwSizeReserv = (1<<17));
	HRESULT Init(ISequentialInStream *aStream);
  void SetCutValue(UINT32 aCutValue) { m_CutValue = aCutValue; }
  UINT32 GetLongestMatch(UINT32 *aDistances);
  void DummyLongestMatch();
  HRESULT MovePos()
  {
    RETURN_IF_NOT_S_OK(CIn::MovePos());
    if (m_Pos == kMaxValForNormalize)
      Normalize();
    return S_OK;
  }
  void ReduceOffsets(UINT32 aSubValue)
  {
    CIn::ReduceOffsets(aSubValue);
    m_Son += aSubValue;
  }
};

}

