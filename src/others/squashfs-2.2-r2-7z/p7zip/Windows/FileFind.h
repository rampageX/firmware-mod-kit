// Windows/FileFind.h

#ifndef __WINDOWS_FILEFIND_H
#define __WINDOWS_FILEFIND_H

#include "../Common/String.h"
#include "FileName.h"
#include "Defs.h"

#include <sys/types.h> /* for DIR */
#include <dirent.h>

namespace NWindows {
namespace NFile {
namespace NFind {

namespace NAttributes
{
  inline bool IsReadOnly(DWORD attributes) { return (attributes & FILE_ATTRIBUTE_READONLY) != 0; }
  inline bool IsHidden(DWORD attributes) { return (attributes & FILE_ATTRIBUTE_HIDDEN) != 0; }
  inline bool IsSystem(DWORD attributes) { return (attributes & FILE_ATTRIBUTE_SYSTEM) != 0; }
  inline bool IsDirectory(DWORD attributes) { return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
  inline bool IsArchived(DWORD attributes) { return (attributes & FILE_ATTRIBUTE_ARCHIVE) != 0; }
  inline bool IsCompressed(DWORD attributes) { return (attributes & FILE_ATTRIBUTE_COMPRESSED) != 0; }
  inline bool IsEncrypted(DWORD attributes) { return (attributes & FILE_ATTRIBUTE_ENCRYPTED) != 0; }
}

class CFileInfoBase
{ 
  bool MatchesMask(UINT32 mask) const  { return ((Attributes & mask) != 0); }
public:
  DWORD Attributes;
  FILETIME CreationTime;  
  FILETIME LastAccessTime; 
  FILETIME LastWriteTime;
  UINT64 Size;
  
  bool IsDirectory() const { return MatchesMask(FILE_ATTRIBUTE_DIRECTORY); }
};

class CFileInfo: public CFileInfoBase
{ 
public:
  CSysString Name;
  bool IsDots() const;
};

#ifdef _UNICODE
typedef CFileInfo CFileInfoW;
#else
class CFileInfoW: public CFileInfoBase
{ 
public:
  UString Name;
  bool IsDots() const;
};
#endif

class CFindFile
{
  friend class CEnumerator;
  HANDLE _handle;
  DIR *_dirp;
  AString _pattern;
  AString _directory;  
public:
  bool IsHandleAllocated() const { return (_dirp != 0); }
  CFindFile(): _dirp(0) {}
  ~CFindFile() {  Close(); }
  bool FindFirst(LPCTSTR wildcard, CFileInfo &fileInfo);
  bool FindNext(CFileInfo &fileInfo);
  #ifndef _UNICODE
  bool FindFirst(LPCWSTR wildcard, CFileInfoW &fileInfo);
  bool FindNext(CFileInfoW &fileInfo);
  #endif
  bool Close();
};

bool FindFile(LPCTSTR wildcard, CFileInfo &fileInfo);

bool DoesFileExist(LPCTSTR name);
#ifndef _UNICODE
bool FindFile(LPCWSTR wildcard, CFileInfoW &fileInfo);
bool DoesFileExist(LPCWSTR name);
#endif

class CEnumerator
{
  CFindFile _findFile;
  CSysString _wildcard;
  bool NextAny(CFileInfo &fileInfo);
public:
  CEnumerator(): _wildcard(NName::kAnyStringWildcard) {}
  CEnumerator(const CSysString &wildcard): _wildcard(wildcard) {}
  bool Next(CFileInfo &fileInfo);
  bool Next(CFileInfo &fileInfo, bool &found);
};

#ifdef _UNICODE
typedef CEnumerator CEnumeratorW;
#else
class CEnumeratorW
{
  CFindFile _findFile;
  UString _wildcard;
  bool NextAny(CFileInfoW &fileInfo);
public:
  CEnumeratorW(): _wildcard(NName::kAnyStringWildcard) {}
  CEnumeratorW(const UString &wildcard): _wildcard(wildcard) {}
  bool Next(CFileInfoW &fileInfo);
  bool Next(CFileInfoW &fileInfo, bool &found);
};
#endif

}}}

#endif

