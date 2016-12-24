//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Video Overlay support interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "MsgIntf.h"
#include "WindowIntf.h"
#include "VideoOvlIntf.h"
#include "LayerIntf.h"

//---------------------------------------------------------------------------
// tTJSNI_BaseVideoOverlay
//---------------------------------------------------------------------------
tTJSNI_BaseVideoOverlay::tTJSNI_BaseVideoOverlay()
{
	ActionOwner.Object = ActionOwner.ObjThis = NULL;
	Status = ssUnload;
	Owner = NULL;
	CanDeliverEvents = true;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSNI_BaseVideoOverlay::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object == NULL) TVPThrowExceptionMessage(TVPSpecifyWindow);
	tTJSNI_Window *win = NULL;
	if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
		tTJSNC_Window::ClassID, (iTJSNativeInstance**)&win)))
		TVPThrowExceptionMessage(TVPSpecifyWindow);
	if(!win) TVPThrowExceptionMessage(TVPSpecifyWindow);
	Window = win;

	Window->RegisterVideoOverlayObject(this);

	ActionOwner = param[0]->AsObjectClosure();
	Owner = tjs_obj;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_BaseVideoOverlay::Invalidate()
{
	Owner = NULL;
	if(Window) Window->UnregisterVideoOverlayObject(this);
	ActionOwner.Release();

	inherited::Invalidate();
}
//---------------------------------------------------------------------------
ttstr tTJSNI_BaseVideoOverlay::GetStatusString() const
{
	static ttstr unload(TJS_W("unload"));
	static ttstr play(TJS_W("play"));
	static ttstr stop(TJS_W("stop"));
	static ttstr unknown(TJS_W("unknown"));
	static ttstr pause(TJS_W("pause"));
	static ttstr ready(TJS_W("ready"));

	switch(Status)
	{
	case ssUnload:	return unload;
	case ssPlay:	return play;
	case ssStop:	return stop;
	case ssPause:	return pause;
	case ssReady:	return ready;
	default:		return unknown;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseVideoOverlay::SetStatus(tTVPSoundStatus s)
{
	// this function may call the onStatusChanged event immediately
	
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
void tTJSNI_BaseVideoOverlay::SetStatusAsync(tTVPSoundStatus s)
{
	// this function posts the onStatusChanged event

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
				TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_POST,
					1, &param);
			}
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseVideoOverlay::FireCallbackCommand(const ttstr & command,
	const ttstr & argument)
{
	// fire call back command event.
	// this is always synchronized event.
	if(Owner)
	{
		// fire
		if(CanDeliverEvents)
		{
			// fire onStatusChanged event
			tTJSVariant param[2] = {command, argument};
			static ttstr eventname(TJS_W("onCallbackCommand"));
			TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE,
				2, param);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseVideoOverlay::FirePeriodEvent(tTVPPeriodEventReason reason)
{
	// fire onPeriod event
	// this is always synchronized event.
	
	if(Owner)
	{
		// fire
		if(CanDeliverEvents)
		{
			// fire onPeriod event
			tTJSVariant param[1] = {(tjs_int)reason};
			static ttstr eventname(TJS_W("onPeriod"));
			TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE,
				1, param);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseVideoOverlay::FireFrameUpdateEvent( tjs_int frame )
{
	// fire onFrameUpdate event
	// this is always synchronized event.
	
	if(Owner)
	{
		// fire
		if(CanDeliverEvents)
		{
			// fire onPeriod event
			tTJSVariant param[1] = {frame};
			static ttstr eventname(TJS_W("onFrameUpdate"));
			TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE,
				1, param);
		}
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNC_VideoOverlay : TJS VideoOverlay class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_VideoOverlay::ClassID = -1;
tTJSNC_VideoOverlay::tTJSNC_VideoOverlay()  :
	tTJSNativeClass(TJS_W("VideoOverlay"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(VideoOverlay) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this,
	/*var.type*/tTJSNI_VideoOverlay,
	/*TJS class name*/VideoOverlay)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/VideoOverlay)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/open)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	_this->Open(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/open)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/play)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->Play();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/play)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stop)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->Stop();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/close)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->Close();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/close)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setPos) // not SetPosition
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->SetPosition(*param[0], *param[1]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setPos)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setSize)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->SetSize(*param[0], *param[1]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setBounds)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	if(numparams < 4) return TJS_E_BADPARAMCOUNT;
	tTVPRect r;
	r.left = *param[0];
	r.top = *param[1];
	r.right = r.left + (tjs_int)*param[2];
	r.bottom = r.top + (tjs_int)*param[3];
	_this->SetBounds(r);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setBounds)
//----------------------------------------------------------------------
// Start: Add:	2004/08/23	T.Imoto
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/pause)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->Pause();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/pause)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/rewind)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->Rewind();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/rewind)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/prepare)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->Prepare();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/prepare)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setSegmentLoop)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->SetSegmentLoop(*param[0],*param[1]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setSegmentLoop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/cancelSegmentLoop)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->CancelSegmentLoop();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/cancelSegmentLoop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setPeriodEvent)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	if( numparams < 1 )
		_this->SetPeriodEvent( -1 );
	else if( numparams < 2 )
		_this->SetPeriodEvent( *param[0] );

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setPeriodEvent)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/cancelPeriodEvent)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->SetPeriodEvent( -1 );

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/cancelPeriodEvent)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/selectAudioStream)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	if( numparams < 1 ) return TJS_E_BADPARAMCOUNT;

	_this->SelectAudioStream( (tjs_uint)(tjs_int)*param[0] );

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/selectAudioStream)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setMixingLayer)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	if( numparams < 1 ) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer *src = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
		if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	_this->SetMixingLayer(src);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setMixingLayer)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/resetMixingLayer)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	_this->ResetMixingBitmap();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/resetMixingLayer)
//----------------------------------------------------------------------
// End: Add:	2004/08/23	T.Imoto
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onStatusChanged)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onStatusChanged", objthis);
		TVP_ACTION_INVOKE_MEMBER("status");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onStatusChanged)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onCallbackCommand)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(2, "onCallbackCommand", objthis);
		TVP_ACTION_INVOKE_MEMBER("command");
		TVP_ACTION_INVOKE_MEMBER("arg");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onCallbackCommand)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onPeriod)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onPeriod", objthis);
		TVP_ACTION_INVOKE_MEMBER("reason");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onPeriod)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onFrameUpdate)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_VideoOverlay);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onFrameUpdate", objthis);
		TVP_ACTION_INVOKE_MEMBER("frame");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onFrameUpdate)
//----------------------------------------------------------------------

//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(position)
{
// Start: Add:	T.Imoto
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = (tjs_int64)_this->GetTimePosition();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetTimePosition((tjs_int64)*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
// End: Add:	T.Imoto
}
TJS_END_NATIVE_PROP_DECL(position)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(left)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetLeft();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetLeft(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(left)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(top)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetTop();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetTop(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(top)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(width)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetWidth();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetWidth(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(width)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(height)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetHeight();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetHeight(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(height)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(originalWidth)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetOriginalWidth();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(originalWidth)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(originalHeight)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetOriginalHeight();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(originalHeight)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(visible)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetVisible();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetVisible(param->operator bool());

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(visible)
//----------------------------------------------------------------------
// Start: Add:		T.Imoto
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(loop)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetLoop();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetLoop(param->operator bool());

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(loop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(frame)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetFrame();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetFrame(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(frame)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fps)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetFPS();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fps)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(numberOfFrame)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetNumberOfFrame();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(numberOfFrame)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(totalTime)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		*result = _this->GetTotalTime();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(totalTime)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(layer1)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		tTJSNI_BaseLayer *layer1 = _this->GetLayer1();
		if(layer1)
		{
			iTJSDispatch2 *dsp = layer1->GetOwnerNoAddRef();
			*result = tTJSVariant(dsp, dsp);
		}
		else
		{
			*result = tTJSVariant((iTJSDispatch2 *) NULL);
		}

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		tTJSNI_BaseLayer *src = NULL;
		tTJSVariantClosure clo = param->AsObjectClosureNoAddRef();
		if(clo.Object)
		{
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
				TVPThrowExceptionMessage(TVPSpecifyLayer);

			if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);
		}

		_this->SetLayer1(src);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(layer1)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(layer2)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		tTJSNI_BaseLayer *layer2 = _this->GetLayer2();
		if(layer2)
		{
			iTJSDispatch2 *dsp = layer2->GetOwnerNoAddRef();
			*result = tTJSVariant(dsp, dsp);
		}
		else
		{
			*result = tTJSVariant((iTJSDispatch2 *) NULL);
		}

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		tTJSNI_BaseLayer *src = NULL;
		tTJSVariantClosure clo = param->AsObjectClosureNoAddRef();
		if(clo.Object)
		{
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
				TVPThrowExceptionMessage(TVPSpecifyLayer);

			if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);
		}

		_this->SetLayer2(src);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(layer2)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(mode)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetMode();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetMode((tTVPVideoOverlayMode) (tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(mode)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(playRate)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetPlayRate();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);

		_this->SetPlayRate((tjs_real)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(playRate)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(segmentLoopStartFrame)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetSegmentLoopStartFrame();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(segmentLoopStartFrame)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(segmentLoopEndFrame)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetSegmentLoopEndFrame();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(segmentLoopEndFrame)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(periodEventFrame)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetPeriodEventFrame();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetPeriodEvent((tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(periodEventFrame)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(audioBalance)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetAudioBalance();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetAudioBalance((tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(audioBalance)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(audioVolume)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetAudioVolume();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetAudioVolume((tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(audioVolume)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(numberOfAudioStream)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetNumberOfAudioStream();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(numberOfAudioStream)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(enabledAudioStream)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetEnabledAudioStream();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SelectAudioStream( (tjs_uint)(tjs_int)*param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(enabledAudioStream)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(numberOfVideoStream)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetNumberOfVideoStream();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(numberOfVideoStream)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(enabledVideoStream)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetEnabledVideoStream();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SelectVideoStream( (tjs_uint)(tjs_int)*param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(enabledVideoStream)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(mixingMovieAlpha)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetMixingMovieAlpha();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetMixingMovieAlpha( (tjs_real)*param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(mixingMovieAlpha)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(mixingMovieBGColor)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_int)_this->GetMixingMovieBGColor();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetMixingMovieBGColor( (tjs_uint)(tjs_int)*param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(mixingMovieBGColor)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(contrastRangeMin)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetContrastRangeMin();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(contrastRangeMin)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(contrastRangeMax)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetContrastRangeMax();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(contrastRangeMax)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(contrastDefaultValue)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetContrastDefaultValue();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(contrastDefaultValue)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(contrastStepSize)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetContrastStepSize();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(contrastStepSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(contrast)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetContrast();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetContrast( (tjs_real)*param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(contrast)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(brightnessRangeMin)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetBrightnessRangeMin();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(brightnessRangeMin)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(brightnessRangeMax)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetBrightnessRangeMax();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(brightnessRangeMax)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(brightnessDefaultValue)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetBrightnessDefaultValue();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(brightnessDefaultValue)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(brightnessStepSize)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetBrightnessStepSize();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(brightnessStepSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(brightness)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetBrightness();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetBrightness( (tjs_real)*param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(brightness)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hueRangeMin)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetHueRangeMin();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hueRangeMin)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hueRangeMax)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetHueRangeMax();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hueRangeMax)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hueDefaultValue)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetHueDefaultValue();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hueDefaultValue)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hueStepSize)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetHueStepSize();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hueStepSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hue)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetHue();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetHue( (tjs_real)*param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hue)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(saturationRangeMin)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetSaturationRangeMin();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(saturationRangeMin)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(saturationRangeMax)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetSaturationRangeMax();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(saturationRangeMax)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(saturationDefaultValue)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetSaturationDefaultValue();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(saturationDefaultValue)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(saturationStepSize)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetSaturationStepSize();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(saturationStepSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(saturation)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		*result = (tjs_real)_this->GetSaturation();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_VideoOverlay);
		_this->SetSaturation( (tjs_real)*param );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(saturation)
//----------------------------------------------------------------------
// End: Add:	T.Imoto
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS

//----------------------------------------------------------------------


}

