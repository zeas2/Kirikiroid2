//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// implementation of tTJSVariant
//---------------------------------------------------------------------------
#ifndef tjsVariantH
#define tjsVariantH

//#include <stdlib.h>
//#include <stdexcept>

#ifdef TJS_SUPPORT_VCL
 #include <vcl.h>
#endif

#include "tjsInterface.h"
#include "tjsVariantString.h"
#include "tjsString.h"

namespace TJS
{
TJS_EXP_FUNC_DEF(void, TJSThrowNullAccess, ());
TJS_EXP_FUNC_DEF(void, TJSThrowDivideByZero, ());
//---------------------------------------------------------------------------
class tTJSVariantString;
class tTJSString;
//---------------------------------------------------------------------------



/*[*/
//---------------------------------------------------------------------------
// tTJSVariantOctet
//---------------------------------------------------------------------------

#pragma pack(push, 4)
struct tTJSVariantOctet_S
{
	tjs_uint Length;
	tjs_int RefCount;
	tjs_uint8 *Data;
};
#pragma pack(pop)
/*]*/

/*start-of-tTJSVariantOctet*/
class tTJSVariantOctet : protected tTJSVariantOctet_S
{
public:
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariantOctet,
		(const tjs_uint8 *data, tjs_uint length));
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariantOctet,
		(const tjs_uint8 *data1, tjs_uint len1, const tjs_uint8 *data2,
		tjs_uint len2));
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariantOctet,
		(const tTJSVariantOctet *o1, const tTJSVariantOctet *o2));
	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, ~tTJSVariantOctet, ());

	TJS_METHOD_DEF(void, AddRef, ())
	{
		RefCount ++;
	}

	TJS_METHOD_DEF(void, Release, ());

	TJS_CONST_METHOD_DEF(tjs_uint, GetLength, ())
	{
		return Length;
	}

	TJS_CONST_METHOD_DEF(const tjs_uint8 *,  GetData, ())
	{
		return Data;
	}

	tjs_int QueryPersistSize() { return sizeof(tjs_uint) + Length; }
	void Persist(tjs_uint8 * dest)
	{
		*(tjs_uint*)dest = Length;
		if(Data) TJS_octetcpy(dest + sizeof(tjs_uint), Data, Length);
	}
};
/*end-of-tTJSVariantOctet*/
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tTJSVariantOctet *, TJSAllocVariantOctet, (const tjs_uint8 *data, tjs_uint length));
TJS_EXP_FUNC_DEF(tTJSVariantOctet *, TJSAllocVariantOctet, (const tjs_uint8 *data1, tjs_uint len1,
	const tjs_uint8 *data2, tjs_uint len2));
TJS_EXP_FUNC_DEF(tTJSVariantOctet *, TJSAllocVariantOctet, (const tTJSVariantOctet *o1, const
	tTJSVariantOctet *o2));
TJS_EXP_FUNC_DEF(tTJSVariantOctet *, TJSAllocVariantOctet, (const tjs_uint8 **src));
TJS_EXP_FUNC_DEF(void, TJSDeallocVariantOctet, (tTJSVariantOctet *o));
TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSOctetToListString, (const tTJSVariantOctet *oct));
//---------------------------------------------------------------------------



/*[*/
//---------------------------------------------------------------------------
// tTJSVariant_S
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#pragma option push -b
#endif
enum tTJSVariantType
{
	tvtVoid,  // empty
	tvtObject,
	tvtString,
	tvtOctet,  // octet binary data
	tvtInteger,
	tvtReal
};
#ifdef __BORLANDC__
#pragma option pop
#endif
/*]*/
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#pragma option push -b -a4
#endif

/*[*/
#pragma pack(push, 4)
class iTJSDispatch2;
struct tTJSVariantClosure_S
{
	iTJSDispatch2 *Object;
	iTJSDispatch2 *ObjThis;
};
class tTJSVariantClosure;

class tTJSVariantString;
class tTJSVariantOctet;
struct tTJSVariant_S
{
	//---- data members -----------------------------------------------------

	#define tTJSVariant_BITCOPY(a,b) \
	{\
		*(tTJSVariant_S*)&(a) = *(tTJSVariant_S*)&(b); \
	}

	union
	{
		tTJSVariantClosure_S Object;
		tTVInteger Integer;
		tTVReal Real;
		tTJSVariantString *String;
		tTJSVariantOctet *Octet;
	};
	tTJSVariantType vt;
};
#pragma pack(pop)
/*]*/

#ifdef __BORLANDC__
#pragma option pop
#endif
//---------------------------------------------------------------------------
// tTJSVariantClosure
//---------------------------------------------------------------------------
extern tTJSVariantClosure_S TJSNullVariantClosure;

/*[*/
//---------------------------------------------------------------------------
// tTJSVariantClosure
//---------------------------------------------------------------------------
/*]*/
#if 0
// for plug-in
/*[*/
void TJSThrowNullAccess();
/*]*/
#endif
/*[*/

class tTJSVariantClosure : public tTJSVariantClosure_S
{
	// tTJSVariantClosure does not provide any function of object lifetime
	// namagement. ( AddRef and Release are provided but tTJSVariantClosure
	// has no responsibility for them )

public:
	tTJSVariantClosure() {;} // note that default constructor does nothing 

	tTJSVariantClosure(iTJSDispatch2 *obj, iTJSDispatch2 *objthis = NULL)
	{ Object = obj, ObjThis = objthis; }

	iTJSDispatch2 * SelectObjectNoAddRef()
		{ return ObjThis ? ObjThis : Object; }

public:

	bool operator == (const tTJSVariantClosure &rhs)
	{
		return Object == rhs.Object && ObjThis == rhs.ObjThis;
	}

	bool operator != (const tTJSVariantClosure &rhs)
	{
		return ! this->operator ==(rhs);
	}


	void AddRef()
	{
		if(Object) Object->AddRef();
		if(ObjThis) ObjThis->AddRef();
	}

	void Release()
	{
		if(Object) Object->Release();
		if(ObjThis) ObjThis->Release();
	}


	tjs_error
	FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->FuncCall(flag, membername, hint, result, numparams, param,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	FuncCallByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->FuncCallByNum(flag, num, result, numparams, param,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->PropGet(flag, membername, hint, result,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	PropGetByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->PropGetByNum(flag, num, result,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->PropSet(flag, membername, hint, param,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	PropSetByNum(tjs_uint32 flag, tjs_int num, const tTJSVariant *param,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->PropSetByNum(flag, num, param,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	GetCount(tjs_int *result, const tjs_char *membername, tjs_uint32 *hint,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->GetCount(result, membername, hint,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	GetCountByNum(tjs_int *result, tjs_int num, iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->GetCountByNum(result, num,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	PropSetByVS(tjs_uint32 flag, tTJSVariantString *membername,
		const tTJSVariant *param, iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->PropSetByVS(flag, membername, param,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	EnumMembers(tjs_uint32 flag, tTJSVariantClosure *callback,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->EnumMembers(flag, callback,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	DeleteMember(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->DeleteMember(flag, membername, hint,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	DeleteMemberByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->DeleteMemberByNum(flag, num,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	Invalidate(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->Invalidate(flag, membername, hint,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	InvalidateByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->InvalidateByNum(flag, num,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	IsValid(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->IsValid(flag, membername, hint,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	IsValidByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->IsValidByNum(flag, num,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	CreateNew(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		iTJSDispatch2 **result,
		tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->CreateNew(flag, membername, hint, result, numparams,
			param, ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	CreateNewByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 **result,
		tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->CreateNewByNum(flag, num, result, numparams, param,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

/*
	tjs_error
	Reserved1() { }
*/

	tjs_error
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tjs_char *classname, iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->IsInstanceOf(flag, membername, hint, classname,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	IsInstanceOf(tjs_uint32 flag, tjs_int num, tjs_char *classname,
		iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->IsInstanceOfByNum(flag, num, classname,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	Operation(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		tTJSVariant *result, const tTJSVariant *param,	iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->Operation(flag, membername, hint, result, param,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

	tjs_error
	OperationByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		const tTJSVariant *param,	iTJSDispatch2 *objthis) const
	{
		if(!Object) TJSThrowNullAccess();
		return Object->OperationByNum(flag, num, result, param,
			ObjThis?ObjThis:(objthis?objthis:Object));
	}

/*
	tjs_error
	Reserved2() { }
*/

/*
	tjs_error
	Reserved3() { }
*/

};




/*]*/
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSObjectToString, (const tTJSVariantClosure &dsp));
TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSIntegerToString, (tjs_int64 i));
TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSRealToString, (tjs_real r));
TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSRealToHexString, (tjs_real r));
TJS_EXP_FUNC_DEF(tTVInteger, TJSStringToInteger, (const tjs_char *str));
TJS_EXP_FUNC_DEF(tTVReal, TJSStringToReal, (const tjs_char *str));
//---------------------------------------------------------------------------
extern void TJSThrowVariantConvertError(const tTJSVariant & from, tTJSVariantType to);
extern void TJSThrowVariantConvertError(const tTJSVariant & from, tTJSVariantType to1,
	tTJSVariantType to2);
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSVariant
//---------------------------------------------------------------------------

#ifdef TJS_SUPPORT_VCL //  suppresses warnings about inline function
	#pragma warn -8027
	#pragma warn -8026
#endif


/*start-of-tTJSVariant*/
class tTJSVariant : protected tTJSVariant_S
{

	//---- object management ------------------------------------------------
private:

	void AddRefObject()
	{
		if(Object.Object) Object.Object->AddRef();
		if(Object.ObjThis) Object.ObjThis->AddRef();
		// does not addref the string or octet
	}

	void ReleaseObject()
	{
		iTJSDispatch2 * object = Object.Object;
		iTJSDispatch2 * objthis = Object.ObjThis;
		if(object) Object.Object = NULL, object->Release();
		if(objthis) Object.ObjThis = NULL, objthis->Release();
		// does not release the string nor octet
	}

	void AddRefContent()
	{
		if(vt==tvtObject)
		{
			if(Object.Object) Object.Object->AddRef();
			if(Object.ObjThis) Object.ObjThis->AddRef();
		}
		else
		{
			if(vt==tvtString) { if(String) String->AddRef(); }
			else if(vt==tvtOctet) { if(Octet) Octet->AddRef(); }
		}
	}

	void ReleaseContent()
	{
		if(vt==tvtObject)
		{
			ReleaseObject();
		}
		else
		{
			if(vt==tvtString) { if(String) String->Release(); }
			else if(vt==tvtOctet) { if(Octet) Octet->Release(); }
		}
	}

public:

	TJS_METHOD_DEF(void, ChangeClosureObjThis, (iTJSDispatch2 *objthis))
	{
		if(objthis) objthis->AddRef();
		if(vt!=tvtObject) TJSThrowVariantConvertError(*this, tvtObject);
		if(Object.ObjThis)
		{
			iTJSDispatch2 * objthis = Object.ObjThis;
			Object.ObjThis = NULL;
			objthis->Release();
		}
		Object.ObjThis = objthis;
	}

	//---- constructor ------------------------------------------------------
public:

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, ())
	{
		vt=tvtVoid;
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (const tTJSVariant &ref)); // from tTJSVariant

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (iTJSDispatch2 *ref)) // from Object
	{
		if(ref) ref->AddRef();
		vt=tvtObject;
		Object.Object = ref;
		Object.ObjThis = NULL;
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (iTJSDispatch2 *obj, iTJSDispatch2 *objthis)) // from closure
	{
		if(obj) obj->AddRef();
		if(objthis) objthis->AddRef();
		vt=tvtObject;
		Object.Object = obj;
		Object.ObjThis = objthis;
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (const tjs_char *ref)) //  from String
	{
		vt=tvtString;
		if(ref)
		{
			String=TJSAllocVariantString(ref);
		}
		else
		{
			String=NULL;
		}
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (const tTJSString & ref)) // from tTJSString
	{
		vt = tvtString;
		String = ref.AsVariantStringNoAddRef();
		if(String) String->AddRef();
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (const tjs_nchar *ref)) //  from NarrowString
	{
		vt=tvtString;
		if(ref)
		{
			String=TJSAllocVariantString(ref);
		}
		else
		{
			String=NULL;
		}
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (const tjs_uint8 *ref, tjs_uint len)) // from octet
	{
		vt=tvtOctet;
		if(ref)
		{
			Octet = TJSAllocVariantOctet(ref, len);
		}
		else
		{
			Octet = NULL;
		}
	}

#ifdef TJS_SUPPORT_VCL
	tTJSVariant(const WideString ref) // from WideString
	{
		vt=tvtString;
		if(ref.IsEmpty())
			String=NULL;
		else
			String=TJSAllocVariantString(ref);
	}
#endif

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (bool ref))
	{
		vt=tvtInteger;
		Integer=(tjs_int64)(tjs_int)ref;
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (tjs_int32 ref))
	{
		vt=tvtInteger;
		Integer=(tjs_int64)ref;
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (tjs_int64 ref))  // from Integer64
	{
		vt=tvtInteger;
		Integer=ref;
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (tjs_real ref))  // from double
	{
		vt=tvtReal;
		TJSSetFPUE();
		Real=ref;
	}

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, tTJSVariant, (const tjs_uint8 ** src)); // from persistent storage

	//---- destructor -------------------------------------------------------

	TJS_METHOD_DEF(TJS_METHOD_RET_EMPTY, ~tTJSVariant, ());

	//---- type -------------------------------------------------------------

	TJS_METHOD_DEF(tTJSVariantType, Type, ()) { return vt; } /* for plug-in compatibility */
	TJS_CONST_METHOD_DEF(tTJSVariantType, Type, ()) { return vt; }

	//---- compare ----------------------------------------------------------

	TJS_CONST_METHOD_DEF(bool, NormalCompare, (const tTJSVariant &val2));
	TJS_CONST_METHOD_DEF(bool, DiscernCompare, (const tTJSVariant &val2));
	TJS_CONST_METHOD_DEF(bool, DiscernCompareStrictReal, (const tTJSVariant &val2));
	TJS_CONST_METHOD_DEF(bool, GreaterThan, (const tTJSVariant &val2));
	TJS_CONST_METHOD_DEF(bool, LittlerThan, (const tTJSVariant &val2));

	TJS_CONST_METHOD_DEF(bool, IsInstanceOf, (const tjs_char * classname));

	//---- clear ------------------------------------------------------------

	TJS_METHOD_DEF(void, Clear, ());

	//---- type conversion --------------------------------------------------

	TJS_CONST_METHOD_DEF(iTJSDispatch2 *, AsObject, ())
	{
		if(vt==tvtObject)
		{
			if(Object.Object) Object.Object->AddRef();
			return Object.Object;
		}

		TJSThrowVariantConvertError(*this, tvtObject);

		return NULL;
	}

	TJS_CONST_METHOD_DEF(iTJSDispatch2 *, AsObjectNoAddRef, ())
	{
		if(vt==tvtObject)
			return Object.Object;
		TJSThrowVariantConvertError(*this, tvtObject);
		return NULL;
	}

	TJS_CONST_METHOD_DEF(iTJSDispatch2 *, AsObjectThis, ())
	{
		if(vt==tvtObject)
		{
			if(Object.ObjThis) Object.ObjThis->AddRef();
			return Object.ObjThis;
		}
		TJSThrowVariantConvertError(*this, tvtObject);
		return NULL;
	}

	TJS_CONST_METHOD_DEF(iTJSDispatch2 *, AsObjectThisNoAddRef, ())
	{
		if(vt==tvtObject)
		{
			return Object.ObjThis;
		}
		TJSThrowVariantConvertError(*this, tvtObject);
		return NULL;
	}

	TJS_METHOD_DEF(tTJSVariantClosure &, AsObjectClosure, ())
	{
		if(vt==tvtObject)
		{
			AddRefObject();
			return *(tTJSVariantClosure*)&Object;
		}
		TJSThrowVariantConvertError(*this, tvtObject);
		return *(tTJSVariantClosure*)&TJSNullVariantClosure;
	}


	TJS_CONST_METHOD_DEF(tTJSVariantClosure &, AsObjectClosureNoAddRef, ())
	{
		if(vt==tvtObject)
		{
			return *(tTJSVariantClosure*)&Object;
		}
		TJSThrowVariantConvertError(*this, tvtObject);
		return *(tTJSVariantClosure*)&TJSNullVariantClosure;
	}

	TJS_METHOD_DEF(void, ToObject, ())
	{
		switch(vt)
		{
		case tvtObject:  break;
		case tvtVoid:
		case tvtString:
		case tvtInteger:
		case tvtReal:
		case tvtOctet:    TJSThrowVariantConvertError(*this, tvtObject);
		}

	}

#ifdef TJS_SUPPORT_VCL
	void AttachTo(WideString &ws)
	{
		tTJSVariantString *str = AsString();
		ws.Attach(SysAllocString(*str));
		if(str) str->Release();
	}
#endif

	TJS_METHOD_DEF(TJS_METHOD_RET(iTJSDispatch2 *), operator iTJSDispatch2 *, ())
	{
		return AsObject();
	}

	TJS_CONST_METHOD_DEF(tTJSVariantString *, AsString, ())
	{
		switch(vt)
		{
		case tvtVoid:    return NULL;
		case tvtObject:  return TJSObjectToString(*(tTJSVariantClosure*)&Object);
		case tvtString:  { if(String) { String->AddRef(); } return String; }
		case tvtInteger: return TJSIntegerToString(Integer);
		case tvtReal:    return TJSRealToString(Real);
		case tvtOctet:   TJSThrowVariantConvertError(*this, tvtString);
		}
		return NULL;
	}

	TJS_CONST_METHOD_DEF(tTJSVariantString *, AsStringNoAddRef, ())
	{
		switch(vt)
		{
		case tvtVoid:    return NULL;
		case tvtString:  return String;
		case tvtObject:
		case tvtInteger:
		case tvtReal:
		case tvtOctet:   TJSThrowVariantConvertError(*this, tvtString);
		}
		return NULL;
	}

	TJS_METHOD_DEF(void, ToString, ());

	TJS_CONST_METHOD_DEF(const tjs_char *, GetString, ())
	{
		// returns String
		if(vt!=tvtString) TJSThrowVariantConvertError(*this, tvtString);
		return *String;
	}

	TJS_METHOD_DEF(tjs_uint32 *, GetHint, ())
	{
		// returns String Hint
		if(vt!=tvtString) TJSThrowVariantConvertError(*this, tvtString);
		if(!String) return NULL;
		return String->GetHint();
	}

	TJS_CONST_METHOD_DEF(tTJSVariantOctet *, AsOctet, ())
	{
		switch(vt)
		{
		case tvtVoid:    return NULL;
		case tvtOctet:   { if(Octet) Octet->AddRef(); } return Octet;
		case tvtString:
		case tvtInteger:
		case tvtReal:
		case tvtObject:  TJSThrowVariantConvertError(*this, tvtOctet);
		}
		return NULL;
	}

	TJS_CONST_METHOD_DEF(tTJSVariantOctet *, AsOctetNoAddRef, ())
	{
		switch(vt)
		{
		case tvtVoid:    return NULL;
		case tvtOctet:   return Octet;
		case tvtString:
		case tvtInteger:
		case tvtReal:
		case tvtObject:  TJSThrowVariantConvertError(*this, tvtOctet);
		}
		return NULL;
	}

	TJS_METHOD_DEF(void, ToOctet, ());


#ifdef TJS_SUPPORT_VCL
	operator AnsiString () const
	{
		if(vt==tvtString)
		{
			return AnsiString(*String);
		}
		else
		{
			tTJSVariantString *s=AsString();
			if(s)
			{
				AnsiString r=AnsiString(*s);
				s->Release();
				return r;
			}
			else
			{
				return "";
			}
		}
	}
#endif

	TJS_CONST_METHOD_DEF(tTVInteger, AsInteger, ())
	{
		switch(vt)
		{
		case tvtVoid:    return 0;
		case tvtObject:  TJSThrowVariantConvertError(*this, tvtInteger);
		case tvtString:  return String->ToInteger();
		case tvtInteger: return Integer;
		case tvtReal:    TJSSetFPUE(); return (tTVInteger)Real;
		case tvtOctet:   TJSThrowVariantConvertError(*this, tvtInteger);
		}
		return 0;
	}

	TJS_CONST_METHOD_DEF(void, AsNumber, (tTJSVariant &targ))
	{
		switch(vt)
		{
		case tvtVoid:    targ = (tjs_int)0; return;
		case tvtObject:  TJSThrowVariantConvertError(*this, tvtInteger, tvtReal);
		case tvtString:  String->ToNumber(targ); return;
		case tvtInteger: targ = Integer; return;
		case tvtReal:    TJSSetFPUE(); targ = Real; return;
		case tvtOctet:   TJSThrowVariantConvertError(*this, tvtInteger, tvtReal);
		}
		return;
	}

	TJS_METHOD_DEF(void, ToInteger, ());

	TJS_CONST_METHOD_DEF(TJS_METHOD_RET(tTVInteger), operator tTVInteger, ())
	{
		return AsInteger();
	}

	TJS_CONST_METHOD_DEF(TJS_METHOD_RET(bool), operator bool, ())
	{
		switch(vt)
		{
		case tvtVoid:		return false;
		//case tvtObject:		return (bool)Object.Object;
		case tvtObject:		return Object.Object != NULL;
		//case tvtString:		return (bool)AsInteger();
		case tvtString:		return AsInteger() != 0;
		case tvtOctet:		return 0!=Octet;
		case tvtInteger:	return 0!=Integer;
		case tvtReal:		TJSSetFPUE(); return 0!=Real;
		}
		return false;
	}

	TJS_CONST_METHOD_DEF(TJS_METHOD_RET(tjs_int), operator tjs_int, ())
	{
		return (tjs_int)AsInteger();
	}

	TJS_CONST_METHOD_DEF(tTVReal, AsReal, ())
	{
		TJSSetFPUE();

		switch(vt)
		{
		case tvtVoid:    return 0;
		case tvtObject:  TJSThrowVariantConvertError(*this, tvtReal);
		case tvtString:  return String->ToReal();
		case tvtInteger: return (tTVReal)Integer;
		case tvtReal:    return Real;
		case tvtOctet:   TJSThrowVariantConvertError(*this, tvtReal);
		}
		return 0.0f;
	}

	TJS_METHOD_DEF(void, ToReal, ());

	TJS_CONST_METHOD_DEF(TJS_METHOD_RET(tTVReal), operator tTVReal, ())
	{
		return AsReal();
	}

	//---- substitution -----------------------------------------------------

	TJS_METHOD_DEF(tTJSVariant &, operator =, (const tTJSVariant &ref))
	{
		// from tTJSVariant
		CopyRef(ref);
		return *this;
	}
	TJS_METHOD_DEF(void, CopyRef, (const tTJSVariant & ref)); // from reference to tTJSVariant
	TJS_METHOD_DEF(tTJSVariant &, operator =, (iTJSDispatch2 *ref)); // from Object
	TJS_METHOD_DEF(tTJSVariant &, SetObject, (iTJSDispatch2 *ref)) { return this->operator =(ref); }
	TJS_METHOD_DEF(tTJSVariant &, SetObject, (iTJSDispatch2 *object, iTJSDispatch2 *objthis));
	TJS_METHOD_DEF(tTJSVariant &, operator =, (tTJSVariantClosure ref)); // from Object Closure
#ifdef TJS_SUPPORT_VCL
	tTJSVariant & operator =(WideString s); // from WideString
#endif
	TJS_METHOD_DEF(tTJSVariant &, operator =, (tTJSVariantString *ref)); // from tTJSVariantString
	TJS_METHOD_DEF(tTJSVariant &, operator =, (tTJSVariantOctet *ref)); // from tTJSVariantOctet
	TJS_METHOD_DEF(tTJSVariant &, operator =, (const tTJSString & ref)); // from tTJSString
	TJS_METHOD_DEF(tTJSVariant &, operator =, (const tjs_char *ref)); //  from String
	TJS_METHOD_DEF(tTJSVariant &, operator =, (const tjs_nchar *ref)); // from narrow string
	TJS_METHOD_DEF(tTJSVariant &, operator =, (bool ref));
	TJS_METHOD_DEF(tTJSVariant &, operator =, (tjs_int32 ref));
	TJS_METHOD_DEF(tTJSVariant &, operator =, (const tTVInteger ref)); // from Integer64
	TJS_METHOD_DEF(tTJSVariant &, operator =, (tjs_real ref)); // from double

	//---- operators --------------------------------------------------------

	TJS_CONST_METHOD_DEF(tTJSVariant, operator ||, (const tTJSVariant & rhs))
	{
		return operator bool()||rhs.operator bool();
	}

	TJS_METHOD_DEF(void, logicalorequal, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator &&, (const tTJSVariant & rhs))
	{
		return operator bool()&&rhs.operator bool();
	}

	TJS_METHOD_DEF(void, logicalandequal, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator |, (const tTJSVariant & rhs))
	{
		return (tTVInteger)(AsInteger()|rhs.AsInteger());
	}

	TJS_METHOD_DEF(void, operator |=, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator ^, (const tTJSVariant & rhs))
	{
		return (tTVInteger)(AsInteger()^rhs.AsInteger());
	}

	TJS_METHOD_DEF(void, increment, ());

	TJS_METHOD_DEF(void, decrement, ());

	TJS_METHOD_DEF(void, operator ^=, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator &, (const tTJSVariant & rhs))
	{
		return (tTVInteger)(AsInteger()&rhs.AsInteger());
	}

	TJS_METHOD_DEF(void, operator &=, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator !=, (const tTJSVariant & rhs))
	{
		return !NormalCompare(rhs);
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator ==, (const tTJSVariant & rhs))
	{
		return NormalCompare(rhs);
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator <, (const tTJSVariant & rhs))
	{
		return GreaterThan(rhs);
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator >, (const tTJSVariant & rhs))
	{
		return LittlerThan(rhs);
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator <=, (const tTJSVariant & rhs))
	{
		return !LittlerThan(rhs);
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator >=, (const tTJSVariant & rhs))
	{
		return !GreaterThan(rhs);
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator >>, (const tTJSVariant & rhs))
	{
		return (tTVInteger)(AsInteger()>>(tjs_int)rhs.AsInteger());
	}

	TJS_METHOD_DEF(void, operator >>=, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, rbitshift, (tjs_int count))
	{
		return (tTVInteger)((tjs_uint64)AsInteger()>> count);
	}

	TJS_METHOD_DEF(void, rbitshiftequal, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator <<, (const tTJSVariant & rhs))
	{
		return (tTVInteger)(AsInteger()<<(tjs_int)rhs.AsInteger());
	}

	TJS_METHOD_DEF(void, operator <<=, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator %, (const tTJSVariant & rhs))
	{
		tTVInteger r = rhs.AsInteger();
		if(r == 0) TJSThrowDivideByZero();
		return (tTVInteger)(AsInteger()%r);
	}

	TJS_METHOD_DEF(void, operator %=, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator /, (const tTJSVariant & rhs))
	{
		TJSSetFPUE();
		tTVReal r = rhs.AsReal();
		return (AsReal()/r);
	}

	TJS_METHOD_DEF(void, operator /=, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, idiv, (const tTJSVariant & rhs))
	{
		tTVInteger r = rhs.AsInteger();
		if(r == 0) TJSThrowDivideByZero();
		return (tTVInteger)(AsInteger() / r);
	}

	TJS_METHOD_DEF(void, idivequal, (const tTJSVariant &rhs));

	TJS_CONST_METHOD_DEF(tTJSVariant, operator *, (const tTJSVariant & rhs))
	{
		tTJSVariant l(*this);
		l *= rhs;
		return l;
	}

private:
	void InternalMul(const tTJSVariant &rhs);
public:
	TJS_METHOD_DEF(void, operator *=, (const tTJSVariant &rhs))
	{
		if(vt == tvtInteger && rhs.vt == tvtInteger)
		{
			Integer *= rhs.Integer;
			return;
		}
		InternalMul(rhs);
	}

	TJS_METHOD_DEF(void, logicalnot, ());

	TJS_CONST_METHOD_DEF(tTJSVariant, operator !, ())
	{
		return (tjs_int)!operator bool();
	}

	TJS_METHOD_DEF(void, bitnot, ());

	TJS_CONST_METHOD_DEF(tTJSVariant, operator ~, ())
	{
		return (tjs_int64)~AsInteger();
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator -, (const tTJSVariant & rhs))
	{
		tTJSVariant l(*this);
		l -= rhs;
		return l;
	}

	TJS_METHOD_DEF(void, tonumber, ());

	TJS_CONST_METHOD_DEF(tTJSVariant, operator +, ())
	{
		if(vt==tvtInteger || vt==tvtReal) return *this;

		if(vt==tvtString)
		{
			tTJSVariant val;
			String->ToNumber(val);
			return val;
		}

		if(vt==tvtVoid) return 0;

		TJSThrowVariantConvertError(*this, tvtInteger, tvtReal);
		return tTJSVariant();
	}

private:
	void InternalChangeSign();
public:
	TJS_METHOD_DEF(void, changesign, ())
	{
		if(vt==tvtInteger)
		{
			Integer = -Integer;
			return;
		}
		InternalChangeSign();
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator -, ())
	{
		tTJSVariant v(*this);
		v.changesign();
		return v;
	}

private:
	void InternalSub(const tTJSVariant &rhs);
public:
	TJS_METHOD_DEF(void, operator -=, (const tTJSVariant &rhs))
	{
		if(vt == tvtInteger && rhs.vt == tvtInteger)
		{
			Integer -= rhs.Integer;
			return;
		}
        InternalSub(rhs);
	}

	TJS_CONST_METHOD_DEF(tTJSVariant, operator +, (const tTJSVariant & rhs))
	{
		if(vt==tvtString || rhs.vt==tvtString)
		{
			// combines as string
			tTJSVariant val;
			val.vt = tvtString;
			tTJSVariantString *s1, *s2;
			s1 = AsString();
			s2 = rhs.AsString();
			val.String = TJSAllocVariantString(*s1, *s2);
			if(s1) s1->Release();
			if(s2) s2->Release();
			return val;
		}

		if(vt == rhs.vt)
		{
			if(vt==tvtOctet)
			{
				// combine as octet
				tTJSVariant val;
				val.vt = tvtOctet;
				val.Octet = TJSAllocVariantOctet(Octet, rhs.Octet);
				return val;
			}

			if(vt==tvtInteger)
			{
				return Integer+rhs.Integer;
			}
		}

		if(vt == tvtVoid)
		{
			if(rhs.vt == tvtInteger || rhs.vt == tvtReal)
				return rhs;
		}

		if(rhs.vt == tvtVoid)
		{
			if(vt == tvtInteger || vt == tvtReal) return *this;
		}


		TJSSetFPUE();
		return AsReal()+rhs.AsReal();
	}


	TJS_METHOD_DEF(void, operator +=, (const tTJSVariant &rhs));

	//------ allocator/deallocater ------------------------------------------
	TJS_STATIC_METHOD_DEF(void *, operator new, (size_t size)) { return new char[size]; }
	TJS_STATIC_METHOD_DEF(void, operator delete, (void * p)) { delete [] ((char*)p); }

	TJS_STATIC_METHOD_DEF(void *, operator new [], (size_t size)) { return new char[size]; }
	TJS_STATIC_METHOD_DEF(void, operator delete [], (void *p)) { delete [] ((char*)p); }

	TJS_STATIC_METHOD_DEF(void *, operator new, (size_t size, void *buf)) { return buf; }

	//------ persist --------------------------------------------------------

	tjs_int QueryPersistSize() const;
	void Persist(tjs_uint8 * dest);
};
/*end-of-tTJSVariant*/
//---------------------------------------------------------------------------
#ifdef TJS_SUPPORT_VCL
	#pragma warn .8027
	#pragma warn .8026
#endif

//---------------------------------------------------------------------------

} // namespace TJS
#endif



