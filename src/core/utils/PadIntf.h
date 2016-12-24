//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Text Editor
//---------------------------------------------------------------------------
#ifndef PadIntfH
#define PadIntfH
#include "tjsNative.h"


//---------------------------------------------------------------------------
// tTJSNI_BasePad
//---------------------------------------------------------------------------
class tTJSNI_BasePad : public tTJSNativeInstance
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
// font ralated constants
// core/visual/tvpfontstruc.h Ç∆ìØÇ∂Ç‡ÇÃÇ≈Ç∑ÅB
//---------------------------------------------------------------------------
// #define TVP_TF_ITALIC    0x01
// #define TVP_TF_BOLD      0x02
// #define TVP_TF_UNDERLINE 0x04
// #define TVP_TF_STRIKEOUT 0x08
//---------------------------------------------------------------------------


#include "PadImpl.h"


//---------------------------------------------------------------------------
// tTJSNC_Pad : TJS Pad Class
//---------------------------------------------------------------------------
class tTJSNC_Pad : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;
public:
	tTJSNC_Pad();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Pad();
//---------------------------------------------------------------------------
#endif
