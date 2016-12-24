//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Date class implementation
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include "tjsError.h"
#include "tjsDate.h"
#include "tjsDateParser.h"

/*
note:
	To ensure portability, TJS uses time_t as a date/time representation.
	if compiler's time_t holds only 32bits, it will cause the year 2038 problem.
	The author assumes that it is a compiler dependented problem, so any remedies
	are not given here.
*/

namespace TJS
{
//---------------------------------------------------------------------------
static time_t TJSParseDateString(const tjs_char *str)
{
	tTJSDateParser parser(str);
	return (time_t)(parser.GetTime() / 1000);
}
//---------------------------------------------------------------------------
// tTJSNI_Date : TJS Native Instance : Date
//---------------------------------------------------------------------------
tTJSNI_Date::tTJSNI_Date()
{
	// C++ constructor
}
//---------------------------------------------------------------------------
// tTJSNC_Date : TJS Native Class : Date
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Date::ClassID = (tjs_uint32)-1;
tTJSNC_Date::tTJSNC_Date() :
	tTJSNativeClass(TJS_W("Date"))
{
	// class constructor

	TJS_BEGIN_NATIVE_MEMBERS(/*TJS class name*/Date)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var. name*/_this, /*var. type*/tTJSNI_Date,
	/*TJS class name*/ Date)
{
	if(numparams == 0)
	{
		time_t curtime;
		_this->DateTime = time(&curtime); // GMT current date/time
	}
	else if(numparams >= 1)
	{
		if(param[0]->Type() == tvtString)
		{
			// formatted string -> date/time
			_this->DateTime = TJSParseDateString(param[0]->GetString());
		}
		else
		{
			tjs_int y, mon=0, day=1, h=0, m=0, s=0;
			y = (tjs_int)param[0]->AsInteger();
			if(TJS_PARAM_EXIST(1)) mon = (tjs_int)param[1]->AsInteger();
			if(TJS_PARAM_EXIST(2)) day = (tjs_int)param[2]->AsInteger();
			if(TJS_PARAM_EXIST(3)) h = (tjs_int)param[3]->AsInteger();
			if(TJS_PARAM_EXIST(4)) m = (tjs_int)param[4]->AsInteger();
			if(TJS_PARAM_EXIST(5)) s = (tjs_int)param[5]->AsInteger();
			tm t;
			memset(&t, 0, sizeof(tm));
			t.tm_year = y - 1900;
			t.tm_mon = mon;
			t.tm_mday = day;
			t.tm_hour = h;
			t.tm_min = m;
			t.tm_sec = s;
			_this->DateTime = mktime(&t);
			if(_this->DateTime == -1) TJS_eTJSError(TJSInvalidValueForTimestamp);
//			_this->DateTime -= TJS_timezone;
		}
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Date)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setYear)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tm *te = localtime(&_this->DateTime);
	tm t;
	memcpy(&t, te, sizeof(tm));
	t.tm_year = (tjs_int)param[0]->AsInteger() - 1900;
	_this->DateTime = mktime(&t);
	if(_this->DateTime == -1) TJS_eTJSError(TJSInvalidValueForTimestamp);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setYear)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setMonth)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tm *te = localtime(&_this->DateTime);
	tm t;
	memcpy(&t, te, sizeof(tm));
	t.tm_mon = (tjs_int)param[0]->AsInteger();
	_this->DateTime = mktime(&t);
	if(_this->DateTime == -1) TJS_eTJSError(TJSInvalidValueForTimestamp);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setMonth)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setDate)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tm *te = localtime(&_this->DateTime);
	tm t;
	memcpy(&t, te, sizeof(tm));
	t.tm_mday = (tjs_int)param[0]->AsInteger();
	_this->DateTime = mktime(&t);
	if(_this->DateTime == -1) TJS_eTJSError(TJSInvalidValueForTimestamp);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setDate)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setHours)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tm *te = localtime(&_this->DateTime);
	tm t;
	memcpy(&t, te, sizeof(tm));
	t.tm_hour = (tjs_int)param[0]->AsInteger();
	_this->DateTime = mktime(&t) ;
	if(_this->DateTime == -1) TJS_eTJSError(TJSInvalidValueForTimestamp);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setHours)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setMinutes)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tm *te = localtime(&_this->DateTime);
	tm t;
	memcpy(&t, te, sizeof(tm));
	t.tm_min = (tjs_int)param[0]->AsInteger();
	_this->DateTime = mktime(&t);
	if(_this->DateTime == -1) TJS_eTJSError(TJSInvalidValueForTimestamp);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setMinutes)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setSeconds)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tm *te = localtime(&_this->DateTime);
	tm t;
	memcpy(&t, te, sizeof(tm));
	t.tm_sec = (tjs_int)param[0]->AsInteger();
	_this->DateTime = mktime(&t);
	if(_this->DateTime == -1) TJS_eTJSError(TJSInvalidValueForTimestamp);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setSeconds)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setTime)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	_this->DateTime = (time_t)(param[0]->AsInteger()/1000L);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setTime)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getDate)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	tm *t = localtime(&_this->DateTime);

	if(result) result->CopyRef(tTJSVariant(t->tm_mday));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getDate)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getDay)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	tm *t = localtime(&_this->DateTime);

	if(result) result->CopyRef(tTJSVariant(t->tm_wday));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getDay)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getHours)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	tm *t = localtime(&_this->DateTime);

	if(result) result->CopyRef(tTJSVariant(t->tm_hour));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getHours)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getMinutes)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	tm *t = localtime(&_this->DateTime);

	if(result) result->CopyRef(tTJSVariant(t->tm_min));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getMinutes)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getMonth)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	tm *t = localtime(&_this->DateTime);

	if(result) result->CopyRef(tTJSVariant(t->tm_mon));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getMonth)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getSeconds)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	tm *t = localtime(&_this->DateTime);

	if(result) result->CopyRef(tTJSVariant(t->tm_sec));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getSeconds)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getTime)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(result) result->CopyRef(tTJSVariant(
			(tjs_int64)(_this->DateTime)*1000L));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getTime)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getTimezoneOffset) // static
{
	if(result) result->CopyRef(tTJSVariant((tjs_int)(TJS_timezone/60)));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getTimezoneOffset)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getYear)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	tm *t = localtime(&_this->DateTime);

	if(result) result->CopyRef(tTJSVariant(t->tm_year+1900));

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getYear)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/parse)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Date);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	_this->DateTime = TJSParseDateString(param[0]->GetString());

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/parse)
//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_Date::CreateNativeInstance()
{
	return new tTJSNI_Date(); 
}
//---------------------------------------------------------------------------
} // namespace TJS

//---------------------------------------------------------------------------

