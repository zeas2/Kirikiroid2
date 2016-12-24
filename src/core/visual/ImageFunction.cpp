
#include "tjsCommHead.h"
#include "ImageFunction.h"
#include "BitmapIntf.h"
#include "RectItf.h"

tTJSNI_ImageFunction::tTJSNI_ImageFunction() {}

tjs_error TJS_INTF_METHOD tTJSNI_ImageFunction::Construct(tjs_int numparams, tTJSVariant **param, iTJSDispatch2 *tjs_obj) {
	return TJS_S_OK;
}

void TJS_INTF_METHOD tTJSNI_ImageFunction::Invalidate() {}

tTVPBBBltMethod tTJSNC_ImageFunction::GetBltMethodFromOperationMode( tTVPBlendOperationMode mode, tTVPDrawFace face )
{
	tTVPBBBltMethod met = bmAlphaOnAlpha;
	switch(mode)
	{
	case omPsNormal:			met = bmPsNormal;			break;
	case omPsAdditive:			met = bmPsAdditive;			break;
	case omPsSubtractive:		met = bmPsSubtractive;		break;
	case omPsMultiplicative:	met = bmPsMultiplicative;	break;
	case omPsScreen:			met = bmPsScreen;			break;
	case omPsOverlay:			met = bmPsOverlay;			break;
	case omPsHardLight:			met = bmPsHardLight;		break;
	case omPsSoftLight:			met = bmPsSoftLight;		break;
	case omPsColorDodge:		met = bmPsColorDodge;		break;
	case omPsColorDodge5:		met = bmPsColorDodge5;		break;
	case omPsColorBurn:			met = bmPsColorBurn;		break;
	case omPsLighten:			met = bmPsLighten;			break;
	case omPsDarken:			met = bmPsDarken;			break;
	case omPsDifference:   		met = bmPsDifference;		break;
	case omPsDifference5:   	met = bmPsDifference5;		break;
	case omPsExclusion:			met = bmPsExclusion;		break;
	case omAdditive:			met = bmAdd;				break;
	case omSubtractive:			met = bmSub;				break;
	case omMultiplicative:		met = bmMul;				break;
	case omDodge:				met = bmDodge;				break;
	case omDarken:				met = bmDarken;				break;
	case omLighten:				met = bmLighten;			break;
	case omScreen:				met = bmScreen;				break;
	case omAlpha:
		if(face == dfAlpha)
						{	met = bmAlphaOnAlpha; break;		}
		else if(face == dfAddAlpha)
						{	met = bmAlphaOnAddAlpha; break;		}
		else if(face == dfOpaque)
						{	met = bmAlpha; break;				}
		break;
	case omAddAlpha:
		if(face == dfAlpha)
						{	met = bmAddAlphaOnAlpha; break;		}
		else if(face == dfAddAlpha)
						{	met = bmAddAlphaOnAddAlpha; break;	}
		else if(face == dfOpaque)
						{	met = bmAddAlpha; break;			}
		break;
	case omOpaque:
		if(face == dfAlpha)
						{	met = bmCopyOnAlpha; break;			}
		else if(face == dfAddAlpha)
						{	met = bmCopyOnAddAlpha; break;		}
		else if(face == dfOpaque)
						{	met = bmCopy; break;				}
		break;
	case omAuto:
		break;
	}
	return met;
}
bool tTJSNC_ImageFunction::ClipDestPointAndSrcRect(tjs_int &dx, tjs_int &dy, tTVPRect &srcrectout, const tTVPRect &srcrect, const tTVPRect &clipRect)
{
	// clip (dx, dy) <- srcrect	with current clipping rectangle
	srcrectout = srcrect;
	tjs_int dr = dx + srcrect.right - srcrect.left;
	tjs_int db = dy + srcrect.bottom - srcrect.top;

	if(dx < clipRect.left)
	{
		srcrectout.left += (clipRect.left - dx);
		dx = clipRect.left;
	}

	if(dr > clipRect.right)
	{
		srcrectout.right -= (dr - clipRect.right);
	}

	if(srcrectout.right <= srcrectout.left) return false; // out of the clipping rect

	if(dy < clipRect.top)
	{
		srcrectout.top += (clipRect.top - dy);
		dy = clipRect.top;
	}

	if(db > clipRect.bottom)
	{
		srcrectout.bottom -= (db - clipRect.bottom);
	}

	if(srcrectout.bottom <= srcrectout.top) return false; // out of the clipping rect

	return true;
}


tjs_uint32 tTJSNC_ImageFunction::ClassID = -1;

tTJSNC_ImageFunction::tTJSNC_ImageFunction() : inherited(TJS_W("ImageFunction") )  {

	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(ImageFunction) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_ImageFunction,
	/*TJS class name*/ImageFunction)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/ImageFunction)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/operateAffine)
{
	// dst, src, x0/a, y0/b, x1/c, y1/d, x2/tx, y2/ty,
	// srcrect=null, cliprect=null, affine=true,
	// mode=omAlpha, face=dfAlpha, opa=255, type=0, hda=false
	if(numparams < 11) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	tTJSNI_Bitmap * src = NULL;
	clo = param[1]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&src)))
			return TJS_E_INVALIDPARAM;
	}
	if( (!dst) || (!src) ) return TJS_E_INVALIDPARAM;

	tTVPRect srcRect( 0, 0, src->GetWidth(), src->GetHeight() );
	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 9 && param[8]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[8]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			srcRect = rect->Get();
		}
	}
	if(numparams >= 10 && param[9]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[9]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}
	bool affine = true;
	if(numparams >= 11 && param[10]->Type() != tvtVoid)
		affine = param[10]->operator bool();

	tTVPBlendOperationMode mode = omAlpha;
	if(numparams >= 12 && param[11]->Type() != tvtVoid)
		mode = (tTVPBlendOperationMode)(tjs_int)(*param[11]);
	if( mode == omAuto ) mode = omAlpha;

	tTVPDrawFace face = dfAlpha;
	if(numparams >= 13 && param[12]->Type() != tvtVoid)
		face = (tTVPDrawFace)(tjs_int)(*param[12]);
	if( face != dfAlpha && face != dfAddAlpha && face != dfOpaque ) face = dfAlpha;

	tjs_int opa = 255;
	if(numparams >= 14 && param[13]->Type() != tvtVoid)
		opa = (tjs_int)*param[13];

	tTVPBBStretchType type = stNearest;
	if(numparams >= 15 && param[14]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[14];

	bool hda = false;
	if(numparams >= 16 && param[15]->Type() != tvtVoid)
		hda = ((tjs_int)*param[15]) ? true : false;

	tTVPBBBltMethod met = GetBltMethodFromOperationMode( mode, face );
	tTVPRect updaterect; // ignore
	bool updated;
	if( affine ) {
		// affine matrix mode
		t2DAffineMatrix mat;
		mat.a = *param[2];
		mat.b = *param[3];
		mat.c = *param[4];
		mat.d = *param[5];
		mat.tx = *param[6];
		mat.ty = *param[7];
		updated = dst->GetBitmap()->AffineBlt(clipRect, src->GetBitmap(), srcRect, mat, met, opa, &updaterect, hda, type);
	} else {
		// points mode
		tTVPPointD points[3];
		points[0].x = *param[2];
		points[0].y = *param[3];
		points[1].x = *param[4];
		points[1].y = *param[5];
		points[2].x = *param[6];
		points[2].y = *param[7];
		updated = dst->GetBitmap()->AffineBlt(clipRect, src->GetBitmap(), srcRect, points, met, opa, &updaterect, hda, type);
	}
	if( result ) {
		if( updated ) {
			iTJSDispatch2 *ret = TVPCreateRectObject( updaterect.left, updaterect.top, updaterect.right, updaterect.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/operateAffine)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/operateRect)
{
	// dst, dleft, dtop, src, srect=null, cliprect=null
	// mode=omAlpha, face=dfAlpha, opa=255, hda=false
	if(numparams < 6) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	tTJSNI_Bitmap * src = NULL;
	clo = param[3]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&src)))
			return TJS_E_INVALIDPARAM;
	}
	if( (!dst) || (!src) ) return TJS_E_INVALIDPARAM;

	tTVPRect srcRect( 0, 0, src->GetWidth(), src->GetHeight() );
	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 5 && param[4]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[4]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			srcRect = rect->Get();
		}
	}
	if(numparams >= 6 && param[5]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[5]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}
	tjs_int dleft = *param[1];
	tjs_int dtop = *param[2];

	tTVPBlendOperationMode mode = omAlpha;
	if(numparams >= 7 && param[6]->Type() != tvtVoid)
		mode = (tTVPBlendOperationMode)(tjs_int)(*param[6]);
	
	if( mode == omAuto ) mode = omAlpha;

	tTVPDrawFace face = dfAlpha;
	if(numparams >= 8 && param[7]->Type() != tvtVoid)
		face = (tTVPDrawFace)(tjs_int)(*param[7]);

	if( face != dfAlpha && face != dfAddAlpha && face != dfOpaque ) face = dfAlpha;

	tjs_int opa = 255;
	if(numparams >= 9 && param[8]->Type() != tvtVoid)
		opa = *param[8];

	bool hda = false;
	if(numparams >= 10 && param[9]->Type() != tvtVoid)
		hda = ((tjs_int)*param[9]) ? true : false;

	tTVPRect rect;
	bool updated = false;
	if( ClipDestPointAndSrcRect( dleft, dtop, rect, srcRect, clipRect ) ) {
		tTVPBBBltMethod met = GetBltMethodFromOperationMode( mode, face );
		updated = dst->GetBitmap()->Blt( dleft, dtop, src->GetBitmap(), rect, met, opa, hda );
	}
	if( result ) {
		if( updated ) {
			tTVPRect ur = rect;
			ur.set_offsets(dleft, dtop);
			iTJSDispatch2 *ret = TVPCreateRectObject( ur.left, ur.top, ur.right, ur.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/operateRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/operateStretch)
{
	// dst, src, dstrect=null, srcrect=null, cliprect=null
	// mode=omAlpha, face=dfAlpha, opa=255, type=0, hda=false
	// dfAlpha,dfAddAlpha,dfOpaque

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	tTJSNI_Bitmap * src = NULL;
	clo = param[1]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&src)))
			return TJS_E_INVALIDPARAM;
	}
	if( (!dst) || (!src) ) return TJS_E_INVALIDPARAM;

	tTVPRect dstRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	tTVPRect srcRect( 0, 0, src->GetWidth(), src->GetHeight() );
	tTVPRect clipRect( dstRect );

	if(numparams >= 3 && param[2]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[2]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			dstRect = rect->Get();
		}
	}
	if(numparams >= 4 && param[3]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[3]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			srcRect = rect->Get();
		}
	}
	if(numparams >= 5 && param[4]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[4]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}

	tTVPBlendOperationMode mode = omAlpha;
	if(numparams >= 6 && param[5]->Type() != tvtVoid)
		mode = (tTVPBlendOperationMode)(tjs_int)(*param[5]);

	if( mode == omAuto ) mode = omAlpha;

	tTVPDrawFace face = dfAlpha;
	if(numparams >= 7 && param[6]->Type() != tvtVoid)
		face = (tTVPDrawFace)(tjs_int)(*param[6]);

	if( face != dfAlpha && face != dfAddAlpha && face != dfOpaque ) face = dfAlpha;

	tjs_int opa = 255;
	if(numparams >= 8 && param[7]->Type() != tvtVoid)
		opa = *param[7];

	tTVPBBStretchType type = stNearest;
	if(numparams >= 9 && param[8]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[8];

	bool hda = false;
	if(numparams >= 10 && param[9]->Type() != tvtVoid)
		hda = ((tjs_int)*param[9]) ? true : false;

	tjs_real typeopt = 0.0;
	if(numparams >= 11)
		typeopt = (tjs_real)*param[10];
	else if( type == stFastCubic || type == stCubic )
		typeopt = -1.0;

	bool updated = false;
	tTVPRect ur = dstRect;
	if(ur.right < ur.left) std::swap(ur.right, ur.left);
	if(ur.bottom < ur.top) std::swap(ur.bottom, ur.top);
	if( TVPIntersectRect(&ur, ur, clipRect) ) {
		tTVPBBBltMethod met = GetBltMethodFromOperationMode( mode, face );
		updated = dst->GetBitmap()->StretchBlt( clipRect, dstRect, src->GetBitmap(), srcRect, met, opa, hda, type, typeopt);
	}
	if( result ) {
		if( updated ) {
			iTJSDispatch2 *ret = TVPCreateRectObject( ur.left, ur.top, ur.right, ur.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/operateStretch)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/flipLR)
{
	// dst, rect
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tTVPRect dstRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 2 && param[1]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[1]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			dstRect = rect->Get();
		}
	}
	dst->GetBitmap()->LRFlip(dstRect);
	if( result ) {
		iTJSDispatch2 *ret = TVPCreateRectObject( dstRect.left, dstRect.top, dstRect.right, dstRect.bottom );
		*result = tTJSVariant(ret, ret);
		ret->Release();
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/flipLR)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/flipUD)
{
	// dst, rect
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tTVPRect dstRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 2 && param[1]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[1]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			dstRect = rect->Get();
		}
	}
	dst->GetBitmap()->UDFlip(dstRect);
	if( result ) {
		iTJSDispatch2 *ret = TVPCreateRectObject( dstRect.left, dstRect.top, dstRect.right, dstRect.bottom );
		*result = tTJSVariant(ret, ret);
		ret->Release();
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/flipUD)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/adjustGamma)
{
	// bmp, rgamma=1.0, rfloor=0, rceil=255, ggamma=1.0, gfloor=0, gceil=255, bgamma=1.0, bfloor=0, bceil=255, cliprect=null, isaddalpha=false
	// cliprect=null, isaddalpha=false
	if( numparams < 1 ) return TJS_E_BADPARAMCOUNT;
	if( numparams == 1 ) return TJS_S_OK;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tTVPGLGammaAdjustData data;
	memcpy(&data, &TVPIntactGammaAdjustData, sizeof(data));

	if(numparams >= 2 && param[1]->Type() != tvtVoid)
		data.RGamma = static_cast<float>((double)*param[1]);
	if(numparams >= 3 && param[2]->Type() != tvtVoid)
		data.RFloor = *param[2];
	if(numparams >= 4 && param[3]->Type() != tvtVoid)
		data.RCeil  = *param[3];
	if(numparams >= 5 && param[4]->Type() != tvtVoid)
		data.GGamma = static_cast<float>((double)*param[4]);
	if(numparams >= 6 && param[5]->Type() != tvtVoid)
		data.GFloor = *param[5];
	if(numparams >= 7 && param[6]->Type() != tvtVoid)
		data.GCeil  = *param[6];
	if(numparams >= 8 && param[7]->Type() != tvtVoid)
		data.BGamma = static_cast<float>((double)*param[7]);
	if(numparams >= 9 && param[8]->Type() != tvtVoid)
		data.BFloor = *param[8];
	if(numparams >= 10 && param[9]->Type() != tvtVoid)
		data.BCeil  = *param[9];

	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 11 && param[10]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[10]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}
	bool isaddalpha = false;
	if(numparams >= 12 && param[11]->Type() != tvtVoid)
		isaddalpha = ((tjs_int)*param[11]) ? true : false;
	
	if( isaddalpha ) {
		dst->GetBitmap()->AdjustGammaForAdditiveAlpha( clipRect, data);
	} else {
		dst->GetBitmap()->AdjustGamma( clipRect, data);
	}
	if( result ) {
		iTJSDispatch2 *ret = TVPCreateRectObject( clipRect.left, clipRect.top, clipRect.right, clipRect.bottom );
		*result = tTJSVariant(ret, ret);
		ret->Release();
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/adjustGamma)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/doBoxBlur)
{
	// bmp, xblur=1, yblur=1, clipRect=null, isalpha=true
	if( numparams < 1 ) return TJS_E_BADPARAMCOUNT;
	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tjs_int xblur = 1;
	tjs_int yblur = 1;

	if(numparams >= 2 && param[1]->Type() != tvtVoid)
		xblur = (tjs_int)*param[1];
	
	if(numparams >= 3 && param[2]->Type() != tvtVoid)
		yblur = (tjs_int)*param[2];

	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 4 && param[3]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[3]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}
	bool isalpha = true;
	if(numparams >= 5 && param[4]->Type() != tvtVoid)
		isalpha = ((tjs_int)*param[4]) ? true : false;

	bool updated = false;
	if( isalpha == false )
		updated = dst->GetBitmap()->DoBoxBlur(clipRect, tTVPRect(-xblur, -yblur, xblur, yblur));
	else
		updated = dst->GetBitmap()->DoBoxBlurForAlpha(clipRect, tTVPRect(-xblur, -yblur, xblur, yblur));

	if( result ) {
		if( updated ) {
			iTJSDispatch2 *ret = TVPCreateRectObject( clipRect.left, clipRect.top, clipRect.right, clipRect.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/doBoxBlur)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/doGrayScale)
{
	// bmp, clipRect=null
	if( numparams < 1 ) return TJS_E_BADPARAMCOUNT;
	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 2 && param[1]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[1]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}

	dst->GetBitmap()->DoGrayScale(clipRect);
	if( result ) {
		iTJSDispatch2 *ret = TVPCreateRectObject( clipRect.left, clipRect.top, clipRect.right, clipRect.bottom );
		*result = tTJSVariant(ret, ret);
		ret->Release();
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/doGrayScale)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fillRect)
{
	// bmp, value, rect=null, isalpha=true, cliprect=null
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tjs_uint32 color = static_cast<tjs_uint32>((tjs_int64)*param[1]);

	tTVPRect rect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 3 && param[2]->Type() == tvtObject ) {
		tTJSNI_Rect * rectni = NULL;
		clo = param[2]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rectni)))
				return TJS_E_INVALIDPARAM;
			rect = rectni->Get();
		}
	}

	bool isalpha = true;
	if(numparams >= 4 && param[3]->Type() != tvtVoid)
		isalpha = ((tjs_int)*param[3]) ? true : false;

	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 5 && param[4]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[4]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}

	bool updated = false;
	tTVPRect destrect;
	if( TVPIntersectRect(&destrect, rect, clipRect) ) {
		if( isalpha ) {
			color = (color & 0xff000000) + (TVPToActualColor(color&0xffffff)&0xffffff);
			updated = dst->GetBitmap()->Fill( destrect, color );
		} else {
			color = TVPToActualColor(color);
			updated = dst->GetBitmap()->FillColor(destrect, color, 255);
		}
	}
	if( result ) {
		if( updated ) {
			iTJSDispatch2 *ret = TVPCreateRectObject( destrect.left, destrect.top, destrect.right, destrect.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/fillRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/colorRect)
{
	// bmp, value, opa=255, rect=null, face=dfAlpha, cliprect=null
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tjs_uint32 color = static_cast<tjs_uint32>((tjs_int64)*param[1]);

	tjs_int opa = 255;
	if(numparams >= 3 && param[2]->Type() != tvtVoid)
		opa = *param[2];

	tTVPRect rect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 4 && param[3]->Type() == tvtObject ) {
		tTJSNI_Rect * rectni = NULL;
		clo = param[3]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rectni)))
				return TJS_E_INVALIDPARAM;
			rect = rectni->Get();
		}
	}

	tTVPDrawFace face = dfAlpha;
	if(numparams >= 5 && param[4]->Type() != tvtVoid)
		face = (tTVPDrawFace)(tjs_int)(*param[4]);
	
	if( face != dfAlpha && face != dfAddAlpha && face != dfOpaque && face != dfMask ) face = dfAlpha;

	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 6 && param[5]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[5]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}
	bool updated = false;
	tTVPRect destrect;
	if( TVPIntersectRect(&destrect, rect, clipRect) ) {
		switch( face ) {
		case dfAlpha:
			if( opa > 0 ) {
				color = TVPToActualColor(color);
				updated = dst->GetBitmap()->FillColorOnAlpha(destrect, color, opa);
			} else {
				updated = dst->GetBitmap()->RemoveConstOpacity(destrect, -opa);
			}
			break;
		case dfAddAlpha:
			if( opa >= 0 ) {
				color = TVPToActualColor(color);
				updated = dst->GetBitmap()->FillColorOnAddAlpha(destrect, color, opa);
			}
			break;
		case dfOpaque:
			color = TVPToActualColor(color);
			updated = dst->GetBitmap()->FillColor(destrect, color, opa);
			// note that tTVPBaseBitmap::FillColor always holds destination alpha
			break;
		case dfMask:
			updated = dst->GetBitmap()->FillMask(destrect, color&0xff );
			break;
		case dfProvince:
		case dfAuto:
			break;
		}
	}

	if( result ) {
		if( updated ) {
			iTJSDispatch2 *ret = TVPCreateRectObject( destrect.left, destrect.top, destrect.right, destrect.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/colorRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/drawText)
{
	// bmp, font, x, y, text, color, opa=255, aa=true, face=dfAlpha, shadowlevel=0, shadowcolor=0x000000, shadowwidth=0, shadowofsx=0, shadowofsy=0, hda=false, clipRect=null
	if(numparams < 6) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tTJSNI_Font* font = NULL;
	clo = param[1]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Font::ClassID, (iTJSNativeInstance**)&font)))
			return TJS_E_INVALIDPARAM;
	}
	if( !font ) return TJS_E_INVALIDPARAM;

	tjs_int x = *param[2];
	tjs_int y = *param[3];
	ttstr text = *param[4];
	tjs_uint32 color = static_cast<tjs_uint32>((tjs_int64)*param[5]);
	tjs_int opa = (numparams >= 7 && param[6]->Type() != tvtVoid)?(tjs_int)*param[6] : (tjs_int)255;
	bool aa = (numparams >= 8 && param[7]->Type() != tvtVoid)? param[7]->operator bool() : true;
	tTVPDrawFace face = (numparams >= 9 && param[8]->Type() != tvtVoid)? (tTVPDrawFace)(tjs_int)(*param[8]) : dfAlpha;
	tjs_int shadowlevel = (numparams >= 10 && param[9]->Type() != tvtVoid)? (tjs_int)*param[9] : 0;
	tjs_uint32 shadowcolor = (numparams >= 11 && param[10]->Type() != tvtVoid)? static_cast<tjs_uint32>((tjs_int64)*param[10]) : 0;
	tjs_int shadowwidth = (numparams >= 12 && param[11]->Type() != tvtVoid)? (tjs_int)*param[11] : 0;
	tjs_int shadowofsx = (numparams >=13 && param[12]->Type() != tvtVoid)? (tjs_int)*param[12] : 0;
	tjs_int shadowofsy = (numparams >=14 && param[13]->Type() != tvtVoid)? (tjs_int)*param[13] : 0;
	bool hda = (numparams >=15 && param[14]->Type() != tvtVoid)? param[14]->operator bool() : false;

	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 16 && param[15]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[15]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}

	tTVPBBBltMethod met;
	switch(face)
	{
	case dfAlpha:
		met = bmAlphaOnAlpha;
		break;
	case dfAddAlpha:
		if(opa<0) return TJS_E_INVALIDPARAM;
		met = bmAlphaOnAddAlpha;
		break;
	case dfOpaque:
		met = bmAlpha;
		break;
	default:
		met = bmAlphaOnAlpha;
		break;
	}
	color = TVPToActualColor(color);
	tTVPComplexRect r;
	dst->GetBitmap()->SetFont( font->GetFont() );
	dst->GetBitmap()->DrawText(clipRect, x, y, text, color, met, opa, hda, aa, shadowlevel, shadowcolor, shadowwidth,
		shadowofsx, shadowofsy, &r);

	if( result ) {
		if( r.GetCount() ) {
			const tTVPRect& ur = r.GetBound();
			iTJSDispatch2 *ret = TVPCreateRectObject( ur.left, ur.top, ur.right, ur.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/drawText)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/drawGlyph)
{
	// bmp, x, y, glyph, color, opa=255, aa=true, face=dfAlpha, shadowlevel=0, shadowcolor=0x000000, shadowwidth=0, shadowofsx=0, shadowofsy=0, hda=false, clipRect=null
	if(numparams < 5) return TJS_E_BADPARAMCOUNT;

	tTJSNI_Bitmap * dst = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dst)))
			return TJS_E_INVALIDPARAM;
	}
	if( !dst ) return TJS_E_INVALIDPARAM;

	tjs_int x = *param[1];
	tjs_int y = *param[2];
	iTJSDispatch2* glyph = param[3]->AsObjectNoAddRef();
	tjs_uint32 color = static_cast<tjs_uint32>((tjs_int64)*param[4]);
	tjs_int opa = (numparams >= 6 && param[5]->Type() != tvtVoid)?(tjs_int)*param[5] : (tjs_int)255;
	bool aa = (numparams >= 7 && param[6]->Type() != tvtVoid)? param[6]->operator bool() : true;
	tTVPDrawFace face = (numparams >= 8 && param[7]->Type() != tvtVoid)? (tTVPDrawFace)(tjs_int)(*param[7]) : dfAlpha;
	tjs_int shadowlevel = (numparams >= 9 && param[8]->Type() != tvtVoid)? (tjs_int)*param[8] : 0;
	tjs_uint32 shadowcolor = (numparams >= 10 && param[9]->Type() != tvtVoid)? static_cast<tjs_uint32>((tjs_int64)*param[9]) : 0;
	tjs_int shadowwidth = (numparams >= 11 && param[10]->Type() != tvtVoid)? (tjs_int)*param[10] : 0;
	tjs_int shadowofsx = (numparams >=12 && param[11]->Type() != tvtVoid)? (tjs_int)*param[11] : 0;
	tjs_int shadowofsy = (numparams >=13 && param[12]->Type() != tvtVoid)? (tjs_int)*param[12] : 0;
	bool hda = (numparams >=14 && param[13]->Type() != tvtVoid)? param[13]->operator bool() : false;

	tTVPRect clipRect( 0, 0, dst->GetWidth(), dst->GetHeight() );
	if(numparams >= 15 && param[14]->Type() == tvtObject ) {
		tTJSNI_Rect * rect = NULL;
		clo = param[14]->AsObjectClosureNoAddRef();
		if(clo.Object) {
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Rect::ClassID, (iTJSNativeInstance**)&rect)))
				return TJS_E_INVALIDPARAM;
			clipRect = rect->Get();
		}
	}

	tTVPBBBltMethod met;
	switch(face)
	{
	case dfAlpha:
		met = bmAlphaOnAlpha;
		break;
	case dfAddAlpha:
		if(opa<0) return TJS_E_INVALIDPARAM;
		met = bmAlphaOnAddAlpha;
		break;
	case dfOpaque:
		met = bmAlpha;
		break;
	default:
		met = bmAlphaOnAlpha;
		break;
	}
	color = TVPToActualColor(color);
	tTVPComplexRect r;
	dst->GetBitmap()->DrawGlyph(glyph, clipRect, x, y, color, met, opa, hda, aa, shadowlevel, shadowcolor, shadowwidth,
		shadowofsx, shadowofsy, &r);

	if( result ) {
		if( r.GetCount() ) {
			const tTVPRect& ur = r.GetBound();
			iTJSDispatch2 *ret = TVPCreateRectObject( ur.left, ur.top, ur.right, ur.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/drawGlyph)

	TJS_END_NATIVE_MEMBERS
}

tTJSNativeInstance *tTJSNC_ImageFunction::CreateNativeInstance() {
	return new tTJSNI_ImageFunction();
}
tTJSNativeClass * TVPCreateNativeClass_ImageFunction()
{
	tTJSNativeClass *cls = new tTJSNC_ImageFunction();
	return cls;
}