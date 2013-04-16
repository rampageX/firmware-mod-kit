#ifndef __PORTABLE_H
#define __PORTABLE_H

//BRCM modification
#ifdef _HOST_TOOL
#include <string.h>
#endif

#ifdef _CFE_
#include <string.h>
#endif

#ifdef __KERNEL__
#include <linux/string.h>
#endif

//bcm
//#ifdef __GNUC__   
//#include <types/vxTypesOld.h>
//#define INLINE static inline
//#else
typedef char INT8;
typedef unsigned char UINT8;
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef unsigned int UINT32;
typedef int BOOL;
#define INLINE static inline
//#define INLINE static __inline__
//#endif
typedef long long INT64;           // %%%% Changed from "long long"
typedef unsigned long long UINT64; // %%%% Changed from "long long"

typedef UINT8 BYTE;
typedef UINT16 WORD;
typedef UINT32 DWORD;

typedef unsigned UINT_PTR;
#define FALSE 0
#define TRUE 1

#define HRESULT int
#define S_OK 0
#define E_INVALIDARG -1
#define E_OUTOFMEMORY -2
#define E_FAIL -3
#define E_INTERNAL_ERROR -4
#define E_INVALIDDATA -5

#define MyMin( a, b ) ( a < b ? a : b )

#define MyMax( a, b ) ( a > b ? a : b )

#define kNumMoveBits 5

#define RETURN_IF_NOT_S_OK(x) { HRESULT __aResult_ = (x); if(__aResult_ != S_OK) return __aResult_; }

#endif
