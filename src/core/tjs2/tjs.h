//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "tTJS" script language API class implementation
//---------------------------------------------------------------------------

#ifndef tjsH
#define tjsH

#include <vector>
#include "tjsConfig.h"
#include "tjsVariant.h"
#include "tjsInterface.h"
#include "tjsString.h"
#include "tjsMessage.h"


namespace TJS
{

//---------------------------------------------------------------------------
// TJS version
//---------------------------------------------------------------------------
extern const tjs_int TJSVersionMajor;
extern const tjs_int TJSVersionMinor;
extern const tjs_int TJSVersionRelease;
extern const tjs_int TJSVersionHex;

extern tjs_char TJSCompiledDate[];
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Console output callback interface
//---------------------------------------------------------------------------
class iTJSConsoleOutput
{
public:
	virtual void ExceptionPrint(const tjs_char *msg) = 0;
	virtual void Print(const tjs_char *msg) = 0;
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Object Hash Size Limit ( must be larger than or equal to 0 )
//---------------------------------------------------------------------------
extern tjs_int TJSObjectHashBitsLimit;



//---------------------------------------------------------------------------
// global options
//---------------------------------------------------------------------------
extern bool TJSEvalOperatorIsOnGlobal;
	// Post-! operator (evaluate expression) is to be executed on "this" context
	// since TJS2 2.4.1.
	// Turn this switch true makes post-! operator running on global context,
	// like TJS2 before 2.4.1.
extern bool TJSWarnOnNonGlobalEvalOperator;
	// Output warning against non-local post-! operator.
	// (For checking where the post-! operators are used)
extern bool TJSEnableDebugMode;
	// Enable TJS2 Debugging support. Enabling this may make the
	// program somewhat slower and using more memory.
	// Do not use this mode unless you want to debug the program.
extern bool TJSWarnOnExecutionOnDeletingObject;
	// Output warning against running code on context of
	// deleting-in-progress object. This is available only the Debug mode is
	// enabled.
extern bool TJSUnaryAsteriskIgnoresPropAccess;
	// Unary '*' operator means accessing property object directly without
	// normal property access, if this options is set true.
	// This is replaced with '&' operator since TJS2 2.4.15. Turn true for
	// gaining old compatibility.


//---------------------------------------------------------------------------
// tTJS class - "tTJS" TJS API Class
//---------------------------------------------------------------------------
class tTJSScriptBlock;
class tTJSPPMap;
class tTJSCustomObject;
class tTJSScriptCache;
class tTJS
{
	friend class tTJSScriptBlock;
private:
	tjs_uint RefCount; // reference count

public:
	tTJS();

protected:
	virtual ~tTJS();

public:
	void Cleanup();

	void AddRef();
	void Release();

	void Shutdown();

private:
	tTJSPPMap * PPValues;

	std::vector<tTJSScriptBlock*> ScriptBlocks;

	iTJSConsoleOutput *ConsoleOutput;

	tTJSCustomObject * Global;

	tTJSScriptCache * Cache;
	class tTJSVariantArrayStack *VariantArrayStack = nullptr;

public:
	iTJSDispatch2 * GetGlobal();
	iTJSDispatch2 * GetGlobalNoAddRef() const;

	tTJSVariantArrayStack* GetVariantArrayStack() { return VariantArrayStack; }

private:
	void AddScriptBlock(tTJSScriptBlock *block);
	void RemoveScriptBlock(tTJSScriptBlock *block);

public:
	void SetConsoleOutput(iTJSConsoleOutput *console);
	iTJSConsoleOutput * GetConsoleOutput() const { return ConsoleOutput; };
	void OutputToConsole(const tjs_char *msg) const;
	void OutputExceptionToConsole(const tjs_char *msg) const;
	void OutputToConsoleWithCentering(const tjs_char *msg, tjs_uint width) const;
	void OutputToConsoleSeparator(const tjs_char *text, tjs_uint count) const;

	void Dump(tjs_uint width = 80) const; // dumps all existing script block

	void ExecScript(const tjs_char *script, tTJSVariant *result = NULL,
		iTJSDispatch2 *context = NULL,
		const tjs_char *name = NULL, tjs_int lineofs = 0);

	void ExecScript(const ttstr &script, tTJSVariant *result = NULL,
		iTJSDispatch2 *context = NULL,
		const ttstr *name = NULL, tjs_int lineofs = 0);

	void EvalExpression(const tjs_char *expression, tTJSVariant *result,
		iTJSDispatch2 *context = NULL,
		const tjs_char *name = NULL, tjs_int lineofs = 0);

	void EvalExpression(const ttstr &expression, tTJSVariant *result,
		iTJSDispatch2 *context = NULL,
		const ttstr *name = NULL, tjs_int lineofs = 0);

	void SetPPValue(const tjs_char *name, const tjs_int32 value);
	tjs_int32 GetPPValue(const tjs_char *name);

	void DoGarbageCollection();

	// for Bytecode
	void LoadByteCode( const tjs_uint8* buff, size_t len, tTJSVariant *result = NULL,
		iTJSDispatch2 *context = NULL, const tjs_char *name = NULL);

	bool LoadByteCode( class tTJSBinaryStream* stream, tTJSVariant *result = NULL,
		iTJSDispatch2 *context = NULL, const tjs_char *name = NULL);

	// for Binary Dictionay Array
	static bool LoadBinaryDictionayArray( class tTJSBinaryStream* stream, tTJSVariant *result );

	void CompileScript( const tjs_char *script, class tTJSBinaryStream* output, bool isresultneeded = false, bool outputdebug = false, bool isexpression = false, const tjs_char *name = NULL, tjs_int lineofs = 0 );
};
//---------------------------------------------------------------------------


/*[*/
//---------------------------------------------------------------------------
// iTJSTextStream - used by Array.save/load Dictionaty.save/load
//---------------------------------------------------------------------------
class tTJSString;
class iTJSTextReadStream
{
public:
	virtual tjs_uint TJS_INTF_METHOD Read(tTJSString & targ, tjs_uint size) = 0;
	virtual void TJS_INTF_METHOD Destruct() = 0; // must delete itself
};
//---------------------------------------------------------------------------
class iTJSTextWriteStream
{
public:
	virtual void TJS_INTF_METHOD Write(const tTJSString & targ) = 0;
	virtual void TJS_INTF_METHOD Destruct() = 0; // must delete itself
};
//---------------------------------------------------------------------------
extern iTJSTextReadStream * (*TJSCreateTextStreamForRead)(const tTJSString &name,
	const tTJSString &modestr);
extern iTJSTextWriteStream * (*TJSCreateTextStreamForWrite)(const tTJSString &name,
	const tTJSString &modestr);
extern class tTJSBinaryStream * (*TJSCreateBinaryStreamForRead)(const tTJSString &name,
	const tTJSString &modestr);
extern class tTJSBinaryStream * (*TJSCreateBinaryStreamForWrite)(const tTJSString &name,
	const tTJSString &modestr);
//---------------------------------------------------------------------------




/*]*/
/*[*/
//---------------------------------------------------------------------------
// tTJSBinaryStream constants
//---------------------------------------------------------------------------
#define TJS_BS_READ 0
#define TJS_BS_WRITE 1
#define TJS_BS_APPEND 2
#define TJS_BS_UPDATE 3

#define TJS_BS_DELETE_ON_CLOSE	0x10

#define TJS_BS_ACCESS_MASK 0x0f
#define TJS_BS_OPTION_MASK 0xf0

#define TJS_BS_SEEK_SET 0
#define TJS_BS_SEEK_CUR 1
#define TJS_BS_SEEK_END 2
//---------------------------------------------------------------------------




/*]*/




//---------------------------------------------------------------------------
// tTJSBinaryStream base stream class
//---------------------------------------------------------------------------
class tTJSBinaryStream
{
private:
public:
	//-- must implement
	virtual tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence) = 0;
		/* if error, position is not changed */

	//-- optionally to implement
	virtual tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size) = 0;
		/* returns actually read size */

	virtual tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size) = 0;
		/* returns actually written size */

	virtual void TJS_INTF_METHOD SetEndOfStorage();
		// the default behavior is raising a exception
		/* if error, raises exception */

	//-- should re-implement for higher performance
	virtual tjs_uint64 TJS_INTF_METHOD GetSize() = 0;

	virtual ~tTJSBinaryStream() {;}

	tjs_uint64 GetPosition();

	void SetPosition(tjs_uint64 pos);

	void ReadBuffer(void *buffer, tjs_uint read_size);
	void WriteBuffer(const void *buffer, tjs_uint write_size);

	tjs_uint64 ReadI64LE(); // reads little-endian integers
	tjs_uint32 ReadI32LE();
	tjs_uint16 ReadI16LE();
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------

}
#endif


