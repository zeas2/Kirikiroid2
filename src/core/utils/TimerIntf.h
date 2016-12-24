//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Timer Object Interface
//---------------------------------------------------------------------------
#ifndef TimerIntfH
#define TimerIntfH

#include "tjsNative.h"
#include "EventIntf.h"


// the timer has sub-milliseconds precision by fixed-point real.
#define TVP_SUBMILLI_FRAC_BITS 16


//---------------------------------------------------------------------------
// global variables
//---------------------------------------------------------------------------
extern bool TVPLimitTimerCapacity;
	// limit timer capacity to one
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNI_BaseTimer
//---------------------------------------------------------------------------
class tTJSNI_BaseTimer : public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;

protected:
	iTJSDispatch2 *Owner;
	tTJSVariantClosure ActionOwner; // object to send action
	tjs_uint16 Counter; // serial number for event tag
	tjs_int Capacity; // max queue size for this timer object
	ttstr ActionName;
	tTVPAsyncTriggerMode Mode; // trigger mode

public:
	tTJSNI_BaseTimer();
	tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

protected:
	void Fire(tjs_uint n);
	void CancelEvents();
	bool AreEventsInQueue();

public:
	tTJSVariantClosure GetActionOwnerNoAddRef() const { return ActionOwner; }
	ttstr & GetActionName() { return ActionName; }

	tjs_int GetCapacity() const { return Capacity; }
	void SetCapacity(tjs_int c) { Capacity = c; }

	tTVPAsyncTriggerMode GetMode() const { return Mode; }
	void SetMode(tTVPAsyncTriggerMode mode) { Mode = mode; }
};
//---------------------------------------------------------------------------

#include "TimerImpl.h" // must define tTJSNI_Timer class

//---------------------------------------------------------------------------
// tTJSNC_Timer : TJS Timer class
//---------------------------------------------------------------------------
class tTJSNC_Timer : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;

public:
	tTJSNC_Timer();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
	/*
		implement this in each platform.
		this must return a proper instance of tTJSNI_Timer.
	*/
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_Timer();
	/*
		implement this in each platform.
		this must return a proper instance of tTJSNI_Timer.
		usually simple returns: new tTJSNC_Timer();
	*/
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
#endif
