//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// System Initialization and Uninitialization
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <vector>
#include <algorithm>
#include <functional>

#include "tjsUtils.h"
#include "SysInitIntf.h"
#include "ScriptMgnIntf.h"
#include "tvpgl.h"
#include "Protect.h"


//---------------------------------------------------------------------------
// global data
//---------------------------------------------------------------------------
ttstr TVPProjectDir; // project directory (in unified storage name)
ttstr TVPDataPath; // data directory (in unified storage name)
//---------------------------------------------------------------------------



extern void TVPGL_C_Init();
//---------------------------------------------------------------------------
// TVPSystemInit : Entire System Initialization
//---------------------------------------------------------------------------
void TVPSystemInit(void)
{
#if CC_TARGET_PLATFORM != CC_PLATFORM_WIN32
#ifndef CC_TARGET_OS_IPHONE
	if (!TVPProtectInit()) return;
#endif
//#else
#ifdef USING_PROTECT
	while (!TVPProtectInit()) {
		TVPUpdateLicense();
	}
#endif
#endif

	TVPBeforeSystemInit();

	TVPInitScriptEngine();

	TVPInitTVPGL();
//	TVPGL_C_Init();

	TVPAfterSystemInit();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPSystemUninit : System shutdown, cleanup, etc...
//---------------------------------------------------------------------------
static void TVPCauseAtExit();
bool TVPSystemUninitCalled = false;
void TVPSystemUninit(void)
{
	if(TVPSystemUninitCalled) return;
	TVPSystemUninitCalled = true;

	TVPBeforeSystemUninit();

	TVPUninitTVPGL();

	try
	{
		TVPUninitScriptEngine();
	}
	catch(...)
	{
		// ignore errors
	}

	TVPAfterSystemUninit();

	TVPCauseAtExit();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPAddAtExitHandler related
//---------------------------------------------------------------------------
struct tTVPAtExitInfo
{
	tTVPAtExitInfo(tjs_int pri, void(*handler)())
	{
		Priority = pri, Handler = handler;
	}

	tjs_int Priority;
	void (*Handler)();

	bool operator <(const tTVPAtExitInfo & r) const
		{ return this->Priority < r.Priority; }
	bool operator >(const tTVPAtExitInfo & r) const
		{ return this->Priority > r.Priority; }
	bool operator ==(const tTVPAtExitInfo & r) const
		{ return this->Priority == r.Priority; }

};
static std::vector<tTVPAtExitInfo> *TVPAtExitInfos = NULL;
static bool TVPAtExitShutdown = false;
//---------------------------------------------------------------------------
void TVPAddAtExitHandler(tjs_int pri, void (*handler)())
{
	if(TVPAtExitShutdown) return;

	if(!TVPAtExitInfos) TVPAtExitInfos = new std::vector<tTVPAtExitInfo>();
	TVPAtExitInfos->push_back(tTVPAtExitInfo(pri, handler));
}
//---------------------------------------------------------------------------
static void TVPCauseAtExit()
{
	if(TVPAtExitShutdown) return;
	TVPAtExitShutdown = true;

	std::sort(TVPAtExitInfos->begin(), TVPAtExitInfos->end()); // descending sort

	std::vector<tTVPAtExitInfo>::iterator i;
	for(i = TVPAtExitInfos->begin(); i!=TVPAtExitInfos->end(); i++)
	{
		i->Handler();
	}

	delete TVPAtExitInfos;
}
//---------------------------------------------------------------------------






