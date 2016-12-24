//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Sound Buffer Base implementation
//---------------------------------------------------------------------------

#ifndef SoundBufferBaseImplH
#define SoundBufferBaseImplH

#define TVP_SB_BEAT_INTERVAL 60

#include "SoundBufferBaseIntf.h"


//---------------------------------------------------------------------------
class tTJSNI_SoundBuffer : public tTJSNI_BaseSoundBuffer
{
	typedef tTJSNI_BaseSoundBuffer inherited;

public:
	tTJSNI_SoundBuffer();
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

public:

protected:
};
//---------------------------------------------------------------------------
#endif
