//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Regular Expression Support
//---------------------------------------------------------------------------
#ifndef tjsRegExpH
#define tjsRegExpH

#ifndef TJS_NO_REGEXP
#define ONIG_EXTERN extern
#include "oniguruma.h"
#include "tjsNative.h"

namespace TJS
{

//---------------------------------------------------------------------------
// tTJSNI_RegExp
//---------------------------------------------------------------------------
class tTJSNI_RegExp : public tTJSNativeInstance
{
public:
	regex_t* RegEx;
	//OnigRegion* Region;
	tjs_uint32 Flags;
	tjs_uint Start;
	tTJSVariant Array;
	tjs_uint Index;
	ttstr Input;
	tjs_uint LastIndex;
	ttstr LastMatch;
	ttstr LastParen;
	ttstr LeftContext;
	ttstr RightContext;
private:

public:
	tTJSNI_RegExp();
	~tTJSNI_RegExp();
	void Split(iTJSDispatch2 ** array, const ttstr &target, bool purgeempty);
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNC_RegExp
//---------------------------------------------------------------------------
class tTJSNC_RegExp : public tTJSNativeClass
{
public:
	tTJSNC_RegExp();

	static void Compile(tjs_int numparam, tTJSVariant **param, tTJSNI_RegExp *_this);
	static bool Match(OnigRegion* region, const ttstr &target, tTJSNI_RegExp *_this);
	static bool Exec(OnigRegion* region, const ttstr &target, tTJSNI_RegExp *_this);
	static iTJSDispatch2 * GetResultArray(bool matched, const tjs_char *target, tTJSNI_RegExp *_this, const OnigRegion* region );
	static iTJSDispatch2 * GetResultArray(bool matched, const ttstr &target, tTJSNI_RegExp *_this, const OnigRegion* region ) {
		return GetResultArray(matched, target.c_str(), _this, region);
	}

private:
	tTJSNativeInstance *CreateNativeInstance();

public:
	static tjs_uint32 ClassID;

	static tTJSVariant LastRegExp;
};
//---------------------------------------------------------------------------
extern iTJSDispatch2 * TJSCreateRegExpClass();
extern void TJSReleaseRegex();
//---------------------------------------------------------------------------

} // namespace TJS
#endif
#endif
