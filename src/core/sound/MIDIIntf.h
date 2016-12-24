//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// MIDI sequencer interface
//---------------------------------------------------------------------------
#ifndef MIDIIntfH
#define MIDIIntfH


#include "tjsNative.h"
#include "SoundBufferBaseIntf.h"

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNI_BaseMIDISoundBuffer
//---------------------------------------------------------------------------
class tTJSNI_BaseMIDISoundBuffer : public tTJSNI_SoundBuffer
{
	typedef tTJSNI_SoundBuffer inherited;

public:
	tTJSNI_BaseMIDISoundBuffer();
	tjs_error TJS_INTF_METHOD
	Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

protected:
public:
};
//---------------------------------------------------------------------------

#include "MIDIImpl.h" // must define tTJSNI_MIDISoundBuffer class

//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSNC_MIDISoundBuffer : TJS MIDISoundBuffer class
//---------------------------------------------------------------------------
class tTJSNC_MIDISoundBuffer : public tTJSNativeClass
{
public:
	tTJSNC_MIDISoundBuffer();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_MIDISoundBuffer();
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
#endif
