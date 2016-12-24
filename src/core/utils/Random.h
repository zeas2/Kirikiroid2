//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Simple Pseudo Random Generator
//---------------------------------------------------------------------------
#ifndef RandomH
#define RandomH
//---------------------------------------------------------------------------


TJS_EXP_FUNC_DEF(void, TVPPushEnvironNoise, (const void *buf, tjs_int bufsize));
TJS_EXP_FUNC_DEF(void, TVPGetRandomBits128, (void *dest));

//---------------------------------------------------------------------------
#endif
