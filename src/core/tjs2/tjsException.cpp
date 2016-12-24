//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS Exception Class
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsException.h"

namespace TJS
{



//---------------------------------------------------------------------------
// tTJSNC_Exception : TJS Native Class : Exception
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Exception::ClassID = (tjs_uint32)-1;
tTJSNC_Exception::tTJSNC_Exception() :
	tTJSNativeClass(TJS_W("Exception"))
{
	// class constructor

	TJS_BEGIN_NATIVE_MEMBERS(/*TJS class name*/Exception)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(/*TJS class name*/Exception)
{
	tTJSVariant val = TJS_W("");
	if(TJS_PARAM_EXIST(0)) val.CopyRef(*param[0]);

	static tTJSString message_name(TJS_W("message"));
	objthis->PropSet(TJS_MEMBERENSURE, message_name.c_str(), message_name.GetHint(),
		&val, objthis);

	if(TJS_PARAM_EXIST(1)) val.CopyRef(*param[1]); else val = TJS_W("");

	static tTJSString trace_name(TJS_W("trace"));
	objthis->PropSet(TJS_MEMBERENSURE, trace_name.c_str(), trace_name.GetHint(),
		&val, objthis);

	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Exception)
//---------------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
} // tTJSNC_Exception::tTJSNC_Exception()
//---------------------------------------------------------------------------
} // namespace TJS

