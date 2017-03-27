//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Layer Management
//---------------------------------------------------------------------------
#ifndef LayerManagerH
#define LayerManagerH


#include "LayerIntf.h"
#include "drawable.h"
#include <algorithm>
#include <vector>

/*[*/
class tTJSNI_BaseLayer;
//---------------------------------------------------------------------------
// abstract class of Layer Manager 
//---------------------------------------------------------------------------
class iTVPLayerManager
{
public:
//-- object lifetime management
	//! @brief	参照カウンタをインクリメントする
	virtual void TJS_INTF_METHOD AddRef() = 0;

	//! @brief	参照カウンタをデクリメントする
	virtual void TJS_INTF_METHOD Release() = 0;

//-- draw device specific information
	//! @brief	描画デバイス固有の情報を設定する
	//! @param	data	描画デバイス固有の情報
	//! @note	描画デバイス固有の情報をレイヤマネージャに設定する。
	//!			レイヤマネージャではこの情報の中身については関知しない。
	//!			描画デバイス側で目印に使ったり、特定の情報と結びつけて管理する。
	virtual void TJS_INTF_METHOD SetDrawDeviceData(void * data) = 0;

	//! @brief	描画デバイス固有の情報を取得する
	//! @return	描画デバイス固有の情報
	virtual void * TJS_INTF_METHOD GetDrawDeviceData() const = 0;

//-- layer metrics
	//! @brief	プライマリレイヤのサイズを取得する
	//! @param	w	レイヤの横幅(ピクセル単位)
	//! @param	h	レイヤの縦幅(ピクセル単位)
	//! @return	取得に成功すれば真、失敗すれば偽
	virtual bool TJS_INTF_METHOD GetPrimaryLayerSize(tjs_int &w, tjs_int &h) const = 0;

//-- layer structure information
	//! @brief	プライマリレイヤの取得
	//! @return	プライマリレイヤ
	virtual tTJSNI_BaseLayer * TJS_INTF_METHOD GetPrimaryLayer() const = 0;

	//! @brief	フォーカスのあるレイヤの取得
	//! @return	フォーカスのあるレイヤ
	virtual tTJSNI_BaseLayer * TJS_INTF_METHOD GetFocusedLayer() const = 0;

	//! @brief	フォーカスのあるレイヤの設定
	//! @param	layer	フォーカスのあるレイヤ
	virtual void TJS_INTF_METHOD SetFocusedLayer(tTJSNI_BaseLayer * layer) = 0;

//-- HID releted
	//! @brief		クリックされた
	//! @param		x		プライマリレイヤ座標上における x 位置
	//! @param		y		プライマリレイヤ座標上における y 位置
	virtual void TJS_INTF_METHOD NotifyClick(tjs_int x, tjs_int y) = 0;

	//! @brief		ダブルクリックされた
	//! @param		x		プライマリレイヤ座標上における x 位置
	//! @param		y		プライマリレイヤ座標上における y 位置
	virtual void TJS_INTF_METHOD NotifyDoubleClick(tjs_int x, tjs_int y) = 0;

	//! @brief		マウスボタンが押下された
	//! @param		x		プライマリレイヤ座標上における x 位置
	//! @param		y		プライマリレイヤ座標上における y 位置
	//! @param		mb		どのマウスボタンか
	//! @param		flags	フラグ(TVP_SS_*定数の組み合わせ)
	virtual void TJS_INTF_METHOD NotifyMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags) = 0;

	//! @brief		マウスボタンが離された
	//! @param		x		プライマリレイヤ座標上における x 位置
	//! @param		y		プライマリレイヤ座標上における y 位置
	//! @param		mb		どのマウスボタンか
	//! @param		flags	フラグ(TVP_SS_*定数の組み合わせ)
	virtual void TJS_INTF_METHOD NotifyMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags) = 0;

	//! @brief		マウスが移動した
	//! @param		x		プライマリレイヤ座標上における x 位置
	//! @param		y		プライマリレイヤ座標上における y 位置
	//! @param		flags	フラグ(TVP_SS_*定数の組み合わせ)
	virtual void TJS_INTF_METHOD NotifyMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags) = 0;

	//! @brief		マウスキャプチャを解放する
	//! @note		マウスキャプチャを解放すべき場合にウィンドウから呼ばれる。
	virtual void TJS_INTF_METHOD ReleaseCapture() = 0;

	//! @brief		マウスがプライマリレイヤ外に移動した
	virtual void TJS_INTF_METHOD NotifyMouseOutOfWindow() = 0;

	//! @brief		キーが押された
	//! @param		key		仮想キーコード
	//! @param		shift	シフトキーの状態
	virtual void TJS_INTF_METHOD NotifyKeyDown(tjs_uint key, tjs_uint32 shift) = 0;

	//! @brief		キーが離された
	//! @param		key		仮想キーコード
	//! @param		shift	シフトキーの状態
	virtual void TJS_INTF_METHOD NotifyKeyUp(tjs_uint key, tjs_uint32 shift) = 0;

	//! @brief		キーによる入力
	//! @param		key		文字コード
	virtual void TJS_INTF_METHOD NotifyKeyPress(tjs_char key) = 0;

	//! @brief		マウスホイールが回転した
	//! @param		shift	シフトキーの状態
	//! @param		delta	回転角
	//! @param		x		プライマリレイヤ座標上における x 位置
	//! @param		y		プライマリレイヤ座標上における y 位置
	virtual void TJS_INTF_METHOD NotifyMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x, tjs_int y) = 0;

	//! @brief		画面がタッチされた
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		id		タッチ識別用ID
	virtual void TJS_INTF_METHOD NotifyTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) = 0;

	//! @brief		タッチが離された
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		id		タッチ識別用ID
	virtual void TJS_INTF_METHOD NotifyTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) = 0;

	//! @brief		タッチが移動した
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		id		タッチ識別用ID
	virtual void TJS_INTF_METHOD NotifyTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) = 0;

	//! @brief		拡大タッチ操作が行われた
	//! @param		startdist	開始時の2点間の幅
	//! @param		curdist	現在の2点間の幅
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		flag	タッチ状態フラグ
	virtual void TJS_INTF_METHOD NotifyTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag ) = 0;

	//! @brief		回転タッチ操作が行われた
	//! @param		startangle	開始時の角度
	//! @param		curangle	現在の角度
	//! @param		dist	現在の2点間の幅
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		flag	タッチ状態フラグ
	virtual void TJS_INTF_METHOD NotifyTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag ) = 0;

	//! @brief		マルチタッチ状態が更新された
	virtual void TJS_INTF_METHOD NotifyMultiTouch() = 0;

	//! @brief		入力状態のチェック
	//! @note		ウィンドウから約1秒おきに、レイヤマネージャがユーザからの入力の状態を
	//!				再チェックするために呼ばれる。レイヤ状態の変化がユーザの入力とは
	//!				非同期に行われた場合、たとえばマウスカーソルの下にレイヤが出現した
	//!				のにもかかわらず、マウスカーソルがそのレイヤの指定する形状に変更されない
	//!				といった状況が発生しうる。このような状況に対処するため、ウィンドウから
	//!				このメソッドが約1秒おきに呼ばれる。
	virtual void TJS_INTF_METHOD RecheckInputState() = 0;

//-- invalidation/update
	//! @brief		描画デバイスが望むレイヤの出力形式を設定する
	//! @param		type	レイヤ形式
	//! @note		デフォルトは ltOpaque 。描画デバイスが他の形式の画像を出力として
	//!				望むならばその形式を指定する。ただし、プライマリレイヤの type
	//!				プロパティも同様に変更すること。
	virtual void TJS_INTF_METHOD SetDesiredLayerType(tTVPLayerType type) = 0;

	//! @brief		特定の矩形の再描画を要求する
	//! @param		r		プライマリレイヤ座標上における矩形
	//! @note		特定の矩形の再描画をレイヤマネージャに対して要求する。
	//!				要求は記録されるだけでこのメソッドはすぐに戻る。実際にそれが
	//!				演算されるのは UpdateToDrawDevice() を呼んだときである。
	virtual void TJS_INTF_METHOD RequestInvalidation(const tTVPRect &r) = 0; // draw device -> layer

	//! @brief		内容の再描画を行う
	//! @note		内容の再描画を行う際に呼ぶ。このメソッド内では、レイヤマネージャは
	//!				iTVPDrawDevice::StartBitmapCompletion()
	//!				iTVPDrawDevice::NotifyBitmapCompleted()
	//!				iTVPDrawDevice::EndBitmapCompletion() の各メソッドを用い、
	//!				いままでに変更が行われた領域などを順次描画デバイスに送る。
	virtual void TJS_INTF_METHOD UpdateToDrawDevice() = 0;

//-- debug assist
	//! @brief		(Window->DrawDevice) レイヤ構造をコンソールにダンプする
	virtual void TJS_INTF_METHOD DumpLayerStructure() = 0;
    virtual iTVPBaseBitmap *GetDrawBuffer() = 0;
};
//---------------------------------------------------------------------------
/*]*/

//---------------------------------------------------------------------------
// Touch capture infomation for layer
//---------------------------------------------------------------------------
struct tTVPTouchCaptureLayer {
	tjs_uint32	TouchID;
	tTJSNI_BaseLayer*	Owner;
	tTVPTouchCaptureLayer( tjs_uint32 id, tTJSNI_BaseLayer* layer ) : TouchID(id), Owner(layer){}
};

// texture for last render target
class tTVPDestTexture : public tTVPBaseTexture
{
	bool HoldAlpha = true;

public:
    tTVPDestTexture(tjs_uint w, tjs_uint h) : tTVPBaseTexture(w, h) {}

//     bool Blt(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
// 		tTVPRect refrect, tTVPBBBltMethod method, tjs_int opa);
	virtual bool CopyRect(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
		tTVPRect refrect, tjs_int plane = (TVP_BB_COPY_MAIN | TVP_BB_COPY_MASK));

	void SetHoldAlpha(bool b) { HoldAlpha = b; }
};

//---------------------------------------------------------------------------
// tTVPLayerManager
//---------------------------------------------------------------------------
// layer mamager which is to be connected to draw device
//---------------------------------------------------------------------------
class tTVPLayerManager : public iTVPLayerManager, public tTVPDrawable
{
	tjs_int RefCount; //!< reference count
	class iTVPLayerTreeOwner* LayerTreeOwner;

	void * DrawDeviceData; //!< draw device specific information

	tTVPBaseTexture * DrawBuffer;
	tTVPLayerType DesiredLayerType; //!< desired layer type by the draw device for this layer manager

	tTJSNI_BaseLayer * CaptureOwner;
	tTJSNI_BaseLayer * LastMouseMoveSent;
	std::vector<tTVPTouchCaptureLayer> TouchCapture;	//!< 同時タッチ数は多くても10点程度なのでvectorで持つ(ほぼ1or2点)
	tjs_int64 ReleaseTouchCaptureIDMark;	//!< last touch down id

	std::vector<tTJSNI_BaseLayer *> ModalLayerVector;
		// pointer to modal layer vector
	tTJSNI_BaseLayer * FocusedLayer; // pointer to current focused layer
	tTJSNI_BaseLayer * Primary; // primary layer
	bool OverallOrderIndexValid;
	std::vector<tTJSNI_BaseLayer*> AllNodes;
		// hold overall nodes;
		// use GetAllNodes to retrieve the newest information of this
	tTVPComplexRect UpdateRegion; // window update region

	bool FocusChangeLock;

	bool VisualStateChanged;
		// flag for visual
		// state changing ( see tTJSNI_BaseLaye::NotifyChildrenVisualStateChanged)

	tjs_int LastMouseMoveX;
	tjs_int LastMouseMoveY;

	bool ReleaseCaptureCalled;

	bool InNotifyingHintOrCursorChange;
	bool HoldAlpha = true;
public:
	tTVPLayerManager(class iTVPLayerTreeOwner *owner);

private:
	virtual ~tTVPLayerManager();
public:
	virtual void TJS_INTF_METHOD AddRef();
	virtual void TJS_INTF_METHOD Release();

	virtual void TJS_INTF_METHOD SetDrawDeviceData(void * data) { DrawDeviceData = data; }
	virtual void * TJS_INTF_METHOD GetDrawDeviceData() const { return DrawDeviceData; }

public:
	void RegisterSelfToWindow();
	void UnregisterSelfFromWindow();

public:
	virtual void TJS_INTF_METHOD SetDesiredLayerType(tTVPLayerType type) { DesiredLayerType = type; }
	void SetHoldAlpha(bool b);

public: // methods from tTVPDrawable
	virtual tTVPBaseTexture * GetDrawTargetBitmap(const tTVPRect &rect,
		tTVPRect &cliprect);

	virtual tTVPLayerType GetTargetLayerType();

	virtual void DrawCompleted(const tTVPRect &destrect,
		tTVPBaseTexture *bmp, const tTVPRect &cliprect,
		tTVPLayerType type, tjs_int opacity) override;
	virtual tTVPBaseTexture *GetDrawBuffer() { return DrawBuffer; }
	tTVPBaseTexture* GetOrCreateDrawBuffer();

public:
	void AttachPrimary(tTJSNI_BaseLayer *pri); // attach primary layer to the manager
	void DetachPrimary(); // detach primary layer from the manager

	virtual tTJSNI_BaseLayer * TJS_INTF_METHOD GetPrimaryLayer() const { return Primary; }
	bool IsPrimaryLayerAttached() const { return Primary != NULL; } 

	virtual bool TJS_INTF_METHOD GetPrimaryLayerSize(tjs_int &w, tjs_int &h) const;

	void NotifyPart(tTJSNI_BaseLayer *lay); // notifies layer parting from its parent

	tTVPComplexRect & GetUpdateRegionForCompletion() { return UpdateRegion; }

private:

	void _RecreateOverallOrderIndex(tjs_uint& index,
		std::vector<tTJSNI_BaseLayer*>& nodes);

public:
	void InvalidateOverallIndex();
	void RecreateOverallOrderIndex();

	std::vector<tTJSNI_BaseLayer*> & GetAllNodes();

	void NotifyVisualStateChanged() { VisualStateChanged = true; }
	bool GetVisualStateChanged() { return VisualStateChanged; }
	void QueryUpdateExcludeRect();


public:

	void NotifyMouseCursorChange(tTJSNI_BaseLayer *layer, tjs_int cursor);
	void SetMouseCursor(tjs_int cursor);

	void GetCursorPos(tjs_int &x, tjs_int &y);
		// get cursor position in primary coordinates
	void SetCursorPos(tjs_int x, tjs_int y);
		// set cursor position in primary coordinates

	void NotifyHintChange(tTJSNI_BaseLayer *layer, const ttstr & hint);
	void SetHint(iTJSDispatch2* sender, const ttstr &hint);
		// set layer hint to current window

	void NotifyLayerResize();  // layer -> window
	void NotifyWindowInvalidation(); // layer -> window

public:
	class iTVPLayerTreeOwner* GetLayerTreeOwner() const { return LayerTreeOwner; }
	void SetLayerTreeOwner(class iTVPLayerTreeOwner* owner);
	void NotifyResizeFromWindow(tjs_uint w, tjs_uint h); // draw device -> layer
	virtual void TJS_INTF_METHOD RequestInvalidation(const tTVPRect &r); // draw device -> layer

	virtual void TJS_INTF_METHOD NotifyClick(tjs_int x, tjs_int y) { PrimaryClick(x, y); }
	virtual void TJS_INTF_METHOD NotifyDoubleClick(tjs_int x, tjs_int y) { PrimaryDoubleClick(x, y); }
	virtual void TJS_INTF_METHOD NotifyMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags)
		{ PrimaryMouseDown(x, y, mb, flags); }
	virtual void TJS_INTF_METHOD NotifyMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags)
		{ PrimaryMouseUp(x, y, mb, flags); }
	virtual void TJS_INTF_METHOD NotifyMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags)
		{ PrimaryMouseMove(x, y, flags); }
	virtual void TJS_INTF_METHOD NotifyMouseOutOfWindow()
		{ MouseOutOfWindow(); }
	virtual void TJS_INTF_METHOD NotifyKeyDown(tjs_uint key, tjs_uint32 shift)
		{ PrimaryKeyDown(key, shift); }
	virtual void TJS_INTF_METHOD NotifyKeyUp(tjs_uint key, tjs_uint32 shift)
		{ PrimaryKeyUp(key, shift); }
	virtual void TJS_INTF_METHOD NotifyKeyPress(tjs_char key)
		{ PrimaryKeyPress(key); }
	virtual void TJS_INTF_METHOD NotifyMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x, tjs_int y)
		{ PrimaryMouseWheel(shift, delta, x, y); }

	virtual void TJS_INTF_METHOD NotifyTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id )
		{ PrimaryTouchDown(x, y, cx, cy, id); }
	virtual void TJS_INTF_METHOD NotifyTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id )
		{ PrimaryTouchUp(x, y, cx, cy, id); }
	virtual void TJS_INTF_METHOD NotifyTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id )
		{ PrimaryTouchMove(x, y, cx, cy, id); }
	virtual void TJS_INTF_METHOD NotifyTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag )
		{ PrimaryTouchScaling(startdist, curdist, cx, cy, flag); }
	virtual void TJS_INTF_METHOD NotifyTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag )
		{ PrimaryTouchRotate(startangle, curangle, dist, cx, cy, flag); }
	virtual void TJS_INTF_METHOD NotifyMultiTouch()
		{ PrimaryMultiTouch(); }

	void PrimaryTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void PrimaryTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void PrimaryTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void PrimaryTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag );
	void PrimaryTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag );
	void PrimaryMultiTouch();

	tTJSNI_BaseLayer * GetMostFrontChildAt(tjs_int x, tjs_int y,
		tTJSNI_BaseLayer *except = NULL, bool get_disabled = false);

	void PrimaryClick(tjs_int x, tjs_int y);
	void PrimaryDoubleClick(tjs_int x, tjs_int y);

	void PrimaryMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags);
	void PrimaryMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags);

	void PrimaryMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags);
	void ForceMouseLeave();
	void ForceMouseRecheck();
	void MouseOutOfWindow();
	void LeaveMouseFromTree(tTJSNI_BaseLayer *root); // force to leave mouse

	virtual void TJS_INTF_METHOD ReleaseCapture();
	void ReleaseCaptureFromTree(tTJSNI_BaseLayer * layer);

	bool BlurTree(tTJSNI_BaseLayer *root); // remove focus from "root"
	tTJSNI_BaseLayer *SearchFirstFocusable(bool ignore_chain_focusable = true); // search first focusable layer


	virtual tTJSNI_BaseLayer * TJS_INTF_METHOD GetFocusedLayer() const { return FocusedLayer; }
	void CheckTreeFocusableState(tTJSNI_BaseLayer *root);
		// check newly added tree's focusable state
	bool SetFocusTo(tTJSNI_BaseLayer *layer, bool direction = true);
		// set focus to layer
	void TJS_INTF_METHOD SetFocusedLayer(tTJSNI_BaseLayer * layer) { SetFocusTo(layer, false); }
	tTJSNI_BaseLayer *FocusPrev(); // focus to previous layer
	tTJSNI_BaseLayer *FocusNext(); // focus to next layer
	void ReleaseAllModalLayer(); // release all modal layer on invalidation
	void SetModeTo(tTJSNI_BaseLayer *layer); // set mode to layer
	void RemoveModeFrom(tTJSNI_BaseLayer *layer); // remove mode from layer
	void RemoveTreeModalState(tTJSNI_BaseLayer *root);
		// remove modal state from given tree
	tTJSNI_BaseLayer *GetCurrentModalLayer() const;

private:
	bool SearchAttentionPoint(tTJSNI_BaseLayer *target, tjs_int &x, tjs_int &y);
	void SetAttentionPointOf(tTJSNI_BaseLayer *layer);
	void DisableAttentionPoint();

public:
	void NotifyAttentionStateChanged(tTJSNI_BaseLayer *from);

private:
	void SetImeModeOf(tTJSNI_BaseLayer *layer);
	void ResetImeMode();

public:
	void NotifyImeModeChanged(tTJSNI_BaseLayer *from);

private:
	tjs_int EnabledWorkRefCount;
public:
	void SaveEnabledWork();
	void NotifyNodeEnabledState();
	void PrimaryKeyDown(tjs_uint key, tjs_uint32 shift);
	void PrimaryKeyUp(tjs_uint key, tjs_uint32 shift);
	void PrimaryKeyPress(tjs_char key);
	void PrimaryMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x, tjs_int y);

	void AddUpdateRegion(const tTVPComplexRect &rects);
	void AddUpdateRegion(const tTVPRect & rect);
	void PrimaryUpdateByWindow(const tTVPRect &rect);
	virtual void TJS_INTF_METHOD UpdateToDrawDevice();
	void NotifyUpdateRegionFixed();

public:
	void TJS_INTF_METHOD RecheckInputState();
		// To re-check current layer under current mouse position
		// and update hint, cursor type and process layer enter/leave.
		// This can be reasonably slow, about 1 sec interval.

public:
	void TJS_INTF_METHOD DumpLayerStructure();
	
	void ReleaseTouchCapture( tjs_uint32 id );
	void ReleaseTouchCaptureAll();
private:
	struct FindTouchID {
		tjs_uint32 TouchID;
		inline FindTouchID( tjs_uint32 id ) : TouchID(id) {}
		inline bool operator()(const tTVPTouchCaptureLayer& touch) const { return touch.TouchID == TouchID; }
	};
	inline tTJSNI_BaseLayer* GetTouchCapture( tjs_uint32 id )
	{
		FindTouchID pred( id );
		std::vector<tTVPTouchCaptureLayer>::iterator itr = std::find_if( TouchCapture.begin(), TouchCapture.end(), pred );
		if( itr != TouchCapture.end() )
		{
			return itr->Owner;
		}
		else
		{
			return NULL;
		}
	}
	void SetTouchCapture( tjs_uint32 id, tTJSNI_BaseLayer* layer );
};
//---------------------------------------------------------------------------



#endif
