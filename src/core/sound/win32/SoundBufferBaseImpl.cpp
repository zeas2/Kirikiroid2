//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Sound Buffer Base implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <algorithm>
#include "SoundBufferBaseImpl.h"
#include "WaveImpl.h"

#include "TVPTimer.h"

//---------------------------------------------------------------------------
// Sound Buffer Timer Dispatcher ( for fading or commiting defered settings )
//---------------------------------------------------------------------------
static class TVPTimer * TVPSoundBufferTimer = NULL;
static std::vector<tTJSNI_SoundBuffer *> TVPSoundBufferVector;
//---------------------------------------------------------------------------
class tTVPSoundBufferTimerDispatcher
{
public:
	void Handler()
	{
		std::vector<tTJSNI_SoundBuffer *>::iterator i;
		for(i = TVPSoundBufferVector.begin(); i != TVPSoundBufferVector.end();
			i++)
		{
			(*i)->TimerBeatHandler();
		}
		TVPWaveSoundBufferCommitSettings();
	}
} static TVPSoundBufferTimerDispatcher;
//---------------------------------------------------------------------------
void TVPAddSoundBuffer(tTJSNI_SoundBuffer * buf)
{
	if(TVPSoundBufferVector.size() == 0)
	{
		// first buffer
		TVPSoundBufferTimer = new TVPTimer(); // Create Timer Object
		TVPSoundBufferTimer->SetInterval( TVP_SB_BEAT_INTERVAL );
		TVPSoundBufferTimer->SetOnTimerHandler( &TVPSoundBufferTimerDispatcher, &tTVPSoundBufferTimerDispatcher::Handler );
	}

	TVPSoundBufferVector.push_back(buf);
}
//---------------------------------------------------------------------------
void TVPRemoveSoundBuffer(tTJSNI_SoundBuffer *buf)
{
	if(TVPSoundBufferVector.size() != 0)
	{
		std::vector<tTJSNI_SoundBuffer *>::iterator i;
		i = std::find(TVPSoundBufferVector.begin(), TVPSoundBufferVector.end(),
			buf);
		if(i != TVPSoundBufferVector.end())
		{
			TVPSoundBufferVector.erase(i);
		}
	}

	if(TVPSoundBufferVector.size() == 0)
	{
		// all buffer was removed
		delete TVPSoundBufferTimer;
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNI_SoundBuffer
//---------------------------------------------------------------------------
tTJSNI_SoundBuffer::tTJSNI_SoundBuffer()
{
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSNI_SoundBuffer::Construct(tjs_int numparams,
	tTJSVariant **param, iTJSDispatch2 *tjs_obj)
{
	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;

	TVPAddSoundBuffer(this);

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_SoundBuffer::Invalidate()
{
	TVPRemoveSoundBuffer(this);

	inherited::Invalidate();
}
//---------------------------------------------------------------------------

