//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// return values as tjs_error
//---------------------------------------------------------------------------

#ifndef tjsErrorDefsH
#define tjsErrorDefsH

namespace TJS
{

// #define TJS_STRICT_ERROR_CODE_CHECK
	// defining this enables strict error code checking,
	// for debugging.

/*[*/
//---------------------------------------------------------------------------
// return values as tjs_error
//---------------------------------------------------------------------------
#define TJS_E_MEMBERNOTFOUND		(-1001)
#define TJS_E_NOTIMPL				(-1002)
#define TJS_E_INVALIDPARAM			(-1003)
#define TJS_E_BADPARAMCOUNT			(-1004)
#define TJS_E_INVALIDTYPE			(-1005)
#define TJS_E_INVALIDOBJECT			(-1006)
#define TJS_E_ACCESSDENYED			(-1007)
#define TJS_E_NATIVECLASSCRASH		(-1008)

#define TJS_S_TRUE					(1)
#define TJS_S_FALSE					(2)

#define TJS_S_OK                    (0)
#define TJS_E_FAIL					(-1)

#define TJS_S_MAX (2)
	// maximum possible number of success status.
	// numbers over this may be regarded as a failure in
	// strict-checking mode.

#ifdef TJS_STRICT_ERROR_CODE_CHECK
	static inline bool TJS_FAILED(tjs_error hr)
	{
		if(hr < 0) return true;
		if(hr > TJS_S_MAX) return true;
		return false;
	}
#else
	#define TJS_FAILED(x)				((x)<0)
#endif
#define TJS_SUCCEEDED(x)			(!TJS_FAILED(x))

static inline bool TJSIsObjectValid(tjs_error hr)
{
	// checks object validity by returning value of iTJSDispatch2::IsValid

	if(hr == TJS_S_TRUE) return true;  // mostly expected value for valid object
	if(hr == TJS_E_NOTIMPL) return true; // also valid for object which does not implement IsValid

	return false; // otherwise the object is not valid
}

/*]*/

} // namespace TJS

#endif
