//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Text Editor
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "PadIntf.h"
//#include "PadImpl.h"

//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSNI_BasePad::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *dsp)
{
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD
tTJSNI_BasePad::Invalidate()
{
}
//---------------------------------------------------------------------------

tTJSNativeInstance *tTJSNC_Pad::CreateNativeInstance()
{
	return new tTJSNativeInstance();
}


//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Pad::ClassID = -1;
//---------------------------------------------------------------------------
tTJSNC_Pad::tTJSNC_Pad(): inherited(TJS_W("Pad"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(Pad) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_Pad,
	/*TJS class name*/Pad)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Pad)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
//----------------------------------------------------------------------

//--properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(text)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TJS_W("");
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(text)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fileName)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TJS_W("");
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fileName)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(color)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(color)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(visible)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(visible)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(title)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TJS_W("");
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(title)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fontColor)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fontColor)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fontHeight)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)10;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fontHeight)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fontSize)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)10;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fontSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fontBold)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fontBold)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fontItalic)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fontItalic)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fontUnderline)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fontUnderline)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fontStrikeOut)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fontStrikeOut)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(fontFace)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TJS_W("");
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(fontFace)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(readOnly)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = (tjs_int)1;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(readOnly)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(wordWrap)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = false;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(wordWrap)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(opacity)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = 255;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(opacity)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(showStatusBar)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = false;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(showStatusBar)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(showScrollBars)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = false;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(showScrollBars)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(statusText)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TJS_W("");
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(statusText)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(borderStyle)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = 0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(borderStyle)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(width)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = 100;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(width)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(height)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = 100;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(height)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(top)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = 0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(top)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(left)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = 0;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(left)
//----------------------------------------------------------------------



	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------



tTJSNativeClass * TVPCreateNativeClass_Pad()
{
	return new tTJSNC_Pad();
}
