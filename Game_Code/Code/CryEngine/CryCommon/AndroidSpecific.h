////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   AndroidSpecific.h
//  Version:     v1.00
//  Created:     24/10/2013 by Leander Beernaert.
//  Compilers:   Android NDK 
//  Description: Specific to Android declarations, inline functions etc.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _CRY_COMMON_ANDROID_SPECIFIC_HDR_
#define _CRY_COMMON_ANDROID_SPECIFIC_HDR_

#if defined(__arm__)
#define _CPU_ARM
#elif defined(__mips__)
#define _CPU_MIPS
#endif

#if defined(__ARM_NEON__)
#define _CPU_NEON
#endif

#ifndef CRY_MOBILE
#define CRY_MOBILE
#endif

#define CRY_FORCE_MALLOC_NEW_ALIGN

#define DEBUG_BREAK raise(SIGTRAP)
#define RC_EXECUTABLE "rc"
#define USE_CRT 1
#define SIZEOF_PTR 4

//////////////////////////////////////////////////////////////////////////
// Standard includes.
//////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <stdint.h>
#include <fcntl.h>
#include <float.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <sys/socket.h>
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
#include "BaseTypes.h"

typedef signed long long  INT64;

typedef double real;

typedef uint32				DWORD;
typedef DWORD*				LPDWORD;
typedef DWORD								DWORD_PTR;
typedef int INT_PTR, *PINT_PTR;
typedef unsigned int UINT_PTR, *PUINT_PTR;
typedef char *LPSTR, *PSTR;
typedef uint64      __uint64;
typedef int64       INT64;
typedef uint64      UINT64;

typedef long LONG_PTR, *PLONG_PTR, *PLONG;
typedef unsigned long ULONG_PTR, *PULONG_PTR;

typedef unsigned char				BYTE;
typedef unsigned short			WORD;
typedef void*								HWND;
typedef UINT_PTR 						WPARAM;
typedef LONG_PTR 						LPARAM;
typedef LONG_PTR 						LRESULT;
#define PLARGE_INTEGER LARGE_INTEGER*
typedef const char *LPCSTR, *PCSTR;
typedef long long						LONGLONG;
typedef	ULONG_PTR						SIZE_T;
typedef unsigned char				byte;

#define ILINE __forceinline

#define _A_RDONLY (0x01)
#define _A_SUBDIR (0x10)

//////////////////////////////////////////////////////////////////////////
// Win32 FileAttributes.
//////////////////////////////////////////////////////////////////////////
#define FILE_ATTRIBUTE_READONLY             0x00000001  
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
#define FILE_ATTRIBUTE_DEVICE               0x00000040  
#define FILE_ATTRIBUTE_NORMAL               0x00000080  
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100  
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200  
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400  
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800  
#define FILE_ATTRIBUTE_OFFLINE              0x00001000  
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000  
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000

#define INVALID_FILE_ATTRIBUTES (-1)

#define DEFINE_ALIGNED_DATA( type, name, alignment ) \
	type __attribute__ ((aligned(alignment))) name;
#define DEFINE_ALIGNED_DATA_STATIC( type, name, alignment ) \
	static type __attribute__ ((aligned(alignment))) name;
#define DEFINE_ALIGNED_DATA_CONST( type, name, alignment ) \
	const type __attribute__ ((aligned(alignment))) name;

#include "LinuxSpecific.h"
// these functions do not exist int the wchar.h header
// we'll use the implementation for the PS3 instead.
#undef wscasecomp
#undef wscasencomp
extern int wcsicmp (const wchar_t* s1, const wchar_t* s2);
extern int wcsnicmp (const wchar_t* s1, const wchar_t* s2, size_t count);

#define TARGET_DEFAULT_ALIGN (16U)

#ifdef _RELEASE
    #define __debugbreak()
#else
    #define __debugbreak() __builtin_trap()
#endif

// there is no __finite in android, only __isfinite
#undef __finite
#define __finite __isfinite

#define S_IWRITE S_IWUSR

#define ILINE __forceinline
#define _A_RDONLY (0x01)
#define _A_SUBDIR (0x10)
#define _A_HIDDEN (0x02)


// Force reading all paks from single Android OBB
// #define ANDROID_OBB

#endif //_CRY_COMMON_ANDROID_SPECIFIC_HDR_
