//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Timer Object Interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "TimerIntf.h"
#include "EventIntf.h"
#include "SysInitIntf.h"


#define TVP_DEFAULT_TIMER_CAPACITY 6

//---------------------------------------------------------------------------
// global variables
//---------------------------------------------------------------------------
bool TVPLimitTimerCapacity = false;
	// limit timer capacity to one
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNI_BaseTimer
//---------------------------------------------------------------------------
tTJSNI_BaseTimer::tTJSNI_BaseTimer()
{
	Owner = NULL;
	Counter = 0;
	Capacity = TVP_DEFAULT_TIMER_CAPACITY;
	ActionOwner.Object = ActionOwner.ObjThis = NULL;
	ActionName = TVPActionName;
	Mode = atmNormal;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
		tTJSNI_BaseTimer::Construct(tjs_int numparams, tTJSVariant **param,
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
void TJS_INTF_METHOD tTJSNI_BaseTimer::Invalidate()
{
	TVPCancelSourceEvents(Owner);
	Owner = NULL;

	ActionOwner.Release();
	ActionOwner.ObjThis = ActionOwner.Object = NULL;

	inherited::Invalidate();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseTimer::Fire(tjs_uint n)
{
	if(!Owner) return;
	static ttstr eventname(TJS_W("onTimer"));

	tjs_int count = TVPCountEventsInQueue(Owner, Owner, eventname, 0);

	tjs_int cap = TVPLimitTimerCapacity ? 1 : (Capacity == 0 ? 65535 : Capacity);
		// 65536 should be considered as to be no-limit.

	tjs_int more = cap - count;

	if(more > 0)
	{
		if(n > (tjs_uint)more) n = more;
		if(Owner)
		{
			tjs_uint32 tag = 1 + ((tjs_uint32)Counter << 1);
			tjs_uint32 flags = TVP_EPT_POST|TVP_EPT_DISCARDABLE;
			switch(Mode)
			{
			case atmNormal:			flags |= TVP_EPT_NORMAL; break;
			case atmExclusive:		flags |= TVP_EPT_EXCLUSIVE; break;
			case atmAtIdle:			flags |= TVP_EPT_IDLE; break;
			}
			while(n--)
			{
				TVPPostEvent(Owner, Owner, eventname, tag,
					flags, 0, NULL);
			}
		}

		Counter++;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseTimer::CancelEvents()
{
	// cancel all events
	if(Owner)
	{
		static ttstr eventname(TJS_W("onTimer"));
		TVPCancelEvents(Owner, Owner, eventname, 0);
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseTimer::AreEventsInQueue()
{
	// are events in event queue ?

	if(Owner)
	{
		static ttstr eventname(TJS_W("onTimer"));
		return TVPAreEventsInQueue(Owner, Owner, eventname, 0);
	}
	return 0;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_Timer
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Timer::ClassID = -1;
tTJSNC_Timer::tTJSNC_Timer() : inherited(TJS_W("Timer"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(Timer) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_Timer,
	/*TJS class name*/Timer)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Timer)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onTimer)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_Timer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		ttstr & actionname = _this->GetActionName();
		TVP_ACTION_INVOKE_BEGIN(0, "onTimer", objthis);
		TVP_ACTION_INVOKE_END_NAME(obj,
			actionname.IsEmpty() ? NULL :actionname.c_str(),
			actionname.IsEmpty() ? NULL :actionname.GetHint());
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onTimer)
//----------------------------------------------------------------------

//--properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(interval)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Timer);
		/*
			bcc 5.5.1 has an inliner bug which cannot treat 64bit integer return
			value properly in some occasions.
			OK : tjs_uint64 interval = _this->GetInterval(); *result = (tjs_int64)interval;
			NG : *result = (tjs_int64)interval;
		*/
		double interval = _this->GetInterval() * (1.0 / (1<<TVP_SUBMILLI_FRAC_BITS));
		*result = interval;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Timer);
		double interval = (double)*param * (1<<TVP_SUBMILLI_FRAC_BITS);
		_this->SetInterval((tjs_int64)(interval + 0.5));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(interval)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(enabled)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Timer);
		*result = _this->GetEnabled();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Timer);
		_this->SetEnabled(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(enabled)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(capacity)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Timer);
		*result = _this->GetCapacity();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Timer);
		_this->SetCapacity(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(capacity)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(mode)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Timer);
		*result = (tjs_int)_this->GetMode();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Timer);
		_this->SetMode((tTVPAsyncTriggerMode)(tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(mode)
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS


	tTJSVariant val;
	if(TVPGetCommandLine(TJS_W("-laxtimer"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("yes"))
			TVPLimitTimerCapacity = true;
	}

}
//---------------------------------------------------------------------------



