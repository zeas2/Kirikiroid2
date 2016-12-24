//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// CPU idetification / features detection routine
//---------------------------------------------------------------------------
#ifndef DetectCPUH
#define DetectCPUH
//---------------------------------------------------------------------------
extern "C"
{
	extern tjs_uint32 TVPCPUType;
}

extern void TVPDetectCPU();
TJS_EXP_FUNC_DEF(tjs_uint32, TVPGetCPUType, ());

//---------------------------------------------------------------------------
#endif
