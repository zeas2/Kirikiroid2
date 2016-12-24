//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Utilities for Debugging
//---------------------------------------------------------------------------
#ifndef DebugIntfH
#define DebugIntfH

#include "tjsNative.h"

//---------------------------------------------------------------------------
// global definitions
//---------------------------------------------------------------------------
extern bool TVPAutoLogToFileOnError;
extern bool TVPAutoClearLogOnError;
extern bool TVPLoggingToFile;
extern void TVPSetOnLog(void (*func)(const ttstr & line));
TJS_EXP_FUNC_DEF(void, TVPAddLog, (const ttstr &line));
TJS_EXP_FUNC_DEF(void, TVPAddImportantLog, (const ttstr &line));
extern ttstr TVPGetLastLog(tjs_uint n);
extern iTJSConsoleOutput * TVPGetTJS2ConsoleOutputGateway();
extern iTJSConsoleOutput * TVPGetTJS2DumpOutputGateway();
extern void TVPTJS2StartDump();
extern void TVPTJS2EndDump();
extern void TVPOnError();
extern ttstr TVPGetImportantLog();
extern void TVPSetLogLocation(const ttstr &loc);
extern ttstr TVPNativeLogLocation;
extern void TVPStartLogToFile(bool clear);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// implement in each platform
//---------------------------------------------------------------------------
//extern void TVPOnErrorHook();
	// called from TVPOnError, on system error.
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNC_Debug : TJS Debug Class
//---------------------------------------------------------------------------
class tTJSNC_Debug : public tTJSNativeClass
{
public:
	tTJSNC_Debug();

	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance * CreateNativeInstance();
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_Debug();
//---------------------------------------------------------------------------




#endif
