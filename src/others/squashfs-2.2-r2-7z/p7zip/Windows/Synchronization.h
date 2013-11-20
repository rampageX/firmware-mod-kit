// Windows/Synchronization.h

// #pragma once

#ifndef __WINDOWS_SYNCHRONIZATION_H
#define __WINDOWS_SYNCHRONIZATION_H

#include "Defs.h"

#ifdef ENV_BEOS
#include <Locker.h>
#include <kernel/OS.h>
#include <list>
#endif

namespace NWindows {
namespace NSynchronization {

class CBaseEvent
{
public:
  bool _manual_reset;
  bool _state;

  CBaseEvent() {} 
  ~CBaseEvent() { Close(); }

  operator HANDLE() { return (HANDLE)this; }

  bool Create(bool manualReset, bool initiallyOwn)
  {
    _manual_reset = manualReset;
    _state        = initiallyOwn;
    return true;
  }

  bool Set();
  bool Reset();

  bool Lock();
  
  bool Close() { return true; }
};

class CEvent: public CBaseEvent
{
public:
  CEvent() {};
  CEvent(bool manualReset, bool initiallyOwn);
};

class CManualResetEvent: public CEvent
{
public:
  CManualResetEvent(bool initiallyOwn = false):
    CEvent(true, initiallyOwn) {};
};

class CAutoResetEvent: public CEvent
{
public:
  CAutoResetEvent(bool initiallyOwn = false):
    CEvent(false, initiallyOwn) {};
};

#ifdef ENV_BEOS
class CCriticalSection : BLocker
{
  std::list<thread_id> _waiting;
public:
  CCriticalSection() {}
  ~CCriticalSection() {}
  void Enter() { Lock(); }
  void Leave() { Unlock(); }
  void WaitCond() { 
    _waiting.push_back(find_thread(NULL));
    thread_id sender;
    Unlock();
    int msg = receive_data(&sender, NULL, 0);
    Lock();
  }
  void SignalCond() {
    Lock();
    for (std::list<thread_id>::iterator index = _waiting.begin(); index != _waiting.end(); index++) {
      send_data(*index, '7zCN', NULL, 0);
    }
   _waiting.clear();
    Unlock();
  }
};
#else
class CCriticalSection
{
  pthread_mutex_t _object;
  pthread_cond_t _cond;
public:
  CCriticalSection() {
    ::pthread_mutex_init(&_object,0);
    ::pthread_cond_init(&_cond,0);
  }
  ~CCriticalSection() {
    ::pthread_mutex_destroy(&_object);
    ::pthread_cond_destroy(&_cond);
  }
  void Enter() { ::pthread_mutex_lock(&_object); }
  void Leave() { ::pthread_mutex_unlock(&_object); }
  void WaitCond() { ::pthread_cond_wait(&_cond, &_object); }
  void SignalCond() { ::pthread_cond_broadcast(&_cond); }
};
#endif

class CCriticalSectionLock
{
  CCriticalSection &_object;
  void Unlock()  { _object.Leave(); }
public:
  CCriticalSectionLock(CCriticalSection &object): _object(object) 
    {_object.Enter(); } 
  ~CCriticalSectionLock() { Unlock(); }
};

}}

#endif
