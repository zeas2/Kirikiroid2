//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Script block caching
//---------------------------------------------------------------------------

#ifndef tjsScriptCacheH
#define tjsScriptCacheH

#include "tjsConfig.h"
#include "tjsHashSearch.h"
#include "tjsUtils.h"


namespace TJS
{

//---------------------------------------------------------------------------
// tTJSScriptCache - a class to cache script blocks
//---------------------------------------------------------------------------
class tTJS;
class tTJSScriptCache
{
private:
	struct tScriptCacheData
	{
		ttstr Script;
		bool ExpressionMode;
		bool MustReturnResult;

		bool operator ==(const tScriptCacheData &rhs) const
		{
			return Script == rhs.Script && ExpressionMode == rhs.ExpressionMode &&
				MustReturnResult == rhs.MustReturnResult;
		}
	};

	class tScriptCacheHashFunc
	{
	public:
		static tjs_uint32 Make(const tScriptCacheData &val)
		{
			tjs_uint32 v = tTJSHashFunc<ttstr>::Make(val.Script);
			v ^= val.ExpressionMode?1:0;
			v ^= val.MustReturnResult?1:0;
			return v;
		}
	};

	tTJS *Owner;

	typedef tTJSRefHolder<tTJSScriptBlock> tScriptBlockHolder;

	typedef tTJSHashCache<tScriptCacheData, tScriptBlockHolder, tScriptCacheHashFunc>
		tCache;

	tCache Cache;

public:
	tTJSScriptCache(tTJS *owner);
	virtual ~tTJSScriptCache();


public:
	void ExecScript(const tjs_char *script, tTJSVariant *result,
		iTJSDispatch2 * context,
		const tjs_char *name, tjs_int lineofs);

	void ExecScript(const ttstr &script, tTJSVariant *result,
		iTJSDispatch2 * context,
		const ttstr *name, tjs_int lineofs);


public:

	void EvalExpression(const tjs_char *expression, tTJSVariant *result,
		iTJSDispatch2 * context,
		const tjs_char *name, tjs_int lineofs);

	void EvalExpression(const ttstr &expression, tTJSVariant *result,
		iTJSDispatch2 * context,
		const ttstr *name, tjs_int lineofs);

	// for Bytecode
	void LoadByteCode( const tjs_uint8* buff, size_t len, tTJSVariant *result,
		iTJSDispatch2 *context, const tjs_char *name );
};
//---------------------------------------------------------------------------

}
#endif


