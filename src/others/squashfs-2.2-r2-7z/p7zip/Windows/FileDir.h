// Windows/FileDir.h

#ifndef __WINDOWS_FILEDIR_H
#define __WINDOWS_FILEDIR_H

#include "../Common/String.h"
#include "Defs.h"

/* GetFullPathName for 7zAES.cpp */
DWORD WINAPI GetFullPathName( LPCSTR name, DWORD len, LPSTR buffer, LPSTR *lastpart );

namespace NWindows {
namespace NFile {
namespace NDirectory {

bool MySetFileAttributes(LPCTSTR fileName, DWORD fileAttributes);
#ifndef _UNICODE
bool MySetFileAttributes(LPCWSTR fileName, DWORD fileAttributes);
#endif

bool MyMoveFile(LPCTSTR existFileName, LPCTSTR newFileName);
#ifndef _UNICODE
bool MyMoveFile(LPCWSTR existFileName, LPCWSTR newFileName);
#endif

bool MyRemoveDirectory(LPCTSTR pathName);
#ifndef _UNICODE
bool MyRemoveDirectory(LPCWSTR pathName);
#endif

bool MyCreateDirectory(LPCTSTR pathName);
bool CreateComplexDirectory(LPCTSTR pathName);
#ifndef _UNICODE
bool MyCreateDirectory(LPCWSTR pathName);
bool CreateComplexDirectory(LPCWSTR pathName);
#endif

bool DeleteFileAlways(LPCTSTR name);
#ifndef _UNICODE
bool DeleteFileAlways(LPCWSTR name);
#endif

bool RemoveDirectoryWithSubItems(const CSysString &path);
#ifndef _UNICODE
bool RemoveDirectoryWithSubItems(const UString &path);
#endif

#ifndef _WIN32_WCE
bool MyGetFullPathName(LPCTSTR fileName, CSysString &resultPath, 
    int &fileNamePartStartIndex);
bool MyGetFullPathName(LPCTSTR fileName, CSysString &resultPath);
bool GetOnlyName(LPCTSTR fileName, CSysString &resultName);
bool GetOnlyDirPrefix(LPCTSTR fileName, CSysString &resultName);
#ifndef _UNICODE
bool MyGetFullPathName(LPCWSTR fileName, UString &resultPath, 
    int &fileNamePartStartIndex);
bool MyGetFullPathName(LPCWSTR fileName, UString &resultPath);
bool GetOnlyName(LPCWSTR fileName, UString &resultName);
bool GetOnlyDirPrefix(LPCWSTR fileName, UString &resultName);
#endif

#endif

bool MySearchPath(LPCTSTR path, LPCTSTR fileName, LPCTSTR extension, 
  CSysString &resultPath, UINT32 &filePart);
#ifndef _UNICODE
bool MySearchPath(LPCWSTR path, LPCWSTR fileName, LPCWSTR extension, 
  UString &resultPath, UINT32 &filePart);
#endif

inline bool MySearchPath(LPCTSTR path, LPCTSTR fileName, LPCTSTR extension, 
  CSysString &resultPath)
{
  UINT32 value;
  return MySearchPath(path, fileName, extension, resultPath, value);
}

#ifndef _UNICODE
inline bool MySearchPath(LPCWSTR path, LPCWSTR fileName, LPCWSTR extension, 
  UString &resultPath)
{
  UINT32 value;
  return MySearchPath(path, fileName, extension, resultPath, value);
}
#endif

bool MyGetTempPath(CSysString &resultPath);
#ifndef _UNICODE
bool MyGetTempPath(UString &resultPath);
#endif

UINT MyGetTempFileName(LPCTSTR dirPath, LPCTSTR prefix, CSysString &resultPath);
#ifndef _UNICODE
UINT MyGetTempFileName(LPCWSTR dirPath, LPCWSTR prefix, UString &resultPath);
#endif

class CTempFile
{
  bool _mustBeDeleted;
  CSysString _fileName;
public:
  CTempFile(): _mustBeDeleted(false) {}
  ~CTempFile() { Remove(); }
  void DisableDeleting() { _mustBeDeleted = false; }
  UINT Create(LPCTSTR dirPath, LPCTSTR prefix, CSysString &resultPath);
  bool Create(LPCTSTR prefix, CSysString &resultPath);
  bool Remove();
};

#ifdef _UNICODE
typedef CTempFile CTempFileW;
#else
class CTempFileW
{
  bool _mustBeDeleted;
  UString _fileName;
public:
  CTempFileW(): _mustBeDeleted(false) {}
  ~CTempFileW() { Remove(); }
  void DisableDeleting() { _mustBeDeleted = false; }
  UINT Create(LPCWSTR dirPath, LPCWSTR prefix, UString &resultPath);
  bool Create(LPCWSTR prefix, UString &resultPath);
  bool Remove();
};
#endif

}}}

#endif
