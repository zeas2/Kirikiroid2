//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Support for C++ native class/method/property definitions
//---------------------------------------------------------------------------
#ifndef tjsNativeH
#define tjsNativeH

#include "tjsObjectExtendable.h"

namespace TJS
{
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tjs_int32, TJSRegisterNativeClass, (const tjs_char *name));
TJS_EXP_FUNC_DEF(tjs_int32, TJSFindNativeClassID, (const tjs_char *name));
TJS_EXP_FUNC_DEF(const tjs_char *, TJSFindNativeClassName, (tjs_int32 id));
//---------------------------------------------------------------------------

/*[*/
//---------------------------------------------------------------------------
// tTJSNativeInstanceType
//---------------------------------------------------------------------------
enum tTJSNativeInstanceType
{
	nitClass,
	nitMethod,
	nitProperty
};
//---------------------------------------------------------------------------



/*]*/
/*[*/
//---------------------------------------------------------------------------
// tTJSNativeInstance
//---------------------------------------------------------------------------
class tTJSNativeInstance : public iTJSNativeInstance
{
public:
	virtual tjs_error TJS_INTF_METHOD Construct(tjs_int numparams,
		tTJSVariant **param, iTJSDispatch2 *tjs_obj) {return TJS_S_OK;}
	virtual void TJS_INTF_METHOD Invalidate() {;}
	virtual void TJS_INTF_METHOD Destruct() { delete this; }
	virtual ~tTJSNativeInstance() {;};
};
//---------------------------------------------------------------------------



/*]*/
/*[*/
//---------------------------------------------------------------------------
// tTJSNativeClassMethod
//---------------------------------------------------------------------------
typedef tjs_error (TJS_INTF_METHOD *tTJSNativeClassMethodCallback)
	(tTJSVariant *result,tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 *objthis);
/*]*/
#ifdef __TP_STUB_H__
/*[*/
class tTJSNativeClassMethod : public iTJSDispatch2 { };
/*]*/
#else
class tTJSNativeClassMethod;
#endif
/*[*/


/*]*/
TJS_EXP_FUNC_DEF(tTJSNativeClassMethod *, TJSCreateNativeClassMethod,
	(tTJSNativeClassMethodCallback callback));
//---------------------------------------------------------------------------
class tTJSNativeClassMethod : public tTJSDispatch
{
	typedef tTJSDispatch inherited;

protected:
	tTJSNativeClassMethodCallback Process;
public:

	tTJSNativeClassMethod(tTJSNativeClassMethodCallback processfunc);
	~tTJSNativeClassMethod();

	tjs_error TJS_INTF_METHOD
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tjs_char *classname,
		iTJSDispatch2 *objthis);
	tjs_error  TJS_INTF_METHOD
		FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis);
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNativeClassConstructor
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tTJSNativeClassMethod *, TJSCreateNativeClassConstructor,
	(tTJSNativeClassMethodCallback callback));
//---------------------------------------------------------------------------
class tTJSNativeClassConstructor : public tTJSNativeClassMethod
{
	typedef tTJSNativeClassMethod inherited;
public:
	tTJSNativeClassConstructor(tTJSNativeClassMethodCallback processfunc)
		: tTJSNativeClassMethod(processfunc) {;}

	tjs_error  TJS_INTF_METHOD
		FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis);
};
//---------------------------------------------------------------------------




/*[*/
//---------------------------------------------------------------------------
// tTJSNativeClassProperty
//---------------------------------------------------------------------------
typedef tjs_error (TJS_INTF_METHOD *tTJSNativeClassPropertyGetCallback)
	(tTJSVariant *result, iTJSDispatch2 *objthis);
typedef tjs_error (TJS_INTF_METHOD *tTJSNativeClassPropertySetCallback)
	(const tTJSVariant *param, iTJSDispatch2 *objthis);
/*]*/
#ifdef __TP_STUB_H__
/*[*/
class tTJSNativeClassProperty : public iTJSDispatch2 { };
/*]*/
#else
class tTJSNativeClassProperty;
#endif
/*[*/

/*]*/
TJS_EXP_FUNC_DEF(tTJSNativeClassProperty *, TJSCreateNativeClassProperty,
	(tTJSNativeClassPropertyGetCallback get,
	 tTJSNativeClassPropertySetCallback set));
//---------------------------------------------------------------------------
class tTJSNativeClassProperty : public tTJSDispatch
{
	typedef tTJSDispatch inherited;

protected:
	tTJSNativeClassPropertyGetCallback Get;
	tTJSNativeClassPropertySetCallback Set;
public:

	tTJSNativeClassProperty(tTJSNativeClassPropertyGetCallback get,
		tTJSNativeClassPropertySetCallback set);
	~tTJSNativeClassProperty();

	tjs_error TJS_INTF_METHOD
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tjs_char *classname,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
		iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tTJSVariant *param,
		iTJSDispatch2 *objthis);
};
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNativeClass
//---------------------------------------------------------------------------
class tTJSNativeClass : public tTJSExtendableObject
{
	typedef tTJSExtendableObject inherited;

public:
	tTJSNativeClass(const ttstr &name);
	virtual ~tTJSNativeClass();

	void RegisterNCM(const tjs_char *name,
		iTJSDispatch2 *dsp,
		const tjs_char *classname,
		tTJSNativeInstanceType type,
		tjs_uint32 flags = 0);

protected:
	tjs_int32 _ClassID;
	ttstr ClassName;
	void Finalize(void);

	virtual iTJSNativeInstance *CreateNativeInstance()  {return NULL;}

	virtual iTJSDispatch2 *CreateBaseTJSObject();

private:

public:
	tjs_error TJS_INTF_METHOD
	FuncCall(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		tTJSVariant *result,
			tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis);
	tjs_error TJS_INTF_METHOD
	CreateNew(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint,
		 iTJSDispatch2 **result,
			tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis);
	tjs_error TJS_INTF_METHOD
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		 const tjs_char *classname,
		iTJSDispatch2 *objthis);


	const ttstr & GetClassName() const { return ClassName; }

	void SetClassID(tjs_int32 classid) { _ClassID = classid; }
};
//---------------------------------------------------------------------------
inline TJS_EXP_FUNC_DEF(void, TJSNativeClassRegisterNCM, (
		tTJSNativeClass *cls,
		const tjs_char *name,
		iTJSDispatch2 *dsp,
		const tjs_char *classname,
		tTJSNativeInstanceType type,
		tjs_uint32 flags = 0))
{
	cls->RegisterNCM(name, dsp, classname, type, flags);
}
//---------------------------------------------------------------------------
inline TJS_EXP_FUNC_DEF(void, TJSNativeClassSetClassID, (
		tTJSNativeClass *cls,
		tjs_int32 classid))
{
	cls->SetClassID(classid);
}
//---------------------------------------------------------------------------



/*[*/
//---------------------------------------------------------------------------
// tTJSNativeClassForPlugin : service class for plugins
//---------------------------------------------------------------------------
typedef iTJSNativeInstance * (TJS_INTF_METHOD *tTJSCreateNativeInstance)();
/*]*/
#ifdef __TP_STUB_H__
/*[*/
class tTJSNativeClass : public iTJSDispatch2 { };
class tTJSNativeClassForPlugin : public tTJSNativeClass { };
/*]*/
#else
class tTJSNativeClassForPlugin;
#endif
/*[*/

/*]*/
//---------------------------------------------------------------------------
// This class is for nasty workaround of inter-compiler compatibility
class tTJSNativeClassForPlugin : public tTJSNativeClass
{
	tTJSCreateNativeInstance procCreateNativeInstance;
public:
	tTJSNativeClassForPlugin(const ttstr &name,
		tTJSCreateNativeInstance proc) :
			procCreateNativeInstance(proc), tTJSNativeClass(name)
	{
		;
	}

protected:
	iTJSNativeInstance *CreateNativeInstance()
		{return procCreateNativeInstance();}
};
//---------------------------------------------------------------------------
inline TJS_EXP_FUNC_DEF(tTJSNativeClassForPlugin *, TJSCreateNativeClassForPlugin,
	(const ttstr &name, tTJSCreateNativeInstance createinstance))
{
	return new tTJSNativeClassForPlugin(name, createinstance);
}
//---------------------------------------------------------------------------



/*[*/
//---------------------------------------------------------------------------
// following macros are to be written in the constructor of child class
// to define native methods/properties.
/*]*/
#ifdef __TP_STUB_H__
/*[*/
#define TJS_NCM_REG_THIS classobj
#define TJS_NATIVE_SET_ClassID TJS_NATIVE_CLASSID_NAME = TJS_NCM_CLASSID;
/*]*/
#else
#define TJS_NCM_REG_THIS  this
#define TJS_NATIVE_SET_ClassID ClassID = TJS_NCM_CLASSID;
#define TJS_NATIVE_CLASSID_NAME TJS_NCM_CLASSID
#endif
/*[*/

#define TJS_GET_NATIVE_INSTANCE(varname, typename) \
		if(!objthis) return TJS_E_NATIVECLASSCRASH; \
		typename *varname; \
		{ \
			tjs_error hr; \
			hr = objthis->NativeInstanceSupport(TJS_NIS_GETINSTANCE, \
					TJS_NATIVE_CLASSID_NAME, (iTJSNativeInstance**)&varname); \
			if(TJS_FAILED(hr)) return TJS_E_NATIVECLASSCRASH; \
		}

#define TJS_GET_NATIVE_INSTANCE_OUTER(classname, varname, typename) \
		if(!objthis) return TJS_E_NATIVECLASSCRASH; \
		typename *varname; \
		{ \
			tjs_error hr; \
			hr = objthis->NativeInstanceSupport(TJS_NIS_GETINSTANCE, \
					classname::ClassID, (iTJSNativeInstance**)&varname); \
			if(TJS_FAILED(hr)) return TJS_E_NATIVECLASSCRASH; \
		}

#define TJS_BEGIN_NATIVE_MEMBERS(classname) \
	{ \
		static const tjs_char *__classname = TJS_W(#classname); \
		static tjs_int32 TJS_NCM_CLASSID = \
			TJSRegisterNativeClass(__classname); \
		TJSNativeClassSetClassID(TJS_NCM_REG_THIS, TJS_NCM_CLASSID); \
		TJS_NATIVE_SET_ClassID

#define TJS_BEGIN_NATIVE_METHOD_DECL(name) \
		struct NCM_##name { \
			static tjs_error TJS_INTF_METHOD \
			Process( tTJSVariant *result, \
				tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis) {

#define TJS_END_NATIVE_METHOD_DECL_INT \
			} \
		};

#define TJS_END_NATIVE_METHOD_DECL(name) \
		TJS_END_NATIVE_METHOD_DECL_INT \
		TJSNativeClassRegisterNCM(TJS_NCM_REG_THIS, TJS_W(#name), \
		TJSCreateNativeClassMethod(NCM_##name::Process), __classname, nitMethod);

#define TJS_END_NATIVE_HIDDEN_METHOD_DECL(name) \
		TJS_END_NATIVE_METHOD_DECL_INT \
		TJSNativeClassRegisterNCM(TJS_NCM_REG_THIS, TJS_W(#name), \
		TJSCreateNativeClassMethod(NCM_##name::Process), __classname, nitMethod, \
		TJS_HIDDENMEMBER);

#define TJS_END_NATIVE_STATIC_METHOD_DECL(name) \
		TJS_END_NATIVE_METHOD_DECL_INT \
		TJSNativeClassRegisterNCM(TJS_NCM_REG_THIS, TJS_W(#name), \
		TJSCreateNativeClassMethod(NCM_##name::Process), __classname, nitMethod, \
		TJS_STATICMEMBER);

#define TJS_END_NATIVE_METHOD_DECL_OUTER(object, name) \
		TJS_END_NATIVE_METHOD_DECL_INT \
		TJSNativeClassRegisterNCM((object), TJS_W(#name), \
		TJSCreateNativeClassMethod(NCM_##name::Process), \
		(object)->GetClassName().c_str(), nitMethod);

#define TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(object, name) \
		TJS_END_NATIVE_METHOD_DECL_INT \
		TJSNativeClassRegisterNCM((object), TJS_W(#name), \
		TJSCreateNativeClassMethod(NCM_##name::Process), \
		(object)->GetClassName().c_str(), nitMethod, TJS_STATICMEMBER);


#define TJS_DECL_EMPTY_FINALIZE_METHOD \
	TJS_BEGIN_NATIVE_METHOD_DECL(finalize) \
	{ return TJS_S_OK; } \
	TJS_END_NATIVE_METHOD_DECL(finalize)

#define TJS_NATIVE_CONSTRUCTOR_CALL_NATIVE_CONSTRUCTOR(varname, typename) \
				typename *varname; \
				{ \
					tjs_error hr; \
					hr = objthis->NativeInstanceSupport(TJS_NIS_GETINSTANCE, \
						TJS_NATIVE_CLASSID_NAME, \
						(iTJSNativeInstance**)&varname); \
					if(TJS_FAILED(hr)) return TJS_E_NATIVECLASSCRASH; \
					if(!varname) return TJS_E_NATIVECLASSCRASH; \
					hr = varname->Construct(numparams, param, objthis); \
					if(TJS_FAILED(hr)) return hr; \
				}

#define TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(classname) \
		struct NCM_##classname { \
			static tjs_error TJS_INTF_METHOD \
			Process(tTJSVariant *result, \
			tjs_int numparams, tTJSVariant **param,	iTJSDispatch2 *objthis) {

#define TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(varname, typename, classname) \
		TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(classname) \
		TJS_NATIVE_CONSTRUCTOR_CALL_NATIVE_CONSTRUCTOR(varname, typename)

#define TJS_END_NATIVE_CONSTRUCTOR_DECL(name) \
		TJS_END_NATIVE_METHOD_DECL_INT \
		TJSNativeClassRegisterNCM(TJS_NCM_REG_THIS, TJS_W(#name), \
		TJSCreateNativeClassConstructor(NCM_##name::Process), __classname, \
		nitMethod);

#define TJS_END_NATIVE_STATIC_CONSTRUCTOR_DECL(name) \
		TJS_END_NATIVE_METHOD_DECL_INT \
		TJSNativeClassRegisterNCM(TJS_NCM_REG_THIS, TJS_W(#name), \
		TJSCreateNativeClassConstructor(NCM_##name::Process), __classname, \
		nitMethod, TJS_STATICMEMBER);

#define TJS_BEGIN_NATIVE_PROP_DECL(name) \
		struct NCM_##name

#define TJS_END_NATIVE_PROP_DECL(name) \
		;TJSNativeClassRegisterNCM(TJS_NCM_REG_THIS, TJS_W(#name), \
		TJSCreateNativeClassProperty(NCM_##name::Get, NCM_##name::Set), \
		__classname, nitProperty);

#define TJS_END_NATIVE_PROP_DECL_OUTER(object, name) \
		;TJSNativeClassRegisterNCM((object), TJS_W(#name), \
		TJSCreateNativeClassProperty(NCM_##name::Get, NCM_##name::Set), \
		(object)->GetClassName().c_str(), nitProperty);

#define TJS_END_NATIVE_STATIC_PROP_DECL(name) \
		;TJSNativeClassRegisterNCM(TJS_NCM_REG_THIS, TJS_W(#name), \
		TJSCreateNativeClassProperty(NCM_##name::Get, NCM_##name::Set), \
		__classname, nitProperty, TJS_STATICMEMBER);

#define TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(object, name) \
		;TJSNativeClassRegisterNCM((object), TJS_W(#name), \
		TJSCreateNativeClassProperty(NCM_##name::Get, NCM_##name::Set), \
		(object)->GetClassName().c_str(), nitProperty, TJS_STATICMEMBER);


#define TJS_BEGIN_NATIVE_PROP_GETTER \
		static tjs_error TJS_INTF_METHOD Get(tTJSVariant *result, \
		iTJSDispatch2 *objthis) { \

#define TJS_END_NATIVE_PROP_GETTER \
		}

#define TJS_DENY_NATIVE_PROP_GETTER \
		static tjs_error TJS_INTF_METHOD Get(tTJSVariant *result, \
		iTJSDispatch2 *objthis) \
		{ return TJS_E_ACCESSDENYED; }

#define TJS_BEGIN_NATIVE_PROP_SETTER \
		static tjs_error TJS_INTF_METHOD Set(const tTJSVariant *param, \
		iTJSDispatch2 *objthis) { \

#define TJS_END_NATIVE_PROP_SETTER \
		}

#define TJS_DENY_NATIVE_PROP_SETTER \
		static tjs_error TJS_INTF_METHOD Set(const tTJSVariant *param, \
		iTJSDispatch2 *objthis) \
		{ return TJS_E_ACCESSDENYED; }

#define TJS_END_NATIVE_MEMBERS \
	}

#define TJS_PARAM_EXIST(num) (numparams>(num) ? param[num]->Type()!=tvtVoid : false)


/*]*/

//---------------------------------------------------------------------------
// tTJSNativeFunction
//---------------------------------------------------------------------------
// base class used for native function ( for non-class-method )
class tTJSNativeFunction : public tTJSDispatch
{
	typedef tTJSDispatch inherited;
public:
	tTJSNativeFunction(const tjs_char *name = NULL);
		// 'name' is just to be used as a label for debugging
	~tTJSNativeFunction();

	tjs_error TJS_INTF_METHOD FuncCall(
		tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint, tTJSVariant *result,
		tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *objthis);

	tjs_error TJS_INTF_METHOD
	IsInstanceOf(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint,
		const tjs_char *classname,
		iTJSDispatch2 *objthis);

protected:
	tjs_error virtual Process(tTJSVariant *result, tjs_int numparams,
		tTJSVariant **param, iTJSDispatch2 *objthis) = 0;
		// override this instead of FuncCall
};
//---------------------------------------------------------------------------
} // namespace TJS

#endif
