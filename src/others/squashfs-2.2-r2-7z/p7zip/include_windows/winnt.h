#ifndef _WINNT_H
#define _WINNT_H

#ifdef __cplusplus
extern "C" {
#endif

/* BEGIN <winerror.h> */
#define NO_ERROR                    0L
#define ERROR_ALREADY_EXISTS        EEXIST
#define ERROR_FILE_EXISTS           EEXIST
#define ERROR_INVALID_HANDLE        EBADF
#define ERROR_PATH_NOT_FOUND        ENOENT
#define ERROR_FILENAME_EXCED_RANGE  ENAMETOOLONG
#define ERROR_DISK_FULL             ENOSPC
#define ERROR_NO_MORE_FILES         0x100123 // FIXME

/* see Common/WyWindows.h
#define S_OK ((HRESULT)0x00000000L)
#define S_FALSE ((HRESULT)0x00000001L)
#define E_INVALIDARG ((HRESULT)0x80070057L)
#define E_NOTIMPL ((HRESULT)0x80004001L)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_ABORT ((HRESULT)0x80004004L)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define STG_E_INVALIDFUNCTION ((HRESULT)0x80030001L)
#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#define FAILED(Status) ((HRESULT)(Status)<0)
*/

/* END   <winerror.h> */

#include <string.h>
#include <stddef.h>
/* #include <wchar.h> */

#ifdef __cplusplus
}
#endif

#ifndef VOID
#define VOID void
#endif
typedef void *PVOID,*LPVOID;
typedef WCHAR *PWCHAR,*LPWCH,*PWCH,*NWPSTR,*LPWSTR,*PWSTR;
typedef CHAR *PCHAR,*LPCH,*PCH,*NPSTR,*LPSTR,*PSTR;
typedef TCHAR *LPTCH,*PTSTR,*LPTSTR,*LP,*PTCHAR;

#ifdef UNICODE
/*
 * __TEXT is a private macro whose specific use is to force the expansion of a
 * macro passed as an argument to the macro TEXT.  DO NOT use this
 * macro within your programs.  It's name and function could change without
 * notice.
 */
#define __TEXT(q) L##q
#else
#define __TEXT(q) q
#endif
/*
 * UNICODE a constant string when UNICODE is defined, else returns the string
 * unmodified.
 * The corresponding macros  _TEXT() and _T() for mapping _UNICODE strings
 * passed to C runtime functions are defined in mingw/tchar.h
 */
#define TEXT(q) __TEXT(q)    
typedef void *HANDLE;

typedef BYTE BOOLEAN;

/* BEGIN #include <basetsd.h> */
#ifndef __int64
#define __int64 long long
#endif
typedef unsigned __int64 UINT64;
typedef __int64 INT64;
/* END #include <basetsd.h> */

#define FILE_ATTRIBUTE_READONLY   1
#define FILE_ATTRIBUTE_HIDDEN	2
#define FILE_ATTRIBUTE_SYSTEM	4
#define FILE_ATTRIBUTE_DIRECTORY	16
#define FILE_ATTRIBUTE_ARCHIVE	32
#define FILE_ATTRIBUTE_DEVICE	64
#define FILE_ATTRIBUTE_NORMAL	128
#define FILE_ATTRIBUTE_TEMPORARY	256
#define FILE_ATTRIBUTE_SPARSE_FILE	512
#define FILE_ATTRIBUTE_REPARSE_POINT	1024
#define FILE_ATTRIBUTE_COMPRESSED	2048
#define FILE_ATTRIBUTE_OFFLINE	0x1000
#define FILE_ATTRIBUTE_ENCRYPTED	0x4000
#define FILE_ATTRIBUTE_UNIX_EXTENSION	0x8000   /* trick for Unix */
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)

#endif

