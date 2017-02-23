
#ifndef __TVP_WINDOW_H__
#define __TVP_WINDOW_H__

#include <string>
#if 0
#include <shellapi.h>
#include <oleidl.h> // for MK_ALT
#include "tvpinputdefs.h"
#include "SystemImpl.h"
#include "ImeControl.h"

#ifndef MK_ALT 
#define MK_ALT (0x20)
#endif
#endif
enum {
	 ssShift = TVP_SS_SHIFT,
	 ssAlt = TVP_SS_ALT,
	 ssCtrl = TVP_SS_CTRL,
	 ssLeft = TVP_SS_LEFT,
	 ssRight = TVP_SS_RIGHT,
	 ssMiddle = TVP_SS_MIDDLE,
	 ssDouble = TVP_SS_DOUBLE,
	 ssRepeat = TVP_SS_REPEAT,
};
#if 0
class tTVPWindow {
	WNDCLASSEX	wc_;
	bool		created_;

protected:
	enum CloseAction {
	  caNone,
	  caHide,
	  caFree,
	  caMinimize
	};
	enum FormState {
		fsCreating,
		fsVisible,
		fsShowing,
		fsModal,
		fsCreatedMDIChild,
		fsActivated
	};

	HWND				window_handle_;

	std::wstring	window_class_name_;
	std::wstring	window_title_;
	SIZE		window_client_size_;
	SIZE		min_size_;
	SIZE		max_size_;
	int			border_style_;
	bool		in_window_;
	bool		ignore_touch_mouse_;

	bool in_mode_;
	int modal_result_;

	bool has_parent_;

	static const UINT SIZE_CHANGE_FLAGS;
	static const UINT POS_CHANGE_FLAGS;
	static const DWORD DEFAULT_EX_STYLE;
	static const ULONG REGISTER_TOUCH_FLAG;
	static const DWORD DEFAULT_TABLETPENSERVICE_PROPERTY;
	static const DWORD MI_WP_SIGNATURE;
	static const DWORD SIGNATURE_MASK;

	bool left_double_click_;

	ImeControl* ime_control_;

	enum ModeFlag {
		FALG_FULLSCREEN = 0x01,
	};
	
	unsigned long flags_;
	void SetFlag( unsigned long f ) {
		flags_ |= f;
	}
	void ResetFlag( unsigned long f ) {
		flags_ &= ~f;
	}
	bool GetFlag( unsigned long f ) {
		return 0 != (flags_ & f);
	}
	
	void UnregisterWindow();

	// window procedure
	static LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	virtual LRESULT WINAPI Proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	HRESULT CreateWnd( const std::wstring& classname, const std::wstring& title, int width, int height, HWND hParent=NULL );

	virtual void OnDestroy();
	virtual void OnPaint();

	inline int GetAltKeyState() const {
		if( ::GetKeyState( VK_MENU  ) < 0 ) {
			return MK_ALT;
		} else {
			return 0;
		}
	}
	inline int GetShiftState( WPARAM wParam ) const {
		int shift = GET_KEYSTATE_WPARAM(wParam) & (MK_SHIFT|MK_CONTROL);
		shift |= GetAltKeyState();
		return shift;
	}
	inline int GetShiftState() const {
		int shift = 0;
		if( ::GetKeyState( VK_MENU  ) < 0 ) shift |= MK_ALT;
		if( ::GetKeyState( VK_SHIFT ) < 0 ) shift |= MK_SHIFT;
		if( ::GetKeyState( VK_CONTROL ) < 0 ) shift |= MK_CONTROL;
		return shift;
	}
	inline int GetMouseButtonState() const {
		int s = 0;
		if(TVPGetAsyncKeyState(VK_LBUTTON)) s |= ssLeft;
		if(TVPGetAsyncKeyState(VK_RBUTTON)) s |= ssRight;
		if(TVPGetAsyncKeyState(VK_MBUTTON)) s |= ssMiddle;
		return s;
	}
	inline bool IsTouchEvent(LPARAM extraInfo) const {
		return (extraInfo & SIGNATURE_MASK) == MI_WP_SIGNATURE;
	}

	void SetMouseCapture() {
		::SetCapture( GetHandle() );
	}
	void ReleaseMouseCapture() {
		::ReleaseCapture();
	}
	HICON GetBigIcon();

	static bool HasMenu( HWND hWnd );
public:
	tTVPWindow()
	: window_handle_(NULL), created_(false), left_double_click_(false), ime_control_(NULL), border_style_(0), modal_result_(0),
		in_window_(false), ignore_touch_mouse_(false), in_mode_(false), has_parent_(false) {
		min_size_.cx = min_size_.cy = 0;
		max_size_.cx = max_size_.cy = 0;
	}
	virtual ~tTVPWindow();

	bool HasFocus() const {
		return window_handle_ == ::GetFocus();
	}
	bool IsValidHandle() const {
		return ( window_handle_ != NULL && window_handle_ != INVALID_HANDLE_VALUE && ::IsWindow(window_handle_) );
	}

	virtual bool Initialize();

	void SetWidnowTitle( const std::wstring& title );
	void SetScreenSize( int width, int height );

	HWND GetHandle() { return window_handle_; }
	HWND GetHandle() const { return window_handle_; }

	ImeControl* GetIME() { return ime_control_; }
	const ImeControl* GetIME() const { return ime_control_; }

	static void SetClientSize( HWND hWnd, SIZE& size );

//-- properties
	bool GetVisible() const;
	void SetVisible(bool s);
	void Show() { SetVisible( true ); BringToFront(); }
	void Hide() { SetVisible( false ); }

	bool GetEnable() const;
	void SetEnable( bool s );

	void GetCaption( std::wstring& v ) const;
	void SetCaption( const std::wstring& v );
	
	void SetBorderStyle( enum tTVPBorderStyle st);
	enum tTVPBorderStyle GetBorderStyle() const;

	void SetWidth( int w );
	int GetWidth() const;
	void SetHeight( int h );
	int GetHeight() const;
	void SetSize( int w, int h );
	void GetSize( int &w, int &h );

	void SetLeft( int l );
	int GetLeft() const;
	void SetTop( int t );
	int GetTop() const;
	void SetPosition( int l, int t );
	
	void SetBounds( int x, int y, int width, int height );

	void SetMinWidth( int v ) {
		min_size_.cx = v;
		CheckMinMaxSize();
	}
	int GetMinWidth() const{ return min_size_.cx; }
	void SetMinHeight( int v ) {
		min_size_.cy = v;
		CheckMinMaxSize();
	}
	int GetMinHeight() const { return min_size_.cy; }
	void SetMinSize( int w, int h ) {
		min_size_.cx = w;
		min_size_.cy = h;
		CheckMinMaxSize();
	}

	void SetMaxWidth( int v ) {
		max_size_.cx = v;
		CheckMinMaxSize();
	}
	int GetMaxWidth() const{ return max_size_.cx; }
	void SetMaxHeight( int v ) {
		max_size_.cy = v;
		CheckMinMaxSize();
	}
	int GetMaxHeight() const{ return max_size_.cy; }
	void SetMaxSize( int w, int h ) {
		max_size_.cx = w;
		max_size_.cy = h;
		CheckMinMaxSize();
	}
	void CheckMinMaxSize() {
		int maxw = max_size_.cx;
		int maxh = max_size_.cy;
		int minw = min_size_.cx;
		int minh = min_size_.cy;
		int dw, dh;
		GetSize( dw, dh );
		int sw = dw;
		int sh = dh;
		if( maxw > 0 && dw > maxw ) dw = maxw;
		if( maxh > 0 && dh > maxh ) dh = maxh;
		if( minw > 0 && dw < minw ) dw = minw;
		if( minh > 0 && dh < minh ) dh = minh;
		if( sw != dw || sh != dh ) {
			SetSize( dw, dh );
		}
	}

	void SetInnerWidth( int w );
	int GetInnerWidth() const;
	void SetInnerHeight( int h );
	int GetInnerHeight() const;
	void SetInnerSize( int w, int h );
	
	void BringToFront();
	void SetStayOnTop( bool b );
	bool GetStayOnTop() const;

	int ShowModal();
	void closeModal();
	bool IsModal() const { return in_mode_; }
	void Close();

	void GetClientRect( struct tTVPRect& rt );

	// メッセージハンドラ
	virtual void OnActive( HWND preactive ) {}
	virtual void OnDeactive( HWND postactive ) {}
	virtual void OnClose( CloseAction& action ){}
	virtual bool OnCloseQuery() { return true; }
	virtual void OnFocus(HWND hFocusLostWnd) {}
	virtual void OnFocusLost(HWND hFocusingWnd) {}
	virtual void OnMouseDown( int button, int shift, int x, int y ){}
	virtual void OnMouseUp( int button, int shift, int x, int y ){}
	virtual void OnMouseMove( int shift, int x, int y ){}
	virtual void OnMouseDoubleClick( int button, int x, int y ) {}
	virtual void OnMouseClick( int button, int shift, int x, int y ){}
	virtual void OnMouseWheel( int delta, int shift, int x, int y ){}
	virtual void OnKeyUp( WORD vk, int shift ){}
	virtual void OnKeyDown( WORD vk, int shift, int repeat, bool prevkeystate ){}
	virtual void OnKeyPress( WORD vk, int repeat, bool prevkeystate, bool convertkey ){}
	virtual void OnMove( int x, int y ) {}
	virtual void OnResize( UINT_PTR state, int w, int h ) {}
	virtual void OnDropFile( HDROP hDrop ) {}
	virtual int OnMouseActivate( HWND hTopLevelParentWnd, WORD hitTestCode, WORD MouseMsg ) { return MA_ACTIVATE; }
	virtual bool OnSetCursor( HWND hContainsCursorWnd, WORD hitTestCode, WORD MouseMsg ) { return false; }
	virtual void OnEnable( bool enabled ) {}
	virtual void OnEnterMenuLoop( bool entered ) {}
	virtual void OnExitMenuLoop( bool isShortcutMenu ) {}
	virtual void OnDeviceChange( UINT_PTR event, void *data ) {}
	virtual void OnNonClientMouseDown( int button, UINT_PTR hittest, int x, int y ){}
	virtual void OnMouseEnter() {}
	virtual void OnMouseLeave() {}
	virtual void OnShow( UINT_PTR status ) {}
	virtual void OnHide( UINT_PTR status ) {}

	virtual void OnTouchDown( double x, double y, double cx, double cy, DWORD id, DWORD tick ) {}
	virtual void OnTouchMove( double x, double y, double cx, double cy, DWORD id, DWORD tick ) {}
	virtual void OnTouchUp( double x, double y, double cx, double cy, DWORD id, DWORD tick ) {}
	virtual void OnTouchSequenceStart() {}
	virtual void OnTouchSequenceEnd() {}

	virtual void OnDisplayChange( UINT_PTR bpp, WORD hres, WORD vres ) {}
	virtual void OnApplicationActivateChange( bool activated, DWORD thread_id ) {}
};
#endif

enum tTVPWMRRegMode { wrmRegister = 0, wrmUnregister = 1 };
enum {
	orientUnknown,
	orientPortrait,
	orientLandscape,
};

namespace cocos2d {
	class Node;
}

class iWindowLayer {
protected:
	tTVPMouseCursorState MouseCursorState = mcsVisible;
	tjs_int HintDelay = 500;
	tjs_int ZoomDenom = 1; // Zooming factor denominator (setting)
	tjs_int ZoomNumer = 1; // Zooming factor numerator (setting)
	double TouchScaleThreshold = 5, TouchRotateThreshold = 5;

public:
	virtual void SetPaintBoxSize(tjs_int w, tjs_int h) = 0;
	virtual bool GetFormEnabled() = 0;
	virtual void SetDefaultMouseCursor() = 0;
	virtual void GetCursorPos(tjs_int &x, tjs_int &y) = 0;
	virtual void SetCursorPos(tjs_int x, tjs_int y) = 0;
	virtual void SetHintText(const ttstr &text) = 0;
	virtual void SetAttentionPoint(tjs_int left, tjs_int top, const struct tTVPFont * font) = 0;
	virtual void ZoomRectangle(
		tjs_int & left, tjs_int & top,
		tjs_int & right, tjs_int & bottom) = 0;
	virtual void BringToFront() = 0;
	virtual void ShowWindowAsModal() = 0;
	virtual bool GetVisible() = 0;
	virtual void SetVisible(bool bVisible) = 0;
	virtual const char *GetCaption() = 0;
	virtual void SetCaption(const std::string &) = 0;
	virtual void SetWidth(tjs_int w) = 0;
	virtual void SetHeight(tjs_int h) = 0;
	virtual void SetSize(tjs_int w, tjs_int h) = 0;
	virtual void GetSize(tjs_int &w, tjs_int &h) = 0;
	virtual tjs_int GetWidth() const = 0;
	virtual tjs_int GetHeight() const = 0;
	virtual void GetWinSize(tjs_int &w, tjs_int &h) = 0;
	virtual void SetZoom(tjs_int numer, tjs_int denom) = 0;
	virtual void UpdateDrawBuffer(iTVPTexture2D *tex) = 0;
#if 0
	virtual void AddOverlay(tTJSNI_BaseVideoOverlay *ovl) = 0;
	virtual void RemoveOverlay(tTJSNI_BaseVideoOverlay *ovl) = 0;
	virtual void UpdateOverlay() = 0;
#endif
	virtual void InvalidateClose() = 0;
	virtual bool GetWindowActive() = 0;
	virtual void Close() = 0;
	virtual void OnCloseQueryCalled(bool b) = 0;
	virtual void InternalKeyDown(tjs_uint16 key, tjs_uint32 shift) = 0;
	virtual void OnKeyUp(tjs_uint16 vk, int shift) = 0;
	virtual void OnKeyPress(tjs_uint16 vk, int repeat, bool prevkeystate, bool convertkey) = 0;
	virtual tTVPImeMode GetDefaultImeMode() const = 0;
	virtual void SetImeMode(tTVPImeMode mode) = 0;
	virtual void ResetImeMode() = 0;
	virtual void UpdateWindow(tTVPUpdateType type) = 0;
	virtual void SetVisibleFromScript(bool b) = 0;
	virtual void SetUseMouseKey(bool b) = 0;
	virtual bool GetUseMouseKey() const = 0;
	virtual void ResetMouseVelocity() = 0;
	virtual void ResetTouchVelocity(tjs_int id) = 0;
	virtual bool GetMouseVelocity(float& x, float& y, float& speed) const = 0;
	virtual void TickBeat() = 0;
	virtual cocos2d::Node *GetPrimaryArea() = 0;

	void SetZoomNumer(tjs_int n) { SetZoom(n, ZoomDenom); }
	tjs_int GetZoomNumer() const { return ZoomNumer; }
	void SetZoomDenom(tjs_int d) { SetZoom(ZoomNumer, d); }
	tjs_int GetZoomDenom() const { return ZoomDenom; }

	// dummy function
	void RegisterWindowMessageReceiver(tTVPWMRRegMode mode, void * proc, const void *userdata) {}
	void SetLeft(tjs_int) {}
	void SetTop(tjs_int) {}
	void SetMinWidth(tjs_int) {}
	void SetMaxWidth(tjs_int) {}
	void SetMinHeight(tjs_int) {}
	void SetMaxHeight(tjs_int) {}
	void SetInnerWidth(tjs_int v) { SetWidth(v); }
	void SetInnerHeight(tjs_int v) { SetHeight(v); }
	void SetInnerSize(tjs_int w, tjs_int h) { SetSize(w, h); }
	void SetMinSize(tjs_int, tjs_int) {}
	void SetMaxSize(tjs_int, tjs_int) {}
	void SetPosition(tjs_int, tjs_int) {}
	void SetBorderStyle(tTVPBorderStyle) {}
	void SetStayOnTop(bool) {}
	void SetFullScreenMode(bool) {}
	tjs_int GetLeft() { return 0; }
	tjs_int GetTop() { return 0; }
	tjs_int GetMinWidth() { return 0; }
	tjs_int GetMaxWidth() { return 1920; }
	tjs_int GetMinHeight() { return 0; }
	tjs_int GetMaxHeight() { return 1080; }
	tjs_int GetInnerWidth() { return GetWidth(); }
	tjs_int GetInnerHeight() { return GetHeight(); }
	bool GetStayOnTop() { return false; }
	bool GetFullScreenMode() { return false; }
	tTVPBorderStyle GetBorderStyle() const { return bsNone; }
	void SetTrapKey(bool b) {}
	bool GetTrapKey() const { return false; }
	void RemoveMaskRegion() {}
	void SetMouseCursorState(tTVPMouseCursorState mcs) { MouseCursorState = mcs; }
	tTVPMouseCursorState GetMouseCursorState() const { return MouseCursorState; }
	void HideMouseCursor() { }
	void SetFocusable(bool b) {}
	bool GetFocusable() const { return true; }
	int GetDisplayRotate() { return 0; }
	int GetDisplayOrientation() { return orientLandscape; }
	void SetEnableTouch(bool b) {}
	bool GetEnableTouch() const { return false; }
	void SetHintDelay(tjs_int delay) { HintDelay = delay; }
	tjs_int GetHintDelay() const { return HintDelay; }
	void SetInnerSunken(bool b) {}
	bool GetInnerSunken() const { return false; }

	// TODO
	void SetMouseCursor(tjs_int handle) {}
	void SetHintText(iTJSDispatch2* sender, const ttstr &text) {}
	void DisableAttentionPoint() {}
	void GetVideoOffset(tjs_int &ofsx, tjs_int &ofsy) { ofsx = 0; ofsy = 0; }
	void SetTouchScaleThreshold(double threshold) { TouchScaleThreshold = threshold; }
	double GetTouchScaleThreshold() { return TouchScaleThreshold; }
	void SetTouchRotateThreshold(double threshold) { TouchRotateThreshold = threshold; }
	double GetTouchRotateThreshold() { return TouchRotateThreshold; }
	tjs_real GetTouchPointStartX(tjs_int index) const { return 0; }
	tjs_real GetTouchPointStartY(tjs_int index) const { return 0; }
	tjs_real GetTouchPointX(tjs_int index) const { return 0; }
	tjs_real GetTouchPointY(tjs_int index) const { return 0; }
	tjs_int GetTouchPointID(tjs_int index) const { return 0; }
	tjs_int GetTouchPointCount() const { return 0; }
	bool GetTouchVelocity(tjs_int id, float& x, float& y, float& speed) const { return false; }
	void ResetDrawDevice() {}
	void SendCloseMessage() {}
	void BeginMove() {}
	void SetLayerLeft(tjs_int l) {}
	tjs_int GetLayerLeft() const { return 0; }
	void SetLayerTop(tjs_int t) {}
	tjs_int GetLayerTop() const { return 0; }
	void SetLayerPosition(tjs_int l, tjs_int t) {}
	void SetShowScrollBars(bool b) {}
	bool GetShowScrollBars() const { return true; }
};

#endif // __TVP_WINDOW_H__
