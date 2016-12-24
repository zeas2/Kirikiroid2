//---------------------------------------------------------------------------
/**
 * 
 */
//---------------------------------------------------------------------------
//!@file レイヤーツリーオーナー
//---------------------------------------------------------------------------
#ifndef LayerTreeOwnerImple_H
#define LayerTreeOwnerImple_H

#include "LayerTreeOwner.h"
#include "tvpinputdefs.h"

/**
 * 最小限のLayerTreeOwner機能を提供する。
 * ほぼ iTVPLayerManager 関連メソッドのみ
 * いくつかのメソッドは未実装なので、継承した先で実装する必要がある
 */
class tTVPLayerTreeOwner : public iTVPLayerTreeOwner
{
protected:
	size_t PrimaryLayerManagerIndex; //!< プライマリレイヤマネージャ
	std::vector<iTVPLayerManager *> Managers; //!< レイヤマネージャの配列
	tTVPRect DestRect; //!< 描画先位置

protected:
	iTVPLayerManager* GetLayerManagerAt(size_t index);
	const iTVPLayerManager* GetLayerManagerAt(size_t index) const;
	bool TransformToPrimaryLayerManager(tjs_int &x, tjs_int &y);
	bool TransformToPrimaryLayerManager(tjs_real &x, tjs_real &y);
	bool TransformFromPrimaryLayerManager(tjs_int &x, tjs_int &y);

	void GetPrimaryLayerSize( tjs_int &w, tjs_int &h ) const;

public:
	tTVPLayerTreeOwner();

	// LayerManager/Layer -> LTO
	virtual void TJS_INTF_METHOD RegisterLayerManager( class iTVPLayerManager* manager );
	virtual void TJS_INTF_METHOD UnregisterLayerManager( class iTVPLayerManager* manager );

	/* 実際の描画
	virtual void TJS_INTF_METHOD StartBitmapCompletion(iTVPLayerManager * manager) = 0;
	virtual void TJS_INTF_METHOD NotifyBitmapCompleted(class iTVPLayerManager * manager,
		tjs_int x, tjs_int y, const void * bits, const class BitmapInfomation * bitmapinfo,
		const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity) = 0;
	virtual void TJS_INTF_METHOD EndBitmapCompletion(iTVPLayerManager * manager) = 0;
	*/

	// 以下は何もしない
	virtual void TJS_INTF_METHOD SetMouseCursor(class iTVPLayerManager* manager, tjs_int cursor);
	virtual void TJS_INTF_METHOD GetCursorPos(class iTVPLayerManager* manager, tjs_int &x, tjs_int &y);
	virtual void TJS_INTF_METHOD SetCursorPos(class iTVPLayerManager* manager, tjs_int x, tjs_int y);
	virtual void TJS_INTF_METHOD ReleaseMouseCapture(class iTVPLayerManager* manager);

	virtual void TJS_INTF_METHOD SetHint(class iTVPLayerManager* manager, iTJSDispatch2* sender, const ttstr &hint);

	virtual void TJS_INTF_METHOD NotifyLayerResize(class iTVPLayerManager* manager);
	virtual void TJS_INTF_METHOD NotifyLayerImageChange(class iTVPLayerManager* manager);

	virtual void TJS_INTF_METHOD SetAttentionPoint(class iTVPLayerManager* manager, tTJSNI_BaseLayer *layer, tjs_int x, tjs_int y);
	virtual void TJS_INTF_METHOD DisableAttentionPoint(class iTVPLayerManager* manager);

	virtual void TJS_INTF_METHOD SetImeMode( class iTVPLayerManager* manager, tjs_int mode );
	virtual void TJS_INTF_METHOD ResetImeMode( class iTVPLayerManager* manager );

	// virtual iTJSDispatch2 * TJS_INTF_METHOD GetOwnerNoAddRef() const = 0;

	// 以下は上述のメソッドがコールされた後に、実際に値を設定するために呼ばれる
	// cursor == 0 は default
	virtual void OnSetMouseCursor( tjs_int cursor ) = 0;
	virtual void OnGetCursorPos(tjs_int &x, tjs_int &y) = 0;
	virtual void OnSetCursorPos(tjs_int x, tjs_int y) = 0;
	virtual void OnReleaseMouseCapture() = 0;
	virtual void OnSetHintText(iTJSDispatch2* sender, const ttstr &hint) = 0;

	/**
	 * プライマリーレイヤーのサイズが変更された時に呼ばれる
	 */
	virtual void OnResizeLayer( tjs_int w, tjs_int h ) = 0;
	/**
	 * プライマリーレイヤーの画像が変更されたので、必要に応じて再描画を行う
	 * NotifyLayerImageChange から呼ばれている
	 */
	virtual void OnChangeLayerImage() = 0;

	virtual void OnSetAttentionPoint(tTJSNI_BaseLayer *layer, tjs_int x, tjs_int y) = 0;
	virtual void OnDisableAttentionPoint() = 0;
	virtual void OnSetImeMode(tjs_int mode) = 0;
	virtual void OnResetImeMode() = 0;

	// LTO -> LayerManager/Layer
	// LayerManager に対してイベントを通知するためのメソッド
	void FireClick(tjs_int x, tjs_int y);
	void FireDoubleClick(tjs_int x, tjs_int y);
	void FireMouseDown(tjs_int x, tjs_int y, enum tTVPMouseButton mb, tjs_uint32 flags);
	void FireMouseUp(tjs_int x, tjs_int y, enum tTVPMouseButton mb, tjs_uint32 flags);
	void FireMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags);
	void FireMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x, tjs_int y);

	void FireReleaseCapture();
	void FireMouseOutOfWindow();

	void FireTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void FireTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void FireTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void FireTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag );
	void FireTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag );
	void FireMultiTouch();

	void FireKeyDown(tjs_uint key, tjs_uint32 shift);
	void FireKeyUp(tjs_uint key, tjs_uint32 shift);
	void FireKeyPress(tjs_char key);

	void FireDisplayRotate( tjs_int orientation, tjs_int rotate, tjs_int bpp, tjs_int hresolution, tjs_int vresolution );

	void FireRecheckInputState();

	// レイヤー管理補助
	tTJSNI_BaseLayer* GetPrimaryLayer();
	tTJSNI_BaseLayer* GetFocusedLayer();
	void SetFocusedLayer(tTJSNI_BaseLayer * layer);
};

#endif
