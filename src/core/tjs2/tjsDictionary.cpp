//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Dictionary class implementation
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include "tjsDictionary.h"
#include "tjsArray.h"
#include "tjsBinarySerializer.h"
#include "tjsDebug.h"

namespace TJS
{
//---------------------------------------------------------------------------
static tjs_int32 ClassID_Dictionary;
//---------------------------------------------------------------------------
// tTJSDictionaryClass : tTJSDictionary class
//---------------------------------------------------------------------------
tjs_uint32 tTJSDictionaryClass::ClassID = (tjs_uint32)-1;
tTJSDictionaryClass::tTJSDictionaryClass() :
	tTJSNativeClass(TJS_W("Dictionary"))
{
	// TJS class constructor

	TJS_BEGIN_NATIVE_MEMBERS(/*TJS class name*/Dictionary)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/* var. name */_this,
	/* var. type */tTJSDictionaryNI, /* TJS class name */ Dictionary)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_CONSTRUCTOR_DECL(/*TJS class name*/Dictionary)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/load)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSDictionaryNI);
	if(!ni->IsValid()) return TJS_E_INVALIDOBJECT;

	// TODO: implement Dictionary.load()
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/load)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/loadStruct)
{
	if(numparams<1) return TJS_E_BADPARAMCOUNT;

	bool dicfree = true;
	tTJSDictionaryObject* dic = NULL;
	if( objthis ) {
		tTJSDictionaryNI* ni;
		tjs_error hr = objthis->NativeInstanceSupport(TJS_NIS_GETINSTANCE, TJS_NATIVE_CLASSID_NAME, (iTJSNativeInstance**)&ni);
		if( TJS_SUCCEEDED(hr) ) {
			if(!ni->IsValid()) return TJS_E_INVALIDOBJECT;
			ni->Clear();
			dic = (tTJSDictionaryObject*)objthis;
			dicfree = false;
		}
	}

	ttstr name(*param[0]);
	ttstr mode;
	if(numparams >= 2 && param[1]->Type() != tvtVoid) mode =*param[1];

	tTJSBinaryStream* stream = TJSCreateBinaryStreamForRead(name, mode);
	if( !stream ) return TJS_E_INVALIDPARAM;

	bool isbin = false;
	try {
		tjs_uint64 streamlen = stream->GetSize();
		if( streamlen >= tTJSBinarySerializer::HEADER_LENGTH ) {
			tjs_uint8 header[tTJSBinarySerializer::HEADER_LENGTH];
			stream->Read( header, tTJSBinarySerializer::HEADER_LENGTH );
			if( tTJSBinarySerializer::IsBinary( header ) ) {
				if( !dic ) dic = (tTJSDictionaryObject*)TJSCreateDictionaryObject();
				tTJSBinarySerializer binload(dic);
				tTJSVariant* var = binload.Read( stream );
				if( var ) {
					if( result ) *result = *var;
					delete var;
					isbin = true;
				}
				if( dicfree ) {
					if( dic ) dic->Release();
					dic = NULL;
				}
			}
		}
	} catch(...) {
		delete stream;
		if( dicfree ) {
			if( dic ) dic->Release();
			dic = NULL;
		}
		throw;
	}
	delete stream;
	if( isbin ) return TJS_S_OK;
	return TJS_E_INVALIDPARAM;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/loadStruct)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func.name*/save)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSDictionaryNI);
	if(!ni->IsValid()) return TJS_E_INVALIDOBJECT;

	// TODO: implement Dictionary.save();
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func.name*/save)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func.name*/saveStruct)
{
	// Structured output for flie;
	// the content can be interpret as an expression to re-construct the object.

	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSDictionaryNI);
	if(!ni->IsValid()) return TJS_E_INVALIDOBJECT;

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr name(*param[0]);
	ttstr mode;
	if(numparams >= 2 && param[1]->Type() != tvtVoid) mode = *param[1];

	if( TJS_strchr(mode.c_str(), TJS_W('b')) != NULL ) {
		tTJSBinaryStream* stream = TJSCreateBinaryStreamForWrite(name, mode);
		try {
			stream->Write( tTJSBinarySerializer::HEADER, tTJSBinarySerializer::HEADER_LENGTH );
			std::vector<iTJSDispatch2 *> stack;
			stack.push_back(objthis);
			ni->SaveStructuredBinary(stack, *stream);
		} catch(...) {
			delete stream;
			throw;
		}
		delete stream;
	} else {
		iTJSTextWriteStream * stream = TJSCreateTextStreamForWrite(name, mode);
		try
		{
			std::vector<iTJSDispatch2 *> stack;
			stack.push_back(objthis);
			ni->SaveStructuredData(stack, *stream, TJS_W(""));
		}
		catch(...)
		{
			stream->Destruct();
			throw;
		}
		stream->Destruct();
	}

	if(result) *result = tTJSVariant(objthis, objthis);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func.name*/saveStruct)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func.name*/assign)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSDictionaryNI);
	if(!ni->IsValid()) return TJS_E_INVALIDOBJECT;

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	bool clear = true;
	if(numparams >= 2 && param[1]->Type() != tvtVoid)
		clear = 0!=(tjs_int)*param[1];

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.ObjThis)
		ni->Assign(clo.ObjThis, clear);
	else if(clo.Object)
		ni->Assign(clo.Object, clear);
	else TJS_eTJSError(TJSNullAccess);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func.name*/assign)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/* func.name */assignStruct)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSDictionaryNI);
	if(!ni->IsValid()) return TJS_E_INVALIDOBJECT;

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	std::vector<iTJSDispatch2 *> stack;

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.ObjThis)
		ni->AssignStructure(clo.ObjThis, stack);
	else if(clo.Object)
		ni->AssignStructure(clo.Object, stack);
	else TJS_eTJSError(TJSNullAccess);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/* func.name */assignStruct)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func.name*/clear)
{
	TJS_GET_NATIVE_INSTANCE(/* var. name */ni, /* var. type */tTJSDictionaryNI);
	if(!ni->IsValid()) return TJS_E_INVALIDOBJECT;

	ni->Clear();

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func.name*/clear)
//----------------------------------------------------------------------

	ClassID_Dictionary = TJS_NCM_CLASSID;
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
tTJSDictionaryClass::~tTJSDictionaryClass()
{
}
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSDictionaryClass::CreateNativeInstance()
{
	return new tTJSDictionaryNI();
}
//---------------------------------------------------------------------------
iTJSDispatch2 *tTJSDictionaryClass::CreateBaseTJSObject()
{
	return new tTJSDictionaryObject();
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSDictionaryClass::CreateNew(tjs_uint32 flag, const tjs_char * membername,
	tjs_uint32 *hint,
	iTJSDispatch2 **result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	// CreateNew
	if( numparams < 1 || param[0]->Type() != tvtInteger || (tjs_int)(*param[0]) < 8 ) {
		return inherited::CreateNew( flag, membername, hint, result, numparams, param, objthis );
	}
	tjs_int v = (tjs_int)(*param[0]);
	tjs_int r;
	if(v & 0xffff0000) r = 16, v >>= 16; else r = 0;
	if(v & 0xff00) r += 8, v >>= 8;
	if(v & 0xf0) r += 4, v >>= 4;
	v<<=1;
	tjs_int hashbits = r + ((0xffffaa50 >> v) &0x03) + 2;
	iTJSDispatch2 *dsp = new tTJSDictionaryObject(hashbits);

	// same as tTJSNativeClass
	tjs_error hr;
	try 
	{
		// set object type for debugging
		if(TJSObjectHashMapEnabled())
			TJSObjectHashSetType(dsp, TJS_W("instance of class ") + ClassName);

		// instance initialization
		hr = FuncCall(0, NULL, NULL, NULL, 0, NULL, dsp); // add member to dsp

		if(TJS_FAILED(hr)) return hr;

		hr = FuncCall(0, ClassName.c_str(), ClassName.GetHint(), NULL, numparams, param, dsp);
			// call the constructor
		if(hr == TJS_E_MEMBERNOTFOUND) hr = TJS_S_OK;
			// missing constructor is OK ( is this ugly ? )
	}
	catch(...)
	{
		dsp->Release();
		throw;
	}

	if(TJS_SUCCEEDED(hr)) *result = dsp;
	return hr;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSDictionaryNI
//---------------------------------------------------------------------------
tTJSDictionaryNI::tTJSDictionaryNI()
{
	Owner = NULL;
}
//---------------------------------------------------------------------------
tTJSDictionaryNI::~tTJSDictionaryNI()
{
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSDictionaryNI::Construct(tjs_int numparams,
	tTJSVariant **param, iTJSDispatch2 *tjsobj)
{
	// called from TJS constructor
	//if(numparams != 0) return TJS_E_BADPARAMCOUNT;
	if(numparams > 1) return TJS_E_BADPARAMCOUNT;
	Owner = static_cast<tTJSCustomObject*>(tjsobj);
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSDictionaryNI::Invalidate() // Invalidate override
{
	// put here something on invalidation
	Owner = NULL;
	inherited::Invalidate();
}
//---------------------------------------------------------------------------
void tTJSDictionaryNI::Assign(iTJSDispatch2 * dsp, bool clear)
{
	// copy members from "dsp" to "Owner"

	// determin dsp's object type
	tTJSArrayNI *arrayni = NULL;
	if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		TJSGetArrayClassID(), (iTJSNativeInstance**)&arrayni)) )
	{
		// convert from array
		if(clear) Owner->Clear();

		// reserve area
		tjs_int reqcount = (tjs_int)(Owner->Count + arrayni->Items.size());
		Owner->RebuildHash( reqcount );

		tTJSArrayNI::tArrayItemIterator i;
		for(i = arrayni->Items.begin(); i != arrayni->Items.end(); i++)
		{
			tTJSVariantString *name = i->AsStringNoAddRef();
			i++;
			if(arrayni->Items.end() == i) break;
			Owner->PropSetByVS(TJS_MEMBERENSURE|TJS_IGNOREPROP, name,
				&(*i),
				Owner);
		}
	}
	else
	{
		// otherwise
		if(clear) Owner->Clear();
		
		tSaveMemberCountCallback countCallback;
		tTJSVariantClosure clo1(&countCallback, NULL);
		dsp->EnumMembers(TJS_IGNOREPROP, &clo1, dsp);
		tjs_int reqcount = countCallback.Count + Owner->Count;
		Owner->RebuildHash( reqcount );

		tAssignCallback callback;
		callback.Owner = Owner;

		tTJSVariantClosure clo2(&callback, NULL);
		dsp->EnumMembers(TJS_IGNOREPROP, &clo2, dsp);

	}
}
//---------------------------------------------------------------------------
void tTJSDictionaryNI::Clear()
{
	Owner->Clear();
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSDictionaryNI::tAssignCallback::FuncCall(tjs_uint32 flag,
	const tjs_char * membername, tjs_uint32 *hint, tTJSVariant *result,
	tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis)
{
	// called from iTJSDispatch2::EnumMembers
	// (tTJSDictionaryNI::Assign calls iTJSDispatch2::EnumMembers)
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;

	// hidden members are not copied
	tjs_uint32 flags = (tjs_int)*param[1];
	if(flags & TJS_HIDDENMEMBER)
	{
		if(result) *result = (tjs_int)1;
		return TJS_S_OK;
	}

	Owner->PropSetByVS(TJS_MEMBERENSURE|TJS_IGNOREPROP|flags,
		param[0]->AsStringNoAddRef(), param[2], Owner);

	if(result) *result = (tjs_int)1;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void tTJSDictionaryNI::SaveStructuredData(std::vector<iTJSDispatch2 *> &stack,
	iTJSTextWriteStream & stream, const ttstr &indentstr)
{
#ifdef TJS_TEXT_OUT_CRLF
	stream.Write(TJS_W("(const) %[\r\n"));
#else
	stream.Write(TJS_W("(const) %[\n"));
#endif
	ttstr indentstr2 = indentstr + TJS_W(" ");

	tSaveStructCallback callback;
	callback.Stack = &stack;
	callback.Stream = &stream;
	callback.IndentStr = &indentstr2;
	callback.First = true;
    tTJSVariantClosure clo(&callback, NULL);
	Owner->EnumMembers(TJS_IGNOREPROP, &clo, Owner);

#ifdef TJS_TEXT_OUT_CRLF
	if(!callback.First) stream.Write(TJS_W("\r\n"));
#else
	if(!callback.First) stream.Write(TJS_W("\n"));
#endif
	stream.Write(indentstr);
	stream.Write(TJS_W("]"));
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSDictionaryNI::tSaveStructCallback::FuncCall(
	tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	// called indirectly from tTJSDictionaryNI::SaveStructuredData

	if(numparams < 3) return TJS_E_BADPARAMCOUNT;

	// hidden members are not processed
	tjs_uint32 flags = (tjs_int)*param[1];
	if(flags & TJS_HIDDENMEMBER)
	{
		if(result) *result = (tjs_int)1;
		return TJS_S_OK;
	}

#ifdef TJS_TEXT_OUT_CRLF
	if(!First) Stream->Write(TJS_W(",\r\n"));
#else
	if(!First) Stream->Write(TJS_W(",\n"));
#endif

	First = false;

	Stream->Write(*IndentStr);

	Stream->Write(TJS_W("\""));
	Stream->Write(ttstr(*param[0]).EscapeC());
	Stream->Write(TJS_W("\" => "));

	tTJSVariantType type = param[2]->Type();
	if(type == tvtObject)
	{
		// object
		tTJSVariantClosure clo = param[2]->AsObjectClosureNoAddRef();
		tTJSArrayNI::SaveStructuredDataForObject(clo.SelectObjectNoAddRef(),
			*Stack, *Stream, *IndentStr);
	}
	else
	{
		Stream->Write(TJSVariantToExpressionString(*param[2]));
	}

	if(result) *result = (tjs_int)1;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void tTJSDictionaryNI::SaveStructuredBinary(std::vector<iTJSDispatch2 *> &stack, tTJSBinaryStream &stream )
{
	tSaveMemberCountCallback countCallback;
    tTJSVariantClosure cclo(&countCallback, NULL);
	Owner->EnumMembers(TJS_IGNOREPROP, &cclo, Owner);

	tjs_int count = countCallback.Count;
	tTJSBinarySerializer::PutStartMap( &stream, count );

	tSaveStructBinayCallback callback;
	callback.Stack = &stack;
	callback.Stream = &stream;
    tTJSVariantClosure clo(&callback, NULL);
	Owner->EnumMembers(TJS_IGNOREPROP, &clo, Owner);
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSDictionaryNI::tSaveStructBinayCallback::FuncCall(
	tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	// called indirectly from tTJSDictionaryNI::SaveStructuredBinary
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;

	// hidden members are not processed
	tjs_uint32 flags = (tjs_int)*param[1];
	if(flags & TJS_HIDDENMEMBER) {
		if(result) *result = (tjs_int)1;
		return TJS_S_OK;
	}

	tTJSBinarySerializer::PutString( Stream, param[0]->AsStringNoAddRef() );

	tTJSVariantType type = param[2]->Type();
	if( type == tvtObject ) {
		// object
		tTJSVariantClosure clo = param[2]->AsObjectClosureNoAddRef();
		tTJSArrayNI::SaveStructuredBinaryForObject( clo.SelectObjectNoAddRef(), *Stack, *Stream );
	} else {
		tTJSBinarySerializer::PutVariant( Stream, *param[2] );
	}

	if(result) *result = (tjs_int)1;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSDictionaryNI::tSaveMemberCountCallback::FuncCall(
	tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	// called indirectly from tTJSDictionaryNI::SaveStructuredBinary
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;
	// hidden members are not processed
	tjs_uint32 flags = (tjs_int)*param[1];
	if(flags & TJS_HIDDENMEMBER) {
		if(result) *result = (tjs_int)1;
		return TJS_S_OK;
	}
	Count++;
	if(result) *result = (tjs_int)1;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void tTJSDictionaryNI::AssignStructure(iTJSDispatch2 * dsp,
	std::vector<iTJSDispatch2 *> &stack)
{
	// assign structured data from dsp
	tTJSArrayNI *dicni = NULL;
	if(TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		ClassID_Dictionary, (iTJSNativeInstance**)&dicni)) )
	{
		// copy from dictionary
		stack.push_back(dsp);
		try
		{
			Owner->Clear();
			
			// reserve area
			tSaveMemberCountCallback countCallback;
            tTJSVariantClosure cclo(&countCallback, NULL);
			dsp->EnumMembers(TJS_IGNOREPROP, &cclo, dsp);
			tjs_int reqcount = countCallback.Count + Owner->Count;
			Owner->RebuildHash( reqcount );

			tAssignStructCallback callback;
			callback.Dest = Owner;
			callback.Stack = &stack;
            tTJSVariantClosure clo(&callback, NULL);
			dsp->EnumMembers(TJS_IGNOREPROP, &clo, dsp);
		}
		catch(...)
		{
			stack.pop_back();
			throw;
		}
		stack.pop_back();
	}
	else
	{
		TJS_eTJSError(TJSSpecifyDicOrArray);
	}

}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSDictionaryNI::tAssignStructCallback::FuncCall(
	tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
	tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	// called indirectly from tTJSDictionaryNI::AssignStructure or
	// tTJSArrayNI::AssignStructure

	if(numparams < 3) return TJS_E_BADPARAMCOUNT;

	// hidden members are not processed
	tjs_uint32 flags = (tjs_int)*param[1];
	if(flags & TJS_HIDDENMEMBER)
	{
		if(result) *result = (tjs_int)1;
		return TJS_S_OK;
	}

	tTJSVariant &value = *param[2];

	tTJSVariantType type = value.Type();
	if(type == tvtObject)
	{
		// object

		iTJSDispatch2 *dsp = value.AsObjectNoAddRef();
		// determin dsp's object type

		tTJSVariant val;

		tTJSDictionaryNI *dicni = NULL;
		tTJSArrayNI *arrayni = NULL;

		if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			TJSGetDictionaryClassID(), (iTJSNativeInstance**)&dicni)) )
		{
			// dictionary
			bool objrec = false;
			std::vector<iTJSDispatch2 *>::iterator i;
			for(i = Stack->begin(); i != Stack->end(); i++)
			{
				if(*i == dsp)
				{
					// object recursion detected
					objrec = true;
					break;
				}
			}
			if(objrec)
			{
				val.SetObject(NULL); // becomes null
			}
			else
			{
				iTJSDispatch2 * newobj = TJSCreateDictionaryObject();
				val.SetObject(newobj, newobj);
				newobj->Release();
				tTJSDictionaryNI * newni = NULL;
				if(TJS_SUCCEEDED(newobj->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
					TJSGetDictionaryClassID(), (iTJSNativeInstance**)&newni)) )
				{
					newni->AssignStructure(dsp, *Stack);
				}
			}
		}
		else if(dsp && TJS_SUCCEEDED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			TJSGetArrayClassID(), (iTJSNativeInstance**)&arrayni)) )
		{
			// array
			bool objrec = false;
			std::vector<iTJSDispatch2 *>::iterator i;
			for(i = Stack->begin(); i != Stack->end(); i++)
			{
				if(*i == dsp)
				{
					// object recursion detected
					objrec = true;
					break;
				}
			}
			if(objrec)
			{
				val.SetObject(NULL); // becomes null
			}
			else
			{
				iTJSDispatch2 * newobj = TJSCreateArrayObject();
				val.SetObject(newobj, newobj);
				newobj->Release();
				tTJSArrayNI * newni = NULL;
				if(TJS_SUCCEEDED(newobj->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
					TJSGetArrayClassID(), (iTJSNativeInstance**)&newni)) )
				{
					newni->AssignStructure(dsp, *Stack);
				}
			}
		}
		else
		{
			// other object types
			val = value;
		}

		Dest->PropSetByVS(TJS_MEMBERENSURE|TJS_IGNOREPROP, param[0]->AsStringNoAddRef(), &val, Dest);
	}
	else
	{
		// other types
		Dest->PropSetByVS(TJS_MEMBERENSURE|TJS_IGNOREPROP, param[0]->AsStringNoAddRef(), &value, Dest);
	}

	if(result) *result = (tjs_int)1;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSDictionaryObject
//---------------------------------------------------------------------------
tTJSDictionaryObject::tTJSDictionaryObject() : tTJSCustomObject()
{
	CallFinalize = false;
}
//---------------------------------------------------------------------------
tTJSDictionaryObject::tTJSDictionaryObject(tjs_int hashbits) : tTJSCustomObject(hashbits)
{
	CallFinalize = false;
}
//---------------------------------------------------------------------------
tTJSDictionaryObject::~tTJSDictionaryObject()
{
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDictionaryObject::FuncCall(tjs_uint32 flag, const tjs_char * membername,
		tjs_uint32 *hint,
		tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::FuncCall(flag, membername, hint, result, numparams,
		param, objthis);
//	if(hr == TJS_E_MEMBERNOTFOUND)
//		return TJS_E_INVALIDTYPE; // call operation for void
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDictionaryObject::PropGet(tjs_uint32 flag, const tjs_char * membername,
		tjs_uint32 *hint,
		tTJSVariant *result, iTJSDispatch2 *objthis)
{
	tjs_error hr;
	hr = inherited::PropGet(flag, membername, hint, result, objthis);
	if(hr == TJS_E_MEMBERNOTFOUND && !(flag & TJS_MEMBERMUSTEXIST))
	{
		if(result) result->Clear(); // returns void
		return TJS_S_OK;
	}
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSDictionaryObject::CreateNew(tjs_uint32 flag, const tjs_char * membername,
		tjs_uint32 *hint,
		iTJSDispatch2 **result, tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::CreateNew(flag, membername, hint, result, numparams,
		param, objthis);
	if(hr == TJS_E_MEMBERNOTFOUND && !(flag & TJS_MEMBERMUSTEXIST))
		return TJS_E_INVALIDTYPE; // call operation for void
	return hr;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD 
	tTJSDictionaryObject::Operation(tjs_uint32 flag, const tjs_char *membername,
		tjs_uint32 *hint,
		tTJSVariant *result, const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::Operation(flag, membername, hint, result, param, objthis);
	if(hr == TJS_E_MEMBERNOTFOUND && !(flag & TJS_MEMBERMUSTEXIST))
	{
		// value not found -> create a value, do the operation once more
		static tTJSVariant VoidVal;
		hr = inherited::PropSet(TJS_MEMBERENSURE, membername, hint, &VoidVal, objthis);
		if(TJS_FAILED(hr)) return hr;
		hr = inherited::Operation(flag, membername, hint, result, param, objthis);
	}
	return hr;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSGetDictionaryClassID
//---------------------------------------------------------------------------
tjs_int32 TJSGetDictionaryClassID()
{
	return ClassID_Dictionary;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSCreateDictionaryObject
//---------------------------------------------------------------------------
iTJSDispatch2 * TJSCreateDictionaryObject(iTJSDispatch2 **classout)
{
	// create a Dictionary object
	struct tHolder
	{
		iTJSDispatch2 * Obj;
		tHolder() { Obj = new tTJSDictionaryClass(); }
		~tHolder() { Obj->Release(); }
	} static dictionaryclass;

	if(classout) *classout = dictionaryclass.Obj, dictionaryclass.Obj->AddRef();

	tTJSDictionaryObject *dictionaryobj;
	(dictionaryclass.Obj)->CreateNew(0, NULL,  NULL,
		(iTJSDispatch2**)&dictionaryobj, 0, NULL, dictionaryclass.Obj);
	return dictionaryobj;
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
} // namespace TJS









