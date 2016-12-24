//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// MIDI sequencer implementation
//---------------------------------------------------------------------------

#ifndef MIDIImplH
#define MIDIImplH

#include "MIDIIntf.h"
//#include "Messages.hpp"


//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(void, TVPMIDIOutData, (const tjs_uint8 *data, int len));
	/* output MIDI data (can be multiple data at once) */
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNI_MIDISoundBuffer : MIDI Native Instance
//---------------------------------------------------------------------------
class tTVPSMFTrack;
class tTJSNI_MIDISoundBuffer : public tTJSNI_BaseMIDISoundBuffer
{
	friend class tTVPSMFTrack;
	typedef  tTJSNI_BaseMIDISoundBuffer inherited;

public:
	tTJSNI_MIDISoundBuffer();
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

#ifdef TVP_ENABLE_MIDI
private:
	std::vector<tTVPSMFTrack *> Tracks;
	tjs_int Division;

	tjs_int64 Position;

	tjs_int64 TickCount;  // TickCount << 16

	tjs_int64 TickCountDelta; //

	bool UsingChannel[16]; // using channel is true
	tjs_uint32 UsingNote[16][4]; // using notes

	int Volumes[16]; // track volumes
	tjs_int Volume;
	tjs_int Volume2;
	tjs_int BufferVolume;

	bool Looping;

	bool Playing;
	bool Loaded;

	bool NextSetVolume;

	tjs_uint64 LastTickTime; // tick count of last OnTimer()

	HWND UtilWindow; // a dummy window for receiving status from playing thread
	void WndProc(Messages::TMessage &Msg); // its window procedure

private:
	void AllNoteOff();

	void SetTempo(tjs_uint tempo);

public:
	bool OnTimer();
	void Open(const ttstr &name);

private:
	bool StopPlay();
	bool StartPlay();

public:
	void Play();
	void Stop();

private:
	void SetBufferVolume(tjs_int nv);

public:
	tjs_int GetVolume() const { return Volume; } // GetVolume override
	void SetVolume(tjs_int v); // SetVolume override

	tjs_int GetVolume2() const { return Volume2; }
	void SetVolume2(tjs_int v);

	bool GetLooping() const { return Looping; }
	void SetLooping(bool b) { Looping = b; }

protected:
#else
    void SetVolume(tjs_int v){}
    tjs_int GetVolume() const { return 0; }
#endif
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#endif
