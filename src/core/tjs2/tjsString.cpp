//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// tTJSVariant friendly string class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <stdarg.h>
#include <stdio.h>


#include "tjsString.h"
#include "tjsVariant.h"

#define TJS_TTSTR_SPRINTF_BUF_SIZE 8192


namespace TJS
{
const tjs_char *TJSNullStrPtr = TJS_W("");
//---------------------------------------------------------------------------
tTJSString::tTJSString(const tTJSVariant & val)
{
	Ptr = val.AsString();
}
//---------------------------------------------------------------------------
tTJSString::tTJSString(tjs_int n) // from int
{
	Ptr = TJSIntegerToString(n);
}
//---------------------------------------------------------------------------
tTJSString::tTJSString(tjs_int64 n) // from int64
{
	Ptr = TJSIntegerToString(n);
}
//---------------------------------------------------------------------------
tjs_int tTJSString::GetNarrowStrLen() const
{
	// note that this function will return -1 when there are invalid chars in string.
	if(!Ptr) return 0;
	return (tjs_int)TJS_wcstombs(NULL, c_str(), 0);
}
//---------------------------------------------------------------------------
void tTJSString::ToNarrowStr(tjs_nchar *dest, tjs_int destmaxlen) const
{
	// dest must be an array of char, its size must be at least destmaxlen+1
	dest[TJS_wcstombs(dest, c_str(), destmaxlen)] = 0;
}
//---------------------------------------------------------------------------
tjs_char * tTJSString::InternalIndepend()
{
	// severs sharing of the string instance
	// and returns independent internal buffer

	tTJSVariantString *newstr =
		TJSAllocVariantString(Ptr->operator const tjs_char*());

	Ptr->Release();
	Ptr = newstr;

	return const_cast<tjs_char *>(newstr->operator const tjs_char*());
}
//---------------------------------------------------------------------------
tjs_int64 tTJSString::AsInteger() const
{
	return Ptr->ToInteger();
}
//---------------------------------------------------------------------------
void tTJSString::Replace(const tTJSString &from, const tTJSString &to, bool forall)
{
	// replaces the string partial "from", to "to".
	// all "from" are replaced when "forall" is true.
	if(IsEmpty()) return;
	if(from.IsEmpty()) return;

	tjs_int fromlen = from.GetLen();

	for(;;)
	{
		const tjs_char *st;
		const tjs_char *p;
		st = c_str();
		p = TJS_strstr(st, from.c_str());
		if(p)
		{
			tTJSString name(*this, (int)(p-st));
			tTJSString n2(p + fromlen);
			*this = name + to + n2;
			if(!forall) break;
		}
		else
		{
			break;
		}
	}
}
//---------------------------------------------------------------------------
tTJSString tTJSString::AsLowerCase() const
{
	tjs_int len = GetLen();

	if(len == 0) return tTJSString();

	tTJSString ret((tTJSStringBufferLength)(len));

	const tjs_char *s = c_str();
	tjs_char *d = ret.Independ();
	while(*s)
	{
		if(*s >= TJS_W('A') && *s <= TJS_W('Z'))
			*d = *s +(TJS_W('a')-TJS_W('A'));
		else
			*d = *s;
		d++;
		s++;
	}

	return ret;
}
//---------------------------------------------------------------------------
tTJSString tTJSString::AsUpperCase() const
{
	tjs_int len = GetLen();

	if(len == 0) return tTJSString();

	tTJSString ret((tTJSStringBufferLength)(len));

	const tjs_char *s = c_str();
	tjs_char *d = ret.Independ();
	while(*s)
	{
		if(*s >= TJS_W('a') && *s <= TJS_W('z'))
			*d = *s +(TJS_W('A')-TJS_W('a'));
		else
			*d = *s;
		d++;
		s++;
	}

	return ret;
}
//---------------------------------------------------------------------------
void tTJSString::ToLowerCase()
{
	tjs_char *p = Independ();
	if(p)
	{
		while(*p)
		{
			if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
				*p += (TJS_W('a')-TJS_W('A'));
			p++;
		}
	}
}
//---------------------------------------------------------------------------
void tTJSString::ToUppserCase()
{
	tjs_char *p = Independ();
	if(p)
	{
		while(*p)
		{
			if(*p >= TJS_W('a') && *p <= TJS_W('z'))
				*p += (TJS_W('A')-TJS_W('a'));
			p++;
		}
	}
}
//---------------------------------------------------------------------------
tjs_int TJS_cdecl tTJSString::printf(const tjs_char *format, ...)
{
	tjs_int r;
	tjs_char *buf = new tjs_char [TJS_TTSTR_SPRINTF_BUF_SIZE];
	try
	{
		tjs_int size = TJS_TTSTR_SPRINTF_BUF_SIZE-1; /*TJS_vsnprintf(NULL, 0, format, param);*/
		va_list param;
		va_start(param, format);
		r = TJS_vsnprintf(buf, size, format, param);
		AllocBuffer(r);
		if(r)
		{
			TJS_strcpy(const_cast<tjs_char*>(c_str()), buf);
		}
		va_end(param);
		FixLen();
	}
	catch(...)
	{
		delete [] buf;
		throw;
	}
	delete [] buf;
	return r;
}
//---------------------------------------------------------------------------
tTJSString tTJSString::EscapeC() const
{
	ttstr ret;
	const tjs_char * p = c_str();
	bool hexflag = false;
	for(;*p;p++)
	{
		switch(*p)
		{
		case 0x07: ret += TJS_W("\\a"); hexflag = false; continue;
		case 0x08: ret += TJS_W("\\b"); hexflag = false; continue;
		case 0x0c: ret += TJS_W("\\f"); hexflag = false; continue;
		case 0x0a: ret += TJS_W("\\n"); hexflag = false; continue;
		case 0x0d: ret += TJS_W("\\r"); hexflag = false; continue;
		case 0x09: ret += TJS_W("\\t"); hexflag = false; continue;
		case 0x0b: ret += TJS_W("\\v"); hexflag = false; continue;
		case TJS_W('\\'): ret += TJS_W("\\\\"); hexflag = false; continue;
		case TJS_W('\''): ret += TJS_W("\\\'"); hexflag = false; continue;
		case TJS_W('\"'): ret += TJS_W("\\\""); hexflag = false; continue;
		default:
			if(hexflag)
			{
				if((*p >= TJS_W('a') && *p <= TJS_W('f')) ||
					(*p >= TJS_W('A') && *p <= TJS_W('F')) ||
						(*p >= TJS_W('0') && *p <= TJS_W('9')) )
				{
					tjs_char buf[20];
					TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), TJS_W("\\x%02x"), (int)*p);
					hexflag = true;
					ret += buf;
					continue;
				}
			}

			if(*p < 0x20)
			{
				tjs_char buf[20];
				TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), TJS_W("\\x%02x"), (int)*p);
				hexflag = true;
				ret += buf;
			}
			else
			{
				ret += *p;
				hexflag = false;
			}
		}
	}
	return ret;
}
//---------------------------------------------------------------------------
tTJSString tTJSString::UnescapeC() const
{
	// TODO: UnescapeC
	return TJS_W("");
}
//---------------------------------------------------------------------------
bool tTJSString::StartsWith(const tjs_char *string) const
{
	// return true if this starts with "string"
	if(!Ptr)
	{
		if(!*string) return true; // empty string starts with empty string
		return false;
	}
	const tjs_char *this_p = *Ptr;
	while(*string && *this_p)
	{
		if(*string != *this_p) return false;
		string++, this_p++;
	}
	if(!*string) return true;
	return false;
}

TJS::tTJSString tTJSString::SubString( unsigned int pos, unsigned int len ) const
{
    if(Ptr == NULL || len == 0 || pos >= Ptr->GetLength()) return tTJSString();
    if(pos == 0 && len >= Ptr->GetLength()) return tTJSString(*this);
    return tTJSString(Ptr->operator const tjs_char *() + pos, len);
}

TJS::tTJSString tTJSString::Trim()
{
    const tjs_char * p = c_str();
    while( *p > '\0' && *p < 0x20 )
        p++;

    tTJSString _str(p);
    tjs_char * p0 = (tjs_char *)_str.c_str();
    tjs_char * p1 = (tjs_char *)_str.c_str() + _str.length() - 1;
    while( p0 < p1 && *p1 != '\0' && *p1 < 0x20 )
        *p1-- = '\0';
    _str.Ptr->FixLength();
    return _str;
}

int tTJSString::IndexOf( const tTJSString& str, unsigned int pos /*= 0*/ ) const
{
    if(!Ptr || !str.Ptr) return -1;
    //    if(str.length() == 0 || pos >= length()) return -1;
    const tjs_char *p = TJS_strstr(Ptr->operator const tjs_char *() + pos, str.Ptr->operator const tjs_char *());
    if(p == NULL) return -1;
    return p - Ptr->operator const tjs_char*();
}
#if 0
const std::string tTJSString::AsStdString() const {
	if (!Ptr) return "";
	// this constant string value must match std::string in type
	const tjs_char * wide = Ptr->operator const tjs_char*();
	int n = TJS_wcstombs(nullptr, wide, 0);
	if (n == -1) return "";
	std::string ret;
	ret.resize(n);
	TJS_wcstombs((char*)ret.c_str(), wide, n);
	return ret;
}
#endif
//---------------------------------------------------------------------------
tTJSString operator + (const tjs_char *lhs, const tTJSString &rhs)
{
	tTJSString ret(lhs);
	ret += rhs;
	return ret;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
tTJSString TJSInt32ToHex(tjs_uint32 num, int zeropad)
{
	// convert given number to HEX string.
	// zeros are padded when the output string's length is smaller than "zeropad".
	// "zeropad" cannot be larger than 8.
	if(zeropad > 8) zeropad = 8;

	tjs_char buf[12];
	tjs_char buf2[12];

	tjs_char *p = buf;
	tjs_char *d = buf2;

	do
	{
		*(p++) = (TJS_W("0123456789ABCDEF"))[num % 16];
		num /= 16;
		zeropad --;
	} while(zeropad || num);

	p--;
	while(buf <= p) *(d++) = *(p--);
	*d = 0;

	return ttstr(buf2); 
}
//---------------------------------------------------------------------------
} // namespace TJS

