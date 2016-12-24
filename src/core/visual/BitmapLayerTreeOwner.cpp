//---------------------------------------------------------------------------
/**
 * 描画先を Bitmap とする Layer Tree Owner
 * レイヤーに描かれて、合成された内容は、このクラスの保持する Bitmap に描かれる
 * 設定のために呼びれたメソッドもイベントとして通知される
 */
//---------------------------------------------------------------------------
//!@file レイヤーツリーオーナー
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include "ComplexRect.h"
#include "BitmapIntf.h"
#include "LayerIntf.h"

#include "BitmapLayerTreeOwner.h"
#include "MsgIntf.h"

#include <assert.h>

tTJSNI_BitmapLayerTreeOwner::tTJSNI_BitmapLayerTreeOwner() 
: Owner(NULL), BitmapObject(NULL), BitmapNI(NULL)
{
}
tTJSNI_BitmapLayerTreeOwner::~tTJSNI_BitmapLayerTreeOwner() {
}

// tTJSNativeInstance
tjs_error TJS_INTF_METHOD tTJSNI_BitmapLayerTreeOwner::Construct(tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *tjs_obj) {

	Owner = tjs_obj; // no addref

	BitmapObject = GetBitmapObjectNoAddRef();
	if(TJS_FAILED(BitmapObject->NativeInstanceSupport(TJS_NIS_GETINSTANCE, tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&BitmapNI)))
		return TJS_E_INVALIDPARAM;

	return TJS_S_OK;
}
void TJS_INTF_METHOD tTJSNI_BitmapLayerTreeOwner::Invalidate() {
	// invalidate bitmap object
	BitmapNI = NULL;
	if( BitmapObject ) {
		BitmapObject->Invalidate(0, NULL, NULL, BitmapObject);
		BitmapObject->Release();
		BitmapObject = NULL;
	}
}

iTJSDispatch2* tTJSNI_BitmapLayerTreeOwner::GetBitmapObjectNoAddRef() {
	if(BitmapObject) return BitmapObject;
	// create bitmap object if the object is not yet created.
	BitmapObject = TVPCreateBitmapObject();
	return BitmapObject;
}

// tTVPLayerTreeOwner
void TJS_INTF_METHOD tTJSNI_BitmapLayerTreeOwner::StartBitmapCompletion(iTVPLayerManager * manager) {
}
void TJS_INTF_METHOD tTJSNI_BitmapLayerTreeOwner::NotifyBitmapCompleted(class iTVPLayerManager * manager,
	tjs_int x, tjs_int y, tTVPBaseTexture *bitmapinfo,
	const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity) {
	assert(BitmapNI);

	tjs_int w, h;
	GetPrimaryLayerSize( w, h );
	if( BitmapNI ) {
		BitmapNI->SetSize( w, h );
	}
	tjs_uint8* dstbits = (tjs_uint8*)BitmapNI->GetPixelBufferForWrite();
	tjs_int dstpitch = BitmapNI->GetPixelBufferPitch();
	// cliprect がはみ出していいないことを確認
	if( !(x < 0 || y < 0 || x + cliprect.get_width() > w || y + cliprect.get_height() > h) &&
		!(cliprect.left < 0 || cliprect.top < 0 ||
			cliprect.right > bitmapinfo->GetWidth() || cliprect.bottom > bitmapinfo->GetHeight()) )
	{
		// bitmapinfo で表された cliprect の領域を x,y にコピーする
		long src_y       = cliprect.top;
		long src_y_limit = cliprect.bottom;
		long src_x       = cliprect.left;
		long width_bytes   = cliprect.get_width() * 4; // 32bit
		long dest_y      = y;
		long dest_x      = x;
		const tjs_uint8 * src_p = (const tjs_uint8 *)bitmapinfo->GetScanLine(0);
		long src_pitch;
#if 0
		if(bitmapinfo->GetHeight() < 0) {
			// bottom-down
			src_pitch = bitmapinfo->GetWidth() * 4;
		} else
#endif
		{
			// bottom-up
			src_pitch = -(int)bitmapinfo->GetWidth() * 4;
			src_p += bitmapinfo->GetWidth() * 4 * (bitmapinfo->GetHeight() - 1);
		}

		for(; src_y < src_y_limit; src_y ++, dest_y ++) {
			const void *srcp = src_p + src_pitch * src_y + src_x * 4;
			void *destp = dstbits + dstpitch * dest_y + dest_x * 4;
			memcpy(destp, srcp, width_bytes);
		}
	}
}
void TJS_INTF_METHOD tTJSNI_BitmapLayerTreeOwner::EndBitmapCompletion(iTVPLayerManager * manager) {
}

void tTJSNI_BitmapLayerTreeOwner::OnSetMouseCursor( tjs_int cursor ) {
	if( Owner ) {
		tTJSVariant arg[1] = { cursor };
		static ttstr eventname(TJS_W("onSetMouseCursor"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 1, arg);
	}
}
void tTJSNI_BitmapLayerTreeOwner::OnGetCursorPos(tjs_int &x, tjs_int &y) {
	if( Owner ) {
		tjs_int vx = x, vy = y;
		tTJSVariant arg[2] = { vx, vy };
		static ttstr eventname(TJS_W("onGetCursorPos"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 2, arg);
		x = arg[0];
		y = arg[1];
	}
}
void tTJSNI_BitmapLayerTreeOwner::OnSetCursorPos(tjs_int x, tjs_int y) {
	if( Owner ) {
		tTJSVariant arg[2] = { x, y };
		static ttstr eventname(TJS_W("onSetCursorPos"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 2, arg);
	}
}
void tTJSNI_BitmapLayerTreeOwner::OnReleaseMouseCapture() {
	if( Owner ) {
		static ttstr eventname(TJS_W("onReleaseMouseCapture"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	}
}
void tTJSNI_BitmapLayerTreeOwner::OnSetHintText(iTJSDispatch2* sender, const ttstr &hint) {
	if( Owner ) {
		tTJSVariant clo(sender, sender);
		tTJSVariant arg[2] = { clo, hint };
		static ttstr eventname(TJS_W("onSetHintText"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 2, arg);
	}
}

void tTJSNI_BitmapLayerTreeOwner::OnResizeLayer( tjs_int w, tjs_int h ) {
	if( BitmapNI ) {
		BitmapNI->SetSize( w, h ); // サイズ変更に応じて、内部のBitmapもサイズ変更する
	}
	if( Owner ) {
		tTJSVariant arg[2] = { w, h };
		static ttstr eventname(TJS_W("onResizeLayer"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 2, arg);
	}
}
void tTJSNI_BitmapLayerTreeOwner::OnChangeLayerImage() {
	if( Owner ) {
		static ttstr eventname(TJS_W("onChangeLayerImage"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	}
}

void tTJSNI_BitmapLayerTreeOwner::OnSetAttentionPoint(tTJSNI_BaseLayer *layer, tjs_int x, tjs_int y) {
	if( Owner ) {
		iTJSDispatch2* owner = GetOwnerNoAddRef();
		tTJSVariant clo(owner, owner);
		tTJSVariant arg[3] = { clo, x, y };
		static ttstr eventname(TJS_W("onSetAttentionPoint"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 3, arg);
	}
}
void tTJSNI_BitmapLayerTreeOwner::OnDisableAttentionPoint() {
	if( Owner ) {
		static ttstr eventname(TJS_W("onDisableAttentionPoint"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	}
}
void tTJSNI_BitmapLayerTreeOwner::OnSetImeMode(tjs_int mode) {
	if( Owner ) {
		tTJSVariant arg[1] = { mode };
		static ttstr eventname(TJS_W("onSetImeMode"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 1, arg);
	}
}
void tTJSNI_BitmapLayerTreeOwner::OnResetImeMode() {
	if( Owner ) {
		static ttstr eventname(TJS_W("onResetImeMode"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	}
}


//----------------------------------------------------------------------
tjs_uint32 tTJSNC_BitmapLayerTreeOwner::ClassID = -1;

//----------------------------------------------------------------------
tTJSNC_BitmapLayerTreeOwner::tTJSNC_BitmapLayerTreeOwner() : inherited(TJS_W("BitmapLayerTreeOwner") ) {

	// registration of native members
	TJS_BEGIN_NATIVE_MEMBERS(BitmapLayerTreeOwner) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_BitmapLayerTreeOwner,
	/*TJS class name*/BitmapLayerTreeOwner)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/BitmapLayerTreeOwner)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireClick)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->FireClick(*param[0], *param[1]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireClick)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireDoubleClick)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->FireDoubleClick(*param[0], *param[1] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireDoubleClick)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireMouseDown)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 4) return TJS_E_BADPARAMCOUNT;
	_this->FireMouseDown(*param[0], *param[1], (tTVPMouseButton)(tjs_int)*param[2], (tjs_uint32)(tjs_int64)*param[3]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireMouseDown)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireMouseUp)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 4) return TJS_E_BADPARAMCOUNT;
	_this->FireMouseUp(*param[0], *param[1], (tTVPMouseButton)(tjs_int)*param[2], (tjs_uint32)(tjs_int64)*param[3]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireMouseUp)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireMouseMove)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;
	_this->FireMouseMove(*param[0], *param[1], (tjs_uint32)(tjs_int64)*param[3]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireMouseMove)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireMouseWheel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 4) return TJS_E_BADPARAMCOUNT;
	_this->FireMouseWheel((tjs_uint32)(tjs_int64)*param[0], *param[1], *param[2], *param[3] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireMouseWheel)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireReleaseCapture)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	_this->FireReleaseCapture();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireReleaseCapture)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireMouseOutOfWindow)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	_this->FireMouseOutOfWindow();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireMouseOutOfWindow)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireTouchDown)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	_this->FireTouchDown(*param[0], *param[1], *param[2], *param[3], (tjs_uint32)(tjs_int64)*param[4]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireTouchDown)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireTouchUp)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	_this->FireTouchUp(*param[0], *param[1], *param[2], *param[3], (tjs_uint32)(tjs_int64)*param[4]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireTouchUp)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireTouchMove)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	_this->FireTouchMove(*param[0], *param[1], *param[2], *param[3], (tjs_uint32)(tjs_int64)*param[4]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireTouchMove)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireTouchScaling)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	_this->FireTouchScaling(*param[0], *param[1], *param[2], *param[3], *param[4]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireTouchScaling)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireTouchRotate)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 6) return TJS_E_BADPARAMCOUNT;
	_this->FireTouchRotate(*param[0], *param[1], *param[2], *param[3], *param[4], *param[5]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireTouchRotate)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireMultiTouch)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	_this->FireMultiTouch();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireMultiTouch)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireKeyDown)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->FireKeyDown( (tjs_uint32)(tjs_int64)*param[0], (tjs_uint32)(tjs_int64)*param[1] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireKeyDown)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireKeyUp)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->FireKeyUp( (tjs_uint32)(tjs_int64)*param[0], (tjs_uint32)(tjs_int64)*param[1] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireKeyUp)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireKeyPress)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	_this->FireKeyPress( (tjs_char)(tjs_int)*param[0] );
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireKeyPress)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireDisplayRotate)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	_this->FireDisplayRotate(*param[0], *param[1], *param[2], *param[3], *param[4]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireDisplayRotate)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireRecheckInputState)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	_this->FireRecheckInputState();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireRecheckInputState)
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onSetMouseCursor)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onSetMouseCursor)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onGetCursorPos)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onGetCursorPos)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onSetCursorPos)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onSetCursorPos)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onReleaseMouseCapture)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onReleaseMouseCapture)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onSetHintText)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onSetHintText)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onResizeLayer)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onResizeLayer)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onChangeLayerImage)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onChangeLayerImage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onSetAttentionPoint)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onSetAttentionPoint)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onDisableAttentionPoint)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onDisableAttentionPoint)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onSetImeMode)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onSetImeMode)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onResetImeMode)
{
	//TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onResetImeMode)
//----------------------------------------------------------------------

//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(width)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
		*result = (tjs_int64)_this->GetWidth();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(width)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(height)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
		*result = (tjs_int64)_this->GetHeight();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(height)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(bitmap)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
		if( _this->GetBitmapObjectNoAddRef() )
			*result = tTJSVariant(_this->GetBitmapObjectNoAddRef(), _this->GetBitmapObjectNoAddRef());
		else
			*result = tTJSVariant((iTJSDispatch2*)NULL);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(bitmap)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(layerTreeOwnerInterface)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
		*result = reinterpret_cast<tjs_int64>(static_cast<iTVPLayerTreeOwner*>(_this));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(layerTreeOwnerInterface)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(focusedLayer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
		tTJSNI_BaseLayer *lay = _this->GetFocusedLayer();
		if(lay && lay->GetOwnerNoAddRef())
			*result = tTJSVariant(lay->GetOwnerNoAddRef(), lay->GetOwnerNoAddRef());
		else
			*result = tTJSVariant((iTJSDispatch2*)NULL);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);

		tTJSNI_BaseLayer *to = NULL;

		if(param->Type() != tvtVoid)
		{
			tTJSVariantClosure clo = param->AsObjectClosureNoAddRef();
			if(clo.Object)
			{
				if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
					tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&to)))
					TVPThrowExceptionMessage(TVPSpecifyLayer);
			}
		}

		_this->SetFocusedLayer(to);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(focusedLayer)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(primaryLayer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BitmapLayerTreeOwner);
		tTJSNI_BaseLayer *pri = _this->GetPrimaryLayer();
		if(!pri) TVPThrowExceptionMessage(TJS_W("Not have primary layer"));

		if(pri && pri->GetOwnerNoAddRef())
			*result = tTJSVariant(pri->GetOwnerNoAddRef(), pri->GetOwnerNoAddRef());
		else
			*result = tTJSVariant((iTJSDispatch2*)NULL);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(primaryLayer)
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS
}

//----------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_BitmapLayerTreeOwner::CreateNativeInstance() {
	return new tTJSNI_BitmapLayerTreeOwner();
}
//----------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_BitmapLayerTreeOwner()
{
	return new tTJSNC_BitmapLayerTreeOwner();
}
//----------------------------------------------------------------------

