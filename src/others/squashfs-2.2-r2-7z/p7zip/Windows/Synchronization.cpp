// Windows/Synchronization.cpp

#include "StdAfx.h"

#include "Synchronization.h"

#ifdef DEBUG
#include <assert.h>
#endif

static NWindows::NSynchronization::CCriticalSection gbl_criticalSection;

#define myEnter() gbl_criticalSection.Enter()
#define myLeave() gbl_criticalSection.Leave()
#define myYield() gbl_criticalSection.WaitCond()


DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE *handles, BOOL wait_all, DWORD timeout )
{
#ifdef DEBUG
  assert(timeout == INFINITE);
#endif

  myEnter();
  if (wait_all) {
    while(true) {
      bool found_all = true;
      for(DWORD i=0;i<count;i++) {
        NWindows::NSynchronization::CBaseEvent* hitem = (NWindows::NSynchronization::CBaseEvent*)handles[i];
#ifdef DEBUG
        assert(hitem);
#endif
        if (hitem->_state == false) {
          found_all = false;
          break;
        }
      }
      if (found_all) {
        for(DWORD i=0;i<count;i++) {
          NWindows::NSynchronization::CBaseEvent* hitem = (NWindows::NSynchronization::CBaseEvent*)handles[i];

          if (hitem->_manual_reset == false) {
            hitem->_state = false;
          }
        }
        myLeave();
        return WAIT_OBJECT_0;
      } else {
        myYield();
      }
    }
  } else {
    while(true) {
      for(DWORD i=0;i<count;i++) {
        NWindows::NSynchronization::CBaseEvent* hitem = (NWindows::NSynchronization::CBaseEvent*)handles[i];
#ifdef DEBUG
        assert(hitem);
        assert(hitem->type == TYPE_EVENT);
#endif
        if (hitem->_state == true) {
          if (hitem->_manual_reset == false) {
            hitem->_state = false;
          }
          myLeave();
          return WAIT_OBJECT_0+i;
        }
      }
      myYield();
    }
  }
}


namespace NWindows {
namespace NSynchronization {

bool CBaseEvent::Set()
{
  myEnter();
  _state = true;
  myLeave();
  gbl_criticalSection.SignalCond();
  return true;
}

bool CBaseEvent::Reset()
{
  myEnter();
  _state = false;
  myLeave();
  gbl_criticalSection.SignalCond();
  return true;
}

bool CBaseEvent::Lock()
{
  // return (::myInfiniteWaitForSingleEvent(_handle) == WAIT_OBJECT_0);
  myEnter();
  while(true) {
    if (_state == true) {
      if (_manual_reset == false) {
        _state = false;
      }
      myLeave();
      return true;
    }
    myYield();
  }
  // dead code
}

CEvent::CEvent(bool manualReset, bool initiallyOwn)
{
  if (!Create(manualReset, initiallyOwn))
    throw "CreateEvent error";
}

}}
