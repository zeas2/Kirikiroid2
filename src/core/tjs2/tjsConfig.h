//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// configuration
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifndef tjsConfigH
#define tjsConfigH
#include "stdarg.h"
#include <wchar.h>

namespace TJS
{
//---------------------------------------------------------------------------


/*
	many settings can be changed here.

	tjsCommHead.h includes most common headers that will be needed to
	compile the entire TJS program.

	configuration about Critical Section for multithreading support is there in
	tjsUtils.cpp/h.
*/

// TODO: autoconf integration

#include "tjsTypes.h"


// #define TJS_NO_AVOID_ISWDIGIT
// #define TJS_SUPPORT_VCL
// #define TJS_MSG_EMPTY
// #define TJS_DEBUG_TRACE
// #define TJS_JP_LOCALIZED
// #define TJS_TEXT_OUT_CRLF
// #define TJS_WITH_IS_NOT_RESERVED_WORD

TJS_EXP_FUNC_DEF(tjs_int, TJS_atoi, (const tjs_char *s));
TJS_EXP_FUNC_DEF(tjs_char *, TJS_int_to_str, (tjs_int value, tjs_char *string));
TJS_EXP_FUNC_DEF(tjs_char *, TJS_tTVInt_to_str, (tjs_int64 value, tjs_char *string));
TJS_EXP_FUNC_DEF(tjs_int, TJS_strnicmp, (const tjs_char *s1, const tjs_char *s2, size_t maxlen));
TJS_EXP_FUNC_DEF(tjs_int, TJS_stricmp, (const tjs_char *s1, const tjs_char *s2));
TJS_EXP_FUNC_DEF(void, TJS_strcpy_maxlen, (tjs_char *d, const tjs_char *s, size_t len));
TJS_EXP_FUNC_DEF(void, TJS_strcpy, (tjs_char *d, const tjs_char *s));
TJS_EXP_FUNC_DEF(size_t, TJS_strlen, (const tjs_char *d));
int TJS_strcmp (const tjs_char * src, const tjs_char * dst);
int TJS_strncmp (const tjs_char * first, const tjs_char * last, size_t count);
tjs_char * TJS_strncpy (tjs_char * dest, const tjs_char * source, size_t count);
tjs_char *  TJS_strcat (tjs_char * dst, const tjs_char * src);
tjs_char * TJS_strstr (const tjs_char * wcs1, const tjs_char * wcs2);
tjs_char * TJS_strchr (const tjs_char * string, tjs_char ch);
void *TJS_malloc(size_t n);
void *TJS_realloc(void* orig, size_t n);
void TJS_free(void *p);
#define TJS_nsprintf		sprintf
#define TJS_nstrcpy			strcpy
#define TJS_nstrcat			strcat
#define TJS_nstrlen			strlen
#define TJS_nstrstr			strstr
size_t TJS_strftime (tjs_char *wstring, size_t maxsize, const tjs_char *wformat, const tm *timeptr);
int TJS_vsnprintf (tjs_char *string, size_t count, const tjs_char *format, va_list ap);
#define TJS_vfprintf		vfwprintf
#define TJS_octetcpy		memcpy
#define TJS_octetcmp		memcmp
double TJS_strtod(const tjs_char* pString, tjs_char** ppEnd);
TJS_EXP_FUNC_DEF(tjs_int64, TJS_atoll, (const tjs_char *s));
tjs_char *TJS_strrchr(const tjs_char *s, int c);

extern size_t TJS_mbstowcs(tjs_char *pwcs, const tjs_nchar *s, size_t n);
extern size_t TJS_wcstombs(tjs_nchar *s, const tjs_char *pwcs, size_t n);
extern int TJS_mbtowc(tjs_char *pwc, const tjs_nchar *s, size_t n);
extern int TJS_wctomb(tjs_nchar *s, tjs_char wc);
extern tjs_int TJS_sprintf(tjs_char *s, const tjs_char *format, ...);

#define TJS_strncpy_s(d, dl, s, sl)		TJS_strncpy(d, s, sl)
#if defined(_MSC_VER)
	#define TJS_cdecl __cdecl
	#define TJS_timezone _timezone
#else
	#define TJS_cdecl
	#define TJS_timezone timezone
#endif
extern tjs_int TJS_snprintf(tjs_char *s, size_t count, const tjs_char *format, ...);

#define TJS_narrowtowidelen(X) TJS_mbstowcs(NULL, (X),0) // narrow->wide (if) converted length
#define TJS_narrowtowide TJS_mbstowcs

void TJS_debug_out(const tjs_char *format, ...);

#ifdef TJS_DEBUG_TRACE
#define TJS_D(x)	TJS_debug_out x;
#define TJS_F_TRACE(x) tTJSFuncTrace ___trace(TJS_W(x));
#else
#define TJS_D(x)
#define TJS_F_TRACE(x)
#endif


#ifdef _MSC_VER
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "
#endif

extern void TJSNativeDebuggerBreak();


extern void TJSSetFPUE();
extern void TJSRestoreFPUE();


//---------------------------------------------------------------------------
// elapsed time profiler
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
extern tjs_uint TJSGetTickCount();
class tTJSTimeProfiler
{
	tjs_uint & timevar;
	tjs_uint start;
public:
	tTJSTimeProfiler(tjs_uint & tv) : timevar(tv)
	{
		start = TJSGetTickCount();
	}

	~tTJSTimeProfiler()
	{
		timevar += TJSGetTickCount() - start;
	}
};
#endif
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// function tracer
//---------------------------------------------------------------------------
class tTJSFuncTrace
{
	tjs_char *funcname;
public:
	tTJSFuncTrace(tjs_char *p)
	{
		funcname = p;
		TJS_debug_out(TJS_W("enter: %ls\n"), funcname);
	}
	~tTJSFuncTrace()
	{
		TJS_debug_out(TJS_W("exit: %ls\n"), funcname);
	}
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNarrowStringHolder : converts wide -> narrow, and holds it until be destroyed
//---------------------------------------------------------------------------
struct tTJSNarrowStringHolder
{
	bool Allocated;
	tjs_nchar *Buf;
public:
	tTJSNarrowStringHolder(const tjs_char *wide);

	~tTJSNarrowStringHolder(void);

	operator const tjs_nchar *()
	{
		return Buf;
	}
};
//---------------------------------------------------------------------------

}


#endif

