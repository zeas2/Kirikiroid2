//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Script/Window Event Handling and Dispatching / System Idle Event Delivering
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <algorithm>
#include "SysInitIntf.h"
#include "EventIntf.h"
#include "WindowIntf.h"
#include "tjsDictionary.h"
#include "MsgIntf.h"
#include "ScriptMgnIntf.h"
#include "TickCount.h"
#include "SystemImpl.h"




//---------------------------------------------------------------------------
// tTVPEvent  : script event class
//---------------------------------------------------------------------------
extern tjs_uint64 TVPEventSequenceNumber;
class tTVPEvent
{
	iTJSDispatch2 *Target;
	iTJSDispatch2 *Source;
	ttstr EventName;
	tjs_uint32 Tag;
	tjs_uint NumArgs;
	tTJSVariant *Args;
	tjs_uint32 Flags;
	tjs_uint64 Sequence;

public:
	tTVPEvent(iTJSDispatch2 *target, iTJSDispatch2 *source,
		ttstr &eventname, tjs_uint32 tag, tjs_uint numargs, tTJSVariant *args,
		tjs_uint32 flags)
	{
		// constructor

		// eventname is not a const object but this object only touch to
		// eventname.GetHint()

		Args = NULL;
		Target = NULL;
		Source = NULL;

		Sequence = TVPEventSequenceNumber;
		EventName = eventname;
		NumArgs = numargs;
		Args = new tTJSVariant[NumArgs];
		for(tjs_uint i=0; i<NumArgs; i++)
			Args[i]=args[i];
		Target = target;
		Source = source;
		Tag = tag;
		Flags = flags;
		if(Target) Target->AddRef();
		if(Source) Source->AddRef();
	}


	tTVPEvent(const tTVPEvent &ref)
	{
		// copy constructor
		Args = NULL;
		Target = NULL;
		Source = NULL;

		EventName = ref.EventName;
		NumArgs = ref.NumArgs;
		Args = new tTJSVariant[NumArgs];
		for(tjs_uint i=0; i<NumArgs; i++)
			Args[i]=ref.Args[i];
		Target = ref.Target;
		Source = ref.Source;
		Tag = ref.Tag;
		if(Target) Target->AddRef();
		if(Source) Source->AddRef();
	}

	~tTVPEvent()
	{
		if(Args) delete [] Args;
		if(Target) Target->Release();
		if(Source) Source->Release();
	}

	void Deliver()
	{
		if(!TJSIsObjectValid(Target->IsValid(0, NULL, NULL, Target)))
			return; // The target had been invalidated
		tTJSVariant **ArgsPtr = new tTJSVariant*[NumArgs];
		for(tjs_uint i=0; i<NumArgs; i++)
			ArgsPtr[i] = Args + i;
		try
		{
			Target->FuncCall(0, EventName.c_str(), EventName.GetHint(),
				NULL, NumArgs, ArgsPtr,
				Target);
		}
		catch(...)
		{
			delete [] ArgsPtr;
			throw;
		}
		delete [] ArgsPtr;
	}


	iTJSDispatch2 * GetTargetNoAddRef() const { return Target; }
	iTJSDispatch2 * GetSourceNoAddRef() const { return Source; }
	ttstr & GetEventName() { return EventName; }
	tjs_uint32 GetTag() const { return Tag; }
	tjs_uint32 GetFlags() const { return Flags; }
	tjs_uint64 GetSequence() const;
};
//---------------------------------------------------------------------------
tjs_uint64 tTVPEvent::GetSequence() const { return Sequence; }
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPWinUpdateEvent : window update event class
//---------------------------------------------------------------------------
class tTVPWinUpdateEvent
{
	tTJSNI_BaseWindow *Window;

public:
	tTVPWinUpdateEvent(tTJSNI_BaseWindow *window)
	{
		Window = window;
	}

	tTVPWinUpdateEvent(const tTVPWinUpdateEvent & ref)
	{
		Window = ref.Window;
	}

	~tTVPWinUpdateEvent()
	{
	}

	void Deliver() const
	{
		if (static_cast<tTJSNI_Window*>(Window)->GetVisible())
			Window->UpdateContent();
	}

    tTJSNI_BaseWindow * GetWindow() const { return Window; }

	void MarkEmpty() { Window = NULL; }

	bool IsEmpty() const { return Window == NULL; }
};
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// global/static definitions
//---------------------------------------------------------------------------
// event queue must be a globally sequential queue
std::vector<tTVPBaseInputEvent *> TVPInputEventQueue;
std::vector<tTVPEvent *> TVPEventQueue;
std::vector<tTVPWinUpdateEvent> TVPWinUpdateEventQueue;
bool TVPExclusiveEventPosted = false; // true if exclusive event is posted
tjs_uint64 TVPEventSequenceNumber = 0; // event sequence number
tjs_uint64 TVPEventSequenceNumberToProcess = 0;
	// current event sequence which must be processed

static void TVPDestroyEventQueue()
{
	// delete all event objects
	// deletion of event object may cause other deletion of event objects.
	{
		std::vector<tTVPEvent *>::iterator i;
		while(TVPEventQueue.size())
		{
			i = TVPEventQueue.end() -1;
			tTVPEvent * ev = *i;
			TVPEventQueue.erase(i);
			delete ev;
		}
	}
//--
	{
		std::vector<tTVPBaseInputEvent *>::iterator i;
		while(TVPInputEventQueue.size())
		{
			i = TVPInputEventQueue.end() - 1;
			tTVPBaseInputEvent * ev = *i;
			TVPInputEventQueue.erase(i);
			delete ev;
		}
	}
}

static tTVPAtExit TVPDestroyEventQueueAtExit
	(TVP_ATEXIT_PRI_PREPARE, TVPDestroyEventQueue);

bool TVPEventDisabled = false;
bool TVPEventInterrupting = false;

//#define TVP_EVENT_TASK_RETURN_TICK 100000
	/* TVP event system once returns to Operation system when
		TVP_EVENT_TASK_RETURN_TICK is elapsed during event delivering. */
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// TVPPostEvent
//---------------------------------------------------------------------------
void TVPPostEvent(iTJSDispatch2 * source, iTJSDispatch2 *target,
	ttstr &eventname, tjs_uint32 tag, tjs_uint32 flag,
	tjs_uint numargs, tTJSVariant *args)
{
	bool evdisabled = TVPEventDisabled || TVPGetSystemEventDisabledState();

	if((flag & TVP_EPT_DISCARDABLE) &&
		(TVPEventInterrupting || evdisabled)) return;

	tjs_int method = flag & TVP_EPT_METHOD_MASK;

	if(method == TVP_EPT_IMMEDIATE)
	{
		// the event is delivered immediately

		if(evdisabled) return;

		try
		{
			try
			{
				tTVPEvent(target, source, eventname, tag, numargs, args, flag).
					Deliver();
			}
			TJS_CONVERT_TO_TJS_EXCEPTION
		}
		TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION(TJS_W("immediate event"));

		return;
	}


	if(method == TVP_EPT_REMOVE_POST)
	{
		// events in queue that have same target/source/name/tag are to be removed
		std::vector<tTVPEvent *>::iterator i;
		i = TVPEventQueue.begin();
		while(/*TVPEventQueue.size() &&*/ i != TVPEventQueue.end())
		{
			if(source == (*i)->GetSourceNoAddRef() &&
				target == (*i)->GetTargetNoAddRef() &&
				eventname == (*i)->GetEventName() &&
				((tag==0)?true:(tag==(*i)->GetTag())) )
			{
				tTVPEvent *ev = *i;
				TVPEventQueue.erase(i);
				i = TVPEventQueue.begin();
				delete ev;
			}
			else
			{
				i++;
			}
		}
	}

	// put into queue
	TVPEventQueue.push_back(new tTVPEvent(target, source, eventname, tag,
									numargs, args, flag));

	// is exclusive?
	if((flag & TVP_EPT_PRIO_MASK) == TVP_EPT_EXCLUSIVE) TVPExclusiveEventPosted = true;

	// make sure that the event is to be delivered.
	TVPInvokeEvents();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPCancelEvents
//---------------------------------------------------------------------------
tjs_int TVPCancelEvents(iTJSDispatch2 * source, iTJSDispatch2 *target,
	const ttstr &eventname, tjs_uint32 tag)
{
	tjs_int count = 0;
	std::vector<tTVPEvent *>::iterator i;
	i = TVPEventQueue.begin();
	while(/*TVPEventQueue.size() &&*/ i != TVPEventQueue.end())
	{
		if(source == (*i)->GetSourceNoAddRef() &&
			target == (*i)->GetTargetNoAddRef() &&
			eventname == (*i)->GetEventName() &&
				((tag==0)?true:(tag==(*i)->GetTag())) )
		{
			tTVPEvent *ev = *i;
			TVPEventQueue.erase(i);
			i = TVPEventQueue.begin();
			delete ev;
			count ++;
		}
		else
		{
			i++;
		}
	}
	return count;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPAreEventsInQueue
//---------------------------------------------------------------------------
bool TVPAreEventsInQueue(iTJSDispatch2 * source, iTJSDispatch2 *target,
	const ttstr &eventname, tjs_uint32 tag)
{
	std::vector<tTVPEvent *>::iterator i;
	i = TVPEventQueue.begin();
	while(/*TVPEventQueue.size() &&*/ i != TVPEventQueue.end())
	{
		if(source == (*i)->GetSourceNoAddRef() &&
			target == (*i)->GetTargetNoAddRef() &&
			eventname == (*i)->GetEventName() &&
				((tag==0)?true:(tag==(*i)->GetTag())) )
		return true;
		i++;
	}
	return false;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPCountEventsInQueue
//---------------------------------------------------------------------------
tjs_int TVPCountEventsInQueue(iTJSDispatch2 * source, iTJSDispatch2 *target,
	const ttstr &eventname, tjs_uint32 tag)
{
	tjs_int count = 0;
	std::vector<tTVPEvent *>::iterator i;
	i = TVPEventQueue.begin();
	while(/*TVPEventQueue.size() &&*/ i != TVPEventQueue.end())
	{
		if(source == (*i)->GetSourceNoAddRef() &&
			target == (*i)->GetTargetNoAddRef() &&
			eventname == (*i)->GetEventName() &&
				((tag==0)?true:(tag==(*i)->GetTag())) )
		count ++;
		i++;
	}
	return count;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPCancelEventByTag
//---------------------------------------------------------------------------
void TVPCancelEventsByTag(iTJSDispatch2 * source, iTJSDispatch2 *target,
	tjs_uint32 tag)
{
	std::vector<tTVPEvent *>::iterator i;
	i = TVPEventQueue.begin();
	while(/*TVPEventQueue.size() &&*/ i != TVPEventQueue.end())
	{
		if(source == (*i)->GetSourceNoAddRef() &&
			target == (*i)->GetTargetNoAddRef() &&
				((tag==0)?true:(tag==(*i)->GetTag())) )
		{
			tTVPEvent *ev = *i;
			TVPEventQueue.erase(i);
			i = TVPEventQueue.begin();
			delete ev;
		}
		else
		{
			i++;
		}
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPCancelSourceEvent
//---------------------------------------------------------------------------
void TVPCancelSourceEvents(iTJSDispatch2 * source)
{
	std::vector<tTVPEvent *>::iterator i;
	i = TVPEventQueue.begin();
	while(/*TVPEventQueue.size() &&*/ i != TVPEventQueue.end())
	{
		if(source == (*i)->GetSourceNoAddRef())
		{
			tTVPEvent *ev = *i;
			TVPEventQueue.erase(i);
			i = TVPEventQueue.begin();
			delete ev;
		}
		else
		{
			i++;
		}
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPDiscardAllDiscardableEvents
//---------------------------------------------------------------------------
void TVPDiscardAllDiscardableEvents()
{
	std::vector<tTVPEvent *>::iterator i;
	i = TVPEventQueue.begin();
	while(/*TVPEventQueue.size() &&*/ i != TVPEventQueue.end())
	{
		if((*i)->GetFlags() & TVP_EPT_DISCARDABLE)
		{
			tTVPEvent *ev = *i;
			TVPEventQueue.erase(i);
			i = TVPEventQueue.begin();
			delete ev;
		}
		else
		{
			i++;
		}
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPDeliverAllEvents
//---------------------------------------------------------------------------
static void _TVPDeliverEventByPrio(tjs_uint prio)
{
	while(true)
	{
		tTVPEvent *e;

		// retrieve item to deliver
		if(TVPEventQueue.size() == 0) break;
		std::vector<tTVPEvent *>::iterator i =
			TVPEventQueue.begin();
		while(i != TVPEventQueue.end())
		{
			if((*i)->GetSequence() <= TVPEventSequenceNumberToProcess &&
				(((*i)->GetFlags() & TVP_EPT_PRIO_MASK) == prio)) break;
			i++;
		}
		if(i == TVPEventQueue.end()) break;
		e = *i;
		TVPEventQueue.erase(i);

		// event delivering
		try
		{
			e->Deliver();
		}
		catch(...)
		{
			delete e;
			throw;
		}
		delete e;
	}
}


static bool _TVPDeliverAllEvents2()
{
	TVPExclusiveEventPosted = false;

	// process exclusive events
	_TVPDeliverEventByPrio(TVP_EPT_EXCLUSIVE);

	// check exclusive events
	if(TVPExclusiveEventPosted) return true;

	// process input event queue
	while(true)
	{
		tTVPBaseInputEvent *e;

		// retrieve item to deliver
		if(TVPInputEventQueue.size() == 0) break;
		std::vector<tTVPBaseInputEvent *>::iterator i =
			TVPInputEventQueue.begin();
		e = *i;
		TVPInputEventQueue.erase(i);

		// event delivering
		try
		{
			e->Deliver();
		}
		catch(...)
		{
			delete e;
			throw;
		}
		delete e;

		// check exclusive events
		if(TVPExclusiveEventPosted) return true;

	}

	// process normal event queue
	_TVPDeliverEventByPrio(TVP_EPT_NORMAL);

	// check exclusive events
	if(TVPExclusiveEventPosted) return true;

	return true;
}
//---------------------------------------------------------------------------
static bool _TVPDeliverAllEvents()
{
	// deliver all pending events to targets.
	if(TVPEventDisabled) return true;

	// event invokation was received...
	TVPEventReceived();

	// for script event objects

	bool ret_value;

	ret_value = _TVPDeliverAllEvents2();

	return ret_value;
}
//---------------------------------------------------------------------------
void TVPDeliverAllEvents()
{
	bool r;

	if(!TVPEventInterrupting)
	{
		TVPEventSequenceNumberToProcess = TVPEventSequenceNumber;
		TVPEventSequenceNumber ++; // increment sequence number
	}

	TVPEventInterrupting = false;
	try
	{
	   try
		{
			r = _TVPDeliverAllEvents();
		}
		TJS_CONVERT_TO_TJS_EXCEPTION
	}
	TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION(TJS_W("event"));

	if(!r)
	{
		// event processing is to be interrupted
		// XXX: currently this is not functional
		TVPEventInterrupting = true;
		TVPCallDeliverAllEventsOnIdle();
	}

	if(!TVPExclusiveEventPosted && !TVPEventInterrupting)
	{
		try
		{
			try
			{
				// process idle event queue
				_TVPDeliverEventByPrio(TVP_EPT_IDLE);
			}
			TJS_CONVERT_TO_TJS_EXCEPTION
		}
		TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION(TJS_W("idle event"));

		// process continuous events
		if(TVPProcessContinuousHandlerEventFlag)
		{
			TVPProcessContinuousHandlerEventFlag = false; // processed
			// XXX: strictly saying, we need something like InterlockedExchange
			// to look/set this flag, because TVPProcessContinuousHandlerEventFlag
			// may be accessed by another thread. But I have no dought about
			// that no one does care of missing one event in rare race condition.

			TVPDeliverContinuousEvent();
		}
		try
		{
		   try
			{
				// for window content updating
				TVPDeliverWindowUpdateEvents();
			}
			TJS_CONVERT_TO_TJS_EXCEPTION
		}
		TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION(TJS_W("window update"));
	} else {
	}

	if(TVPEventQueue.size() == 0)
	{
		TVPEventSequenceNumber = 0; // reset the number
	}

}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// TVPPostWindowUpdate
//---------------------------------------------------------------------------
bool TVPWindowUpdateEventsDelivering = false;
void TVPPostWindowUpdate(tTJSNI_BaseWindow *window)
{

	if(!TVPWindowUpdateEventsDelivering)
	{
		if(TVPWinUpdateEventQueue.size())
		{
			// since duplication is not allowed ...
			std::vector<tTVPWinUpdateEvent>::const_iterator i;
			for(i = TVPWinUpdateEventQueue.begin();
				i !=TVPWinUpdateEventQueue.end(); i++)
			{
				if(!i->IsEmpty() && window == i->GetWindow()) return;
			}
		}
	}
	else
	{
		if(TVPWinUpdateEventQueue.size())
		{
			// duplication is allowed up to two
			tjs_int count = 0;
			std::vector<tTVPWinUpdateEvent>::const_iterator i;
			for(i = TVPWinUpdateEventQueue.begin();
				i !=TVPWinUpdateEventQueue.end(); i++)
			{
				if(!i->IsEmpty() && window == i->GetWindow())
				{
					count++;
					if(count == 2) return;
				}
			}
		}
	}

	// put into queue.
	TVPWinUpdateEventQueue.emplace_back(window);

	// make sure that the event is to be delivered.
	TVPInvokeEvents();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPRemoveWindowUpdate
//---------------------------------------------------------------------------
void TVPRemoveWindowUpdate(tTJSNI_BaseWindow *window)
{
	// removes all window update events from queue.
	if(TVPWinUpdateEventQueue.size())
	{
		std::vector<tTVPWinUpdateEvent>::iterator i;
		for(i = TVPWinUpdateEventQueue.begin();
			i !=TVPWinUpdateEventQueue.end(); i++)
		{
			if(!i->IsEmpty() && window == i->GetWindow())
				i->MarkEmpty();
		}
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPDeliverWindowUpdateEvents
//---------------------------------------------------------------------------
void TVPDeliverWindowUpdateEvents()
{
	if(TVPWindowUpdateEventsDelivering) return; // does not allow re-entering
	TVPWindowUpdateEventsDelivering = true;

	try
	{
		for(tjs_uint i = 0; i < TVPWinUpdateEventQueue.size(); i++)
		{
			if(!TVPWinUpdateEventQueue[i].IsEmpty())
				TVPWinUpdateEventQueue[i].Deliver();
		}
	}
	catch(...)
	{
		TVPWinUpdateEventQueue.clear();
		TVPWindowUpdateEventsDelivering = false;
		throw;
	}
	TVPWinUpdateEventQueue.clear();
	TVPWindowUpdateEventsDelivering = false;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// Input Event related
//---------------------------------------------------------------------------
tjs_int TVPInputEventTagMax = 0;
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPPostInputEvent
//---------------------------------------------------------------------------
void TVPPostInputEvent(tTVPBaseInputEvent *ev, tjs_uint32 flags)
{
	// flag check
	if((flags & TVP_EPT_DISCARDABLE) &&
		(TVPEventDisabled || TVPGetSystemEventDisabledState()))
	{
		delete ev;
		return;
	}

	if(flags & TVP_EPT_REMOVE_POST)
	{
		// cancel previously posted events
		TVPCancelInputEvents(ev->GetSource(), ev->GetTag());
	}


	// push into the event queue
	TVPInputEventQueue.push_back(ev);

	// make sure that the event is to be delivered.
	TVPInvokeEvents();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPCancelInputEvents
//---------------------------------------------------------------------------
void TVPCancelInputEvents(void * source)
{
	// removes all evens which have the same source
	if(TVPInputEventQueue.size())
	{
		std::vector<tTVPBaseInputEvent *>::iterator i;
		for(i = TVPInputEventQueue.begin();
			i !=TVPInputEventQueue.end();)
		{
			if(source == (*i)->GetSource())
			{
				tTVPBaseInputEvent *ev = *i;
				i = TVPInputEventQueue.erase(i);
				delete ev;
			}
			else
			{
				i++;
			}
		}
	}
}
//---------------------------------------------------------------------------
void TVPCancelInputEvents(void * source, tjs_int tag)
{
	// removes all evens which have the same source and the same tag
	if(TVPInputEventQueue.size())
	{
		std::vector<tTVPBaseInputEvent *>::iterator i;
		for(i = TVPInputEventQueue.begin();
			i !=TVPInputEventQueue.end();)
		{
			if(source == (*i)->GetSource() && tag == (*i)->GetTag())
			{
				tTVPBaseInputEvent *ev = *i;
				i = TVPInputEventQueue.erase(i);
				delete ev;
			}
			else
			{
				i++;
			}
		}
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPGetInputEventCount
//---------------------------------------------------------------------------
tjs_int TVPGetInputEventCount()
{
	return (tjs_int)TVPInputEventQueue.size();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPCreateEventObject
//---------------------------------------------------------------------------
iTJSDispatch2 * TVPCreateEventObject(const tjs_char *type,
	iTJSDispatch2 *targthis, iTJSDispatch2 *targ)
{
	// create a dictionary object for event dispatching ( to "action" method )
	iTJSDispatch2 * object = TJSCreateDictionaryObject();

	static ttstr type_name(TJS_W("type"));
	static ttstr target_name(TJS_W("target"));

	{
		tTJSVariant val(type);
		if(TJS_FAILED(object->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP,
			type_name.c_str(), type_name.GetHint(), &val, object)))
				TVPThrowInternalError;
	}

	{
		tTJSVariant val(targthis, targ);
		if(TJS_FAILED(object->PropSet(TJS_MEMBERENSURE|TJS_IGNOREPROP,
			target_name.c_str(), target_name.GetHint(), &val, object)))
				TVPThrowInternalError;
	}
	
	return object;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
ttstr TVPActionName(TJS_W("action"));
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// Continuous Event Delivering related
//---------------------------------------------------------------------------
bool TVPProcessContinuousHandlerEventFlag = false;
static std::vector<tTVPContinuousEventCallbackIntf *> TVPContinuousEventVector;
static std::vector<tTJSVariantClosure> TVPContinuousHandlerVector;
static bool TVPContinuousEventProcessing = false;

static void TVPDestroyContinuousHandlerVector()
{
	std::vector<tTJSVariantClosure>::iterator i;
	for(i = TVPContinuousHandlerVector.begin();
		i != TVPContinuousHandlerVector.end();
		i++)
	{
		i->Release();
	}
	TVPContinuousHandlerVector.clear();
}

static tTVPAtExit TVPDestroyContinuousHandlerVectorAtExit
	(TVP_ATEXIT_PRI_PREPARE, TVPDestroyContinuousHandlerVector);
//---------------------------------------------------------------------------
void TVPAddContinuousEventHook(tTVPContinuousEventCallbackIntf *cb)
{
	TVPBeginContinuousEvent();
	TVPContinuousEventVector.push_back(cb);
}
//---------------------------------------------------------------------------
void TVPRemoveContinuousEventHook(tTVPContinuousEventCallbackIntf *cb)
{
	std::vector<tTVPContinuousEventCallbackIntf *>::iterator i;
	for(i = TVPContinuousEventVector.begin();
		i !=TVPContinuousEventVector.end();)
	{
		if(cb == *i) *i = NULL; // simply assign a null
		i++;
	}
}
//---------------------------------------------------------------------------
static void _TVPDeliverContinuousEvent() // internal
{
	TVPStartTickCount();
	tjs_uint64 tick = TVPGetTickCount();

	if(TVPContinuousEventVector.size())
	{
		bool emptyflag = false;
		for(tjs_uint32 i = 0; i < TVPContinuousEventVector.size(); i++)
		{
			// note that the handler can remove itself while the event
			if(TVPContinuousEventVector[i])
				TVPContinuousEventVector[i]->OnContinuousCallback(tick);
			else
				emptyflag = true;

			if(TVPExclusiveEventPosted) return;  // check exclusive events
		}

		if(emptyflag)
		{
			// the array has empty cell

			// eliminate empty
            std::vector<tTVPContinuousEventCallbackIntf *>::iterator i;
			for(i = TVPContinuousEventVector.begin();
				i !=TVPContinuousEventVector.end();)
			{
				if(*i == NULL)
					i = TVPContinuousEventVector.erase(i);
				else
					i++;
			}
		}
	}

	if(!TVPEventDisabled && TVPContinuousHandlerVector.size())
	{
		bool emptyflag = false;
		tTJSVariant vtick((tjs_int64)tick);
		tTJSVariant *pvtick = &vtick;
		for(tjs_uint i = 0; i < TVPContinuousHandlerVector.size(); i++)
		{
			if(TVPContinuousHandlerVector[i].Object)
			{
				tjs_error er;
				try
				{
					er =
						TVPContinuousHandlerVector[i].FuncCall(0, NULL, NULL, NULL, 1, &pvtick, NULL);
				}
				catch(...)
				{
					// failed
					TVPContinuousHandlerVector[i].Release();
					TVPContinuousHandlerVector[i].Object =
					TVPContinuousHandlerVector[i].ObjThis = NULL;
					throw;
				}
				if(TJS_FAILED(er))
				{
					// failed
					TVPContinuousHandlerVector[i].Release();
					TVPContinuousHandlerVector[i].Object =
					TVPContinuousHandlerVector[i].ObjThis = NULL;
					emptyflag = true;
				}
				if(TVPExclusiveEventPosted) return;  // check exclusive events
			}
			else
			{
				emptyflag = true;
			}

		}

		if(emptyflag)
		{
			// the array has empty cell

			// eliminate empty
            std::vector<tTJSVariantClosure>::iterator i;
			for(i = TVPContinuousHandlerVector.begin();
				i !=TVPContinuousHandlerVector.end();)
			{
				if(!i->Object)
				{
					i->Release();
					i = TVPContinuousHandlerVector.erase(i);
				}
				else
				{
					i++;
				}
			}
		}
	}

	if(!TVPContinuousEventVector.size() && !TVPContinuousHandlerVector.size())
		TVPEndContinuousEvent();
}
//---------------------------------------------------------------------------
void TVPDeliverContinuousEvent()
{
	if(TVPContinuousEventProcessing) return;
	TVPContinuousEventProcessing = true;
	try
	{
		try
		{
			try
			{
				_TVPDeliverContinuousEvent();
			}
			catch(...)
			{
				TVPContinuousEventProcessing = false;
				throw;
			}
		}
		TJS_CONVERT_TO_TJS_EXCEPTION
	}
	TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION(TJS_W("continuous event"));

	TVPContinuousEventProcessing = false;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TVPAddContinuousHandler(tTJSVariantClosure clo)
{
	std::vector<tTJSVariantClosure>::iterator i;
	i = std::find(TVPContinuousHandlerVector.begin(),
		TVPContinuousHandlerVector.end(), clo);
	if(i == TVPContinuousHandlerVector.end())
	{
		TVPBeginContinuousEvent();
		clo.AddRef();
		TVPContinuousHandlerVector.emplace_back(clo);
	}
}
//---------------------------------------------------------------------------
void TVPRemoveContinuousHandler(tTJSVariantClosure clo)
{
	std::vector<tTJSVariantClosure>::iterator i;
	i = std::find(TVPContinuousHandlerVector.begin(),
		TVPContinuousHandlerVector.end(), clo);
	if(i != TVPContinuousHandlerVector.end())
	{
		i->Release();
		i->Object = i->ObjThis = NULL;
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// "Compact" Event Delivering related
//---------------------------------------------------------------------------
// Compact events are to be delivered when:
// 1. the application is in idle state for long duration
// 2. the application had been deactivated ( application has lost the focus )
// 3. the application had been minimized
// these are to reduce memory usage, like garbage collection, cache cleaning,
// or etc ...
//---------------------------------------------------------------------------
static std::vector<tTVPCompactEventCallbackIntf *> TVPCompactEventVector;
bool TVPEnableGlobalHeapCompaction = false;
//---------------------------------------------------------------------------
void TVPAddCompactEventHook(tTVPCompactEventCallbackIntf *cb)
{
	TVPCompactEventVector.push_back(cb);
}
//---------------------------------------------------------------------------
void TVPRemoveCompactEventHook(tTVPCompactEventCallbackIntf *cb)
{
	std::vector<tTVPCompactEventCallbackIntf *>::iterator i;
	for(i = TVPCompactEventVector.begin();
		i !=TVPCompactEventVector.end();)
	{
		if(cb == *i) *i = NULL; // simply assign a null
		i++;
	}
}
//---------------------------------------------------------------------------
extern void TVPDoSaveSystemVariables();
void TVPDeliverCompactEvent(tjs_int level)
{
	// must be called by each platforms's implementation
	//std::vector<tTVPCompactEventCallbackIntf *>::iterator i;
	if(TVPCompactEventVector.size())
	{
		bool emptyflag = false;
		for(tjs_uint i = 0; i < TVPCompactEventVector.size(); i ++)
		{
			// note that the handler can remove itself while the event
			try
			{
				try
				{
					if(TVPCompactEventVector[i])
						TVPCompactEventVector[i]->OnCompact(level); else emptyflag = true;
				}
				TJS_CONVERT_TO_TJS_EXCEPTION
			}
			TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION_FORCE_SHOW_EXCEPTION(TJS_W("Compact Event"));
		}

		if(emptyflag)
		{
			// the array has empty cell

			// eliminate empty
			std::vector<tTVPCompactEventCallbackIntf *>::iterator i;
			for(i = TVPCompactEventVector.begin();
				i !=TVPCompactEventVector.end();)
			{
				if(*i == NULL)
					i = TVPCompactEventVector.erase(i);
				else
					i++;
			}
		}
	}
	TVPDoSaveSystemVariables();
#if 0
	if( level >= TVP_COMPACT_LEVEL_MAX && TVPEnableGlobalHeapCompaction )
	{	// Do compact CRT and Global Heap
		HANDLE hHeap = ::GetProcessHeap();
		if( hHeap ) {
			::HeapCompact( hHeap, 0 );
		}
		HANDLE hCrtHeap = (HANDLE)_get_heap_handle();
		if( hCrtHeap && hCrtHeap != hHeap ) {
			::HeapCompact( hCrtHeap, 0 );
		}
	}
#endif
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// AsyncTrigger related
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNI_AsyncTrigger
//---------------------------------------------------------------------------
tTJSNI_AsyncTrigger::tTJSNI_AsyncTrigger()
{
	Owner = NULL;
	Cached = true;
	IdlePendingCount = 0;
	Mode = atmNormal;
	ActionOwner.Object = ActionOwner.ObjThis = NULL;
	ActionName = TVPActionName;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
		tTJSNI_AsyncTrigger::Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;

	if(numparams >= 2 && param[1]->Type() != tvtVoid)
		ActionName = *param[1]; // action function to be called

	ActionOwner = param[0]->AsObjectClosure();
	Owner = tjs_obj;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_AsyncTrigger::Invalidate()
{
	TVPCancelSourceEvents(Owner);
	Owner = NULL;

	ActionOwner.Release();
	ActionOwner.ObjThis = ActionOwner.Object = NULL;

	inherited::Invalidate();
}
//---------------------------------------------------------------------------
void tTJSNI_AsyncTrigger::Trigger()
{
	// trigger event
	if(Owner)
	{
		if(Cached)
		{
			// remove undelivered events from queue when "Cached" flag is set
			TVPCancelSourceEvents(Owner);
		}
		static ttstr eventname(TJS_W("onFire"));

		tjs_uint32 flags = TVP_EPT_POST;
		if(Mode == atmExclusive) flags |= TVP_EPT_EXCLUSIVE;  // fire exclusive event
		if(Mode == atmAtIdle)    flags |= TVP_EPT_IDLE;       // fire idle event

		TVPPostEvent(Owner, Owner, eventname, 0, flags, 0, NULL);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_AsyncTrigger::Cancel()
{
	// cancel event
	if(Owner) TVPCancelSourceEvents(Owner);
	IdlePendingCount = 0;
}
//---------------------------------------------------------------------------
void tTJSNI_AsyncTrigger::SetCached(bool b)
{
	// set cached operation flag.
	// when this flag is set, only one event is delivered at once.
	if(Cached != b)
	{
		Cached = b;
		Cancel(); // all events are canceled
	}
}
//---------------------------------------------------------------------------
void tTJSNI_AsyncTrigger::SetMode(tTVPAsyncTriggerMode m)
{
	if(Mode != m)
	{
		Mode = m;
		Cancel();
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNC_AsyncTrigger
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_AsyncTrigger::ClassID = -1;
tTJSNC_AsyncTrigger::tTJSNC_AsyncTrigger() : inherited(TJS_W("AsyncTrigger"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(AsyncTrigger) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_AsyncTrigger,
	/*TJS class name*/AsyncTrigger)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/AsyncTrigger)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/trigger)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_AsyncTrigger);
	_this->Trigger();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/trigger)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/cancel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_AsyncTrigger);
	_this->Cancel();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/cancel)
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onFire)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_AsyncTrigger);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		ttstr & actionname = _this->GetActionName();
		TVP_ACTION_INVOKE_BEGIN(0, "onFire", objthis);
		TVP_ACTION_INVOKE_END_NAME(obj,
			actionname.IsEmpty() ? NULL :actionname.c_str(),
			actionname.IsEmpty() ? NULL :actionname.GetHint());
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onFire)
//----------------------------------------------------------------------

//--properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(cached)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_AsyncTrigger);
		*result = _this->GetCached();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_AsyncTrigger);
		_this->SetCached(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(cached)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(mode)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_AsyncTrigger);
		*result = (tjs_int)_this->GetMode();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_AsyncTrigger);
		_this->SetMode((tTVPAsyncTriggerMode)(tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(mode)
//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS

}
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_AsyncTrigger::CreateNativeInstance()
{
	return new tTJSNI_AsyncTrigger();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_AsyncTrigger()
{
	return new tTJSNC_AsyncTrigger();
}
//---------------------------------------------------------------------------
