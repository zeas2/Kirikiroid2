//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "System" class interface
//---------------------------------------------------------------------------
#ifndef SystemIntfH
#define SystemIntfH
#include "tjsNative.h"

//---------------------------------------------------------------------------
// tTJSNC_System : TJS System class
//---------------------------------------------------------------------------
class tTJSNC_System : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;

public:
	tTJSNC_System();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_System();
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(ttstr, TVPGetPlatformName, ());
		// retrieve platform name (eg. "Win32")
		// implement in each platform.
TJS_EXP_FUNC_DEF(ttstr, TVPGetOSName, ());
		// retrieve OS name
		// implement in each platform.
extern void TVPFireOnApplicationActivateEvent(bool activate_or_deactivate);
extern tjs_int TVPGetOSBits();
//---------------------------------------------------------------------------
#endif
