#pragma once
#ifndef _BASETSD_H_
#ifndef MAX_PATH
#define MAX_PATH  260
#endif
#include "tjsTypes.h"
#include <stdint.h>

/* public enums and structures that GDI+ reuse from the other Windows API */

#define LF_FACESIZE		32

/* SetBkMode */
//#define TRANSPARENT		1
//#define OPAQUE			2

/* SetMapMode */
#define MM_TEXT			1
#define MM_LOMETRIC		2
#define MM_HIMETRIC		3
#define MM_LOENGLISH		4
#define MM_HIENGLISH		5
#define MM_TWIPS		6
#define MM_ISOTROPIC		7
#define MM_ANISOTROPIC		8

/* CreatePenIndirect */
#define PS_NULL			0x00000005
#define PS_STYLE_MASK		0x0000000F
#define PS_ENDCAP_ROUND		0x00000000
#define PS_ENDCAP_SQUARE	0x00000100
#define PS_ENDCAP_FLAT		0x00000200
#define PS_ENDCAP_MASK		0x00000F00
#define PS_JOIN_ROUND		0x00000000
#define PS_JOIN_BEVEL		0x00001000
#define PS_JOIN_MITER		0x00002000
#define PS_JOIN_MASK		0x0000F000

/* CreateBrushIndirect */
#define BS_SOLID		0
#define BS_NULL			1
#define BS_HATCHED		2
#define BS_PATTERN		3
#define BS_INDEXED		4

/* SetPolyFillMode */
//#define ALTERNATE		1
//#define WINDING			2

/* SetRelabs */
//#define ABSOLUTE		1
//#define RELATIVE		2

/* ModifyWorldTransform */
#define MWT_IDENTITY		1
#define MWT_LEFTMULTIPLY	2
#define MWT_RIGHTMULTIPLY	3

typedef int LANGID;
typedef int INT;
typedef uint32_t UINT;
typedef uint32_t ARGB;
typedef uint32_t UINT32;
typedef int32_t PROPID;
//typedef uint32_t ULONG_PTR; /* not a pointer! */
typedef float REAL;

typedef void* HBITMAP;
typedef void* HDC;
typedef void* HENHMETAFILE;
typedef void* HFONT;
typedef void* HICON;
typedef void* HINSTANCE;
typedef void* HMETAFILE;
typedef void* HPALETTE;

/* mono/io-layer/uglify.h also has these typedefs.
 * To avoid a dependency on mono we have copied all
 * the required stuff here. We don't include our defs
 * if uglify.h is included somehow.
 */
//#ifndef _WAPI_UGLIFY_H_		/* to avoid conflict with uglify.h */

//typedef const tjs_char *LPCTSTR;
//typedef tjs_char *LPTSTR;
typedef uint8_t BYTE;
typedef uint8_t *LPBYTE;
typedef uint16_t WORD;
#ifdef _MSC_VER
#define DWORD uint32_t
#else
typedef uint32_t DWORD;
#endif
typedef void* PVOID;
typedef void* LPVOID;
typedef uint32_t *LPDWORD;
typedef int16_t SHORT;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int32_t *PLONG;
typedef int64_t LONGLONG;
//typedef tjs_char TCHAR;
typedef size_t SIZE_T;

typedef void* HANDLE;
typedef void* *LPHANDLE;
typedef uint32_t SOCKET;
typedef void* HMODULE;
typedef void* HRGN;
#define CONST const
#define VOID void

#define IN
#define OUT
//#define WINAPI

typedef unsigned char BYTE;
typedef unsigned char *LPBYTE;
typedef unsigned short WORD;
typedef unsigned short UINT16;
typedef short INT16;
typedef unsigned int  UINT_PTR;
typedef unsigned int  UINT;
typedef unsigned int UINT32;
typedef float REAL;
typedef int                 INT;
typedef signed int INT32;
//typedef long long LONGLONG;
typedef uint64_t ULONGLONG;
typedef tjs_char *LPWSTR;
typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER;
typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER;
typedef char CHAR;
typedef CHAR *NPSTR, *LPSTR, *PSTR;
typedef const CHAR *LPCSTR;
typedef struct _FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} 	FILETIME;
typedef void *HWND;
#define TYPE_RECT_DEFINED
typedef struct {
	int	left;
	int	top;
	int	right;
	int	bottom;
} RECT, RECTL;
typedef intptr_t			LONG_PTR;
typedef LONG HRESULT;
#define TYPE_GUID_DEFINED
typedef struct {
	DWORD Data1;
	WORD  Data2;
	WORD  Data3;
	BYTE  Data4[8];
} GUID, Guid, CLSID;
#endif