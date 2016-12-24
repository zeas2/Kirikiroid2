//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS Debugging support
//---------------------------------------------------------------------------
#ifndef tjsDebugH
#define tjsDebugH

#include "tjsString.h"

#ifdef ENABLE_DEBUGGER
#include <list>
#include <string>
#endif //ENABLE_DEBUGGER

namespace TJS
{

#ifdef ENABLE_DEBUGGER
struct ScopeKey {
	int ClassIndex;	//!< クラス名インデックス
	int FuncIndex;	//!< 関数名インデックス
	int FileIndex;	//!< ファイル名インデックス
	int CodeOffset;	//!< VM コードオフセット

	ScopeKey()
	: ClassIndex(-1), FuncIndex(-1), FileIndex(-1), CodeOffset(-1)
	{}
	ScopeKey( int classidx, int func, int file, int codeoffset )
	: ClassIndex(classidx), FuncIndex(func), FileIndex(file), CodeOffset(codeoffset)
	{}
	void Set( int classidx, int func, int file, int codeoffset ) {
		ClassIndex = classidx;
		FuncIndex = func;
		FileIndex = file;
		CodeOffset = codeoffset;
	}

	bool operator ==( const ScopeKey& rhs ) const {
		return( ClassIndex == rhs.ClassIndex && FuncIndex == rhs.FuncIndex && FileIndex == rhs.FileIndex && CodeOffset == rhs.CodeOffset );
	}
	bool operator !=( const ScopeKey& rhs ) const {
		return( ClassIndex != rhs.ClassIndex || FuncIndex != rhs.FuncIndex || FileIndex != rhs.FileIndex || CodeOffset != rhs.CodeOffset );
	}
	bool operator < ( const ScopeKey& rhs ) const {
		// クラス、関数名
		if( ClassIndex == rhs.ClassIndex ) {
			if( FuncIndex == rhs.FuncIndex ) {
				if( FileIndex == rhs.FileIndex ) {
					return CodeOffset < rhs.CodeOffset;
				} else {
					return FileIndex < rhs.FileIndex;
				}
			} else {
				return FuncIndex < rhs.FuncIndex;
			}
		} else {
			return ClassIndex < rhs.ClassIndex;
		}
	}
};
#endif // ENABLE_DEBUGGER

//---------------------------------------------------------------------------
// ObjectHashMap : hash map to track object construction/destruction
//---------------------------------------------------------------------------
// object hash map flags
#define TJS_OHMF_EXIST        1  // The object is in object hash map
#define TJS_OHMF_INVALIDATED  2  // The object had been invalidated  // currently not used
#define TJS_OHMF_DELETING     4  // The object is now being deleted
#define TJS_OHMF_SET          (~0)
#define TJS_OHMF_UNSET        (0)
//---------------------------------------------------------------------------
class tTJSScriptBlock;
struct tTJSObjectHashMapRecord;

class tTJSObjectHashMap;
extern tTJSObjectHashMap * TJSObjectHashMap;
extern tTJSBinaryStream * TJSObjectHashMapLog;
extern void TJSAddRefObjectHashMap();
extern void TJSReleaseObjectHashMap();
extern void TJSAddObjectHashRecord(void * object);
extern void TJSRemoveObjectHashRecord(void * object);
extern void TJSObjectHashSetType(void * object, const ttstr &type);
extern void TJSSetObjectHashFlag(void * object, tjs_uint32 flags_to_change, tjs_uint32 bits);
extern void TJSReportAllUnfreedObjects(iTJSConsoleOutput * output);
extern bool TJSObjectHashAnyUnfreed();
extern void TJSObjectHashMapSetLog(tTJSBinaryStream * stream);
extern void TJSWriteAllUnfreedObjectsToLog();
extern void TJSWarnIfObjectIsDeleting(iTJSConsoleOutput * output, void * object);
extern void TJSReplayObjectHashMapLog();
static inline bool TJSObjectHashMapEnabled() { return TJSObjectHashMap || TJSObjectHashMapLog; }
extern inline bool TJSObjectTypeInfoEnabled() { return 0!=TJSObjectHashMap; }
extern inline bool TJSObjectFlagEnabled() { return 0!=TJSObjectHashMap; }
extern ttstr TJSGetObjectTypeInfo(void * object);
extern tjs_uint32 TJSGetObjectHashCheckFlag(void * object);
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// StackTracer : stack to trace function call trace
//---------------------------------------------------------------------------
class tTJSStackTracer;
class tTJSInterCodeContext;
extern tTJSStackTracer * TJSStackTracer;
extern void TJSAddRefStackTracer();
extern void TJSReleaseStackTracer();
extern void TJSStackTracerPush(tTJSInterCodeContext *context, bool in_try);
extern void TJSStackTracerSetCodePointer(const tjs_int32 * codebase, tjs_int32 * const * codeptr);
extern void TJSStackTracerPop();
extern ttstr TJSGetStackTraceString(tjs_int limit = 0, const tjs_char *delimiter = NULL);
static inline bool TJSStackTracerEnabled() { return 0!=TJSStackTracer; }
//---------------------------------------------------------------------------

#ifdef ENABLE_DEBUGGER
extern void TJSDebuggerHook( tjs_int evtype, const tjs_char *filename, tjs_int lineno, tTJSInterCodeContext* ctx = NULL );
extern void TJSDebuggerLog( const ttstr &line, bool impotant );

extern void TJSDebuggerGetScopeKey( struct ScopeKey& scope,  const tjs_char* classname, const tjs_char* funcname, const tjs_char* filename, int codeoffset );
extern void TJSDebuggerAddLocalVariable( const struct ScopeKey& key, const tjs_char* varname, int regaddr );
extern void TJSDebuggerAddLocalVariable( const tjs_char* filename, const tjs_char* classname, const tjs_char* funcname, int codeoffset, const tjs_char* varname, int regaddr );
extern void TJSDebuggerGetLocalVariableString( const struct ScopeKey& key, tTJSVariant* ra, std::list<std::wstring>& values );
extern void TJSDebuggerGetLocalVariableString( const tjs_char* filename, const tjs_char* classname, const tjs_char* funcname, int codeoffset, tTJSVariant* ra, std::list<std::wstring>& values );
extern void TJSDebuggerClearLocalVariable( const ScopeKey& key );
extern void TJSDebuggerClearLocalVariable( const tjs_char* classname, const tjs_char* funcname, const tjs_char* filename, int codeoffset );

extern void TJSDebuggerAddClassVariable( const tjs_char* classname, const tjs_char* varname, int regaddr );
extern void TJSDebuggerGetClassVariableString( const tjs_char* classname, tTJSVariant* ra, tTJSVariant* da, std::list<std::wstring>& values );
extern void TJSDebuggerClearLocalVariable( const tjs_char* classname );
#endif	// ENABLE_DEBUGGER

} // namespace TJS

#endif
