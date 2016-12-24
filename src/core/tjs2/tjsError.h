//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS2's C++ exception class and exception message
//---------------------------------------------------------------------------


#ifndef tjsErrorH
#define tjsErrorH

#ifndef TJS_DECL_MESSAGE_BODY

#include <stdexcept>
#include <string>
#include "tjsVariant.h"
#include "tjsString.h"
#include "tjsMessage.h"

namespace TJS
{
//---------------------------------------------------------------------------
extern ttstr TJSNonamedException;

//---------------------------------------------------------------------------
// macro
//---------------------------------------------------------------------------
#ifdef TJS_SUPPORT_VCL
	#define TJS_CONVERT_TO_TJS_EXCEPTION_ADDITIONAL \
		catch(const Exception &e) \
		{ \
			TJS_eTJSError(e.Message.c_str()); \
		}
#else
	#define TJS_CONVERT_TO_TJS_EXCEPTION_ADDITIONAL
#endif


#define TJS_CONVERT_TO_TJS_EXCEPTION \
	catch(const eTJSSilent &e) \
	{ \
		throw e; \
	} \
	catch(const eTJSScriptException &e) \
	{ \
		throw e; \
	} \
	catch(const eTJSScriptError &e) \
	{ \
		throw e; \
	} \
	catch(const eTJSError &e) \
	{ \
		throw e; \
	} \
	catch(const eTJS &e) \
	{ \
		TJS_eTJSError(e.GetMessage()); \
	} \
	catch(const std::exception &e) \
	{ \
		TJS_eTJSError(e.what()); \
	} \
	catch(const tjs_char *text) \
	{ \
		TJS_eTJSError(text); \
	} \
	catch(const char *text) \
	{ \
		TJS_eTJSError(text); \
	} \
	TJS_CONVERT_TO_TJS_EXCEPTION_ADDITIONAL
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TJSGetExceptionObject : retrieves TJS 'Exception' object
//---------------------------------------------------------------------------
extern void TJSGetExceptionObject(class tTJS *tjs, tTJSVariant *res, tTJSVariant &msg,
	tTJSVariant *trace/* trace is optional */ = NULL);
//---------------------------------------------------------------------------

#ifdef TJS_SUPPORT_VCL
	#define TJS_CONVERT_TO_TJS_EXCEPTION_OBJECT_ADDITIONAL(_tjs, _result_condition, _result_addr, _before_catched, _when_catched) \
	catch(EAccessViolation &e) \
	{ \
		_before_catched; \
		if(_result_condition) \
		{ \
			tTJSVariant msg(e.Message.c_str()); \
			TJSGetExceptionObject((_tjs), (_result_addr), msg, NULL); \
		} \
		_when_catched; \
	} \
	catch(Exception &e) \
	{ \
		_before_catched; \
		if(_result_condition) \
		{ \
			tTJSVariant msg(e.Message.c_str()); \
			TJSGetExceptionObject((_tjs), (_result_addr), msg, NULL); \
		} \
		_when_catched; \
	}
#else
	#define TJS_CONVERT_TO_TJS_EXCEPTION_OBJECT_ADDITIONAL(_tjs, _result_condition, _result_addr, _before_catched, _when_catched)
#endif


#define TJS_CONVERT_TO_TJS_EXCEPTION_OBJECT(tjs, result_condition, result_addr, before_catched, when_catched) \
	catch(eTJSSilent &e) \
	{ \
		throw e; \
	} \
	catch(eTJSScriptException &e) \
	{ \
		before_catched \
		if(result_condition) *(result_addr) = e.GetValue(); \
		when_catched; \
	} \
	catch(eTJSScriptError &e) \
	{ \
		before_catched \
		if(result_condition) \
		{ \
			tTJSVariant msg(e.GetMessage()); \
			tTJSVariant trace(e.GetTrace()); \
			TJSGetExceptionObject((tjs), (result_addr), msg, &trace); \
		} \
		when_catched; \
	} \
	catch(eTJS &e)  \
	{  \
		before_catched \
		if(result_condition) \
		{ \
			tTJSVariant msg(e.GetMessage()); \
			TJSGetExceptionObject((tjs), (result_addr), msg, NULL); \
		} \
		when_catched; \
	} \
	catch(exception &e) \
	{ \
		before_catched \
		if(result_condition) \
		{ \
			tTJSVariant msg(e.what()); \
			TJSGetExceptionObject((tjs), (result_addr), msg, NULL); \
		} \
		when_catched; \
	} \
	TJS_CONVERT_TO_TJS_EXCEPTION_OBJECT_ADDITIONAL(tjs, result_condition, result_addr, before_catched, when_catched) \
	catch(...) \
	{ \
		before_catched \
		if(result_condition) (result_addr)->Clear(); \
		when_catched; \
	}




//---------------------------------------------------------------------------
// eTJSxxxxx
//---------------------------------------------------------------------------
class eTJSSilent
{
	// silent exception
};
//---------------------------------------------------------------------------
class eTJS
{
public:
	eTJS() {;}
	eTJS(const eTJS&) {;}
	eTJS& operator= (const eTJS& e) { return *this; }
	virtual ~eTJS() {;}
	virtual const ttstr & GetMessage() const 
	{ return TJSNonamedException; }
};
//---------------------------------------------------------------------------
void TJS_eTJS();
//---------------------------------------------------------------------------
class eTJSError : public eTJS
{
public:
	eTJSError(const ttstr & Msg) :
		Message(Msg) {;}
	const ttstr & GetMessage() const { return Message; }

	void AppendMessage(const ttstr & msg) { Message += msg; }

private:
	ttstr Message;
};
//---------------------------------------------------------------------------
void TJS_eTJSError(const ttstr & msg);
void TJS_eTJSError(const tjs_char* msg);
//---------------------------------------------------------------------------
class eTJSVariantError : public eTJSError
{
public:
	eTJSVariantError(const ttstr & Msg) :
		eTJSError(Msg) {;}

	eTJSVariantError(const eTJSVariantError &ref) :
		eTJSError(ref) {;}
};
//---------------------------------------------------------------------------
void TJS_eTJSVariantError(const ttstr & msg);
void TJS_eTJSVariantError(const tjs_char * msg);
//---------------------------------------------------------------------------
class tTJSScriptBlock;
class tTJSInterCodeContext;
class eTJSScriptError : public eTJSError
{
	class tScriptBlockHolder
	{
	public:
		tScriptBlockHolder(tTJSScriptBlock *block);
		~tScriptBlockHolder();
		tScriptBlockHolder(const tScriptBlockHolder &holder);
		tTJSScriptBlock *Block;
	} Block;

	tjs_int Position;

	ttstr Trace;

public:
	tTJSScriptBlock * GetBlockNoAddRef() { return Block.Block; }

	tjs_int GetPosition() const { return Position; }

	tjs_int GetSourceLine() const;

	const tjs_char * GetBlockName() const;

	const ttstr & GetTrace() const { return Trace; }

	bool AddTrace(tTJSScriptBlock *block, tjs_int srcpos);
	bool AddTrace(tTJSInterCodeContext *context, tjs_int codepos);
	bool AddTrace(const ttstr & data);

	eTJSScriptError(const ttstr &  Msg,
		tTJSScriptBlock *block, tjs_int pos);

	eTJSScriptError(const eTJSScriptError &ref) :
		eTJSError(ref), Block(ref.Block), Position(ref.Position), Trace(ref.Trace) {;}
};
//---------------------------------------------------------------------------
void TJS_eTJSScriptError(const ttstr &msg, tTJSScriptBlock *block, tjs_int srcpos);
void TJS_eTJSScriptError(const tjs_char *msg, tTJSScriptBlock *block, tjs_int srcpos);
void TJS_eTJSScriptError(const ttstr &msg, tTJSInterCodeContext *context, tjs_int codepos);
void TJS_eTJSScriptError(const tjs_char *msg, tTJSInterCodeContext *context, tjs_int codepos);
//---------------------------------------------------------------------------
class eTJSScriptException : public eTJSScriptError
{
	tTJSVariant Value;
public:
	tTJSVariant & GetValue() { return Value; }

	eTJSScriptException(const ttstr & Msg,
		tTJSScriptBlock *block, tjs_int pos, tTJSVariant &val)
			: eTJSScriptError(Msg, block, pos), Value(val) {}

	eTJSScriptException(const eTJSScriptException &ref) :
		eTJSScriptError(ref), Value(ref.Value) {;}
};
//---------------------------------------------------------------------------
void TJS_eTJSScriptException(const ttstr &msg, tTJSScriptBlock *block,
	tjs_int srcpos, tTJSVariant &val);
void TJS_eTJSScriptException(const tjs_char *msg, tTJSScriptBlock *block,
	tjs_int srcpos, tTJSVariant &val);
void TJS_eTJSScriptException(const ttstr &msg, tTJSInterCodeContext *context,
	tjs_int codepos, tTJSVariant &val);
void TJS_eTJSScriptException(const tjs_char *msg, tTJSInterCodeContext *context,
	tjs_int codepos, tTJSVariant &val);
//---------------------------------------------------------------------------
class eTJSCompileError : public eTJSScriptError
{
public:
	eTJSCompileError(const ttstr &  Msg, tTJSScriptBlock *block, tjs_int pos) :
		eTJSScriptError(Msg, block, pos) {;}

	eTJSCompileError(const eTJSCompileError &ref) : eTJSScriptError(ref) {;}

};
//---------------------------------------------------------------------------
void TJS_eTJSCompileError(const ttstr & msg, tTJSScriptBlock *block, tjs_int srcpos);
void TJS_eTJSCompileError(const tjs_char * msg, tTJSScriptBlock *block, tjs_int srcpos);
//---------------------------------------------------------------------------
void TJSThrowFrom_tjs_error(tjs_error hr, const tjs_char *name = NULL);
#define TJS_THROW_IF_ERROR(x) { \
	tjs_error ____er; ____er = (x); if(TJS_FAILED(____er)) TJSThrowFrom_tjs_error(____er); }
//---------------------------------------------------------------------------
} // namespace TJS
//---------------------------------------------------------------------------
#endif // #ifndef TJS_DECL_MESSAGE_BODY



//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// messages
//---------------------------------------------------------------------------
namespace TJS
{
#ifdef TJS_DECL_MESSAGE_BODY
	#define TJS_MSG_DECL(name, msg) tTJSMessageHolder name(TJS_W(#name), msg);
	#define TJS_MSG_DECL_NULL(name) tTJSMessageHolder name(TJS_W(#name), NULL);
#else
	#define TJS_MSG_DECL(name, msg) extern tTJSMessageHolder name;
	#define TJS_MSG_DECL_NULL(name) extern tTJSMessageHolder name;
#endif
//---------------------------------------------------------------------------
#include "tjsErrorInc.h"

#undef TJS_MSG_DECL
#undef TJS_MSG_DECL_NULL
//---------------------------------------------------------------------------

}
//---------------------------------------------------------------------------


#endif // #ifndef tjsErrorH




