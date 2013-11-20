// FileStreams.h

#ifndef __FILESTREAMS_H
#define __FILESTREAMS_H

#if defined(_WIN32) || defined(ENV_UNIX)
#include "../../Windows/FileIO.h"
#else
#include "../../Common/C_FileIO.h"
#endif

#include "../IStream.h"
#include "../../Common/MyCom.h"

class CInFileStream: 
  public IInStream,
  public IStreamGetSize,
  public CMyUnknownImp
{
  bool _ignoreSymbolicLink;
public:
  #if defined(_WIN32) || defined(ENV_UNIX)
  NWindows::NFile::NIO::CInFile File;
  #else
  NC::NFile::NIO::CInFile File;
  #endif
  CInFileStream(bool b=false) { _ignoreSymbolicLink = b; }
  virtual ~CInFileStream() {}

  bool Open(LPCTSTR fileName);
  #if defined(_WIN32) || defined(ENV_UNIX)
  #ifndef _UNICODE
  bool Open(LPCWSTR fileName);
  #endif
  #endif

  MY_UNKNOWN_IMP2(IInStream, IStreamGetSize)

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);

  STDMETHOD(GetSize)(UInt64 *size);
};

#ifndef _WIN32_WCE
class CStdInFileStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
public:
  // HANDLE File;
  // CStdInFileStream() File(INVALID_HANDLE_VALUE): {}
  // void Open() { File = GetStdHandle(STD_INPUT_HANDLE); };
  MY_UNKNOWN_IMP

  virtual ~CStdInFileStream() {}
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};
#endif

class COutFileStream: 
  public IOutStream,
  public CMyUnknownImp
{
public:
  #if defined(_WIN32) || defined(ENV_UNIX)
  NWindows::NFile::NIO::COutFile File;
  #else
  NC::NFile::NIO::COutFile File;
  #endif
  virtual ~COutFileStream() {}
  bool Create(LPCTSTR fileName, bool createAlways);
  #if defined(_WIN32) || defined(ENV_UNIX)
  #ifndef _UNICODE
  bool Create(LPCWSTR fileName, bool createAlways);
  #endif
  #endif
  
  MY_UNKNOWN_IMP1(IOutStream)

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(SetSize)(Int64 newSize);
};

#ifndef _WIN32_WCE
class CStdOutFileStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  virtual ~CStdOutFileStream() {}
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};
#endif

#endif
