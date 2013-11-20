// stdafx.h

#ifndef __STDAFX_H
#define __STDAFX_H

#include "config.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include "Common/MyWindows.h"
#include "Common/Types.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#if !defined(__UCLIBC__) || defined(__UCLIBC_HAS_WCHAR__)
#include <wchar.h>
#endif
#include <stddef.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

/***************************/
typedef void * HINSTANCE; // FIXME
typedef void * HMODULE; // FIXME

// FIXME
#define lstrcpy strcpy
#define lstrcat strcat

typedef int (WINAPI *FARPROC)();
#define CLASS_E_CLASSNOTAVAILABLE        ((HRESULT)0x80040111L)
#define DLL_PROCESS_ATTACH   1

DWORD GetModuleFileNameA( HMODULE hModule, LPSTR lpFilename, DWORD nSize);
#define GetModuleFileName  GetModuleFileNameA
extern "C" void mySetModuleFileNameA(const char * moduleFileName);

/************************* FILES *************************/

#define FILE_SHARE_READ	1
#define GENERIC_READ	0x80000000
#define GENERIC_WRITE	0x40000000

#define CREATE_NEW	  1
#define CREATE_ALWAYS	  2
#define OPEN_EXISTING	  3
#define OPEN_ALWAYS	  4
/* #define TRUNCATE_EXISTING 5 */

/************************* EVENTS *************************/

DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE *handles, BOOL wait_all, DWORD timeout );

/************************* OTHERS *************************/
extern int global_use_utf16_conversion;
const char *my_getlocale(void);

/************************* LastError *************************/
inline DWORD WINAPI GetLastError(void) { return errno; }
inline void WINAPI SetLastError( DWORD err ) { errno = err; }

#define AreFileApisANSI() 1

#endif 

