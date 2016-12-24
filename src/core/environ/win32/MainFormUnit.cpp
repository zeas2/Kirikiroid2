//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// System Main Window (Controller)
//---------------------------------------------------------------------------
#include "tjsCommHead.h"
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
#include "MainFormUnit.h"
#include "PadFormUnit.h"
#include "ConsoleFormUnit.h"
#include "EventIntf.h"
#include "MsgIntf.h"
#include "WindowFormUnit.h"
#include "SysInitIntf.h"
#include "SysInitImpl.h"
#include "ScriptMgnIntf.h"
#include "WatchFormUnit.h"
#include "WindowIntf.h"
#include "WindowImpl.h"
#include "HaltWarnFormUnit.h"
#include "StorageIntf.h"
#include "Random.h"
#include "EmergencyExit.h" // for TVPCPUClock
#include "DebugIntf.h"
#include "FontSelectFormUnit.h"
#include "HintWindow.h"
#include "VersionFormUnit.h"
#include "WaveImpl.h"
#include "SystemImpl.h"

//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#pragma package(smart_init)
#pragma resource "*.dfm"
#else
#include "MainFormUnit_dfm.h"
#endif
#include "TickCount.h"
TTVPMainForm *TVPMainForm = NULL;
bool TVPMainFormAlive = false;
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// Get whether debugging support windows can be shown
//---------------------------------------------------------------------------
static bool TVPDebugSupportShowableGot = false;
static bool TVPDebugSupportShowable = true;
static bool TVPGetDebugSupportShowable()
{
	if(TVPDebugSupportShowableGot) return TVPDebugSupportShowable;
	tTJSVariant val;
	if( TVPGetCommandLine(TJS_W("-debugwin"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("no"))
			TVPDebugSupportShowable = false;
	}

	TVPDebugSupportShowableGot = true;

	return TVPDebugSupportShowable;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// Get whether to control main thread priority or to insert wait
//---------------------------------------------------------------------------
static bool TVPMainThreadPriorityControlInit = false;
static bool TVPMainThreadPriorityControl = false;
static bool TVPGetMainThreadPriorityControl()
{
	if(TVPMainThreadPriorityControlInit) return TVPMainThreadPriorityControl;
	tTJSVariant val;
	if( TVPGetCommandLine(TJS_W("-lowpri"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("yes"))
			TVPMainThreadPriorityControl = true;
	}

	TVPMainThreadPriorityControlInit = true;

	return TVPMainThreadPriorityControl;
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// Get whether the modal windows are rearranged in full-screen
// (to avoid the modal window hides behind the main window)
//---------------------------------------------------------------------------
//static bool TVPModalWindowRearrangeInFullScreenInit = false;
//static bool TVPModalWindowRearrangeInFullScreen = false;
static bool TVPGetModalWindowRearrangeInFullScreen()
{
	return true;
/*
	if(TVPModalWindowRearrangeInFullScreenInit)
		return TVPModalWindowRearrangeInFullScreen;
	tTJSVariant val;
	if( TVPGetCommandLine(TJS_W("-remodal"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("yes"))
			TVPModalWindowRearrangeInFullScreen = true;
	}

	TVPModalWindowRearrangeInFullScreenInit = true;

	return TVPModalWindowRearrangeInFullScreen;
*/
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// Global Definitions
//---------------------------------------------------------------------------
TTVPPadForm *TVPMainPadForm = NULL;
TTVPConsoleForm *TVPMainConsoleForm = NULL;
TTVPWatchForm * TVPMainWatchForm = NULL;
//---------------------------------------------------------------------------
static void TVPUninitEnvironProfile();
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TTVPMainForm
//---------------------------------------------------------------------------
#include "float.h"
__fastcall TTVPMainForm::TTVPMainForm(TComponent* Owner)
	: TForm(Owner)
{
#ifndef __BORLANDC__
	init(this);
#endif
	ContinuousEventCalling = false;
	AutoShowConsoleOnError = false;
	ApplicationStayOnTop = false;
	ApplicationActivating = true;
	ApplicationNotMinimizing = true;
	LastCloseClickedTick = 0;
	LastShowModalWindowSentTick = 0;
	LastRehashedTick = 0;
#ifdef __BORLANDC__
	Application->OnIdle = ApplicationIdle;
	Application->OnActivate = ApplicationActivate;
	Application->OnDeactivate = ApplicationDeactivate;
	Application->OnMinimize = ApplicationMinimize;
	Application->OnRestore = ApplicationRestore;
#else
	Application->OnIdle = EVENT_FUNC2(TTVPMainForm, ApplicationIdle);
	Application->OnActivate = EVENT_FUNC1(TTVPMainForm, ApplicationActivate);
	Application->OnDeactivate = EVENT_FUNC1(TTVPMainForm, ApplicationDeactivate);
	Application->OnMinimize = EVENT_FUNC1(TTVPMainForm, ApplicationMinimize);
	Application->OnRestore = EVENT_FUNC1(TTVPMainForm, ApplicationRestore);
#endif
	// get command-line options which specifies global hot keys.
	SetHotKey(ShowControllerMenuItem, TJS_W("-hkcontroller"));
	SetHotKey(ShowScriptEditorMenuItem, TJS_W("-hkeditor"));
	SetHotKey(ShowWatchMenuItem, TJS_W("-hkwatch"));
	SetHotKey(ShowConsoleMenuItem, TJS_W("-hkconsole"));
	SetHotKey(ShowAboutMenuItem, TJS_W("-hkabout"));
	SetHotKey(CopyImportantLogMenuItem, TJS_W("-hkclipenvinfo"));


	// read previous state from environ profile
	tTVPProfileHolder *prof = TVPGetEnvironProfile();
	TVPEnvironProfileAddRef();
	static AnsiString section("controller");
	int n;
	n = prof->ReadInteger(section, "left", -1);
	if(n != -1) Left = n;
	n = prof->ReadInteger(section, "top", -1);
	if(n != -1) Top = n;
	n = prof->ReadInteger(section, "stayontop", 0);
	FormStyle = n ? fsStayOnTop : fsNormal;

	if(Left > Screen->Width) Left = Screen->Width - Width;
	if(Top > Screen->Height) Top = Screen->Height - Height;

	section = "console";
	n = prof->ReadInteger(section, "autoshowonerror", 0);
	AutoShowConsoleOnError = (bool)n;

	TVPMainFormAlive = true;
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::FormDestroy(TObject *Sender)
{
	// write to environ profile
	TVPMainFormAlive = false;

	tTVPProfileHolder *prof = TVPGetEnvironProfile();
	static AnsiString section("controller");
	prof->WriteInteger(section, "left", Left);
	prof->WriteInteger(section, "top", Top);
	prof->WriteInteger(section, "stayontop", FormStyle == fsStayOnTop);
	section = "console";
	prof->WriteInteger(section, "autoshowonerror", (int)AutoShowConsoleOnError);
	TVPEnvironProfileRelease(); // this may cause writing profile to disk
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::FormClose(TObject *Sender,
	  TCloseAction &Action)
{
	//
	if(TVPGetWindowCount() == 0)
	{
		// no window
		Action = caFree;
	}
	else
	{
		Action = caNone;
		Visible = false;
		if(TVPMainWindow)
			TVPMainWindow->SendCloseMessage();
	}
}
//---------------------------------------------------------------------------
TShortCut TTVPMainForm::GetHotKeyFromOption(TShortCut def, const tjs_char * optname)
{
	tTJSVariant val;
	if(TVPGetCommandLine(optname, &val))
	{
		TShortCut sc;
		ttstr str(val);
		if(str == TJS_W(""))
		{
			sc = (TShortCut)0;
		}
		else
		{
			sc = TextToShortCut(str.AsAnsiString());
			if(sc == 0)
				TVPThrowExceptionMessage(TVPInvalidCommandLineParam,
					optname, str);
		}
		return sc;
	}
	return def;
}
//---------------------------------------------------------------------------
void TTVPMainForm::SetHotKey(TMenuItem *item, const tjs_char * optname)
{
	item->ShortCut = GetHotKeyFromOption(item->ShortCut, optname);
}
//---------------------------------------------------------------------------
bool TTVPMainForm::CanHideAnyWindow()
{
	int viscount = 0;
	for(int i = 0; i < Screen->FormCount; i++)
	{
		if(Screen->Forms[i]->Visible) viscount++;
	}
	return viscount >=2;
}
//---------------------------------------------------------------------------
void TTVPMainForm::ShowController()
{
	// show self
	if(TVPGetDebugSupportShowable())
	{
		if(Visible)
		{
			 if(CanHideAnyWindow()) Visible = false;
		}
		else
		{
			Visible = true;
			BringToFront();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ShowControllerMenuItemClick(TObject *Sender)
{
	ShowController();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ShowScriptEditorButtonClick(TObject *Sender)
{
	if(TVPGetDebugSupportShowable())
	{
		if(TVPMainPadForm && TVPMainPadForm->Visible)
		{
			if(CanHideAnyWindow())
			{
				TVPMainPadForm->Visible = false;
				ShowScriptEditorButton->Down = false;
			}
		}
		else
		{
			if(!TVPMainPadForm)
			{
				TVPMainPadForm = new TTVPPadForm(Application, true);
				TVPMainPadForm->Caption = ttstr(TVPMainCDPName).AsAnsiString();
			}
			TVPMainPadForm->Visible = true;
			ShowScriptEditorButton->Down = true;
			TVPMainPadForm->BringToFront();
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ShowWatchButtonClick(TObject *Sender)
{
	if(TVPGetDebugSupportShowable())
	{
		if(TVPMainWatchForm && TVPMainWatchForm->Visible)
		{
			if(CanHideAnyWindow())
			{
				TVPMainWatchForm->Visible = false;
				ShowWatchButton->Down = false;
			}
		}
		else
		{
			if(!TVPMainWatchForm)
			{
				TVPMainWatchForm = new TTVPWatchForm(Application);
			}
			TVPMainWatchForm->Visible = true;
			TVPMainWatchForm->BringToFront();
			ShowWatchButton->Down = true;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ShowConsoleButtonClick(TObject *Sender)
{
	if(TVPGetDebugSupportShowable())
	{
		if(TVPMainConsoleForm && TVPMainConsoleForm->Visible)
		{
			if(CanHideAnyWindow())
			{
				TVPMainConsoleForm->Visible = false;
				ShowConsoleButton->Down = false;
			}
		}
		else
		{
			if(!TVPMainConsoleForm)
			{
				TVPMainConsoleForm = new TTVPConsoleForm(Application);
			}
			TVPMainConsoleForm->Visible = true;
			TVPMainConsoleForm->BringToFront();
			ShowConsoleButton->Down = true;
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ShowOnTopMenuItemClick(TObject *Sender)
{
	FormStyle = FormStyle == fsStayOnTop ? fsNormal : fsStayOnTop;
}
//---------------------------------------------------------------------------
void TTVPMainForm::SetConsoleVisible(bool b)
{
	if(b)
	{
		if(!TVPMainConsoleForm)
		{
			TVPMainConsoleForm = new TTVPConsoleForm(Application);
		}
		TVPMainConsoleForm->Visible = true;
		ShowConsoleButton->Down = true;
	}
	else
	{
		if(TVPMainConsoleForm)
			TVPMainConsoleForm->Visible = false;
		ShowConsoleButton->Down = false;
	}
}
//---------------------------------------------------------------------------
bool TTVPMainForm::GetConsoleVisible()
{
	if(TVPMainConsoleForm)
		return TVPMainConsoleForm->Visible;
	return false;
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyConsoleHiding()
{
	ShowConsoleButton->Down = false;
}
//---------------------------------------------------------------------------
bool TTVPMainForm::GetScriptEditorVisible()
{
	if(TVPMainPadForm) return TVPMainPadForm->Visible;
	return false;
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyScriptEditorHiding()
{
	ShowScriptEditorButton->Down = false;
}
//---------------------------------------------------------------------------
bool TTVPMainForm::GetWatchVisible()
{
	if(TVPMainWatchForm) return TVPMainWatchForm->Visible;
	return false;
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyWatchHiding()
{
	ShowWatchButton->Down = false;
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ExitButtonClick(TObject *Sender)
{
	Application->Terminate();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::PopupMenuPopup(TObject *Sender)
{
	ShowOnTopMenuItem->Checked = FormStyle == fsStayOnTop;
	ShowScriptEditorMenuItem->Checked = GetScriptEditorVisible();
	ShowWatchMenuItem->Checked = GetWatchVisible();
	ShowConsoleMenuItem->Checked = GetConsoleVisible();
	EnableEventMenuItem->Checked = EventButton->Down;
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::EnableEventMenuItemClick(TObject *Sender)
{
	TVPSetSystemEventDisabledState(EventButton->Down);
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::EventButtonClick(TObject *Sender)
{
	TVPSetSystemEventDisabledState(!EventButton->Down);
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ShowAboutMenuItemClick(TObject *Sender)
{
    TVPShowVersionForm();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::CopyImportantLogMenuItemClick(
	  TObject *Sender)
{
	TVPCopyImportantLogToClipboard();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::DumpMenuItemClick(TObject *Sender)
{
	TVPDumpScriptEngine();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::CreateMessageMapFileMenuItemClick(
	  TObject *Sender)
{
	try
	{
		ttstr fn = TVPGetAppPath() + TJS_W("msgmap.tjs");
		TVPGetLocalName(fn);
		MessageMapFileSaveDialog->FileName = fn.AsAnsiString();
	}
	catch(...)
	{
		MessageMapFileSaveDialog->FileName = "msgmap.tjs";
	}

	if(MessageMapFileSaveDialog->Execute())
	{
		TVPCreateMessageMapFile(MessageMapFileSaveDialog->FileName);
	}
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::RestartScriptEngineMenuItemClick(TObject *Sender)
{
	TVPRestartScriptEngine();
}
//---------------------------------------------------------------------------
void TTVPMainForm::InvokeEvents()
{
	CallDeliverAllEventsOnIdle();
}
//---------------------------------------------------------------------------
void TTVPMainForm::CallDeliverAllEventsOnIdle()
{
	::PostMessage(TVPMainForm->Handle, WM_USER+0x31/*dummy msg*/, 0, 0);
//	if(SystemWatchTimer->Interval != 50)
//		SystemWatchTimer->Interval = 50;
}
//---------------------------------------------------------------------------
void TTVPMainForm::BeginContinuousEvent()
{
	if(!ContinuousEventCalling)
	{
		ContinuousEventCalling = true;
		InvokeEvents();
		if(TVPGetMainThreadPriorityControl())
		{
			// make main thread priority lower
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
		}
	}
}
//---------------------------------------------------------------------------
void TTVPMainForm::EndContinuousEvent()
{
	if(ContinuousEventCalling)
	{
		ContinuousEventCalling = false;
		if(TVPGetMainThreadPriorityControl())
		{
			// make main thread priority normal
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		}
	}
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyCloseClicked()
{
	// close Button is clicked
	LastCloseClickedTick = TVPGetRoughTickCount32();
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyEventDelivered()
{
	// called from event system, notifying the event is delivered.
	LastCloseClickedTick = 0;
	if(TVPHaltWarnForm) delete TVPHaltWarnForm, TVPHaltWarnForm = NULL;
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyHaltWarnFormClosed()
{
	LastCloseClickedTick = 0;
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifySystemError()
{
	if(AutoShowConsoleOnError) SetConsoleVisible(true);
}
//---------------------------------------------------------------------------
void TTVPMainForm::SetApplicationStayOnTop(bool b)
{
	ApplicationStayOnTop = b;
	if(ApplicationStayOnTop)
		SetWindowPos(Application->Handle,HWND_TOPMOST ,0,0,0,0,
			SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
	else
		SetWindowPos(Application->Handle,HWND_NOTOPMOST ,0,0,0,0,
			SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::WMInvokeEvents(TMessage &Msg)
{
	// indirectly called by TVPInvokeEvents  *** currently not used ***
	if(EventButton->Down)
	{
		TVPDeliverAllEvents();
	}
}
//---------------------------------------------------------------------------
void TTVPMainForm::DeliverEvents()
{
	if(ContinuousEventCalling)
		TVPProcessContinuousHandlerEventFlag = true; // set flag

	if(EventButton->Down) TVPDeliverAllEvents();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ApplicationIdle(TObject *Sender, bool &Done)
{
	DeliverEvents();

	bool cont = ContinuousEventCalling;
	Done = !cont;

	MixedIdleTick += TVPGetRoughTickCount32();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ApplicationActivate(TObject *Sender)
{
	ApplicationActivating = true;

	TVPRestoreFullScreenWindowAtActivation();
	TVPShowModalAtAppActivate();
	TVPShowFontSelectFormAtAppActivate();
	TVPResetVolumeToAllSoundBuffer();

	// trigger System.onActivate event
	TVPPostApplicationActivateEvent();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ApplicationDeactivate(TObject *Sender)
{
	ApplicationActivating = false;

	TVPHideModalAtAppDeactivate();
	TVPHideFontSelectFormAtAppDeactivate();

	TVPMinimizeFullScreenWindowAtInactivation();


	// fire compact event
	TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_DEACTIVATE);

	// set application-level stay-on-top state
	if(ApplicationStayOnTop)
		SetWindowPos(Application->Handle,HWND_TOPMOST ,0,0,0,0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
	else
		SetWindowPos(Application->Handle,HWND_NOTOPMOST ,0,0,0,0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

	// set sound volume
	TVPResetVolumeToAllSoundBuffer();

	// trigger System.onDeactivate event
	TVPPostApplicationDeactivateEvent();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ApplicationMinimize(TObject *Sender)
{
	// fire compact event
	ApplicationNotMinimizing = false;

	TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MINIMIZE);

	// set sound volume
	TVPResetVolumeToAllSoundBuffer();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::ApplicationRestore(TObject *Sender)
{
	ApplicationNotMinimizing = true;

	// set sound volume
	TVPResetVolumeToAllSoundBuffer();
}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::SystemWatchTimerTimer(TObject *Sender)
{
	if(TVPTerminated)
	{
		// this will ensure terminating the application.
		// the WM_QUIT message disappears in some unknown situations...
		::PostMessage(TVPMainForm->Handle, WM_USER+0x31/*dummy msg*/, 0, 0);
		Application->Terminate();
		::PostMessage(TVPMainForm->Handle, WM_USER+0x31/*dummy msg*/, 0, 0);
	}

	// call events
	DWORD tick = TVPGetRoughTickCount32();

	// push environ noise
	TVPPushEnvironNoise(&tick, sizeof(tick));
	TVPPushEnvironNoise(&LastCompactedTick, sizeof(LastCompactedTick));
	TVPPushEnvironNoise(&LastShowModalWindowSentTick, sizeof(LastShowModalWindowSentTick));
	TVPPushEnvironNoise(&MixedIdleTick, sizeof(MixedIdleTick));
	POINT pt;
	GetCursorPos(&pt);
	TVPPushEnvironNoise(&pt, sizeof(pt));

	// CPU clock monitoring
	{
		static bool clock_rough_printed = false;
		if(!clock_rough_printed && TVPCPUClockAccuracy == ccaRough)
		{
			tjs_char msg[80];
			TJS_sprintf(msg, TJS_W("(info) CPU clock (roughly) : %dMHz"), (int)TVPCPUClock);
			TVPAddImportantLog(msg);
			clock_rough_printed = true;
		}
		static bool clock_printed = false;
		if(!clock_printed && TVPCPUClockAccuracy == ccaAccurate)
		{
			tjs_char msg[80];
			TJS_sprintf(msg, TJS_W("(info) CPU clock : %.1fMHz"), (float)TVPCPUClock);
			TVPAddImportantLog(msg);
			clock_printed = true;
		}
	}

	// check status and deliver events
	DeliverEvents();

	// call TickBeat
	tjs_int count = TVPGetWindowCount();
	for(tjs_int i = 0; i<count; i++)
	{
		tTJSNI_Window *win = TVPGetWindowListAt(i);
		win->TickBeat();
	}

	if(!ContinuousEventCalling && tick - LastCompactedTick > 4000)
	{
		// idle state over 4 sec.
		LastCompactedTick = tick;

		// fire compact event
		TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_IDLE);
	}

	if(!ContinuousEventCalling && tick > LastRehashedTick + 1500)
	{
		// TJS2 object rehash
		LastRehashedTick = tick;
		TJSDoRehash();
	}

	if(LastCloseClickedTick && tick - LastCloseClickedTick > 3100)
	{
		// display force suicide confirmation form
		if(TVPHaltWarnForm)
		{
			TVPHaltWarnForm->Visible = true;
		}
		else
		{
			TVPHaltWarnForm = new TTVPHaltWarnForm(Application);
			TVPHaltWarnForm->Visible = true;
		}
	}

	// ensure modal window visible
	if(tick > LastShowModalWindowSentTick + 4100)
	{
		//	::PostMessage(Handle, WM_USER+0x32, 0, 0);
		// This is currently disabled because IME composition window
		// hides behind the window which is bringed top by the
		// window-rearrangement.
		LastShowModalWindowSentTick = tick;
	}

}
//---------------------------------------------------------------------------
void __fastcall TTVPMainForm::WMRearrangeModalWindows(TMessage &Msg)
{
	if(TVPFullScreenedWindow != NULL && TVPGetModalWindowRearrangeInFullScreen())
	{
		HDWP hdwp = BeginDeferWindowPos(1);
		hdwp = TVPShowModalAtTimer(hdwp);
		hdwp = TVPShowFontSelectFormTop(hdwp);
		hdwp = TVPShowHintWindowTop(hdwp);
		EndDeferWindowPos(hdwp);
	}
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// Environment Profiling Support
//---------------------------------------------------------------------------
static tTVPProfileHolder *TVPEnvironProfile = NULL;
tjs_int TVPEnvironProfileRefCount = 0;
static bool TVPProfileWrite = false;
//---------------------------------------------------------------------------
static void TVPInitEnvironProfile()
{
	// initialize environ profile
	if(!TVPEnvironProfile)
	{
		// read profile from project directory or
		// current directory ( if projdir was not specified )
		if(strchr(TVPNativeDataPath.c_str(), TVPArchiveDelimiter))
			TVPProfileWrite = false; else TVPProfileWrite = true;
		try
		{
			TVPEnvironProfile =
				new tTVPProfileHolder(TVPNativeDataPath + "krenvprf.kep");
		}
		catch(...)
		{
			TVPEnvironProfile = new tTVPProfileHolder(TVPGetTemporaryName().AsAnsiString());
			TVPProfileWrite = false;
		}
	}
}
//---------------------------------------------------------------------------
void TVPWriteEnvironProfile()
{
	// write environ profile values
	if(TVPEnvironProfile)
	{
		try
		{
			if(TVPProfileWrite) TVPEnvironProfile->UpdateFile();
		}
		catch(...)
		{
			// suppress error
		}
	}
}
//---------------------------------------------------------------------------
static void TVPUninitEnvironProfile()
{
	// clean up environ profile values
	TVPWriteEnvironProfile();
	if(TVPEnvironProfile)
	{
		delete TVPEnvironProfile;
		TVPEnvironProfile = NULL;
	}
}
//---------------------------------------------------------------------------
void TVPEnvironProfileAddRef()
{
	TVPEnvironProfileRefCount ++;
}
//---------------------------------------------------------------------------
void TVPEnvironProfileRelease()
{
	TVPEnvironProfileRefCount --;
	if(TVPEnvironProfileRefCount == 0) TVPUninitEnvironProfile();
}
//---------------------------------------------------------------------------
tTVPProfileHolder *TVPGetEnvironProfile()
{
	// read value from section and name, return defstr if not found.
	TVPInitEnvironProfile();
	return TVPEnvironProfile;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
static AnsiString TVPEscapeAnsiString(const AnsiString & str)
{
	// escape to harmless string
	char *buf = new char [str.Length() * 2 + 1];  // enough to hold
	const char *s = str.c_str();
	int i = 0;
	while(*s)
	{
		if(*s == '@')
		{
			buf[i++] = '@';
			buf[i++] = '@';
		}
		else if((unsigned char)*s < 0x20)
		{
			buf[i++] = '@';
			buf[i++] = ' ' + *s;
		}
		else
		{
			buf[i++] = *s;
		}

		s++;
	}
	buf[i] = 0;
	AnsiString ret =  buf;
	delete [] buf;
	return ret;
}
//---------------------------------------------------------------------------
static AnsiString TVPUnescapeAnsiString(const AnsiString & str)
{
	// unescape
	int strlen = str.Length();
	char *buf = new char [strlen + 1];
	const char *s = str.c_str();
	const char *slim = s + strlen;
	int i = 0;
	while(s<slim)
	{
		if(*s == '@')
		{
			if(s[1] == '@')
				buf[i++] = '@';
			else
				buf[i++] = s[1] - ' ';
			s+=2;
		}
		else
		{
			buf[i++] = *s;
			s++;
		}
	}
	buf[i] = 0;
	AnsiString ret = buf;
	delete [] buf;
	return ret;
}
//---------------------------------------------------------------------------
void __fastcall tTVPProfileHolder::WriteStrings(const AnsiString &section,
	const AnsiString &ident, TStrings * strings)
{
	WriteString(section, ident, TVPEscapeAnsiString(strings->Text));
}
//---------------------------------------------------------------------------
void __fastcall tTVPProfileHolder::ReadStrings(const AnsiString &section,
	const AnsiString &ident, TStrings * strings)
{
	AnsiString str;
	str = ReadString(section, ident, "");
	if(str != "") strings->Text = TVPUnescapeAnsiString(str);
}
//---------------------------------------------------------------------------







