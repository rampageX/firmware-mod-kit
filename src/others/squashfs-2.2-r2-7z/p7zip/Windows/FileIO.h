// Windows/FileIO.h

#ifndef __WINDOWS_FILEIO_H
#define __WINDOWS_FILEIO_H

#include <Common/String.h>

namespace NWindows {
namespace NFile {
namespace NIO {

class CFileBase
{
protected:
  int     _fd;
  AString _unix_filename;
  time_t   _lastAccessTime;
  time_t   _lastWriteTime;
#ifdef HAVE_LSTAT
  int     _size;
  char    _buffer[MAX_PATHNAME_LEN+1];
  int     _offset;
#endif

  bool Create(LPCTSTR fileName, DWORD desiredAccess,
      DWORD shareMode, DWORD creationDisposition,  DWORD flagsAndAttributes,bool ignoreSymbolicLink=false);
  #ifndef _UNICODE
  bool Create(LPCWSTR fileName, DWORD desiredAccess,
      DWORD shareMode, DWORD creationDisposition,  DWORD flagsAndAttributes,bool ignoreSymbolicLink=false);
  #endif

public:
  CFileBase():
    _fd(-1){};
  virtual ~CFileBase();

  virtual bool Close();

  bool GetLength(UINT64 &length) const;

  bool Seek(INT64 distanceToMove, DWORD moveMethod, UINT64 &newPosition);
  bool Seek(UINT64 position, UINT64 &newPosition); 
};

class CInFile: public CFileBase
{
public:
  bool Open(LPCTSTR fileName, DWORD shareMode, 
      DWORD creationDisposition,  DWORD flagsAndAttributes);
  bool Open(LPCTSTR fileName,bool ignoreSymbolicLink=false);
  #ifndef _UNICODE
  bool Open(LPCWSTR fileName, DWORD shareMode, 
      DWORD creationDisposition,  DWORD flagsAndAttributes);
  bool Open(LPCWSTR fileName,bool ignoreSymbolicLink=false);
  #endif
  bool ReadPart(void *data, UINT32 size, UINT32 &processedSize);
  bool Read(void *data, UINT32 size, UINT32 &processedSize);
};

class COutFile: public CFileBase
{
public:
  bool Open(LPCTSTR fileName, DWORD shareMode, 
      DWORD creationDisposition, DWORD flagsAndAttributes);
  bool Open(LPCTSTR fileName, DWORD creationDisposition);
  bool Create(LPCTSTR fileName, bool createAlways);

  #ifndef _UNICODE
  bool Open(LPCWSTR fileName, DWORD shareMode, 
      DWORD creationDisposition, DWORD flagsAndAttributes);
  bool Open(LPCWSTR fileName, DWORD creationDisposition);
  bool Create(LPCWSTR fileName, bool createAlways);
  #endif

  bool SetTime(const FILETIME *creationTime,
      const FILETIME *lastAccessTime, const FILETIME *lastWriteTime);
  bool SetLastWriteTime(const FILETIME *lastWriteTime);
  bool WritePart(const void *data, UINT32 size, UINT32 &processedSize);
  bool Write(const void *data, UINT32 size, UINT32 &processedSize);
  bool SetEndOfFile();
  bool SetLength(UINT64 length);
};

}}}

#endif
