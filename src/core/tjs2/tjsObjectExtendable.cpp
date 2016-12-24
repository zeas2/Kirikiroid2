//---------------------------------------------------------------------------
// TJS2 "ExtendableObject" class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsObjectExtendable.h"

namespace TJS
{

tTJSExtendableObject::~tTJSExtendableObject() {
	if( SuperClass ) {
		SuperClass->Release();
		SuperClass = NULL;
	}
}
void tTJSExtendableObject::SetSuper( iTJSDispatch2* dsp ) {
	if( SuperClass ) {
		SuperClass->Release();
		SuperClass = NULL;
	}
	SuperClass = dsp;
	SuperClass->AddRef();
}
void tTJSExtendableObject::ExtendsClass( iTJSDispatch2* global, const ttstr& classname ) {
	tTJSVariant val;
	tjs_error er = global->PropGet( TJS_MEMBERMUSTEXIST, classname.c_str(), NULL, &val, global );
	if( TJS_FAILED(er) ) TJSThrowFrom_tjs_error( er, classname.c_str() );

	SetSuper( val.AsObjectNoAddRef() );
}

tjs_error TJS_INTF_METHOD
tTJSExtendableObject::FuncCall(tjs_uint32 flag, const tjs_char * membername,
	tjs_uint32 *hint,
	tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::FuncCall( flag, membername, hint, result, numparams, param, objthis );
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->FuncCall( flag, membername, hint, result, numparams, param, objthis );
	}
	return hr;
}
tjs_error TJS_INTF_METHOD
tTJSExtendableObject::CreateNew(tjs_uint32 flag, const tjs_char * membername,
	tjs_uint32 *hint,
	iTJSDispatch2 **result, tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::CreateNew( flag, membername, hint, result, numparams, param, objthis );
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->CreateNew( flag, membername, hint, result, numparams, param, objthis );
	}
	return hr;
}

tjs_error TJS_INTF_METHOD
tTJSExtendableObject::PropGet( tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint, tTJSVariant *result, iTJSDispatch2 *objthis )
{
	tjs_error hr = inherited::PropGet( flag, membername, hint, result, objthis );
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->PropGet( flag, membername, hint, result, objthis );
	}
	return hr;
}
tjs_error TJS_INTF_METHOD
tTJSExtendableObject::PropSet( tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint, const tTJSVariant *param, iTJSDispatch2 *objthis )
{
	tjs_error hr = inherited::PropSet( flag, membername, hint, param, objthis );
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->PropSet( flag, membername, hint, param, objthis );
	}
	return hr;
}
tjs_error TJS_INTF_METHOD
tTJSExtendableObject::IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint, const tjs_char *classname, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::IsInstanceOf(flag, membername, hint, classname, objthis);
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->IsInstanceOf(flag, membername, hint, classname, objthis);
	}
	return hr;
}
tjs_error TJS_INTF_METHOD
tTJSExtendableObject::GetCount(tjs_int *result, const tjs_char *membername, tjs_uint32 *hint, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::GetCount(result, membername, hint, objthis);
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->GetCount(result, membername, hint, objthis);
	}
	return hr;
}
tjs_error TJS_INTF_METHOD
tTJSExtendableObject::DeleteMember(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::DeleteMember( flag, membername, hint, objthis);
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->DeleteMember( flag, membername, hint, objthis);
	}
	return hr;
}
tjs_error TJS_INTF_METHOD
tTJSExtendableObject::Invalidate(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::Invalidate( flag, membername, hint, objthis);
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->Invalidate( flag, membername, hint, objthis);
	}
	if( membername == NULL ) {
		SuperClass->Invalidate( flag, membername, hint, objthis);
	}
	return hr;
}
tjs_error TJS_INTF_METHOD
tTJSExtendableObject::IsValid(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::IsValid( flag, membername, hint, objthis);
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->IsValid( flag, membername, hint, objthis);
	}
	return hr;
}

tjs_error TJS_INTF_METHOD
tTJSExtendableObject::Operation(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
	tTJSVariant *result, const tTJSVariant *param, iTJSDispatch2 *objthis)
{
	tjs_error hr = inherited::Operation( flag, membername, hint, result, param, objthis);
	if( hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL && membername != NULL ) {
		hr = SuperClass->Operation( flag, membername, hint, result, param, objthis);
	}
	return hr;
}

tjs_error TJS_INTF_METHOD
tTJSExtendableObject::NativeInstanceSupport(tjs_uint32 flag, tjs_int32 classid, iTJSNativeInstance **pointer) {
	tjs_error hr = inherited::NativeInstanceSupport( flag, classid, pointer );
	if( hr != TJS_S_OK && SuperClass != NULL && flag == TJS_NIS_GETINSTANCE ) {
		hr = SuperClass->NativeInstanceSupport( flag, classid, pointer );
	}
	return hr;
}
tjs_error TJS_INTF_METHOD tTJSExtendableObject::ClassInstanceInfo(tjs_uint32 flag, tjs_uint num, tTJSVariant *value) {
	if( flag == TJS_CII_SET_SUPRECLASS ) {
		SetSuper( value->AsObjectNoAddRef() );
		return TJS_S_OK;
	} else if( flag == TJS_CII_GET_SUPRECLASS ) {
		if( SuperClass ) {
			*value = tTJSVariant(SuperClass,SuperClass);
			return TJS_S_OK;
		} else {
			return TJS_E_FAIL;
		}
	} else {
		return inherited::ClassInstanceInfo( flag, num, value );
	}
}

} // namespace TJS

