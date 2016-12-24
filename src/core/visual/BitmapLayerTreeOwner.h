//---------------------------------------------------------------------------
/**
 * 描画先を Bitmap とする Layer Tree Owner
 * レイヤーに描かれて、合成された内容は、このクラスの保持する Bitmap に描かれる
 * 設定のために呼びれたメソッドもイベントとして通知される
 */
//---------------------------------------------------------------------------
//!@file レイヤーツリーオーナー
//---------------------------------------------------------------------------

#ifndef BitmapLayerTreeOwner_H
#define BitmapLayerTreeOwner_H

#include "LayerTreeOwnerImpl.h"

class tTJSNI_BitmapLayerTreeOwner : public tTJSNativeInstance, public tTVPLayerTreeOwner
{
	iTJSDispatch2 *Owner;
	iTJSDispatch2 *BitmapObject;
	tTJSNI_Bitmap *BitmapNI;

public:public:
	tTJSNI_BitmapLayerTreeOwner();
	~tTJSNI_BitmapLayerTreeOwner();

	// tTJSNativeInstance
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

	iTJSDispatch2* GetBitmapObjectNoAddRef();

	// tTVPLayerTreeOwner
	iTJSDispatch2 * TJS_INTF_METHOD GetOwnerNoAddRef() const { return Owner; }

	virtual void TJS_INTF_METHOD StartBitmapCompletion(iTVPLayerManager * manager);
	virtual void TJS_INTF_METHOD NotifyBitmapCompleted(class iTVPLayerManager * manager,
		tjs_int x, tjs_int y, tTVPBaseTexture *bmp,
		const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity);
	virtual void TJS_INTF_METHOD EndBitmapCompletion(iTVPLayerManager * manager);

	virtual void OnSetMouseCursor( tjs_int cursor );
	virtual void OnGetCursorPos(tjs_int &x, tjs_int &y);
	virtual void OnSetCursorPos(tjs_int x, tjs_int y);
	virtual void OnReleaseMouseCapture();
	virtual void OnSetHintText(iTJSDispatch2* sender, const ttstr &hint);

	virtual void OnResizeLayer( tjs_int w, tjs_int h );
	virtual void OnChangeLayerImage();

	virtual void OnSetAttentionPoint(tTJSNI_BaseLayer *layer, tjs_int x, tjs_int y);
	virtual void OnDisableAttentionPoint();
	virtual void OnSetImeMode(tjs_int mode);
	virtual void OnResetImeMode();

	tjs_int GetWidth() const { return BitmapNI->GetWidth(); }
	tjs_int GetHeight() const { return BitmapNI->GetHeight(); }
};

class tTJSNC_BitmapLayerTreeOwner : public tTJSNativeClass {
	typedef tTJSNativeClass inherited;
public:
	tTJSNC_BitmapLayerTreeOwner();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};

extern tTJSNativeClass * TVPCreateNativeClass_BitmapLayerTreeOwner();
#endif
