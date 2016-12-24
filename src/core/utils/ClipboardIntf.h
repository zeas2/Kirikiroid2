//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Clipboard Class interface
//---------------------------------------------------------------------------
#ifndef ClipboardIntfH
#define ClipboardIntfH
#include "tjsNative.h"

/*[*/
//---------------------------------------------------------------------------
// tTVPClipboardFormat
//---------------------------------------------------------------------------
enum tTVPClipboardFormat
{
	cbfText = 1
};
/*]*/


//---------------------------------------------------------------------------
// implement these in each platform
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(bool, TVPClipboardHasFormat, (tTVPClipboardFormat format));
TJS_EXP_FUNC_DEF(void, TVPClipboardSetText, (const ttstr & text));
TJS_EXP_FUNC_DEF(bool, TVPClipboardGetText, (ttstr & text));


//---------------------------------------------------------------------------
// tTJSNI_BaseClipboard
//---------------------------------------------------------------------------
class tTJSNI_BaseClipboard : public tTJSNativeInstance
{
public:
	virtual tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *dsp);
	virtual void TJS_INTF_METHOD
		Invalidate();
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNC_Clipboard : TJS Clipboard Class
//---------------------------------------------------------------------------
class tTJSNC_Clipboard : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;
public:
	tTJSNC_Clipboard();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_Clipboard();
//---------------------------------------------------------------------------
#endif
