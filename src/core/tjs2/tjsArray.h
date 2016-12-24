//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS Array class implementation
//---------------------------------------------------------------------------

#ifndef tjsArrayH
#define tjsArrayH


#include <deque>
#include "tjsNative.h"


// note: this TJS class cannot be inherited

namespace TJS
{
//---------------------------------------------------------------------------
// tTJSStringAppender
//---------------------------------------------------------------------------
class tTJSStringAppender
{
	// fast operation for string concat

	tjs_char * Data;
	tjs_int DataLen;
	tjs_int DataCapacity; // in characters, not in bytes

public:
	tTJSStringAppender();
	~tTJSStringAppender();

	tjs_int GetLen() const { return DataLen; }
	const tjs_char * GetData() const { return Data; }
	void Append(const tjs_char *string, tjs_int len);

	void operator += (const tjs_char * string)
		{ Append(string, (tjs_int)TJS_strlen(string)); }

	void operator += (const ttstr & string)
		{ Append(string.c_str(), string.GetLen()); }
};
//---------------------------------------------------------------------------
// tTJSSaveStructuredDataCallback
//---------------------------------------------------------------------------
struct tTJSSaveStructuredDataCallback
{
	virtual void SaveStructuredData(std::vector<iTJSDispatch2 *> &stack,
                                        iTJSTextWriteStream &stream, const ttstr&indentstr) = 0;

	virtual void SaveStructuredBinary(std::vector<iTJSDispatch2 *> &stack, tTJSBinaryStream &stream ) = 0;

};
//---------------------------------------------------------------------------
// tTJSArrayClass : tTJSArray TJS class
//---------------------------------------------------------------------------
class tTJSArrayClass : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;

public:
	tTJSArrayClass();
	~tTJSArrayClass();

protected:
	tTJSNativeInstance *CreateNativeInstance();
	iTJSDispatch2 *CreateBaseTJSObject();

private:

	static tjs_uint32 ClassID;

protected:
};
//---------------------------------------------------------------------------
// tTJSArrayNI : TJS Array native C++ instance
//---------------------------------------------------------------------------
class tTJSArrayNI : public tTJSNativeInstance,
					public tTJSSaveStructuredDataCallback
{
	typedef tTJSNativeInstance inherited;
public:
	typedef std::vector<tTJSVariant>::iterator tArrayItemIterator;
	std::vector<tTJSVariant> Items;
	tTJSArrayNI();

	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **params,
		iTJSDispatch2 *tjsobj);

	void Assign(iTJSDispatch2 *dsp);

private:
	struct tDictionaryEnumCallback : public tTJSDispatch
	{
		std::vector<tTJSVariant> * Items;
		
		tjs_error TJS_INTF_METHOD
		FuncCall(tjs_uint32 flag, const tjs_char * membername,
			tjs_uint32 *hint, tTJSVariant *result, tjs_int numparams,
			tTJSVariant **param, iTJSDispatch2 *objthis);
	};
	friend struct tDictionaryEnumCallback;

public:
	void SaveStructuredData(std::vector<iTJSDispatch2 *> &stack,
		iTJSTextWriteStream &stream, const ttstr&indentstr);
	void SaveStructuredBinary(std::vector<iTJSDispatch2 *> &stack, tTJSBinaryStream &stream );
		// method from tTJSSaveStructuredDataCallback
	static void SaveStructuredDataForObject(iTJSDispatch2 *dsp,
		std::vector<iTJSDispatch2 *> &stack, iTJSTextWriteStream &stream, const ttstr&indentstr);
	static void SaveStructuredBinaryForObject(iTJSDispatch2 *dsp,
		std::vector<iTJSDispatch2 *> &stack, tTJSBinaryStream &stream );

	void AssignStructure(iTJSDispatch2 * dsp, std::vector<iTJSDispatch2 *> &stack);
//---------------------------------------------------------------------------
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class tTJSArrayObject : public tTJSCustomObject
{
	typedef tTJSCustomObject inherited;

	void CheckObjectClosureAdd(const tTJSVariant &val)
	{
		// member's object closure often points the container,
		// so we must adjust the reference counter to avoid
		// mutual reference lock.
		if(val.Type() == tvtObject)
		{
			iTJSDispatch2 *dsp = val.AsObjectClosureNoAddRef().ObjThis;
			if(dsp == (iTJSDispatch2*)this) this->Release();
		}
	}

	void CheckObjectClosureRemove(const tTJSVariant &val)
	{
		if(val.Type() == tvtObject)
		{
			iTJSDispatch2 *dsp = val.AsObjectClosureNoAddRef().ObjThis;
			if(dsp == (iTJSDispatch2*)this) this->AddRef();
		}
	}



public:
	tTJSArrayObject();
	~tTJSArrayObject();

protected:
	void Finalize(); // Finalize override

public:
	void Clear(tTJSArrayNI *ni);

	void Add(tTJSArrayNI *ni, const tTJSVariant &val);
	tjs_int Remove(tTJSArrayNI *ni, const tTJSVariant &ref, bool removeall);
	void Erase(tTJSArrayNI *ni, tjs_int num);
	void Insert(tTJSArrayNI *ni, const tTJSVariant &val, tjs_int num);
	void Insert(tTJSArrayNI *ni, tTJSVariant *const *val, tjs_int numvals, tjs_int num);

public:
	tjs_error TJS_INTF_METHOD
	FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	FuncCallByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropGetByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropSetByNum(tjs_uint32 flag, tjs_int num, const tTJSVariant *param,
		iTJSDispatch2 *objthis);

/*
	GetCount
	GetCountByNum
*/

	tjs_error TJS_INTF_METHOD
	PropSetByVS(tjs_uint32 flag, tTJSVariantString *membername,
		const tTJSVariant *param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	EnumMembers(tjs_uint32 flag, tTJSVariantClosure *callback, iTJSDispatch2 *objthis)
	{
		return TJS_E_NOTIMPL; // currently not implemented
	}

	tjs_error TJS_INTF_METHOD
	DeleteMember(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	DeleteMemberByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	Invalidate(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	InvalidateByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	IsValid(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	IsValidByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	CreateNew(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		iTJSDispatch2 **result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	CreateNewByNum(tjs_uint32 flag, tjs_int num, iTJSDispatch2 **result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis);
/*
	tjs_error
	GetSuperClass(tjs_uint32 flag, iTJSDispatch2 **result, iTJSDispatch2 *objthis);
*/
	tjs_error TJS_INTF_METHOD
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tjs_char *classname,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	IsInstanceOfByNum(tjs_uint32 flag, tjs_int num, const tjs_char *classname,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	Operation(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		tTJSVariant *result,
		const tTJSVariant *param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	OperationByNum(tjs_uint32 flag, tjs_int num, tTJSVariant *result,
		const tTJSVariant *param, iTJSDispatch2 *objthis);
/*
	tjs_error
	NativeInstanceSupport(tjs_uint32 flag, tjs_int32 classid,
		tTJSNativeInstance **pointer);
*/
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSGetArrayClassID
//---------------------------------------------------------------------------
extern tjs_int32 TJSGetArrayClassID();
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSCreateArrayObject
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(iTJSDispatch2 *, TJSCreateArrayObject, (iTJSDispatch2 **classout = NULL));
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Utility functions
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tjs_int, TJSGetArrayElementCount, (iTJSDispatch2 * dsp));
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tjs_int, TJSCopyArrayElementTo, (iTJSDispatch2 * dsp, tTJSVariant *dest, tjs_uint start, tjs_int count));
//---------------------------------------------------------------------------


} // namespace TJS

#endif
