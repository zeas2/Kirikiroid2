//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//!@file "PassThrough" 描画デバイス管理
//---------------------------------------------------------------------------
#ifndef PASSTHROUGHDRAWDEVICE_H
#define PASSTHROUGHDRAWDEVICE_H

#include "DrawDevice.h"

class tTVPDrawer;
//---------------------------------------------------------------------------
//! @brief		「Pass Through」デバイス(もっとも基本的な描画を行うのみのデバイス)
//---------------------------------------------------------------------------
class tTVPPassThroughDrawDevice : public tTVPDrawDevice
{
	typedef tTVPDrawDevice inherited;
	//SDL_Window *TargetWindow;
	//bool IsMainWindow;
	tTVPDrawer * Drawer; //!< 描画を行うもの

public:
	//! @brief	drawerのタイプ
	enum tDrawerType
	{
		dtNone, //!< drawer なし
		dtDrawDib, //!< もっとも単純なdrawer
		dtDBGDI, // GDI によるダブルバッファリングを行うdrawer
		dtDBDD, // DirectDraw によるダブルバッファリングを行うdrawer
		dtDBD3D // Direct3D によるダブルバッファリングを行うdrawer
	};

private:
	tDrawerType DrawerType; //!< drawer のタイプ
	tDrawerType PreferredDrawerType; //!< 使って欲しい drawer のタイプ

	bool DestSizeChanged; //!< DestRect のサイズに変更があったか
	bool SrcSizeChanged; //!< SrcSize に変更があったか

public:
	tTVPPassThroughDrawDevice(); //!< コンストラクタ
private:
	~tTVPPassThroughDrawDevice(); //!< デストラクタ

public:
	void SetToRecreateDrawer() { DestroyDrawer(); }
	void DestroyDrawer();
private:
	void CreateDrawer(tDrawerType type);
	void CreateDrawer(bool zoom_required, bool should_benchmark);

public:
	void EnsureDrawer();

	tDrawerType GetDrawerType() const { return DrawerType; }
	void SetPreferredDrawerType(tDrawerType type) { PreferredDrawerType = type; }
	tDrawerType GetPreferredDrawerType() const { return PreferredDrawerType; }

//---- LayerManager の管理関連
	virtual void TJS_INTF_METHOD AddLayerManager(iTVPLayerManager * manager);

//---- 描画位置・サイズ関連
	virtual void TJS_INTF_METHOD SetTargetWindow(int wnd, bool is_main);
	virtual void TJS_INTF_METHOD SetDestRectangle(const tTVPRect & rect);
	virtual void TJS_INTF_METHOD NotifyLayerResize(iTVPLayerManager * manager);

//---- 再描画関連
	virtual void TJS_INTF_METHOD Show();

//---- LayerManager からの画像受け渡し関連
	virtual void TJS_INTF_METHOD StartBitmapCompletion(iTVPLayerManager * manager);
	virtual void TJS_INTF_METHOD NotifyBitmapCompleted(iTVPLayerManager * manager,
		tjs_int x, tjs_int y, tTVPBaseTexture * bmp,
		const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity);
	virtual void TJS_INTF_METHOD EndBitmapCompletion(iTVPLayerManager * manager);

//---- デバッグ支援
	virtual void TJS_INTF_METHOD SetShowUpdateRect(bool b);
    virtual void Clear();
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNI_PassThroughDrawDevice
//---------------------------------------------------------------------------
class tTJSNI_PassThroughDrawDevice :
	public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;

	tTVPPassThroughDrawDevice * Device;

public:
	tTJSNI_PassThroughDrawDevice();
	~tTJSNI_PassThroughDrawDevice();
	tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

public:
	tTVPPassThroughDrawDevice * GetDevice() const { return Device; }

};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_PassThroughDrawDevice
//---------------------------------------------------------------------------
class tTJSNC_PassThroughDrawDevice : public tTJSNativeClass
{
public:
	tTJSNC_PassThroughDrawDevice();

	static tjs_uint32 ClassID;

private:
	iTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------


#endif
