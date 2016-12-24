//---------------------------------------------------------------------------
/**
 * レイヤーツリーを保持する機能を Windowのみでなく、一般化し、このインターフェイス
 * を持つクラスであれば、レイヤーツリーを持てるようにする
 * 
 */
//---------------------------------------------------------------------------
//!@file レイヤーツリーオーナー
//---------------------------------------------------------------------------
#ifndef LayerTreeOwner_H
#define LayerTreeOwner_H
#include "drawable.h"


class iTVPLayerTreeOwner
{
public:
	// LayerManager/Layer -> LTO
	virtual void TJS_INTF_METHOD RegisterLayerManager( class iTVPLayerManager* manager ) = 0;
	virtual void TJS_INTF_METHOD UnregisterLayerManager( class iTVPLayerManager* manager ) = 0;

	virtual void TJS_INTF_METHOD StartBitmapCompletion(iTVPLayerManager * manager) = 0;
	virtual void TJS_INTF_METHOD NotifyBitmapCompleted(class iTVPLayerManager * manager,
		tjs_int x, tjs_int y, class tTVPBaseTexture *bmp,
		const struct tTVPRect &cliprect, enum tTVPLayerType type, tjs_int opacity) = 0;
	virtual void TJS_INTF_METHOD EndBitmapCompletion(iTVPLayerManager * manager) = 0;

	virtual void TJS_INTF_METHOD SetMouseCursor(class iTVPLayerManager* manager, tjs_int cursor) = 0;
	virtual void TJS_INTF_METHOD GetCursorPos(class iTVPLayerManager* manager, tjs_int &x, tjs_int &y) = 0;
	virtual void TJS_INTF_METHOD SetCursorPos(class iTVPLayerManager* manager, tjs_int x, tjs_int y) = 0;
	virtual void TJS_INTF_METHOD ReleaseMouseCapture(class iTVPLayerManager* manager) = 0;

	virtual void TJS_INTF_METHOD SetHint(class iTVPLayerManager* manager, iTJSDispatch2* sender, const ttstr &hint) = 0;

	virtual void TJS_INTF_METHOD NotifyLayerResize(class iTVPLayerManager* manager) = 0;
	virtual void TJS_INTF_METHOD NotifyLayerImageChange(class iTVPLayerManager* manager) = 0;

	virtual void TJS_INTF_METHOD SetAttentionPoint(class iTVPLayerManager* manager, class tTJSNI_BaseLayer *layer, tjs_int x, tjs_int y) = 0;
	virtual void TJS_INTF_METHOD DisableAttentionPoint(class iTVPLayerManager* manager) = 0;

	virtual void TJS_INTF_METHOD SetImeMode( class iTVPLayerManager* manager, tjs_int mode ) = 0; // mode == tTVPImeMode
	virtual void TJS_INTF_METHOD ResetImeMode( class iTVPLayerManager* manager ) = 0;

	virtual iTJSDispatch2 * TJS_INTF_METHOD GetOwnerNoAddRef() const = 0;
	// LTO -> LayerManager/Layer
	// LTO からの通知は必要要件ではない
};

#endif
