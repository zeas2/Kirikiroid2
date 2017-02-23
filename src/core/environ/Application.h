
#ifndef __T_APPLICATION_H__
#define __T_APPLICATION_H__

#include "tjsVariant.h"
#include "tjsString.h"
#include <vector>
#include <functional>
#include <mutex>
#include <tuple>
#include <map>

ttstr ExePath();

// 見通しのよい方法に変更した方が良い
extern int _argc;
extern char ** _argv;

enum {
	mrOk,
	mrAbort,
	mrCancel,
};

enum {
  mtWarning = /*MB_ICONWARNING*/0x00000030L,
  mtError = /*MB_ICONERROR*/0x00000010L,
  mtInformation = /*MB_ICONINFORMATION*/0x00000040L,
  mtConfirmation = /*MB_ICONQUESTION*/0x00000020L,
  mtStop = /*MB_ICONSTOP*/0x00000010L,
  mtCustom = 0
};
enum {
	mbOK = /*MB_OK*/0x00000000L,
};
enum class eTVPActiveEvent {
	onActive,
	onDeactive,
};
#if 0
class AcceleratorKey {
	HACCEL hAccel_;
	ACCEL* keys_;
	int key_count_;

public:
	AcceleratorKey();
	~AcceleratorKey();
	void AddKey( WORD id, WORD key, BYTE virt );
	void DelKey( WORD id );
	HACCEL GetHandle() { return hAccel_; }
};
class AcceleratorKeyTable {
	std::map<HWND,AcceleratorKey*> keys_;
	HACCEL hAccel_;

public:
	AcceleratorKeyTable();
	~AcceleratorKeyTable();
	void AddKey( HWND hWnd, WORD id, WORD key, BYTE virt );
	void DelKey( HWND hWnd, WORD id );
	void DelTable( HWND hWnd );
	HACCEL GetHandle(HWND hWnd) {
		std::map<HWND,AcceleratorKey*>::iterator i = keys_.find(hWnd);
		if( i != keys_.end() ) {
			return i->second->GetHandle();
		}
		return hAccel_;
	}
};
#endif
class tTVPApplication {
//	std::vector<class TTVPWindowForm*> windows_list_;
	ttstr title_;

	bool is_attach_console_;
	ttstr console_title_;
//	AcceleratorKeyTable accel_key_;
	bool tarminate_;
	bool application_activating_;
	bool has_map_report_process_;

	class tTVPAsyncImageLoader* image_load_thread_;
#if 0
	std::stack<class tTVPWindow*> modal_window_stack_;
#endif
private:
	void CheckConsole();
	void CloseConsole();
	void CheckDigitizer();
	void ShowException( const ttstr& e );
	void Initialize() {}

public:
	tTVPApplication();
	~tTVPApplication();
	bool StartApplication(ttstr path);
	void Run();
	void ProcessMessages();

	static void PrintConsole(const ttstr &mes, bool important = false);
	bool IsAttachConsole() { return is_attach_console_; }

	bool IsTarminate() const { return tarminate_; }

//	HWND GetHandle();
	bool IsIconic() {
#if 0
		HWND hWnd = GetHandle();
		if( hWnd != INVALID_HANDLE_VALUE ) {
			return 0 != ::IsIconic(hWnd);
		}
#endif
		return true; // そもそもウィンドウがない
	}
#if 0
	void Minimize();
	void Restore();
	void BringToFront();

	void AddWindow( class TTVPWindowForm* win ) {
		windows_list_.push_back( win );
	}
	void RemoveWindow( class TTVPWindowForm* win );
	unsigned int GetWindowCount() const {
		return (unsigned int)windows_list_.size();
	}

	void FreeDirectInputDeviceForWindows();

	bool ProcessMessage( MSG &msg );
	void ProcessMessages();
	void HandleMessage();
	void HandleIdle(MSG &msg);
#endif
	const ttstr &GetTitle() const { return title_; }
	void SetTitle( const ttstr& caption );
#if 0
	static inline int MessageDlg( const ttstr& string, const ttstr& caption, int type, int button ) {
		return ::MessageBox( NULL, string.c_str(), caption.c_str(), type|button  );
	}
#endif
	void Terminate();
	void SetHintHidePause( int v ) {}
	void SetShowHint( bool b ) {}
	void SetShowMainForm( bool b ) {}


//	HWND GetMainWindowHandle() const;

	int ArgC;
	char ** ArgV;
	typedef std::function<void()> tMsg;

//	void PostMessageToMainWindow(UINT message, WPARAM wParam, LPARAM lParam);
	void PostUserMessage(const std::function<void()> &func, void* param1 = nullptr, int param2 = 0);
	void FilterUserMessage(const std::function<void(std::vector<std::tuple<void*, int, tMsg> > &)> &func);

#if 0
	void ModalStarted( class tTVPWindow* form ) {
		modal_window_stack_.push(form);
	}

	void ModalFinished();
	void OnActiveAnyWindow();
	void DisableWindows();
	void EnableWindows( const std::vector<class TTVPWindowForm*>& win );
	void GetDisableWindowList( std::vector<class TTVPWindowForm*>& win );
	void GetEnableWindowList( std::vector<class TTVPWindowForm*>& win, class TTVPWindowForm* activeWindow );

	void RegisterAcceleratorKey(HWND hWnd, char virt, short key, short cmd);
	void UnregisterAcceleratorKey(HWND hWnd, short cmd);
	void DeleteAcceleratorKeyTable( HWND hWnd );
#endif
	void OnActivate(  );
	void OnDeactivate(  );
	void OnExit();
	void OnLowMemory();

	bool GetActivating() const { return application_activating_; }
	bool GetNotMinimizing() const;

	/**
	 * 画像の非同期読込み要求
	 */
	void LoadImageRequest( class iTJSDispatch2 *owner, class tTJSNI_Bitmap* bmp, const ttstr &name );
	tTVPAsyncImageLoader* GetAsyncImageLoader() { return image_load_thread_; }

	void RegisterActiveEvent(void *host, const std::function<void(void*, eTVPActiveEvent)>& func/*empty = unregister*/);

private:
	std::mutex m_msgQueueLock;

	std::vector<std::tuple<void*, int, tMsg> > m_lstUserMsg;
	std::map<void*, std::function<void(void*, eTVPActiveEvent)> > m_activeEvents;
};
std::vector<std::string>* LoadLinesFromFile( const ttstr& path );

//inline HINSTANCE GetHInstance() { return ((HINSTANCE)GetModuleHandle(0)); }
extern class tTVPApplication* Application;


#endif // __T_APPLICATION_H__
