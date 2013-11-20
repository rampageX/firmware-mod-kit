/* 
 * tchar.h
 *
 * Unicode mapping layer for the standard C library. By including this
 * file and using the 't' names for string functions
 * (eg. _tprintf) you can make code which can be easily adapted to both
 * Unicode and non-unicode environments. In a unicode enabled compile define
 * _UNICODE before including tchar.h, otherwise the standard non-unicode
 * library functions will be used.
 *
 * Note that you still need to include string.h or stdlib.h etc. to define
 * the appropriate functions. Also note that there are several defines
 * included for non-ANSI functions which are commonly available (but using
 * the convention of prepending an underscore to non-ANSI library function
 * names).
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 3308 $
 * $Author: vapier $
 * $Date: 2006-08-05 15:58:49 +0800 (Sat, 05 Aug 2006) $
 *
 */

#ifndef	_TCHAR_H_
#define _TCHAR_H_

/* All the headers include this file. */
#ifndef __int64
#define __int64 long long
#endif

#ifndef __cdecl
#define __cdecl /* */
#endif


/*
 * NOTE: This tests _UNICODE, which is different from the UNICODE define
 *       used to differentiate Win32 API calls.
 */
#ifdef	_UNICODE


/*
 * Use TCHAR instead of char or wchar_t. It will be appropriately translated
 * if _UNICODE is correctly defined (or not).
 */
#ifndef _TCHAR_DEFINED
typedef	wchar_t	TCHAR;
#define _TCHAR_DEFINED
#endif


/*
 * __TEXT is a private macro whose specific use is to force the expansion of a
 * macro passed as an argument to the macros _T or _TEXT.  DO NOT use this
 * macro within your programs.  It's name and function could change without
 * notice.
 */
#define	__TEXT(q)	L##q

/*
 * Unicode functions
 */
#define _tfopen     _wfopen
FILE *_wfopen( const wchar_t *filename, const wchar_t *mode );


#else	/* Not _UNICODE */

#define _tfopen     fopen

/*
 * TCHAR, the type you should use instead of char.
 */
#ifndef _TCHAR_DEFINED
typedef char	TCHAR;
#define _TCHAR_DEFINED
#endif

/*
 * __TEXT is a private macro whose specific use is to force the expansion of a
 * macro passed as an argument to the macros _T or _TEXT.  DO NOT use this
 * macro within your programs.  It's name and function could change without
 * notice.
 */
#define	__TEXT(q)	q

/*
 * Non-unicode (standard) functions
 */

#endif	/* Not _UNICODE */

/*
 * UNICODE a constant string when _UNICODE is defined else returns the string
 * unmodified.  Also defined in w32api/winnt.h.
 */
#define _TEXT(x)	__TEXT(x)
/* #define	_T(x)		__TEXT(x) */

#endif	/* Not _TCHAR_H_ */

