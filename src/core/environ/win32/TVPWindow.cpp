
#include "tjsCommHead.h"

#include "WindowIntf.h"
#include "ComplexRect.h"
#include "TVPWindow.h"
#include <vector>
#include <windowsx.h>
#include "SystemControl.h"
#include "Exception.h"
#include "Application.h"
#include "Resource.h"
#include "CompatibleNativeFuncs.h"
#include "WindowsUtil.h"
#include "MsgIntf.h"
#include "UserEvent.h"
#include "TickCount.h"
#include "SysInitIntf.h"

#include <TCHAR.h>
#include <tpcshrd.h> // for MICROSOFT_TABLETPENSERVICE_PROPERTY

// touch mouse message extraInfo (cf. http://msdn.microsoft.com/en-us/library/windows/desktop/ms703320(v=vs.85).aspx )
const DWORD tTVPWindow::MI_WP_SIGNATURE = 0xFF515780;
const DWORD tTVPWindow::SIGNATURE_MASK  = 0xFFFFFF80;

tTVPWindow::~tTVPWindow() {
	if( ime_control_ ) delete ime_control_;
	::SetWindowLongPtr( window_handle_, GWLP_WNDPROC, (LONG_PTR)::DefWindowProc );
	::SetWindowLongPtr( window_handle_, GWLP_USERDATA, (LONG_PTR)NULL );
}
LRESULT WINAPI tTVPWindow::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	tTVPWindow *win = reinterpret_cast<tTVPWindow*>(GetWindowLongPtr(hWnd,GWLP_USERDATA));
	if( win != NULL ) {
		return win->Proc( hWnd, msg, wParam, lParam );
	}
	return ::DefWindowProc(hWnd,msg,wParam,lParam);
}

LRESULT WINAPI tTVPWindow::Proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	// マウスメッセージがきた時、Mouse Enter の判定を行う
	if( (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) && msg != WM_MOUSELEAVE ) {
		if( in_window_ == false ) {
			OnMouseEnter();
			in_window_ = true;
			TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
			tme.dwHoverTime = HOVER_DEFAULT;
            ::TrackMouseEvent( &tme ); // ここでエラーをハンドリングしてもあまり意味無いので無視
		}
	}

	switch( msg ) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint( hWnd, &ps );
		EndPaint( hWnd, &ps );
		if( created_ ) OnPaint();
		return 0;
	}
	case WM_CLOSE: {
		Close();
		return 0;
	}

	case WM_MOUSELEAVE:
		if( in_window_ ) {
			OnMouseLeave();
			in_window_ = false;
		}
		return 0;

	case WM_MOUSEWHEEL:
		OnMouseWheel( GET_WHEEL_DELTA_WPARAM(wParam), GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		return 0;

	case WM_MOUSEMOVE:
		if( ignore_touch_mouse_ == false || IsTouchEvent( ::GetMessageExtraInfo() ) == false ) {
			OnMouseMove( GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		}
		return 0;

	case WM_LBUTTONDOWN:
		if( ignore_touch_mouse_ == false || IsTouchEvent( ::GetMessageExtraInfo() ) == false ) {
			left_double_click_ = false;
			OnMouseDown( mbLeft, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		}
		return 0;
	case WM_LBUTTONUP:
		if( ignore_touch_mouse_ == false || IsTouchEvent( ::GetMessageExtraInfo() ) == false ) {
			if( left_double_click_ == false ) {
				OnMouseClick( mbLeft, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
			}
			left_double_click_ = false;
			OnMouseUp( mbLeft, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) ); 
		}
		return 0;
	case WM_LBUTTONDBLCLK:
		if( ignore_touch_mouse_ == false || IsTouchEvent( ::GetMessageExtraInfo() ) == false ) {
			left_double_click_ = true;
			OnMouseDoubleClick( mbLeft, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
			OnMouseDown( mbLeft, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		}
		return 0;

	case WM_RBUTTONDOWN:
		OnMouseDown( mbRight, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		return 0;
	case WM_RBUTTONUP:
		OnMouseUp( mbRight, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		return 0;
	case WM_RBUTTONDBLCLK: // 右ダブルクリックは無視
		return ::DefWindowProc(hWnd,msg,wParam,lParam);

	case WM_MBUTTONDOWN:
		OnMouseDown( mbMiddle, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		return 0;
	case WM_MBUTTONUP:
		OnMouseUp( mbMiddle, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		return 0;
	case WM_MBUTTONDBLCLK: // 中ダブルクリックは無視
		return ::DefWindowProc(hWnd,msg,wParam,lParam);

	case WM_XBUTTONDBLCLK: // Xダブルクリックは無視
		return ::DefWindowProc(hWnd,msg,wParam,lParam);
	case WM_XBUTTONUP:
	    switch(GET_XBUTTON_WPARAM(wParam)){
        case XBUTTON1: // サイドキーが第1Xボタン
            OnMouseUp( mbX1, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
			return 0;
        case XBUTTON2: // サイドキーが第2Xボタン
            OnMouseUp( mbX2, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
			return 0;
		}
		return ::DefWindowProc(hWnd,msg,wParam,lParam);
	case WM_XBUTTONDOWN: // 戻るや進むが割り当てられる
	    switch(GET_XBUTTON_WPARAM(wParam)){
        case XBUTTON1: // サイドキーが第1Xボタン
            OnMouseDown( mbX1, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
			return 0;
        case XBUTTON2: // サイドキーが第2Xボタン
            OnMouseDown( mbX2, GetShiftState(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
			return 0;
		}
		return ::DefWindowProc(hWnd,msg,wParam,lParam);
	case WM_TOUCH: {
		// user32.dll から GetTouchInputInfo など読み込む
		if( procGetTouchInputInfo && procCloseTouchInputHandle ) {
			UINT cInputs = LOWORD(wParam);
			PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
			if( NULL != pInputs ) {
				OnTouchSequenceStart();
				try {
					DWORD basetick = TVPGetRoughTickCount32();
					if( procGetTouchInputInfo( (HTOUCHINPUT)lParam, cInputs, pInputs, sizeof(TOUCHINPUT)) ) {
						// process pInputs
						for( UINT i = 0; i < cInputs; i++ ) {
							int x = pInputs[i].x / 100;
							int y = pInputs[i].y / 100;
							POINT pt = {x,y};
							::ScreenToClient( GetHandle(), &pt );
							int lx = pInputs[i].x % 100;
							int ly = pInputs[i].y % 100;
							double vx = pt.x + static_cast<double>(lx)/100.0;
							double vy = pt.y + static_cast<double>(ly)/100.0;
							double cx = 1;
							double cy = 1;
							if( pInputs[i].dwMask & TOUCHINPUTMASKF_CONTACTAREA ) {
								cx = static_cast<double>(pInputs[i].cxContact) / 100.0;
								cy = static_cast<double>(pInputs[i].cyContact) / 100.0;
							}
							DWORD tick;
							if( pInputs[i].dwMask & TOUCHINPUTMASKF_TIMEFROMSYSTEM ) {
								tick = pInputs[i].dwTime;
							} else {
								tick = basetick;
							}
							if( pInputs[i].dwFlags & TOUCHEVENTF_DOWN ) {
								OnTouchDown( vx, vy, cx, cy, pInputs[i].dwID, tick );
							}
							if( pInputs[i].dwFlags & TOUCHEVENTF_MOVE ) {
								OnTouchMove( vx, vy, cx, cy, pInputs[i].dwID, tick );
							}
							if( pInputs[i].dwFlags & TOUCHEVENTF_UP ) {
								OnTouchUp( vx, vy, cx, cy, pInputs[i].dwID, tick );
							}
						}
						if( !procCloseTouchInputHandle((HTOUCHINPUT)lParam) ) {
							// error handling
							TVPThrowWindowsErrorException();
						}
					} else {
						TVPThrowWindowsErrorException();
					}
				} catch(...) {
					OnTouchSequenceEnd();
					delete[] pInputs;
					throw;
				}
				OnTouchSequenceEnd();
				delete[] pInputs;
			} else {
				// error handling, presumably out of memory
				TVPThrowExceptionMessage( TVPInsufficientMemory );
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	case WM_GESTURE:
		if( procGetGestureInfo && procCloseGestureInfoHandle ) {
			GESTUREINFO gi;
			ZeroMemory(&gi, sizeof(GESTUREINFO));
			gi.cbSize = sizeof(gi);
			BOOL bResult = procGetGestureInfo((HGESTUREINFO)lParam, &gi);
			BOOL bHandled = FALSE;
			switch (gi.dwID){
			case GID_ZOOM:
				// Code for zooming goes here
				// gi.ullArguments distance
				bHandled = TRUE;
				break;
			case GID_PAN:
				// gi.ullArguments distance
				bHandled = TRUE;
				break;
			case GID_ROTATE: {
				// double rot = GID_ROTATE_ANGLE_FROM_ARGUMENT(gi.ullArguments)
				/*
				POINT cpt;
				cpt.x = gi.ptsLocation.x;
				cpt.y = gi.ptsLocation.y;
				::ScreenToClient(gi.hwndTarget,&cpt);
				*/
				bHandled = TRUE;
				break;
			}
			case GID_TWOFINGERTAP:
				bHandled = TRUE;
				break;
			case GID_PRESSANDTAP:
				bHandled = TRUE;
				break;
			default:
				// You have encountered an unknown gesture
				break;
			}
			procCloseGestureInfoHandle((HGESTUREINFO)lParam);
			if( bHandled ) return 0;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		OnKeyDown( (WORD)wParam, GetShiftState(), lParam&0xffff, (lParam&(1<<30))?true:false );
		return ::DefWindowProc(hWnd,msg,wParam,lParam);
	case WM_SYSKEYUP:
	case WM_KEYUP:
		OnKeyUp( (WORD)wParam, GetShiftState() );
		return ::DefWindowProc(hWnd,msg,wParam,lParam);
	case WM_SYSCHAR:
	case WM_CHAR:
		OnKeyPress( (WORD)wParam, lParam&0xffff, (lParam&(1<<30))?true:false, (lParam&(1<<31))?true:false );
		return ::DefWindowProc(hWnd,msg,wParam,lParam);

	case WM_SETFOCUS:
		OnFocus( reinterpret_cast<HWND>(wParam) );
		return 0;
	case WM_KILLFOCUS:
		OnFocusLost( reinterpret_cast<HWND>(wParam) );
		return 0;
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* lpmmi = (LPMINMAXINFO)lParam;
			// lpmmi->ptMaxPosition 最大化時の位置
			// lpmmi->ptMaxSize 最大化時のサイズ
			// 最小サイズ
			if( min_size_.cx > 0 ) {
				lpmmi->ptMinTrackSize.x = min_size_.cx;
			}
			if( min_size_.cy > 0 ) {
				lpmmi->ptMinTrackSize.y = min_size_.cy;
			}
			if( max_size_.cx > 0 ) {
				lpmmi->ptMaxTrackSize.x = max_size_.cx; // サイズ変更時の最大サイズ
			} else {
				lpmmi->ptMaxTrackSize.x = INT_MAX; // サイズ変更時の最大サイズ
			}
			if( max_size_.cy > 0 ) {
				lpmmi->ptMaxTrackSize.y = max_size_.cy; // サイズ変更時の最大サイズ
			} else {
				lpmmi->ptMaxTrackSize.y = INT_MAX; // サイズ変更時の最大サイズ
			}
			return 0;
		}
	case WM_ACTIVATE: {
		WPARAM fActive =  wParam & 0xFFFF;
		if( fActive == WA_INACTIVE ) {
			OnDeactive( (HWND)lParam );
		} else if( fActive == WA_ACTIVE || fActive == WA_CLICKACTIVE ) {
			OnActive( (HWND)lParam );
		}
		return ::DefWindowProc(hWnd,msg,wParam,lParam);
	}
	case WM_SYSCOMMAND:
		if( wParam == SC_CLOSE ) {
			if( in_mode_ ) modal_result_ = mrCancel;
		} else if( wParam == SC_KEYMENU ) {
			if( HasMenu( hWnd ) == false ) {
				return 0; // メニューがない時は、コマンドを消費して、マウスカーソル入力のロストを防ぐ
			}
		}
		return ::DefWindowProc(hWnd,msg,wParam,lParam);
	case WM_DESTROY:
		OnDestroy();
		return 0;
	case WM_MOVE:
		OnMove( GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		break;
	case WM_SIZE:
		OnResize( wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		break;
	case WM_DROPFILES:
		OnDropFile( reinterpret_cast<HDROP>(wParam) );
		return 0;
	case WM_MOUSEACTIVATE:
		return OnMouseActivate( reinterpret_cast<HWND>(wParam), LOWORD(lParam), HIWORD(lParam) );
	case WM_SETCURSOR:
		if( OnSetCursor( reinterpret_cast<HWND>(wParam), LOWORD(lParam), HIWORD(lParam) ) ) return 1;
		break;
	case WM_ENABLE:
		OnEnable( wParam != 0 );
		break;
	case WM_ENTERMENULOOP:
		OnEnterMenuLoop( wParam != 0 );
		break;
	case WM_EXITMENULOOP:
		OnExitMenuLoop( wParam != 0 );
		break;
	case WM_DEVICECHANGE:
		OnDeviceChange( wParam, reinterpret_cast<void*>(lParam) );
		break;
	case WM_NCLBUTTONDOWN:
		OnNonClientMouseDown( mbLeft, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		break;
	case WM_NCRBUTTONDOWN:
		OnNonClientMouseDown( mbRight, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) );
		break;
	case WM_SHOWWINDOW:
		if( wParam ) {
			OnShow( lParam );
		} else {
			OnHide( lParam );
		}
		break;
	case TVP_EV_WINDOW_RELEASE:
		::DestroyWindow( GetHandle() );
		delete this;
		break;
	case WM_DISPLAYCHANGE:
		// bpp, h resolution, v resolution
		OnDisplayChange( wParam, LOWORD(lParam), HIWORD(lParam) );
		break;
	case WM_ACTIVATEAPP:
		OnApplicationActivateChange( wParam != 0, (DWORD)lParam );
		break;
	default:
		return ::DefWindowProc(hWnd,msg,wParam,lParam);
	}
	return ::DefWindowProc(hWnd,msg,wParam,lParam);
}
void tTVPWindow::OnPaint() {
}
const DWORD tTVPWindow::DEFAULT_EX_STYLE = WS_EX_ACCEPTFILES | WS_EX_APPWINDOW;
const ULONG tTVPWindow::REGISTER_TOUCH_FLAG = TWF_WANTPALM|TWF_FINETOUCH;
const DWORD tTVPWindow::DEFAULT_TABLETPENSERVICE_PROPERTY = (
	TABLET_DISABLE_PRESSANDHOLD | // disables press and hold (right-click) gesture
	TABLET_DISABLE_PENTAPFEEDBACK | // disables UI feedback on pen up (waves)
	TABLET_DISABLE_PENBARRELFEEDBACK | // disables UI feedback on pen button down (circle)
	TABLET_DISABLE_FLICKS // disables pen flicks (back, forward, drag down, drag up)
	);

HRESULT tTVPWindow::CreateWnd( const std::wstring& classname, const std::wstring& title, int width, int height, HWND hParent )
{
	window_class_name_ = classname;
	window_title_ = title;
	window_client_size_.cx = width;
	window_client_size_.cy = height;

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC | CS_DBLCLKS, tTVPWindow::WndProc, 0L, 0L,
						GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
						window_class_name_.c_str(), NULL };
	wc.hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TVPWIN32));
	wc.hIconSm = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TVPWIN32));

	WNDCLASSEX tmpwc = { sizeof(WNDCLASSEX) };
	BOOL ClassRegistered = ::GetClassInfoEx( wc.hInstance, wc.lpszClassName, &tmpwc );
	if( ClassRegistered == 0 ) {
		if( ::RegisterClassEx( &wc ) == 0 ) {
#ifdef _DEBUG
			TVPOutputWindowsErrorToDebugMessage();
#endif
			return HRESULT_FROM_WIN32(::GetLastError());
		}
	}
	wc_ = wc;

	RECT	winRc = { 0, 0, window_client_size_.cx, window_client_size_.cy };
	::AdjustWindowRectEx( &winRc, WS_OVERLAPPEDWINDOW, NULL, DEFAULT_EX_STYLE );
	DWORD exStyle = DEFAULT_EX_STYLE;
	if( hParent ) {
		exStyle |= WS_EX_CONTROLPARENT;
		has_parent_ = true;
	}
	window_handle_ = ::CreateWindowEx( exStyle, window_class_name_.c_str(), window_title_.c_str(),
						WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, winRc.right-winRc.left, winRc.bottom-winRc.top,
						hParent, NULL, wc.hInstance, NULL );
	
	if( window_handle_ == NULL ) {
#ifdef _DEBUG
			TVPOutputWindowsErrorToDebugMessage();
#endif
		return HRESULT_FROM_WIN32(::GetLastError());
	}
	created_ = true;
	ime_control_ = new ImeControl(window_handle_);
	border_style_ = bsSizeable;
	
    ::SetWindowLongPtr( window_handle_, GWLP_WNDPROC, (LONG_PTR)tTVPWindow::WndProc );
	::SetWindowLongPtr( window_handle_, GWLP_USERDATA, (LONG_PTR)this );

	::ShowWindow(window_handle_,SW_HIDE);
	if( ::UpdateWindow(window_handle_) == 0 )
		return HRESULT_FROM_WIN32(::GetLastError());

	bool ignore_touch = false;
	tTJSVariant touch;
	if(TVPGetCommandLine(TJS_W("-ignoretouch"), &touch)) {
		ttstr str = touch;
		if(str == TJS_W("true")) ignore_touch = true;
	}
	// ハードがマルチタッチをサポートしているかどうか
	if( procRegisterTouchWindow && ignore_touch == false ) {
		int value= ::GetSystemMetrics( SM_DIGITIZER );
		if( (value & (NID_MULTI_INPUT|NID_READY)) == (NID_MULTI_INPUT|NID_READY) ) {
			// マルチタッチサポート & 準備できている
#ifndef TVP_TOUCH_DISABLE
			procRegisterTouchWindow( window_handle_, REGISTER_TOUCH_FLAG );
			ignore_touch_mouse_ = true;
#endif
			// MICROSOFT_TABLETPENSERVICE_PROPERTY プロパティを変更する
			ATOM atom = ::GlobalAddAtom( MICROSOFT_TABLETPENSERVICE_PROPERTY );
			::SetProp( window_handle_, MICROSOFT_TABLETPENSERVICE_PROPERTY, reinterpret_cast<HANDLE>( DEFAULT_TABLETPENSERVICE_PROPERTY ) );
			::GlobalDeleteAtom( atom );
		}
	}
	return S_OK;
}
void tTVPWindow::UnregisterWindow() {
	::UnregisterClass( window_class_name_.c_str(), wc_.hInstance );
}

void tTVPWindow::SetWidnowTitle( const std::wstring& title ) {
	if( window_title_ != title ) {
		window_title_ = title;
		if( window_handle_ ) {
			::SetWindowText( window_handle_, window_title_.c_str() );
		}
	}
}

void tTVPWindow::SetScreenSize( int width, int height ) {
	if( window_client_size_.cx != width || window_client_size_.cy != height ) {
		window_client_size_.cx = width;
		window_client_size_.cy = height;
		if( window_handle_ ) {
			RECT	winRc = { 0, 0, window_client_size_.cx, window_client_size_.cy };
			::AdjustWindowRect( &winRc, WS_OVERLAPPEDWINDOW, NULL );
			::SetWindowPos( window_handle_, HWND_TOP, 0, 0, winRc.right-winRc.left, winRc.bottom-winRc.top,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER );
		}
	}
}

bool tTVPWindow::Initialize() {
	return true;
}
void tTVPWindow::OnDestroy() {
	::SetWindowLongPtr( window_handle_, GWLP_WNDPROC, (LONG_PTR)::DefWindowProc );
	::SetWindowLongPtr( window_handle_, GWLP_USERDATA, (LONG_PTR)NULL );
}

bool tTVPWindow::HasMenu( HWND hWnd ) {
	HMENU hMenu = ::GetMenu( hWnd );
	if( hMenu ) {
		int count = ::GetMenuItemCount(hMenu);
		if( count <= 0 ) {
			hMenu = NULL;
		}
	}
	return hMenu != NULL;
}
void tTVPWindow::SetClientSize( HWND hWnd, SIZE& size ) {
	DWORD style = ::GetWindowLong( hWnd, GWL_STYLE );
	DWORD exStyle = ::GetWindowLong( hWnd, GWL_EXSTYLE );
	RECT rect;
	::SetRect( &rect, 0, 0, size.cx, size.cy );
	if( ::AdjustWindowRectEx( &rect, style, HasMenu( hWnd ) ? TRUE : FALSE, exStyle ) ) {
		RECT rect2;
		if( ::GetWindowRect( hWnd, &rect2 ) ) {
			if( ::SetWindowPos( hWnd, NULL, rect2.left, rect2.top, rect.right-rect.left, rect.bottom-rect.top, SIZE_CHANGE_FLAGS ) == 0 ) {
				TVPThrowWindowsErrorException();
			}
		} else {
			TVPThrowWindowsErrorException();
		}
	} else {
		TVPThrowWindowsErrorException();
	}
}


// 表示状態
bool tTVPWindow::GetVisible() const {
	return ::IsWindowVisible( GetHandle() ) ? true : false;
}
void tTVPWindow::SetVisible( bool s ) {
	::ShowWindow( GetHandle(), s ? SW_SHOW : SW_HIDE );
}

bool tTVPWindow::GetEnable() const {
	return ::IsWindowEnabled( GetHandle() ) ? true : false;
}
void tTVPWindow::SetEnable( bool s ) {
	::EnableWindow( GetHandle(), s ? TRUE : FALSE );
}

void tTVPWindow::GetCaption( std::wstring& v ) const {
	v.clear();
	int len = ::GetWindowTextLength( GetHandle() );
	if( len > 0 ) {
		std::vector<wchar_t> caption(len+1,0);
		int readlen = ::GetWindowText( GetHandle(), &(caption[0]), len+1 );
		if( readlen > 0 ) {
			v.assign( &(caption[0]) );
		}
	}
}
void tTVPWindow::SetCaption( const std::wstring& v ) {
	if( window_title_ != v ) {
		window_title_ = v;
		::SetWindowText( GetHandle(), window_title_.c_str() );
	}
}

void tTVPWindow::SetBorderStyle(tTVPBorderStyle st) {
	const DWORD notStyle = WS_POPUP | WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_DLGFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
	DWORD style = ::GetWindowLong( GetHandle(), GWL_STYLE);
	style &= ~notStyle;
	DWORD exStyle = ::GetWindowLong( GetHandle(), GWL_EXSTYLE);
	exStyle &= ~DEFAULT_EX_STYLE;
	if( has_parent_ ) {
		exStyle |= WS_EX_CONTROLPARENT;
	}
	border_style_ = static_cast<int>(st);
	switch( st ) {
	case bsDialog:
		style |= WS_DLGFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
		exStyle |= WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;
		break;
	case bsSingle:
		style |= WS_CAPTION | WS_BORDER;
		style |= WS_MINIMIZEBOX | WS_SYSMENU;
		break;
	case bsNone:
		style |= WS_POPUP;
		style |= WS_MINIMIZEBOX;
		if( has_parent_ ) {
			exStyle |= WS_EX_TOOLWINDOW; // taskbar unlist
		} else {
			style |= WS_SYSMENU; // taskbar right click menu
		}
		break;
	case bsSizeable:
		style |= WS_CAPTION | WS_THICKFRAME;
		style |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
		break;
	case bsToolWindow:
		style |= WS_CAPTION | WS_BORDER;
		style |= WS_SYSMENU;
		exStyle |= WS_EX_TOOLWINDOW;
		break;
	case bsSizeToolWin:
		style |= WS_CAPTION | WS_THICKFRAME;
		style |= WS_SYSMENU;
		exStyle |= WS_EX_TOOLWINDOW;
		break;
	default:
		style = ::GetWindowLong( GetHandle(), GWL_STYLE);
		exStyle = ::GetWindowLong( GetHandle(), GWL_EXSTYLE);
		break;
	}
	::SetWindowLong( GetHandle(), GWL_STYLE, style );
	::SetWindowLong( GetHandle(), GWL_EXSTYLE, exStyle );
	if( st == bsDialog ) {
		::SendMessage( GetHandle(), WM_SETICON, ICON_BIG, LPARAM(GetBigIcon()));
	} else {
		::SendMessage( GetHandle(), WM_SETICON, ICON_BIG, 0 );
	}
	::GetSystemMenu( GetHandle(), TRUE );
	::SendMessage( GetHandle(), WM_NCCREATE, 0, 0 );
	::SetWindowPos( GetHandle(), 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE );
	::InvalidateRect( GetHandle(), NULL, TRUE );
}
tTVPBorderStyle tTVPWindow::GetBorderStyle() const {
	return static_cast<tTVPBorderStyle>(border_style_);
}
HICON tTVPWindow::GetBigIcon() {
	return (HICON)SendMessage( GetHandle(), WM_GETICON, ICON_BIG, 0 );
}

const UINT tTVPWindow::SIZE_CHANGE_FLAGS = SWP_DEFERERASE | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;

void tTVPWindow::SetWidth( int w ) {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		if( ::SetWindowPos( GetHandle(), NULL, rect.left, rect.top, w, rect.bottom-rect.top, SIZE_CHANGE_FLAGS ) == 0 ) {
			TVPThrowWindowsErrorException();
		}
	} else {
		TVPThrowWindowsErrorException();
	}
}

int tTVPWindow::GetWidth() const {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		return rect.right - rect.left;
	} else {
		TVPThrowWindowsErrorException();
		return 0;
	}
}

void tTVPWindow::SetHeight( int h ) {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		if( ::SetWindowPos( GetHandle(), NULL, rect.left, rect.top, rect.right-rect.left, h, SIZE_CHANGE_FLAGS ) == 0 ) {
			TVPThrowWindowsErrorException();
		}
	} else {
		TVPThrowWindowsErrorException();
	}
}

int tTVPWindow::GetHeight() const {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		return rect.bottom - rect.top;
	} else {
		TVPThrowWindowsErrorException();
		return 0;
	}
}

void tTVPWindow::SetSize( int w, int h ) {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		if( ::SetWindowPos( GetHandle(), NULL, rect.left, rect.top, w, h, SIZE_CHANGE_FLAGS ) == 0 ) {
			TVPThrowWindowsErrorException();
		}
	} else {
		TVPThrowWindowsErrorException();
	}
}
void tTVPWindow::GetSize( int &w, int &h ) {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		h = rect.bottom - rect.top;
		w = rect.right - rect.left;
	} else {
		h = w = 0;
	}
}
const UINT tTVPWindow::POS_CHANGE_FLAGS = SWP_DEFERERASE | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER;

void tTVPWindow::SetLeft( int l ) {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		if( ::SetWindowPos( GetHandle(), NULL, l, rect.top, rect.right-rect.left, rect.bottom-rect.top, POS_CHANGE_FLAGS ) == 0 ) {
			TVPThrowWindowsErrorException();
		}
	} else {
		TVPThrowWindowsErrorException();
	}
}
int tTVPWindow::GetLeft() const {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		return rect.left;
	} else {
		TVPThrowWindowsErrorException();
		return 0;
	}
}
void tTVPWindow::SetTop( int t ) {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		if( ::SetWindowPos( GetHandle(), NULL, rect.left, t, rect.right-rect.left, rect.bottom-rect.top, POS_CHANGE_FLAGS ) == 0 ) {
			TVPThrowWindowsErrorException();
		}
	} else {
		TVPThrowWindowsErrorException();
	}
}
int tTVPWindow::GetTop() const {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		return rect.top;
	} else {
		TVPThrowWindowsErrorException();
		return 0;
	}
}
void tTVPWindow::SetPosition( int l, int t ) {
	RECT rect;
	if( ::GetWindowRect( GetHandle(), &rect ) ) {
		if( ::SetWindowPos( GetHandle(), NULL, l, t, rect.right-rect.left, rect.bottom-rect.top, POS_CHANGE_FLAGS ) == 0 ) {
			TVPThrowWindowsErrorException();
		}
	} else {
		TVPThrowWindowsErrorException();
	}
}
void tTVPWindow::SetBounds( int x, int y, int width, int height ) {
	if( ::SetWindowPos( GetHandle(), NULL, x, y, width, height, SWP_DEFERERASE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER ) == 0 ) {
		TVPThrowWindowsErrorException();
	}
}
void tTVPWindow::SetInnerWidth( int w ) {
	RECT rect;
	if( ::GetClientRect( GetHandle(), &rect ) ) {
		SIZE size;
		size.cx = w;
		size.cy = rect.bottom - rect.top;
		SetClientSize( GetHandle(), size );
	} else {
		TVPThrowWindowsErrorException();
	}
}

int tTVPWindow::GetInnerWidth() const {
	RECT rect;
	if( ::GetClientRect( GetHandle(), &rect ) ) {
		return rect.right - rect.left;
	} else {
		TVPThrowWindowsErrorException();
		return 0;
	}
}
void tTVPWindow::SetInnerHeight( int h ) {
	RECT rect;
	if( ::GetClientRect( GetHandle(), &rect ) ) {
		SIZE size;
		size.cx = rect.right - rect.left;
		size.cy = h;
		SetClientSize( GetHandle(), size );
	} else {
		TVPThrowWindowsErrorException();
	}
}

int tTVPWindow::GetInnerHeight() const {
	RECT rect;
	if( ::GetClientRect( GetHandle(), &rect ) ) {
		return rect.bottom - rect.top;
	} else {
		TVPThrowWindowsErrorException();
		return 0;
	}
}

void tTVPWindow::SetInnerSize( int w, int h ) {
	SIZE size;
	size.cx = w;
	size.cy = h;
	SetClientSize( GetHandle(), size );
}

void tTVPWindow::BringToFront() {
	if( ::SetWindowPos( GetHandle(), HWND_TOP, 0, 0, 0, 0, (SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE) ) == 0 ) {
		TVPThrowWindowsErrorException();
	}
}
void tTVPWindow::SetStayOnTop( bool b ) {
	static const UINT flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOSENDCHANGING;
	if( ::SetWindowPos( GetHandle(), b ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, flags ) == 0 ) {
		TVPThrowWindowsErrorException();
	}
}
bool tTVPWindow::GetStayOnTop() const {
	DWORD exStyle = ::GetWindowLong( GetHandle(), GWL_EXSTYLE );
	if( exStyle == 0 ) {
		TVPThrowWindowsErrorException();
		return false;
	}
	return (exStyle & WS_EX_TOPMOST) ? true : false;
}

void tTVPWindow::GetClientRect( struct tTVPRect& rt ) {
	RECT r;
	if( ::GetClientRect( GetHandle(), &r ) == 0 ) {
		TVPThrowWindowsErrorException();
	}
	rt.top = r.top;
	rt.left = r.left;
	rt.bottom = r.bottom;
	rt.right = r.right;
}

void tTVPWindow::Close() {
	if( in_mode_ ) {
		modal_result_ = mrCancel;
	} else if( OnCloseQuery() ) {
		CloseAction action = caFree;
		OnClose(action);
		switch( action ) {
		case caNone:
			break;
		case caHide:
			Hide();
			break;
		case caMinimize:
			::ShowWindow( GetHandle(), SW_MINIMIZE );
			break;
		case caFree:
		default:
			::PostMessage( GetHandle(), TVP_EV_WINDOW_RELEASE, 0, 0 );
			break;
		}
	}
}

void tTVPWindow::closeModal() {
	try {
		CloseAction action = caNone;
		if( OnCloseQuery() ) {
			action = caHide;
			OnClose(action);
		}
		switch( action ) {
		case caNone:
			modal_result_ = 0;
			break;
		case caFree:
			::PostMessage( GetHandle(), TVP_EV_WINDOW_RELEASE, 0, 0 );
			break;
		}
	} catch(...) {
		modal_result_ = 0;
		throw;
	}
}
int tTVPWindow::ShowModal() {
	if( GetVisible() || !GetEnable() ) {
		throw Exception( (const tjs_char*)TVPCannotShowModalAreadyShowed );
	}
	if( Application->GetWindowCount() == 1 ) {
		// 1個しか Windowがない時はModal化する意味がないのと、不具合の元なので例外発生
		throw Exception( (const tjs_char*)TVPCannotShowModalSingleWindow );
	}
	if( in_mode_ == false ) in_mode_ = true;
	try {
		if( ::GetCapture() != 0 ) {
			::SendMessage( ::GetCapture(), WM_CANCELMODE, 0, 0 );
		}
		::ReleaseCapture();
		Application->ModalStarted( this );
		std::vector<class TTVPWindowForm*> enableWindows;
		try {
			Application->GetEnableWindowList( enableWindows, NULL );
			Application->DisableWindows();
			SetEnable( true );
			Show();
			::SetActiveWindow( GetHandle() );
			modal_result_ = 0;
			while( modal_result_ == 0 ) {
				Application->HandleMessage();
				if( Application->IsTarminate() ) {
					modal_result_ = mrCancel;
				} else if( modal_result_ != 0 ) {
					closeModal();
				}
			}
			Application->EnableWindows( enableWindows );
			enableWindows.clear();
		} catch(...) {
			if( enableWindows.size() > 0 ) {
				Application->EnableWindows( enableWindows );
			}
			enableWindows.clear();
			Application->ModalFinished();
			in_mode_ = false;
			Hide();
			throw;
		}
		Application->ModalFinished();
		in_mode_ = false;
		Hide();
		return modal_result_;
	} catch(...) {
		in_mode_ = false;
		throw;
	}
}
