// Common/String.cpp

#include "StdAfx.h"

#include <ctype.h>
#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#endif
#include "StringConvert.h" // FIXED

#include "Common/String.h" // FIXED to avoid confusion with <string.h> on some filesystems

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif

#include <limits.h>
#ifndef MB_LEN_MAX
#define MB_LEN_MAX 1024
#endif

extern int global_use_utf16_conversion;

LPSTR WINAPI CharPrevA( LPCSTR start, LPCSTR ptr ) { // OK for MBS
  while (*start && (start < ptr)) {
    LPCSTR next = CharNextA( start );
    if (next >= ptr)
      break;
    start = next;
  }
  return (LPSTR)start;
}

LPSTR WINAPI CharNextA( LPCSTR ptr ) {
  if (!*ptr)
    return (LPSTR)ptr;
#ifdef HAVE_MBRTOWC
  if (global_use_utf16_conversion)
  {
    wchar_t wc;
    size_t len  = mbrtowc(&wc,ptr,MB_LEN_MAX,0); 
    if (len >= 1) return (LPSTR)(ptr + len);
    printf("INTERNAL ERROR - CharNextA\n");
    exit(EXIT_FAILURE);
  } else {
    return (LPSTR)(ptr + 1);
  }
#else
  return (LPSTR)(ptr + 1); // FIXME
#endif
}


wchar_t MyCharUpper(wchar_t c)
{
#ifdef HAVE_TOWUPPER
   return towupper(c);
#else
   int ret = c;
   if ((ret >= 1) && (ret <256)) ret = toupper(ret);
   return (wchar_t)ret;
#endif
}

wchar_t * MyStringUpper(wchar_t *s)
{
  if (s == 0)
    return 0;
  wchar_t *ret = s;
  while (*s)
  {
   *s = MyCharUpper(*s);
    s++;
  }
  return ret;
}

int MyStringCompare(const char *s1, const char *s2)
{ 
  while (true)
  {
    unsigned char c1 = (unsigned char)*s1++;
    unsigned char c2 = (unsigned char)*s2++;
    if (c1 < c2) return -1;
    if (c1 > c2) return 1;
    if (c1 == 0) return 0;
  }
}

int MyStringCompare(const wchar_t *s1, const wchar_t *s2)
{ 
  while (true)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 < c2) return -1;
    if (c1 > c2) return 1;
    if (c1 == 0) return 0;
  }
}

int MyStringCompareNoCase(const wchar_t *s1, const wchar_t *s2)
{ 
  while (true)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2)
    {
      wchar_t u1 = MyCharUpper(c1);
      wchar_t u2 = MyCharUpper(c2);
      if (u1 < u2) return -1;
      if (u1 > u2) return 1;
    }
    if (c1 == 0) return 0;
  }
}

int MyStringCompareNoCase(const char *s1, const char *s2)
{
  UString us1 = MultiByteToUnicodeString(s1, 0);
  UString us2 = MultiByteToUnicodeString(s2, 0);

  return MyStringCompareNoCase(us1,us2);
}

