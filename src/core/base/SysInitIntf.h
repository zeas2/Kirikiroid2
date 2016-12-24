//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// System Initialization and Uninitialization
//---------------------------------------------------------------------------
#ifndef SysInitIntfH
#define SysInitIntfH


//---------------------------------------------------------------------------
// System initialization and uninitialization
//---------------------------------------------------------------------------

//-- global data
extern ttstr TVPProjectDir; // project directory
extern ttstr TVPDataPath; // data directory


//-- implementation in this unit
extern void TVPSystemInit(void);
extern void TVPSystemUninit(void);



//-- implement in each platform
extern void TVPBeforeSystemInit(); // this must set TVPProjectDir
extern void TVPAfterSystemInit();
extern void TVPBeforeSystemUninit();
extern void TVPAfterSystemUninit();

extern void TVPTerminateAsync(int code=0); // do acynchronous teminating of application
extern void TVPTerminateSync(int code=0); // do synchronous teminating of application(never return)
extern void TVPMainWindowClosed(); // called from WindowIntf.cpp, caused by closing main window.
	// this function must shutdown the application, unless the controller window is visible.
//---------------------------------------------------------------------------

extern bool TVPSystemUninitCalled;
	// whether TVPSystemUninit is called or not

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// AtExit related
//---------------------------------------------------------------------------
void TVPAddAtExitHandler(tjs_int pri, void (*handler)());
struct tTVPAtExit
{
	tTVPAtExit(tjs_int pri, void (*handler)())
	{
		TVPAddAtExitHandler(pri, handler);
	}
};
#define TVP_ATEXIT_PRI_PREPARE    10
#define TVP_ATEXIT_PRI_SHUTDOWN   100
#define TVP_ATEXIT_PRI_RELEASE    1000
#define TVP_ATEXIT_PRI_CLEANUP    10000
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// Command line parameter operations (implement in each platform)
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(bool, TVPGetCommandLine, (const tjs_char * name, tTJSVariant *value = NULL));
	// retrieves command line parameter named "name".
	// command line parameter format must be "-name=value"
	// returns false if the the parameter is not exist, otherwise
	// sets the value to "value" and returns true.
TJS_EXP_FUNC_DEF(tjs_int, TVPGetCommandLineArgumentGeneration, ());
	// retrieves command line argument generation count. you can check
	// whether the command line options has changed, by comparing this value
	// to your value which is remembered when of previous call of this.
TJS_EXP_FUNC_DEF(void, TVPSetCommandLine, (const tjs_char * name, const ttstr & value));
	// sets command line to the specified value.
	// note that this function does not check any consistency or correctness of the value.

//---------------------------------------------------------------------------

#endif
