//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// native functions support
//---------------------------------------------------------------------------

#include "tjsCommHead.h"


#define TVP_WNF_A
#include "CompatibleNativeFuncs.h"
#undef TVP_WNF_A

#undef CompatibleNativeFuncsH

#define TVP_WNF_B
#include "CompatibleNativeFuncs.h"
#undef TVP_WNF_B

//---------------------------------------------------------------------------
// TVPInitCompatibleNativeFunctions
//---------------------------------------------------------------------------
void TVPInitCompatibleNativeFunctions()
{
	// retrieve function pointer from each module
	const tjs_int n = sizeof(TVPCompatibleNativeFuncs)/sizeof(tTVPCompatibleNativeFunc);
	for( tjs_int i = 0; i<n; i++ ) 	{
		tTVPCompatibleNativeFunc * p = TVPCompatibleNativeFuncs + i;
		HMODULE module = GetModuleHandle(p->Module);
		if(module) *(p->Ptr) = (void*)GetProcAddress(module, p->Name);
	}
}
//---------------------------------------------------------------------------
