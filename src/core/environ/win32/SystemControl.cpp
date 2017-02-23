#include "tjsCommHead.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "SystemControl.h"
#include "EventIntf.h"
#include "MsgIntf.h"
//#include "WindowFormUnit.h"
#include "SysInitIntf.h"
#include "SysInitImpl.h"
#include "ScriptMgnIntf.h"
#include "WindowIntf.h"
#include "WindowImpl.h"
#include "StorageIntf.h"
#include "EmergencyExit.h" // for TVPCPUClock
#include "DebugIntf.h"
//#include "VersionFormUnit.h"
#include "WaveImpl.h"
#include "SystemImpl.h"
#include "UserEvent.h"
#include "Application.h"
#include "TickCount.h"
#include "Random.h"

tTVPSystemControl *TVPSystemControl;
bool TVPSystemControlAlive = false;

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


tTVPSystemControl::tTVPSystemControl() : EventEnable(true) {
	ContinuousEventCalling = false;
	AutoShowConsoleOnError = false;

	LastCompactedTick = 0;
	LastCloseClickedTick = 0;
	LastShowModalWindowSentTick = 0;
	LastRehashedTick = 0;

	TVPSystemControlAlive = true;
#if 0
	SystemWatchTimer.SetInterval(50);
	SystemWatchTimer.SetOnTimerHandler( this, &tTVPSystemControl::SystemWatchTimerTimer );
	SystemWatchTimer.SetEnabled( true );
#endif
}
void tTVPSystemControl::InvokeEvents() {
	CallDeliverAllEventsOnIdle();
}
void tTVPSystemControl::CallDeliverAllEventsOnIdle() {
//	Application->PostMessageToMainWindow( TVP_EV_DELIVER_EVENTS_DUMMY, 0, 0 );
}

void tTVPSystemControl::BeginContinuousEvent() {
	if(!ContinuousEventCalling)
	{
		ContinuousEventCalling = true;
		InvokeEvents();
		if(TVPGetMainThreadPriorityControl())
		{
			// make main thread priority lower
//			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
		}
	}
}
void tTVPSystemControl::EndContinuousEvent() {
	if(ContinuousEventCalling)
	{
		ContinuousEventCalling = false;
		if(TVPGetMainThreadPriorityControl())
		{
			// make main thread priority normal
//			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		}
	}
}
//---------------------------------------------------------------------------
void tTVPSystemControl::NotifyCloseClicked() {
	// close Button is clicked
	LastCloseClickedTick = TVPGetRoughTickCount32();
}

void tTVPSystemControl::NotifyEventDelivered() {
	// called from event system, notifying the event is delivered.
	LastCloseClickedTick = 0;
	// if(TVPHaltWarnForm) delete TVPHaltWarnForm, TVPHaltWarnForm = NULL;
}

bool tTVPSystemControl::ApplicationIdle() {
	DeliverEvents();
	bool cont = !ContinuousEventCalling;
	MixedIdleTick += TVPGetRoughTickCount32();
	return cont;
}

void tTVPSystemControl::DeliverEvents() {
	if(ContinuousEventCalling)
		TVPProcessContinuousHandlerEventFlag = true; // set flag

	if (EventEnable) {
		TVPDeliverAllEvents();
	}
}

void tTVPSystemControl::SystemWatchTimerTimer() {
	if( TVPTerminated ) {
		// this will ensure terminating the application.
		// the WM_QUIT message disappears in some unknown situations...
//		Application->PostMessageToMainWindow( TVP_EV_DELIVER_EVENTS_DUMMY, 0, 0 );
		Application->Terminate();
//		Application->PostMessageToMainWindow( TVP_EV_DELIVER_EVENTS_DUMMY, 0, 0 );
	}

	// call events
	uint32_t tick = TVPGetRoughTickCount32();
	// push environ noise
	TVPPushEnvironNoise(&tick, sizeof(tick));
	TVPPushEnvironNoise(&LastCompactedTick, sizeof(LastCompactedTick));
	TVPPushEnvironNoise(&LastShowModalWindowSentTick, sizeof(LastShowModalWindowSentTick));
	TVPPushEnvironNoise(&MixedIdleTick, sizeof(MixedIdleTick));
#if 0
	POINT pt;
	::GetCursorPos(&pt);
	TVPPushEnvironNoise(&pt, sizeof(pt));

	// CPU clock monitoring
	{
		static bool clock_rough_printed = false;
		if( !clock_rough_printed && TVPCPUClockAccuracy == ccaRough ) {
			tjs_char msg[80];
			TJS_snprintf(msg, 80, TVPInfoCpuClockRoughly, (int)TVPCPUClock);
			TVPAddImportantLog(msg);
			clock_rough_printed = true;
		}
		static bool clock_printed = false;
		if( !clock_printed && TVPCPUClockAccuracy == ccaAccurate ) {
			tjs_char msg[80];
			TJS_snprintf(msg, 80, TVPInfoCpuClock, (float)TVPCPUClock);
			TVPAddImportantLog(msg);
			clock_printed = true;
		}
	}
#endif
	// check status and deliver events
	DeliverEvents();

	// call TickBeat
	tjs_int count = TVPGetWindowCount();
	for( tjs_int i = 0; i<count; i++ ) {
		tTJSNI_Window *win = TVPGetWindowListAt(i);
		win->TickBeat();
	}

	if( !ContinuousEventCalling && tick - LastCompactedTick > 4000 ) {
		// idle state over 4 sec.
		LastCompactedTick = tick;

		// fire compact event
		TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_IDLE);
	}
	if( !ContinuousEventCalling && tick > LastRehashedTick + 1500 ) {
		// TJS2 object rehash
		LastRehashedTick = tick;
		TJSDoRehash();
	}
	// ensure modal window visible
	if( tick > LastShowModalWindowSentTick + 4100 ) {
		//	::PostMessage(Handle, WM_USER+0x32, 0, 0);
		// This is currently disabled because IME composition window
		// hides behind the window which is bringed top by the
		// window-rearrangement.
		LastShowModalWindowSentTick = tick;
	}
}

