//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// MIDI sequencer interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "MIDIIntf.h"
#include "EventIntf.h"

//---------------------------------------------------------------------------
// tTJSNI_BaseMIDISoundBuffer
//---------------------------------------------------------------------------
tTJSNI_BaseMIDISoundBuffer::tTJSNI_BaseMIDISoundBuffer()
{
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSNI_BaseMIDISoundBuffer::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_BaseMIDISoundBuffer::Invalidate()
{

	inherited::Invalidate();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_MIDISoundBuffer : TJS MIDISoundBuffer class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_MIDISoundBuffer::ClassID = -1;
tTJSNC_MIDISoundBuffer::tTJSNC_MIDISoundBuffer()  :
	tTJSNativeClass(TJS_W("MIDISoundBuffer"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(MIDISoundBuffer) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this,
	/*var.type*/tTJSNI_MIDISoundBuffer,
	/*TJS class name*/MIDISoundBuffer)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/MIDISoundBuffer)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/open)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MIDISoundBuffer);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
#ifdef TVP_ENABLE_MIDI
    _this->Open(*param[0]);
#endif

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/open)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/play)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MIDISoundBuffer);

#ifdef TVP_ENABLE_MIDI
    _this->Play();
#endif

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/play)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stop)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MIDISoundBuffer);

#ifdef TVP_ENABLE_MIDI
    _this->Stop();
#endif

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fade)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MIDISoundBuffer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
#ifdef TVP_ENABLE_MIDI

	tjs_int to;
	tjs_int time;
	tjs_int delay = 0;
	to = (tjs_int)(*param[0]);
	time = (tjs_int)(*param[1]);
	if(numparams >= 3 && param[2]->Type() != tvtVoid)
		delay = (tjs_int)(*param[2]);

	_this->Fade(to, time, delay);
#endif

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fade)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stopFade)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MIDISoundBuffer);

#ifdef TVP_ENABLE_MIDI
    _this->StopFade(false, true);
#endif

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stopFade)
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onStatusChanged)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MIDISoundBuffer);

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
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onFadeCompleted)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MIDISoundBuffer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onFadeCompleted", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onFadeCompleted)
//----------------------------------------------------------------------

//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(position)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		// not yet implemented

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		// not yet implemented

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(position)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(paused)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		// not yet implemented

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		// not yet implemented

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(paused)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(totalTime)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		// not yet implemented

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		// not yet implemented

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(totalTime)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(looping)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

#ifdef TVP_ENABLE_MIDI
        *result = _this->GetLooping();
#endif
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

#ifdef TVP_ENABLE_MIDI
        _this->SetLooping(*param);
#endif

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(looping)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(volume)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		*result = _this->GetVolume();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		_this->SetVolume(*param);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(volume)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(volume2)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

#ifdef TVP_ENABLE_MIDI
        *result = _this->GetVolume2();
#else
        *result = 0;
#endif

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

#ifdef TVP_ENABLE_MIDI
        _this->SetVolume2(*param);
#endif

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(volume2)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(status)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MIDISoundBuffer);

		*result = _this->GetStatusString();

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(status)
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS

//----------------------------------------------------------------------


}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------


