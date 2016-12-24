//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Utilities for Debugging
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "DebugImpl.h"


//---------------------------------------------------------------------------
// on-error hook
//---------------------------------------------------------------------------
void TVPOnErrorHook()
{
//	if(TVPMainForm) TVPMainForm->NotifySystemError();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSNC_Controller : TJS Controller Class
//---------------------------------------------------------------------------
class tTJSNC_Controller : public tTJSNativeClass
{
public:
	tTJSNC_Controller();

	static tjs_uint32 ClassID;
};
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Controller::ClassID = (tjs_uint32)-1;
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
tTJSNC_Controller::tTJSNC_Controller() : tTJSNativeClass(TJS_W("Controller"))
{
	TJS_BEGIN_NATIVE_MEMBERS(Debug)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(/*TJS class name*/Controller)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Controller)
//----------------------------------------------------------------------

// properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(visible)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = 0;// TVPMainForm->getVisible();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		//TVPMainForm->setVisible(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(visible)
//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS

}
//---------------------------------------------------------------------------
static iTJSDispatch2 * TVPGetControllerClass()
{
	struct tClassHolder
	{
		iTJSDispatch2 *Object;
		tClassHolder() { Object = new tTJSNC_Controller(); }
		~tClassHolder() { Object->Release(); }
	} static Holder;

	Holder.Object->AddRef();
	return Holder.Object;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_Console : TJS Console Class
//---------------------------------------------------------------------------
class tTJSNC_Console : public tTJSNativeClass
{
public:
	tTJSNC_Console();

	static tjs_uint32 ClassID;
};
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Console::ClassID = (tjs_uint32)-1;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
tTJSNC_Console::tTJSNC_Console() : tTJSNativeClass(TJS_W("Console"))
{
	TJS_BEGIN_NATIVE_MEMBERS(Debug)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(/*TJS class name*/Console)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Console)
//----------------------------------------------------------------------

// properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(visible)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
//		*result = TVPMainForm->GetConsoleVisible();
		*result = false;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		bool visible = param->operator bool();
//		TVPMainForm->SetConsoleVisible(visible);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(visible)
//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS

}
//---------------------------------------------------------------------------
static iTJSDispatch2 * TVPGetConsoleClass()
{
	struct tClassHolder
	{
		iTJSDispatch2 *Object;
		tClassHolder() { Object = new tTJSNC_Console(); }
		~tClassHolder() { Object->Release(); }
	} static Holder;

	Holder.Object->AddRef();
	return Holder.Object;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNC_Debug
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_Debug::CreateNativeInstance()
{
	return NULL;
}
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Debug()
{
	tTJSNativeClass *cls = new tTJSNC_Debug();

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(controller)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		iTJSDispatch2 *dsp = TVPGetControllerClass();
		*result = tTJSVariant(dsp, dsp);
		dsp->Release();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, controller)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(console)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		iTJSDispatch2 *dsp = TVPGetConsoleClass();
		*result = tTJSVariant(dsp, dsp);
		dsp->Release();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, console)
//----------------------------------------------------------------------
	return cls;
}
//---------------------------------------------------------------------------



