#ifndef _WINDEF_H
#define _WINDEF_H

#include "Common/MyWindows.h" // FIXED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONST
#define CONST const
#endif

#undef MAX_PATH
#define MAX_PATH 260

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define WINAPI 

typedef int WINBOOL;
typedef WINBOOL BOOL;

#include <winnt.h>

#ifdef __cplusplus
}
#endif
#endif
