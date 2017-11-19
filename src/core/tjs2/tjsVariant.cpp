//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// implementation of tTJSVariant
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsVariant.h"
#include "tjsError.h"
#include "tjsLex.h"
#include "tjsUtils.h"
#include "tjsDebug.h"


namespace TJS
{
//---------------------------------------------------------------------------
// tTJSVariantOctet related
//---------------------------------------------------------------------------
tTJSVariantOctet::tTJSVariantOctet(const tjs_uint8 *data, tjs_uint length)
{
	// TODO : tTJSVariantOctet check
	RefCount = 1;
	Length = length;
	Data = new tjs_uint8[length];
	TJS_octetcpy(Data, data, length);
}
//---------------------------------------------------------------------------
tTJSVariantOctet::tTJSVariantOctet(const tjs_uint8 *data1, tjs_uint len1,
	const tjs_uint8 *data2, tjs_uint len2)
{
	RefCount = 1;
	Length = len1 + len2;
	Data = new tjs_uint8[Length];
	if(len1) TJS_octetcpy(Data, data1, len1);
	if(len2) TJS_octetcpy(Data + len1, data2, len2);
}
//---------------------------------------------------------------------------
tTJSVariantOctet::tTJSVariantOctet(const tTJSVariantOctet *o1,
	const tTJSVariantOctet *o2)
{
	RefCount = 1;
	Length = (o1?o1->Length:0) + (o2?o2->Length:0);
	Data = new tjs_uint8[Length];
	if(o1 && o1->Length) TJS_octetcpy(Data, o1->Data, o1->Length);
	if(o2 && o2->Length) TJS_octetcpy(Data + (o1?o1->Length:0), o2->Data,
							o2->Length);
}
//---------------------------------------------------------------------------
tTJSVariantOctet::~tTJSVariantOctet()
{
	delete [] Data;
}
//---------------------------------------------------------------------------
void tTJSVariantOctet::Release()
{
	if(RefCount == 1)
		delete this;
	else
		RefCount--;
}
//---------------------------------------------------------------------------
tTJSVariantOctet * TJSAllocVariantOctet(const tjs_uint8 *data, tjs_uint length)
{
	if(!data) return NULL;
	if(length == 0) return NULL;
	return new tTJSVariantOctet(data, length);
}
//---------------------------------------------------------------------------
tTJSVariantOctet * TJSAllocVariantOctet(const tjs_uint8 *data1, tjs_uint len1,
	const tjs_uint8 *data2, tjs_uint len2)
{
	if(!data1) len1 = 0;
	if(!data2) len2 = 0;
	if(len1 + len2 == 0) return NULL;

	return new tTJSVariantOctet(data1, len1, data2, len2);
}
//---------------------------------------------------------------------------
tTJSVariantOctet * TJSAllocVariantOctet(const tTJSVariantOctet *o1, const
	tTJSVariantOctet *o2)
{
	if(!o1 && !o2) return NULL;
	return new tTJSVariantOctet(o1, o2);
}
//---------------------------------------------------------------------------
tTJSVariantOctet * TJSAllocVariantOctet(const tjs_uint8 **src)
{
	tjs_uint size = *(const tjs_uint*)(*src);
	*src += sizeof(tjs_uint);
	if(!size) return NULL;
	tTJSVariantOctet *octet =  new tTJSVariantOctet(*src, size);
	*src += size;
	return octet;
}
//---------------------------------------------------------------------------
void TJSDeallocVariantOctet(tTJSVariantOctet *o)
{
	delete o;
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSOctetToListString(const tTJSVariantOctet *oct)
{
	if(!oct) return NULL;
	if(oct->GetLength() == 0) return NULL;
	tjs_int stringlen = oct->GetLength() * 3 -1;
	tTJSVariantString * str = TJSAllocVariantStringBuffer(stringlen);

	tjs_char *buf = const_cast<tjs_char*>(str->operator const tjs_char*());
	static const tjs_char hex[] = TJS_W("0123456789ABCDEF");
	const tjs_uint8 *data = oct->GetData();
	tjs_uint n = oct->GetLength();
	while(n--)
	{
		buf[0] = hex[*data >> 4];
		buf[1] = hex[*data & 0x0f];
		if(n != 0)
			buf[2] = TJS_W(' ');
		buf+=3;
		data++;
	}

	return str;
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// Utility Functions
//---------------------------------------------------------------------------
tTJSVariantClosure_S TJSNullVariantClosure={NULL,NULL};
//---------------------------------------------------------------------------
void TJSThrowVariantConvertError(const tTJSVariant & from, tTJSVariantType to)
{
	if(to == tvtObject)
	{
		ttstr msg(TJSVariantConvertErrorToObject);

		msg.Replace(TJS_W("%1"), TJSVariantToReadableString(from));

		TJS_eTJSVariantError(msg);
	}
	else
	{
		ttstr msg(TJSVariantConvertError);

		msg.Replace(TJS_W("%1"), TJSVariantToReadableString(from));

		msg.Replace(TJS_W("%2"), TJSVariantTypeToTypeString(to));

		TJS_eTJSVariantError(msg);
	}
}
//---------------------------------------------------------------------------
void TJSThrowVariantConvertError(const tTJSVariant & from, tTJSVariantType to1,
	tTJSVariantType to2)
{
	ttstr msg(TJSVariantConvertError);

	msg.Replace(TJS_W("%1"), TJSVariantToReadableString(from));

	msg.Replace(TJS_W("%2"), TJSVariantTypeToTypeString(to1) + ttstr(TJS_W("/")) +
		TJSVariantTypeToTypeString(to2));

	TJS_eTJSVariantError(msg);
}
//---------------------------------------------------------------------------
void TJSThrowNullAccess()
{
	TJS_eTJSError(TJSNullAccess);
}
//---------------------------------------------------------------------------
void TJSThrowDivideByZero()
{
	TJS_eTJSVariantError(TJSDivideByZero);
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
tTJSVariantString * TJSObjectToString(const tTJSVariantClosure &dsp)
{
	if(TJSObjectTypeInfoEnabled())
	{
		// retrieve object type information from debugging facility
		tjs_char tmp[256];
		TJS_snprintf(tmp, sizeof(tmp)/sizeof(tjs_char), TJS_W("(object %p"), dsp.Object);
		ttstr ret = tmp;
		ttstr type = TJSGetObjectTypeInfo(dsp.Object);
		if(!type.IsEmpty()) ret += TJS_W("[") + type + TJS_W("]");
		TJS_snprintf(tmp, sizeof(tmp)/sizeof(tjs_char), TJS_W(":%p"), dsp.ObjThis);
		ret += tmp;
		type = TJSGetObjectTypeInfo(dsp.ObjThis);
		if(!type.IsEmpty()) ret += TJS_W("[") + type + TJS_W("]");
		ret += TJS_W(")");
		tTJSVariantString * str = ret.AsVariantStringNoAddRef();
		str->AddRef();
		return str;
	}
	else
	{
		tjs_char tmp[256];
		TJS_snprintf(tmp, sizeof(tmp)/sizeof(tjs_char), TJS_W("(object %p:%p)"), dsp.Object, dsp.ObjThis);
		return TJSAllocVariantString(tmp);
	}
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSIntegerToString(tjs_int64 i)
{
	tjs_char tmp[34];
	return TJSAllocVariantString( TJS_tTVInt_to_str( i , tmp));
}
//---------------------------------------------------------------------------
static  tTJSVariantString * TJSSpecialRealToString(tjs_real r)
{
	tjs_int32 cls = TJSGetFPClass(r);

	if(TJS_FC_IS_NAN(cls))
	{
		return TJSAllocVariantString(TJS_W("NaN"));
	}
	if(TJS_FC_IS_INF(cls))
	{
		if(TJS_FC_IS_NEGATIVE(cls))
			return TJSAllocVariantString(TJS_W("-Infinity"));
		else
			return TJSAllocVariantString(TJS_W("+Infinity"));
	}
	if(r == 0.0)
	{
		if(TJS_FC_IS_NEGATIVE(cls))
			return TJSAllocVariantString(TJS_W("-0.0"));
		else
			return TJSAllocVariantString(TJS_W("+0.0"));
	}
	return NULL;
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSRealToString(tjs_real r)
{
	tTJSVariantString *v = TJSSpecialRealToString(r);
	if(v) return v;

	TJSSetFPUE();
	tjs_char tmp[128];
	TJS_snprintf(tmp, sizeof(tmp)/sizeof(tjs_char), TJS_W("%.15lg"), r);
    tjs_char ttmp[64];
    // fast convert from ascii-only string
    for(int i = 0; i < sizeof(tmp); ++i) {
        int c = tmp[i];
        ttmp[i] = c;
        if(!c) break;
	}

	return TJSAllocVariantString(ttmp);
}
//---------------------------------------------------------------------------
tTJSVariantString * TJSRealToHexString(tjs_real r)
{
	tTJSVariantString *v = TJSSpecialRealToString(r);
	if(v) return v;

	tjs_uint64 *ui64 = (tjs_uint64*)&r;

	tjs_char tmp[128];

	tjs_char *p;

	if(TJS_IEEE_D_GET_SIGN(*ui64))
	{
		TJS_strcpy(tmp, TJS_W("-0x1."));
		p = tmp + 5;
	}
	else
	{
		TJS_strcpy(tmp, TJS_W("0x1."));
		p = tmp + 4;
	}

	static tjs_char hexdigits[] = TJS_W("0123456789ABCDEF");

	tjs_int exp = TJS_IEEE_D_GET_EXP(*ui64);

	tjs_int bits = TJS_IEEE_D_SIGNIFICAND_BITS;

	while(true)
	{
		bits -= 4;
		if(bits < 0) break;
		*(p++) = hexdigits[(tjs_int)(*ui64 >> bits) & 0x0f];
	}

	*(p++) = TJS_W('p');
	//TJS_sprintf(p, TJS_W("%d"), exp);
	//TJS_snprintf(p, (sizeof(tmp)-(p-tmp))/sizeof(tjs_char), TJS_W("%d"), exp);
	tjs_char t2[128];
	TJS_snprintf( t2, 128, TJS_W("%d"), exp);
	for( tjs_int i = 0; t2[i] != 0 && p != (tmp+128); i++ ) {
		*(p++) = t2[i];
	}
	if( p != (tmp+128) ) *p = 0;
	else tmp[127] = 0;

	return TJSAllocVariantString(tmp);
}
//---------------------------------------------------------------------------
tTVInteger TJSStringToInteger(const tjs_char *str)
{
	tTJSVariant val;
	if(TJSParseNumber(val, &str)) 	return val.AsInteger();
	return 0;
}
//---------------------------------------------------------------------------
tTVReal TJSStringToReal(const tjs_char *str)
{
	tTJSVariant val;
	if(TJSParseNumber(val, &str)) 	return val.AsReal();
	return 0;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSVariant member
//---------------------------------------------------------------------------
tTJSVariant::tTJSVariant(const tTJSVariant &ref) // from tTJSVariant
{

	tTJSVariant_BITCOPY(*this, ref);
	if(vt==tvtObject)
	{
		if(Object.Object) Object.Object->AddRef();
		if(Object.ObjThis) Object.ObjThis->AddRef();
	}
	else
	{
		if(vt==tvtString) {	if(String) String->AddRef(); }
		else if(vt==tvtOctet) { if(Octet) Octet->AddRef(); }
	}
}
//---------------------------------------------------------------------------
tTJSVariant::tTJSVariant(const tjs_uint8 ** src)
{
	// from persistent storage
	vt = (tTJSVariantType) **src;
	src++;

	switch(vt)
	{
	case tvtVoid:
		break;

	case tvtObject:
		Object = *(tTJSVariantClosure_S*)(*src);
		// no addref
		break;

	case tvtString:
		String = TJSAllocVariantString(src);
		break;

	case tvtInteger:
		Integer = *(const tTVInteger *)(*src);
		break;

	case tvtReal:
		TJSSetFPUE();
		Real = *(const tTVReal *)(*src);
		break;

	case tvtOctet:
		Octet = TJSAllocVariantOctet(src);
		break;

	default:
		break; // unknown type
	}
}
//---------------------------------------------------------------------------
tTJSVariant::~tTJSVariant()
{
	Clear();
}
//---------------------------------------------------------------------------
void tTJSVariant::Clear()
{
	tTJSVariantType o_vt = vt;
	vt = tvtVoid;
	switch(o_vt)
	{
	case tvtObject:
		if(Object.Object) Object.Object->Release();
		if(Object.ObjThis) Object.ObjThis->Release();
		break;
	case tvtString:
		if(String) String->Release();
		break;
	case tvtOctet:
		if(Octet) Octet->Release();
		break;
	default:
		break;
	}
}
//---------------------------------------------------------------------------
void tTJSVariant::ToString()
{
	switch(vt)
	{
	case tvtVoid:
		String=NULL;
		vt=tvtString;
		break;

	case tvtObject:
	  {
		tTJSVariantString * string = TJSObjectToString(*(tTJSVariantClosure*)&Object);
		ReleaseObject();
		String = string;
		vt=tvtString;
		break;
	  }

	case tvtString:
		break;

	case tvtInteger:
		String=TJSIntegerToString(Integer);
		vt=tvtString;
		break;

	case tvtReal:
		String=TJSRealToString(Real);
		vt=tvtString;
		break;

	case tvtOctet:
		TJSThrowVariantConvertError(*this, tvtString);
		break;
	}
}
//---------------------------------------------------------------------------
void tTJSVariant::ToOctet()
{
	switch(vt)
	{
	case tvtVoid:
		Octet = NULL;
		vt = tvtOctet;
		break;


	case tvtOctet:
		break;

	case tvtString:
	case tvtInteger:
	case tvtReal:
	case tvtObject:
		TJSThrowVariantConvertError(*this, tvtOctet);
	}
}
//---------------------------------------------------------------------------
void tTJSVariant::ToInteger()
{
	tTVInteger i = AsInteger();
	ReleaseContent();
	vt = tvtInteger;
	Integer = i;
}
//---------------------------------------------------------------------------
void tTJSVariant::ToReal()
{
	tTVReal r = AsReal();
	ReleaseContent();
	vt = tvtReal;
	Real = r;
}
//---------------------------------------------------------------------------
void tTJSVariant::CopyRef(const tTJSVariant & ref) // from reference to tTJSVariant
{
	switch(ref.vt)
	{
	case tvtVoid:
		ReleaseContent();
		vt = tvtVoid;
		return;
	case tvtObject:
		if(this != &ref)
		{
			/*
				note:
				ReleaseContent makes the object variables null during clear,
				thus makes the resulting object also null when the ref and this are
				exactly the same object.
				This does not affect string nor octet because the ReleaseContent
				does *not* make the pointer null during clear when the variant type
				is string or octet.
			*/
			((tTJSVariantClosure&)ref.Object).AddRef();
			ReleaseContent();
			Object = ref.Object;
			vt = tvtObject;
		}
		return;
	case tvtString:
		if(ref.String) ref.String->AddRef();
		ReleaseContent();
		String = ref.String;
		vt = tvtString;
		return;
	case tvtOctet:
		if(ref.Octet) ref.Octet->AddRef();
		ReleaseContent();
		Octet = ref.Octet;
		vt = tvtOctet;
		return;
	default:
		ReleaseContent();
		tTJSVariant_BITCOPY(*this,ref);
		return;
	}

}
//---------------------------------------------------------------------------

tTJSVariant & tTJSVariant::operator =(iTJSDispatch2 *ref) // from Object
{
	if(ref) ref->AddRef();
	ReleaseContent();
	vt = tvtObject;
	Object.Object = ref;
	Object.ObjThis = NULL;
	return *this;
}
//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::SetObject(iTJSDispatch2 *object, iTJSDispatch2 *objthis)
{
	if(object) object->AddRef();
	if(objthis) objthis->AddRef();
	ReleaseContent();
	vt = tvtObject;
	Object.Object = object;
	Object.ObjThis = objthis;
	return *this;
}
//---------------------------------------------------------------------------

tTJSVariant & tTJSVariant::operator =(tTJSVariantClosure ref) // from Object Closure
{
	ReleaseContent();
	vt=tvtObject;
	Object=ref;
	AddRefContent();
	return *this;
}
//---------------------------------------------------------------------------

#ifdef TJS_SUPPORT_VCL
tTJSVariant & tTJSVariant::operator =(WideString s) // from WideString
{
	ReleaseContent();
	vt=tvtString;
	if(s.IsEmpty())
		String=NULL;
	else
		String=TJSAllocVariantString(s.c_bstr());
	return *this;
}
#endif

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(tTJSVariantString *ref) // from tTJSVariantString
{
	if(ref) ref->AddRef();
	ReleaseContent();
	vt = tvtString;
	String = ref;
	return *this;
}

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(tTJSVariantOctet *ref) // from tTJSVariantOctet
{
	if(ref) ref->AddRef();
	ReleaseContent();
	vt = tvtOctet;
	Octet = ref;
	return *this;
}

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(const tTJSString & ref) // from tTJSString
{
	ReleaseContent();
	vt = tvtString;
	String = ref.AsVariantStringNoAddRef();
	if(String) String->AddRef();
	return *this;
}

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(const tjs_char *ref) //  from String
{
	ReleaseContent();
	vt=tvtString;
	String=TJSAllocVariantString(ref);
	return *this;
}

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(const tjs_nchar *ref) // from narrow string
{
	ReleaseContent();
	vt=tvtString;
	String = TJSAllocVariantString(ref);
	return *this;
}

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(bool ref)
{
	ReleaseContent();
	vt=tvtInteger;
	Integer=(tjs_int64)(tjs_int)ref;
	return *this;
}

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(tjs_int32 ref)
{
	ReleaseContent();
	vt=tvtInteger;
	Integer=(tTVInteger)ref;
	return *this;
}

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(const tTVInteger ref) // from Integer64
{
	ReleaseContent();
	vt=tvtInteger;
	Integer=ref;
	return *this;
}

//---------------------------------------------------------------------------
tTJSVariant & tTJSVariant::operator =(tjs_real ref) // from double
{
	ReleaseContent();
	TJSSetFPUE();
	vt=tvtReal;
	Real=ref;
	return *this;
}
//---------------------------------------------------------------------------
bool tTJSVariant::NormalCompare(const tTJSVariant &val2) const
{
	try
	{

		if(vt == val2.vt)
		{
			if(vt == tvtInteger)
			{
				return Integer == val2.Integer;
			}

			if(vt == tvtString)
			{
				tTJSVariantString *s1, *s2;
				s1 = String;
				s2 = val2.String;
				if(s1 == s2) return true; // both empty string or the same pointer
				if(!s1 && s2) return false;
				if(s1 && !s2) return false;
				if(s1->Length != s2->Length) return false;
				return (! TJS_strcmp(*s1, *s2));
			}

			if(vt == tvtOctet)
			{
				if(Octet == val2.Octet) return true; // both empty octet or the same pointer
				if(!Octet && val2.Octet) return false;
				if(Octet && !val2.Octet) return false;
				if(Octet->GetLength() != val2.Octet->GetLength()) return false;
				return !TJS_octetcmp(Octet->GetData(), val2.Octet->GetData(),
					Octet->GetLength());
			}

			if(vt == tvtObject)
			{
				return Object.Object == val2.Object.Object/* &&
					Object.ObjThis == val2.Object.ObjThis*/;
			}

			if(vt == tvtVoid) return true;
		}

		if(vt==tvtString || val2.vt==tvtString)
		{
			tTJSVariantString *s1, *s2;
			s1 = AsString();
			s2 = val2.AsString();
			if(!s1 && !s2) return true; // both empty string
			if(!s1 && s2)
			{
				s2->Release();
				return false;
			}
			if(s1 && !s2)
			{
				s1->Release();
				return false;
			}
			bool res = ! TJS_strcmp(*s1, *s2);
			s1->Release();
			s2->Release();
			return res;
		}

		if(vt == tvtVoid)
		{
			switch(val2.vt)
			{
			case tvtInteger:	return val2.Integer == 0;
			case tvtReal:		{ TJSSetFPUE(); return val2.Real == 0; }
			case tvtString:		return val2.String == 0;
			default:			return false;
			}
		}
		if(val2.vt == tvtVoid)
		{
			switch(vt)
			{
			case tvtInteger:	return Integer == 0;
			case tvtReal:		{ TJSSetFPUE(); return Real == 0; }
			case tvtString:		return String == 0;
			default:			return false;
			}
		}
#if 1
		static const bool TypeComparableTable[6][6] = {
			//	   v, o, s, 8, i, r
			/*v*/{ 1, 0, 1, 0, 1, 1 },
			/*o*/{ 0, 1, 0, 0, 0, 0 },
			/*s*/{ 1, 0, 1, 0, 1, 1 },
			/*8*/{ 0, 0, 0, 1, 0, 0 },
			/*i*/{ 1, 0, 1, 0, 1, 1 },
			/*r*/{ 1, 0, 1, 0, 1, 1 },
		};
		if (!TypeComparableTable[vt][val2.vt])
			return false;
#endif

		TJSSetFPUE();

		tTVReal r1 = AsReal();
		tTVReal r2 = val2.AsReal();

		tjs_uint32 c1 = TJSGetFPClass(r1);
		tjs_uint32 c2 = TJSGetFPClass(r2);

		if(TJS_FC_IS_NAN(c1) || TJS_FC_IS_NAN(c2)) return false;
			// compare to NaN is always false
		if(TJS_FC_IS_INF(c1) || TJS_FC_IS_INF(c2))
		{
			return c1 == c2;
			// +inf == +inf : true , -inf == -inf : true, otherwise : false
		}
		return r1 == r2;
	}
	catch(eTJSVariantError &/*e*/)
	{
		return false;
	}
	catch(...)
	{
		throw;
	}
}
//---------------------------------------------------------------------------
bool tTJSVariant::DiscernCompare(const tTJSVariant &val2) const
{
	if(vt==val2.vt)
	{
		switch(vt)
		{
		case tvtObject:
			return Object.Object == val2.Object.Object &&
				Object.ObjThis == val2.Object.ObjThis;
		case tvtString:
			return NormalCompare(val2);
		case tvtOctet:
			return NormalCompare(val2);
		case tvtVoid:
			return true;
		case tvtReal:
		  {
			TJSSetFPUE();

			tjs_uint32 c1 = TJSGetFPClass(Real);
			tjs_uint32 c2 = TJSGetFPClass(val2.Real);

			if(TJS_FC_IS_NAN(c1) || TJS_FC_IS_NAN(c2)) return false;
				// compare to NaN is always false
			if(TJS_FC_IS_INF(c1) || TJS_FC_IS_INF(c2))
			{
				return c1 == c2;
				// +inf == +inf : true , -inf == -inf : true, otherwise : false
			}
			return Real == val2.Real;
		  }
		case tvtInteger:
			return Integer == val2.Integer;
		}
		return false;
	}
	else
	{
		return false;
	}
}
//---------------------------------------------------------------------------
bool tTJSVariant::DiscernCompareStrictReal(const tTJSVariant &val2) const
{
	// this performs strict real compare
	if(vt == val2.vt && vt == tvtReal)
	{
		tjs_uint64 *ui64 = (tjs_uint64*)&Real;
		tjs_uint64 *v2ui64 = (tjs_uint64*)&(val2.Real);
		return *ui64 == *v2ui64;
	}

	return DiscernCompare(val2);
}
//---------------------------------------------------------------------------
bool tTJSVariant::GreaterThan(const tTJSVariant &val2) const
{
	if(vt!=tvtString || val2.vt!=tvtString)
	{
		// compare as number
		if(vt==tvtInteger && val2.vt==tvtInteger)
		{
			return AsInteger()<val2.AsInteger();
		}
		TJSSetFPUE();
		return AsReal()<val2.AsReal();
	}
	// compare as string
	tTJSVariantString *s1, *s2;
	s1 = AsString();
	s2 = val2.AsString();
	const tjs_char *p1 = *s1;
	const tjs_char *p2 = *s2;
	if(!p1) p1=TJS_W("");
	if(!p2) p2=TJS_W("");
	bool res = TJS_strcmp(p1, p2)<0;
	if(s1) s1->Release();
	if(s2) s2->Release();
	return res;
}
//---------------------------------------------------------------------------
bool tTJSVariant::LittlerThan(const tTJSVariant &val2) const
{
	if(vt!=tvtString || val2.vt!=tvtString)
	{
		// compare as number
		if(vt==tvtInteger && val2.vt==tvtInteger)
		{
			return AsInteger()>val2.AsInteger();
		}
		TJSSetFPUE();
		return AsReal()>val2.AsReal();
	}
	// compare as string
	tTJSVariantString *s1, *s2;
	s1 = AsString();
	s2 = val2.AsString();
	const tjs_char *p1 = *s1;
	const tjs_char *p2 = *s2;
	if(!p1) p1=TJS_W("");
	if(!p2) p2=TJS_W("");
	bool res = TJS_strcmp(p1, p2)>0;
	if(s1) s1->Release();
	if(s2) s2->Release();
	return res;
}
//---------------------------------------------------------------------------
void tTJSVariant::logicalorequal (const tTJSVariant &rhs)
{
	bool l=operator bool();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l||rhs.operator bool();
}
//---------------------------------------------------------------------------
void tTJSVariant::logicalandequal (const tTJSVariant &rhs)
{
	bool l=operator bool();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l&&rhs.operator bool();
}
//---------------------------------------------------------------------------
void tTJSVariant::operator |= (const tTJSVariant &rhs)
{
	tTVInteger l=AsInteger();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l|rhs.AsInteger();
}
//---------------------------------------------------------------------------
void tTJSVariant::increment(void)
{
	if(vt == tvtString)
		String->ToNumber(*this);

	if(vt == tvtReal)
	{
		TJSSetFPUE();
		Real+=1.0;
	}
	else if(vt == tvtInteger)
		Integer ++;
	else if(vt == tvtVoid)
		vt = tvtInteger, Integer = 1;
	else
		TJSThrowVariantConvertError(*this, tvtInteger, tvtReal);
}
//---------------------------------------------------------------------------
void tTJSVariant::decrement(void)
{
	if(vt == tvtString)
		String->ToNumber(*this);

	if(vt == tvtReal)
	{
		TJSSetFPUE();
		Real-=1.0;
	}
	else if(vt == tvtInteger)
		Integer --;
	else if(vt == tvtVoid)
		vt = tvtInteger, Integer = -1;
	else
		TJSThrowVariantConvertError(*this, tvtInteger, tvtReal);
}
//---------------------------------------------------------------------------
void tTJSVariant::operator ^= (const tTJSVariant &rhs)
{
	tjs_int64 l=AsInteger();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l^rhs.AsInteger();
}
//---------------------------------------------------------------------------
void tTJSVariant::operator &= (const tTJSVariant &rhs)
{
	tTVInteger l=AsInteger();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l&rhs.AsInteger();
}
//---------------------------------------------------------------------------
void tTJSVariant::operator >>= (const tTJSVariant &rhs)
{
	tTVInteger l=AsInteger();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l>>(tjs_int)rhs.AsInteger();
}
//---------------------------------------------------------------------------
void tTJSVariant::rbitshiftequal (const tTJSVariant &rhs)
{
	tTVInteger l=AsInteger();
	ReleaseContent();
	vt=tvtInteger;
	Integer=(tjs_int64)((tjs_uint64)l>> (tjs_int)rhs);
}
//---------------------------------------------------------------------------
void tTJSVariant::operator <<= (const tTJSVariant &rhs)
{
	tTVInteger l=AsInteger();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l<<(tjs_int)rhs.AsInteger();
}
//---------------------------------------------------------------------------
void tTJSVariant::operator %= (const tTJSVariant &rhs)
{
	tTVInteger r=rhs.AsInteger();
	if(r == 0) TJSThrowDivideByZero();
	tTVInteger l=AsInteger();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l%r;
}
//---------------------------------------------------------------------------
void tTJSVariant::operator /= (const tTJSVariant &rhs)
{
	TJSSetFPUE();
	tTVReal l=AsReal();
	tTVReal r=rhs.AsReal();
	ReleaseContent();
	vt=tvtReal;
	Real=l/r;
}
//---------------------------------------------------------------------------
void tTJSVariant::idivequal (const tTJSVariant &rhs)
{
	tTVInteger r=rhs.AsInteger();
	tTVInteger l=AsInteger();
	if(r == 0) TJSThrowDivideByZero();
	ReleaseContent();
	vt=tvtInteger;
	Integer=l/r;
}
//---------------------------------------------------------------------------
void tTJSVariant::InternalMul(const tTJSVariant &rhs)
{
	tTJSVariant l;
	AsNumber(l);
	ReleaseContent();
	tTJSVariant r;
	rhs.AsNumber(r);
	if(l.vt == tvtInteger && r.vt == tvtInteger)
	{
		vt = tvtInteger;
		Integer = l.Integer * r.Integer;
		return;
	}
	vt = tvtReal;
	TJSSetFPUE();
	Real = l.AsReal() * r.AsReal();
}
//---------------------------------------------------------------------------
void tTJSVariant::logicalnot()
{
	bool res = !operator bool();
	ReleaseContent();
	vt = tvtInteger;
	Integer = (tjs_int)res;
}
//---------------------------------------------------------------------------
void tTJSVariant::bitnot()
{
	tjs_int64 res = ~AsInteger();
	ReleaseContent();
	vt = tvtInteger;
	Integer = res;
}
//---------------------------------------------------------------------------
void tTJSVariant::tonumber()
{
	if(vt==tvtInteger || vt==tvtReal)
		return; // nothing to do

	if(vt==tvtString)
	{
		String->ToNumber(*this);
		return;
	}

	if(vt==tvtVoid) { *this = 0; return; }

	TJSThrowVariantConvertError(*this, tvtInteger, tvtReal);
}
//---------------------------------------------------------------------------
void tTJSVariant::InternalChangeSign()
{
	tTJSVariant val;
	AsNumber(val);
	ReleaseContent();
	if(val.vt == tvtInteger)
	{
		vt = tvtInteger;
		Integer = -val.Integer;
	}
	else
	{
		vt = tvtReal;
		TJSSetFPUE();
		Real = -val.Real;
	}
}
//---------------------------------------------------------------------------
void tTJSVariant::InternalSub(const tTJSVariant &rhs)
{
	tTJSVariant l;
	AsNumber(l);
	ReleaseContent();
	tTJSVariant r;
	rhs.AsNumber(r);
	if(l.vt == tvtInteger && r.vt == tvtInteger)
	{
		vt = tvtInteger;
		Integer = l.Integer - r.Integer;
		return;
	}
	vt = tvtReal;
	TJSSetFPUE();
	Real = l.AsReal() - r.AsReal();
}
//---------------------------------------------------------------------------
void tTJSVariant::operator +=(const tTJSVariant &rhs)
{
	if(vt==tvtString || rhs.vt==tvtString)
	{
		if(vt == tvtString && rhs.vt == tvtString)
		{
			// both are string

			// independ string
			if(String && String->GetRefCount() != 0)
			{
				// sever dependency
				tTJSVariantString *orgstr = String;
				String = TJSAllocVariantString(String->operator const tjs_char*());
				orgstr->Release();
			}

			// append
			String = TJSAppendVariantString(String, rhs.String);
			return;
		}

		tTJSVariant val;
		val.vt = tvtString;
		tTJSVariantString *s1, *s2;
		s1 = AsString();
		s2 = rhs.AsString();
		val.String = TJSAllocVariantString(*s1, *s2);
		if(s1) s1->Release();
		if(s2) s2->Release();   
		*this=val;
		return;
	}


	if(vt == rhs.vt)
	{
		if(vt==tvtOctet)
		{
			tTJSVariant val;
			val.vt = tvtOctet;
			val.Octet = TJSAllocVariantOctet(Octet, rhs.Octet);
			*this=val;
			return;
		}

		if(vt==tvtInteger)
		{
			tTVInteger l=Integer;
			ReleaseContent();
			vt=tvtInteger;
			Integer=l+rhs.Integer;
			return;
		}
	}

	if(vt == tvtVoid)
	{
		if(rhs.vt == tvtInteger)
		{
			vt = tvtInteger;
			Integer = rhs.Integer;
			return;
		}
		if(rhs.vt == tvtReal)
		{
			vt = tvtReal;
			TJSSetFPUE();
			Real = rhs.Real;
			return;
		}
	}

	if(rhs.vt == tvtVoid)
	{
		if(vt == tvtInteger) return;
		if(vt == tvtReal) return;
	}

	TJSSetFPUE();
	tTVReal l=AsReal();
	ReleaseContent();
	vt=tvtReal;
	Real=l+rhs.AsReal();
}
//---------------------------------------------------------------------------
bool tTJSVariant::IsInstanceOf(const tjs_char * classname) const
{
	// not implemented
	return false;
}
//---------------------------------------------------------------------------
tjs_int tTJSVariant::QueryPersistSize() const
{
	// return the size, in bytes, of storage needed to store current state.
	// this state cannot walk across platforms.

	switch(vt)
	{
	case tvtVoid:
		return 1;

	case tvtObject:
		return sizeof(tTJSVariantClosure_S) + 1;

	case tvtString:
		if(String) return sizeof(tTJSVariantString*) + 1 + String->QueryPersistSize();
		return sizeof(tTJSVariantString*) + 1;

	case tvtInteger:
		return sizeof(tTVInteger) + 1;

	case tvtReal:
		return sizeof(tTVReal) + 1;

	case tvtOctet:
		if(Octet) return sizeof(tTJSVariantOctet*) + 1 + Octet->QueryPersistSize();
		return sizeof(tTJSVariantOctet*) + 1;
	}

	return 0;
}
//---------------------------------------------------------------------------
void tTJSVariant::Persist(tjs_uint8 * dest)
{
	// store current state to dest
	*dest = (tjs_uint8)vt;
	dest++;

	switch(vt)
	{
	case tvtVoid:
		break;

	case tvtObject:
		*(tTJSVariantClosure_S*)(dest) = Object;
		break;

	case tvtString:
		if(String)
			String->Persist(dest);
		else
			*(tjs_uint*)(dest) = 0;
		break;

	case tvtInteger:
		*(tTVInteger*)(dest) = Integer;
		break;

	case tvtReal:
		*(tTVReal*)(dest) = Real;
		break;

	case tvtOctet:
		if(Octet)
			Octet->Persist(dest);
		else
			*(tjs_uint*)(dest) = 0;
		break;
	}
}
//---------------------------------------------------------------------------


} // namespace TJS

