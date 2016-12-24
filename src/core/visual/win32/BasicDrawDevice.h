
#ifndef BASIC_DRAW_DEVICE_H
#define BASIC_DRAW_DEVICE_H

#include "DrawDevice.h"
//#include <d3d9.h>

//---------------------------------------------------------------------------
//! @brief		「Basic」デバイス(もっとも基本的な描画を行うのみのデバイス)
//---------------------------------------------------------------------------
class tTVPBasicDrawDevice : public tTVPDrawDevice
{
	typedef tTVPDrawDevice inherited;

	void*TargetWindow;
	bool IsMainWindow;
	bool DrawUpdateRectangle;
	bool BackBufferDirty;

	void*				Direct3D;
	void*		Direct3DDevice;
	void*		Texture;
	int	D3dPP;
	int			DispMode;

//	UINT	CurrentMonitor;
 	void*	TextureBuffer; //!< テクスチャのサーフェースへのメモリポインタ
// 	long	TexturePitch; //!< テクスチャのピッチ

	tjs_uint TextureWidth; //!< テクスチャの横幅
	tjs_uint TextureHeight; //!< テクスチャの縦幅

	bool ShouldShow; //!< show で実際に画面に画像を転送すべきか

	tjs_uint VsyncInterval;

public:
	tTVPBasicDrawDevice(); //!< コンストラクタ

private:
	~tTVPBasicDrawDevice(); //!< デストラクタ

	void InvalidateAll();
#if 0
	UINT GetMonitorNumber( HWND window );

	bool IsTargetWindowActive() const;

	bool GetDirect3D9Device();
	HRESULT InitializeDirect3DState();
	HRESULT DecideD3DPresentParameters();

 	bool CreateD3DDevice();
#endif
	void DestroyD3DDevice() {}

// 	bool CreateTexture();
// 	void DestroyTexture();

	void TryRecreateWhenDeviceLost();
//	void ErrorToLog( HRESULT hr );
//	void CheckMonitorMoved();

public:
//	void SetToRecreateDrawer() { DestroyD3DDevice(); }
	enum tDrawerType
	{
		dtNone, //!< drawer なし
		dtDrawDib, //!< もっとも単純なdrawer
		dtDBGDI, // GDI によるダブルバッファリングを行うdrawer
		dtDBDD, // DirectDraw によるダブルバッファリングを行うdrawer
		dtDBD3D // Direct3D によるダブルバッファリングを行うdrawer
	} DrawerType = dtDrawDib, PreferredDrawerType = dtDrawDib;
	tDrawerType GetDrawerType() const { return DrawerType; }
	void SetPreferredDrawerType(tDrawerType type) { PreferredDrawerType = type; }
	tDrawerType GetPreferredDrawerType() const { return PreferredDrawerType; }

public:
//	void EnsureDevice();

//---- LayerManager の管理関連
	virtual void TJS_INTF_METHOD AddLayerManager(iTVPLayerManager * manager);

//---- 描画位置・サイズ関連
//	virtual void TJS_INTF_METHOD SetTargetWindow(HWND wnd, bool is_main);
	virtual void TJS_INTF_METHOD SetDestRectangle(const tTVPRect & rect);
	virtual void TJS_INTF_METHOD NotifyLayerResize(iTVPLayerManager * manager);

//---- 再描画関連
	virtual void TJS_INTF_METHOD Show();
//	virtual bool TJS_INTF_METHOD WaitForVBlank( tjs_int* in_vblank, tjs_int* delayed );

//---- LayerManager からの画像受け渡し関連
	virtual void TJS_INTF_METHOD StartBitmapCompletion(iTVPLayerManager * manager);
	virtual void TJS_INTF_METHOD NotifyBitmapCompleted(iTVPLayerManager * manager,
		tjs_int x, tjs_int y, tTVPBaseTexture * bmp,
		const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity);
	virtual void TJS_INTF_METHOD EndBitmapCompletion(iTVPLayerManager * manager);

//---- デバッグ支援
	virtual void TJS_INTF_METHOD SetShowUpdateRect(bool b);
#if 0
//---- フルスクリーン
	virtual bool TJS_INTF_METHOD SwitchToFullScreen( HWND window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color, bool changeresolution );
	virtual void TJS_INTF_METHOD RevertFromFullScreen( HWND window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color );
#endif
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNI_BasicDrawDevice
//---------------------------------------------------------------------------
class tTJSNI_BasicDrawDevice :
	public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;

	tTVPBasicDrawDevice * Device;

public:
	tTJSNI_BasicDrawDevice();
	~tTJSNI_BasicDrawDevice();
	tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

public:
	tTVPBasicDrawDevice * GetDevice() const { return Device; }

};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_BasicDrawDevice
//---------------------------------------------------------------------------
class tTJSNC_BasicDrawDevice : public tTJSNativeClass
{
public:
	tTJSNC_BasicDrawDevice();

	static tjs_uint32 ClassID;

private:
	iTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------


#endif // BASIC_DRAW_DEVICE_H