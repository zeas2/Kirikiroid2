//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Timer Object Implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <algorithm>
#include "EventIntf.h"
#include "TickCount.h"
#include "TimerImpl.h"
#include "SysInitIntf.h"
#include "ThreadIntf.h"
#include "MsgIntf.h"

#include "NativeEventQueue.h"
#include "UserEvent.h"

//---------------------------------------------------------------------------

// TVP Timer class gives ability of triggering event on punctual interval.
// a large quantity of event at once may easily cause freeze to system,
// so we must trigger only porocess-able quantity of the event.
#define TVP_LEAST_TIMER_INTERVAL 3
#define INFINITE 0xFFFFFFFF


//---------------------------------------------------------------------------
// tTVPTimerThread
//---------------------------------------------------------------------------
class tTVPTimerThread : public tTVPThread
{
	// thread for triggering punctual event.
	// normal Windows timer cannot call the timer callback routine at
	// too short interval ( roughly less than 50ms ).

	std::vector<tTJSNI_Timer *> List;
	std::vector<tTJSNI_Timer *> Pending; // timer object which has pending events
	bool PendingEventsAvailable;
	tTVPThreadEvent Event;
	
	NativeEventQueue<tTVPTimerThread> EventQueue;

public:

	tTJSCriticalSection TVPTimerCS;

	tTVPTimerThread();
	~tTVPTimerThread();

protected:
	void Execute();

private:
	void Proc( NativeEvent& event );

	void AddItem(tTJSNI_Timer * item);
	bool RemoveItem(tTJSNI_Timer *item);
	void RemoveFromPendingItem(tTJSNI_Timer *item);
	void RegisterToPendingItem(tTJSNI_Timer *item);

public:
	void SetEnabled(tTJSNI_Timer *item, bool enabled); // managed by this class
	void SetInterval(tTJSNI_Timer *item, tjs_uint64 interval); // managed by this class

public:
	static void Init();
	static void Uninit();

	static void Add(tTJSNI_Timer * item);
	static void Remove(tTJSNI_Timer *item);

	static void RemoveFromPending(tTJSNI_Timer *item);
	static void RegisterToPending(tTJSNI_Timer *item);

} static * TVPTimerThread = NULL;
//---------------------------------------------------------------------------
tTVPTimerThread::tTVPTimerThread() : tTVPThread(true), EventQueue(this,&tTVPTimerThread::Proc)
{
	PendingEventsAvailable = false;
	SetPriority(TVPLimitTimerCapacity ? ttpNormal : ttpHighest);
	EventQueue.Allocate();
	Resume();
}
//---------------------------------------------------------------------------
tTVPTimerThread::~tTVPTimerThread()
{
	Terminate();
	Resume();
	Event.Set();
	WaitFor();
	EventQueue.Deallocate();
}
//---------------------------------------------------------------------------
void tTVPTimerThread::Execute()
{
	while(!GetTerminated())
	{
		tjs_uint64 step_next = (tjs_uint64)(tjs_int64)-1L; // invalid value
		tjs_uint64 curtick = TVPGetTickCount() << TVP_SUBMILLI_FRAC_BITS;
		tjs_uint32 sleeptime;

		{	// thread-protected
			tTJSCriticalSectionHolder holder(TVPTimerCS);

			bool any_triggered = false;

			std::vector<tTJSNI_Timer*>::iterator i;
			for(i = List.begin(); i!=List.end(); i ++)
			{
				tTJSNI_Timer * item = *i;

				if(!item->GetEnabled() || item->GetInterval() == 0) continue;

				if(item->GetNextTick() < curtick)
				{
					tjs_uint n = static_cast<tjs_uint>( (curtick - item->GetNextTick()) / item->GetInterval() );
					n++;
					if(n > 40)
					{
						// too large amount of event at once; discard rest
						item->Trigger(1);
						any_triggered = true;
						item->SetNextTick(curtick + item->GetInterval());
					}
					else
					{
						item->Trigger(n);
						any_triggered = true;
						item->SetNextTick(item->GetNextTick() +
							n * item->GetInterval());
					}
				}


				tjs_uint64 to_next = item->GetNextTick() - curtick;

				if(step_next == (tjs_uint64)(tjs_int64)-1L)
				{
					step_next = to_next;
				}
				else
				{
					if(step_next > to_next) step_next = to_next;
				}
			}


			if(step_next != (tjs_uint64)(tjs_int64)-1L)
			{
				// too large step_next must be diminished to size of DWORD.
				if(step_next >= 0x80000000)
					sleeptime = 0x7fffffff; // smaller value than step_next is OK
				else
					sleeptime = static_cast<tjs_uint32>( step_next );
			}
			else
			{
				sleeptime = INFINITE;
			}

			if(List.size() == 0) sleeptime = INFINITE;

			if(any_triggered)
			{
				// triggered; post notification message to the UtilWindow
				if(!PendingEventsAvailable)
				{
					PendingEventsAvailable = true;
					EventQueue.PostEvent( NativeEvent(TVP_EV_TIMER_THREAD) );
				}
			}

		}	// end-of-thread-protected

		// now, sleeptime has sub-milliseconds precision but we need millisecond
		// precision time.
		if(sleeptime != INFINITE)
			sleeptime = (sleeptime >> TVP_SUBMILLI_FRAC_BITS) +
							(sleeptime & ((1<<TVP_SUBMILLI_FRAC_BITS)-1) ? 1: 0); // round up

		// clamp to TVP_LEAST_TIMER_INTERVAL ...
		if(sleeptime != INFINITE && sleeptime < TVP_LEAST_TIMER_INTERVAL)
			sleeptime = TVP_LEAST_TIMER_INTERVAL;

		Event.WaitFor(sleeptime); // wait until sleeptime is elapsed or
									// Event->SetEvent() is executed.
	}
}
//---------------------------------------------------------------------------
//void __fastcall tTVPTimerThread::UtilWndProc(Messages::TMessage &Msg)
void tTVPTimerThread::Proc( NativeEvent& ev )
{
	// Window procedure of UtilWindow
	if( ev.Message == TVP_EV_TIMER_THREAD && !GetTerminated())
	{
		// pending events occur
		tTJSCriticalSectionHolder holder(TVPTimerCS); // protect the object

		std::vector<tTJSNI_Timer *>::iterator i;
		for(i = Pending.begin(); i!=Pending.end(); i ++)
		{
			tTJSNI_Timer * item = *i;
			item->FirePendingEventsAndClear();
		}

		Pending.clear();
		PendingEventsAvailable = false;
	}
	else
	{
		EventQueue.HandlerDefault(ev);
	}
}
//---------------------------------------------------------------------------
void tTVPTimerThread::AddItem(tTJSNI_Timer * item)
{
	tTJSCriticalSectionHolder holder(TVPTimerCS);

	if(std::find(List.begin(), List.end(), item) == List.end())
		List.push_back(item);
}
//---------------------------------------------------------------------------
bool tTVPTimerThread::RemoveItem(tTJSNI_Timer *item)
{
	tTJSCriticalSectionHolder holder(TVPTimerCS);

	std::vector<tTJSNI_Timer *>::iterator i;

	// remove from the List
	for(i = List.begin(); i != List.end(); /**/)
	{
		if(*i == item) i = List.erase(i); else i++;
	}

	// also remove from the Pending list
	RemoveFromPendingItem(item);

	return List.size() != 0;
}
//---------------------------------------------------------------------------
void tTVPTimerThread::RemoveFromPendingItem(tTJSNI_Timer *item)
{
	// remove item from pending list
 	std::vector<tTJSNI_Timer *>::iterator i;
	for(i = Pending.begin(); i != Pending.end(); /**/)
	{
		if(*i == item) i = Pending.erase(i); else i++;
	}

	item->ZeroPendingCount();
}
//---------------------------------------------------------------------------
void tTVPTimerThread::RegisterToPendingItem(tTJSNI_Timer *item)
{
	// register item to the pending list
	Pending.push_back(item);
}
//---------------------------------------------------------------------------
void tTVPTimerThread::SetEnabled(tTJSNI_Timer *item, bool enabled)
{
	{ // thread-protected
		tTJSCriticalSectionHolder holder(TVPTimerCS);

		item->InternalSetEnabled(enabled);
		if(enabled)
		{
			item->SetNextTick((TVPGetTickCount()  << TVP_SUBMILLI_FRAC_BITS) + item->GetInterval());
		}
		else
		{
			item->CancelEvents();
			item->ZeroPendingCount();
		}
	} // end-of-thread-protected

	if(enabled) Event.Set();
}
//---------------------------------------------------------------------------
void tTVPTimerThread::SetInterval(tTJSNI_Timer *item, tjs_uint64 interval)
{
	{ // thread-protected
		tTJSCriticalSectionHolder holder(TVPTimerCS);

		item->InternalSetInterval(interval);
		if(item->GetEnabled())
		{
			item->CancelEvents();
			item->ZeroPendingCount();
			item->SetNextTick((TVPGetTickCount()  << TVP_SUBMILLI_FRAC_BITS) + item->GetInterval());
		}
	} // end-of-thread-protected

	if(item->GetEnabled()) Event.Set();

}
//---------------------------------------------------------------------------
void tTVPTimerThread::Init()
{
	if(!TVPTimerThread)
	{
		TVPStartTickCount(); // in TickCount.cpp
		TVPTimerThread = new tTVPTimerThread();
	}
}
//---------------------------------------------------------------------------
void tTVPTimerThread::Uninit()
{
	if(TVPTimerThread)
	{
		delete TVPTimerThread;
		TVPTimerThread = NULL;
	}
}
//---------------------------------------------------------------------------
static tTVPAtExit TVPTimerThreadUninitAtExit(TVP_ATEXIT_PRI_SHUTDOWN,
	tTVPTimerThread::Uninit);
//---------------------------------------------------------------------------
void tTVPTimerThread::Add(tTJSNI_Timer * item)
{
	// at this point, item->GetEnebled() must be false.

	Init();

	TVPTimerThread->AddItem(item);
}
//---------------------------------------------------------------------------
void tTVPTimerThread::Remove(tTJSNI_Timer *item)
{
	if(TVPTimerThread)
	{
		if(!TVPTimerThread->RemoveItem(item)) Uninit();
	}
}
//---------------------------------------------------------------------------
void tTVPTimerThread::RemoveFromPending(tTJSNI_Timer *item)
{
	if(TVPTimerThread)
	{
		TVPTimerThread->RemoveFromPendingItem(item);
	}
}
//---------------------------------------------------------------------------
void tTVPTimerThread::RegisterToPending(tTJSNI_Timer *item)
{
	if(TVPTimerThread)
	{
		TVPTimerThread->RegisterToPendingItem(item);
	}
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// tTJSNI_Timer
//---------------------------------------------------------------------------
tTJSNI_Timer::tTJSNI_Timer()
{
	NextTick = 0;
	Interval = 1000;
	PendingCount = 0;
	Enabled = false;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSNI_Timer::Construct(tjs_int numparams,
	tTJSVariant **param, iTJSDispatch2 *tjs_obj)
{
	inherited::Construct(numparams, param, tjs_obj);

	tTVPTimerThread::Add(this);
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_Timer::Invalidate()
{
	tTVPTimerThread::Remove(this);
	ZeroPendingCount();
	CancelEvents();
	inherited::Invalidate();  // this sets Owner = NULL
}
//---------------------------------------------------------------------------
void tTJSNI_Timer::SetInterval(tjs_uint64 n)
{
	TVPTimerThread->SetInterval(this, n);
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSNI_Timer::GetInterval() const
{
	return Interval;
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSNI_Timer::GetNextTick() const
{
	return NextTick;
}
//---------------------------------------------------------------------------
void tTJSNI_Timer::SetEnabled(bool b)
{
	TVPTimerThread->SetEnabled(this, b);
}
//---------------------------------------------------------------------------
void tTJSNI_Timer::Trigger(tjs_uint n)
{
	// this function is called by sub-thread.
	if(PendingCount == 0) tTVPTimerThread::RegisterToPending(this);
	PendingCount += n;
}
//---------------------------------------------------------------------------
void tTJSNI_Timer::FirePendingEventsAndClear()
{
	// fire all pending events and clear the pending event count
	if(PendingCount)
	{
		Fire(PendingCount);
		ZeroPendingCount();
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_Timer
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_Timer::CreateNativeInstance()
{
	return new tTJSNI_Timer();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPCreateNativeClass_Timer
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Timer()
{
	return new tTJSNC_Timer();
}
//---------------------------------------------------------------------------

