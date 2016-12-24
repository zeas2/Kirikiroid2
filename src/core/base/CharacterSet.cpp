//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Character code conversion
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "CharacterSet.h"
#include "MsgIntf.h"
//---------------------------------------------------------------------------
static tjs_int inline TVPWideCharToUtf8(tjs_char in, char * out)
{
	// convert a wide character 'in' to utf-8 character 'out'
	if     (in < (1<< 7))
	{
		if(out)
		{
			out[0] = (char)in;
		}
		return 1;
	}
	else if(in < (1<<11))
	{
		if(out)
		{
			out[0] = (char)(0xc0 | (in >> 6));
			out[1] = (char)(0x80 | (in & 0x3f));
		}
		return 2;
	}
	else if(in < (1<<16))
	{
		if(out)
		{
			out[0] = (char)(0xe0 | (in >> 12));
			out[1] = (char)(0x80 | ((in >> 6) & 0x3f));
			out[2] = (char)(0x80 | (in & 0x3f));
		}
		return 3;
	}
#if 1
	else
	{
		TVPThrowExceptionMessage(TJS_W("Out of UTF-16 range conversion from UTF-8 code"));
	}
#else
	// 以下オリジナルのコードだけど、通らないはず。
	else if(in < (1<<21))
	{
		if(out)
		{
			out[0] = (char)(0xf0 | (in >> 18));
			out[1] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 6 ) & 0x3f));
			out[3] = (char)(0x80 | (in & 0x3f));
		}
		return 4;
	}
	else if(in < (1<<26))
	{
		if(out)
		{
			out[0] = (char)(0xf8 | (in >> 24));
			out[1] = (char)(0x80 | ((in >> 16) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[3] = (char)(0x80 | ((in >> 6 ) & 0x3f));
			out[4] = (char)(0x80 | (in & 0x3f));
		}
		return 5;
	}
	else if(in < (1<<31))
	{
		if(out)
		{
			out[0] = (char)(0xfc | (in >> 30));
			out[1] = (char)(0x80 | ((in >> 24) & 0x3f));
			out[2] = (char)(0x80 | ((in >> 18) & 0x3f));
			out[3] = (char)(0x80 | ((in >> 12) & 0x3f));
			out[4] = (char)(0x80 | ((in >> 6 ) & 0x3f));
			out[5] = (char)(0x80 | (in & 0x3f));
		}
		return 6;
	}
#endif
	return -1;
}
//---------------------------------------------------------------------------
tjs_int TVPWideCharToUtf8String(const tjs_char *in, char * out)
{
	// convert input wide string to output utf-8 string
	int count = 0;
	while(*in)
	{
		tjs_int n;
		if(out)
		{
			n = TVPWideCharToUtf8(*in, out);
			out += n;
		}
		else
		{
			n = TVPWideCharToUtf8(*in, NULL);
				/*
					in this situation, the compiler's inliner
					will collapse all null check parts in
					TVPWideCharToUtf8.
				*/
		}
		if(n == -1) return -1; // invalid character found
		count += n;
		in++;
	}
	return count;
}
//---------------------------------------------------------------------------
static bool inline TVPUtf8ToWideChar(const char * & in, tjs_char *out)
{
	// convert a utf-8 charater from 'in' to wide charater 'out'
	const unsigned char * & p = (const unsigned char * &)in;
	if(p[0] < 0x80)
	{
		if(out) *out = (tjs_char)in[0];
		in++;
		return true;
	}
	else if(p[0] < 0xc2)
	{
		// invalid character
		return false;
	}
	else if(p[0] < 0xe0)
	{
		// two bytes (11bits)
		if((p[1] & 0xc0) != 0x80) return false;
		if(out) *out = ((p[0] & 0x1f) << 6) + (p[1] & 0x3f);
		in += 2;
		return true;
	}
	else if(p[0] < 0xf0)
	{
		// three bytes (16bits)
		if((p[1] & 0xc0) != 0x80) return false;
		if((p[2] & 0xc0) != 0x80) return false;
		if(out) *out = ((p[0] & 0x1f) << 12) + ((p[1] & 0x3f) << 6) + (p[2] & 0x3f);
		in += 3;
		return true;
	}
	else if(p[0] < 0xf8)
	{
		// four bytes (21bits)
		if((p[1] & 0xc0) != 0x80) return false;
		if((p[2] & 0xc0) != 0x80) return false;
		if((p[3] & 0xc0) != 0x80) return false;
		if(out) *out = ((p[0] & 0x07) << 18) + ((p[1] & 0x3f) << 12) +
			((p[2] & 0x3f) << 6) + (p[3] & 0x3f);
		in += 4;
		return true;
	}
	else if(p[0] < 0xfc)
	{
		// five bytes (26bits)
		if((p[1] & 0xc0) != 0x80) return false;
		if((p[2] & 0xc0) != 0x80) return false;
		if((p[3] & 0xc0) != 0x80) return false;
		if((p[4] & 0xc0) != 0x80) return false;
		if(out) *out = ((p[0] & 0x03) << 24) + ((p[1] & 0x3f) << 18) +
			((p[2] & 0x3f) << 12) + ((p[3] & 0x3f) << 6) + (p[4] & 0x3f);
		in += 5;
		return true;
	}
	else if(p[0] < 0xfe)
	{
		// six bytes (31bits)
		if((p[1] & 0xc0) != 0x80) return false;
		if((p[2] & 0xc0) != 0x80) return false;
		if((p[3] & 0xc0) != 0x80) return false;
		if((p[4] & 0xc0) != 0x80) return false;
		if((p[5] & 0xc0) != 0x80) return false;
		if(out) *out = ((p[0] & 0x01) << 30) + ((p[1] & 0x3f) << 24) +
			((p[2] & 0x3f) << 18) + ((p[3] & 0x3f) << 12) +
			((p[4] & 0x3f) << 6) + (p[5] & 0x3f);
		in += 6;
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
tjs_int TVPUtf8ToWideCharString(const char * in, tjs_char *out)
{
	// convert input utf-8 string to output wide string
	int count = 0;
	while(*in)
	{
		tjs_char c;
		if(out)
		{
			if(!TVPUtf8ToWideChar(in, &c))
				return -1; // invalid character found
			*out++ = c;
		}
		else
		{
			if(!TVPUtf8ToWideChar(in, NULL))
				return -1; // invalid character found
		}
		count ++;
	}
	return count;
}
//---------------------------------------------------------------------------
tjs_int TVPUtf8ToWideCharString(const char * in, tjs_uint length, tjs_char *out)
{
	// convert input utf-8 string to output wide string
	int count = 0;
	const char *end = in + length;
	while(*in && in < end)
	{
		if(in + 6 > end)
		{
			// fetch utf-8 character length
			const unsigned char ch = *(const unsigned char *)in;

			if(ch >= 0x80)
			{
				tjs_uint len = 0;

				if(ch < 0xc2) return -1;
				else if(ch < 0xe0) len = 2;
				else if(ch < 0xf0) len = 3;
				else if(ch < 0xf8) len = 4;
				else if(ch < 0xfc) len = 5;
				else if(ch < 0xfe) len = 6;
				else return -1;

				if(in + len > end) return -1;
			}
		}

		tjs_char c;
		if(out)
		{
			if(!TVPUtf8ToWideChar(in, &c))
				return -1; // invalid character found
			*out++ = c;
		}
		else
		{
			if(!TVPUtf8ToWideChar(in, NULL))
				return -1; // invalid character found
		}
		count ++;
	}
	return count;
}
//---------------------------------------------------------------------------

