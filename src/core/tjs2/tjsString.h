//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// tTJSVariant friendly string class implementation
//---------------------------------------------------------------------------

#ifndef tjsStringH
#define tjsStringH

#include <string>
#include "tjsConfig.h"
#ifdef TJS_SUPPORT_VCL
	#include <vcl.h>
#endif
#include "tjsVariantString.h"

namespace TJS
{
/*[*/
//---------------------------------------------------------------------------
// tTJSStringBufferLength
//---------------------------------------------------------------------------
#pragma pack(push, 4)
class tTJSStringBufferLength
{
public:
	tjs_int n;
	tTJSStringBufferLength(tjs_int n) {this->n = n;}
};
#pragma pack(pop)
/*]*/
//---------------------------------------------------------------------------





class tTJSVariant;
extern const tjs_char *TJSNullStrPtr;

/*[*/
//---------------------------------------------------------------------------
// tTJSString
//---------------------------------------------------------------------------
#pragma pack(push, 4)
class tTJSVariantString;
struct tTJSString_S
{
	tTJSVariantString *Ptr;
};
#pragma pack(pop)
class tTJSString;
/*]*/

/*start-of-tTJSString*/
class tTJSString : protected tTJSString_S
{
public:
	//-------------------------------------------------------- constructor --
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, ()) { Ptr = NULL; }
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (const tTJSString &rhs)) { Ptr = rhs.Ptr; if(Ptr) Ptr->AddRef(); }
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (tTJSVariantString *vstr))   { Ptr = vstr; if(Ptr) Ptr->AddRef(); }
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (const tjs_char *str)) { Ptr = TJSAllocVariantString(str); }
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (const tjs_nchar *str)) { Ptr = TJSAllocVariantString(str); }
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (const tTJSStringBufferLength len))
		{ Ptr = TJSAllocVariantStringBuffer(len.n); }
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (tjs_char rch))
	{
		tjs_char ch[2];
		ch[0] = rch;
		ch[1] = 0;
		Ptr = TJSAllocVariantString(ch);
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (const tTJSVariant & val));

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (const tTJSString &str, int n)) // construct with first n chars of str
		{ Ptr = TJSAllocVariantString(str.c_str(), n); }

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (const tjs_char *str, int n)) // same as above except for str's type
		{ Ptr = TJSAllocVariantString(str, n); }

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (tjs_int n)); // from int
    TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSString, (tjs_int64 n)); // from int64

#ifdef TJS_SUPPORT_VCL
	tTJSString(const AnsiString &str) { Ptr = TJSAllocVariantString(str.c_str()); }
	tTJSString(const WideString &str) { Ptr = TJSAllocVariantString(str.c_bstr()); }
#endif
	tTJSString(const std::basic_string<tjs_char> &str) { Ptr = TJSAllocVariantString(str.c_str()); }
	tTJSString(const std::string &str) { Ptr = TJSAllocVariantString(str.c_str()); }

	//--------------------------------------------------------- destructor --
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, ~tTJSString, ()) { if(Ptr) Ptr->Release(); }

	//--------------------------------------------------------- conversion --
	TJS_CONST_METHOD_DEF(const tjs_char *, c_str, ())
		{ return Ptr?Ptr->operator const tjs_char *():TJSNullStrPtr; }

#ifdef TJS_SUPPORT_VCL
	const AnsiString AsAnsiString() const
	{
		if(!Ptr) return "";
		tTJSNarrowStringHolder holder(Ptr->operator const tjs_char*());
		return AnsiString(holder.operator const char *());
	}
	const WideString AsWideString() const
	{
		if(!Ptr) return L"";
		return WideString(Ptr->operator const tjs_char *());
	}
#endif

	const std::string AsStdString() const
	{
#if 0
		if (!Ptr) return std::wstring(TJS_W(""));
		return std::wstring(c_str());
#else
		if(!Ptr) return std::string("");
			// this constant string value must match std::string in type
		tTJSNarrowStringHolder holder(Ptr->operator const tjs_char*());
		return std::string(holder.operator const char *());
#endif
	}
	const std::string AsNarrowStdString() const
	{
		if(!Ptr) return std::string("");
			// this constant string value must match std::string in type
		tTJSNarrowStringHolder holder(Ptr->operator const tjs_char*());
		return std::string(holder.operator const char *());
	}

	TJS_CONST_METHOD_DEF(tTJSVariantString *, AsVariantStringNoAddRef, ())
	{
		return Ptr;
	}

	TJS_CONST_METHOD_DEF(tjs_int64, AsInteger, ());

	//------------------------------------------------------- substitution --
	TJS_METHOD_DEF(tTJSString &, operator =, (const tTJSString & rhs))
	{
		if(rhs.Ptr) rhs.Ptr->AddRef();
		if(Ptr) Ptr->Release();
		Ptr = rhs.Ptr;
		return *this;
	}

	TJS_METHOD_DEF(tTJSString &, operator =, (const tjs_char * rhs))
	{
		if(Ptr)
		{
			Independ();
			if(rhs && rhs[0])
				Ptr->ResetString(rhs);
			else
				Ptr->Release(), Ptr = NULL;
		}
		else
		{
			Ptr = TJSAllocVariantString(rhs);
		}
		return *this;
	}

	TJS_METHOD_DEF(tTJSString &, operator =, (const tjs_nchar *rhs))
	{
		if(Ptr) Ptr->Release();
		Ptr = TJSAllocVariantString(rhs);
		return *this;
	}

#ifdef TJS_SUPPORT_VCL
	tTJSString & operator =(AnsiString &rhs)
	{
		if(Ptr) Ptr->Release();
		Ptr = TJSAllocVariantString(rhs.c_str());
		return *this;
	}

	tTJSString & operator =(WideString &rhs)
	{
		if(Ptr) Ptr->Release();
		Ptr = TJSAllocVariantString(rhs.c_bstr());
		return *this;
	}
#endif

	//------------------------------------------------------------ compare --
	TJS_CONST_METHOD_DEF(bool, operator ==, (const tTJSString & ref))
	{
		if(Ptr == ref.Ptr) return true; // both empty or the same pointer
		if(!Ptr && ref.Ptr) return false;
		if(Ptr && !ref.Ptr) return false;
		if(Ptr->Length != ref.Ptr->Length) return false;
		return !TJS_strcmp(*Ptr, *ref.Ptr);
	}

	TJS_CONST_METHOD_DEF(bool, operator !=, (const tTJSString &ref))
	{
		return ! this->operator == (ref);
	}

	TJS_CONST_METHOD_DEF(tjs_int, CompareIC, (const tTJSString & ref))
	{
		if(!Ptr && !ref.Ptr) return true; // both empty string
		if(!Ptr && ref.Ptr) return false;
		if(Ptr && !ref.Ptr) return false;
		return TJS_stricmp(*Ptr, *ref.Ptr);
	}

	TJS_CONST_METHOD_DEF(bool, operator ==, (const tjs_char * ref))
	{
		bool rnemp = ref && ref[0];
		if(!Ptr && !rnemp) return true; // both empty string
		if(!Ptr && rnemp) return false;
		if(Ptr && !rnemp) return false;
		return !TJS_strcmp(*Ptr, ref);
	}

	TJS_CONST_METHOD_DEF(bool, operator !=, (const tjs_char * ref))
	{
		return ! this->operator == (ref);
	}

	TJS_CONST_METHOD_DEF(tjs_int, CompareIC, (const tjs_char * ref))
	{
		bool rnemp = ref && ref[0];
		if(!Ptr && !rnemp) return true; // both empty string
		if(!Ptr && rnemp) return false;
		if(Ptr && !rnemp) return false;
		return TJS_stricmp(*Ptr, ref);
	}

	TJS_CONST_METHOD_DEF(bool, operator <, (const tTJSString &ref))
	{
		if(!Ptr && !ref.Ptr) return false;
		if(!Ptr && ref.Ptr) return true;
		if(Ptr && !ref.Ptr) return false;
		return TJS_strcmp(*Ptr, *ref.Ptr)<0;
	}

	TJS_CONST_METHOD_DEF(bool, operator >, (const tTJSString &ref))
	{
		if(!Ptr && !ref.Ptr) return false;
		if(!Ptr && ref.Ptr) return false;
		if(Ptr && !ref.Ptr) return true;
		return TJS_strcmp(*Ptr, *ref.Ptr)>0;
	}

	//---------------------------------------------------------- operation --
	TJS_METHOD_DEF(void, operator +=, (const tTJSString &ref))
	{
		if(!ref.Ptr) return;
		Independ();
		Ptr = TJSAppendVariantString(Ptr, *ref.Ptr);
	}

	TJS_METHOD_DEF(void, operator +=, (const tTJSVariantString *ref))
	{
		if(!ref) return;
		Independ();
		Ptr = TJSAppendVariantString(Ptr, ref);
	}

	TJS_METHOD_DEF(void, operator +=, (const tjs_char *ref))
	{
		if(!ref) return;
		Independ();
		Ptr = TJSAppendVariantString(Ptr, ref);
	}

	TJS_METHOD_DEF(void, operator +=, (tjs_char rch))
	{
		Independ();
		tjs_char ch[2];
		ch[0] = rch;
		ch[1] = 0;
		Ptr = TJSAppendVariantString(Ptr, ch);
	}

	TJS_CONST_METHOD_DEF(tTJSString, operator +, (const tTJSString &ref))
	{
		if(!ref.Ptr && !Ptr) return tTJSString();
		if(!ref.Ptr) return *this;
		if(!Ptr) return ref;

		tTJSString newstr;
		newstr.Ptr = TJSAllocVariantString(*this->Ptr, *ref.Ptr);
		return newstr;
	}

	TJS_CONST_METHOD_DEF(tTJSString, operator +, (const tjs_char *ref))
	{
		if(!ref && !Ptr) return tTJSString();
		if(!ref) return *this;
		if(!Ptr) return tTJSString(ref);

		tTJSString newstr;
		newstr.Ptr = TJSAllocVariantString(*this->Ptr, ref);
		return newstr;
	}

	TJS_CONST_METHOD_DEF(tTJSString, operator +, (tjs_char rch))
	{
		if(!Ptr) return tTJSString(rch);
		tjs_char ch[2];
		ch[0] = rch;
		ch[1] = 0;
		tTJSString newstr;
		newstr.Ptr = TJSAllocVariantString(*this->Ptr, ch);
		return newstr;
	}

	/*m[*/
	friend tTJSString operator + (const tjs_char *lhs, const tTJSString &rhs);
	/*]m*/

	TJS_CONST_METHOD_DEF(tjs_char, operator [], (tjs_uint i))
	{
		// returns character at i. this function does not check the range.
		if(!Ptr) return 0;
		return Ptr->operator const tjs_char *() [i];
	}

	TJS_METHOD_DEF(void, Clear, ())
	{
		if(Ptr) Ptr->Release(), Ptr = NULL;
	}

	TJS_METHOD_DEF(tjs_char *, AllocBuffer, (tjs_uint len))
	{
		/* you must call FixLen when you allocate larger buffer than actual string length */

		if(Ptr) Ptr->Release();
		Ptr = TJSAllocVariantStringBuffer(len);
		return const_cast<tjs_char*>(Ptr->operator const tjs_char *());
	}

	TJS_METHOD_DEF(tjs_char *, AppendBuffer, (tjs_uint len))
	{
		/* you must call FixLen when you allocate larger buffer than actual string length */

		if(!Ptr) return AllocBuffer(len);
		Independ();
		Ptr->AppendBuffer(len);
		return const_cast<tjs_char *>(Ptr->operator const tjs_char *());
	}


	TJS_METHOD_DEF(void, FixLen, ())
	{
		Independ();
		if(Ptr) Ptr = Ptr->FixLength();
	}

	TJS_METHOD_DEF(void, Replace,
		(const tTJSString &from, const tTJSString &to, bool forall = true));

	tTJSString AsLowerCase() const;
	tTJSString AsUpperCase() const;

	TJS_CONST_METHOD_DEF(void, AsLowerCase, (tTJSString &dest)) { dest = AsLowerCase(); }
	TJS_CONST_METHOD_DEF(void, AsUpperCase, (tTJSString &dest)) { dest = AsUpperCase(); }

	TJS_METHOD_DEF(void, ToLowerCase, ());
	TJS_METHOD_DEF(void, ToUppserCase, ());

	tjs_int printf(const tjs_char *format, ...);

	tTJSString EscapeC() const;   // c-style string escape/unescaep
	tTJSString UnescapeC() const;

	TJS_CONST_METHOD_DEF(void, EscapeC, (tTJSString &dest)) { dest = EscapeC(); }
	TJS_CONST_METHOD_DEF(void, UnescapeC, (tTJSString &dest)) { dest = UnescapeC(); }

	TJS_CONST_METHOD_DEF(bool, StartsWith, (const tjs_char *string));
	TJS_CONST_METHOD_DEF(bool, StartsWith, (const tTJSString & string))
		{ return StartsWith(string.c_str()); }

	TJS_METHOD_DEF(tjs_uint32 *, GetHint, ()) { if(!Ptr) return NULL; return Ptr->GetHint(); }

	TJS_CONST_METHOD_DEF(tjs_int, GetNarrowStrLen, ());
	TJS_CONST_METHOD_DEF(void, ToNarrowStr, (tjs_nchar *dest, tjs_int destmaxlen));

	//------------------------------------------------------------- others --
	TJS_CONST_METHOD_DEF(bool, IsEmpty, ()) { return Ptr==NULL; }

private:
	tjs_char * InternalIndepend();

public:
	TJS_METHOD_DEF(tjs_char *, Independ, ())
	{
		// severs sharing of the string instance
		// and returns independent internal buffer

		// note that you must call FixLen after making modification of the buffer
		// if you shorten the string using this method's return value.
		// USING THIS METHOD'S RETURN VALUE AND MODIFYING THE INTERNAL
		// BUFFER IS VERY DANGER.

		if(!Ptr) return NULL;

		if(Ptr->GetRefCount() == 0)
		{
			// already indepentent
			return const_cast<tjs_char *>(Ptr->operator const tjs_char *());
		}
		return InternalIndepend();
	}


	TJS_CONST_METHOD_DEF(tjs_int, GetLen, ())
	{
#ifdef __CODEGUARD__
		if(!Ptr) return 0; // tTJSVariantString::GetLength can return zero if 'this' is NULL
#endif
		return Ptr->GetLength();
	}

	TJS_CONST_METHOD_DEF(tjs_int, length, ()) { return GetLen(); }

	TJS_CONST_METHOD_DEF(tjs_char, GetLastChar, ())
	{
		if(!Ptr) return (tjs_char)0;
		const tjs_char * p = Ptr->operator const tjs_char*();
		return p[Ptr->GetLength() - 1];
	}


	//---------------------------------------------- allocator/deallocater --
	TJS_STATIC_METHOD_DEF(void *, operator new, (size_t size)) { return new char[size]; }
	TJS_STATIC_METHOD_DEF(void, operator delete, (void * p)) { delete [] ((char*)p); }

	TJS_STATIC_METHOD_DEF(void *, operator new [], (size_t size)) { return new char[size]; }
	TJS_STATIC_METHOD_DEF(void, operator delete [], (void *p)) { delete [] ((char*)p); }

	TJS_STATIC_METHOD_DEF(void *, operator new, (size_t size, void *buf)) { return buf; }
    //--------------------------------------------- indexer / finder --
    int      IndexOf (const tTJSString& str, unsigned int pos = 0) const;
    int      IndexOf (const char* s, unsigned int pos = 0, unsigned int n = -1) const { return IndexOf(tTJSString(s, n), pos); }
    int      IndexOf (tjs_char c, unsigned int pos = 0) const { return IndexOf(tTJSString(&c, 1), pos); }
    tTJSString    SubString(unsigned int pos, unsigned int len) const;
    tTJSString Trim();
};
/*end-of-tTJSString*/
TJS_EXP_FUNC_DEF(tTJSString, operator +, (const tjs_char *lhs, const tTJSString &rhs));


//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tTJSString, TJSInt32ToHex, (tjs_uint32 num, int zeropad = 8));
//---------------------------------------------------------------------------
/*[*/
typedef tTJSString ttstr;
/*]*/
//---------------------------------------------------------------------------
} // namespace TJS
#endif
