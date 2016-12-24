//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Sound Buffer Base interface
//---------------------------------------------------------------------------
#ifndef SoundBufferBaseIntfH
#define SoundBufferBaseIntfH

#include "tjsNative.h"


//---------------------------------------------------------------------------
// tTVPSoundStatus
//---------------------------------------------------------------------------
enum tTVPSoundStatus
{
	ssUnload, // data is not specified
	ssStop, // stop
	ssPlay, // play
	ssPause, // pause
	ssReady, // ready
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNI_BaseSoundBuffer
//---------------------------------------------------------------------------
class tTJSNI_BaseSoundBuffer : public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;
	friend class tTVPSoundBufferTimerDispatcher;

public:
	tTJSNI_BaseSoundBuffer();
	tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

protected:
	iTJSDispatch2 *Owner; // owner object
	tTJSVariantClosure ActionOwner; // object to send action
	tTVPSoundStatus Status; // status

	// volume functions ( implement this in child classes )
	// tTJSNI_BaseSoundBuffer/tTJSNI_SoundBuffer manage this when fading the
	// volume.
	virtual void SetVolume(tjs_int i) = 0;
	virtual tjs_int GetVolume() const = 0;

public:
	ttstr GetStatusString() const;

protected:
	bool CanDeliverEvents;
	void SetStatus(tTVPSoundStatus s);
	void SetStatusAsync(tTVPSoundStatus s);

public:
	tTVPSoundStatus GetStatus() const { return Status; }


public:
	tTJSVariantClosure GetActionOwnerNoAddRef() const { return ActionOwner; }

	//-- fading stuff -----------------------------------------------------
protected:
	virtual void TimerBeatHandler();
		// call this in tTJSNI_SoundBuffer periodically.
		// interval time must be declered as TVP_SB_BEAT_INTERVAL in ms.
		// accuracy is so not be required.

private:
	bool InFading;
	tjs_int TargetVolume; // distination volume
	tjs_int DeltaVolume; // delta volume for each interval
	tjs_int FadeCount; // beat count over fading
	tjs_int BlankLeft; // blank time until fading

public:
	void Fade(tjs_int to, tjs_int time, tjs_int blanktime);
	void StopFade(bool async, bool settargetvol);

};

//---------------------------------------------------------------------------

#include "SoundBufferBaseImpl.h"

//---------------------------------------------------------------------------


#endif
