//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "Plugins" class interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"


#include "PluginIntf.h"
#include "MsgIntf.h"


//---------------------------------------------------------------------------
// tTJSNC_Plugins
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Plugins::ClassID = -1;
tTJSNC_Plugins::tTJSNC_Plugins() : inherited(TJS_W("Plugins"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(Plugins)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------

//--properties

//---------------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
tTJSNativeInstance * tTJSNC_Plugins::CreateNativeInstance()
{
	// this class cannot create an instance
	TVPThrowExceptionMessage(TVPCannotCreateInstance);
	return NULL;
}
//---------------------------------------------------------------------------

