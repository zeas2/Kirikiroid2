//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Sound Buffer Base interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "SoundBufferBaseIntf.h"
#include "MsgIntf.h"
#include "EventIntf.h"


//---------------------------------------------------------------------------
// tTJSNI_SoundBuffer
//---------------------------------------------------------------------------
tTJSNI_BaseSoundBuffer::tTJSNI_BaseSoundBuffer()
{
	InFading = false;
	CanDeliverEvents = true;
	Owner = NULL;
	ActionOwner.Object = ActionOwner.ObjThis = NULL;
	Status = ssUnload;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSNI_BaseSoundBuffer::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;

	ActionOwner = param[0]->AsObjectClosure();
	Owner = tjs_obj;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_BaseSoundBuffer::Invalidate()
{
	CanDeliverEvents = false;
	TVPCancelSourceEvents(Owner);
	Owner = NULL;

	ActionOwner.Release();
	ActionOwner.ObjThis = ActionOwner.Object = NULL;

	inherited::Invalidate();
}
//---------------------------------------------------------------------------
ttstr tTJSNI_BaseSoundBuffer::GetStatusString() const
{
	static ttstr unload(TJS_W("unload"));
	static ttstr play(TJS_W("play"));
	static ttstr stop(TJS_W("stop"));
	static ttstr unknown(TJS_W("unknown"));

	switch(Status)
	{
	case ssUnload:	return unload;
	case ssPlay:	return play;
	case ssStop:	return stop;
	default:		return unknown;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseSoundBuffer::SetStatus(tTVPSoundStatus s)
{
	if(Status != s)
	{
		Status = s;

		if(Owner)
		{
			// Cancel Previous un-delivered Events
			TVPCancelSourceEvents(Owner);

			// fire
			if(CanDeliverEvents)
			{
				// fire onStatusChanged event
				tTJSVariant param(GetStatusString());
				static ttstr eventname(TJS_W("onStatusChanged"));
				TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE,
					1, &param);
			}
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseSoundBuffer::SetStatusAsync(tTVPSoundStatus s)
{
	// asynchronous version of SetStatus
	// the event may not be delivered immediately.

	if(Status != s)
	{
		Status = s;

		if(CanDeliverEvents)
		{
			// fire onStatusChanged event
			if(Owner)
			{
				tTJSVariant param(GetStatusString());
				static ttstr eventname(TJS_W("onStatusChanged"));
				TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_POST,
					1, &param);
			}
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseSoundBuffer::TimerBeatHandler()
{
	// fade handling

	if(!Owner) return;   // "Owner" indicates the owner object is valid

	if(!InFading) return;

	if(BlankLeft)
	{
		BlankLeft -= TVP_SB_BEAT_INTERVAL;
		if(BlankLeft < 0) BlankLeft = 0;
	}
	else if(FadeCount)
	{
		if(FadeCount == 1)
		{
			StopFade(true, true);
		}
		else
		{
			FadeCount--;
			tjs_int v;
			v = GetVolume();
			v += DeltaVolume;
			if(v<0) v = 0;
			if(v>100000) v = 100000;
			SetVolume(v);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseSoundBuffer::Fade(tjs_int to, tjs_int time, tjs_int blanktime)
{
	// start fading

	if(!Owner) return;

	if(time<=0 || blanktime <0)
	{
		TVPThrowExceptionMessage(TVPInvalidParam);
	}

	// stop current fade
	if(InFading) StopFade(false, false);

	// set some parameters
	DeltaVolume = (to - GetVolume()) * TVP_SB_BEAT_INTERVAL / time;
	TargetVolume = to;
	FadeCount = time / TVP_SB_BEAT_INTERVAL;
	BlankLeft = blanktime;
	InFading = true;
	if(FadeCount == 0 && BlankLeft == 0) StopFade(false, true);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseSoundBuffer::StopFade(bool async, bool settargetvol)
{
	// stop fading

	if(!Owner) return;

	if(InFading)
	{
		InFading = false;

		if(settargetvol) SetVolume(TargetVolume);

		// post "onFadeCompleted" event to the owner
		if(CanDeliverEvents)
		{
			static ttstr eventname(TJS_W("onFadeCompleted"));
			TVPPostEvent(Owner, Owner, eventname, 0,
				async?TVP_EPT_POST:TVP_EPT_IMMEDIATE, 0, NULL);
		}
	}
}
//---------------------------------------------------------------------------




