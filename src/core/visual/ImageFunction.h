
#ifndef ImageFunctionH
#define ImageFunctionH

#include "tjsNative.h"
#include "LayerIntf.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapImpl.h"
#include "ComplexRect.h"

class tTJSNI_ImageFunction : public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;

public:
	tTJSNI_ImageFunction();
	tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();
};


class tTJSNC_ImageFunction : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;

public:
	tTJSNC_ImageFunction();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();

	static tTVPBBBltMethod GetBltMethodFromOperationMode( tTVPBlendOperationMode mode, tTVPDrawFace face );
	static bool ClipDestPointAndSrcRect(tjs_int &dx, tjs_int &dy, tTVPRect &srcrectout, const tTVPRect &srcrect, const tTVPRect &clipRect);
};

extern tTJSNativeClass * TVPCreateNativeClass_ImageFunction();
#endif // ImageFunctionH
