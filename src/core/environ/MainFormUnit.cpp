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
#include "EventIntf.h"
#include "MsgIntf.h"
#include "WindowFormUnit.h"
#include "SysInitIntf.h"
#include "SysInitImpl.h"
#include "ScriptMgnIntf.h"
#include "WindowIntf.h"
#include "WindowImpl.h"
#include "StorageIntf.h"
#include "Random.h"
//#include "EmergencyExit.h" // for TVPCPUClock
#include "DebugIntf.h"
// #include "HintWindow.h"
#include "WaveImpl.h"
#include "SystemImpl.h"
#include "Application.h"

//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#pragma package(smart_init)
#pragma resource "*.dfm"
#else
//#include "MainFormUnit_dfm.h"
#endif
#include "TickCount.h"
#include "Platform.h"
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
//TTVPPadForm *TVPMainPadForm = NULL;
// TTVPConsoleForm *TVPMainConsoleForm = NULL;
// TTVPWatchForm * TVPMainWatchForm = NULL;
//---------------------------------------------------------------------------
static void TVPUninitEnvironProfile();
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TTVPMainForm
//---------------------------------------------------------------------------
#include "float.h"
TTVPMainForm::TTVPMainForm(TWinControl* Owner)
	//: TForm(Owner)
{
#ifndef __BORLANDC__
	//init(this);
#endif
	ContinuousEventCalling = false;
	AutoShowConsoleOnError = false;
	ApplicationStayOnTop = false;
	ApplicationActivating = true;
	ApplicationNotMinimizing = true;
	LastCloseClickedTick = 0;
	LastShowModalWindowSentTick = 0;
	LastRehashedTick = 0;
    //SystemWatchTimer = 0;
#ifdef __BORLANDC__
//	Application->OnIdle = ApplicationIdle;
// 	Application->OnActivate = ApplicationActivate;
// 	Application->OnDeactivate = ApplicationDeactivate;
// 	Application->OnMinimize = ApplicationMinimize;
// 	Application->OnRestore = ApplicationRestore;
#else
	Application->OnIdle = EVENT_FUNC2(TTVPMainForm, ApplicationIdle);
// 	Application->OnActivate = EVENT_FUNC1(TTVPMainForm, ApplicationActivate);
// 	Application->OnDeactivate = EVENT_FUNC1(TTVPMainForm, ApplicationDeactivate);
// 	Application->OnMinimize = EVENT_FUNC1(TTVPMainForm, ApplicationMinimize);
// 	Application->OnRestore = EVENT_FUNC1(TTVPMainForm, ApplicationRestore);
#endif
	// get command-line options which specifies global hot keys.
// 	SetHotKey(ShowControllerMenuItem, TJS_W("-hkcontroller"));
// 	SetHotKey(ShowScriptEditorMenuItem, TJS_W("-hkeditor"));
// 	SetHotKey(ShowWatchMenuItem, TJS_W("-hkwatch"));
// 	SetHotKey(ShowConsoleMenuItem, TJS_W("-hkconsole"));
// 	SetHotKey(ShowAboutMenuItem, TJS_W("-hkabout"));
// 	SetHotKey(CopyImportantLogMenuItem, TJS_W("-hkclipenvinfo"));


	// read previous state from environ profile
	tTVPProfileHolder *prof = TVPGetEnvironProfile();
	TVPEnvironProfileAddRef();
	static AnsiString section("controller");
	int n;
	n = prof->ReadInteger(section, "left", -1);
//	if(n != -1) setLeft (n);
	n = prof->ReadInteger(section, "top", -1);
//	if(n != -1) setTop (n);
	n = prof->ReadInteger(section, "stayontop", 0);
//	setFormStyle (n ? fsStayOnTop : fsNormal);

//     if(getLeft() > Screen->getWidth()) setLeft (Screen->getWidth() - getWidth());
// 	if(getTop() > Screen->getHeight()) setTop (Screen->getHeight() - getHeight());

	section = "console";
	n = prof->ReadInteger(section, "autoshowonerror", 0);
	AutoShowConsoleOnError = (bool)n;

	TVPMainFormAlive = true;
}
//---------------------------------------------------------------------------
void TTVPMainForm::FormDestroy(TObject *Sender)
{
	// write to environ profile
	TVPMainFormAlive = false;

	tTVPProfileHolder *prof = TVPGetEnvironProfile();
	static AnsiString section("controller");
// 	prof->WriteInteger(section, "left", getLeft());
// 	prof->WriteInteger(section, "top", getTop());
//	prof->WriteInteger(section, "stayontop", getFormStyle() == fsStayOnTop);
	section = "console";
	prof->WriteInteger(section, "autoshowonerror", (int)AutoShowConsoleOnError);
	TVPEnvironProfileRelease(); // this may cause writing profile to disk
}
//---------------------------------------------------------------------------
void TTVPMainForm::FormClose(TObject *Sender,
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
//		setVisible (false);
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
// 	for(int i = 0; i < Screen->getFormCount(); i++)
// 	{
// 		if(Screen->getForms(i)->getVisible()) viscount++;
// 	}
	return viscount >=2;
}
//---------------------------------------------------------------------------
void TTVPMainForm::ShowController()
{
	// show self
	if(TVPGetDebugSupportShowable())
	{
// 		if(getVisible())
// 		{
// 			 if(CanHideAnyWindow()) setVisible (false);
// 		}
// 		else
// 		{
// 			setVisible (true);
// 			BringToFront();
//		}
	}
}
//---------------------------------------------------------------------------
void TTVPMainForm::ShowControllerMenuItemClick(TObject *Sender)
{
	ShowController();
}
//---------------------------------------------------------------------------
void TTVPMainForm::ShowScriptEditorButtonClick(TObject *Sender)
{
	if(TVPGetDebugSupportShowable())
	{
// 		if(TVPMainPadForm && TVPMainPadForm->Visible)
// 		{
// 			if(CanHideAnyWindow())
// 			{
// 				TVPMainPadForm->Visible = false;
// 				ShowScriptEditorButton->Down = false;
// 			}
// 		}
// 		else
// 		{
// 			if(!TVPMainPadForm)
// 			{
// 				TVPMainPadForm = new TTVPPadForm(Application, true);
// 				TVPMainPadForm->Caption = ttstr(TVPMainCDPName).AsAnsiString();
// 			}
// 			TVPMainPadForm->Visible = true;
// 			ShowScriptEditorButton->Down = true;
// 			TVPMainPadForm->BringToFront();
// 		}
	}
}
//---------------------------------------------------------------------------
void TTVPMainForm::ShowWatchButtonClick(TObject *Sender)
{
	if(TVPGetDebugSupportShowable())
	{
// 		if(TVPMainWatchForm && TVPMainWatchForm->Visible)
// 		{
// 			if(CanHideAnyWindow())
// 			{
// 				TVPMainWatchForm->Visible = false;
// 				ShowWatchButton->Down = false;
// 			}
// 		}
// 		else
// 		{
// 			if(!TVPMainWatchForm)
// 			{
// 				TVPMainWatchForm = new TTVPWatchForm(Application);
// 			}
// 			TVPMainWatchForm->Visible = true;
// 			TVPMainWatchForm->BringToFront();
// 			ShowWatchButton->Down = true;
// 		}
	}
}
//---------------------------------------------------------------------------
void TTVPMainForm::ShowConsoleButtonClick(TObject *Sender)
{
	if(TVPGetDebugSupportShowable())
	{
// 		if(TVPMainConsoleForm && TVPMainConsoleForm->Visible)
// 		{
// 			if(CanHideAnyWindow())
// 			{
// 				TVPMainConsoleForm->Visible = false;
// 				ShowConsoleButton->Down = false;
// 			}
// 		}
// 		else
// 		{
// 			if(!TVPMainConsoleForm)
// 			{
// 				TVPMainConsoleForm = new TTVPConsoleForm(Application);
// 			}
// 			TVPMainConsoleForm->Visible = true;
// 			TVPMainConsoleForm->BringToFront();
// 			ShowConsoleButton->Down = true;
// 		}
	}
}
//---------------------------------------------------------------------------
void TTVPMainForm::ShowOnTopMenuItemClick(TObject *Sender)
{
//    setFormStyle (fsStayOnTop ? fsNormal : fsStayOnTop);
}
//---------------------------------------------------------------------------
void TTVPMainForm::SetConsoleVisible(bool b)
{
// 	if(b)
// 	{
// 		if(!TVPMainConsoleForm)
// 		{
// 			TVPMainConsoleForm = new TTVPConsoleForm(Application);
// 		}
// 		TVPMainConsoleForm->Visible = true;
// 		ShowConsoleButton->Down = true;
// 	}
// 	else
// 	{
// 		if(TVPMainConsoleForm)
// 			TVPMainConsoleForm->Visible = false;
// 		ShowConsoleButton->Down = false;
// 	}
}
//---------------------------------------------------------------------------
bool TTVPMainForm::GetConsoleVisible()
{
// 	if(TVPMainConsoleForm)
// 		return TVPMainConsoleForm->Visible;
	return false;
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyConsoleHiding()
{
//	ShowConsoleButton->setDown (false);
}
//---------------------------------------------------------------------------
bool TTVPMainForm::GetScriptEditorVisible()
{
//	if(TVPMainPadForm) return TVPMainPadForm->Visible;
	return false;
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyScriptEditorHiding()
{
//	ShowScriptEditorButton->setDown (false);
}
//---------------------------------------------------------------------------
bool TTVPMainForm::GetWatchVisible()
{
//	if(TVPMainWatchForm) return TVPMainWatchForm->Visible;
	return false;
}
//---------------------------------------------------------------------------
void TTVPMainForm::NotifyWatchHiding()
{
//	ShowWatchButton->setDown (false);
}
//---------------------------------------------------------------------------
void TTVPMainForm::ExitButtonClick(TObject *Sender)
{
	Application->Terminate();
}
//---------------------------------------------------------------------------
void TTVPMainForm::PopupMenuPopup(TObject *Sender)
{
// 	ShowOnTopMenuItem->setChecked (getFormStyle() == fsStayOnTop);
// 	ShowScriptEditorMenuItem->setChecked (GetScriptEditorVisible());
// 	ShowWatchMenuItem->setChecked (GetWatchVisible());
// 	ShowConsoleMenuItem->setChecked (GetConsoleVisible());
// 	EnableEventMenuItem->setChecked( EventButton->getDown());
}
//---------------------------------------------------------------------------
void TTVPMainForm::EnableEventMenuItemClick(TObject *Sender)
{
//	TVPSetSystemEventDisabledState(EventButton->getDown());
}
//---------------------------------------------------------------------------
void TTVPMainForm::EventButtonClick(TObject *Sender)
{
//	TVPSetSystemEventDisabledState(!EventButton->getDown());
}
//---------------------------------------------------------------------------
void TTVPMainForm::ShowAboutMenuItemClick(TObject *Sender)
{
//    TVPShowVersionForm();
}
//---------------------------------------------------------------------------
void TTVPMainForm::CopyImportantLogMenuItemClick(
	  TObject *Sender)
{
//	TVPCopyImportantLogToClipboard();
}
//---------------------------------------------------------------------------
void TTVPMainForm::DumpMenuItemClick(TObject *Sender)
{
	TVPDumpScriptEngine();
}
//---------------------------------------------------------------------------
void TTVPMainForm::CreateMessageMapFileMenuItemClick(
	  TObject *Sender)
{
	try
	{
		ttstr fn = TVPGetAppPath() + TJS_W("msgmap.tjs");
		TVPGetLocalName(fn);
//		MessageMapFileSaveDialog->FileName = fn.AsAnsiString();
	}
	catch(...)
	{
//		MessageMapFileSaveDialog->FileName = "msgmap.tjs";
	}

// 	if(MessageMapFileSaveDialog->Execute())
// 	{
// 		TVPCreateMessageMapFile(MessageMapFileSaveDialog->FileName);
// 	}
}
//---------------------------------------------------------------------------
void TTVPMainForm::RestartScriptEngineMenuItemClick(TObject *Sender)
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
	//::PostMessage(TVPMainForm->Handle, WM_USER+0x31/*dummy msg*/, 0, 0);
//     if(!SystemWatchTimer)
//         SystemWatchTimer = SDL_AddTimer(50, SDL_SystemWatchTimerTimer, this);
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
			//SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
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
            //SDL_SetThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
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
    //if(TVPHaltWarnForm) delete TVPHaltWarnForm, TVPHaltWarnForm = NULL;
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
// 	if(ApplicationStayOnTop)
// 		SetWindowPos(Application->Handle,HWND_TOPMOST ,0,0,0,0,
// 			SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
// 	else
// 		SetWindowPos(Application->Handle,HWND_NOTOPMOST ,0,0,0,0,
// 			SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
}
//---------------------------------------------------------------------------
// void TTVPMainForm::WMInvokeEvents(TMessage &Msg)
// {
// 	// indirectly called by TVPInvokeEvents  *** currently not used ***
// 	if(EventButton->getDown())
// 	{
// 		TVPDeliverAllEvents();
// 	}
// }
//---------------------------------------------------------------------------
void TTVPMainForm::DeliverEvents()
{
	if(ContinuousEventCalling)
		TVPProcessContinuousHandlerEventFlag = true; // set flag

	/*if(EventButton->getDown())*/ TVPDeliverAllEvents();
}
//---------------------------------------------------------------------------
void TTVPMainForm::ApplicationIdle(TObject *Sender, bool &Done)
{
	DeliverEvents();

	bool cont = ContinuousEventCalling;
	Done = !cont;

	MixedIdleTick += TVPGetRoughTickCount32();
}
//---------------------------------------------------------------------------
void TTVPMainForm::ApplicationActivate()
{
	ApplicationActivating = true;

	TVPRestoreFullScreenWindowAtActivation();
	//TVPShowModalAtAppActivate();
//	TVPShowFontSelectFormAtAppActivate();
	TVPResetVolumeToAllSoundBuffer();
	UnlockSoundMixer();

	// trigger System.onActivate event
	TVPPostApplicationActivateEvent();
}
//---------------------------------------------------------------------------
void TTVPMainForm::ApplicationDeactivate()
{
	ApplicationActivating = false;

	//TVPHideModalAtAppDeactivate();
//	TVPHideFontSelectFormAtAppDeactivate();

	TVPMinimizeFullScreenWindowAtInactivation();


	// fire compact event
	TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_DEACTIVATE);

	// set application-level stay-on-top state
// 	if(ApplicationStayOnTop)
// 		SetWindowPos(Application->Handle,HWND_TOPMOST ,0,0,0,0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
// 	else
// 		SetWindowPos(Application->Handle,HWND_NOTOPMOST ,0,0,0,0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

	// set sound volume
	TVPResetVolumeToAllSoundBuffer();
	LockSoundMixer();

	// trigger System.onDeactivate event
	TVPPostApplicationDeactivateEvent();
}
//---------------------------------------------------------------------------
void TTVPMainForm::ApplicationMinimize(TObject *Sender)
{
	// fire compact event
	ApplicationNotMinimizing = false;

	TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MINIMIZE);

	// set sound volume
	TVPResetVolumeToAllSoundBuffer();
}
//---------------------------------------------------------------------------
void TTVPMainForm::ApplicationRestore(TObject *Sender)
{
	ApplicationNotMinimizing = true;

	// set sound volume
	TVPResetVolumeToAllSoundBuffer();
}
extern tjs_uint TVPTotalPhysMemory;
//---------------------------------------------------------------------------
void TTVPMainForm::SystemWatchTimerTimer(TObject *)
{
// 	if(TVPTerminated)
// 	{
// 		TVPExitApplication(0);
// 	}

	// call events
	uint32_t tick = TVPGetRoughTickCount32();

	// push environ noise
	TVPPushEnvironNoise(&tick, sizeof(tick));
	TVPPushEnvironNoise(&LastCompactedTick, sizeof(LastCompactedTick));
	TVPPushEnvironNoise(&LastShowModalWindowSentTick, sizeof(LastShowModalWindowSentTick));
	TVPPushEnvironNoise(&MixedIdleTick, sizeof(MixedIdleTick));
// 	POINT pt;
// 	GetCursorPos(&pt);
// 	TVPPushEnvironNoise(&pt, sizeof(pt));
	
	// CPU clock monitoring
// 	{
// 		static bool clock_rough_printed = false;
// 		if(!clock_rough_printed && TVPCPUClockAccuracy == ccaRough)
// 		{
// 			tjs_char msg[80];
// 			TJS_sprintf(msg, TJS_W("(info) CPU clock (roughly) : %dMHz"), (int)TVPCPUClock);
// 			TVPAddImportantLog(msg);
// 			clock_rough_printed = true;
// 		}
// 		static bool clock_printed = false;
// 		if(!clock_printed && TVPCPUClockAccuracy == ccaAccurate)
// 		{
// 			tjs_char msg[80];
// 			TJS_sprintf(msg, TJS_W("(info) CPU clock : %.1fMHz"), (float)TVPCPUClock);
// 			TVPAddImportantLog(msg);
// 			clock_printed = true;
// 		}
// 	}

	// check status and deliver events
	DeliverEvents();

	if (tick - LastCompactedTick > 60000 || (!ContinuousEventCalling && tick - LastCompactedTick > 4000))
	{
		// idle state over 4 sec. or forced clear after 1 min
		LastCompactedTick = tick;

		// fire compact event
		if (TVPGetSelfUsedMemory() > 1536 || 
			TVPGetSystemFreeMemory() < (TVPTotalPhysMemory / 1024) / 10) {
			//less than 10% memory
			TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX);
		} else {
			TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_IDLE);
		}
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
// 		if(TVPHaltWarnForm)
// 		{
// 			TVPHaltWarnForm->Visible = true;
// 		}
// 		else
// 		{
// 			TVPHaltWarnForm = new TTVPHaltWarnForm(Application);
// 			TVPHaltWarnForm->Visible = true;
// 		}
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

TTVPMainForm::~TTVPMainForm()
{
    //if(SystemWatchTimer) SDL_RemoveTimer(SystemWatchTimer);
}

//---------------------------------------------------------------------------
// void TTVPMainForm::WMRearrangeModalWindows(TMessage &Msg)
// {
// 	if(TVPFullScreenedWindow != NULL && TVPGetModalWindowRearrangeInFullScreen())
// 	{
// 		HDWP hdwp = BeginDeferWindowPos(1);
// 		hdwp = TVPShowModalAtTimer(hdwp);
// 		hdwp = TVPShowFontSelectFormTop(hdwp);
// 		hdwp = TVPShowHintWindowTop(hdwp);
// 		EndDeferWindowPos(hdwp);
// 	}
// }
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
	char *buf = new char [str.length() * 2 + 1];  // enough to hold
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
	int strlen = str.length();
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
void tTVPProfileHolder::WriteStrings(const AnsiString &section,
	const AnsiString &ident, TStrings * strings)
{
	//WriteString(section, ident, TVPEscapeAnsiString(strings->getText()));
}
//---------------------------------------------------------------------------
void tTVPProfileHolder::ReadStrings(const AnsiString &section,
	const AnsiString &ident, TStrings * strings)
{
	AnsiString str;
	str = ReadString(section, ident, "");
	//if(str != "") strings->getText() = TVPUnescapeAnsiString(str);
}
//---------------------------------------------------------------------------







