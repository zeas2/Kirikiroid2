
#define NOMINMAX
#include "tjsCommHead.h"
#include "DrawDevice.h"
#include "BasicDrawDevice.h"
#include "LayerIntf.h"
#include "MsgIntf.h"
#include "SysInitIntf.h"
#include "WindowIntf.h"
#include "DebugIntf.h"
#include "ThreadIntf.h"
#include "ComplexRect.h"
#include "EventImpl.h"
#include "WindowImpl.h"

#define ZeroMemory(p,n) memset(p, 0, n);
// #include <d3d9.h>
// #include <mmsystem.h>
#include <algorithm>

//---------------------------------------------------------------------------
// オプション
//---------------------------------------------------------------------------
static tjs_int TVPBasicDrawDeviceOptionsGeneration = 0;
bool TVPZoomInterpolation = true;
//---------------------------------------------------------------------------
static void TVPInitBasicDrawDeviceOptions()
{
	if(TVPBasicDrawDeviceOptionsGeneration == TVPGetCommandLineArgumentGeneration()) return;
	TVPBasicDrawDeviceOptionsGeneration = TVPGetCommandLineArgumentGeneration();

	tTJSVariant val;
	TVPZoomInterpolation = true;
	if(TVPGetCommandLine(TJS_W("-smoothzoom"), &val))
	{
		ttstr str(val);
		if(str == TJS_W("no"))
			TVPZoomInterpolation = false;
		else
			TVPZoomInterpolation = true;
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
tTVPBasicDrawDevice::tTVPBasicDrawDevice()
{
	TVPInitBasicDrawDeviceOptions(); // read and initialize options
	TargetWindow = NULL;
	DrawUpdateRectangle = false;
	BackBufferDirty = true;

	Direct3D = NULL;
	Direct3DDevice = NULL;
	Texture = NULL;
	ShouldShow = false;
	TextureBuffer = NULL;
	TextureWidth = TextureHeight = 0;
	VsyncInterval = 16;
 	ZeroMemory( &D3dPP, sizeof(D3dPP) );
 	ZeroMemory( &DispMode, sizeof(DispMode) );
}
//---------------------------------------------------------------------------
tTVPBasicDrawDevice::~tTVPBasicDrawDevice()
{
	DestroyD3DDevice();
}
#if 0
//---------------------------------------------------------------------------
void tTVPBasicDrawDevice::DestroyD3DDevice() {
	DestroyTexture();
	if(Direct3DDevice) Direct3DDevice->Release(), Direct3DDevice = NULL;
	if(Direct3D) Direct3D = NULL;
}
//---------------------------------------------------------------------------
void tTVPBasicDrawDevice::DestroyTexture() {
	if(TextureBuffer && Texture) Texture->UnlockRect(0), TextureBuffer = NULL;
	if(Texture) Texture->Release(), Texture = NULL;
}
#endif
//---------------------------------------------------------------------------
void tTVPBasicDrawDevice::InvalidateAll()
{
	// レイヤ演算結果をすべてリクエストする
	// サーフェースが lost した際に内容を再構築する目的で用いる
	RequestInvalidation(tTVPRect(0, 0, DestRect.get_width(), DestRect.get_height()));
}
#if 0
//---------------------------------------------------------------------------
void tTVPBasicDrawDevice::CheckMonitorMoved() {
	UINT iCurrentMonitor = GetMonitorNumber( TargetWindow );
	if( CurrentMonitor != iCurrentMonitor ) {
		// モニタ移動が発生しているので、デバイスを再生成する
		CreateD3DDevice();
	}
}
//---------------------------------------------------------------------------
bool tTVPBasicDrawDevice::IsTargetWindowActive() const {
 	if( TargetWindow == NULL ) return false;
 	return ::GetForegroundWindow() == TargetWindow;
}
//---------------------------------------------------------------------------
bool tTVPBasicDrawDevice::GetDirect3D9Device() {
	DestroyD3DDevice();

	TVPEnsureDirect3DObject();

	if( NULL == ( Direct3D = TVPGetDirect3DObjectNoAddRef() ) )
		TVPThrowExceptionMessage( TVPFaildToCreateDirect3D );

	HRESULT hr;
	if( FAILED( hr = DecideD3DPresentParameters() ) ) {
		if( IsTargetWindowActive() ) {
			ErrorToLog( hr );
			TVPThrowExceptionMessage( TVPFaildToDecideBackbufferFormat );
		}
		return false;
	}

	UINT iCurrentMonitor = GetMonitorNumber( TargetWindow );
	DWORD	BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;
	if( D3D_OK != ( hr = Direct3D->CreateDevice( iCurrentMonitor, D3DDEVTYPE_HAL, TargetWindow, BehaviorFlags, &D3dPP, &Direct3DDevice ) ) ) {
		if( IsTargetWindowActive() ) {
			ErrorToLog( hr );
			TVPThrowExceptionMessage( TVPFaildToCreateDirect3DDevice );
		}
		return false;
	}
	CurrentMonitor = iCurrentMonitor;
	BackBufferDirty = true;

	/*
	D3DVIEWPORT9 vp;
	vp.X  = DestLeft;
	vp.Y  = DestTop;
	vp.Width = DestWidth != 0 ? DestWidth : D3dPP.BackBufferWidth;
	vp.Height = DestHeight != 0 ? DestHeight : D3dPP.BackBufferHeight;
	*/
	
	D3DVIEWPORT9 vp;
	vp.X  = 0;
	vp.Y  = 0;
	vp.Width = D3dPP.BackBufferWidth;
	vp.Height = D3dPP.BackBufferHeight;
	vp.MinZ  = 0.0f;
	vp.MaxZ  = 1.0f;
	if( FAILED(hr = Direct3DDevice->SetViewport(&vp)) ) {
		if( IsTargetWindowActive() ) {
			ErrorToLog( hr );
			TVPThrowExceptionMessage( TVPFaildToSetViewport );
		}
		return false;
	}

	if( FAILED( hr = InitializeDirect3DState() ) ) {
		if( IsTargetWindowActive() ) {
			ErrorToLog( hr );
 			TVPThrowExceptionMessage( TVPFaildToSetRenderState );
		}
		return false;
	}

	int refreshrate = DispMode.RefreshRate;
	if( refreshrate == 0 ) {
		HDC hdc;
		hdc = ::GetDC(TargetWindow);
		refreshrate = GetDeviceCaps( hdc, VREFRESH );
		::ReleaseDC( TargetWindow, hdc );
	}
	VsyncInterval = 1000 / refreshrate;
	return true;
}
//---------------------------------------------------------------------------
HRESULT tTVPBasicDrawDevice::InitializeDirect3DState() {
	HRESULT	hr;
	D3DCAPS9	d3dcaps;
	if( FAILED( hr = Direct3DDevice->GetDeviceCaps( &d3dcaps ) ) )
		return hr;

	if( d3dcaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR ) {
		if( FAILED( hr = Direct3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, TVPZoomInterpolation?D3DTEXF_LINEAR:D3DTEXF_POINT ) ) )
			return hr;
	} else {
		if( FAILED( hr = Direct3DDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT ) ) )
			return hr;
	}

	if( d3dcaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR ) {
		if( FAILED( hr = Direct3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, TVPZoomInterpolation?D3DTEXF_LINEAR:D3DTEXF_POINT ) ) )
			return hr;
	} else {
		if( FAILED( hr = Direct3DDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT ) ) )
		return hr;
	}

	if( FAILED( hr = Direct3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP ) ) )
		return hr;
	if( FAILED( hr = Direct3DDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP ) ) )
		return hr;
	if( FAILED( hr = Direct3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) ) )
		return hr;
	if( FAILED( hr = Direct3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE ) ) )
		return hr;
	if( FAILED( hr = Direct3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE ) ) )
		return hr;
	if( FAILED( hr = Direct3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE ) ) )
		return hr;
	if( FAILED( hr = Direct3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 ) ) )
		return hr;
	if( FAILED( hr = Direct3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE ) ) )
		return hr;
	if( FAILED( hr = Direct3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE ) ) )
		return hr;

	return S_OK;
}
//---------------------------------------------------------------------------
UINT tTVPBasicDrawDevice::GetMonitorNumber( HWND window )
{
	if( Direct3D == NULL || window == NULL ) return D3DADAPTER_DEFAULT;
	HMONITOR windowMonitor = ::MonitorFromWindow( window, MONITOR_DEFAULTTOPRIMARY );
	UINT iCurrentMonitor = 0;
	UINT numOfMonitor = Direct3D->GetAdapterCount();
	for( ; iCurrentMonitor < numOfMonitor; ++iCurrentMonitor ) 	{
		if( Direct3D->GetAdapterMonitor(iCurrentMonitor) == windowMonitor )
			break;
	}
	if( iCurrentMonitor == numOfMonitor )
		iCurrentMonitor = D3DADAPTER_DEFAULT;
	return iCurrentMonitor;
}
//---------------------------------------------------------------------------
HRESULT tTVPBasicDrawDevice::DecideD3DPresentParameters() {
	HRESULT			hr;
	UINT iCurrentMonitor = GetMonitorNumber(TargetWindow);
	if( FAILED( hr = Direct3D->GetAdapterDisplayMode( iCurrentMonitor, &DispMode ) ) )
		return hr;

	ZeroMemory( &D3dPP, sizeof(D3dPP) );
	D3dPP.Windowed = TRUE;
	D3dPP.SwapEffect = D3DSWAPEFFECT_COPY;
	D3dPP.BackBufferFormat = D3DFMT_UNKNOWN;
	D3dPP.BackBufferHeight = DispMode.Height > (tjs_uint)DestRect.get_height() ? DispMode.Height : (tjs_uint)DestRect.get_height();
	D3dPP.BackBufferWidth = DispMode.Width > (tjs_uint)DestRect.get_width() ? DispMode.Width : (tjs_uint)DestRect.get_width();
	D3dPP.hDeviceWindow = TargetWindow;
	D3dPP.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	return S_OK;
}
//---------------------------------------------------------------------------
bool tTVPBasicDrawDevice::CreateD3DDevice()
{
	// Direct3D デバイス、テクスチャなどを作成する
	DestroyD3DDevice();
	if( TargetWindow ) {
		tjs_int w, h;
		GetSrcSize( w, h );
		if( w > 0 && h > 0 ) {
			// get Direct3D9 interface
			if( GetDirect3D9Device() ) {
				return CreateTexture();
			}
		}
	}
	return false;
}
//---------------------------------------------------------------------------
bool tTVPBasicDrawDevice::CreateTexture() {
	DestroyTexture();
	tjs_int w, h;
	GetSrcSize( w, h );
	if(TargetWindow && w > 0 && h > 0) {
		HRESULT hr = S_OK;

		D3DCAPS9 d3dcaps;
		Direct3DDevice->GetDeviceCaps( &d3dcaps );

		TextureWidth = w;
		TextureHeight = h;
		if( d3dcaps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) {
			// only square textures are supported
			TextureWidth = std::max(TextureHeight, TextureWidth);
			TextureHeight = TextureWidth;
		}

		DWORD dwWidth = 64;
		DWORD dwHeight = 64;
		if( d3dcaps.TextureCaps & D3DPTEXTURECAPS_POW2 ) {
			// 2の累乗のみ許可するかどうか判定
			while( dwWidth < TextureWidth ) dwWidth = dwWidth << 1;
			while( dwHeight < TextureHeight ) dwHeight = dwHeight << 1;
			TextureWidth = dwWidth;
			TextureHeight = dwHeight;

			if( dwWidth > d3dcaps.MaxTextureWidth || dwHeight > d3dcaps.MaxTextureHeight ) {
				TVPAddLog( (const tjs_char*)TVPWarningImageSizeTooLargeMayBeCannotCreateTexture );
			}
			TVPAddLog( (const tjs_char*)TVPUsePowerOfTwoSurface );
		} else {
			dwWidth = TextureWidth;
			dwHeight = TextureHeight;
		}

		if( D3D_OK != ( hr = Direct3DDevice->CreateTexture( dwWidth, dwHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &Texture, NULL) ) ) {
			if( IsTargetWindowActive() ) {
				ErrorToLog( hr );
				TVPThrowExceptionMessage(TVPCannotAllocateD3DOffScreenSurface,TJSInt32ToHex(hr, 8));
			}
			return false;
		}
	}
	return true;
}
//---------------------------------------------------------------------------
void tTVPBasicDrawDevice::EnsureDevice()
{
	TVPInitBasicDrawDeviceOptions();
	if( TargetWindow ) {
		try {
			bool recreate = false;
			if( Direct3D == NULL || Direct3DDevice == NULL ) {
				if( GetDirect3D9Device() == false ) {
					return;
				}
				recreate = true;
			}
			if( Texture == NULL ) {
				if( CreateTexture() == false ) {
					return;
				}
				recreate = true;
			}
			if( recreate ) {
				InvalidateAll();
			}
		} catch(const eTJS & e) {
			TVPAddImportantLog( TVPFormatMessage(TVPBasicDrawDeviceFailedToCreateDirect3DDevice,e.GetMessage()) );
			DestroyD3DDevice();
		} catch(...) {
			TVPAddImportantLog( (const tjs_char*)TVPBasicDrawDeviceFailedToCreateDirect3DDeviceUnknownReason );
			DestroyD3DDevice();
		}
	}
}
//---------------------------------------------------------------------------
void tTVPBasicDrawDevice::TryRecreateWhenDeviceLost()
{
	bool success = false;
	if( Direct3DDevice ) {
		DestroyTexture();
		HRESULT hr = Direct3DDevice->TestCooperativeLevel();
		if( hr == D3DERR_DEVICENOTRESET ) {
			hr = Direct3DDevice->Reset(&D3dPP);
		}
		if( FAILED(hr) ) {
			success = CreateD3DDevice();
		} else {
			if( D3D_OK == InitializeDirect3DState() ) {
				success = true;
			}
		}
	} else {
		success = CreateD3DDevice();
	}
	if( success ) {
		InvalidateAll();	// 画像の再描画(Layer Update)を要求する
	}
}
//---------------------------------------------------------------------------
void tTVPBasicDrawDevice::ErrorToLog( HRESULT hr ) {
	switch( hr ) {
	case D3DERR_DEVICELOST:
		TVPAddLog( (const tjs_char*)TVPD3dErrDeviceLost );
		break;
	case D3DERR_DRIVERINTERNALERROR:
		TVPAddLog( (const tjs_char*)TVPD3dErrDriverIinternalError );
		break;
	case D3DERR_INVALIDCALL:
		TVPAddLog( (const tjs_char*)TVPD3dErrInvalidCall );
		break;
	case D3DERR_OUTOFVIDEOMEMORY:
		TVPAddLog( (const tjs_char*)TVPD3dErrOutOfVideoMemory );
		break;
	case E_OUTOFMEMORY:
		TVPAddLog( (const tjs_char*)TVPD3dErrOutOfMemory );
		break;
	case D3DERR_WRONGTEXTUREFORMAT:
		TVPAddLog( (const tjs_char*)TVPD3dErrWrongTextureFormat );
		break;
	case D3DERR_UNSUPPORTEDCOLOROPERATION:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsuportedColorOperation );
		break;
	case D3DERR_UNSUPPORTEDCOLORARG:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsuportedColorArg );
		break;
	case D3DERR_UNSUPPORTEDALPHAOPERATION:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsuportedAalphtOperation );
		break;
	case D3DERR_UNSUPPORTEDALPHAARG:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsuportedAlphaArg );
		break;
	case D3DERR_TOOMANYOPERATIONS:
		TVPAddLog( (const tjs_char*)TVPD3dErrTooManyOperations );
		break;
	case D3DERR_CONFLICTINGTEXTUREFILTER:
		TVPAddLog( (const tjs_char*)TVPD3dErrConflictioningTextureFilter );
		break;
	case D3DERR_UNSUPPORTEDFACTORVALUE:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsuportedFactorValue );
		break;
	case D3DERR_CONFLICTINGRENDERSTATE:
		TVPAddLog( (const tjs_char*)TVPD3dErrConflictioningRenderState );
		break;
	case D3DERR_UNSUPPORTEDTEXTUREFILTER:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsupportedTextureFilter );
		break;
	case D3DERR_CONFLICTINGTEXTUREPALETTE:
		TVPAddLog( (const tjs_char*)TVPD3dErrConflictioningTexturePalette );
		break;
	case D3DERR_NOTFOUND:
		TVPAddLog( (const tjs_char*)TVPD3dErrNotFound );
		break;
	case D3DERR_MOREDATA:
		TVPAddLog( (const tjs_char*)TVPD3dErrMoreData );
		break;
	case D3DERR_DEVICENOTRESET:
		TVPAddLog( (const tjs_char*)TVPD3dErrDeviceNotReset );
		break;
	case D3DERR_NOTAVAILABLE:
		TVPAddLog( (const tjs_char*)TVPD3dErrNotAvailable );
		break;
	case D3DERR_INVALIDDEVICE:
		TVPAddLog( (const tjs_char*)TVPD3dErrInvalidDevice );
		break;
	case D3DERR_DRIVERINVALIDCALL:
		TVPAddLog( (const tjs_char*)TVPD3dErrDriverInvalidCall );
		break;
	case D3DERR_WASSTILLDRAWING:
		TVPAddLog( (const tjs_char*)TVPD3dErrWasStillDrawing );
		break;
	case D3DERR_DEVICEHUNG:
		TVPAddLog( (const tjs_char*)TVPD3dErrDeviceHung );
		break;
	case D3DERR_UNSUPPORTEDOVERLAY:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsupportedOverlay );
		break;
	case D3DERR_UNSUPPORTEDOVERLAYFORMAT:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsupportedOverlayFormat );
		break;
	case D3DERR_CANNOTPROTECTCONTENT:
		TVPAddLog( (const tjs_char*)TVPD3dErrCannotProtectContent );
		break;
	case D3DERR_UNSUPPORTEDCRYPTO:
		TVPAddLog( (const tjs_char*)TVPD3dErrUnsupportedCrypto );
		break;
	case D3DERR_PRESENT_STATISTICS_DISJOINT:
		TVPAddLog( (const tjs_char*)TVPD3dErrPresentStatisticsDisJoint );
		break;
	case D3DERR_DEVICEREMOVED:
		TVPAddLog( (const tjs_char*)TVPD3dErrDeviceRemoved );
		break;
	case D3D_OK:
		break;
	case D3DOK_NOAUTOGEN:
		TVPAddLog( (const tjs_char*)TVPD3dOkNoAutoGen );
		break;
	case E_FAIL:
		TVPAddLog( (const tjs_char*)TVPD3dErrFail );
		break;
	case E_INVALIDARG:
		TVPAddLog( (const tjs_char*)TVPD3dErrInvalidArg );
		break;
	default:
		TVPAddLog( (const tjs_char*)TVPD3dUnknownError );
		break;
	}
}
#endif
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::AddLayerManager(iTVPLayerManager * manager)
{
	if(inherited::Managers.size() > 0)
	{
		// "Basic" デバイスでは２つ以上のLayer Managerを登録できない
		TVPThrowExceptionMessage(TVPBasicDrawDeviceDoesNotSupporteLayerManagerMoreThanOne);
	}
	inherited::AddLayerManager(manager);

	manager->SetDesiredLayerType(ltOpaque); // ltOpaque な出力を受け取りたい
}
//---------------------------------------------------------------------------
#if 0
void TJS_INTF_METHOD tTVPBasicDrawDevice::SetTargetWindow(HWND wnd, bool is_main)
{
	TVPInitBasicDrawDeviceOptions();
	DestroyD3DDevice();
	TargetWindow = wnd;
	IsMainWindow = is_main;
}
#endif
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::SetDestRectangle(const tTVPRect & rect)
{
#if 0
	BackBufferDirty = true;
	// 位置だけの変更の場合かどうかをチェックする
	if(rect.get_width() == DestRect.get_width() && rect.get_height() == DestRect.get_height()) {
		// 位置だけの変更だ
		inherited::SetDestRectangle(rect);
	} else {
		// サイズも違う
		if( rect.get_width() > (tjs_int)D3dPP.BackBufferWidth || rect.get_height() > (tjs_int)D3dPP.BackBufferHeight ) {
			// バックバッファサイズよりも大きいサイズが指定された場合一度破棄する。後のEnsureDeviceで再生成される。
			DestroyD3DDevice();
		}
		bool success = true;
		inherited::SetDestRectangle(rect);

		try {
			EnsureDevice();
		} catch(const eTJS & e) {
			TVPAddImportantLog( TVPFormatMessage(TVPBasicDrawDeviceFailedToCreateDirect3DDevices,e.GetMessage() ) );
			success = false;
		} catch(...) {
			TVPAddImportantLog( (const tjs_char*)TVPBasicDrawDeviceFailedToCreateDirect3DDevicesUnknownReason );
			success = false;
		}
		if( success == false ) {
			DestroyD3DDevice();
		}
	}
#endif
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::NotifyLayerResize(iTVPLayerManager * manager)
{
	inherited::NotifyLayerResize(manager);

	BackBufferDirty = true;

	// テクスチャを捨てて作り直す。
//	CreateTexture();
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::Show()
{
	if (Window) {
		iWindowLayer *form = Window->GetForm();
		if (form && !Managers.empty()) {
			iTVPBaseBitmap *buf = Managers.back()->GetDrawBuffer();
			if (buf);
				form->UpdateDrawBuffer(buf->GetTexture());
		}
	}
#if 0
	if(!TargetWindow) return;
	if(!Texture) return;
	if(!Direct3DDevice) return;
	if(!ShouldShow) return;

	ShouldShow = false;

	HRESULT hr = D3D_OK;
	RECT client;
	if( ::GetClientRect( TargetWindow, &client ) ) {
		RECT drect;
		drect.left   = 0;
		drect.top    = 0;
		drect.right  = client.right - client.left;
		drect.bottom = client.bottom - client.top;

		RECT srect = drect;
		hr = Direct3DDevice->Present( &srect, &drect, TargetWindow, NULL );
	} else {
		ShouldShow = true;
	}

	if(hr == D3DERR_DEVICELOST) {
		if( IsTargetWindowActive() ) ErrorToLog( hr );
		TryRecreateWhenDeviceLost();
	} else if(hr != D3D_OK) {
		ErrorToLog( hr );
		TVPAddImportantLog( TVPFormatMessage(TVPBasicDrawDeviceInfDirect3DDevicePresentFailed,TJSInt32ToHex(hr, 8)) );
	}
#endif
}
//---------------------------------------------------------------------------
#if 0
bool TJS_INTF_METHOD tTVPBasicDrawDevice::WaitForVBlank( tjs_int* in_vblank, tjs_int* delayed )
{
	if( Direct3DDevice == NULL ) return false;

	bool inVsync = false;
	D3DRASTER_STATUS rs;
	if( D3D_OK == Direct3DDevice->GetRasterStatus(0,&rs) ) {
		inVsync = rs.InVBlank == TRUE;
	}

	// VSync 待ちを行う
	bool isdelayed = false;
	if(!inVsync) {
		// vblank から抜けるまで待つ
		DWORD timeout_target_tick = ::timeGetTime() + 1;
		rs.InVBlank = FALSE;
		HRESULT hr = D3D_OK;
		do {
			hr = Direct3DDevice->GetRasterStatus(0,&rs);
		} while( D3D_OK == hr && rs.InVBlank == TRUE && (long)(::timeGetTime() - timeout_target_tick) <= 0);

		// vblank に入るまで待つ
		rs.InVBlank = TRUE;
		do {
			hr = Direct3DDevice->GetRasterStatus(0,&rs);
		} while( D3D_OK == hr && rs.InVBlank == FALSE && (long)(::timeGetTime() - timeout_target_tick) <= 0);

		if((int)(::timeGetTime() - timeout_target_tick) > 0) {
			// フレームスキップが発生したと考えてよい
			isdelayed  = true;
		}
		inVsync = rs.InVBlank == TRUE;
	}
	*delayed = isdelayed ? 1 : 0;
	*in_vblank = inVsync ? 1 : 0;
	return true;
}
#endif
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::StartBitmapCompletion(iTVPLayerManager * manager)
{
#if 0
	EnsureDevice();

	if( Texture && TargetWindow ) {
		if(TextureBuffer) {
			TVPAddImportantLog( (const tjs_char*)TVPBasicDrawDeviceTextureHasAlreadyBeenLocked );
			Texture->UnlockRect(0), TextureBuffer = NULL;
		}

		D3DLOCKED_RECT rt;
		HRESULT hr = Texture->LockRect( 0, &rt, NULL, D3DLOCK_NO_DIRTY_UPDATE );

		if(hr == D3DERR_INVALIDCALL && IsTargetWindowActive() ) {
			TVPThrowExceptionMessage( TVPInternalErrorResult, TJSInt32ToHex(hr, 8));
		}

		if(hr != D3D_OK) {
			ErrorToLog( hr );
			TextureBuffer = NULL;
			TryRecreateWhenDeviceLost();
		} else /*if(hr == DD_OK) */ {
			TextureBuffer = rt.pBits;
			TexturePitch = rt.Pitch;
		}
	}
#endif
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::NotifyBitmapCompleted(iTVPLayerManager * manager,
	tjs_int x, tjs_int y, tTVPBaseTexture * bmp,
	const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity)
{
	// bits, bitmapinfo で表されるビットマップの cliprect の領域を、x, y に描画
	// する。
	// opacity と type は無視するしかないので無視する
	tjs_int w, h;
	GetSrcSize( w, h );
#if 0
	if( TextureBuffer && TargetWindow &&
		!(x < 0 || y < 0 ||
			x + cliprect.get_width() > w ||
			y + cliprect.get_height() > h) &&
		!(cliprect.left < 0 || cliprect.top < 0 ||
			cliprect.right > bitmapinfo->bmiHeader.biWidth ||
			cliprect.bottom > bitmapinfo->bmiHeader.biHeight))
	{
		// 範囲外の転送は(一部だけ転送するのではなくて)無視してよい
		ShouldShow = true;

		// bitmapinfo で表された cliprect の領域を x,y にコピーする
		long src_y       = cliprect.top;
		long src_y_limit = cliprect.bottom;
		long src_x       = cliprect.left;
		long width_bytes   = cliprect.get_width() * 4; // 32bit
		long dest_y      = y;
		long dest_x      = x;
		const tjs_uint8 * src_p = (const tjs_uint8 *)bits;
		long src_pitch;

		if(bitmapinfo->bmiHeader.biHeight < 0)
		{
			// bottom-down
			src_pitch = bitmapinfo->bmiHeader.biWidth * 4;
		}
		else
		{
			// bottom-up
			src_pitch = -bitmapinfo->bmiHeader.biWidth * 4;
			src_p += bitmapinfo->bmiHeader.biWidth * 4 * (bitmapinfo->bmiHeader.biHeight - 1);
		}

		for(; src_y < src_y_limit; src_y ++, dest_y ++)
		{
			const void *srcp = src_p + src_pitch * src_y + src_x * 4;
			void *destp = (tjs_uint8*)TextureBuffer + TexturePitch * dest_y + dest_x * 4;
			memcpy(destp, srcp, width_bytes);
		}
	}
#endif
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::EndBitmapCompletion(iTVPLayerManager * manager)
{
	if(!TargetWindow) return;
#if 0
	if(!Texture) return;
	if(!Direct3DDevice) return;

	if(!TextureBuffer) return;
	Texture->UnlockRect(0);
	TextureBuffer = NULL;


	//- build vertex list
	struct tVertices
	{
		float x, y, z, rhw;
		float tu, tv;
	};

	// 転送先をクリッピング矩形に基づきクリッピング
	float dl = (float)( DestRect.left < ClipRect.left ? ClipRect.left : DestRect.left );
	float dt = (float)( DestRect.top < ClipRect.top ? ClipRect.top : DestRect.top );
	float dr = (float)( DestRect.right > ClipRect.right ? ClipRect.right : DestRect.right );
	float db = (float)( DestRect.bottom > ClipRect.bottom ? ClipRect.bottom : DestRect.bottom );
	float dw = (float)DestRect.get_width();
	float dh = (float)DestRect.get_height();

	// はみ出している幅を求める
	float cl = dl - (float)DestRect.left;
	float ct = dt - (float)DestRect.top;
	float cr = (float)DestRect.right - dr;
	float cb = (float)DestRect.bottom - db;

	// はみ出している幅を考慮して、転送元画像をクリッピング
	tjs_int w, h;
	GetSrcSize( w, h );
	float sl = (float)(cl * w / dw ) / (float)TextureWidth;
	float st = (float)(ct * h / dh ) / (float)TextureHeight;
	float sr = (float)(w - (cr * w / dw) ) / (float)TextureWidth;
	float sb = (float)(h - (cb * h / dh) ) / (float)TextureHeight;

	tVertices vertices[] =
	{
		{dl - 0.5f, dt - 0.5f, 1.0f, 1.0f, sl, st },
		{dr - 0.5f, dt - 0.5f, 1.0f, 1.0f, sr, st },
		{dl - 0.5f, db - 0.5f, 1.0f, 1.0f, sl, sb },
		{dr - 0.5f, db - 0.5f, 1.0f, 1.0f, sr, sb }
	};

	HRESULT hr;

// フルスクリーン化後、1回は全体消去、それ以降はウィンドウの範囲内のみにした方が効率的。
	D3DVIEWPORT9 vp;
	vp.X  = 0;
	vp.Y  = 0;
	vp.Width = D3dPP.BackBufferWidth;
	vp.Height = D3dPP.BackBufferHeight;
	vp.MinZ  = 0.0f;
	vp.MaxZ  = 1.0f;
	Direct3DDevice->SetViewport(&vp);

	if( SUCCEEDED(Direct3DDevice->BeginScene()) ) {
		struct CAutoEndSceneCall {
			IDirect3DDevice9*	m_Device;
			CAutoEndSceneCall( IDirect3DDevice9* device ) : m_Device(device) {}
			~CAutoEndSceneCall() { m_Device->EndScene(); }
		} autoEnd(Direct3DDevice);

		if( BackBufferDirty ) {
			Direct3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0, 0 );
			BackBufferDirty = false;
		}

		//- draw as triangles
		if( FAILED(hr = Direct3DDevice->SetTexture(0, Texture)) )
			goto got_error;

		if( FAILED( hr = Direct3DDevice->SetFVF( D3DFVF_XYZRHW|D3DFVF_TEX1 ) ) )
			goto got_error;

		if( FAILED( hr = Direct3DDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(tVertices) ) ) )
			goto got_error;

		if( FAILED( hr = Direct3DDevice->SetTexture( 0, NULL) ) )
			goto got_error;
	}

got_error:
	if( hr == D3DERR_DEVICELOST ) {
		TryRecreateWhenDeviceLost();
	} else if(hr == D3DERR_DEVICENOTRESET ) {
		hr = Direct3DDevice->Reset(&D3dPP);
		if( hr == D3DERR_DEVICELOST ) {
			TVPAddLog( (const tjs_char*)TVPD3dErrDeviceLost );
			TryRecreateWhenDeviceLost();
		} else if( hr == D3DERR_DRIVERINTERNALERROR ) {
			TVPAddLog( (const tjs_char*)TVPD3dErrDriverIinternalError );
			TryRecreateWhenDeviceLost();
		} else if( hr == D3DERR_INVALIDCALL ) {
			TVPAddLog( (const tjs_char*)TVPD3dErrInvalidCall );
			TryRecreateWhenDeviceLost();
		} else if( hr  == D3DERR_OUTOFVIDEOMEMORY ) {
			TVPAddLog( (const tjs_char*)TVPD3dErrOutOfVideoMemory );
			TryRecreateWhenDeviceLost();
		} else if( hr == E_OUTOFMEMORY  ) {
			TVPAddLog( (const tjs_char*)TVPD3dErrOutOfMemory );
			TryRecreateWhenDeviceLost();
		} else if( hr == D3D_OK ) {
			if( FAILED( hr = InitializeDirect3DState() ) ) {
				if( IsTargetWindowActive() ) {
					ErrorToLog( hr );
					TVPThrowExceptionMessage( TVPFaildToSetRenderState );
				} else {
					DestroyD3DDevice();
				}
			}
		}
	} else if(hr != D3D_OK) {
		ErrorToLog( hr );
		TVPAddImportantLog( TVPFormatMessage(TVPBasicDrawDeviceInfPolygonDrawingFailed,TJSInt32ToHex(hr, 8)) );
	}
#endif
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::SetShowUpdateRect(bool b)
{
	DrawUpdateRectangle = b;
}
#if 0
//---------------------------------------------------------------------------
bool TJS_INTF_METHOD tTVPBasicDrawDevice::SwitchToFullScreen( HWND window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color, bool changeresolution )
{
	// フルスクリーン化の処理はなにも行わない、互換性のためにウィンドウを全画面化するのみで処理する
	// Direct3D9 でフルスクリーン化するとフォーカスを失うとデバイスをロストするので、そのたびにリセットor作り直しが必要になる。
	// モーダルウィンドウを使用するシステムでは、これは困るので常にウィンドウモードで行う。
	// モーダルウィンドウを使用しないシステムにするのなら、フルスクリーンを使用するDrawDeviceを作ると良い。
	BackBufferDirty = true;
	ShouldShow = true;
	CheckMonitorMoved();
	return true;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPBasicDrawDevice::RevertFromFullScreen( HWND window, tjs_uint w, tjs_uint h, tjs_uint bpp, tjs_uint color )
{
	BackBufferDirty = true;
	ShouldShow = true;
	CheckMonitorMoved();
}
//---------------------------------------------------------------------------
#endif






//---------------------------------------------------------------------------
// tTJSNI_BasicDrawDevice : BasicDrawDevice TJS native class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_BasicDrawDevice::ClassID = (tjs_uint32)-1;
tTJSNC_BasicDrawDevice::tTJSNC_BasicDrawDevice() :
	tTJSNativeClass(TJS_W("BasicDrawDevice"))
{
	// register native methods/properties

	TJS_BEGIN_NATIVE_MEMBERS(BasicDrawDevice)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
// constructor/methods
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_BasicDrawDevice,
	/*TJS class name*/BasicDrawDevice)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/BasicDrawDevice)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/recreate)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BasicDrawDevice);
#if 0
	_this->GetDevice()->SetToRecreateDrawer();
	_this->GetDevice()->EnsureDevice();
#endif
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/recreate)
//----------------------------------------------------------------------


//---------------------------------------------------------------------------
//----------------------------------------------------------------------
// properties
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(interface)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BasicDrawDevice);
		*result = reinterpret_cast<tjs_int64>(_this->GetDevice());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(interface)
//----------------------------------------------------------------------
#define TVP_REGISTER_PTDD_ENUM(name) \
	TJS_BEGIN_NATIVE_PROP_DECL(name) \
	{ \
		TJS_BEGIN_NATIVE_PROP_GETTER \
		{ \
		*result = (tjs_int64)tTVPBasicDrawDevice::name; \
			return TJS_S_OK; \
		} \
		TJS_END_NATIVE_PROP_GETTER \
		TJS_DENY_NATIVE_PROP_SETTER \
	} \
	TJS_END_NATIVE_PROP_DECL(name)
// compatible for old kirikiri2
TVP_REGISTER_PTDD_ENUM(dtNone)
TVP_REGISTER_PTDD_ENUM(dtDrawDib)
TVP_REGISTER_PTDD_ENUM(dtDBGDI)
TVP_REGISTER_PTDD_ENUM(dtDBDD)
TVP_REGISTER_PTDD_ENUM(dtDBD3D)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(preferredDrawer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BasicDrawDevice);
		*result = (tjs_int64)(_this->GetDevice()->GetPreferredDrawerType());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BasicDrawDevice);
		_this->GetDevice()->SetPreferredDrawerType((tTVPBasicDrawDevice::tDrawerType)(tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(preferredDrawer)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(drawer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_BasicDrawDevice);
		*result = (tjs_int64)(_this->GetDevice()->GetDrawerType());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(drawer)
//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
iTJSNativeInstance *tTJSNC_BasicDrawDevice::CreateNativeInstance()
{
	return new tTJSNI_BasicDrawDevice();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
tTJSNI_BasicDrawDevice::tTJSNI_BasicDrawDevice()
{
	Device = new tTVPBasicDrawDevice();
}
//---------------------------------------------------------------------------
tTJSNI_BasicDrawDevice::~tTJSNI_BasicDrawDevice()
{
	if(Device) Device->Destruct(), Device = NULL;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSNI_BasicDrawDevice::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_BasicDrawDevice::Invalidate()
{
	if(Device) Device->Destruct(), Device = NULL;
}
//---------------------------------------------------------------------------

