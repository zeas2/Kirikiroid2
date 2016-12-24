//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS2 lexical analyzer
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <math.h>
#include "tjsInterCodeGen.h"
#include "tjs.tab.hpp"
#include "tjsLex.h"
#include "tjsVariant.h"
#include "tjsError.h"
#include "tjsCompileControl.h"
#include "tjsScriptBlock.h"
#include "tjsObject.h"
#include "tjsMath.h"

namespace TJS
{


tjs_nchar tjsEnableDicFuncQuickHack_mark[] = " - 0 <- Put '1' to enable dicfunc quick-hack - ";
bool tjsEnableDicFuncQuickHack = tjsEnableDicFuncQuickHack_mark[3] != '0';
	//----- dicfunc quick-hack

//---------------------------------------------------------------------------
const tjs_char TJS_SKIP_CODE = (tjs_char)~((tjs_char)0);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJS_iswspace or etc. ( these functions do not collate )
//---------------------------------------------------------------------------
static bool inline TJS_iswspace(tjs_char ch)
{
	if(ch&0xff00) return false; else return 0!=isspace(ch);
}
//---------------------------------------------------------------------------
static bool inline TJS_iswdigit(tjs_char ch)
{
	if(ch&0xff00) return false; else return 0!=isdigit(ch);
}
//---------------------------------------------------------------------------
static bool inline TJS_iswalpha(tjs_char ch)
{
	if(ch&0xff00) return true; else return 0!=isalpha(ch);
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TJSNext
//---------------------------------------------------------------------------
static void _TJSNext(const tjs_char **ptr)
{
	do
	{
		(*ptr)++;
	} while(*(*ptr) && *(*ptr)==TJS_SKIP_CODE);
}

bool inline TJSNext(const tjs_char **ptr)
{
	(*ptr)++;
	if(*(*ptr) == TJS_SKIP_CODE) _TJSNext(ptr);
	return *(*ptr)!=0;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSSkipSpace
//---------------------------------------------------------------------------
bool TJSSkipSpace(const tjs_char **ptr)
{
	while(*(*ptr) && (*(*ptr)==TJS_SKIP_CODE || TJS_iswspace(*(*ptr))))
		(*ptr)++;
	return *(*ptr)!=0;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSHexNum
tjs_int TJSHexNum(tjs_char ch) throw()
{
	if(ch>=TJS_W('a') && ch<=TJS_W('f')) return ch-TJS_W('a')+10;
	if(ch>=TJS_W('A') && ch<=TJS_W('F')) return ch-TJS_W('A')+10;
	if(ch>=TJS_W('0') && ch<=TJS_W('9')) return ch-TJS_W('0');
	return -1;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSOctNum
//---------------------------------------------------------------------------
tjs_int TJSOctNum(tjs_char ch) throw()
{
	if(ch>=TJS_W('0') && ch<=TJS_W('7')) return ch-TJS_W('0');
	return -1;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSDecNum
//---------------------------------------------------------------------------
tjs_int TJSDecNum(tjs_char ch) throw()
{
	if(ch>=TJS_W('0') && ch<=TJS_W('9')) return ch-TJS_W('0');
	return -1;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSBinNum
//---------------------------------------------------------------------------
tjs_int TJSBinNum(tjs_char ch) throw()
{
	if(ch==TJS_W('0')) return 0;
	if(ch==TJS_W('1')) return 1;
	return -1;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSUnescapeBackSlash
//---------------------------------------------------------------------------
tjs_int TJSUnescapeBackSlash(tjs_char ch) throw()
{
	// convert "\?"
	// ch must indicate "?"
	switch(ch)
	{
	case TJS_W('a'): return 0x07;
	case TJS_W('b'): return 0x08;
	case TJS_W('f'): return 0x0c;
	case TJS_W('n'): return 0x0a;
	case TJS_W('r'): return 0x0d;
	case TJS_W('t'): return 0x09;
	case TJS_W('v'): return 0x0b;
	default : return ch;
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSSkipComment
//---------------------------------------------------------------------------
static tTJSSkipCommentResult TJSSkipComment(const tjs_char **ptr)
{
	if((*ptr)[0] != TJS_W('/')) return scrNotComment;

	if((*ptr)[1] == TJS_W('/'))
	{
		// line comment; skip to newline
		while(*(*ptr)!=TJS_W('\n')) if(!TJSNext(&(*ptr))) break;
		if(*(*ptr) ==0) return scrEnded;
		(*ptr)++;
		TJSSkipSpace(&(*ptr));
		if(*(*ptr) ==0) return scrEnded;

		return scrContinue;
	}
	else if((*ptr)[1] == TJS_W('*'))
	{
		// block comment; skip to the next '*' '/'
		// and we must allow nesting of the comment.
		(*ptr) += 2;
		if(*(*ptr) == 0) TJS_eTJSError(TJSUnclosedComment);
		tjs_int level = 0;
		for(;;)
		{
			if((*ptr)[0] == TJS_W('/') && (*ptr)[1] == TJS_W('*'))
			{
				// note: we cannot avoid comment processing when the
				// nested comment is in string literals.
				level ++;
			}
			if((*ptr)[0] == TJS_W('*') && (*ptr)[1] == TJS_W('/'))
			{
				if(level == 0)
				{
					(*ptr) += 2;
					break;
				}
				level --;
			}
			if(!TJSNext(&(*ptr))) TJS_eTJSError(TJSUnclosedComment);
		}
		if(*(*ptr) ==0) return scrEnded;
		TJSSkipSpace(&(*ptr));
		if(*(*ptr) ==0) return scrEnded;

		return scrContinue;
	}

	return scrNotComment;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSStringMatch
//---------------------------------------------------------------------------
bool TJSStringMatch(const tjs_char **sc, const tjs_char *wrd, bool isword)
{
	// compare string with a script starting from sc and wrd.
	// word matching is processed if isword is true.
	const tjs_char *save = (*sc);
	while(*wrd && *(*sc))
	{
		if(*(*sc) != *wrd) break;
		TJSNext(sc);
		wrd++;
	}

	if(*wrd) { (*sc)=save; return false; }
	if(isword)
	{
		if(TJS_iswalpha(*(*sc)) || *(*sc) == TJS_W('_'))
			{ (*sc)=save; return false; }
	}
	return true;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSParseString
//---------------------------------------------------------------------------
enum tTJSInternalParseStringResult
{ psrNone, psrDelimiter, psrAmpersand, psrDollar };
static tTJSInternalParseStringResult
	TJSInternalParseString(tTJSVariant &val, const tjs_char **ptr,
		tjs_char delim, bool embexpmode)
{
	// delim1 must be '\'' or '"'
	// delim2 must be '&' or '\0'

	ttstr str;

	tTJSInternalParseStringResult status = psrNone;

	for(;*(*ptr);)
	{
		if(*(*ptr)==TJS_W('\\'))
		{
			// escape
			if(!TJSNext(ptr)) break;
			if(*(*ptr)==TJS_W('x') || *(*ptr)==TJS_W('X'))
			{
				// hex
				// starts with a "\x", be parsed while characters are
				// recognized as hex-characters, but limited of size of tjs_char.
				// on Windows, \xXXXXX will be parsed to UNICODE 16bit characters.
				if(!TJSNext(ptr)) break;
				tjs_int num;
				tjs_int code = 0;
				tjs_int count = 0;
				while((num = TJSHexNum(*(*ptr)))!=-1 && count<(sizeof(tjs_char)*2))
				{
					code*=16;
					code+=num;
					count ++;
					if(!TJSNext(ptr)) break;
				}
				if(*(*ptr) == 0) break;
				str+=(tjs_char)code;
			}
			else if(*(*ptr) == TJS_W('0'))
			{
				// octal
				if(!TJSNext(ptr)) break;

				tjs_int num;
				tjs_int code=0;
				while((num=TJSOctNum(*(*ptr)))!=-1)
				{
					code*=8;
					code+=num;
					if(!TJSNext(ptr)) break;
				}
				if(*(*ptr) == 0) break;
				str += (tjs_char)code;
			}
			else
			{
				str += (tjs_char)TJSUnescapeBackSlash(*(*ptr));
				TJSNext(ptr);
			}
		}
		else if(*(*ptr) == delim)
		{
			// string delimiters
			if(!TJSNext(ptr))
			{
				status = psrDelimiter;
				break;
			}

			const tjs_char *p=(*ptr);
			TJSSkipSpace(&p);
			if(*p == delim)
			{
				// sequence of 'A' 'B' will be combined as 'AB'
				(*ptr) = p;
				TJSNext(ptr);
			}
			else
			{
				status = psrDelimiter;
				break;
			}
		}
		else if(embexpmode && *(*ptr) == TJS_W('&'))
		{
			// '&'
			if(!TJSNext(ptr)) break;
			status = psrAmpersand;
			break;
		}
		else if(embexpmode && *(*ptr) == TJS_W('$'))
		{
			// '$'
			// '{' must be placed immediately after '$'
			const tjs_char *p = (*ptr);
			if(!TJSNext(ptr)) break;
			if(*(*ptr) == TJS_W('{'))
			{
				if(!TJSNext(ptr)) break;
				status = psrDollar;
				break;
			}
			else
			{
				(*ptr) = p;
				str += *(*ptr);
				TJSNext(ptr);
			}
		}
		else
		{
			str+=*(*ptr);
			TJSNext(ptr);
		}
	}

	if(status == psrNone)
	{
		// error
		TJS_eTJSError(TJSStringParseError);
	}

	str.FixLen();
	val = str;

	return status;
}
//---------------------------------------------------------------------------
bool TJSParseString(tTJSVariant &val, const tjs_char **ptr)
{
	// parse a string starts with '\'' or '"'

	tjs_char delimiter=*(*ptr);

	TJSNext(ptr);

	return TJSInternalParseString(val, ptr, delimiter, false) == psrDelimiter;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSParseNumber
//---------------------------------------------------------------------------
static tTJSString TJSExtractNumber(tjs_int (*validdigits)(tjs_char ch),
	const tjs_char *expmark, const tjs_char **ptr, bool &isreal)
{
	tTJSString tmp;

	bool point_found = false;
	bool exp_found = false;
	while(true)
	{
		if(validdigits(**ptr) != -1 && !exp_found)
		{
			tmp += **ptr;
			if(!TJSNext(ptr)) break;
		}
		else if(**ptr == TJS_W('.') && !point_found && !exp_found)
		{
			point_found = true;
			tmp += **ptr;
			if(!TJSNext(ptr)) break;
		}
		else if((**ptr == expmark[0] || **ptr == expmark[1]) && !exp_found)
		{
			exp_found = true;
			tmp += **ptr;
			if(!TJSNext(ptr)) break;
			if(!TJSSkipSpace(ptr)) break;
			if(**ptr == TJS_W('+'))
			{
				tmp += **ptr;
				if(!TJSNext(ptr)) break;
				if(!TJSSkipSpace(ptr)) break;
			}
			else if(**ptr == TJS_W('-'))
			{
				tmp += **ptr;
				if(!TJSNext(ptr)) break;
				if(!TJSSkipSpace(ptr)) break;
			}
		}
		else if (TJSDecNum(**ptr) != -1 && exp_found)
		{
			tmp += **ptr;
			if(!TJSNext(ptr)) break;
		}
		else
		{
			break;
		}
	}

	isreal = point_found || exp_found;

	return tmp;
}

static bool TJSParseNonDecimalReal(tTJSVariant &val, const tjs_char **ptr,
	tjs_int (*validdigits)(tjs_char ch), tjs_int basebits)
{
	// parse non-decimal(hexiadecimal, octal or binary) floating-point number.
	// this routine heavily depends on IEEE double floating-point number expression.

	tjs_uint64 main = TJS_UI64_VAL(0); // significand
	tjs_int exp = 0; // 2^n exponental
	tjs_int numsignif = 0; // significand bit count (including leading left-most '1') in "main"
	bool pointpassed = false;


	// scan input
	while(true)
	{
		if(**ptr == TJS_W('.'))
		{
			pointpassed = true;
		}
		else if(**ptr == TJS_W('p') || **ptr == TJS_W('P'))
		{
			if(!TJSNext(ptr)) break;
			if(!TJSSkipSpace(ptr)) break;

			bool biassign = false;
			if(**ptr == TJS_W('+'))
			{
				biassign = false;
				if(!TJSNext(ptr)) break;
				if(!TJSSkipSpace(ptr)) break;
			}

			if(**ptr == TJS_W('-'))
			{
				biassign = true;
				if(!TJSNext(ptr)) break;
				if(!TJSSkipSpace(ptr)) break;
			}

			tjs_int bias = 0;
			while(true)
			{
				bias *= 10;
				bias += TJSDecNum(**ptr);
				if(!TJSNext(ptr)) break;
			}
			if(biassign) bias = -bias;
			exp += bias;
			break;
		}
		else
		{
			tjs_int n = validdigits(**ptr);
			if(numsignif == 0)
			{
				// find msb flaged bit
				tjs_int b = basebits - 1;
				while(b >= 0)
				{
					if((1<<b) & n) break;
					b--;
				}

				b++;
				if(b)
				{
					// n is not zero
					// place it to the main's msb
					numsignif = b;
					main |= ((tjs_uint64)n << (64 - numsignif));
					if(pointpassed)
						exp -= (basebits - b + 1);
					else
						exp = b - 1;
				}
				else
				{
					// n is zero
					if(pointpassed) exp -= basebits;
				}
			}
			else
			{
				// append to main
				if(numsignif + basebits < 64)
				{
					numsignif += basebits;
					main |= ((tjs_uint64)n << (64 - numsignif));
				}
				if(!pointpassed) exp += basebits;
			}
		}
		if(!TJSNext(ptr)) break;
	}

	main >>= (64 - 1 - TJS_IEEE_D_SIGNIFICAND_BITS);

	if(main == 0)
	{
		// zero
		val = (tTVReal)0.0;
		return true;
	}

	main &= ((TJS_UI64_VAL(1) << TJS_IEEE_D_SIGNIFICAND_BITS) - TJS_UI64_VAL(1));

	if(exp < TJS_IEEE_D_EXP_MIN)
	{
		// denormal
		// treat as zero
		val = (tTVReal)0.0;
		return true;
	}

	if(exp > TJS_IEEE_D_EXP_MAX)
	{
		// too large
		// treat as infinity
		tjs_real d;
		*(tjs_uint64*)&d = TJS_IEEE_D_P_INF;
		val = d;
		return true;
	}

	// compose IEEE double
	tjs_real d;
	*(tjs_uint64*)&d =
		TJS_IEEE_D_MAKE_SIGN(0) |
		TJS_IEEE_D_MAKE_EXP(exp) |
		TJS_IEEE_D_MAKE_SIGNIFICAND(main);
	val = d;
	return true;
}

static bool TJSParseNonDecimalInteger(tTJSVariant &val, const tjs_char **ptr,
	tjs_int (*validdigits)(tjs_char ch), tjs_int basebits)
{
	tjs_int64 v = 0;
	while(true)
	{
		v <<= basebits;
		v += validdigits(**ptr);
		if(!TJSNext(ptr)) break;
	}
	val = (tTVInteger)v;
	return true;
}

static bool TJSParseNonDecimalNumber(tTJSVariant &val, const tjs_char **ptr,
	tjs_int (*validdigits)(tjs_char ch), tjs_int base)
{
	bool isreal = false;
	tTJSString tmp(TJSExtractNumber(validdigits, TJS_W("Pp"), ptr, isreal));

	if(tmp.IsEmpty()) return false;

	const tjs_char *p = tmp.c_str();
	const tjs_char **pp = &p;

	if(isreal)
		return TJSParseNonDecimalReal(val, pp, validdigits, base);
	else
		return TJSParseNonDecimalInteger(val, pp, validdigits, base);
}

static bool TJSParseDecimalReal(tTJSVariant &val, const tjs_char **pp)
{
	val = (tTVReal)TJS_strtod(*pp, NULL);
	return true;
}

static bool TJSParseDecimalInteger(tTJSVariant &val, const tjs_char **pp)
{
	int n;
	tjs_int64 num = 0;
	while((n = TJSDecNum(**pp)) != -1)
	{
		num *= 10;
		num += n;
		if(!TJSNext(pp)) break;
	}
	val = (tTVInteger)num;
	return true;
}

static bool TJSParseNumber2(tTJSVariant &val, const tjs_char **ptr)
{
	// stage 2

	if(TJSStringMatch(ptr, TJS_W("true"), true))
	{
		val = (tjs_int)true;
		return true;
	}
	if(TJSStringMatch(ptr, TJS_W("false"), true))
	{
		val = (tjs_int)false;
		return true;
	}
	if(TJSStringMatch(ptr, TJS_W("NaN"), true))
	{
		// Not a Number
		tjs_real d;
		*(tjs_uint64*)&d = TJS_IEEE_D_P_NaN;
		val = d;
		return true;
	}
	if(TJSStringMatch(ptr, TJS_W("Infinity"), true))
	{
		// positive inifinity
		tjs_real d;
		*(tjs_uint64*)&d = TJS_IEEE_D_P_INF;
		val = d;
		return true;
	}

	const tjs_char *ptr_save = *ptr;

	if(**ptr == TJS_W('0'))
	{
		if(!TJSNext(ptr))
		{
			val = (tjs_int) 0;
			return true;
		}

		tjs_char mark = **ptr;

		if(mark == TJS_W('X') || mark == TJS_W('x'))
		{
			// hexadecimal
			if(!TJSNext(ptr)) return false;
			return TJSParseNonDecimalNumber(val, ptr, TJSHexNum, 4);
		}

		if(mark == TJS_W('B') || mark == TJS_W('b'))
		{
			// binary
			if(!TJSNext(ptr)) return false;
			return TJSParseNonDecimalNumber(val, ptr, TJSBinNum, 1);
		}

		if(mark == TJS_W('.'))
		{
			// decimal point
			*ptr = ptr_save;
			goto decimal;
		}

		if(mark == TJS_W('E') || mark == TJS_W('e'))
		{
			// exp
			*ptr = ptr_save;
			goto decimal;
		}

		if(mark == TJS_W('P') || mark == TJS_W('p'))
		{
			// 2^n exp
			return false;
		}

		// octal
		*ptr = ptr_save;
		return TJSParseNonDecimalNumber(val, ptr, TJSOctNum, 3);
	}

	// integer decimal or real decimal
decimal:
	bool isreal = false;
	tTJSString tmp(TJSExtractNumber(TJSDecNum, TJS_W("Ee"), ptr, isreal));

	if(tmp.IsEmpty()) return false;

	const tjs_char *p = tmp.c_str();
	const tjs_char **pp = &p;

	if(isreal)
		return TJSParseDecimalReal(val, pp);
	else
		return TJSParseDecimalInteger(val, pp);
}


bool TJSParseNumber(tTJSVariant &val, const tjs_char **ptr)
{
	// parse a number pointed by (*ptr)
	TJSSetFPUE();

	bool sign = false; // true if negative

	if(**ptr == TJS_W('+'))
	{
		sign = false;
		if(!TJSNext(ptr)) return false;
		if(!TJSSkipSpace(ptr)) return false;
	}
	else if(**ptr == TJS_W('-'))
	{
		sign = true;
		if(!TJSNext(ptr)) return false;
		if(!TJSSkipSpace(ptr)) return false;
	}

	if(TJSParseNumber2(val, ptr))
	{
		if(sign) val = -val;
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSParseOctet
//---------------------------------------------------------------------------
static bool TJSParseOctet(tTJSVariant &val, const tjs_char **ptr)
{
	// parse a octet literal;
	// syntax is:
	// <% xx xx xx xx xx xx ... %>
	// where xx is hexadecimal 8bit(octet) binary representation.
	TJSNext(ptr);
	TJSNext(ptr);   // skip <%

	tjs_uint8 *buf = NULL;
	tjs_uint buflen = 0;

	bool leading = true;
	tjs_uint8 cur = 0;

	for(;*(*ptr);)
	{
		switch(TJSSkipComment(ptr))
		{
		case scrEnded:
			TJS_eTJSError(TJSStringParseError);
		case scrContinue:
		case scrNotComment:
			;
		}


		const tjs_char *next = *ptr;
		TJSNext(&next);
		if(*(*ptr) == TJS_W('%') && *next == TJS_W('>'))
		{
			*ptr = next;
			TJSNext(ptr);

			// literal ended

			if(!leading)
			{
				buf = (tjs_uint8*)TJS_realloc(buf, buflen+1);
				if(!buf)
					throw eTJSError(ttstr(TJSInsufficientMem));
				buf[buflen] = cur;
				buflen++;
			}

			val = tTJSVariant(buf, buflen); // create octet variant
			if(buf) TJS_free(buf);
			return true;
		}

		tjs_char ch = *(*ptr);
		tjs_int n = TJSHexNum(ch);
		if(n != -1)
		{
			if(leading)
			{
				cur = (tjs_uint8)(n);
				leading = false;
			}
			else
			{
				cur <<= 4;
				cur += n;

				// store cur
				buf = (tjs_uint8*)TJS_realloc(buf, buflen+1);
				if(!buf)
					throw eTJSError(ttstr(TJSInsufficientMem));
				buf[buflen] = cur;
				buflen++;

				leading = true;
			}
		}

		if(!leading && ch == TJS_W(','))
		{
			buf = (tjs_uint8*)TJS_realloc(buf, buflen+1);
			if(!buf)
				TJS_eTJSError(TJSInsufficientMem);
			buf[buflen] = cur;
			buflen++;

			leading = true;
		}

		*ptr = next;
	}

	// error
	TJS_eTJSError(TJSStringParseError);

	return false;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TJSParseRegExp
//---------------------------------------------------------------------------
static bool TJSParseRegExp(tTJSVariant &pat, const tjs_char **ptr)
{
	// parse a regular expression pointed by 'ptr'.
	// this is essencially the same as string parsing, except for
	// not to decode escaped characters by '\\'.
	// the regexp must be terminated by the delimiter '/', not succeeded by '\\'.

	// this returns an internal representation: '//flag/pattern' that can be parsed by
	// RegExp._compile

//	if(!TJSNext((*ptr))) TJS_eTJSError(TJSStringParseError);

	bool ok = false;
	bool lastbackslash = false;
	ttstr str;

	for(;*(*ptr);)
	{
		if(*(*ptr)==TJS_W('\\'))
		{
			str+=*(*ptr);
			if(lastbackslash)
				lastbackslash = false;
			else
				lastbackslash = true;
		}
		else if(*(*ptr)==TJS_W('/') && !lastbackslash)
		{
			// string delimiters
//			lastbackslash = false;

			if(!TJSNext(ptr))
			{
				ok = true;
				break;
			}

			// flags can be here
			ttstr flag;
			while(*(*ptr) >= TJS_W('a') && *(*ptr) <= TJS_W('z'))
			{
				flag += *(*ptr);
				if(!TJSNext(ptr)) break;
			}
			str = TJS_W("/")TJS_W("/")+ flag + TJS_W("/") + str;
			ok = true;
			break;
		}
		else
		{
			lastbackslash = false;
			str+=*(*ptr);
		}
		TJSNext(ptr);
	}

	if(!ok)
	{
		// error
		TJS_eTJSError(TJSStringParseError);
	}

	pat = str;

	return true;
}

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// hash table for reserved words
//---------------------------------------------------------------------------
static tTJSCustomObject * TJSReservedWordHash = NULL;
static bool TJSReservedWordHashInit = false;
static tjs_int TJSReservedWordHashRefCount;
//---------------------------------------------------------------------------
void TJSReservedWordsHashAddRef()
{
	if(TJSReservedWordHashRefCount == 0)
	{
		TJSReservedWordHashInit = false;
		TJSReservedWordHash = new tTJSCustomObject();
	}
	TJSReservedWordHashRefCount ++;
}
//---------------------------------------------------------------------------
void TJSReservedWordsHashRelease()
{
	TJSReservedWordHashRefCount --;

	if(TJSReservedWordHashRefCount == 0)
	{
		TJSReservedWordHash->Release();
		TJSReservedWordHash = NULL;
	}
}
//---------------------------------------------------------------------------
static void TJSRegisterReservedWordsHash(const tjs_char *word, tjs_int num)
{
	tTJSVariant val(num);
	TJSReservedWordHash->PropSet(TJS_MEMBERENSURE, word, NULL, &val,
		TJSReservedWordHash);
}
//---------------------------------------------------------------------------
#define TJS_REG_RES_WORD(word, value) TJSRegisterReservedWordsHash(TJS_W(word), value);
static void TJSInitReservedWordsHashTable()
{
	if(TJSReservedWordHashInit) return;
	TJSReservedWordHashInit = true;

	TJS_REG_RES_WORD("break", T_BREAK);
	TJS_REG_RES_WORD("continue", T_CONTINUE);
	TJS_REG_RES_WORD("const", T_CONST);
	TJS_REG_RES_WORD("catch", T_CATCH);
	TJS_REG_RES_WORD("class", T_CLASS);
	TJS_REG_RES_WORD("case", T_CASE);
	TJS_REG_RES_WORD("debugger", T_DEBUGGER);
	TJS_REG_RES_WORD("default", T_DEFAULT);
	TJS_REG_RES_WORD("delete", T_DELETE);
	TJS_REG_RES_WORD("do", T_DO);
	TJS_REG_RES_WORD("extends", T_EXTENDS);
	TJS_REG_RES_WORD("export", T_EXPORT);
	TJS_REG_RES_WORD("enum", T_ENUM);
	TJS_REG_RES_WORD("else", T_ELSE);
	TJS_REG_RES_WORD("function", T_FUNCTION);
	TJS_REG_RES_WORD("finally", T_FINALLY);
	TJS_REG_RES_WORD("false", T_FALSE);
	TJS_REG_RES_WORD("for", T_FOR);
	TJS_REG_RES_WORD("global", T_GLOBAL);
	TJS_REG_RES_WORD("getter", T_GETTER);
	TJS_REG_RES_WORD("goto", T_GOTO);
	TJS_REG_RES_WORD("incontextof", T_INCONTEXTOF);
	TJS_REG_RES_WORD("invalidate", T_INVALIDATE);
	TJS_REG_RES_WORD("instanceof", T_INSTANCEOF);
	TJS_REG_RES_WORD("isvalid", T_ISVALID);
	TJS_REG_RES_WORD("import", T_IMPORT);
	TJS_REG_RES_WORD("int", T_INT);
	TJS_REG_RES_WORD("in", T_IN);
	TJS_REG_RES_WORD("if", T_IF);
	TJS_REG_RES_WORD("null", T_NULL);
	TJS_REG_RES_WORD("new", T_NEW);
	TJS_REG_RES_WORD("octet", T_OCTET);
	TJS_REG_RES_WORD("protected", T_PROTECTED);
	TJS_REG_RES_WORD("property", T_PROPERTY);
	TJS_REG_RES_WORD("private", T_PRIVATE);
	TJS_REG_RES_WORD("public", T_PUBLIC);
	TJS_REG_RES_WORD("return", T_RETURN);
	TJS_REG_RES_WORD("real", T_REAL);
	TJS_REG_RES_WORD("synchronized", T_SYNCHRONIZED);
	TJS_REG_RES_WORD("switch", T_SWITCH);
	TJS_REG_RES_WORD("static", T_STATIC);
	TJS_REG_RES_WORD("setter", T_SETTER);
	TJS_REG_RES_WORD("string", T_STRING);
	TJS_REG_RES_WORD("super", T_SUPER);
	TJS_REG_RES_WORD("typeof", T_TYPEOF);
	TJS_REG_RES_WORD("throw", T_THROW);
	TJS_REG_RES_WORD("this", T_THIS);
	TJS_REG_RES_WORD("true", T_TRUE);
	TJS_REG_RES_WORD("try", T_TRY);
	TJS_REG_RES_WORD("void", T_VOID);
	TJS_REG_RES_WORD("var", T_VAR);
	TJS_REG_RES_WORD("while", T_WHILE);
#ifndef TJS_NaN_and_Infinity_ARE_NOT_RESERVED_WORD
	TJS_REG_RES_WORD("NaN", T_NAN);
	TJS_REG_RES_WORD("Infinity", T_INFINITY);
#endif
#ifndef TJS_WITH_IS_NOT_RESERVED_WORD
	TJS_REG_RES_WORD("with", T_WITH);
#endif

}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSLexicalAnalyzer
//---------------------------------------------------------------------------
tTJSLexicalAnalyzer::tTJSLexicalAnalyzer(tTJSScriptBlock *block,
	const tjs_char *script, bool exprmode, bool resneeded)
{
	// resneeded is valid only if exprmode is true

	TJS_F_TRACE("tTJSLexicalAnalyzer::tTJSLexicalAnalyzer");

	TJSInitReservedWordsHashTable();

	Block = block;
	ExprMode = exprmode;
	ResultNeeded = resneeded;
	PrevToken = -1;
	tjs_int len = (tjs_int)TJS_strlen(script);
	Script = new tjs_char[len+2];
	TJS_strcpy(Script, script);
	if(ExprMode)
	{
		// append ';' on expression analyze mode
		Script[len] = TJS_W(';');
		Script[len+1] = 0;
	}
	else
	{
		if(Script[0] == TJS_W('#') && Script[1] == TJS_W('!'))
		{
			// shell script like file
			Script[0] = TJS_W('/');
			Script[1] = TJS_W('/');  // convert #! to //
		}
	}

	if(tjsEnableDicFuncQuickHack) //----- dicfunc quick-hack
	{
		DicFunc = false;
		if(ExprMode && (Script[0] == TJS_W('[') || (Script[0] == TJS_W('%') && Script[1] == TJS_W('['))))
		{
			DicFunc = true;
		}
	}

	Current = Script;
	IfLevel = 0;
	PrevPos = 0;
	NestLevel = 0;
	First = true;
	RegularExpression = false;
	BareWord = false;
	PutValue(tTJSVariant());
}
//---------------------------------------------------------------------------
tTJSLexicalAnalyzer::~tTJSLexicalAnalyzer()
{
	Free();
}
//---------------------------------------------------------------------------
tTJSSkipCommentResult tTJSLexicalAnalyzer::SkipUntil_endif()
{
	// skip until @endif
	tjs_int exl = IfLevel;
	IfLevel ++;
	while(true)
	{
		if(*Current == TJS_W('/'))
		{
			switch(TJSSkipComment(&Current))
			{
			case scrEnded:
				TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));
				break;
			case scrContinue:
				break;
			case scrNotComment:
				if(!TJSNext(&Current))
					TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));
				break;
			}
		}
		else if(*Current == TJS_W('@'))
		{
			Current ++;
			bool skipp = false;

			if(!TJS_strncmp(Current, TJS_W("if"), 2))
			{
				IfLevel ++;
				Current += 2;
				skipp = true;
			}
			else if(!TJS_strncmp(Current, TJS_W("set"), 3))
			{
				Current += 3;
				skipp = true;
			}
			else if(!TJS_strncmp(Current, TJS_W("endif"), 5))
			{
				// endif
				Current += 5;
				IfLevel--;
				if(IfLevel == exl)
				{
					// skip ended
					TJSSkipSpace(&Current);
					if(!*Current) return scrEnded;
					return scrContinue;
				}
			}
			else
			{
				// skipp = false
			}

			if(skipp)
			{
				// skip parenthesises
				if(!TJSSkipSpace(&Current))
					TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));

				if(*Current!=TJS_W('('))
					TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));

				TJSNext(&Current);
				tjs_int plevel = 0;
				while(*Current && (plevel || *Current!=TJS_W(')')))
				{
					if(*Current == TJS_W('(')) plevel++;
					else if(*Current == TJS_W(')')) plevel--;
					TJSNext(&Current);
				}
				if(!*Current)
					TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));
				TJSNext(&Current);
				if(!*Current)
					TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));
			}
		}
		else
		{
			if(!TJSNext(&Current))
				TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));
		}
	}
}
//---------------------------------------------------------------------------
tTJSSkipCommentResult tTJSLexicalAnalyzer::ProcessPPStatement()
{
	// process pre-prosessor statements.
	// here "Current" points '@'.
	const tjs_char *org = Current;

	Current ++;

	if(!TJS_strncmp(Current, TJS_W("set"), 3))
	{
		// set statement
		Block->NotifyUsingPreProcessor();
		Current+=3;
		if(!TJSSkipSpace(&Current))
			TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));

		if(*Current!=TJS_W('('))
			TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));

		TJSNext(&Current);
		const tjs_char *st = Current;
		tjs_int plevel = 0;
		while(*Current && (plevel || *Current!=TJS_W(')')))
		{
			if(*Current == TJS_W('(')) plevel++;
			else if(*Current == TJS_W(')')) plevel--;
			TJSNext(&Current);
		}
		const tjs_char *ed = Current;

		TJSNext(&Current);

		try
		{
			ParsePPExpression(st, (tjs_int)(ed-st)); // evaluate exp
		}
		catch(...)
		{
			TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));
		}

		TJSSkipSpace(&Current);
		if(!*Current) return scrEnded;

		return scrContinue;
	}

	if(!TJS_strncmp(Current, TJS_W("if"), 2))
	{
		// if statement

		Block->NotifyUsingPreProcessor();
		Current += 2;
		if(!TJSSkipSpace(&Current))
			TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));

		if(*Current!=TJS_W('('))
			TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));

		TJSNext(&Current);
		const tjs_char *st = Current;
		tjs_int plevel = 0;
		while(*Current && (plevel || *Current!=TJS_W(')')))
		{
			if(*Current == TJS_W('(')) plevel++;
			else if(*Current == TJS_W(')')) plevel--;
			TJSNext(&Current);
		}
		const tjs_char *ed = Current;

		TJSNext(&Current);

		tjs_int32 ret;

		try
		{
			ret = ParsePPExpression(st, (tjs_int)(ed-st)); // evaluate exp
		}
		catch(...)
		{
			TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));
		}

		if(!ret) return SkipUntil_endif();  // skip to endif

		IfLevel ++;

		TJSSkipSpace(&Current);
		if(!*Current) return scrEnded;
		return scrContinue;
	}


	if(!TJS_strncmp(Current, TJS_W("endif"), 5))
	{
		Current += 5;
		IfLevel --;
		if(IfLevel < 0)
			TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));

		TJSSkipSpace(&Current);
		if(!*Current) return scrEnded;
		return scrContinue;
	}

	Current = org;
	return scrNotComment;
}
//---------------------------------------------------------------------------
#define TJS_MATCH_W(word, code) \
	if(TJSStringMatch(&Current, TJS_W(word), true)) return (code)
#define TJS_MATCH_S(word, code) \
	if(TJSStringMatch(&Current, TJS_W(word), false)) return (code)
#define TJS_MATCH_W_V(word, code, val) \
	if(TJSStringMatch(&Current, TJS_W(word), true)) { n=PutValue(val); return (code); }
#define TJS_MATCH_S_V(word, code, val) \
	if(TJSStringMatch(&Current, TJS_W(word), false)) { n=PutValue(val); return (code); }
#define TJS_1CHAR(code) \
	TJSNext(&Current); return (code)
tjs_int tTJSLexicalAnalyzer::GetToken(tjs_int &n)
{
	// returns token, pointed by 'Current'

	if(*Current == 0) return 0;

	if(RegularExpression)
	{
		// the next token was marked as a regular expression by the parser
		RegularExpression = false;

		Current = Script + PrevPos; // draws position of the first '/' back

		TJSNext(&Current);

		tTJSVariant pat;
		TJSParseRegExp(pat, &Current);
		n = PutValue(pat);

		return T_REGEXP;
	}

re_match:

	PrevPos = (tjs_int)(Current - Script); // remember current position as "PrevPos"

	switch(*Current)
	{
	case TJS_W('>'):
		TJS_MATCH_S(">>>=", T_RBITSHIFTEQUAL);
		TJS_MATCH_S(">>>", T_RBITSHIFT);
		TJS_MATCH_S(">>=", T_RARITHSHIFTEQUAL);
		TJS_MATCH_S(">>", T_RARITHSHIFT);
		TJS_MATCH_S(">=", T_GTOREQUAL);
		TJS_1CHAR(T_GT);

	case TJS_W('<'):
		TJS_MATCH_S("<<=", T_LARITHSHIFTEQUAL);
		TJS_MATCH_S("<->", T_SWAP);
		TJS_MATCH_S("<=", T_LTOREQUAL);
		TJS_MATCH_S("<<", T_LARITHSHIFT);
		{
			const tjs_char *next = Current;
			TJSNext(&next);
			if(*next == TJS_W('%'))
			{
				// '<%'   octet literal
				tTJSVariant v;
				TJSParseOctet(v, &Current);
				n = PutValue(v);
				return T_CONSTVAL;
			}
		}
		TJS_1CHAR(T_LT);

	case TJS_W('='):
		TJS_MATCH_S("===", T_DISCEQUAL);
		TJS_MATCH_S("==", T_EQUALEQUAL);
		TJS_MATCH_S("=>", T_COMMA);
			// just a replacement for comma, like perl
		TJS_1CHAR(T_EQUAL);

	case TJS_W('!'):
		TJS_MATCH_S("!==", T_DISCNOTEQUAL);
		TJS_MATCH_S("!=", T_NOTEQUAL);
		TJS_1CHAR(T_EXCRAMATION);

	case TJS_W('&'):
		TJS_MATCH_S("&&=", T_LOGICALANDEQUAL);
		TJS_MATCH_S("&&", T_LOGICALAND);
		TJS_MATCH_S("&=", T_AMPERSANDEQUAL);
		TJS_1CHAR(T_AMPERSAND);

	case TJS_W('|'):
		TJS_MATCH_S("||=", T_LOGICALOREQUAL);
		TJS_MATCH_S("||", T_LOGICALOR);
		TJS_MATCH_S("|=", T_VERTLINEEQUAL);
		TJS_1CHAR(T_VERTLINE);

	case TJS_W('.'):
		if(Current[1] >= TJS_W('0') && Current[1] <= TJS_W('9'))
		{
			// number
			tTJSVariant v;
			TJSParseNumber(v, &Current);
			n=PutValue(v);
			return T_CONSTVAL;
		}
		TJS_MATCH_S("...", T_OMIT);
		TJS_1CHAR(T_DOT);

	case TJS_W('+'):
		TJS_MATCH_S("++", T_INCREMENT);
		TJS_MATCH_S("+=", T_PLUSEQUAL);
		TJS_1CHAR(T_PLUS);

	case TJS_W('-'):
		TJS_MATCH_S("-=", T_MINUSEQUAL);
		TJS_MATCH_S("--", T_DECREMENT);
		TJS_1CHAR(T_MINUS);

	case TJS_W('*'):
		TJS_MATCH_S("*=", T_ASTERISKEQUAL);
		TJS_1CHAR(T_ASTERISK);

	case TJS_W('/'):
		// check comments
		switch(TJSSkipComment(&Current))
		{
		case scrContinue:
			goto re_match;
		case scrEnded:
			return 0;
		case scrNotComment:
			;
		}

		TJS_MATCH_S("/=", T_SLASHEQUAL);
		TJS_1CHAR(T_SLASH);

	case TJS_W('\\'):
		TJS_MATCH_S("\\=", T_BACKSLASHEQUAL);
		TJS_1CHAR(T_BACKSLASH);

	case TJS_W('%'):
		TJS_MATCH_S("%=", T_PERCENTEQUAL);
		TJS_1CHAR(T_PERCENT);

	case TJS_W('^'):
		TJS_MATCH_S("^=", T_CHEVRONEQUAL);
		TJS_1CHAR(T_CHEVRON);

	case TJS_W('['):
		NestLevel++;
		TJS_1CHAR(T_LBRACKET);

	case TJS_W(']'):
		NestLevel--;
		TJS_1CHAR(T_RBRACKET);

	case TJS_W('('):
		NestLevel++;
		TJS_1CHAR(T_LPARENTHESIS);

	case TJS_W(')'):
		NestLevel--;
		TJS_1CHAR(T_RPARENTHESIS);

	case TJS_W('~'):
		TJS_1CHAR(T_TILDE);

	case TJS_W('?'):
		TJS_1CHAR(T_QUESTION);

	case TJS_W(':'):
		TJS_1CHAR(T_COLON);

	case TJS_W(','):
		TJS_1CHAR(T_COMMA);

	case TJS_W(';'):
		TJS_1CHAR(T_SEMICOLON);

	case TJS_W('{'):
		NestLevel++;
		TJS_1CHAR(T_LBRACE);

	case TJS_W('}'):
		NestLevel--;
		TJS_1CHAR(T_RBRACE);

	case TJS_W('#'):
		TJS_1CHAR(T_SHARP);

	case TJS_W('$'):
		TJS_1CHAR(T_DOLLAR);

	case TJS_W('\''):
	case TJS_W('\"'):
		// literal string
	  {
		tTJSVariant v;
		TJSParseString(v, &Current);
		n=PutValue(v);
		return T_CONSTVAL;
	  }

	case TJS_W('@'):
		// embeddable expression in string (such as @"this can be embeddable like &variable;")
	  {
		const tjs_char *org = Current;
		if(!TJSNext(&Current)) return 0;
		if(!TJSSkipSpace(&Current)) return 0;
		if(*Current == TJS_W('\'') || *Current == TJS_W('\"'))
		{
			tEmbeddableExpressionData data;
			data.State = evsStart;
			data.WaitingNestLevel = NestLevel;
			data.Delimiter = *Current;
			data.NeedPlus = false;
			if(!TJSNext(&Current)) return 0;
			EmbeddableExpressionDataStack.push_back(data);
			return -1;
		}
		else
		{
			Current = org;
		}

		// possible pre-prosessor statements
		switch(ProcessPPStatement())
		{
		case scrContinue:
			goto re_match;
		case scrEnded:
			return 0;
		case scrNotComment:
			Current = org;
			break;
		}
		break;
	  }

	case TJS_W('0'):
	case TJS_W('1'):
	case TJS_W('2'):
	case TJS_W('3'):
	case TJS_W('4'):
	case TJS_W('5'):
	case TJS_W('6'):
	case TJS_W('7'):
	case TJS_W('8'):
	case TJS_W('9'):
		// number
	  {
		tTJSVariant v;
		bool r = TJSParseNumber(v, &Current);
		if(!r) TJS_eTJSCompileError(TJSNumberError, Block, (tjs_int)(Current-Script));
		n=PutValue(v);
		return T_CONSTVAL;
	  }
	}

	if(!TJS_iswalpha(*Current) && *Current!=TJS_W('_'))
	{
		ttstr str(TJSInvalidChar);
		str.Replace(TJS_W("%1"), ttstr(*Current).EscapeC());
		TJS_eTJSError(str);
	}


	const tjs_char *ptr = Current;
	tjs_int nch = 0;
	while(TJS_iswdigit(*ptr) || TJS_iswalpha(*ptr) || *ptr==TJS_W('_') ||
		*ptr>0x0100 || *ptr == TJS_SKIP_CODE)
		ptr++, nch++;

	if(nch == 0)
	{
		ttstr str(TJSInvalidChar);
		str.Replace(TJS_W("%1"), ttstr(*Current).EscapeC());
		TJS_eTJSError(str);
	}

	ttstr str(Current, nch);
	Current += nch;

	tjs_char *s, *d;
	s = d = str.Independ();
	while(*s)
	{
		// eliminate TJS_SKIP_CODE
		if(*s == TJS_SKIP_CODE)
		{
			s++;
			continue;
		}
		*d = *s;
		d++, s++;
	}
	*d = 0;
	str.FixLen();

	tjs_int retnum;

	if(BareWord)
		retnum = -1;
	else
		retnum = TJSReservedWordHash->GetValueInteger(str.c_str(), str.GetHint());

	BareWord = false;

	if(retnum == -1)
	{
		// not a reserved word
		n = PutValue(str);
		return T_SYMBOL;
	}

	switch(retnum)
	{
	case T_FALSE:
		n = PutValue(tTJSVariant(false));
		return T_CONSTVAL;
	case T_NULL:
		n = PutValue(tTJSVariant((iTJSDispatch2*)NULL));
		return T_CONSTVAL;
	case T_TRUE:
		n = PutValue(tTJSVariant(true));
		return T_CONSTVAL;
	case T_NAN:
	  {
		TJSSetFPUE();
		tjs_real d;
		*(tjs_uint64*)&d = TJS_IEEE_D_P_NaN;
		n = PutValue(tTJSVariant(d));
		return T_CONSTVAL;
	  }
	case T_INFINITY:
	  {
		TJSSetFPUE();
		tjs_real d;
		*(tjs_uint64*)&d = TJS_IEEE_D_P_INF;
		n = PutValue(tTJSVariant(d));
		return T_CONSTVAL;
	  }
	}

	return retnum;
}
//---------------------------------------------------------------------------
tjs_int32 tTJSLexicalAnalyzer::ParsePPExpression(const tjs_char *start, tjs_int n)
{
	// parses a conditional compile experssion starting with "start",
	// character count "n".
	tjs_char *buf = new tjs_char[n+1];
	tjs_char *p;
	const tjs_char *lim = start + n;
	p=buf;
	while(start < lim)
	{
		*p = *start;
		TJSNext(&start);
		p++;
	}

	*p = 0;

	tTJSPPExprParser *parser = new tTJSPPExprParser(Block->GetTJS(), buf);

	tjs_int32 result;
	try
	{
		result = parser->Parse();
	}
	catch(...)
	{
		delete parser;
		throw;
	}

	delete parser;

	return result;
}
//---------------------------------------------------------------------------
void tTJSLexicalAnalyzer::PreProcess(void)
{
	TJS_F_TRACE("tTJSLexicalAnalyzer::PreProcess");


	// pre-process

	// proceeds newline code unification, conditional compile control.


	/* unify new line codes */
	TJS_D((TJS_W("unifying new line codes ...\n")))

	tjs_char *p;
	p=Script;
	while(*p)
	{
		if(*p==TJS_W('\r') && *(p+1)==TJS_W('\n'))
			*p=TJS_SKIP_CODE;
		else if(*p==TJS_W('\r'))
			*p=TJS_W('\n');
		p++;
	}
}

//---------------------------------------------------------------------------
tjs_int tTJSLexicalAnalyzer::PutValue(const tTJSVariant &val)
{
	tTJSVariant *v = new tTJSVariant(val);
	Values.push_back(v);
	return (tjs_int)(Values.size() -1);
}
//---------------------------------------------------------------------------
void tTJSLexicalAnalyzer::Free(void)
{
	if(Script) delete [] Script;
	std::vector<tTJSVariant*>::iterator i;
	for(i = Values.begin(); i != Values.end(); i++)
	{
		delete *i;
	}
	Values.clear();
}
/*
//---------------------------------------------------------------------------
void tTJSLexicalAnalyzer::NextBraceIsBlockBrace()
{
	BlockBrace = true;
}
*/
//---------------------------------------------------------------------------
tjs_int tTJSLexicalAnalyzer::GetCurrentPosition()
{
	return (tjs_int)(Current - Script);
}
//---------------------------------------------------------------------------
tjs_int tTJSLexicalAnalyzer::GetNext(tjs_int &value)
{
	TJS_F_TRACE("tTJSLexicalAnalyzer::GetNext");

	if(First)
	{
		TJS_D((TJS_W("pre-processing ...\n")))


		First = false;
		try
		{
			PreProcess();
		}
		catch(eTJSCompileError &e)
		{
			_yyerror(e.GetMessage().c_str(), Block, e.GetPosition());
			return 0;
		}
		Current = Script;
		PrevPos = 0;

		if(ExprMode && ResultNeeded)
		{
			value = 0;
			return T_RETURN;
				// return T_RETURN first if 'expression' mode
				// (and ResultNeeded is specified)
		}
	}

	tjs_int n;
	value = 0;

	do
	{
		if(RetValDeque.size())
		{
			tTokenPair pair = RetValDeque.front();
			RetValDeque.pop_front();
			value = pair.value;
			PrevToken = pair.token;
			return pair.token;
		}



		try
		{
			if(!EmbeddableExpressionDataStack.size())
			{
				// normal mode
				TJSSkipSpace(&Current);
				n = GetToken(value);

				if(tjsEnableDicFuncQuickHack) //----- dicfunc quick-hack
				{
					if(DicFunc)
					{
						if(n == T_PERCENT)
						{
							// push "function { return %"
							RetValDeque.push_back(tTokenPair(T_FUNCTION, 0));
							RetValDeque.push_back(tTokenPair(T_LBRACE, 0));
							RetValDeque.push_back(tTokenPair(T_RETURN, 0));
							RetValDeque.push_back(tTokenPair(T_PERCENT, 0));
							n = -1;
						}
						else if(n == T_LBRACKET && PrevToken != T_PERCENT)
						{
							// push "function { return ["
							RetValDeque.push_back(tTokenPair(T_FUNCTION, 0));
							RetValDeque.push_back(tTokenPair(T_LBRACE, 0));
							RetValDeque.push_back(tTokenPair(T_RETURN, 0));
							RetValDeque.push_back(tTokenPair(T_LBRACKET, 0));
							n = -1;
						}
						else if(n == T_RBRACKET)
						{
							// push "] ; }( )"
							RetValDeque.push_back(tTokenPair(T_RBRACKET, 0));
							RetValDeque.push_back(tTokenPair(T_SEMICOLON, 0));
							RetValDeque.push_back(tTokenPair(T_RBRACE, 0));
							RetValDeque.push_back(tTokenPair(T_LPARENTHESIS, 0));
							RetValDeque.push_back(tTokenPair(T_RPARENTHESIS, 0));
							n = -1;
						}
					}
				}
			}
			else
			{
				// embeddable expression mode
				tEmbeddableExpressionData &data = EmbeddableExpressionDataStack.back();
				switch(data.State)
				{
				case evsStart:
					RetValDeque.push_back(tTokenPair(T_LPARENTHESIS, 0));
					n = -1;
					data.State = evsNextIsStringLiteral;
					break;

				case evsNextIsStringLiteral:
				  {
					tTJSVariant v;
					tTJSInternalParseStringResult res =
						//TJSInternalParseString(v, &Current, data.Delimiter, TJS_W('&'));
						TJSInternalParseString(v, &Current, data.Delimiter, true );
					if(res == psrDelimiter)
					{
						// embeddable expression mode ended
						ttstr str(v);
						if(!str.IsEmpty())
						{
							if(data.NeedPlus)
								RetValDeque.push_back(tTokenPair(T_PLUS, 0));
						}
						if(!str.IsEmpty() || !data.NeedPlus)
							RetValDeque.push_back(tTokenPair(T_CONSTVAL, PutValue(v)));
						RetValDeque.push_back(tTokenPair(T_RPARENTHESIS, 0));
						EmbeddableExpressionDataStack.pop_back();
						n = -1;
						break;
					}
					else
					{
						// *Current is next to ampersand mark or '${'
						ttstr str(v);
						if(!str.IsEmpty())
						{
							if(data.NeedPlus)
								RetValDeque.push_back(tTokenPair(T_PLUS, 0));
							RetValDeque.push_back(tTokenPair(T_CONSTVAL, PutValue(v)));
							data.NeedPlus = true;
						}
						if(data.NeedPlus)
							RetValDeque.push_back(tTokenPair(T_PLUS, 0));
						RetValDeque.push_back(tTokenPair(T_STRING, 0));
						RetValDeque.push_back(tTokenPair(T_LPARENTHESIS, 0));
						data.State = evsNextIsExpression;
						if(res == psrAmpersand)
							data.WaitingToken = T_SEMICOLON;
						else if(res == psrDollar)
							data.WaitingToken = T_RBRACE, NestLevel++;
						n = -1;
						break;
					}
				  }

				case evsNextIsExpression:
				  {
					TJSSkipSpace(&Current);
					n = GetToken(value);

					if(n == data.WaitingToken && NestLevel == data.WaitingNestLevel)
					{
						// end of embeddable expression mode
						RetValDeque.push_back(tTokenPair(T_RPARENTHESIS, 0));
						data.NeedPlus = true;
						data.State = evsNextIsStringLiteral;
						n = -1;
					}

					break;
				  }
				}
			}

			if(n == 0)
			{
				if(IfLevel != 0)
					TJS_eTJSCompileError(TJSPPError, Block, (tjs_int)(Current-Script));
			}
		}
		catch(eTJSCompileError &e)
		{
			_yyerror(e.GetMessage().c_str(), Block);
			return 0;
		}
		catch(eTJSScriptError &e)
		{
			_yyerror(e.GetMessage().c_str(), Block);
			return 0;
		}
		catch(eTJS &e)
		{
			_yyerror(e.GetMessage().c_str(), Block);
			return 0;
		}
	#ifdef TJS_SUPPORT_VCL
		catch(EAccessViolation &e)
		{
			ttstr msg(e.Message.c_str());
			_yyerror(msg.c_str(), Block);
			return 0;
		}
		catch(Exception &e)
		{
			ttstr msg(e.Message.c_str());
			_yyerror(msg.c_str(), Block);
			return 0;
		}
	#endif
	} while(n < 0);

/*
	if(BlockBrace)
	{
		if(n == T_LBRACE) n = T_BLOCK_LBRACE;
		BlockBrace = false;
	}
*/
	PrevToken = n;
	return n;
}
//---------------------------------------------------------------------------
void tTJSLexicalAnalyzer::SetStartOfRegExp(void)
{
	// notifies that a regular expression ( regexp ) 's
	// first '/' ( that indicates the start of the regexp ) had been detected.
	// this will be called by the parser.

	RegularExpression = true;
}
//---------------------------------------------------------------------------
void tTJSLexicalAnalyzer::SetNextIsBareWord(void)
{
	// notifies that the next word must be treated as a bare word
	// (not a reserved word)
	// this will be called after . (dot) operator by the parser.

	BareWord = true;
}
//---------------------------------------------------------------------------


} // namespace TJS



