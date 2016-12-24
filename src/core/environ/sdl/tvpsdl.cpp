//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TVP Win32 Project File
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <assert.h>
//---------------------------------------------------------------------------
void TVPOnError();
//---------------------------------------------------------------------------
#ifdef TVP_SUPPORT_ERI
#	pragma link "../../../../Lib/liberina.lib"
#endif
//---------------------------------------------------------------------------
#if defined(WIN32) && defined(_DEBUG)
#include <new.h>
#define TVP_MAIN_DEBUG
//#include <vld.h>
#include "GraphicsLoaderIntf.h"
#include <set>
#else
#include <new>
#endif
#include "StorageIntf.h"
#include <thread>
#include "EventIntf.h"
#include "Application.h"
#include "SysInitImpl.h"
#include "TickCount.h"
#include "ScriptMgnIntf.h"
#include "DebugIntf.h"
//#include "PluginImpl.h"
#include "ScriptMgnImpl.h"
#include "SystemIntf.h"
#include "ThreadIntf.h"
#include "Platform.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "Exception.h"

//#define HOOK_MALLOC_FOR_OVERRUN
//#define TC_MALLOC

extern "C" void monstartup(const char *libname);

static bool tc_malloc_startup = false;
