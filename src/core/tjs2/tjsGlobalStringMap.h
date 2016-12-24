//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS Global String Map
//---------------------------------------------------------------------------
#ifndef tjsGlobalStringMapH
#define tjsGlobalStringMapH

#include "tjsString.h"

namespace TJS
{
//---------------------------------------------------------------------------
// tTJSGlobalStringMap - hash map to keep constant strings shared
//---------------------------------------------------------------------------
extern void TJSAddRefGlobalStringMap();
extern void TJSReleaseGlobalStringMap();
TJS_EXP_FUNC_DEF(ttstr, TJSMapGlobalStringMap, (const ttstr & string));
//---------------------------------------------------------------------------
} // namespace TJS

#endif
