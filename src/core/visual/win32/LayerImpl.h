//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Layer Management
//---------------------------------------------------------------------------
#ifndef LayerImplH
#define LayerImplH


#include "LayerIntf.h"
//---------------------------------------------------------------------------
// tTJSNI_Layer
//---------------------------------------------------------------------------
class tTJSNI_Layer : public tTJSNI_BaseLayer
{
public:
	tTJSNI_Layer(void);
	~tTJSNI_Layer();
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();
	static tTJSNI_Layer* FromVariant(const tTJSVariant& var);
	static tTJSNI_Layer* FromObject(iTJSDispatch2* obj);
	//HRGN CreateMaskRgn(tjs_uint threshold);
};
//---------------------------------------------------------------------------
#endif
