//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "Window" TJS Class implementation
//---------------------------------------------------------------------------


#ifndef WindowImplH
#define WindowImplH

#include "WindowIntf.h"
#include "win32/TVPWindow.h"

/*[*/
//---------------------------------------------------------------------------
// window message receivers
//---------------------------------------------------------------------------
#if 0
#pragma pack(push, 4)
struct tTVPWindowMessage
{
	unsigned int Msg; // window message
	WPARAM WParam;  // WPARAM
	LPARAM LParam;  // LPARAM
	LRESULT Result;  // result
};
#pragma pack(pop)
typedef bool (__stdcall * tTVPWindowMessageReceiver)
	(void *userdata, tTVPWindowMessage *Message);

#define TVP_WM_DETACH (WM_USER+106)  // before re-generating the window
#define TVP_WM_ATTACH (WM_USER+107)  // after re-generating the window
#define TVP_WM_FULLSCREEN_CHANGING (WM_USER+108)  // before full-screen or window changing
#define TVP_WM_FULLSCREEN_CHANGED  (WM_USER+109)  // after full-screen or window changing
#endif

/*]*/
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
/*
struct tTVP_devicemodeA {
	// copy of DEVMODE, to avoid windows platform SDK version mismatch
#pragma pack(push, 1)
	BYTE   dmDeviceName[CCHDEVICENAME];
	WORD dmSpecVersion;
	WORD dmDriverVersion;
	WORD dmSize;
	WORD dmDriverExtra;
	DWORD dmFields;
	union {
	  struct {
		short dmOrientation;
		short dmPaperSize;
		short dmPaperLength;
		short dmPaperWidth;
	  };
	  POINTL dmPosition;
	};
	short dmScale;
	short dmCopies;
	short dmDefaultSource;
	short dmPrintQuality;
	short dmColor;
	short dmDuplex;
	short dmYResolution;
	short dmTTOption;
	short dmCollate;
	BYTE   dmFormName[CCHFORMNAME];
	WORD   dmLogPixels;
	DWORD  dmBitsPerPel;
	DWORD  dmPelsWidth;
	DWORD  dmPelsHeight;
	union {
		DWORD  dmDisplayFlags;
		DWORD  dmNup;
	};
	DWORD  dmDisplayFrequency;
	DWORD  dmICMMethod;
	DWORD  dmICMIntent;
	DWORD  dmMediaType;
	DWORD  dmDitherType;
	DWORD  dmReserved1;
	DWORD  dmReserved2;
#pragma pack(pop)
};
*/
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Mouse Cursor Management
//---------------------------------------------------------------------------
extern tjs_int TVPGetCursor(const ttstr & name);
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Utility functions
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tjs_uint32, TVPGetCurrentShiftKeyState, ());

// implement for Application
#if 0
TJS_EXP_FUNC_DEF(void, TVPRegisterAcceleratorKey, (HWND hWnd, char virt, short key, short cmd) );
TJS_EXP_FUNC_DEF(void, TVPUnregisterAcceleratorKey, (HWND hWnd, short cmd));
TJS_EXP_FUNC_DEF(void, TVPDeleteAcceleratorKeyTable, (HWND hWnd));
HWND TVPGetModalWindowOwnerHandle();
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// Color Format Detection
//---------------------------------------------------------------------------
extern tjs_int TVPDisplayColorFormat;
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// Screen Mode management
//---------------------------------------------------------------------------

//! @brief		Structure for monitor screen mode
struct tTVPScreenMode
{
	tjs_int Width; //!< width of screen in pixel
	tjs_int Height; //!< height of screen in pixel
	tjs_int BitsPerPixel; //!< bits per pixel (0 = unspecified)

	ttstr Dump() const
	{
		return
			DumpHeightAndWidth() +
			TJS_W(", BitsPerPixel=") + (BitsPerPixel?ttstr(BitsPerPixel):ttstr(TJS_W("unspecified")));
	}

	ttstr DumpHeightAndWidth() const
	{
		return
			TJS_W("Width=") + ttstr(Width) +
			TJS_W(", Height=") + ttstr(Height);
	}

	bool operator < (const tTVPScreenMode & rhs) const {
		tjs_int area_this = Width * Height;
		tjs_int area_rhs  = rhs.Width * rhs.Height;
		if(area_this < area_rhs) return true;
		if(area_this > area_rhs) return false;
		if(BitsPerPixel < rhs.BitsPerPixel) return true;
		if(BitsPerPixel > rhs.BitsPerPixel) return false;
		return false;
	}
	bool operator == (const tTVPScreenMode & rhs) const {
		return ( Width == rhs.Width && Height == rhs.Height && BitsPerPixel == rhs.BitsPerPixel );
	}
};

//! @brief		Structure for monitor screen mode candidate
struct tTVPScreenModeCandidate : tTVPScreenMode
{
	tjs_int ZoomNumer; //!< zoom ratio numer
	tjs_int ZoomDenom; //!< zoom ratio denom
	tjs_int RankZoomIn;
	tjs_int RankBPP;
	tjs_int RankZoomBeauty;
	tjs_int RankSize; //!< candidate preference priority (lower value is higher preference)

	ttstr Dump() const
	{
		return tTVPScreenMode::Dump() +
			TJS_W(", ZoomNumer=") + ttstr(ZoomNumer) +
			TJS_W(", ZoomDenom=") + ttstr(ZoomDenom) +
			TJS_W(", RankZoomIn=") + ttstr(RankZoomIn) +
			TJS_W(", RankBPP=") + ttstr(RankBPP) +
			TJS_W(", RankZoomBeauty=") + ttstr(RankZoomBeauty) +
			TJS_W(", RankSize=") + ttstr(RankSize);
	}

	bool operator < (const tTVPScreenModeCandidate & rhs) const{
		if(RankZoomIn < rhs.RankZoomIn) return true;
		if(RankZoomIn > rhs.RankZoomIn) return false;
		if(RankBPP < rhs.RankBPP) return true;
		if(RankBPP > rhs.RankBPP) return false;
		if(RankZoomBeauty < rhs.RankZoomBeauty) return true;
		if(RankZoomBeauty > rhs.RankZoomBeauty) return false;
		if(RankSize < rhs.RankSize) return true;
		if(RankSize > rhs.RankSize) return false;
		return false;
	}
};
struct IDirect3D9;
extern void TVPTestDisplayMode(tjs_int w, tjs_int h, tjs_int & bpp);
extern void TVPSwitchToFullScreen(HWND window, tjs_int w, tjs_int h, class iTVPDrawDevice* drawdevice);
extern void TVPRecalcFullScreen( tjs_int w, tjs_int h );
extern void TVPRevertFromFullScreen(HWND window,tjs_uint w,tjs_uint h, class iTVPDrawDevice* drawdevice);
TJS_EXP_FUNC_DEF(void, TVPEnsureDirect3DObject, ());
void TVPDumpDirect3DDriverInformation();
extern tTVPScreenModeCandidate TVPFullScreenMode;
/*[*/
//---------------------------------------------------------------------------
// Direct3D former declaration
//---------------------------------------------------------------------------
#ifndef DIRECT3D_VERSION
struct IDirect3D9;
#endif

/*]*/
TJS_EXP_FUNC_DEF(IDirect3D9 *,  TVPGetDirect3DObjectNoAddRef, ());
extern void TVPMinimizeFullScreenWindowAtInactivation();
extern void TVPRestoreFullScreenWindowAtActivation();
#endif
//---------------------------------------------------------------------------










//---------------------------------------------------------------------------
// tTJSNI_Window : Window Native Instance
//---------------------------------------------------------------------------
typedef iWindowLayer TTVPWindowForm;
class iTVPDrawDevice;
class tTJSNI_BaseLayer;
class tTJSNI_Window : public tTJSNI_BaseWindow
{
	TTVPWindowForm *Form;
//	class tTVPVSyncTimingThread *VSyncTimingThread;

public:
	tTJSNI_Window();
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();
	bool CloseFlag;

public:
	bool CanDeliverEvents() const; // tTJSNI_BaseWindow::CanDeliverEvents override

public:
	TTVPWindowForm * GetForm() const override { return Form; }
	void NotifyWindowClose();
	void SendCloseMessage();
	void TickBeat();

private:
	bool GetWindowActive();
	void UpdateVSyncThread();
	/**
	 * フルスクリーン時に操作できない値を変えようとした時に確認のため呼び出し、フルスクリーンの時例外を出す
	 */
	void FullScreenGuard() const;

public:
//-- draw device
	virtual void ResetDrawDevice();

//-- event control
	virtual void PostInputEvent(const ttstr &name, iTJSDispatch2 * params);  // override

//-- interface to layer manager
	void TJS_INTF_METHOD NotifySrcResize(); // is called from primary layer

	void TJS_INTF_METHOD SetDefaultMouseCursor(); // set window mouse cursor to default
	void TJS_INTF_METHOD SetMouseCursor(tjs_int cursor); // set window mouse cursor
	void TJS_INTF_METHOD GetCursorPos(tjs_int &x, tjs_int &y);
	void TJS_INTF_METHOD SetCursorPos(tjs_int x, tjs_int y);
	void TJS_INTF_METHOD WindowReleaseCapture();
	void TJS_INTF_METHOD SetHintText(iTJSDispatch2* sender, const ttstr & text);
	void TJS_INTF_METHOD SetAttentionPoint(tTJSNI_BaseLayer *layer,
		tjs_int l, tjs_int t);
	void TJS_INTF_METHOD DisableAttentionPoint();
	void TJS_INTF_METHOD SetImeMode(tTVPImeMode mode);
	void SetDefaultImeMode(tTVPImeMode mode);
	tTVPImeMode GetDefaultImeMode() const;
	void TJS_INTF_METHOD ResetImeMode();

//-- update managment
	void BeginUpdate(const tTVPComplexRect &rects);
	void EndUpdate();

//-- interface to VideoOverlay object
public:
#if 0
	HWND GetSurfaceWindowHandle();
	HWND GetWindowHandle();
#endif
	void GetVideoOffset(tjs_int &ofsx, tjs_int &ofsy);

	void ReadjustVideoRect();
	void WindowMoved();
	void DetachVideoOverlay();

//-- interface to plugin
	void ZoomRectangle(
		tjs_int & left, tjs_int & top,
		tjs_int & right, tjs_int & bottom);
#if 0
	HWND GetWindowHandleForPlugin();
#endif
	void RegisterWindowMessageReceiver(tTVPWMRRegMode mode,
		void * proc, const void *userdata);

//-- methods
	void Close();
	void OnCloseQueryCalled(bool b);
	
#ifdef USE_OBSOLETE_FUNCTIONS
	void BeginMove();
#endif

	void BringToFront();
	void Update(tTVPUpdateType);

	void ShowModal();

	void HideMouseCursor();

//-- properties
	bool GetVisible() const;
	void SetVisible(bool s);

	void GetCaption(ttstr & v) const;
	void SetCaption(const ttstr & v);

	void SetWidth(tjs_int w);
	tjs_int GetWidth() const;
	void SetHeight(tjs_int h);
	tjs_int GetHeight() const;
	void SetSize(tjs_int w, tjs_int h);

	void SetMinWidth(int v);
	int GetMinWidth() const;
	void SetMinHeight(int v);
	int GetMinHeight() const;
	void SetMinSize(int w, int h);

	void SetMaxWidth(int v);
	int GetMaxWidth() const;
	void SetMaxHeight(int v);
	int GetMaxHeight() const;
	void SetMaxSize(int w, int h);

	void SetLeft(tjs_int l);
	tjs_int GetLeft() const;
	void SetTop(tjs_int t);
	tjs_int GetTop() const;
	void SetPosition(tjs_int l, tjs_int t);

#ifdef USE_OBSOLETE_FUNCTIONS
	void SetLayerLeft(tjs_int l);
	tjs_int GetLayerLeft() const;
	void SetLayerTop(tjs_int t);
	tjs_int GetLayerTop() const;
	void SetLayerPosition(tjs_int l, tjs_int t);

	void SetInnerSunken(bool b);
	bool GetInnerSunken() const;
#endif

	void SetInnerWidth(tjs_int w);
	tjs_int GetInnerWidth() const;
	void SetInnerHeight(tjs_int h);
	tjs_int GetInnerHeight() const;

	void SetInnerSize(tjs_int w, tjs_int h);

	void SetBorderStyle(tTVPBorderStyle st);
	tTVPBorderStyle GetBorderStyle() const;

	void SetStayOnTop(bool b);
	bool GetStayOnTop() const;

#ifdef USE_OBSOLETE_FUNCTIONS
	void SetShowScrollBars(bool b);
	bool GetShowScrollBars() const;
#endif

	void SetFullScreen(bool b);
	bool GetFullScreen() const;

	void SetUseMouseKey(bool b);
	bool GetUseMouseKey() const;

	void SetTrapKey(bool b);
	bool GetTrapKey() const;

	void SetMaskRegion(tjs_int threshold);
	void RemoveMaskRegion();

	void SetMouseCursorState(tTVPMouseCursorState mcs);
    tTVPMouseCursorState GetMouseCursorState() const;

	void SetFocusable(bool b);
	bool GetFocusable();

	void SetZoom(tjs_int numer, tjs_int denom);
	void SetZoomNumer(tjs_int n);
	tjs_int GetZoomNumer() const;
	void SetZoomDenom(tjs_int n);
	tjs_int GetZoomDenom() const;
	
	void SetTouchScaleThreshold( tjs_real threshold );
	tjs_real GetTouchScaleThreshold() const;
	void SetTouchRotateThreshold( tjs_real threshold );
	tjs_real GetTouchRotateThreshold() const;

	tjs_real GetTouchPointStartX( tjs_int index );
	tjs_real GetTouchPointStartY( tjs_int index );
	tjs_real GetTouchPointX( tjs_int index );
	tjs_real GetTouchPointY( tjs_int index );
	tjs_real GetTouchPointID( tjs_int index );
	tjs_int GetTouchPointCount();
	bool GetTouchVelocity( tjs_int id, float& x, float& y, float& speed ) const;
	bool GetMouseVelocity( float& x, float& y, float& speed ) const;
	void ResetMouseVelocity();
	
	void SetHintDelay( tjs_int delay );
	tjs_int GetHintDelay() const;

	void SetEnableTouch( bool b );
	bool GetEnableTouch() const;

	int GetDisplayOrientation();
	int GetDisplayRotate();
	
	bool WaitForVBlank( tjs_int* in_vblank, tjs_int* delayed );

	void OnTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
public: // for iTVPLayerTreeOwner
	// LayerManager -> LTO
	/*
	implements on tTJSNI_BaseWindow
	virtual void TJS_INTF_METHOD RegisterLayerManager( class iTVPLayerManager* manager );
	virtual void TJS_INTF_METHOD UnregisterLayerManager( class iTVPLayerManager* manager );
	*/

	virtual void TJS_INTF_METHOD StartBitmapCompletion(iTVPLayerManager * manager);
	virtual void TJS_INTF_METHOD NotifyBitmapCompleted(class iTVPLayerManager * manager,
		tjs_int x, tjs_int y, tTVPBaseTexture * bmp,
		const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity);
	virtual void TJS_INTF_METHOD EndBitmapCompletion(iTVPLayerManager * manager);

	virtual void TJS_INTF_METHOD SetMouseCursor(class iTVPLayerManager* manager, tjs_int cursor);
	virtual void TJS_INTF_METHOD GetCursorPos(class iTVPLayerManager* manager, tjs_int &x, tjs_int &y);
	virtual void TJS_INTF_METHOD SetCursorPos(class iTVPLayerManager* manager, tjs_int x, tjs_int y);
	virtual void TJS_INTF_METHOD ReleaseMouseCapture(class iTVPLayerManager* manager);

	virtual void TJS_INTF_METHOD SetHint(class iTVPLayerManager* manager, iTJSDispatch2* sender, const ttstr &hint);

	virtual void TJS_INTF_METHOD NotifyLayerResize(class iTVPLayerManager* manager);
	virtual void TJS_INTF_METHOD NotifyLayerImageChange(class iTVPLayerManager* manager);

	virtual void TJS_INTF_METHOD SetAttentionPoint(class iTVPLayerManager* manager, tTJSNI_BaseLayer *layer, tjs_int x, tjs_int y);
	virtual void TJS_INTF_METHOD DisableAttentionPoint(class iTVPLayerManager* manager);

	virtual void TJS_INTF_METHOD SetImeMode( class iTVPLayerManager* manager, tjs_int mode ); // mode == tTVPImeMode
	virtual void TJS_INTF_METHOD ResetImeMode( class iTVPLayerManager* manager );

protected:
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif
