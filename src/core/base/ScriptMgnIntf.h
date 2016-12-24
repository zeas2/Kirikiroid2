//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS2 Script Managing
//---------------------------------------------------------------------------
#ifndef ScriptMgnImtfH
#define ScriptMgnImtfH

#include "tjs.h"

#include "tjsNative.h"
#include "tjsError.h"

//---------------------------------------------------------------------------
// implementation in this unit
//---------------------------------------------------------------------------
extern ttstr TVPStartupScriptName;


extern void TVPInitScriptEngine();
extern void TVPUninitScriptEngine();
extern void TVPRestartScriptEngine();
extern tTJS*  TVPGetScriptEngine();
TJS_EXP_FUNC_DEF(iTJSDispatch2 *, TVPGetScriptDispatch, ());
TJS_EXP_FUNC_DEF(void, TVPExecuteScript, (const ttstr& content, tTJSVariant *result = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteScript, (const ttstr& content, iTJSDispatch2 *context, tTJSVariant *result = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteExpression, (const ttstr &content, tTJSVariant *result = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteExpression, (const ttstr &content, iTJSDispatch2 *context, tTJSVariant *result = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteScript, (const ttstr& content, const ttstr &name, tjs_int lineofs, tTJSVariant *result = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteScript, (const ttstr& content, const ttstr &name, tjs_int lineofs, iTJSDispatch2 *context, tTJSVariant *result = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteExpression, (const ttstr &content, const ttstr &name, tjs_int lineofs, tTJSVariant *result = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteExpression, (const ttstr &content, const ttstr &name, tjs_int lineofs, iTJSDispatch2 *context, tTJSVariant *result = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteStorage, (const ttstr &name, tTJSVariant *result = NULL,
	bool isexpression = false, const tjs_char *modestr = NULL));
TJS_EXP_FUNC_DEF(void, TVPExecuteStorage, (const ttstr &name, iTJSDispatch2 *context, tTJSVariant *result = NULL,
	bool isexpression = false, const tjs_char *modestr = NULL));
TJS_EXP_FUNC_DEF(void, TVPDumpScriptEngine, ());

TJS_EXP_FUNC_DEF(void, TVPExecuteBytecode, (const tjs_uint8* content, size_t len, iTJSDispatch2 *context, tTJSVariant *result = NULL, const tjs_char *name = NULL ));

extern void TVPExecuteStartupScript();
TJS_EXP_FUNC_DEF(void, TVPCreateMessageMapFile, (const ttstr &filename));

//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// implementation in each platform to show script exception message
//---------------------------------------------------------------------------
extern void TVPShowScriptException(eTJS &e);
extern void TVPShowScriptException(eTJSScriptError &e);
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPBeforeProcessUnhandledException (implementation in each platform)
//---------------------------------------------------------------------------
extern void TVPBeforeProcessUnhandledException();
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// unhandled exception handler related macros
//---------------------------------------------------------------------------
extern bool TVPProcessUnhandledException(eTJSScriptException &e);
extern bool TVPProcessUnhandledException(eTJSScriptError &e);
extern bool TVPProcessUnhandledException(eTJS &e);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TVPInitializeStartupScript
//---------------------------------------------------------------------------
extern void TVPInitializeStartupScript();
extern bool TVPCheckProcessLog();

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// unhandled exception handler related macros
//---------------------------------------------------------------------------
#define TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION(origin) \
	catch(eTJSScriptException &e) \
	{ \
		TVPBeforeProcessUnhandledException(); \
		e.AddTrace(ttstr(origin)); \
		if(!TVPProcessUnhandledException(e)) \
			TVPShowScriptException(e); \
	} \
	catch(eTJSScriptError &e) \
	{ \
		TVPBeforeProcessUnhandledException(); \
		e.AddTrace(ttstr(origin)); \
		if(!TVPProcessUnhandledException(e)) \
			TVPShowScriptException(e); \
	} \
	catch(eTJS &e) \
	{ \
		TVPBeforeProcessUnhandledException(); \
		if(!TVPProcessUnhandledException(e)) \
			TVPShowScriptException(e); \
	} \
	catch(...) \
	{ \
		TVPBeforeProcessUnhandledException(); \
		throw; \
	}
#define TVP_CATCH_AND_SHOW_SCRIPT_EXCEPTION_FORCE_SHOW_EXCEPTION(origin) \
	catch(eTJSScriptError &e) \
	{ \
		TVPBeforeProcessUnhandledException(); \
		e.AddTrace(ttstr(origin)); \
		TVPShowScriptException(e); \
	} \
	catch(eTJS &e) \
	{ \
		TVPBeforeProcessUnhandledException(); \
		TVPShowScriptException(e); \
	} \
	catch(...) \
	{ \
		TVPBeforeProcessUnhandledException(); \
		throw; \
	}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNC_Scripts : TJS Scripts class
//---------------------------------------------------------------------------
class tTJSNC_Scripts : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;

public:
	tTJSNC_Scripts();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_Scripts();
//---------------------------------------------------------------------------


#endif
