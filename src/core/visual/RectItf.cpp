
#include "tjsCommHead.h"

#include "RectItf.h"
#include "MsgIntf.h"

tTJSNI_Rect::tTJSNI_Rect() : Rect(0,0,0,0) {
}
tjs_error TJS_INTF_METHOD
	tTJSNI_Rect::Construct(tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *tjs_obj) {
	if( numparams > 0 ) {
		if( numparams == 1 ) {
			tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
			tTJSNI_Rect* src = NULL;
			if( clo.Object == NULL  ) return TJS_E_INVALIDPARAM;

			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&src)))
				return TJS_E_INVALIDPARAM;

			Rect.left = src->Get().left;
			Rect.top = src->Get().top;
			Rect.right = src->Get().right;
			Rect.bottom = src->Get().bottom;
		} else if( numparams == 4 ) {
			Rect.left = *param[0];
			Rect.top = *param[1];
			Rect.right = *param[2];
			Rect.bottom = *param[3];
		}
	}
	return TJS_S_OK;
}
void TJS_INTF_METHOD tTJSNI_Rect::Invalidate() {
}


tjs_uint32 tTJSNC_Rect::ClassID = -1;
tTJSNC_Rect::tTJSNC_Rect() : inherited(TJS_W("Rect") ) {
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(Rect) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_Rect,
	/*TJS class name*/Rect)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Rect)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/isEmpty)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(result) *result = (tjs_int)( _this->IsEmpty() ? 1 : 0 );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/isEmpty)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setSize)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->SetSize( *param[0], *param[1] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setOffset)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->SetOffset( *param[0], *param[1] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setOffset)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/addOffset)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->AddOffset( *param[0], *param[1] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/addOffset)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/clear)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	_this->Clear();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/clear)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/set)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 4) return TJS_E_BADPARAMCOUNT;
	_this->Set( *param[0], *param[1], *param[2], *param[3] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/set)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/clip)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	tTJSNI_Rect* src = NULL;
	if( clo.Object ) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&src)))
			return TJS_E_INVALIDPARAM;
		tjs_int ret = _this->Clip( *src ) ? 1 : 0;
		if(result) *result = ret;
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/clip)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/union)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	tTJSNI_Rect* src = NULL;
	if( clo.Object ) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&src)))
			return TJS_E_INVALIDPARAM;
		tjs_int ret = _this->Union( *src ) ? 1 : 0;
		if(result) *result = ret;
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/union)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/intersects)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	tTJSNI_Rect* src = NULL;
	if( clo.Object ) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&src)))
			return TJS_E_INVALIDPARAM;
		tjs_int ret = _this->Intersects( *src ) ? 1 : 0;
		if(result) *result = ret;
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/intersects)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/included)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	tTJSNI_Rect* src = NULL;
	if( clo.Object ) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&src)))
			return TJS_E_INVALIDPARAM;
		tjs_int ret = _this->Included( *src ) ? 1 : 0;
		if(result) *result = ret;
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/included)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/includedPos)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	tjs_int ret = _this->Included( *param[0], *param[1] ) ? 1 : 0;
	if(result) *result = ret;
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/includedPos)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/equal)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	tTJSNI_Rect* src = NULL;
	if( clo.Object ) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&src)))
			return TJS_E_INVALIDPARAM;
		tjs_int ret = _this->Equal( *src ) ? 1 : 0;
		if(result) *result = ret;
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/equal)
//----------------------------------------------------------------------


//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(width)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		*result = _this->GetWidth();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		_this->SetWidth( *param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(width)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(height)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		*result = _this->GetHeight();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		_this->SetHeight( *param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(height)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(left)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		*result = _this->Get().left;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		_this->Get().left = *param;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(left)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(top)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		*result = _this->Get().top;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		_this->Get().top = *param;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(top)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(right)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		*result = _this->Get().right;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		_this->Get().right = *param;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(right)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(bottom)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		*result = _this->Get().bottom;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		_this->Get().bottom = *param;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(bottom)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(nativeArray)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Rect);
		*result = (tTVInteger)(tjs_intptr_t)_this->Get().array;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(nativeArray)
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS
}

tTJSNativeInstance *tTJSNC_Rect::CreateNativeInstance() {
	return new tTJSNI_Rect();
}



//---------------------------------------------------------------------------
iTJSDispatch2 * TVPCreateRectObject( tjs_int left, tjs_int top, tjs_int right, tjs_int bottom )
{
	struct tHolder
	{
		iTJSDispatch2 * Obj;
		tHolder() { Obj = new tTJSNC_Rect(); }
		~tHolder() { Obj->Release(); }
	} static rectclass;

	iTJSDispatch2 *out;
	tTJSVariant param[4] = { left, top, right, bottom};
	tTJSVariant *pparam[4] = { param, param+1, param+2, param+3 };
	tjs_error hr = rectclass.Obj->CreateNew(0, NULL, NULL, &out, 4, pparam, rectclass.Obj);
	if(TJS_FAILED(hr)) TVPThrowInternalError;

	return out;
}


tTJSNativeClass * TVPCreateNativeClass_Rect()
{
	tTJSNativeClass *cls = new tTJSNC_Rect();
	return cls;
}