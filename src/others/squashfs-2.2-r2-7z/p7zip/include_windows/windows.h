/*
	windows.h - main header file for the Win32 API

	Written by Anders Norlander <anorland@hem2.passagen.se>

	This file is part of a free library for the Win32 API.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/
#ifndef _WINDOWS_H
#define _WINDOWS_H

#include <stdarg.h>
#include <windef.h>
/* BEGIN #include <wincon.h> */
  #ifdef __cplusplus
  extern "C" {
  #endif
  typedef BOOL(*PHANDLER_ROUTINE)(DWORD);
  BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE,BOOL);
  #ifdef __cplusplus
  }
  #endif
/* END #include <wincon.h> */


/* BEGIN #include <winbase.h> */
#define INVALID_HANDLE_VALUE (HANDLE)(-1)

#define FILE_BEGIN	SEEK_SET
#define FILE_CURRENT	SEEK_CUR
#define FILE_END	SEEK_END
#define INVALID_SET_FILE_POINTER	((DWORD)-1)

#define INVALID_FILE_SIZE 0xFFFFFFFF

#define WAIT_OBJECT_0 0
#define INFINITE	0xFFFFFFFF

typedef FILETIME *LPFILETIME;

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME,*LPSYSTEMTIME;

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI DosDateTimeToFileTime(WORD,WORD,FILETIME *);
BOOL WINAPI FileTimeToDosDateTime(CONST FILETIME *,WORD *, WORD *);
BOOL WINAPI FileTimeToLocalFileTime(CONST FILETIME *,FILETIME *);
BOOL WINAPI FileTimeToSystemTime(CONST FILETIME *,LPSYSTEMTIME);
BOOL WINAPI LocalFileTimeToFileTime(CONST FILETIME *,FILETIME *);
VOID WINAPI GetSystemTime(LPSYSTEMTIME);
BOOL WINAPI SystemTimeToFileTime(const SYSTEMTIME*,LPFILETIME);

BOOL WINAPI SetFileAttributesA(LPCSTR,DWORD);
BOOL WINAPI SetFileAttributesW(LPCWSTR,DWORD);

DWORD WINAPI SearchPathA(LPCSTR,LPCSTR,LPCSTR,DWORD,LPSTR,LPSTR*);
DWORD WINAPI SearchPathW(LPCWSTR,LPCWSTR,LPCWSTR,DWORD,LPWSTR,LPWSTR*);

#define lstrcatA strcat /* OK for MBS */

#define MoveMemory memmove

DWORD WINAPI GetTickCount(VOID);


#ifdef UNICODE
#define SearchPath SearchPathW
#define SetFileAttributes SetFileAttributesW
#else
#define SearchPath SearchPathA
#define SetFileAttributes SetFileAttributesA
#define lstrlen strlen  /* OK for MBS */
#define lstrlenA strlen  /* OK for MBS */
#endif

#ifdef __cplusplus
}
#endif
/* END #include <winbase.h> */

/* BEGIN #include <winnls.h> */
#define NORM_IGNORECASE	1

#define CP_ACP 0
#define CP_OEMCP 1
#define CP_UTF8 65001

#define LOCALE_USER_DEFAULT	0x400
#define SORT_STRINGSORT	4096

/* #include <unknwn.h> */
#include <basetyps.h>
struct IEnumSTATPROPSTG;

typedef WCHAR OLECHAR;
typedef LPWSTR LPOLESTR;
typedef LPCWSTR LPCOLESTR;
#define OLESTR(s) L##s

typedef unsigned short VARTYPE;
typedef short VARIANT_BOOL;
typedef VARIANT_BOOL _VARIANT_BOOL;
/*
#define VARIANT_TRUE ((VARIANT_BOOL)0xffff)
#define VARIANT_FALSE ((VARIANT_BOOL)0)
*/
typedef OLECHAR *BSTR;

void CoInitialize(void *);
void CoUninitialize(void);

typedef struct  tagSTATPROPSTG {
	LPOLESTR lpwstrName;
	PROPID propid;
	VARTYPE vt;
} STATPROPSTG;

EXTERN_C const IID IID_ISequentialStream;
#undef INTERFACE
#define INTERFACE ISequentialStream
DECLARE_INTERFACE_(ISequentialStream,IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(Read)(THIS_ void*,ULONG,ULONG*) PURE;
	STDMETHOD(Write)(THIS_ void const*,ULONG,ULONG*) PURE;
};


/* END #include <ole2.h> */

#include <stddef.h>

#endif

