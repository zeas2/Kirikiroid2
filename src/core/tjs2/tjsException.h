//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS Exception Class
//---------------------------------------------------------------------------
#ifndef tjsExceptionH
#define tjsExceptionH

#include "tjsNative.h"

namespace TJS
{
//---------------------------------------------------------------------------
// tTJSNC_Exception
//---------------------------------------------------------------------------
class tTJSNC_Exception : public tTJSNativeClass
{
public:
	tTJSNC_Exception();
private:
	static tjs_uint32 ClassID;
};
//---------------------------------------------------------------------------
} // namespace TJS

#endif
