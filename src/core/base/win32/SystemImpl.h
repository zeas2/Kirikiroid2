//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "System" class implementation
//---------------------------------------------------------------------------
#ifndef SystemImplH
#define SystemImplH
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(bool, TVPGetAsyncKeyState, (tjs_uint keycode, bool getcurrent = true));
//---------------------------------------------------------------------------
extern void TVPPostApplicationActivateEvent();
extern void TVPPostApplicationDeactivateEvent();
extern bool TVPShellExecute(const ttstr &target, const ttstr &param);
extern void TVPDoSaveSystemVariables();
//---------------------------------------------------------------------------
#endif
