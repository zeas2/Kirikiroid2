//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// configuration
//---------------------------------------------------------------------------


#include "tjsCommHead.h"
#include <errno.h>
#include <clocale>
#include <algorithm>
#ifdef __WIN32__
#include <float.h>
#define isfinite _finite
#else
#define isfinite std::isfinite
#endif
#define INTMAX_MAX		0x7fffffffffffffff
#include <assert.h>

/*
 * core/utils/cp932_uni.cpp
 * core/utils/uni_cp932.cpp
 * を一緒にリンクしてください。
 * CP932(ShiftJIS) と Unicode 変換に使用しています。
 * Win32 APIの同等の関数は互換性等の問題があることやマルチプラットフォームの足かせとなる
 * ため使用が中止されました。
 */
#if 0
extern tjs_size SJISToUnicodeString(const char * in, tjs_char *out);
extern tjs_size SJISToUnicodeString(const char * in, tjs_char *out, tjs_size limit );
extern bool IsSJISLeadByte( tjs_nchar b );
extern tjs_uint UnicodeToSJIS(tjs_char in);
extern tjs_size UnicodeToSJISString(const tjs_char *in, tjs_nchar* out );
extern tjs_size UnicodeToSJISString(const tjs_char *in, tjs_nchar* out, tjs_size limit );
#endif

static int
utf8_mbtowc(/*conv_t conv,*/ tjs_char *pwc, const unsigned char *s, int n)
{
	unsigned char c = s[0];

	if (c < 0x80) {
		*pwc = c;
		return 1;
	} else if (c < 0xc2) {
		return -1;
	} else if (c < 0xe0) {
		if (n < 2)
			return -1;
		if (!((s[1] ^ 0x80) < 0x40))
			return -1;
		*pwc = ((tjs_char)(c & 0x1f) << 6)
			| (tjs_char)(s[1] ^ 0x80);
		return 2;
	} else if (c < 0xf0) {
		if (n < 3)
			return -1;
		if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
			&& (c >= 0xe1 || s[1] >= 0xa0)))
			return -1;
		*pwc = ((tjs_char)(c & 0x0f) << 12)
			| ((tjs_char)(s[1] ^ 0x80) << 6)
			| (tjs_char)(s[2] ^ 0x80);
		return 3;
	} else
		return -1;
}

static int
utf8_wctomb(/*conv_t conv,*/ unsigned char *r, tjs_char wc, int n) /* n == 0 is acceptable */
{
	int count;
	if (wc < 0x80)
		count = 1;
	else if (wc < 0x800)
		count = 2;
	else if (wc < 0x10000)
		count = 3;
// 	else if (wc < 0x200000)
// 		count = 4;
// 	else if (wc < 0x4000000)
// 		count = 5;
// 	else if (wc <= 0x7fffffff)
// 		count = 6;
	else
		return -1;
	if (n < count)
		return -2;
	switch (count) { /* note: code falls through cases! */
// 	case 6: r[5] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x4000000;
// 	case 5: r[4] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x200000;
// 	case 4: r[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
	case 3: r[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
	case 2: r[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
	case 1: r[0] = wc;
	}
	return count;
}

namespace TJS
{

//---------------------------------------------------------------------------
// debug support
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
tjs_uint TJSGetTickCount()
{
	return GetTickCount();
}
#endif
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// some wchar_t support functions
//---------------------------------------------------------------------------
tjs_int TJS_atoi(const tjs_char *s)
{
	int r = 0;
	bool sign = false;
	while(*s && *s <= 0x20) s++; // skip spaces
	if(!*s) return 0;
	if(*s == TJS_W('-'))
	{
		sign = true;
		s++;
		while(*s && *s <= 0x20) s++; // skip spaces
		if(!*s) return 0;
	}

	while(*s >= TJS_W('0') && *s <= TJS_W('9'))
	{
		r *= 10;
		r += *s - TJS_W('0');
		s++;
	}
	if(sign) r = -r;
	return r;
}

tjs_int64 TJS_atoll(const tjs_char *s)
{
	tjs_int64 r = 0;
	bool sign = false;
	while (*s && *s <= 0x20) s++; // skip spaces
	if (!*s) return 0;
	if (*s == TJS_W('-'))
	{
		sign = true;
		s++;
		while (*s && *s <= 0x20) s++; // skip spaces
		if (!*s) return 0;
	}

	while (*s >= TJS_W('0') && *s <= TJS_W('9'))
	{
		r *= 10;
		r += *s - TJS_W('0');
		s++;
	}
	if (sign) r = -r;
	return r;
}
//---------------------------------------------------------------------------
tjs_char * TJS_int_to_str(tjs_int value, tjs_char *string)
{
	tjs_char *ostring = string;

	if(value<0) *(string++) = TJS_W('-'), value = -value;

	tjs_char buf[40];

	tjs_char *p = buf;

	do
	{
		*(p++) = (value % 10) + TJS_W('0');
		value /= 10;
	} while(value);

	p--;
	while(buf <= p) *(string++) = *(p--);
	*string = 0;

	return ostring;
}
//---------------------------------------------------------------------------
tjs_char * TJS_tTVInt_to_str(tjs_int64 value, tjs_char *string)
{
	if(value == TJS_UI64_VAL(0x8000000000000000))
	{
		// this is a special number which we must avoid normal conversion
		TJS_strcpy(string, TJS_W("-9223372036854775808"));
		return string;
	}

	tjs_char *ostring = string;

	if(value<0) *(string++) = TJS_W('-'), value = -value;

	tjs_char buf[40];

	tjs_char *p = buf;

	do
	{
		*(p++) = (value % 10) + TJS_W('0');
		value /= 10;
	} while(value);

	p--;
	while(buf <= p) *(string++) = *(p--);
	*string = 0;

	return ostring;
}
//---------------------------------------------------------------------------
tjs_int TJS_strnicmp(const tjs_char *s1, const tjs_char *s2,
	size_t maxlen)
{
	while(maxlen--)
	{
		if(*s1 == TJS_W('\0')) return (*s2 == TJS_W('\0')) ? 0 : -1;
		if(*s2 == TJS_W('\0')) return (*s1 == TJS_W('\0')) ? 0 : 1;
		if(*s1 < *s2) return -1;
		if(*s1 > *s2) return 1;
		s1++;
		s2++;
	}

	return 0;
}
//---------------------------------------------------------------------------
tjs_int TJS_stricmp(const tjs_char *s1, const tjs_char *s2)
{
	// we only support basic alphabets
	// fixme: complete alphabets support

	for(;;)
	{
		tjs_char c1 = *s1, c2 = *s2;
		if(c1 >= TJS_W('a') && c1 <= TJS_W('z')) c1 += TJS_W('Z')-TJS_W('z');
		if(c2 >= TJS_W('a') && c2 <= TJS_W('z')) c2 += TJS_W('Z')-TJS_W('z');
		if(c1 == TJS_W('\0')) return (c2 == TJS_W('\0')) ? 0 : -1;
		if(c2 == TJS_W('\0')) return (c1 == TJS_W('\0')) ? 0 : 1;
		if(c1 < c2) return -1;
		if(c1 > c2) return 1;
		s1++;
		s2++;
	}
}
//---------------------------------------------------------------------------
void TJS_strcpy_maxlen(tjs_char *d, const tjs_char *s, size_t len)
{
	tjs_char ch;
	len++;
	while((ch=*s)!=0 && --len) *(d++) = ch, s++;
	*d = 0;
}
//---------------------------------------------------------------------------
void TJS_strcpy(tjs_char *d, const tjs_char *s)
{
	tjs_char ch;
	while((ch=*s)!=0) *(d++) = ch, s++;
	*d = 0;
}
//---------------------------------------------------------------------------
size_t TJS_strlen(const tjs_char *d)
{
	const tjs_char *p = d;
	while(*d) d++;
	return d-p;
}
//---------------------------------------------------------------------------
tjs_int TJS_sprintf(tjs_char *s, const tjs_char *format, ...)
{
	tjs_int r;
	va_list param;
	va_start(param, format);
	r = TJS_vsnprintf(s, INT_MAX, format, param);
	va_end(param);
	return r;
}
//---------------------------------------------------------------------------

void TVPConsoleLog(const tjs_char *l);
void TJS_debug_out(const tjs_char *format, ...)
{
	tjs_int r;
	va_list param;
	va_start(param, format);
	tjs_char buf[256];
	r = TJS_vsnprintf(buf, 256, format, param);
	va_end(param);
	TVPConsoleLog(buf);
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#define TJS_MB_MAX_CHARLEN 2
//---------------------------------------------------------------------------
size_t TJS_mbstowcs(tjs_char *pwcs, const tjs_nchar *s, size_t n)
{
#if 0
    if(pwcs && n == 0) return 0;

	if(pwcs)
	{
		size_t count = SJISToUnicodeString( s, pwcs, n );
		if( n > count )
		{
			pwcs[count] = TJS_W('\0');
		}
		return count;
	}
	else
	{	// count length
		return SJISToUnicodeString( s, NULL );
	}
#endif
	if (!s) return -1;
	if (pwcs && n == 0) return 0;

	tjs_char wc;
    size_t count = 0;
    int cl;
	if (!pwcs) {
        n = strlen(s);
		while (*s) {
            cl = utf8_mbtowc(&wc, (const unsigned char*)s, n);
			if (cl <= 0)
                break;
            s += cl;
            n -= cl;
            ++count;
        }
    } else {
        tjs_char *pwcsend = pwcs + n;
        n = strlen(s);
		while (*s && pwcs < pwcsend) {
            cl = utf8_mbtowc(&wc, (const unsigned char*)s, n);
			if (cl <= 0)
                return -1;
            s += cl;
            n -= cl;
            *pwcs++ = wc;
            ++count;
        }
    }
    return count;
}
//---------------------------------------------------------------------------
size_t TJS_wcstombs(tjs_nchar *s, const tjs_char *pwcs, size_t n)
{
#if 0
    if(s && !n) return 0;

	if(s)
	{
		tjs_size count = UnicodeToSJISString( pwcs, s, n );
		if( n > count )
		{
			s[count] = '\0';
		}
		return count;
	}
	else
	{
		// Returns the buffer size to store the result
		return UnicodeToSJISString(pwcs,NULL);
	}
#endif
	if (!pwcs) return -1;
	if (s && !n) return 0;

    int cl;
	if (!s) {
        unsigned char tmp[6];
        size_t count = 0;
		while (*pwcs) {
            cl = utf8_wctomb(tmp, *pwcs, 6);
			if (cl <= 0)
                return -1;
			pwcs++;
            count += cl;
        }
        return count;
    } else {
        tjs_nchar *d = s;
		while (*pwcs && n > 0) {
            cl = utf8_wctomb((unsigned char*)d, *pwcs, n);
			if (cl <= 0)
                return -1;
            n -= cl;
            d += cl;
			pwcs++;
        }
        return d - s;
    }
}
//---------------------------------------------------------------------------
// 使われていないようなので未確認注意
int TJS_mbtowc(tjs_char *pwc, const tjs_nchar *s, size_t n)
{
#if 0
	if(!s || !n) return 0;

	if(*s == 0)
	{
		if(pwc) *pwc = 0;
		return 0;
	}

	/* Borland's RTL seems to assume always MB_MAX_CHARLEN = 2. */
	/* This may true while we use Win32 platforms ... */
	if( IsSJISLeadByte( *s ) )
	{
		// multi(double) byte character
		if((int)n < TJS_MB_MAX_CHARLEN) return -1;
		tjs_size count = SJISToUnicodeString( s, pwc, 1 );
		if( count <= 0 )
		{
			return -1;
		}
		return TJS_MB_MAX_CHARLEN;
	}
	else
	{
		// single byte character
		return (int)SJISToUnicodeString( s, pwc, 1 );
	}
#endif
	if (!s || !n) return 0;

	if (*s == 0)
	{
		if (pwc) *pwc = 0;
		return 0;
	}
	tjs_char wc;
    int ret = utf8_mbtowc(&wc, (const unsigned char *)s, n);
	if (ret >= 0) {
        *pwc = wc;
    }
    return ret;
}
//---------------------------------------------------------------------------
// 使われていないようなので未確認注意
int TJS_wctomb(tjs_nchar *s, tjs_char wc)
{
#if 0
    if(!s) return 0;
	tjs_uint c = UnicodeToSJIS(wc);
	if( c == 0 ) return -1;
	int size = 0;
	if( c & 0xff00 )
	{
		*s = static_cast<tjs_nchar>((c>>8)&0xff);
		s++;
		size++;
	}
	*s = static_cast<tjs_nchar>( c&0xff );
	size++;
	return size;
#endif
	if (!s) return 0;
	tjs_char tmp[2] = { wc, 0 };
    return utf8_wctomb((unsigned char*)s, wc, 2);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNarrowStringHolder
//---------------------------------------------------------------------------
tTJSNarrowStringHolder::tTJSNarrowStringHolder(const tjs_char * wide)
{
	int n;
	if(!wide)
		n = -1;
	else
		n = (int)TJS_wcstombs(NULL, wide, 0);

	if( n == -1 )
	{
		Buf = TJS_N("");
		Allocated = false;
		return;
	}
	Buf = new tjs_nchar[n+1];
		Allocated = true;
	Buf[TJS_wcstombs(Buf, wide, n)] = 0;
}
//---------------------------------------------------------------------------
tTJSNarrowStringHolder::~tTJSNarrowStringHolder()
{
	if(Allocated) delete [] Buf;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// native debugger break point
//---------------------------------------------------------------------------
void TJSNativeDebuggerBreak()
{
	// This function is to be called mostly when the "debugger" TJS statement is
	// executed.
	// Step you debbuger back to the the caller, and continue debugging.
	// Do not use "debugger" statement unless you run the program under the native
	// debugger, or the program may cause an unhandled debugger breakpoint
	// exception.

#if defined(__WIN32__)
	#if defined(_M_IX86)
		#ifdef __BORLANDC__
				__emit__ (0xcc); // int 3 (Raise debugger breakpoint exception)
		#else
				_asm _emit 0xcc; // int 3 (Raise debugger breakpoint exception)
		#endif
	#else
		__debugbreak();
	#endif
#endif
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// FPU control
//---------------------------------------------------------------------------
#if defined(__WIN32__) && !defined(__GNUC__)
static unsigned int TJSDefaultFPUCW = 0;
static unsigned int TJSNewFPUCW = 0;
static unsigned int TJSDefaultMMCW = 0;
static bool TJSFPUInit = false;
#endif
// FPU例外をマスク
void TJSSetFPUE()
{
#if defined(__WIN32__) && !defined(__GNUC__)
	if(!TJSFPUInit)
	{
		TJSFPUInit = true;
#if defined(_M_X64)
		TJSDefaultMMCW = _MM_GET_EXCEPTION_MASK();
#else
		TJSDefaultFPUCW = _control87(0, 0);

#ifdef _MSC_VER
		TJSNewFPUCW = _control87(MCW_EM, MCW_EM);
#else
		_default87 = TJSNewFPUCW = _control87(MCW_EM, MCW_EM);
#endif	// _MSC_VER
#ifdef TJS_SUPPORT_VCL
		Default8087CW = TJSNewFPUCW;
#endif	// TJS_SUPPORT_VCL
#endif	// _M_X64
	}

#if defined(_M_X64)
	_MM_SET_EXCEPTION_MASK(_MM_MASK_INVALID|_MM_MASK_DIV_ZERO|_MM_MASK_DENORM|_MM_MASK_OVERFLOW|_MM_MASK_UNDERFLOW|_MM_MASK_INEXACT);
#else
//	_fpreset();
	_control87(TJSNewFPUCW, 0xffff);
#endif
#endif	// defined(__WIN32__) && !defined(__GNUC__)

}
// 例外マスクを解除し元に戻す
void TJSRestoreFPUE()
{
#if defined(__WIN32__) && !defined(__GNUC__)
	if (!TJSFPUInit) return;
#if defined(_M_X64)
	_MM_SET_EXCEPTION_MASK(TJSDefaultMMCW);
#else
	_control87(TJSDefaultFPUCW, 0xffff);
#endif
#endif
}

int TJS_strcmp (
    const tjs_char * src,
    const tjs_char * dst
    )
{
    int ret = 0 ;

    while( ! (ret = (int)(*src - *dst)) && *dst)
        ++src, ++dst;

    if ( ret < 0 )
        ret = -1 ;
    else if ( ret > 0 )
        ret = 1 ;

    return( ret );
}

int TJS_strncmp (
    const tjs_char * first,
    const tjs_char * last,
    size_t count
    )
{
    if (!count)
        return(0);

    while (--count && *first && *first == *last)
    {
        first++;
        last++;
    }

    return((int)(*first - *last));
}

tjs_char * TJS_strncpy (
    tjs_char * dest,
    const tjs_char * source,
    size_t count
    )
{
    tjs_char *start = dest;

    while (count && (*dest++ = *source++))    /* copy string */
        count--;

    if (count)                              /* pad out with zeroes */
        while (--count)
            *dest++ = L'\0';

    return(start);
}

tjs_char * TJS_strcat (
    tjs_char * dst,
    const tjs_char * src
    )
{
    tjs_char * cp = dst;

    while( *cp )
        cp++;                   /* find end of dst */

    while( (*cp++ = *src++) ) ;       /* Copy src to end of dst */

    return( dst );                  /* return dst */

}

tjs_char * TJS_strstr (
    const tjs_char * wcs1,
    const tjs_char * wcs2
    )
{
    tjs_char *cp = (tjs_char *) wcs1;
    tjs_char *s1, *s2;

    if ( !*wcs2)
        return (tjs_char *)wcs1;

    while (*cp)
    {
        s1 = cp;
        s2 = (tjs_char *) wcs2;

        while ( *s1 && *s2 && !(*s1-*s2) )
            s1++, s2++;

        if (!*s2)
            return(cp);

        cp++;
    }

    return(NULL);
}
tjs_char * TJS_strchr (
    const tjs_char * string,
    tjs_char ch
    )
{
    while (*string && *string != (tjs_char)ch)
        string++;

    if (*string == (tjs_char)ch)
        return((tjs_char *)string);
    return(NULL);
}

void * TJS_malloc(size_t len)
{
	char *ret = (char*)malloc(len+sizeof(size_t));
	if (!ret) return nullptr;
	*(size_t*)ret = len; // embed size
	return ret + sizeof(size_t);
}

void * TJS_realloc(void* buf, size_t len)
{
	if (!buf) return TJS_malloc(len);
	// compare embeded size
	size_t * ptr = (size_t *)((char*)buf - sizeof(size_t));
	if (*ptr >= len) return buf; // still adequate
	char *ret = (char*)TJS_malloc(len);
	if (!ret) return nullptr;
	memcpy(ret, ptr + 1, *ptr);
	TJS_free(buf);
	return ret;
}

void TJS_free(void *buf)
{
	free((char*)buf - sizeof(size_t));
}

tjs_char *TJS_strrchr(const tjs_char *s, int c)
{
	tjs_char* ret = 0;
	do {
		if (*s == (char)c)
			ret = (tjs_char *)s;
	} while (*s++);
	return ret;
}

#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
//#include <inttypes.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

/* Some useful macros */

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define CONCAT2(x,y) x ## y
#define CONCAT(x,y) CONCAT2(x,y)
#define NL_ARGMAX 9

/* Convenient bit representation for modifier flags, which all fall
 * within 31 codepoints of the space character. */

#define ALT_FORM   (1U<<('#'-' '))
#define ZERO_PAD   (1U<<('0'-' '))
#define LEFT_ADJ   (1U<<('-'-' '))
#define PAD_POS    (1U<<(' '-' '))
#define MARK_POS   (1U<<('+'-' '))
#define GROUPED    (1U<<('\''-' '))

#define FLAGMASK (ALT_FORM|ZERO_PAD|LEFT_ADJ|PAD_POS|MARK_POS|GROUPED)

#if UINT_MAX == ULONG_MAX
#define LONG_IS_INT
#endif

#if SIZE_MAX != ULONG_MAX || UINTMAX_MAX != ULLONG_MAX
#define ODD_TYPES
#endif

/* State machine to accept length modifiers + conversion specifiers.
 * Result is 0 on failure, or an argument type to pop on success. */

enum {
	BARE, LPRE, LLPRE, HPRE, HHPRE, BIGLPRE,
	ZTPRE, JPRE,
	STOP,
	PTR, INT, UINT, ULLONG,
#ifndef LONG_IS_INT
	LONG, ULONG,
#else
#define LONG INT
#define ULONG UINT
#endif
	SHORT, USHORT, CHAR, UCHAR,
#ifdef ODD_TYPES
	LLONG, SIZET, IMAX, UMAX, PDIFF, UIPTR,
#else
#define LLONG ULLONG
#define SIZET ULONG
#define IMAX LLONG
#define UMAX ULLONG
#define PDIFF LONG
#define UIPTR ULONG
#endif
	DBL, LDBL,
	NOARG,
	MAXSTATE
};
#define S(x) [(x)-'A']

static const unsigned char states[]['z'-'A'+1] = 
{{DBL,
BARE,
INT,
BARE,
DBL,
DBL,
DBL,
BARE,
BARE,
BARE,
BARE,
BIGLPRE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
PTR,
BARE,
BARE,
BARE,
BARE,
UINT,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
DBL,
BARE,
CHAR,
INT,
DBL,
DBL,
DBL,
HPRE,
INT,
JPRE,
BARE,
LPRE,
NOARG,
PTR,
UINT,
UIPTR,
BARE,
BARE,
PTR,
ZTPRE,
UINT,
BARE,
BARE,
UINT,
BARE,
ZTPRE},
{DBL,
BARE,
BARE,
BARE,
DBL,
DBL,
DBL,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
ULONG,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
DBL,
BARE,
INT,
LONG,
DBL,
DBL,
DBL,
BARE,
LONG,
BARE,
BARE,
LLPRE,
BARE,
PTR,
ULONG,
BARE,
BARE,
BARE,
PTR,
BARE,
ULONG,
BARE,
BARE,
ULONG,
BARE,
BARE},
{BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
ULLONG,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
LLONG,
BARE,
BARE,
BARE,
BARE,
LLONG,
BARE,
BARE,
BARE,
BARE,
PTR,
ULLONG,
BARE,
BARE,
BARE,
BARE,
BARE,
ULLONG,
BARE,
BARE,
ULLONG,
BARE,
BARE},
{BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
USHORT,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
SHORT,
BARE,
BARE,
BARE,
HHPRE,
SHORT,
BARE,
BARE,
BARE,
BARE,
PTR,
USHORT,
BARE,
BARE,
BARE,
BARE,
BARE,
USHORT,
BARE,
BARE,
USHORT,
BARE,
BARE},
{BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
UCHAR,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
CHAR,
BARE,
BARE,
BARE,
BARE,
CHAR,
BARE,
BARE,
BARE,
BARE,
PTR,
UCHAR,
BARE,
BARE,
BARE,
BARE,
BARE,
UCHAR,
BARE,
BARE,
UCHAR,
BARE,
BARE},
{LDBL,
BARE,
BARE,
BARE,
LDBL,
LDBL,
LDBL,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
LDBL,
BARE,
BARE,
BARE,
LDBL,
LDBL,
LDBL,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
PTR,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE},
{BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
SIZET,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
PDIFF,
BARE,
BARE,
BARE,
BARE,
PDIFF,
BARE,
BARE,
BARE,
BARE,
PTR,
SIZET,
BARE,
BARE,
BARE,
BARE,
BARE,
SIZET,
BARE,
BARE,
SIZET,
BARE,
BARE},
{BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
UMAX,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
BARE,
IMAX,
BARE,
BARE,
BARE,
BARE,
IMAX,
BARE,
BARE,
BARE,
BARE,
PTR,
UMAX,
BARE,
BARE,
BARE,
BARE,
BARE,
UMAX,
BARE,
BARE,
UMAX,
BARE,
BARE}};
// {
// 	{ /* 0: bare types */
// 		S('d') = INT, S('i') = INT,
// 		S('o') = UINT, S('u') = UINT, S('x') = UINT, S('X') = UINT,
// 		S('e') = DBL, S('f') = DBL, S('g') = DBL, S('a') = DBL,
// 		S('E') = DBL, S('F') = DBL, S('G') = DBL, S('A') = DBL,
// 		S('c') = CHAR, S('C') = INT,
// 		S('s') = PTR, S('S') = PTR, S('p') = UIPTR, S('n') = PTR,
// 		S('m') = NOARG,
// 		S('l') = LPRE, S('h') = HPRE, S('L') = BIGLPRE,
// 		S('z') = ZTPRE, S('j') = JPRE, S('t') = ZTPRE,
// 	}, { /* 1: l-prefixed */
// 		S('d') = LONG, S('i') = LONG,
// 		S('o') = ULONG, S('u') = ULONG, S('x') = ULONG, S('X') = ULONG,
// 		S('e') = DBL, S('f') = DBL, S('g') = DBL, S('a') = DBL,
// 		S('E') = DBL, S('F') = DBL, S('G') = DBL, S('A') = DBL,
// 		S('c') = INT, S('s') = PTR, S('n') = PTR,
// 		S('l') = LLPRE,
// 	}, { /* 2: ll-prefixed */
// 		S('d') = LLONG, S('i') = LLONG,
// 		S('o') = ULLONG, S('u') = ULLONG,
// 		S('x') = ULLONG, S('X') = ULLONG,
// 		S('n') = PTR,
// 	}, { /* 3: h-prefixed */
// 		S('d') = SHORT, S('i') = SHORT,
// 		S('o') = USHORT, S('u') = USHORT,
// 		S('x') = USHORT, S('X') = USHORT,
// 		S('n') = PTR,
// 		S('h') = HHPRE,
// 	}, { /* 4: hh-prefixed */
// 		S('d') = CHAR, S('i') = CHAR,
// 		S('o') = UCHAR, S('u') = UCHAR,
// 		S('x') = UCHAR, S('X') = UCHAR,
// 		S('n') = PTR,
// 	}, { /* 5: L-prefixed */
// 		S('e') = LDBL, S('f') = LDBL, S('g') = LDBL, S('a') = LDBL,
// 		S('E') = LDBL, S('F') = LDBL, S('G') = LDBL, S('A') = LDBL,
// 		S('n') = PTR,
// 	}, { /* 6: z- or t-prefixed (assumed to be same size) */
// 		S('d') = PDIFF, S('i') = PDIFF,
// 		S('o') = SIZET, S('u') = SIZET,
// 		S('x') = SIZET, S('X') = SIZET,
// 		S('n') = PTR,
// 	}, { /* 7: j-prefixed */
// 		S('d') = IMAX, S('i') = IMAX,
// 		S('o') = UMAX, S('u') = UMAX,
// 		S('x') = UMAX, S('X') = UMAX,
// 		S('n') = PTR,
// 	}
// };
#define OOB(x) ((unsigned)(x)-'A' > 'z'-'A')

// typedef long long          intmax_t;
// typedef unsigned long long uintmax_t;
// typedef unsigned short     uint16_t;
// typedef unsigned long      uint32_t;
// typedef unsigned long long uint64_t;
union arg
{
	uintmax_t i;
	long double f;
	void *p;
};

static void pop_arg(union arg *arg, int type, va_list *ap)
{
	/* Give the compiler a hint for optimizing the switch. */
	if ((unsigned)type > MAXSTATE) return;
	switch (type) {
	       case PTR:	arg->p = va_arg(*ap, void *);
	break; case INT:	arg->i = va_arg(*ap, int);
	break; case UINT:	arg->i = va_arg(*ap, unsigned int);
#ifndef LONG_IS_INT
	break; case LONG:	arg->i = va_arg(*ap, long);
	break; case ULONG:	arg->i = va_arg(*ap, unsigned long);
#endif
	break; case ULLONG:	arg->i = va_arg(*ap, unsigned long long);
	break; case SHORT:	arg->i = (short)va_arg(*ap, int);
	break; case USHORT:	arg->i = (unsigned short)va_arg(*ap, int);
	break; case CHAR:	arg->i = (signed char)va_arg(*ap, int);
	break; case UCHAR:	arg->i = (unsigned char)va_arg(*ap, int);
#ifdef ODD_TYPES
	break; case LLONG:	arg->i = va_arg(*ap, long long);
	break; case SIZET:	arg->i = va_arg(*ap, size_t);
	break; case IMAX:	arg->i = va_arg(*ap, intmax_t);
	break; case UMAX:	arg->i = va_arg(*ap, uintmax_t);
	break; case PDIFF:	arg->i = va_arg(*ap, ptrdiff_t);
	break; case UIPTR:	arg->i = (uintptr_t)va_arg(*ap, void *);
#endif
	break; case DBL:	arg->f = va_arg(*ap, double);
	break; case LDBL:	arg->f = va_arg(*ap, long double);
	}
}

struct _tFILE
{
    tjs_char *p;
};

static void out(_tFILE *f, const tjs_char *s, size_t l)
{
    memcpy(f->p, s, l * sizeof(*f->p));
    f->p += l;
}

static void pad(_tFILE *f, tjs_char c, int w, int l, int fl)
{
	tjs_char pad[256];
	if (fl & (LEFT_ADJ | ZERO_PAD) || l >= w) return;
	l = w - l;
    int n = l >sizeof pad / sizeof(pad[0])? sizeof pad / sizeof(pad[0]): l;
    while(n--) pad[n] = c;
	for (; l >= sizeof pad; l -= sizeof pad)
		out(f, pad, sizeof pad / sizeof(pad[0]));
	out(f, pad, l);
}

static const char xdigits[] = 
	"0123456789ABCDEF";


static tjs_char *fmt_x(uintmax_t x, tjs_char *s, int lower)
{
	for (; x; x>>=4) *--s = xdigits[(x&15)]|lower;
	return s;
}

static tjs_char *fmt_o(uintmax_t x, tjs_char *s)
{
	for (; x; x>>=3) *--s = '0' + (x&7);
	return s;
}

static tjs_char *fmt_u(uintmax_t x, tjs_char *s)
{
	unsigned long y;
	for (   ; x>ULONG_MAX; x/=10) *--s = '0' + x%10;
	for (y=x;           y; y/=10) *--s = '0' + y%10;
	return s;
}
#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
union ldshape {
    long double f;
    struct {
        uint64_t m;
        uint16_t se;
    } i;
};
#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384 /*&& __BYTE_ORDER == __LITTLE_ENDIAN*/
union ldshape {
    long double f;
    struct {
        uint64_t m;
        uint16_t se;
    } i;
};
#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384 /*&& __BYTE_ORDER == __LITTLE_ENDIAN*/
union ldshape {
    long double f;
    struct {
        uint64_t lo;
        uint32_t mid;
        uint16_t top;
        uint16_t se;
    } i;
    struct {
        uint64_t lo;
        uint64_t hi;
    } i2;
};
#else
#error Unsupported long double representation
#endif
static unsigned __FLOAT_BITS(float __f)
{
    union {float __f; unsigned __i;} __u = {__f};
    return __u.__i;
}
static __inline unsigned long long __DOUBLE_BITS(double __f)
{
    union {double __f; unsigned long long __i;} __u = {__f};
    return __u.__i;
}
#ifdef signbit
#undef signbit
#endif
int signbit(long double x)
{
    if(sizeof(x) == sizeof(float))
        return (int)(__FLOAT_BITS(x)>>31);
    if(sizeof(x) == sizeof(double))
        return (int)(__DOUBLE_BITS(x)>>63);
    ldshape u = {x};
    return u.i.se >> 15;
}
    
#ifndef FP_NAN
#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4
#endif
// #define isfinite(x) \
//         (sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) < 0x7f800000 : \
//     sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL>>1) < 0x7ffULL<<52 : \
//     __fpclassifyl(x) > FP_INFINITE)

static int fmt_fp(_tFILE *f, long double y, int w, int p, int fl, int t)
{
    uint32_t big[(LDBL_MAX_EXP+LDBL_MANT_DIG)/9+1];
    uint32_t *a, *d, *r, *z;
    int e2=0, e, i, j, l;
    tjs_char buf[9+LDBL_MANT_DIG/4], *s;
    const tjs_char *prefix=TJS_W("-0X+0X 0X-0x+0x 0x");
    int pl;
    tjs_char ebuf0[3*sizeof(int)], *ebuf=&ebuf0[3*sizeof(int)], *estr;

    pl=1;
    if (signbit(y)) {
        y=-y;
    } else if (fl & MARK_POS) {
        prefix+=3;
    } else if (fl & PAD_POS) {
        prefix+=6;
    } else prefix++, pl=0;

    if (!isfinite(y)) {
        const tjs_char *s = (t&32)?TJS_W("inf"):TJS_W("INF");
        if (y!=y) s=(t&32)?TJS_W("nan"):TJS_W("NAN"), pl=0;
        pad(f, ' ', w, 3+pl, fl&~ZERO_PAD);
        out(f, prefix, pl);
        out(f, s, 3);
        pad(f, ' ', w, 3+pl, fl^LEFT_ADJ);
        return MAX(w, 3+pl);
    }

    y = frexpl(y, &e2) * 2;
    if (y) e2--;

    if ((t|32)=='a') {
        long double round = 8.0;
        int re;

        if (t&32) prefix += 9;
        pl += 2;

        if (p<0 || p>=LDBL_MANT_DIG/4-1) re=0;
        else re=LDBL_MANT_DIG/4-1-p;

        if (re) {
            while (re--) round*=16;
            if (*prefix=='-') {
                y=-y;
                y-=round;
                y+=round;
                y=-y;
            } else {
                y+=round;
                y-=round;
            }
        }

        estr=fmt_u(e2<0 ? -e2 : e2, ebuf);
        if (estr==ebuf) *--estr='0';
        *--estr = (e2<0 ? '-' : '+');
        *--estr = t+('p'-'a');

        s=buf;
        do {
            int x=y;
            *s++=xdigits[x]|(t&32);
            y=16*(y-x);
            if (s-buf==1 && (y||p>0||(fl&ALT_FORM))) *s++='.';
        } while (y);

        if (p && s-buf-2 < p)
            l = (p+2) + (ebuf-estr);
        else
            l = (s-buf) + (ebuf-estr);

        pad(f, ' ', w, pl+l, fl);
        out(f, prefix, pl);
        pad(f, '0', w, pl+l, fl^ZERO_PAD);
        out(f, buf, s-buf);
        pad(f, '0', l-(ebuf-estr)-(s-buf), 0, 0);
        out(f, estr, ebuf-estr);
        pad(f, ' ', w, pl+l, fl^LEFT_ADJ);
        return MAX(w, pl+l);
    }
    if (p<0) p=6;

    if (y) y *= 0x1<<28, e2-=28;

    if (e2<0) a=r=z=big;
    else a=r=z=big+sizeof(big)/sizeof(*big) - LDBL_MANT_DIG - 1;

    do {
        *z = y;
        y = 1000000000*(y-*z++);
    } while (y);

    while (e2>0) {
        uint32_t carry=0;
        int sh=MIN(29,e2);
        for (d=z-1; d>=a; d--) {
            uint64_t x = ((uint64_t)*d<<sh)+carry;
            *d = x % 1000000000;
            carry = x / 1000000000;
        }
        if (!z[-1] && z>a) z--;
        if (carry) *--a = carry;
        e2-=sh;
    }
    while (e2<0) {
        uint32_t carry=0, *b;
        int sh=MIN(9,-e2);
        for (d=a; d<z; d++) {
            uint32_t rm = *d & (1<<sh)-1;
            *d = (*d>>sh) + carry;
            carry = (1000000000>>sh) * rm;
        }
        if (!*a) a++;
        if (carry) *z++ = carry;
        /* Avoid (slow!) computation past requested precision */
        b = (t|32)=='f' ? r : a;
        if (z-b > 2+p/9) z = b+2+p/9;
        e2+=sh;
    }

    if (a<z) for (i=10, e=9*(r-a); *a>=i; i*=10, e++);
    else e=0;

    /* Perform rounding: j is precision after the radix (possibly neg) */
    j = p - ((t|32)!='f')*e - ((t|32)=='g' && p);
    if (j < 9*(z-r-1)) {
        uint32_t x;
        /* We avoid C's broken division of negative numbers */
        d = r + 1 + ((j+9*LDBL_MAX_EXP)/9 - LDBL_MAX_EXP);
        j += 9*LDBL_MAX_EXP;
        j %= 9;
        for (i=10, j++; j<9; i*=10, j++);
        x = *d % i;
        /* Are there any significant digits past j? */
        if (x || d+1!=z) {
            long double round = 0x1<<LDBL_MANT_DIG;
            long double small;
            if (*d/i & 1) round += 2;
            if (x<i/2) small=0.5;
            else if (x==i/2 && d+1==z) small=1.0;
            else small=1.5;
            if (pl && *prefix=='-') round*=-1, small*=-1;
            *d -= x;
            /* Decide whether to round by probing round+small */
            if (round+small != round) {
                *d = *d + i;
                while (*d > 999999999) {
                    *d--=0;
                    (*d)++;
                }
                if (d<a) a=d;
                for (i=10, e=9*(r-a); *a>=i; i*=10, e++);
            }
        }
        if (z>d+1) z=d+1;
        for (; !z[-1] && z>a; z--);
    }

    if ((t|32)=='g') {
        if (!p) p++;
        if (p>e && e>=-4) {
            t--;
            p-=e+1;
        } else {
            t-=2;
            p--;
        }
        if (!(fl&ALT_FORM)) {
            /* Count trailing zeros in last place */
            if (z>a && z[-1]) for (i=10, j=0; z[-1]%i==0; i*=10, j++);
            else j=9;
            if ((t|32)=='f')
                p = MIN(p,MAX(0,9*(z-r-1)-j));
            else
                p = MIN(p,MAX(0,9*(z-r-1)+e-j));
        }
    }
    l = 1 + p + (p || (fl&ALT_FORM));
    if ((t|32)=='f') {
        if (e>0) l+=e;
    } else {
        estr=fmt_u(e<0 ? -e : e, ebuf);
        while(ebuf-estr<2) *--estr='0';
        *--estr = (e<0 ? '-' : '+');
        *--estr = t;
        l += ebuf-estr;
    }

    pad(f, ' ', w, pl+l, fl);
    out(f, prefix, pl);
    pad(f, '0', w, pl+l, fl^ZERO_PAD);

    if ((t|32)=='f') {
        if (a>r) a=r;
        for (d=a; d<=r; d++) {
            tjs_char *s = fmt_u(*d, buf+9);
            if (d!=a) while (s>buf) *--s='0';
            else if (s==buf+9) *--s='0';
            out(f, s, buf+9-s);
        }
        if (p || (fl&ALT_FORM)) out(f, TJS_W("."), 1);
        for (; d<z && p>0; d++, p-=9) {
            tjs_char *s = fmt_u(*d, buf+9);
            while (s>buf) *--s='0';
            out(f, s, MIN(9,p));
        }
        pad(f, '0', p+9, 9, 0);
    } else {
        if (z<=a) z=a+1;
        for (d=a; d<z && p>=0; d++) {
            tjs_char *s = fmt_u(*d, buf+9);
            if (s==buf+9) *--s='0';
            if (d!=a) while (s>buf) *--s='0';
            else {
                out(f, s++, 1);
                if (p>0||(fl&ALT_FORM)) out(f, TJS_W("."), 1);
            }
            out(f, s, MIN(buf+9-s, p));
            p -= buf+9-s;
        }
        pad(f, '0', p+18, 18, 0);
        out(f, estr, ebuf-estr);
    }

    pad(f, ' ', w, pl+l, fl^LEFT_ADJ);

    return MAX(w, pl+l);
}

static int getint(tjs_char **s) {
    int i;
    for (i=0; isdigit(**s); (*s)++)
        i = 10*i + (**s-'0');
    return i;
}

static int printf_core(_tFILE *f, const tjs_char *fmt, va_list *ap, union arg *nl_arg, int *nl_type)
{
    tjs_char *a, *z, *s=(tjs_char *)fmt;
    unsigned l10n=0, fl;
    int w, p;
    union arg arg;
    int argpos;
    unsigned st, ps;
    int cnt=0, l=0;
    int i;
    tjs_char buf[sizeof(uintmax_t)*3+3+LDBL_MANT_DIG/4];
    const tjs_char *prefix;
    int t, pl;
    tjs_char wc[2], *ws;
    tjs_char mb[4];

    for (;;) {
        /* Update output count, end loop when fmt is exhausted */
        if (cnt >= 0) {
            if (l > INT_MAX - cnt) {
                errno = EOVERFLOW;
                cnt = -1;
            } else cnt += l;
        }
        if (!*s) break;

        /* Handle literal text and %% format specifiers */
        for (a=s; *s && *s!='%'; s++);
        for (z=s; s[0]=='%' && s[1]=='%'; z++, s+=2);
        l = z-a;
        if (f) out(f, a, l);
        if (l) continue;

        if (isdigit(s[1]) && s[2]=='$') {
            l10n=1;
            argpos = s[1]-'0';
            s+=3;
        } else {
            argpos = -1;
            s++;
        }

        /* Read modifier flags */
        for (fl=0; (unsigned)*s-' '<32 && (FLAGMASK&(1U<<(*s-' '))); s++)
            fl |= 1U<<(*s-' ');

        /* Read field width */
        if (*s=='*') {
            if (isdigit(s[1]) && s[2]=='$') {
                l10n=1;
                nl_type[s[1]-'0'] = INT;
                w = nl_arg[s[1]-'0'].i;
                s+=3;
            } else if (!l10n) {
                w = f ? va_arg(*ap, int) : 0;
                s++;
            } else return -1;
            if (w<0) fl|=LEFT_ADJ, w=-w;
        } else if ((w=getint(&s))<0) return -1;

        /* Read precision */
        if (*s=='.' && s[1]=='*') {
            if (isdigit(s[2]) && s[3]=='$') {
                nl_type[s[2]-'0'] = INT;
                p = nl_arg[s[2]-'0'].i;
                s+=4;
            } else if (!l10n) {
                p = f ? va_arg(*ap, int) : 0;
                s+=2;
            } else return -1;
        } else if (*s=='.') {
            s++;
            p = getint(&s);
        } else p = -1;

        /* Format specifier state machine */
        st=0;
        do {
            if (OOB(*s)) return -1;
            ps=st;
            st=states[st]S(*s++);
        } while (st-1<STOP);
        if (!st) return -1;

        /* Check validity of argument type (nl/normal) */
        if (st==NOARG) {
            if (argpos>=0) return -1;
            else if (!f) continue;
        } else {
            if (argpos>=0) nl_type[argpos]=st, arg=nl_arg[argpos];
            else if (f) pop_arg(&arg, st, ap);
            else return 0;
        }

        if (!f) continue;

        z = buf + sizeof(buf) / sizeof(buf[0]);
        prefix = TJS_W("-+   0X0x");
        pl = 0;
        t = s[-1];

        /* Transform ls,lc -> S,C */
        if (ps && (t&15)==3) t&=~32;

        /* - and 0 flags are mutually exclusive */
        if (fl & LEFT_ADJ) fl &= ~ZERO_PAD;

        switch(t) {
        case 'n':
            switch(ps) {
            case BARE: *(int *)arg.p = cnt; break;
            case LPRE: *(long *)arg.p = cnt; break;
            case LLPRE: *(long long *)arg.p = cnt; break;
            case HPRE: *(unsigned short *)arg.p = cnt; break;
            case HHPRE: *(unsigned char *)arg.p = cnt; break;
            case ZTPRE: *(size_t *)arg.p = cnt; break;
            case JPRE: *(uintmax_t *)arg.p = cnt; break;
            }
            continue;
        case 'p':
            p = MAX(p, 2*sizeof(void*));
            t = 'x';
            fl |= ALT_FORM;
        case 'x': case 'X':
            a = fmt_x(arg.i, z, t&32);
            if (arg.i && (fl & ALT_FORM)) prefix+=(t>>4), pl=2;
            if (0) {
        case 'o':
            a = fmt_o(arg.i, z);
            if ((fl&ALT_FORM) && arg.i) prefix+=5, pl=1;
            } if (0) {
        case 'd': case 'i':
            pl=1;
            if (arg.i>INTMAX_MAX) {
                arg.i=-arg.i;
            } else if (fl & MARK_POS) {
                prefix++;
            } else if (fl & PAD_POS) {
                prefix+=2;
            } else pl=0;
        case 'u':
            a = fmt_u(arg.i, z);
            }
            if (p>=0) fl &= ~ZERO_PAD;
            if (!arg.i && !p) {
                a=z;
                break;
            }
            p = MAX(p, z-a + !arg.i);
            break;
        case 'c':
            *(a=z-(p=1))=arg.i;
            fl &= ~ZERO_PAD;
            break;
//         case 'm':
//             if (1) a = strerror(errno); else
        case 's':
        case 'S':
            z = a = arg.p ? (tjs_char*)arg.p : (tjs_char*)TJS_W("(null)");
            while(*z) z++;
            //z = (tjs_char*)memchr(a, 0, p);
            if (!z) z=a+p;
            else p=z-a;
            fl &= ~ZERO_PAD;
            break;
        case 'C':
            wc[0] = arg.i;
            wc[1] = 0;
            arg.p = wc;
            p = -1;
//         case 'S':
//             ws = (wchar_t*)arg.p;
//             for (i=l=0; i<0U+p && *ws && (l=wctomb(mb, *ws++))>=0 && l<=0U+p-i; i+=l);
//             if (l<0) return -1;
//             p = i;
//             pad(f, ' ', w, p, fl);
//             ws = (wchar_t*)arg.p;
//             for (i=0; i<0U+p && *ws && i+(l=wctomb(mb, *ws++))<=p; i+=l)
//                 out(f, mb, l);
//             pad(f, ' ', w, p, fl^LEFT_ADJ);
//             l = w>p ? w : p;
//             continue;
        case 'e': case 'f': case 'g': case 'a':
        case 'E': case 'F': case 'G': case 'A':
            l = fmt_fp(f, arg.f, w, p, fl, t);
            continue;
        }

        if (p < z-a) p = z-a;
        if (w < pl+p) w = pl+p;

        pad(f, ' ', w, pl+p, fl);
        out(f, prefix, pl);
        pad(f, '0', w, pl+p, fl^ZERO_PAD);
        pad(f, '0', p, z-a, 0);
        out(f, a, z-a);
        pad(f, ' ', w, pl+p, fl^LEFT_ADJ);

        l = w;
    }

    out(f, TJS_W(""), 1);
    if (f) return cnt;
    if (!l10n) return 0;

    for (i=1; i<=NL_ARGMAX && nl_type[i]; i++)
        pop_arg(nl_arg+i, nl_type[i], ap);
    for (; i<=NL_ARGMAX && !nl_type[i]; i++);
    if (i<=NL_ARGMAX) return -1;
    return 1;
}

int _vsnprintf(tjs_char * s, size_t n, const tjs_char * fmt, va_list ap)
{
    int r;
    _tFILE f = {s };
    
    int nl_type[NL_ARGMAX+1] = {0};
    union arg nl_arg[NL_ARGMAX+1];
    unsigned char internal_buf[80], *saved_buf = 0;
    va_list *pap = (va_list *)&ap;
    r = printf_core(&f, fmt, pap, nl_arg, nl_type);

    /* Null-terminate, overwriting last char if dest buffer is full */
    return r;
}

static int snprintf(tjs_char *s, size_t n, const tjs_char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = _vsnprintf(s, n, fmt, ap);
    va_end(ap);
    return ret;
}

static int is_leap(int y)
{
    /* Avoid overflow */
    if (y>INT_MAX-1900) y -= 2000;
    y += 1900;
    return !(y%4) && ((y%100) || !(y%400));
}

static int week_num(const tm *tm)
{
	int val = (tm->tm_yday + 7 - (tm->tm_wday+6)%7) / 7;
	/* If 1 Jan is just 1-3 days past Monday,
	 * the previous week is also in this year. */
	if ((tm->tm_wday - tm->tm_yday - 2 + 371) % 7 <= 2)
		val++;
	if (!val) {
		val = 52;
		/* If 31 December of prev year a Thursday,
		 * or Friday of a leap year, then the
		 * prev year has 53 weeks. */
		int dec31 = (tm->tm_wday - tm->tm_yday - 1 + 7) % 7;
		if (dec31 == 4 || (dec31 == 5 && is_leap(tm->tm_year%400-1)))
			val++;
	} else if (val == 53) {
		/* If 1 January is not a Thursday, and not
		 * a Wednesday of a leap year, then this
		 * year has only 52 weeks. */
		int jan1 = (tm->tm_wday - tm->tm_yday + 371) % 7;
		if (jan1 != 4 && (jan1 != 3 || !is_leap(tm->tm_year)))
			val = 1;
	}
	return val;
}
#define ABDAY_1 0x20000
#define ABDAY_2 0x20001
#define ABDAY_3 0x20002
#define ABDAY_4 0x20003
#define ABDAY_5 0x20004
#define ABDAY_6 0x20005
#define ABDAY_7 0x20006

#define DAY_1 0x20007
#define DAY_2 0x20008
#define DAY_3 0x20009
#define DAY_4 0x2000A
#define DAY_5 0x2000B
#define DAY_6 0x2000C
#define DAY_7 0x2000D

#define ABMON_1 0x2000E
#define ABMON_2 0x2000F
#define ABMON_3 0x20010
#define ABMON_4 0x20011
#define ABMON_5 0x20012
#define ABMON_6 0x20013
#define ABMON_7 0x20014
#define ABMON_8 0x20015
#define ABMON_9 0x20016
#define ABMON_10 0x20017
#define ABMON_11 0x20018
#define ABMON_12 0x20019

#define MON_1 0x2001A
#define MON_2 0x2001B
#define MON_3 0x2001C
#define MON_4 0x2001D
#define MON_5 0x2001E
#define MON_6 0x2001F
#define MON_7 0x20020
#define MON_8 0x20021
#define MON_9 0x20022
#define MON_10 0x20023
#define MON_11 0x20024
#define MON_12 0x20025

#define AM_STR 0x20026
#define PM_STR 0x20027

#define D_T_FMT 0x20028
#define D_FMT 0x20029
#define T_FMT 0x2002A
#define T_FMT_AMPM 0x2002B

#define ERA 0x2002C
#define ERA_D_FMT 0x2002E
#define ALT_DIGITS 0x2002F
#define ERA_D_T_FMT 0x20030
#define ERA_T_FMT 0x20031

#define CODESET 14

#define CRNCYSTR 0x4000F

#define RADIXCHAR 0x10000
#define THOUSEP 0x10001
#define YESEXPR 0x50000
#define NOEXPR 0x50001
#define YESSTR 0x50002
#define NOSTR 0x50003
long long __year_to_secs(long long year, int *is_leap)
{
    if (year-2ULL <= 136) {
        int y = year;
        int leaps = (y-68)>>2;
        if (!((y-68)&3)) {
            leaps--;
            if (is_leap) *is_leap = 1;
        } else if (is_leap) *is_leap = 0;
        return 31536000*(y-70) + 86400*leaps;
    }

    int cycles, centuries, leaps, rem;
    int z = 0;
    if (!is_leap) is_leap = &z;
    cycles = (year-100) / 400;
    rem = (year-100) % 400;
    if (rem < 0) {
        cycles--;
        rem += 400;
    }
    if (!rem) {
        *is_leap = 1;
        centuries = 0;
        leaps = 0;
    } else {
        if (rem >= 200) {
            if (rem >= 300) centuries = 3, rem -= 300;
            else centuries = 2, rem -= 200;
        } else {
            if (rem >= 100) centuries = 1, rem -= 100;
            else centuries = 0;
        }
        if (!rem) {
            *is_leap = 0;
            leaps = 0;
        } else {
            leaps = rem / 4U;
            rem %= 4U;
            *is_leap = !rem;
        }
    }

    leaps += 97*cycles + 24*centuries - *is_leap;

    return (year-100) * 31536000LL + leaps * 86400LL + 946684800 + 86400;
}

int __month_to_secs(int month, int is_leap)
{
    static const int secs_through_month[] = {
        0, 31*86400, 59*86400, 90*86400,
        120*86400, 151*86400, 181*86400, 212*86400,
        243*86400, 273*86400, 304*86400, 334*86400 };
    int t = secs_through_month[month];
    if (is_leap && month >= 2) t+=86400;
    return t;
}

long long __tm_to_secs(const tm *tm)
{
    int is_leap;
    long long year = tm->tm_year;
    int month = tm->tm_mon;
    if (month >= 12 || month < 0) {
        int adj = month / 12;
        month %= 12;
        if (month < 0) {
            adj--;
            month += 12;
        }
        year += adj;
    }
    long long t = __year_to_secs(year, &is_leap);
    t += __month_to_secs(month, is_leap);
    t += 86400LL * (tm->tm_mday-1);
    t += 3600LL * tm->tm_hour;
    t += 60LL * tm->tm_min;
    t += tm->tm_sec;
    return t;
}

static const tjs_char c_time[] = 
    TJS_W("Sun\0" ) TJS_W("Mon\0") TJS_W( "Tue\0" ) TJS_W("Wed\0" ) TJS_W("Thu\0" ) TJS_W("Fri\0" ) TJS_W("Sat\0")
    TJS_W("Sunday\0") TJS_W( "Monday\0") TJS_W( "Tuesday\0" ) TJS_W("Wednesday\0")
    TJS_W("Thursday\0") TJS_W( "Friday\0") TJS_W("Saturday\0")
    TJS_W("Jan\0") TJS_W( "Feb\0") TJS_W( "Mar\0") TJS_W("Apr\0" ) TJS_W("May\0" ) TJS_W("Jun\0")
    TJS_W("Jul\0") TJS_W( "Aug\0") TJS_W( "Sep\0") TJS_W("Oct\0" ) TJS_W("Nov\0" ) TJS_W("Dec\0")
    TJS_W("January\0" ) TJS_W(  "February\0") TJS_W( "March\0"    ) TJS_W("April\0"     )
    TJS_W("May\0"   ) TJS_W(    "June\0"    ) TJS_W( "July\0"     ) TJS_W("August\0"    )
    TJS_W("September\0" ) TJS_W("October\0" ) TJS_W( "November\0" ) TJS_W("December\0"  )
    TJS_W("AM\0") TJS_W("PM\0")
    TJS_W("%a %b %e %T %Y\0")
    TJS_W("%m/%d/%y\0"      )
    TJS_W("%H:%M:%S\0"      )
    TJS_W("%I:%M:%S %p\0"   )
    TJS_W("\0"              )
    TJS_W("%m/%d/%y\0"      )
    TJS_W("0123456789"      )
    TJS_W("%a %b %e %T %Y\0")
    TJS_W("%H:%M:%S"        );

static const tjs_char c_messages[] = TJS_W("^[yY]\0") TJS_W( "^[nN]");
static const tjs_char c_numeric[] = TJS_W(".\0") TJS_W( "");

const tjs_char *__nl_langinfo_l(int item/*, locale_t loc*/)
{
    int cat = item >> 16;
    int idx = item & 65535;
    const tjs_char *str;

    if (item == CODESET) return TJS_W("UTF-8");

    switch (cat) {
    case LC_NUMERIC:
        if (idx > 1) return NULL;
        str = c_numeric;
        break;
    case 2:
        if (idx > 0x31) return NULL;
        str = c_time;
        break;
    case LC_MONETARY:
        if (idx > 0) return NULL;
        str = TJS_W("");
        break;
//     case LC_MESSAGES:
//         if (idx > 1) return NULL;
//         str = c_messages;
//         break;
    default:
        return NULL;
    }

    for (; idx; idx--, str++) for (; *str; str++);
    return (tjs_char *)str;
}

size_t __strftime_l(tjs_char *s, size_t n, const tjs_char *f, const tm *tm);
const tjs_char *__strftime_fmt_1(tjs_char (*s)[100], size_t *l, int f, const tm *tm)
{
    int item;
    long long val;
    const tjs_char *fmt;
    int width = 2;

    switch (f) {
    case 'a':
        item = ABDAY_1 + tm->tm_wday;
        goto nl_strcat;
    case 'A':
        item = DAY_1 + tm->tm_wday;
        goto nl_strcat;
    case 'h':
    case 'b':
        item = ABMON_1 + tm->tm_mon;
        goto nl_strcat;
    case 'B':
        item = MON_1 + tm->tm_mon;
        goto nl_strcat;
    case 'c':
        item = D_T_FMT;
        goto nl_strftime;
    case 'C':
        val = (1900LL+tm->tm_year) / 100;
        goto number;
    case 'd':
        val = tm->tm_mday;
        goto number;
    case 'D':
        fmt = TJS_W("%m/%d/%y");
        goto recu_strftime;
    case 'e':
        *l = snprintf(*s, sizeof *s, TJS_W("%2d"), tm->tm_mday);
        return *s;
    case 'F':
        fmt = TJS_W("%Y-%m-%d");
        goto recu_strftime;
    case 'g':
    case 'G':
        val = tm->tm_year + 1900LL;
        if (tm->tm_yday < 3 && week_num(tm) != 1) val--;
        else if (tm->tm_yday > 360 && week_num(tm) == 1) val++;
        if (f=='g') val %= 100;
        else width = 4;
        goto number;
    case 'H':
        val = tm->tm_hour;
        goto number;
    case 'I':
        val = tm->tm_hour;
        if (!val) val = 12;
        else if (val > 12) val -= 12;
        goto number;
    case 'j':
        val = tm->tm_yday+1;
        width = 3;
        goto number;
    case 'm':
        val = tm->tm_mon+1;
        goto number;
    case 'M':
        val = tm->tm_min;
        goto number;
    case 'n':
        *l = 1;
        return TJS_W("\n");
    case 'p':
        item = tm->tm_hour >= 12 ? PM_STR : AM_STR;
        goto nl_strcat;
    case 'r':
        item = T_FMT_AMPM;
        goto nl_strftime;
    case 'R':
        fmt = TJS_W("%H:%M");
        goto recu_strftime;
    case 's':
        val = __tm_to_secs(tm) /*+ tm->__tm_gmtoff*/;
        goto number;
    case 'S':
        val = tm->tm_sec;
        goto number;
    case 't':
        *l = 1;
        return TJS_W("\t");
    case 'T':
        fmt = TJS_W("%H:%M:%S");
        goto recu_strftime;
    case 'u':
        val = tm->tm_wday ? tm->tm_wday : 7;
        width = 1;
        goto number;
    case 'U':
        val = (tm->tm_yday + 7 - tm->tm_wday) / 7;
        goto number;
    case 'W':
        val = (tm->tm_yday + 7 - (tm->tm_wday+6)%7) / 7;
        goto number;
    case 'V':
        val = week_num(tm);
        goto number;
    case 'w':
        val = tm->tm_wday;
        width = 1;
        goto number;
    case 'x':
        item = D_FMT;
        goto nl_strftime;
    case 'X':
        item = T_FMT;
        goto nl_strftime;
    case 'y':
        val = tm->tm_year % 100;
        goto number;
    case 'Y':
        val = tm->tm_year + 1900;
        if (val >= 10000) {
            *l = snprintf(*s, sizeof *s, TJS_W("+%lld"), val);
            return *s;
        }
        width = 4;
        goto number;
//     case 'z':
//         if (tm->tm_isdst < 0) {
//             *l = 0;
//             return "";
//         }
//         *l = snprintf(*s, sizeof *s, "%+.2d%.2d",
//             (-tm->__tm_gmtoff)/3600,
//             abs(tm->__tm_gmtoff%3600)/60);
//         return *s;
//     case 'Z':
//         if (tm->tm_isdst < 0) {
//             *l = 0;
//             return "";
//         }
//         fmt = __tm_to_tzname(tm);
//         goto string;
    case '%':
        *l = 1;
        return TJS_W("%");
    default:
        return 0;
    }
number:
    *l = snprintf(*s, sizeof *s, TJS_W("%0*lld"), width, val);
    return *s;
nl_strcat:
    fmt = __nl_langinfo_l(item);
string:
    *l = TJS_strlen(fmt);
    return fmt;
nl_strftime:
    fmt = __nl_langinfo_l(item);
recu_strftime:
    *l = __strftime_l(*s, sizeof *s, fmt, tm);
    if (!*l) return 0;
    return *s;
}
/* Lookup table for digit values. -1==255>=36 -> invalid */
static const unsigned char table[] = { 255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,255,255,255,255,255,255,
    255,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
    25,26,27,28,29,30,31,32,33,34,35,255,255,255,255,255,
    255,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
    25,26,27,28,29,30,31,32,33,34,35,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};
int shgetc(_tFILE *f) {return *f->p++;}
void shunget(_tFILE *f) {f->p--;}
unsigned long long __intscan(_tFILE *f, unsigned base, int pok, unsigned long long lim)
{
    const unsigned char *val = table+1;
    int c, neg=0;
    unsigned x;
    unsigned long long y;
    if (base > 36) {
        errno = EINVAL;
        return 0;
    }
    while (isspace((c=shgetc(f))));
    if (c=='+' || c=='-') {
        neg = -(c=='-');
        c = shgetc(f);
    }
    if ((base == 0 || base == 16) && c=='0') {
        c = shgetc(f);
        if ((c|32)=='x') {
            c = shgetc(f);
            if (val[c]>=16) {
                shunget(f);
                if (pok) shunget(f);
//                else shlim(f, 0);
                return 0;
            }
            base = 16;
        } else if (base == 0) {
            base = 8;
        }
    } else {
        if (base == 0) base = 10;
        if (val[c] >= base) {
            shunget(f);
//            shlim(f, 0);
            errno = EINVAL;
            return 0;
        }
    }
    if (base == 10) {
        for (x=0; c-'0'<10U && x<=UINT_MAX/10-1; c=shgetc(f))
            x = x*10 + (c-'0');
        for (y=x; c-'0'<10U && y<=ULLONG_MAX/10 && 10*y<=ULLONG_MAX-(c-'0'); c=shgetc(f))
            y = y*10 + (c-'0');
        if (c-'0'>=10U) goto done;
    } else if (!(base & base-1)) {
        int bs = "\0\1\2\4\7\3\6\5"[(0x17*base)>>5&7];
        for (x=0; val[c]<base && x<=UINT_MAX/32; c=shgetc(f))
            x = x<<bs | val[c];
        for (y=x; val[c]<base && y<=ULLONG_MAX>>bs; c=shgetc(f))
            y = y<<bs | val[c];
    } else {
        for (x=0; val[c]<base && x<=UINT_MAX/36-1; c=shgetc(f))
            x = x*base + val[c];
        for (y=x; val[c]<base && y<=ULLONG_MAX/base && base*y<=ULLONG_MAX-val[c]; c=shgetc(f))
            y = y*base + val[c];
    }
    if (val[c]<base) {
        for (; val[c]<base; c=shgetc(f));
        errno = ERANGE;
        y = lim;
    }
done:
    shunget(f);
    if (y>=lim) {
        if (!(lim&1) && !neg) {
            errno = ERANGE;
            return lim-1;
        } else if (y>lim) {
            errno = ERANGE;
            return lim;
        }
    }
    return (y^neg)-neg;
}

static unsigned long long strtox(const tjs_char *s, tjs_char **p, int base, unsigned long long lim)
{
    /* FIXME: use a helper function or macro to setup the FILE */
    _tFILE f = {(tjs_char*)s};
    unsigned long long ret = __intscan(&f, base, 1, lim);
    if(p) *p = f.p;
    return ret;
}
size_t __strftime_l(tjs_char *s, size_t n, const tjs_char *f, const tm *tm)
{
    size_t l, k;
    tjs_char buf[100];
    tjs_char *p;
    const tjs_char *t;
    int plus;
    unsigned long width;
    for (l=0; l+1<n; f++) {
        if (!*f) {
            s[l] = 0;
            return l;
        }
        if (*f != '%') {
            s[l++] = *f;
            continue;
        }
        f++;
        if ((plus = (*f == '+'))) f++;
        width = strtox(f, &p, 10, -999);
        if (*p == 'C' || *p == 'F' || *p == 'G' || *p == 'Y') {
            if (!width && p!=f) width = 1;
            if (width >= n-l) return 0;
        } else {
            width = 0;
        }
        f = p;
        if (*f == 'E' || *f == 'O') f++;
        t = __strftime_fmt_1(&buf, &k, *f, tm);
        if (!t) return 0;
        if (width) {
            for (; *t=='+' || *t=='-' || (*t=='0'&&t[1]); t++, k--);
            width--;
            if (plus && tm->tm_year >= 10000-1900)
                s[l++] = '+';
            else if (tm->tm_year < -1900)
                s[l++] = '-';
            else
                width++;
            if (width >= n-l) return 0;
            for (; width > k; width--)
                s[l++] = '0';
        }
        if (k >= n-l) return 0;
        memcpy(s+l, t, k * sizeof(*t));
        l += k;
    }
    return 0;
}

double TJS_strtod(const tjs_char* string, tjs_char** endPtr)
{
#if defined (_MSC_VER) && defined(_DEBUG)
	double testret = wcstod(string, endPtr);
#endif
	int sign, expSign = false;
	double fraction, dblExp;
	const double *d;
	const tjs_char *p;
	int c;
	int exp = 0;		/* Exponent read from "EX" field. */
	int fracExp = 0;		/* Exponent that derives from the fractional
							* part.  Under normal circumstatnces, it is
							* the negative of the number of digits in F.
							* However, if I is very long, the last digits
							* of I get dropped (otherwise a long I with a
							* large negative exponent could cause an
							* unnecessary overflow on I alone).  In this
							* case, fracExp is incremented one for each
							* dropped digit. */
	int mantSize;		/* Number of digits in mantissa. */
	int decPt;			/* Number of mantissa digits BEFORE decimal
						* point. */
	const tjs_char *pExp;		/* Temporarily holds location of exponent
							* in string. */
	static const int maxExponent = 511;
	static const double powersOf10[] = {	/* Table giving binary powers of 10.  Entry */
		10.,			/* is 10^2^i.  Used to convert decimal */
		100.,			/* exponents into floating-point numbers. */
		1.0e4,
		1.0e8,
		1.0e16,
		1.0e32,
		1.0e64,
		1.0e128,
		1.0e256
	};
	/*
	* Strip off leading blanks and check for a sign.
	*/

	p = string;
	while (isspace((*p))) {
		p += 1;
	}
	if (*p == '-') {
		sign = true;
		p += 1;
	} else {
		if (*p == '+') {
			p += 1;
		}
		sign = false;
	}

	/*
	* Count the number of digits in the mantissa (including the decimal
	* point), and also locate the decimal point.
	*/

	decPt = -1;
	for (mantSize = 0;; mantSize += 1)
	{
		c = *p;
		if (!isdigit(c)) {
			if ((c != '.') || (decPt >= 0)) {
				break;
			}
			decPt = mantSize;
		}
		p += 1;
	}

	/*
	* Now suck up the digits in the mantissa.  Use two integers to
	* collect 9 digits each (this is faster than using floating-point).
	* If the mantissa has more than 18 digits, ignore the extras, since
	* they can't affect the value anyway.
	*/

	pExp = p;
	p -= mantSize;
	if (decPt < 0) {
		decPt = mantSize;
	} else {
		mantSize -= 1;			/* One of the digits was the point. */
	}
	if (mantSize > 48) {
		fracExp = decPt - 48;
		mantSize = 48;
	} else {
		fracExp = decPt - mantSize;
	}
	if (mantSize == 0) {
		fraction = 0.0;
		p = string;
		goto done;
	} else {
		int frac1, frac2;
		frac1 = 0;
		for (; mantSize > 9; mantSize -= 1)
		{
			c = *p;
			p += 1;
			if (c == '.') {
				c = *p;
				p += 1;
			}
			frac1 = 10 * frac1 + (c - '0');
		}
		frac2 = 0;
		for (; mantSize > 0; mantSize -= 1)
		{
			c = *p;
			p += 1;
			if (c == '.') {
				c = *p;
				p += 1;
			}
			frac2 = 10 * frac2 + (c - '0');
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/*
	* Skim off the exponent.
	*/

	p = pExp;
	if ((*p == 'E') || (*p == 'e')) {
		p += 1;
		if (*p == '-') {
			expSign = true;
			p += 1;
		} else {
			if (*p == '+') {
				p += 1;
			}
			expSign = false;
		}
		if (!isdigit((*p))) {
			p = pExp;
			goto done;
		}
		while (isdigit((*p))) {
			exp = exp * 10 + (*p - '0');
			p += 1;
		}
	}
	if (expSign) {
		exp = fracExp - exp;
	} else {
		exp = fracExp + exp;
	}

	/*
	* Generate a floating-point number that represents the exponent.
	* Do this by processing the exponent one bit at a time to combine
	* many powers of 2 of 10. Then combine the exponent with the
	* fraction.
	*/

	if (exp < 0) {
		expSign = true;
		exp = -exp;
	} else {
		expSign = false;
	}
	if (exp > maxExponent) {
		exp = maxExponent;
		errno = ERANGE;
	}
	dblExp = 1.0;
	for (d = powersOf10; exp != 0; exp >>= 1, d += 1) {
		if (exp & 01) {
			dblExp *= *d;
		}
	}
	if (expSign) {
		fraction /= dblExp;
	} else {
		fraction *= dblExp;
	}

done:
	if (endPtr != NULL) {
		*endPtr = (tjs_char*)p;
	}

	if (sign) {
		return -fraction;
	}
#if defined (_MSC_VER) && defined(_DEBUG)
	assert(fraction == testret);
#endif
	return fraction;
}

size_t TJS_strftime( tjs_char *wstring, size_t maxsize, const tjs_char *wformat, const tm *timeptr )
{
    return __strftime_l(wstring, maxsize, wformat, (tm*)timeptr);
}

int TJS_vsnprintf( tjs_char *string, size_t count, const tjs_char *format, va_list ap )
{
    
    return _vsnprintf(string, count, format, ap);
}

tjs_int TJS_snprintf(tjs_char *s, size_t count, const tjs_char *format, ...)
{
	tjs_int r;
	va_list param;
	va_start(param, format);
	r = TJS_vsnprintf(s, count, format, param);
	va_end(param);
	return r;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
} // namespace TJS



