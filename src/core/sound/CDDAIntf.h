//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// CD-DA access interface
//---------------------------------------------------------------------------
#ifndef CDDAIntfH
#define CDDAIntfH

#include "tjsNative.h"
#include "SoundBufferBaseIntf.h"

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNI_BaseCDDASoundBuffer
//---------------------------------------------------------------------------
class tTJSNI_BaseCDDASoundBuffer : public tTJSNI_SoundBuffer
{
	typedef tTJSNI_SoundBuffer inherited;

public:
	tTJSNI_BaseCDDASoundBuffer();
	tjs_error TJS_INTF_METHOD
	Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

protected:
public:
};
//---------------------------------------------------------------------------

#include "CDDAImpl.h" // must define tTJSNI_CDDASoundBuffer class

//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSNC_CDDASoundBuffer : TJS CDDASoundBuffer class
//---------------------------------------------------------------------------
class tTJSNC_CDDASoundBuffer : public tTJSNativeClass
{
public:
	tTJSNC_CDDASoundBuffer();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_CDDASoundBuffer();
//---------------------------------------------------------------------------

#endif
