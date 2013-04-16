#ifndef __STREAM_WINDOWOUT_H
#define __STREAM_WINDOWOUT_H

#include "IInOutStreams.h"

typedef struct WindowOut
{
  BYTE  *Buffer;
  UINT32 Pos;
} WindowOut;

extern WindowOut out_window;

#define OutWindowInit() \
  { \
    out_window.Buffer = (BYTE *) out_stream.data; \
    out_window.Pos = 0; \
  }

#define OutWindowFlush() \
  { \
    OutStreamSizeSet( out_window.Pos ); \
  } 

// BRCM modification 
INLINE void OutWindowCopyBackBlock(UINT32 aDistance, UINT32 aLen)
  {
    BYTE   *p = out_window.Buffer + out_window.Pos;
    UINT32  i;
    aDistance++;
    for(i = 0; i < aLen; i++)
      p[i] = p[i - aDistance];
    out_window.Pos += aLen;
  }


#define OutWindowPutOneByte(aByte) \
  { \
    out_window.Buffer[out_window.Pos++] = aByte; \
  } 

#define OutWindowGetOneByte(anIndex) \
     (out_window.Buffer[out_window.Pos + anIndex])



#endif
