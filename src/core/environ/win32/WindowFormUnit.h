
#ifndef __WINDOW_FORM_UNIT_H__
#define __WINDOW_FORM_UNIT_H__

#include "tjsCommHead.h"
#include "tvpinputdefs.h"
#include "WindowIntf.h"

#include "TVPWindow.h"
#include "MouseCursor.h"
#include "TouchPoint.h"
#include "TVPTimer.h"
#include "VelocityTracker.h"

enum {
	crDefault = 0x0,
	crNone = -1,
	crArrow = -2,
	crCross = -3,
	crIBeam = -4,
	crSize = -5,
	crSizeNESW = -6,
	crSizeNS = -7,
	crSizeNWSE = -8,
	crSizeWE = -9,
	crUpArrow = -10,
	crHourGlass = -11,
	crDrag = -12,
	crNoDrop = -13,
	crHSplit = -14,
	crVSplit = -15,
	crMultiDrag = -16,
	crSQLWait = -17,
	crNo = -18,
	crAppStart = -19,
	crHelp = -20,
	crHandPoint = -21,
	crSizeAll = -22,
	crHBeam = 1,
};

//---------------------------------------------------------------------------
// Options
//---------------------------------------------------------------------------
extern void TVPInitWindowOptions();
extern int TVPFullScreenBPP; // = 0; // 0 for no-change
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// VCL-based constants to TVP-based constants conversion (and vice versa)
//---------------------------------------------------------------------------
tTVPMouseButton TVP_TMouseButton_To_tTVPMouseButton(int button);
tjs_uint32 TVP_TShiftState_To_uint32(tjs_uint32 state);
tjs_uint32 TVP_TShiftState_From_uint32(tjs_uint32 state);
tjs_uint32 TVPGetCurrentShiftKeyState();
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TTVPWindowForm
//---------------------------------------------------------------------------
#define TVP_WM_SHOWVISIBLE (WM_USER + 2)
#define TVP_WM_SHOWTOP     (WM_USER + 3)
#define TVP_WM_RETRIEVEFOCUS     (WM_USER + 4)
#define TVP_WM_ACQUIREIMECONTROL    (WM_USER + 5)
extern void TVPShowModalAtAppActivate();
extern void TVPHideModalAtAppDeactivate();
//extern HDWP TVPShowModalAtTimer(HDWP);
class TTVPWindowForm;
extern TTVPWindowForm * TVPFullScreenedWindow;
//---------------------------------------------------------------------------
struct tTVPMessageReceiverRecord
{
//	tTVPWindowMessageReceiver Proc;
	const void *UserData;
// 	bool Deliver(tTVPWindowMessage *Message)
// 	{ return Proc(const_cast<void*>(UserData), Message); }	
};
class tTJSNI_Window;
struct tTVPRect;
class tTVPBaseBitmap;
class tTVPWheelDirectInputDevice; // class for DirectInputDevice management
class tTVPPadDirectInputDevice; // class for DirectInputDevice management

class TTVPWindowForm : public tTVPWindow, public TouchHandler {
	static const int TVP_MOUSE_MAX_ACCEL = 30;
	static const int TVP_MOUSE_SHIFT_ACCEL = 40;
	static const int TVP_TOOLTIP_SHOW_DELAY = 500;
private:
	bool Focusable;

	//-- drawdevice related
	bool NextSetWindowHandleToDrawDevice;
	tTVPRect LastSentDrawDeviceDestRect;
	
	//-- interface to plugin
	tObjectList<tTVPMessageReceiverRecord> WindowMessageReceivers;
	
	//-- DirectInput related
	tTVPWheelDirectInputDevice *DIWheelDevice;
#ifndef DISABLE_EMBEDDED_GAME_PAD
	tTVPPadDirectInputDevice *DIPadDevice;
#endif
	bool ReloadDevice;
	DWORD ReloadDeviceTick;

	//-- TJS object related
	tTJSNI_Window * TJSNativeInstance;
	int LastMouseDownX, LastMouseDownY; // in Layer coodinates
	
	struct{ int x, y; } LastMouseMovedPos;  // in Layer coodinates
	//-- full screen managemant related
	int InnerWidthSave;
	int InnerHeightSave;
	DWORD OrgStyle;
	DWORD OrgExStyle;
	int OrgLeft;
	int OrgTop;
	int OrgWidth;
	int OrgHeight;
	int OrgClientWidth;
	int OrgClientHeight;
	tTVPRect FullScreenDestRect;

	//-- keyboard input
	std::string PendingKeyCodes;
	
	tTVPImeMode LastSetImeMode;
	tTVPImeMode DefaultImeMode;

	bool TrapKeys;
	bool CanReceiveTrappedKeys;
	bool InReceivingTrappedKeys;
	bool UseMouseKey; // whether using mouse key emulation
	tjs_int MouseKeyXAccel;
	tjs_int MouseKeyYAccel;
	bool LastMouseMoved;
	bool MouseLeftButtonEmulatedPushed;
	bool MouseRightButtonEmulatedPushed;
	DWORD LastMouseKeyTick;
	
	bool AttentionPointEnabled;
	struct {
		int x, y;
	} AttentionPoint;
	class tTVPSysFont *AttentionFont;

	//-- mouse cursor
	tTVPMouseCursorState MouseCursorState;
	bool ForceMouseCursorVisible; // true in menu select
	MouseCursor MouseCursorManager;
	tjs_int CurrentMouseCursor;
	tjs_int LastMouseScreenX; // managed by RestoreMouseCursor
	tjs_int LastMouseScreenY;

	//-- layer position / size
	tjs_int LayerLeft;
	tjs_int LayerTop;
	tjs_int LayerWidth;
	tjs_int LayerHeight;
	tjs_int ZoomDenom; // Zooming factor denominator (setting)
	tjs_int ZoomNumer; // Zooming factor numerator (setting)
	tjs_int ActualZoomDenom; // Zooming factor denominator (actual)
	tjs_int ActualZoomNumer; // Zooming factor numerator (actual)

	DWORD LastRecheckInputStateSent;

	TouchPointList touch_points_;
	ttstr HintMessage;
	TVPTimer* HintTimer;
	tjs_int HintDelay;
	iTJSDispatch2* LastHintSender;

	tjs_int DisplayOrientation;
	tjs_int DisplayRotate;

	VelocityTrackers TouchVelocityTracker;
	VelocityTracker MouseVelocityTracker;
private:
	void SetDrawDeviceDestRect();
	void TranslateWindowToDrawArea(int &x, int &y);
	void TranslateWindowToDrawArea(double&x, double &y);
	void TranslateDrawAreaToWindow(int &x, int &y);

	void FirePopupHide();
	bool CanSendPopupHide() const { return !Focusable && GetVisible() && GetStayOnTop(); }
	
	void RestoreMouseCursor();
	void SetMouseCursorVisibleState(bool b);
	void SetForceMouseCursorVisible(bool s);
	
	void InternalSetPaintBoxSize();

	void CallWindowDetach(bool close);
	void CallWindowAttach();
	void CallFullScreenChanged();
	void CallFullScreenChanging();
	
// 	bool InternalDeliverMessageToReceiver(tTVPWindowMessage &msg);
// 	bool DeliverMessageToReceiver(tTVPWindowMessage &msg) {
// 		if( WindowMessageReceivers.GetCount() )
// 			return InternalDeliverMessageToReceiver(msg);
// 		else
// 			return false;
// 	}
	void GenerateMouseEvent(bool fl, bool fr, bool fu, bool fd);

	void UnacquireImeControl();
	void AcquireImeControl();
	
	TTVPWindowForm * GetKeyTrapperWindow();

	int ConvertImeMode( tTVPImeMode mode );
	void OffsetClientPoint( int &x, int &y );
	
// 	static bool FindKeyTrapper(LRESULT &result, UINT msg, WPARAM wparam, LPARAM lparam);
// 	bool ProcessTrappedKeyMessage(LRESULT &result, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
//	LRESULT WINAPI Proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

public:
	TTVPWindowForm( class tTVPApplication* app, tTJSNI_Window* ni, tTJSNI_Window* parent = NULL );
	virtual ~TTVPWindowForm();
	
	static void DeliverPopupHide();

	//-- properties
	std::wstring GetCaption() const {
		std::wstring ret;
		tTVPWindow::GetCaption( ret );
		return ret;
	}

	void CleanupFullScreen();

	void SetUseMouseKey(bool b);
	bool GetUseMouseKey() const;

	void SetTrapKey(bool b);
	bool GetTrapKey() const;

	void SetMaskRegion(HRGN threshold);
	void RemoveMaskRegion();

	void SetMouseCursorToWindow( tjs_int cursor );

	void HideMouseCursor();
	void SetMouseCursorState(tTVPMouseCursorState mcs);
    tTVPMouseCursorState GetMouseCursorState() const { return MouseCursorState; }

	void SetFocusable(bool b);
	bool GetFocusable() const { return Focusable; }

	void AdjustNumerAndDenom(tjs_int &n, tjs_int &d);
	void SetZoom(tjs_int numer, tjs_int denom, bool set_logical = true);
	void SetZoomNumer( tjs_int n ) { SetZoom(n, ZoomDenom); }
	tjs_int GetZoomNumer() const { return ZoomNumer; }
	void SetZoomDenom(tjs_int d) { SetZoom(ZoomNumer, d); }
	tjs_int GetZoomDenom() const { return ZoomDenom; }
	
	//-- full screen management
	void SetFullScreenMode(bool b);
	bool GetFullScreenMode() const;
	void RelocateFullScreenMode();

	//-- methods/properties
	void UpdateWindow(tTVPUpdateType type = utNormal);
	void ShowWindowAsModal();

	void SetVisibleFromScript(bool b);

	void RegisterWindowMessageReceiver(tTVPWMRRegMode mode, void * proc, const void *userdata);

	//-- close action related
	bool Closing;
	bool ProgramClosing;
	bool CanCloseWork;
	void Close();
	void InvalidateClose();
	void OnCloseQueryCalled(bool b);
	void SendCloseMessage();
	
	void ZoomRectangle( tjs_int & left, tjs_int & top, tjs_int & right, tjs_int & bottom);
	void GetVideoOffset(tjs_int &ofsx, tjs_int &ofsy);

	HWND GetSurfaceWindowHandle() { return GetHandle(); }
	HWND GetWindowHandle() { return GetHandle(); }
	HWND GetWindowHandleForPlugin() { return GetHandle(); }

	//-- form mode
	bool GetFormEnabled();
	void TickBeat(); // called every 50ms intervally
	bool GetWindowActive();

	//-- draw device
	void ResetDrawDevice();

	void InternalKeyUp(WORD key, tjs_uint32 shift);
	void InternalKeyDown(WORD key, tjs_uint32 shift);

	
	void SetPaintBoxSize(tjs_int w, tjs_int h);

	void SetDefaultMouseCursor();
	void SetMouseCursor(tjs_int handle);

	void GetCursorPos(tjs_int &x, tjs_int &y);
	void SetCursorPos(tjs_int x, tjs_int y);

	void SetHintText(iTJSDispatch2* sender, const ttstr &text);
	void UpdateHint();

	void SetImeMode(tTVPImeMode mode);
	void SetDefaultImeMode(tTVPImeMode mode, bool reset);
	tTVPImeMode GetDefaultImeMode() const { return  DefaultImeMode; }
	void ResetImeMode();
	
	void SetAttentionPoint(tjs_int left, tjs_int top, const struct tTVPFont * font );
	void DisableAttentionPoint();
	
	void InvokeShowVisible();
	void InvokeShowTop(bool activate = true);
//	HDWP ShowTop(HDWP hdwp);

	//-- DirectInput related
	void CreateDirectInputDevice();
	void FreeDirectInputDevice();

	int GetDisplayOrientation() { UpdateOrientation(); return DisplayOrientation; }
	int GetDisplayRotate() { UpdateOrientation(); return DisplayRotate; }

	// message hander
	virtual void OnActive( HWND preactive );
	virtual void OnDeactive( HWND postactive );

	virtual void OnKeyDown( WORD vk, int shift, int repeat, bool prevkeystate );
	virtual void OnKeyUp( WORD vk, int shift );
	virtual void OnKeyPress( WORD vk, int repeat, bool prevkeystate, bool convertkey );

	virtual void OnPaint();
	virtual void OnClose( CloseAction& action );
	virtual bool OnCloseQuery();
	virtual void OnMouseDown( int button, int shift, int x, int y );
	virtual void OnMouseUp( int button, int shift, int x, int y );
	virtual void OnMouseMove( int shift, int x, int y );
	virtual void OnMouseDoubleClick( int button, int x, int y );
	virtual void OnMouseClick( int button, int shift, int x, int y );
	virtual void OnMouseWheel( int delta, int shift, int x, int y );

	virtual void OnMove( int x, int y );
	virtual void OnResize( UINT_PTR state, int w, int h );
//	virtual void OnDropFile( HDROP hDrop );
	virtual int OnMouseActivate( HWND hTopLevelParentWnd, WORD hitTestCode, WORD MouseMsg );
	virtual bool OnSetCursor( HWND hContainsCursorWnd, WORD hitTestCode, WORD MouseMsg );
	virtual void OnEnable( bool enabled );
	virtual void OnDeviceChange( UINT_PTR event, void *data );
	virtual void OnNonClientMouseDown( int button, UINT_PTR hittest, int x, int y );
	virtual void OnMouseEnter();
	virtual void OnMouseLeave();
	virtual void OnEnterMenuLoop( bool entered );
	virtual void OnExitMenuLoop( bool isShortcutMenu );
	virtual void OnShow( UINT_PTR status );
	virtual void OnHide( UINT_PTR status );

	virtual void OnFocus(HWND hFocusLostWnd);
	virtual void OnFocusLost(HWND hFocusingWnd);
	
	virtual void OnTouchDown( double x, double y, double cx, double cy, DWORD id, DWORD tick );
	virtual void OnTouchMove( double x, double y, double cx, double cy, DWORD id, DWORD tick );
	virtual void OnTouchUp( double x, double y, double cx, double cy, DWORD id, DWORD tick );
	virtual void OnTouchSequenceStart();
	virtual void OnTouchSequenceEnd();

	virtual void OnTouchScaling( double startdist, double currentdist, double cx, double cy, int flag );
	virtual void OnTouchRotate( double startangle, double currentangle, double distance, double cx, double cy, int flag );
	virtual void OnMultiTouch();

	virtual void OnDisplayChange( UINT_PTR bpp, WORD hres, WORD vres );
	virtual void OnDisplayRotate( int orientation, int rotate, int bpp, int hresolution, int vresolution );

	virtual void OnApplicationActivateChange( bool activated, DWORD thread_id );

	virtual void OnDestroy();
#if 0
	void WMShowVisible();
	void WMShowTop( WPARAM wParam );
	void WMRetrieveFocus();
	void WMAcquireImeControl();
#endif
	void SetTouchScaleThreshold( double threshold ) {
		touch_points_.SetScaleThreshold( threshold );
	}
	double GetTouchScaleThreshold() const {
		return touch_points_.GetScaleThreshold();
	}
	void SetTouchRotateThreshold( double threshold ) {
		touch_points_.SetRotateThreshold( threshold );
	}
	double GetTouchRotateThreshold() const {
		return touch_points_.GetRotateThreshold();
	}
	tjs_real GetTouchPointStartX( tjs_int index ) const { return touch_points_.GetStartX(index); }
	tjs_real GetTouchPointStartY( tjs_int index ) const { return touch_points_.GetStartY(index); }
	tjs_real GetTouchPointX( tjs_int index ) const { return touch_points_.GetX(index); }
	tjs_real GetTouchPointY( tjs_int index ) const { return touch_points_.GetY(index); }
	tjs_int GetTouchPointID( tjs_int index ) const { return touch_points_.GetID(index); }
	tjs_int GetTouchPointCount() const { return touch_points_.CountUsePoint(); }
	bool GetTouchVelocity( tjs_int id, float& x, float& y, float& speed ) const {
		return TouchVelocityTracker.getVelocity( id, x, y, speed );
	}
	bool GetMouseVelocity( float& x, float& y, float& speed ) const {
		if( MouseVelocityTracker.getVelocity( x, y ) ) {
			speed = hypotf(x, y);
			return true;
		}
		return false;
	}
	void ResetMouseVelocity() {
		MouseVelocityTracker.clear();
	}
	void ResetTouchVelocity( tjs_int id ) {
		TouchVelocityTracker.end( id );
	}

	void SetHintDelay( tjs_int delay ) { HintDelay = delay; }
	tjs_int GetHintDelay() const { return HintDelay; }

	void SetEnableTouch( bool b );
	bool GetEnableTouch() const;

	void UpdateOrientation();
	bool GetOrientation( int& orientation, int& rotate ) const;
};

#endif // __WINDOW_FORM_UNIT_H__
