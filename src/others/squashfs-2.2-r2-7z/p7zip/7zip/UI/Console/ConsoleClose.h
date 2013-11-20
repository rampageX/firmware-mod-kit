// ConsoleCloseUtils.h

#ifndef __CONSOLECLOSEUTILS_H
#define __CONSOLECLOSEUTILS_H

namespace NConsoleClose {

bool TestBreakSignal();

class CCtrlHandlerSetter
{
#ifdef ENV_UNIX
  void (*memo_sig_int)(int);
  void (*memo_sig_term)(int);
#endif
public:
  CCtrlHandlerSetter();
  virtual ~CCtrlHandlerSetter();
};

class CCtrlBreakException 
{};

void CheckCtrlBreak();

}

#endif
