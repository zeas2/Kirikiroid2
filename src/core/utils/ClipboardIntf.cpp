//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Clipboard Class interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "ClipboardIntf.h"


//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSNI_BaseClipboard::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *dsp)
{
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD
tTJSNI_BaseClipboard::Invalidate()
{
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Clipboard::ClassID = -1;
//---------------------------------------------------------------------------
tTJSNC_Clipboard::tTJSNC_Clipboard(): inherited(TJS_W("Clipboard"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(Clipboard) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(/*TJS class name*/Clipboard)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Clipboard)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/hasFormat)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTVPClipboardFormat format  = (tTVPClipboardFormat)(tjs_int)*param[0];

	bool has = TVPClipboardHasFormat(format);

	if(result)
		*result = has;

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/hasFormat)
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
//----------------------------------------------------------------------

//--properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(asText)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		ttstr text;
		bool got = TVPClipboardGetText(text);
		if(got)
			*result = text;
		else
			result->Clear();
			// returns void if the clipboard does not have a text data
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TVPClipboardSetText(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(asText)
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNC_Clipboard
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_Clipboard::CreateNativeInstance()
{
	return NULL;
}
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Clipboard()
{
	tTJSNativeClass *cls = new tTJSNC_Clipboard();

	return cls;
}
//---------------------------------------------------------------------------
