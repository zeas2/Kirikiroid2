//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Video Overlay support interface
//---------------------------------------------------------------------------
#ifndef VideoOvlIntfH
#define VideoOvlIntfH

#include "tjsNative.h"
#include "SoundBufferBaseIntf.h"
#include "voMode.h"
#include <functional>

/*[*/
//---------------------------------------------------------------------------
// tTVPPeriodEventType : event type in onPeriod event
//---------------------------------------------------------------------------
enum tTVPPeriodEventReason
{
	perLoop, // the event is by loop rewind
	perPeriod, // the event is by period point specified by the user
	perPrepare, // the event is by prepare() method
	perSegLoop, // the event is by segment loop rewind
};



/*]*/


//---------------------------------------------------------------------------
// tTJSNI_BaseVideoOverlay
//---------------------------------------------------------------------------
class tTJSNI_Window;
class tTJSNI_BaseVideoOverlay : public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;

public:
	tTJSNI_BaseVideoOverlay();
	tjs_error TJS_INTF_METHOD
	Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

protected:
	iTJSDispatch2 *Owner;
	bool CanDeliverEvents;
	tTJSNI_Window * Window;
	TJS::tTJSVariantClosure ActionOwner;
	tTVPSoundStatus Status; // status

	ttstr GetStatusString() const;
	void SetStatus(tTVPSoundStatus s);
	void SetStatusAsync(tTVPSoundStatus s);
	void FireCallbackCommand(const ttstr & command, const ttstr & argument);
	void FirePeriodEvent(tTVPPeriodEventReason reason);
	void FireFrameUpdateEvent( tjs_int frame );


public:
	virtual void Disconnect() = 0; // called from Window object's invalidation
	virtual bool GetVisible() const = 0;
	virtual const tTVPRect &GetBounds() const = 0;
	virtual tTVPVideoOverlayMode GetMode() const = 0;
	virtual bool GetVideoSize(tjs_int &w, tjs_int &h) const = 0;

	tTJSVariantClosure GetActionOwnerNoAddRef() const { return ActionOwner; }
};
//---------------------------------------------------------------------------

#include "VideoOvlImpl.h" // must define tTJSNI_VideoOverlay class

//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSNC_VideoOverlay : TJS VideoOverlay class
//---------------------------------------------------------------------------
class tTJSNC_VideoOverlay : public tTJSNativeClass
{
public:
	tTJSNC_VideoOverlay();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_VideoOverlay();
//---------------------------------------------------------------------------
#endif
