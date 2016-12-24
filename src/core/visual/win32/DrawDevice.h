//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//!@file 描画デバイス管理
//---------------------------------------------------------------------------
#ifndef DRAWDEVICE_H
#define DRAWDEVICE_H

#include "LayerIntf.h"
#include "LayerManager.h"
#include "ComplexRect.h"

class iTVPWindow;
class tTJSNI_BaseLayer;

/*[*/
//---------------------------------------------------------------------------
//! @brief		描画デバイスインターフェース
//---------------------------------------------------------------------------
class iTVPDrawDevice
{
public:
//---- オブジェクト生存期間制御
	//! @brief		(Window→DrawDevice) 描画デバイスを破棄する
	//! @note		ウィンドウが破棄されるとき、あるいはほかの描画デバイスが
	//!				設定されたためにこの描画デバイスが必要なくなった際に呼ばれる。
	//!				通常、ここでは delete this を実行し、描画デバイスを破棄するが、その前に
	//!				AddLayerManager() でこの描画デバイスの管理下に入っている
	//!				レイヤマネージャをすべて Release する。
	//!				レイヤマネージャの Release 中に RemoveLayerManager() が呼ばれる
	//!				可能性があることに注意すること。
	virtual void TJS_INTF_METHOD Destruct() = 0;

//---- window interface 関連
	//! @brief		(Window→DrawDevice) ウィンドウインターフェースを設定する
	//! @param		window		ウィンドウインターフェース
	//! @note		(TJSから) Window.drawDevice プロパティを設定した直後に呼ばれる。
	virtual void TJS_INTF_METHOD SetWindowInterface(iTVPWindow * window) = 0;

//---- LayerManager の管理関連
	//! @brief		(Window→DrawDevice) レイヤマネージャを追加する
	//! @note		プライマリレイヤがウィンドウに追加されると、自動的にレイヤマネージャが
	//!				作成され、それが描画デバイスにもこのメソッドの呼び出しにて通知される。
	//!				描画デバイスでは iTVPLayerManager::AddRef() を呼び出して、追加された
	//!				レイヤマネージャをロックすること。
	virtual void TJS_INTF_METHOD AddLayerManager(iTVPLayerManager * manager) = 0;

	//! @brief		(Window→DrawDevice) レイヤマネージャを削除する
	//! @note		プライマリレイヤが invalidate される際に呼び出される。
	//TODO: プライマリレイヤ無効化、あるいはウィンドウ破棄時の終了処理が正しいか？
	virtual void TJS_INTF_METHOD RemoveLayerManager(iTVPLayerManager * manager) = 0;

//---- 描画位置・サイズ関連
	//! @brief		(Window→DrawDevice) 描画先ウィンドウの設定
	//! @param		wnd		ウィンドウハンドル
	//! @param		is_main	メインウィンドウの場合に真
	//! @note		ウィンドウから描画先となるウィンドウハンドルを指定するために呼ばれる。
	//!				しばしば、Window.borderStyle プロパティが変更されたり、フルスクリーンに
	//!				移行するときやフルスクリーンから戻る時など、ウィンドウが再作成される
	//!				ことがあるが、そのような場合には、ウィンドウがいったん破棄される直前に
	//!				wnd = NULL の状態でこのメソッドが呼ばれることに注意。ウィンドウが作成
	//!				されたあと、再び有効なウィンドウハンドルを伴ってこのメソッドが呼ばれる。
	//!				このメソッドは、ウィンドウが作成された直後に呼ばれる保証はない。
	//!				たいてい、一番最初にウィンドウが表示された直後に呼ばれる。
//	virtual void TJS_INTF_METHOD SetTargetWindow(int wnd, bool is_main) = 0;

	//! @brief		(Window->DrawDevice) 描画矩形の設定
	//! @note		ウィンドウから、描画先となる矩形を設定するために呼ばれる。
	//!				描画デバイスは、SetTargetWindow() で指定されたウィンドウのクライアント領域の、
	//!				このメソッドで指定された矩形に表示を行う必要がある。
	//!				この矩形は、GetSrcSize で返した値に対し、Window.zoomNumer や Window.zoomDenum
	//!				プロパティによる拡大率や、Window.layerLeft や Window.layerTop が加味された
	//!				矩形である。
	//!				このメソッドによって描画矩形が変わったとしても、このタイミングで
	//!				描画デバイス側で再描画を行う必要はない(必要があれば別メソッドにより
	//!				再描画の必要性が通知されるため)。
	virtual void TJS_INTF_METHOD SetDestRectangle(const tTVPRect & rect) = 0;

	virtual void TJS_INTF_METHOD SetWindowSize(tjs_int w, tjs_int h) = 0;
	//! @brief		(Window->DrawDevice) クリッピング矩形の設定
	//! @note		ウィンドウから、描画先をクリッピングするための矩形を設定するために呼ばれる。
	//!				描画デバイスは、SetDestRectangleで指定された領域を、このメソッドで指定された矩形
	//!				でクリッピングを行い表示を行う必要がある。
	//!				このメソッドによって描画領域が変わったとしても、このタイミングで
	//!				描画デバイス側で再描画を行う必要はない(必要があれば別メソッドにより
	//!				再描画の必要性が通知されるため)。
	virtual void TJS_INTF_METHOD SetClipRectangle(const tTVPRect & rect) = 0;

	//! @brief		(Window->DrawDevice) 元画像のサイズを得る
	//! @note		ウィンドウから、描画矩形のサイズを決定するために元画像のサイズが
	//!				必要になった際に呼ばれる。ウィンドウはこれをもとに SetDestRectangle()
	//!				メソッドで描画矩形を通知してくるだけなので、
	//!				なんらかの意味のあるサイズである必要は必ずしもない。
	virtual void TJS_INTF_METHOD GetSrcSize(tjs_int &w, tjs_int &h) = 0;

	//! @brief		(LayerManager→DrawDevice) レイヤサイズ変更の通知
	//! @param		manager		レイヤマネージャ
	//! @note		レイヤマネージャにアタッチされているプライマリレイヤのサイズが変わった
	//!				際に呼び出される
	virtual void TJS_INTF_METHOD NotifyLayerResize(iTVPLayerManager * manager) = 0;

	//! @brief		(LayerManager→DrawDevice) レイヤの画像の変更の通知
	//! @param		manager		レイヤマネージャ
	//! @note		レイヤの画像に変化があった際に呼び出される。
	//!				この通知を受け取った後に iTVPLayerManager::UpdateToDrawDevice()
	//!				を呼び出せば、該当部分を描画デバイスに対して描画させることができる。
	//!				この通知を受け取っても無視することは可能。その場合は、
	//!				次に iTVPLayerManager::UpdateToDrawDevice() を呼んだ際に、
	//!				それまでの変更分がすべて描画される。
	virtual void TJS_INTF_METHOD NotifyLayerImageChange(iTVPLayerManager * manager) = 0;

//---- ユーザーインターフェース関連
	//! @brief		(Window→DrawDevice) クリックされた
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	virtual void TJS_INTF_METHOD OnClick(tjs_int x, tjs_int y) = 0;

	//! @brief		(Window→DrawDevice) ダブルクリックされた
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	virtual void TJS_INTF_METHOD OnDoubleClick(tjs_int x, tjs_int y) = 0;

	//! @brief		(Window→DrawDevice) マウスボタンが押下された
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		mb		どのマウスボタンか
	//! @param		flags	フラグ(TVP_SS_*定数の組み合わせ)
	virtual void TJS_INTF_METHOD OnMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags) = 0;

	//! @brief		(Window→DrawDevice) マウスボタンが離された
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		mb		どのマウスボタンか
	//! @param		flags	フラグ(TVP_SS_*定数の組み合わせ)
	virtual void TJS_INTF_METHOD OnMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags) = 0;

	//! @brief		(Window→DrawDevice) マウスが移動した
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		flags	フラグ(TVP_SS_*定数の組み合わせ)
	virtual void TJS_INTF_METHOD OnMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags) = 0;

	//! @brief		(Window→DrawDevice) レイヤのマウスキャプチャを解放する
	//! @note		レイヤのマウスキャプチャを解放すべき場合にウィンドウから呼ばれる。
	//! @note		WindowReleaseCapture() と混同しないこと。
	virtual void TJS_INTF_METHOD OnReleaseCapture() = 0;

	//! @brief		(Window→DrawDevice) マウスが描画矩形外に移動した
	virtual void TJS_INTF_METHOD OnMouseOutOfWindow() = 0;

	//! @brief		(Window→DrawDevice) キーが押された
	//! @param		key		仮想キーコード
	//! @param		shift	シフトキーの状態
	virtual void TJS_INTF_METHOD OnKeyDown(tjs_uint key, tjs_uint32 shift) = 0;

	//! @brief		(Window→DrawDevice) キーが離された
	//! @param		key		仮想キーコード
	//! @param		shift	シフトキーの状態
	virtual void TJS_INTF_METHOD OnKeyUp(tjs_uint key, tjs_uint32 shift) = 0;

	//! @brief		(Window→DrawDevice) キーによる入力
	//! @param		key		文字コード
	virtual void TJS_INTF_METHOD OnKeyPress(tjs_char key) = 0;

	//! @brief		(Window→DrawDevice) マウスホイールが回転した
	//! @param		shift	シフトキーの状態
	//! @param		delta	回転角
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	virtual void TJS_INTF_METHOD OnMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x, tjs_int y) = 0;

	//! @brief		(Window→DrawDevice) 画面がタッチされた
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		id		タッチ識別用ID
	virtual void TJS_INTF_METHOD OnTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) = 0;

	//! @brief		(Window→DrawDevice) タッチが離された
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		id		タッチ識別用ID
	virtual void TJS_INTF_METHOD OnTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) = 0;

	//! @brief		(Window→DrawDevice) タッチが移動した
	//! @param		x		描画矩形内における x 位置(描画矩形の左上が原点)
	//! @param		y		描画矩形内における y 位置(描画矩形の左上が原点)
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		id		タッチ識別用ID
	virtual void TJS_INTF_METHOD OnTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) = 0;

	//! @brief		(Window→DrawDevice) 拡大タッチ操作が行われた
	//! @param		startdist	開始時の2点間の幅
	//! @param		curdist	現在の2点間の幅
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		flag	タッチ状態フラグ
	virtual void TJS_INTF_METHOD OnTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag ) = 0;

	//! @brief		(Window→DrawDevice) 回転タッチ操作が行われた
	//! @param		startangle	開始時の角度
	//! @param		curangle	現在の角度
	//! @param		dist	現在の2点間の幅
	//! @param		cx		触れている幅
	//! @param		cy		触れている高さ
	//! @param		flag	タッチ状態フラグ
	virtual void TJS_INTF_METHOD OnTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag ) = 0;

	//! @brief		(Window→DrawDevice) マルチタッチ状態が更新された
	virtual void TJS_INTF_METHOD OnMultiTouch() = 0;

	//! @brief		(Window->DrawDevice) 画面の回転が行われた
	//! @param		orientation	画面の向き ( 横向き、縦向き、不明 )
	//! @param		rotate		回転角度。Degree。負の値の時不明
	//! @param		bpp			Bits per pixel
	//! @param		width		画面幅
	//! @param		height		画面高さ
	virtual void TJS_INTF_METHOD OnDisplayRotate( tjs_int orientation, tjs_int rotate, tjs_int bpp, tjs_int width, tjs_int height ) = 0;

	//! @brief		(Window->DrawDevice) 入力状態のチェック
	//! @note		ウィンドウから約1秒おきに、レイヤマネージャがユーザからの入力の状態を
	//!				再チェックするために呼ばれる。レイヤ状態の変化がユーザの入力とは
	//!				非同期に行われた場合、たとえばマウスカーソルの下にレイヤが出現した
	//!				のにもかかわらず、マウスカーソルがそのレイヤの指定する形状に変更されない
	//!				といった状況が発生しうる。このような状況に対処するため、ウィンドウから
	//!				このメソッドが約1秒おきに呼ばれる。
	virtual void TJS_INTF_METHOD RecheckInputState() = 0;

	//! @brief		(LayerManager→DrawDevice) マウスカーソルの形状をデフォルトに戻す
	//! @param		manager		レイヤマネージャ
	//! @note		マウスカーソルの形状をデフォルトの物に戻したい場合に呼ばれる
	virtual void TJS_INTF_METHOD SetDefaultMouseCursor(iTVPLayerManager * manager) = 0;

	//! @brief		(LayerManager→DrawDevice) マウスカーソルの形状を設定する
	//! @param		manager		レイヤマネージャ
	//! @param		cursor		マウスカーソル形状番号
	virtual void TJS_INTF_METHOD SetMouseCursor(iTVPLayerManager * manager, tjs_int cursor) = 0;

	//! @brief		(LayerManager→DrawDevice) マウスカーソルの位置を取得する
	//! @param		manager		レイヤマネージャ
	//! @param		x			プライマリレイヤ上の座標におけるマウスカーソルのx位置
	//! @param		y			プライマリレイヤ上の座標におけるマウスカーソルのy位置
	//! @note		座標はプライマリレイヤ上の座標なので、必要ならば変換を行う
	virtual void TJS_INTF_METHOD GetCursorPos(iTVPLayerManager * manager, tjs_int &x, tjs_int &y) = 0;

	//! @brief		(LayerManager→DrawDevice) マウスカーソルの位置を設定する
	//! @param		manager		レイヤマネージャ
	//! @param		x			プライマリレイヤ上の座標におけるマウスカーソルのx位置
	//! @param		y			プライマリレイヤ上の座標におけるマウスカーソルのy位置
	//! @note		座標はプライマリレイヤ上の座標なので、必要ならば変換を行う
	virtual void TJS_INTF_METHOD SetCursorPos(iTVPLayerManager * manager, tjs_int x, tjs_int y) = 0;

	//! @brief		(LayerManager→DrawDevice) ウィンドウのマウスキャプチャを解放する
	//! @param		manager		レイヤマネージャ
	//! @note		ウィンドウのマウスキャプチャを解放すべき場合にレイヤマネージャから呼ばれる。
	//! @note		ウィンドウのマウスキャプチャは OnReleaseCapture() で開放できるレイヤのマウスキャプチャ
	//!				と異なることに注意。ウィンドウのマウスキャプチャは主にOSのウィンドウシステムの
	//!				機能であるが、レイヤのマウスキャプチャは吉里吉里がレイヤマネージャごとに
	//!				独自に管理している物である。このメソッドでは基本的には ::ReleaseCapture() などで
	//!				マウスのキャプチャを開放する。
	virtual void TJS_INTF_METHOD WindowReleaseCapture(iTVPLayerManager * manager) = 0;

	//! @brief		(LayerManager→DrawDevice) ツールチップヒントを設定する
	//! @param		manager		レイヤマネージャ
	//! @param		text		ヒントテキスト(空文字列の場合はヒントの表示をキャンセルする)
	virtual void TJS_INTF_METHOD SetHintText(iTVPLayerManager * manager, iTJSDispatch2* sender, const ttstr & text) = 0;

	//! @brief		(LayerManager→DrawDevice) 注視ポイントの設定
	//! @param		manager		レイヤマネージャ
	//! @param		layer		フォント情報の含まれるレイヤ
	//! @param		x			プライマリレイヤ上の座標における注視ポイントのx位置
	//! @param		y			プライマリレイヤ上の座標における注視ポイントのy位置
	//! @note		注視ポイントは通常キャレット位置のことで、そこにIMEのコンポジット・ウィンドウが
	//!				表示されたり、ユーザ補助の拡大鏡がそこを拡大したりする。IMEがコンポジットウィンドウを
	//!				表示したり、未確定の文字をそこに表示したりする際のフォントは layer パラメータ
	//!				で示されるレイヤが持つ情報によるが、プラグインからその情報を得たり設定したり
	//!				するインターフェースは今のところない。
	//! @note		座標はプライマリレイヤ上の座標なので、必要ならば変換を行う。
	virtual void TJS_INTF_METHOD SetAttentionPoint(iTVPLayerManager * manager, tTJSNI_BaseLayer *layer,
							tjs_int l, tjs_int t) = 0;

	//! @brief		(LayerManager→DrawDevice) 注視ポイントの解除
	//! @param		manager		レイヤマネージャ
	virtual void TJS_INTF_METHOD DisableAttentionPoint(iTVPLayerManager * manager) = 0;

	//! @brief		(LayerManager→DrawDevice) IMEモードの設定
	//! @param		manager		レイヤマネージャ
	//! @param		mode		IMEモード
	virtual void TJS_INTF_METHOD SetImeMode(iTVPLayerManager * manager, tTVPImeMode mode) = 0;

	//! @brief		(LayerManager→DrawDevice) IMEモードのリセット
	//! @param		manager		レイヤマネージャ
	virtual void TJS_INTF_METHOD ResetImeMode(iTVPLayerManager * manager) = 0;

//---- プライマリレイヤ関連
	//! @brief		(Window→DrawDevice) プライマリレイヤの取得
	//! @return		プライマリレイヤ
	//! @note		Window.primaryLayer が読み出された際にこのメソッドが呼ばれる。
	//!				それ以外に呼ばれることはない。
	virtual tTJSNI_BaseLayer * TJS_INTF_METHOD GetPrimaryLayer() = 0;

	//! @brief		(Window→DrawDevice) フォーカスのあるレイヤの取得
	//! @return		フォーカスのあるレイヤ(NULL=フォーカスのあるレイヤがない場合)
	//! @note		Window.focusedLayer が読み出された際にこのメソッドが呼ばれる。
	//!				それ以外に呼ばれることはない。
	virtual tTJSNI_BaseLayer * TJS_INTF_METHOD GetFocusedLayer() = 0;

	//! @brief		(Window→DrawDevice) フォーカスのあるレイヤの設定
	//! @param		layer		フォーカスのあるレイヤ(NULL=フォーカスのあるレイヤがない状態にしたい場合)
	//! @note		Window.focusedLayer が書き込まれた際にこのメソッドが呼ばれる。
	//!				それ以外に呼ばれることはない。
	virtual void TJS_INTF_METHOD SetFocusedLayer(tTJSNI_BaseLayer * layer) = 0;


//---- 再描画関連
	//! @brief		(Window→DrawDevice) 描画矩形の無効化の通知
	//! @param		rect		描画矩形内の座標における、無効になった領域
	//!							(描画矩形の左上が原点)
	//! @note		描画矩形の一部あるいは全部が無効になった際にウィンドウから通知される。
	//!				描画デバイスは、なるべく早い時期に無効になった部分を再描画すべきである。
	virtual void TJS_INTF_METHOD RequestInvalidation(const tTVPRect & rect) = 0;

	//! @brief		(Window→DrawDevice) 更新の要求
	//! @note		描画矩形の内容を最新の状態に更新すべきタイミングで、ウィンドウから呼ばれる。
	//!				iTVPWindow::RequestUpdate() を呼んだ後、システムが描画タイミングに入った際に
	//!				呼ばれる。通常、描画デバイスはこのタイミングを利用してオフスクリーン
	//!				サーフェースに画像を描画する。
	virtual void TJS_INTF_METHOD Update() = 0;

	//! @brief		(Window->DrawDevice) 画像の表示
	//! @note		オフスクリーンサーフェースに描画された画像を、オンスクリーンに表示する
	//!				(あるいはフリップする) タイミングで呼ばれる。通常は Update の直後に
	//!				呼ばれるが、VSync 待ちが有効になっている場合は Update 直後ではなく、
	//!				VBlank 中に呼ばれる可能性がある。オフスクリーンサーフェースを
	//!				使わない場合は無視してかまわない。
	virtual void TJS_INTF_METHOD Show() = 0;

//---- LayerManager からの画像受け渡し関連
	//! @brief		(LayerManager->DrawDevice) ビットマップの描画を開始する
	//! @param		manager		描画を開始するレイヤマネージャ
	//! @note		レイヤマネージャから描画デバイスへ画像が転送される前に呼ばれる。
	//!				このあと、NotifyBitmapCompleted() が任意の回数呼ばれ、最後に
	//!				EndBitmapCompletion() が呼ばれる。
	//!				必要ならば、このタイミングで描画デバイス側でサーフェースのロックなどを
	//!				行うこと。
	virtual void TJS_INTF_METHOD StartBitmapCompletion(iTVPLayerManager * manager) = 0;

	//! @brief		(LayerManager->DrawDevice) ビットマップの描画を通知する
	//! @param		manager		画像の提供元のレイヤマネージャ
	//! @param		x			プライマリレイヤ上の座標における画像の左端位置
	//! @param		y			プライマリレイヤ上の座標における画像の上端位置
	//! @param		bits		ビットマップデータ
	//! @param		bitmapinfo	ビットマップの形式情報
	//! @param		cliprect	bits のうち、どの部分を使って欲しいかの情報
	//! @param		type		提供される画像が想定する合成モード
	//! @param		opacity		提供される画像が想定する不透明度(0～255)
	//! @note		レイヤマネージャが合成を完了し、結果を描画デバイスに描画してもらいたい際に
	//!				呼ばれる。一つの更新が複数の矩形で構成される場合があるため、このメソッドは
	//!				StartBitmapCompletion() と EndBitmapCompletion() の間に複数回呼ばれる可能性がある。
	//!				基本的には、bits と bitmapinfo で表されるビットマップのうち、cliprect で
	//!				示される矩形を x, y 位置に転送すればよいが、描画矩形の大きさに合わせた
	//!				拡大や縮小などは描画デバイス側で面倒を見る必要がある。
	virtual void TJS_INTF_METHOD NotifyBitmapCompleted(iTVPLayerManager * manager,
		tjs_int x, tjs_int y, tTVPBaseTexture * bmp,
		const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity) = 0;

	//! @brief		(LayerManager->DrawDevice) ビットマップの描画を終了する
	//! @param		manager		描画を終了するレイヤマネージャ
	virtual void TJS_INTF_METHOD EndBitmapCompletion(iTVPLayerManager * manager) = 0;

//---- デバッグ支援
	//! @brief		(Window->DrawDevice) レイヤ構造をコンソールにダンプする
	virtual void TJS_INTF_METHOD DumpLayerStructure() = 0;

	//! @brief		(Window->DrawDevice) 更新矩形の表示を行うかどうかを設定する
	//! @param		b		表示を行うかどうか
	//! @note		レイヤ表示機構が差分更新を行う際の矩形を表示し、
	//!				差分更新の最適化に役立てるための支援機能。
	//!				実装する必要はないが、実装することが望ましい。
	virtual void TJS_INTF_METHOD SetShowUpdateRect(bool b) = 0;
    virtual void Clear() {}

	//! @brief		(Window->DrawDevice) フルスクリーン化する
	//! @param		window		ウィンドウハンドル
	//! @param		w			要求する幅
	//! @param		h			要求する高さ
	//! @param		bpp			Bit per pixels
	//! @param		color		16bpp の時 565 か 555を指定
	//! @param		changeresolution	解像度変更を行うかどうか
	virtual bool TJS_INTF_METHOD SwitchToFullScreen( int window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color, bool changeresolution ) = 0;
	
	//! @brief		(Window->DrawDevice) フルスクリーンを解除する
	//! @param		window		ウィンドウハンドル
	//! @param		w			要求する幅
	//! @param		h			要求する高さ
	//! @param		bpp			元々のBit per pixels
	//! @param		color		16bpp の時 565 か 555を指定
	virtual void TJS_INTF_METHOD RevertFromFullScreen(int window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color) = 0;

	//! @brief		(Window->DrawDevice) VBlank待ちを行う
	//! @param		in_vblank	待たなくてもVBlank内だったかどうかを返す( !0 : 内、0: 外 )
	//! @param		delayed		1フレーム遅延が発生したかどうかを返す( !0 : 発生、0: 発生せず )
	//! @return		Wait可不可 true : 可能、false : 不可
	virtual bool TJS_INTF_METHOD WaitForVBlank( tjs_int* in_vblank, tjs_int* delayed ) = 0;
};
//---------------------------------------------------------------------------
/*]*/

//---------------------------------------------------------------------------
//! @brief		描画デバイスインターフェースの基本的な実装
//---------------------------------------------------------------------------
class tTVPDrawDevice : public iTVPDrawDevice
{
protected:
	iTVPWindow * Window;
	size_t PrimaryLayerManagerIndex; //!< プライマリレイヤマネージャ
	std::vector<iTVPLayerManager *> Managers; //!< レイヤマネージャの配列
	tTVPRect DestRect; //!< 描画先位置
    tjs_int SrcWidth, SrcHeight;
    tjs_int WinWidth, WinHeight;
	tjs_int LockedWidth = 0, LockedHeight = 0;
	tTVPRect ClipRect; //!< クリッピング矩形

protected:
	tTVPDrawDevice(); //!< コンストラクタ
protected:
	virtual ~tTVPDrawDevice(); //!< デストラクタ

public:
	//! @brief		指定位置にあるレイヤマネージャを得る
	//! @param		index		インデックス(0～)
	//! @return		指定位置にあるレイヤマネージャ(AddRefされないので注意)。
	//!				指定位置にレイヤマネージャがなければNULLが返る
	iTVPLayerManager * GetLayerManagerAt(size_t index)
	{
		if(Managers.size() <= index) return NULL;
		return Managers[index];
	}

	//! @brief		Device→LayerManager方向の座標の変換を行う
	//! @param		x		X位置
	//! @param		y		Y位置
	//! @return		変換に成功すれば真。さもなければ偽。PrimaryLayerManagerIndexに該当する
	//!				レイヤマネージャがなければ偽が返る
	//! @note		x, y は DestRectの (0,0) を原点とする座標として渡されると見なす
	bool TransformToPrimaryLayerManager(tjs_int &x, tjs_int &y);
	bool TransformToPrimaryLayerManager(tjs_real &x, tjs_real &y);

	//! @brief		LayerManager→Device方向の座標の変換を行う
	//! @param		x		X位置
	//! @param		y		Y位置
	//! @return		変換に成功すれば真。さもなければ偽。PrimaryLayerManagerIndexに該当する
	//!				レイヤマネージャがなければ偽が返る
	//! @note		x, y は レイヤの (0,0) を原点とする座標として渡されると見なす
	bool TransformFromPrimaryLayerManager(tjs_int &x, tjs_int &y);

//---- オブジェクト生存期間制御
	virtual void TJS_INTF_METHOD Destruct();

//---- window interface 関連
	virtual void TJS_INTF_METHOD SetWindowInterface(iTVPWindow * window);

//---- LayerManager の管理関連
	virtual void TJS_INTF_METHOD AddLayerManager(iTVPLayerManager * manager);
	virtual void TJS_INTF_METHOD RemoveLayerManager(iTVPLayerManager * manager);

//---- 描画位置・サイズ関連
	virtual void TJS_INTF_METHOD SetDestRectangle(const tTVPRect & rect);
    virtual void TJS_INTF_METHOD SetWindowSize(tjs_int w, tjs_int h);
	virtual void TJS_INTF_METHOD SetClipRectangle(const tTVPRect & rect);
	virtual void TJS_INTF_METHOD GetSrcSize(tjs_int &w, tjs_int &h);
	virtual void TJS_INTF_METHOD NotifyLayerResize(iTVPLayerManager * manager);
	virtual void TJS_INTF_METHOD NotifyLayerImageChange(iTVPLayerManager * manager);

//---- ユーザーインターフェース関連
	// window → drawdevice
	virtual void TJS_INTF_METHOD OnClick(tjs_int x, tjs_int y);
	virtual void TJS_INTF_METHOD OnDoubleClick(tjs_int x, tjs_int y);
	virtual void TJS_INTF_METHOD OnMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags);
	virtual void TJS_INTF_METHOD OnMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags);
	virtual void TJS_INTF_METHOD OnMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags);
	virtual void TJS_INTF_METHOD OnReleaseCapture();
	virtual void TJS_INTF_METHOD OnMouseOutOfWindow();
	virtual void TJS_INTF_METHOD OnKeyDown(tjs_uint key, tjs_uint32 shift);
	virtual void TJS_INTF_METHOD OnKeyUp(tjs_uint key, tjs_uint32 shift);
	virtual void TJS_INTF_METHOD OnKeyPress(tjs_char key);
	virtual void TJS_INTF_METHOD OnMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x, tjs_int y);
	virtual void TJS_INTF_METHOD OnTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	virtual void TJS_INTF_METHOD OnTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	virtual void TJS_INTF_METHOD OnTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	virtual void TJS_INTF_METHOD OnTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag );
	virtual void TJS_INTF_METHOD OnTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag );
	virtual void TJS_INTF_METHOD OnMultiTouch();
	virtual void TJS_INTF_METHOD OnDisplayRotate( tjs_int orientation, tjs_int rotate, tjs_int bpp, tjs_int width, tjs_int height );
	virtual void TJS_INTF_METHOD RecheckInputState();

	// layer manager → drawdevice
	virtual void TJS_INTF_METHOD SetDefaultMouseCursor(iTVPLayerManager * manager);
	virtual void TJS_INTF_METHOD SetMouseCursor(iTVPLayerManager * manager, tjs_int cursor);
	virtual void TJS_INTF_METHOD GetCursorPos(iTVPLayerManager * manager, tjs_int &x, tjs_int &y);
	virtual void TJS_INTF_METHOD SetCursorPos(iTVPLayerManager * manager, tjs_int x, tjs_int y);
	virtual void TJS_INTF_METHOD SetHintText(iTVPLayerManager * manager, iTJSDispatch2* sender, const ttstr & text);
	virtual void TJS_INTF_METHOD WindowReleaseCapture(iTVPLayerManager * manager);

	virtual void TJS_INTF_METHOD SetAttentionPoint(iTVPLayerManager * manager, tTJSNI_BaseLayer *layer,
							tjs_int l, tjs_int t);
	virtual void TJS_INTF_METHOD DisableAttentionPoint(iTVPLayerManager * manager);
	virtual void TJS_INTF_METHOD SetImeMode(iTVPLayerManager * manager, tTVPImeMode mode);
	virtual void TJS_INTF_METHOD ResetImeMode(iTVPLayerManager * manager);

//---- プライマリレイヤ関連
	virtual tTJSNI_BaseLayer * TJS_INTF_METHOD GetPrimaryLayer();
	virtual tTJSNI_BaseLayer * TJS_INTF_METHOD GetFocusedLayer();
	virtual void TJS_INTF_METHOD SetFocusedLayer(tTJSNI_BaseLayer * layer);

//---- 再描画関連
	virtual void TJS_INTF_METHOD RequestInvalidation(const tTVPRect & rect);
	virtual void TJS_INTF_METHOD Update();
	virtual void TJS_INTF_METHOD Show() = 0;
	virtual bool TJS_INTF_METHOD WaitForVBlank( tjs_int* in_vblank, tjs_int* delayed );

//---- デバッグ支援
	virtual void TJS_INTF_METHOD DumpLayerStructure();
	virtual void TJS_INTF_METHOD SetShowUpdateRect(bool b);

	void SetLockedSize(tjs_int w, tjs_int h);
//---- フルスクリーン
	virtual bool TJS_INTF_METHOD SwitchToFullScreen(int window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color, bool changeresolution);
	virtual void TJS_INTF_METHOD RevertFromFullScreen(int window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color);

// ほかのメソッドについては実装しない
};
//---------------------------------------------------------------------------
#endif
