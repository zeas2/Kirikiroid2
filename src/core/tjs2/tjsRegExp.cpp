//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Regular Expression Support
//---------------------------------------------------------------------------
#ifndef TJS_NO_REGEXP
#include "tjsCommHead.h"

#include "tjsRegExp.h"
#include "tjsArray.h"

#include <functional>


namespace TJS
{

//---------------------------------------------------------------------------
// Flags
//---------------------------------------------------------------------------
// some TJS flags
const tjs_uint32 globalsearch = (tjs_uint32)(((tjs_uint32)1)<<31);
const tjs_uint32 tjsflagsmask = (tjs_uint32)0xff000000;

//---------------------------------------------------------------------------
static tjs_uint32 TJSRegExpFlagToValue(tjs_char ch, tjs_uint32 prev)
{
	// converts flag letter to internal flag value.
	// this returns modified prev.
	// when ch is '\0', returns default flag value and prev is ignored.

	if(ch == 0) {
		return ONIG_OPTION_DEFAULT|ONIG_OPTION_CAPTURE_GROUP|ONIG_OPTION_FIND_NOT_EMPTY;
	}

	switch(ch)
	{
	case TJS_W('g'): // global search
		prev|=globalsearch; return prev;
	case TJS_W('i'): // ignore case
		prev|=ONIG_OPTION_IGNORECASE; return prev;
	case TJS_W('l'): // use localized collation
		/*prev &= ~regbase::nocollate;*/ return prev; // ignore
	default:
		return prev;
	}
}
//---------------------------------------------------------------------------
static tjs_uint32 TJSGetRegExpFlagsFromString(const tjs_char *string)
{
	// returns a flag value represented by string

	tjs_uint32 flag = TJSRegExpFlagToValue(0, 0);

	while(*string && *string != TJS_W('/'))
	{
		flag = TJSRegExpFlagToValue(*string, flag);
		string++;
	}

	return flag;
}
//---------------------------------------------------------------------------
void replace_regex( tTJSVariant **param, tjs_int numparams, tTJSNI_RegExp *_this, iTJSDispatch2 *objthis, ttstr &res ) {
	ttstr to;
	tTJSVariantClosure funcval;
	bool func;
	ttstr target = *param[0];
	if(param[1]->Type() != tvtObject) {
		to = (*param[1]);
		func = false;
	} else {
		funcval = param[1]->AsObjectClosureNoAddRef();
		if(funcval.ObjThis == NULL) {
			// replace with objthis when funcval's objthis is null
			funcval.ObjThis = objthis;
		}
		func = true;
	}
	// grep thru target string
	bool isreplaceall = (_this->Flags & globalsearch) != 0;
	OnigRegion* region = onig_region_new();
	const tjs_char* s = target.c_str();
	const tjs_char* send = s + target.GetLen();
	int r = onig_search( _this->RegEx, (UChar*)s, (UChar*)send, (UChar*)s, (UChar*)send, region, ONIG_OPTION_NONE );
	int offset = 0;
	if( r >= 0 ) { // match
		do {
			tjs_int pos = region->beg[0]/sizeof(tjs_char);
			tjs_int end = region->end[0]/sizeof(tjs_char);
			if( pos > 0 ) {
				res += ttstr(s,pos);
			}
			if( !func ) {
				res += to;
			} else {
				// call the callback function descripted as param[1]
				tTJSVariant result;
				tjs_error hr;
				iTJSDispatch2 *array = tTJSNC_RegExp::GetResultArray( true, s, _this, region );
				tTJSVariant arrayval(array, array);
				tTJSVariant *param = &arrayval;
				array->Release();
				hr = funcval.FuncCall(0, NULL, NULL, &result, 1, &param, NULL);
				if( TJS_FAILED(hr) ) {
					onig_region_free( region, 1  );
					return;
				}
				result.ToString();
				res += result.GetString();
			}
			s += end;
			onig_region_free( region, 0  );
		} while( isreplaceall && s < send && onig_search( _this->RegEx, (UChar*)s, (UChar*)send, (UChar*)s, (UChar*)send, region, ONIG_OPTION_NONE ) >= 0 );
		if( s < send ) {
			res += ttstr(s,(int)(send-s));
		}
	} else {
		res += ttstr(s,(int)(send-s));
	}
	onig_region_free( region, 1  );
}
//---------------------------------------------------------------------------
iTJSDispatch2* split_regex( const ttstr &target, iTJSDispatch2 * array, tTJSNI_RegExp *_this, bool purgeempty ) {
	tjs_uint targlen = target.GetLen();
	OnigRegion* region = onig_region_new();
	const tjs_char* s = target.c_str();
	const tjs_char* send = s + targlen;
	int r = onig_search( _this->RegEx, (UChar*)s, (UChar*)send, (UChar*)s, (UChar*)send, region, ONIG_OPTION_NONE );
	int storecount = 0;
	if( r >= 0 ) { // match
		do {
			int len = region->beg[0] / sizeof(tjs_char);
			if( !purgeempty || len > 0 ) {
				tTJSVariant val = ttstr( s, len );
				array->PropSetByNum(TJS_MEMBERENSURE, storecount++, &val, array);
			}
			s += region->end[0] / sizeof(tjs_char);
			onig_region_clear( region );
		} while( onig_search( _this->RegEx, (UChar*)s, (UChar*)send, (UChar*)s, (UChar*)send, region, ONIG_OPTION_NONE ) >= 0 );
		if( !purgeempty || s < send ) {
			tTJSVariant val = ttstr( s, (int)(send-s) );
			array->PropSetByNum(TJS_MEMBERENSURE, storecount++, &val, array);
		}
	} else {
		tTJSVariant val = ttstr( s, (int)(send-s) );
		array->PropSetByNum(TJS_MEMBERENSURE, storecount++, &val, array);
	}
	onig_region_clear( region );
	onig_region_free( region, 1  );
	return array;
}
//---------------------------------------------------------------------------
void TJSReleaseRegex()
{
	onig_end();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNI_RegExp : TJS Native Instance : RegExp
//---------------------------------------------------------------------------
tTJSNI_RegExp::tTJSNI_RegExp() : RegEx(NULL)
{
	// C++constructor
	Flags = TJSRegExpFlagToValue(0, 0);
	Start = 0;
	Index =0;
	LastIndex = 0;
}
//---------------------------------------------------------------------------
tTJSNI_RegExp::~tTJSNI_RegExp() {
	if( RegEx ) {
		onig_free(RegEx);
		RegEx = NULL;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_RegExp::Split(iTJSDispatch2 ** array, const ttstr &target, bool purgeempty)
{
	bool arrayallocated = false;
	if(!*array) *array = TJSCreateArrayObject(), arrayallocated = true;

	try {
		split_regex( target, *array, this, purgeempty );
	} catch(...) {
		if(arrayallocated) (*array)->Release();
		throw;
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNC_RegExp : TJS Native Class : RegExp
//---------------------------------------------------------------------------
tTJSVariant tTJSNC_RegExp::LastRegExp;
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_RegExp::ClassID = (tjs_uint32)-1;
tTJSNC_RegExp::tTJSNC_RegExp() :
	tTJSNativeClass(TJS_W("RegExp"))
{
	// class constructor

	TJS_BEGIN_NATIVE_MEMBERS(/*TJS class name*/RegExp)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var. name*/_this, /*var. type*/tTJSNI_RegExp,
	/*TJS class name*/ RegExp)
{
	/*
		TJS constructor
	*/

	if(numparams >= 1)
	{
		tTJSNC_RegExp::Compile(numparams, param, _this);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/RegExp)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/compile)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);

	/*
		compiles given regular expression and flags.
	*/

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSNC_RegExp::Compile(numparams, param, _this);


	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/compile)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/_compile)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);

	/*
		internal function; compiles given constant regular expression.
		input expression is following format:
		//flags/expression
		where flags is flag letters ( [gil] )
		and expression is a Regular Expression
	*/

	if(numparams != 1) return TJS_E_BADPARAMCOUNT;

	ttstr expr = *param[0];

	const tjs_char *p = expr.c_str();
	if(!p || !p[0]) return TJS_E_FAIL;

	if(p[0] != TJS_W('/') || p[1] != TJS_W('/')) return TJS_E_FAIL;

	p+=2;
	const tjs_char *exprstart = TJS_strchr(p, TJS_W('/'));
	if(!exprstart) return TJS_E_FAIL;
	exprstart ++;

	tjs_uint32 flags = TJSGetRegExpFlagsFromString(p);

	try
	{
		if( _this->RegEx ) {
			onig_free( _this->RegEx );
			_this->RegEx = NULL;
		}
		OnigErrorInfo einfo;
		int r = onig_new( &(_this->RegEx), (UChar*)exprstart, (UChar*)(expr.c_str()+expr.length()),
			flags&((ONIG_OPTION_MAXBIT<<1)-1), ONIG_ENCODING_UTF16_LE, ONIG_SYNTAX_PERL, &einfo );
		if( r ) {
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str( (UChar* )s, r, &einfo );
			TJS_eTJSError( s );
		}
	}
	catch(std::exception &e)
	{
		TJS_eTJSError(e.what());
	}

	_this->Flags = flags;

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/_compile)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/test)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);

	/*
		do the text searching.
		return match found ( true ), or not found ( false ).
		this function *changes* internal status.
	*/

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr target(*param[0]);
	OnigRegion* region = onig_region_new();
	bool matched = tTJSNC_RegExp::Exec( region, target, _this );
	onig_region_free(region, 1  );

	tTJSNC_RegExp::LastRegExp = tTJSVariant(objthis, objthis);

	if(result)
	{
		*result = matched;
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/test)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/match)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);

	/*
		do the text searching.
		this function is the same as test, except for its return value.
		match returns an array that contains each matching part.
		if match failed, returns empty array. eg.
		any internal status will not be changed.
	*/

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;


	if(result)
	{
		ttstr target(*param[0]);
		OnigRegion* region = onig_region_new();
		bool matched = tTJSNC_RegExp::Match( region, target, _this );
		iTJSDispatch2 *array = tTJSNC_RegExp::GetResultArray(matched, target, _this, region);
		onig_region_free(region, 1  );
		*result = tTJSVariant(array, array);
		array->Release();
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/match)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/exec)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);

	/*
		same as the match except for the internal status' change.
		var ar;
		var pat = /:(\d+):(\d+):/g;
		while((ar = pat.match(target)).count)
		{
			// ...
		}
	*/

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr target(*param[0]);
	OnigRegion* region = onig_region_new();
	tTJSNC_RegExp::Exec(region, target, _this);
	onig_region_free(region, 1  );

	tTJSNC_RegExp::LastRegExp = tTJSVariant(objthis, objthis);

	if(result)
	{
		*result = _this->Array;
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/exec)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/replace)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);

	/*
		replaces the string

		newstring = /regexp/.replace(orgstring, newsubstring);
		newsubstring can be:
			1. normal string ( literal or expression that respresents string )
			2. a function
		function is called as in RegExp's context, returns new substring.

		or

		newstring = string.replace(/regexp/, newsubstring);
			( via String.replace method )

		replace method ignores start property, and does not change any
			internal status.
	*/

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	ttstr res;
	replace_regex( param, numparams, _this, objthis, res );
	if(result) *result = res;

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/replace)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/split)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);

	/*
		replaces the string

		array = /regexp/.replace(targetstring, <reserved>, purgeempty);

		or

		array = targetstring.split(/regexp/, <reserved>, purgeempty);

		or

		array = [].split(/regexp/, targetstring, <reserved>, purgeempty);

		this method does not update properties
	*/

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr target(*param[0]);

	bool purgeempty = false;
	if(numparams >= 3) purgeempty = param[2]->operator bool();

	iTJSDispatch2 *array = NULL;

	_this->Split(&array, target, purgeempty);

	if(result) *result = tTJSVariant(array, array);

	array->Release();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/split)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(matches)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = _this->Array;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(matches)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(start)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = (tTVInteger)_this->Start;
		return TJS_S_OK;
	}
    TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/* var. name */_this, /* var. type */tTJSNI_RegExp);
		_this->Start = (tjs_uint)(tTVInteger)*param;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(start)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(index)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = (tTVInteger)_this->Index;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(index)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(lastIndex)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = (tTVInteger)_this->LastIndex;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(lastIndex)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(input)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = _this->Input;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(input)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(lastMatch)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = _this->LastMatch;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(lastMatch)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(lastParen)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = _this->LastParen;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(lastParen)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(leftContext)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = _this->LeftContext;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(leftContext)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(rightContext)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_RegExp);
		*result = _this->RightContext;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(rightContext)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(last)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = tTJSNC_RegExp::LastRegExp;;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(last)
//---------------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_RegExp::CreateNativeInstance()
{
	return new tTJSNI_RegExp();
}
//---------------------------------------------------------------------------
void tTJSNC_RegExp::Compile(tjs_int numparams, tTJSVariant **param, tTJSNI_RegExp *_this)
{
	ttstr expr = *param[0];

	tjs_uint32 flags;
	if( numparams >= 2 ) {
		ttstr fs = *param[1];
		flags = TJSGetRegExpFlagsFromString(fs.c_str());
	} else {
		flags = TJSRegExpFlagToValue(0, 0);
	}

	if(expr.IsEmpty()) expr = TJS_W("(?:)"); // generate empty regular expression

	if( _this->RegEx ) {
		onig_free( _this->RegEx );
		_this->RegEx = NULL;
	}
	OnigErrorInfo einfo;
	int r = onig_new( &(_this->RegEx), (UChar*)expr.c_str(), (UChar*)(expr.c_str()+expr.length()),
		flags&((ONIG_OPTION_MAXBIT<<1)-1), ONIG_ENCODING_UTF16_LE, ONIG_SYNTAX_PERL, &einfo );
	if( r ) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str( (UChar* )s, r, &einfo );
		TJS_eTJSError( s );
	}
	_this->Flags = flags;
}
//---------------------------------------------------------------------------
bool tTJSNC_RegExp::Match(OnigRegion* region, const ttstr &target, tTJSNI_RegExp *_this)
{
	tjs_uint searchstart;
	tjs_uint targlen = target.GetLen();
	if( _this->Start == targlen ) {
		// Start already reached at end
		return false;  // returns true if empty
	} else if(_this->Start > targlen) {
		// Start exceeds target's length
		return false;
	}
	searchstart = _this->Start;
	int r = onig_search( _this->RegEx, (UChar* )(target.c_str()+searchstart), (UChar* )(target.c_str()+targlen),
		  (UChar* )(target.c_str()+searchstart), (UChar* )(target.c_str()+targlen),
		  region, ONIG_OPTION_NONE );
	return r >= 0;
}
//---------------------------------------------------------------------------
bool tTJSNC_RegExp::Exec(OnigRegion* region, const ttstr &target, tTJSNI_RegExp *_this)
{
	bool matched = tTJSNC_RegExp::Match( region, target, _this);
	iTJSDispatch2 *array = tTJSNC_RegExp::GetResultArray(matched, target.c_str() + _this->Start, _this, region );
	_this->Array = tTJSVariant(array, array);
	array->Release();

	_this->Input = target;
	if( !matched || region->num_regs <= 0 ) {
		_this->Index = _this->Start;
		_this->LastIndex = _this->Start;
		_this->LastMatch = ttstr();
		_this->LastParen = ttstr();
		_this->LeftContext = ttstr(target, _this->Start);
	} else {
		int num_regs = region->num_regs;
		_this->Index = _this->Start + region->beg[0];
		int lastindex = 0;
		for( int i = 0; i < num_regs; i++ ) {
			if( lastindex < region->end[i] ) {
				lastindex = region->end[i];
			}
		}
		_this->LastIndex = _this->Start + lastindex/sizeof(tjs_char);
		_this->LastMatch = ttstr( target.c_str()+(region->beg[0]/sizeof(tjs_char)), (region->end[0] - region->beg[0])/sizeof(tjs_char));
		tjs_uint last = num_regs-1;
		_this->LastParen = ttstr( target.c_str()+(region->beg[last]/sizeof(tjs_char)), (region->end[last] - region->beg[last])/sizeof(tjs_char));
		_this->LeftContext = ttstr(target, _this->Start + (region->beg[0])/sizeof(tjs_char));
		_this->RightContext = ttstr(target.c_str() + _this->LastIndex);
		if(_this->Flags & globalsearch)
		{
			// global search flag changes the next search starting position.
			tjs_uint match_end = _this->LastIndex;
			_this->Start = match_end;
		}
	}
	return matched;
}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJSNC_RegExp::GetResultArray( bool matched, const tjs_char *target, tTJSNI_RegExp *_this, const OnigRegion* region ) {
	iTJSDispatch2 *array = TJSCreateArrayObject();
	if( matched ) {
		//if(_this->RegEx->.empty()) {
		if( region->num_regs <= 0 ) {
			tTJSVariant val(TJS_W(""));
			array->PropSetByNum(TJS_MEMBERENSURE|TJS_IGNOREPROP, 0, &val, array);
		} else {
			tjs_uint size = region->num_regs;
			try {
				for( tjs_uint i = 0; i < size; i++ ) {
					tTJSVariant val;
					val = ttstr( target+(region->beg[i]/sizeof(tjs_char)), (region->end[i] - region->beg[i])/sizeof(tjs_char) );
					array->PropSetByNum(TJS_MEMBERENSURE|TJS_IGNOREPROP, i, &val, array);
				}
			} catch(...) {
				array->Release();
				throw;
			}
		}
	}
	return array;
}
//---------------------------------------------------------------------------
iTJSDispatch2 * TJSCreateRegExpClass()
{
	return new tTJSNC_RegExp();
}
//---------------------------------------------------------------------------

} // namespace TJS

#endif