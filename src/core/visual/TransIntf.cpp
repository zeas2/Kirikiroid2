//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Transition handler mamagement & default transition handlers
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "TransIntf.h"
#include "LayerIntf.h"
#include "GraphicsLoaderIntf.h"
#include "tjsHashSearch.h"
#include "MsgIntf.h"
#include "SysInitIntf.h"
#include "tvpgl.h"
#include "TickCount.h"
#include "DebugIntf.h"
#include "RenderManager.h"
#include "Platform.h"

// #define TVP_TRANS_SHOW_FPS

//---------------------------------------------------------------------------
// iTVPSimpleOptionProvider implementation
//---------------------------------------------------------------------------
/* this implementation should move into other proper unit */
tTVPSimpleOptionProvider::tTVPSimpleOptionProvider(tTJSVariantClosure object)
{
	RefCount = 1;
	Object = object;
	Object.AddRef();
}
//---------------------------------------------------------------------------
tTVPSimpleOptionProvider::~tTVPSimpleOptionProvider()
{
	Object.Release();
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPSimpleOptionProvider::AddRef()
{
	RefCount++;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPSimpleOptionProvider::Release()
{
	if(RefCount == 1)
		delete this;
	else
		RefCount--;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPSimpleOptionProvider::GetAsNumber(
		/*in*/const tjs_char *name, /*out*/tjs_int64 *value)
{
	try
	{
		tTJSVariant val;
		tjs_error er = Object.PropGet(0, name, NULL, &val, NULL);
		if(TJS_FAILED(er)) return er;

		if(val.Type() == tvtVoid) return TJS_E_MEMBERNOTFOUND;

		if(value) *value = val.AsInteger();

		return TJS_S_OK;
	}
	catch(...)
	{
		return TJS_E_FAIL;
	}
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPSimpleOptionProvider::GetAsString(
		/*in*/const tjs_char *name, /*out*/const tjs_char **out)
{
	try
	{
		tTJSVariant val;
		tjs_error er = Object.PropGet(0, name, NULL, &val, NULL);
		if(TJS_FAILED(er)) return er;

		if(val.Type() == tvtVoid) return TJS_E_MEMBERNOTFOUND;

		String = val;  // to hold string buffer
		if(out) *out = String.c_str();

		return TJS_S_OK;
	}
	catch(...)
	{
		return TJS_E_FAIL;
	}
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPSimpleOptionProvider::GetValue(
			/*in*/const tjs_char *name, /*out*/tTJSVariant *dest)
{
	try
	{
		if(!dest) return TJS_E_FAIL;
		tjs_error er = Object.PropGet(0, name, NULL, dest, NULL);
		if(TJS_FAILED(er)) return er;
		return TJS_S_OK;
	}
	catch(...)
	{
		return TJS_E_FAIL;
	}
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPSimpleOptionProvider::
	GetDispatchObject(iTJSDispatch2 **dsp)
{
	try
	{
		Object.ObjThis->AddRef();
		if(dsp) *dsp = Object.ObjThis;
		return TJS_S_OK;
	}
	catch(...)
	{
		return TJS_E_FAIL;
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// iTVPSimpleImageProvider implementation
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPSimpleImageProvider::LoadImage(
			/*in*/const tjs_char *name, /*in*/tjs_int bpp,
			/*in*/tjs_uint32 key,
			/*in*/tjs_uint w,
			/*in*/tjs_uint h,
			/*out*/iTVPScanLineProvider ** scpro)
{
	if(bpp != 8 && bpp != 32) return TJS_E_FAIL; // invalid bitmap color depth

	tTVPBaseTexture *bitmap = new tTVPBaseTexture(TVPGetInitialBitmap());

	try
	{
		TVPLoadGraphic(bitmap, name, key, w, h, bpp == 8 ? glmGrayscale : glmNormal);
	}
	catch(...)
	{
		delete bitmap;
		return TJS_E_FAIL;
	}

	if(scpro) *scpro = new tTVPScanLineProviderForBaseBitmap(bitmap, true);

	return TJS_S_OK;
}
tTVPSimpleImageProvider TVPSimpleImageProvider;
//---------------------------------------------------------------------------


#if 0
//---------------------------------------------------------------------------
// Image Provider Service for other plug-ins
//---------------------------------------------------------------------------
iTVPScanLineProvider * TVPSLPLoadImage(const ttstr &name, tjs_int bpp,
	tjs_uint32 key, tjs_uint w, tjs_uint h)
{
	if(bpp != 8 && bpp != 32) return NULL; // invalid bitmap color depth

	tTVPBaseBitmap *bitmap = new tTVPBaseBitmap(TVPGetInitialBitmap());

	iTVPScanLineProvider *pro;

	try
	{
		TVPLoadGraphic(bitmap, name, key, w, h, bpp == 8 ? glmGrayscale : glmNormal);
		pro = new tTVPScanLineProviderForBaseBitmap(bitmap, true);
	}
	catch(...)
	{
		delete bitmap;
		throw;
	}

	return pro;
}
//---------------------------------------------------------------------------
#endif



//---------------------------------------------------------------------------
// iTVPScanLineProvider implementation for image provider ( holds tTVPBaseBitmap )
//---------------------------------------------------------------------------
tTVPScanLineProviderForBaseBitmap::
	tTVPScanLineProviderForBaseBitmap(iTVPBaseBitmap *bmp, bool own)
{
	RefCount = 1;
	Own = own;
	Bitmap = bmp;
}
//---------------------------------------------------------------------------
tTVPScanLineProviderForBaseBitmap::~tTVPScanLineProviderForBaseBitmap()
{
	if(Own) if(Bitmap) delete Bitmap;
}
//---------------------------------------------------------------------------
void tTVPScanLineProviderForBaseBitmap::Attach(iTVPBaseBitmap *bmp)
{
	// attach bitmap
	Bitmap = bmp;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPScanLineProviderForBaseBitmap::AddRef()
{
	RefCount ++;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPScanLineProviderForBaseBitmap::Release()
{
	if(RefCount == 1)
		delete this;
	else
		RefCount--;
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTVPScanLineProviderForBaseBitmap::GetWidth(/*in*/tjs_int *width)
{
	if(width) *width = Bitmap->GetWidth();
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTVPScanLineProviderForBaseBitmap::GetHeight(/*in*/tjs_int *height)
{
	if(height) *height = Bitmap->GetHeight();
	return TJS_S_OK;
}
#if 0
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTVPScanLineProviderForBaseBitmap::GetPixelFormat(/*out*/tjs_int *bpp)
{
	if(bpp) *bpp = Bitmap->GetBPP();
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPScanLineProviderForBaseBitmap::GetPitchBytes(
	/*out*/tjs_int *pitch)
{
	if(pitch) *pitch = Bitmap->GetPitchBytes();
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTVPScanLineProviderForBaseBitmap::GetScanLine(/*in*/tjs_int line,
			/*out*/const void ** scanline)
{
	try
	{
		if(scanline) *scanline = Bitmap->GetScanLine(line);
	}
	catch(...)
	{
		return TJS_E_FAIL;
	}
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTVPScanLineProviderForBaseBitmap::GetScanLineForWrite(/*in*/tjs_int line,
			/*out*/void ** scanline)
{
	try
	{
		if(scanline) *scanline = Bitmap->GetScanLineForWrite(line);
	}
	catch(...)
	{
		return TJS_E_FAIL;
	}
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
#endif

iTVPTexture2D * tTVPScanLineProviderForBaseBitmap::GetTexture()
{
	return Bitmap->GetTexture();
}

iTVPTexture2D * tTVPScanLineProviderForBaseBitmap::GetTextureForRender()
{
	return Bitmap->GetTextureForRender(false, nullptr);
}




//---------------------------------------------------------------------------
// transition handler management
//---------------------------------------------------------------------------
typedef tTJSRefHolder<iTVPTransHandlerProvider> tTVPTransHandlerProviderHolder;
static tTJSHashTable<ttstr, tTVPTransHandlerProviderHolder>
	TVPTransHandlerProviders;
bool TVPTransHandlerProviderInit = true;
//---------------------------------------------------------------------------
void TVPAddTransHandlerProvider(iTVPTransHandlerProvider *pro)
{
	// add transition handler provider to the system

	// retrieve transition name
	const tjs_char *cname;
	if(TJS_FAILED(pro->GetName(&cname)))
		TVPThrowExceptionMessage(TVPTransHandlerError,
		TJS_W("tTVPTransHandlerProvider::GetName failed"));
	ttstr name = cname;
	tTVPTransHandlerProviderHolder *p =
		TVPTransHandlerProviders.Find(name);
	if(p)
		TVPThrowExceptionMessage(TVPTransAlreadyRegistered, name);

	tTVPTransHandlerProviderHolder holder(pro);
	TVPTransHandlerProviders.Add(name, holder);
}
//---------------------------------------------------------------------------
void TVPRemoveTransHandlerProvider(iTVPTransHandlerProvider *pro)
{
	// remove transition handler provider from the system

	// retrieve transition name
	const tjs_char * cname;
	if(TJS_FAILED(pro->GetName(&cname)))
		TVPThrowExceptionMessage(TVPTransHandlerError,
		TJS_W("tTVPTransHandlerProvider::GetName failed"));
	ttstr name = cname;

	TVPTransHandlerProviders.Delete(name);
}
//---------------------------------------------------------------------------
static void TVPRegisterDefaultTransHandlerProvider();
iTVPTransHandlerProvider * TVPFindTransHandlerProvider(const ttstr &name)
{
	if(TVPTransHandlerProviderInit)
	{
		// we assume that transition that has the same name as the other does
		// not exist
		TVPRegisterDefaultTransHandlerProvider();
		TVPTransHandlerProviderInit = false;
	}

	tTVPTransHandlerProviderHolder *holder =
		TVPTransHandlerProviders.Find(name);
	if (!holder) {
		static bool showed = false;
		if (!showed) {
			TVPShowSimpleMessageBox(TVPFormatMessage(TVPCannotFindTransHander, name), TJS_W("Warning"));
			showed = true;
		}
		holder = TVPTransHandlerProviders.Find(TJS_W("crossfade"));
	}

	iTVPTransHandlerProvider * pro = holder->GetObjectNoAddRef();
	pro->AddRef();
	return pro;
}
//---------------------------------------------------------------------------
static void TVPClearTransHandlerProvider()
{
	// called at system shutdown
	TVPTransHandlerProviders.Clear();
}
static tTVPAtExit
	TVPClearTransHandlerProviderAtExit(TVP_ATEXIT_PRI_SHUTDOWN,
		TVPClearTransHandlerProvider);
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// Cross fade transition handler
//---------------------------------------------------------------------------
//extern DWORD acctime;

class tTVPCrossFadeTransHandler : public iTVPDivisibleTransHandler
{
	tjs_int RefCount;
protected:
	iTVPSimpleOptionProvider *Options;
	tTVPLayerType DestLayerType;
	tjs_uint64 StartTick;
	tjs_uint64 Time; // time during transition
	bool First;

	tjs_int PhaseMax;
	tjs_int Phase; // current phase (0 thru PhaseMax)

#ifdef TVP_TRANS_SHOW_FPS
	tjs_int Count;
	tjs_uint32 ProcessTick;
	tjs_uint32 ProcessTime;
	tjs_uint32 BlendTick;
	tjs_uint32 BlendTime;
#endif

public:
	tTVPCrossFadeTransHandler(iTVPSimpleOptionProvider *options,
		tTVPLayerType layertype, tjs_uint64 time, tjs_int phasemax = 255)
	{
		RefCount = 1;
		DestLayerType = layertype;
		Options = options;
		if(Options) Options->AddRef();
		Time = time;
		PhaseMax = phasemax;
#ifdef TVP_TRANS_SHOW_FPS
		Count = 0;
		ProcessTime = 0;
		BlendTime = 0;

//		acctime = 0;
#endif
		First = true;
	}

	virtual ~tTVPCrossFadeTransHandler()
	{
		if(Options) Options->Release();
	}

	tjs_error TJS_INTF_METHOD AddRef()
	{
		RefCount ++;
		return TJS_S_OK;
	}

	tjs_error TJS_INTF_METHOD Release()
	{
		if(RefCount == 1) delete this; else RefCount--;
		return TJS_S_OK;
	}


	tjs_error TJS_INTF_METHOD SetOption(
			/*in*/iTVPSimpleOptionProvider *options // option provider
		)
	{
		if(Options) Options->Release();
		options = options;
		Options = options;
		if(Options) Options->AddRef();
		return TJS_S_OK;
	}

	tjs_error TJS_INTF_METHOD StartProcess(tjs_uint64 tick);
	tjs_error TJS_INTF_METHOD EndProcess();
	tjs_error TJS_INTF_METHOD Process(
			/*in,out*/tTVPDivisibleData *data);
	tjs_error TJS_INTF_METHOD MakeFinalImage(
			/*in,out*/iTVPScanLineProvider ** dest,
			/*in*/iTVPScanLineProvider * src1,
			/*in*/iTVPScanLineProvider * src2);

	virtual void Blend(tTVPDivisibleData *data);
};
//---------------------------------------------------------------------------

tTVPCrossFadeTransHandlerProvider::tTVPCrossFadeTransHandlerProvider()
{
	RefCount = 1;
}

tTVPCrossFadeTransHandlerProvider::~tTVPCrossFadeTransHandlerProvider()
{

}

tjs_error TJS_INTF_METHOD tTVPCrossFadeTransHandlerProvider::AddRef()
{
	RefCount++;
	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD tTVPCrossFadeTransHandlerProvider::Release()
{
	if (RefCount == 1) delete this; else RefCount--;
	return TJS_S_OK;
}

tjs_error TJS_INTF_METHOD tTVPCrossFadeTransHandlerProvider::GetName(/*out*/const tjs_char ** name)
{
	if (name)
	{
		*name = TJS_W("crossfade"); return TJS_S_OK;
	} else
	{
		return TJS_E_FAIL;
	}
}

tjs_error TJS_INTF_METHOD tTVPCrossFadeTransHandlerProvider::StartTransition(/*in*/iTVPSimpleOptionProvider *options, /* option provider */ /*in*/iTVPSimpleImageProvider *imagepro, /* image provider */ /*in*/tTVPLayerType layertype, /* destination layer type */ /*in*/tjs_uint src1w, tjs_uint src1h, /* source 1 size */ /*in*/tjs_uint src2w, tjs_uint src2h, /* source 2 size */ /*out*/tTVPTransType *type, /* transition type */ /*out*/tTVPTransUpdateType * updatetype, /* update typwe */ /*out*/iTVPBaseTransHandler ** handler /* transition handler */)
{
	if (type) *type = ttExchange; // transition type : exchange
	if (updatetype) *updatetype = tutDivisibleFade;
	// update type : divisible fade
	if (!handler) return TJS_E_FAIL;
	if (!options) return TJS_E_FAIL;

	if (src1w != src2w || src1h != src2h)
		TVPThrowExceptionMessage(TVPTransitionLayerSizeMismatch,
		ttstr((tjs_int)src2w) + TJS_W("x") + ttstr((tjs_int)src2h),
		ttstr((tjs_int)src1w) + TJS_W("x") + ttstr((tjs_int)src1h));

	*handler = GetTransitionObject(options, imagepro, layertype, src1w, src1h,
		src2w, src2h);

	return TJS_S_OK;
}

iTVPBaseTransHandler * tTVPCrossFadeTransHandlerProvider::GetTransitionObject(/*in*/iTVPSimpleOptionProvider *options, /* option provider */ /*in*/iTVPSimpleImageProvider *imagepro, /* image provider */ /*in*/tTVPLayerType layertype, /*in*/tjs_uint src1w, tjs_uint src1h, /* source 1 size */ /*in*/tjs_uint src2w, tjs_uint src2h) // source 2 size
{
	tjs_int64 time;
	tjs_error er = options->GetAsNumber(TJS_W("time"), (tjs_int64 *)&time);
	if (TJS_FAILED(er)) TVPThrowExceptionMessage(TVPSpecifyOption, TJS_W("time"));
	if (time < 2) time = 2; // too small time may cause problem

	return  (iTVPBaseTransHandler *)
		(new tTVPCrossFadeTransHandler(options, layertype, time));
}

//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPCrossFadeTransHandler::
	StartProcess(tjs_uint64 tick)
{
	// notifies starting of the update
	if(First)
	{
		First = false;
		StartTick = tick;

	}

	// compute phase ( 0 thru 255 )
	Phase = static_cast<tjs_int>( (tick - StartTick)  * PhaseMax / Time );
	if(Phase >= PhaseMax) Phase = PhaseMax;

#ifdef TVP_TRANS_SHOW_FPS
	if((tick-StartTick) >= Time/2)
	{
		if(Count)
		{
			TVPAddLog(TJS_W("update count : ") + ttstr(Count));
			TVPAddLog(TJS_W("trans time : ") + ttstr((tjs_int)(tick-StartTick)));
			TVPAddLog(TJS_W("process time : ") + ttstr((tjs_int)ProcessTime));
			TVPAddLog(TJS_W("process time / trans time (%) : ") +
				ttstr((tjs_int)(ProcessTime*100/(tick-StartTick))));
			TVPAddLog(TJS_W("blend time / trans time (%) : ") +
				ttstr((tjs_int)(BlendTime*100/(tick-StartTick))));
//			TVPAddLog(TJS_W("blt time / trans time (%) : ") +
//				ttstr((tjs_int)(acctime*100/(tick-StartTick))));
			tjs_int avgtime;
			avgtime = ProcessTime / Count;
			TVPAddLog(TJS_W("process time / update count : ") + ttstr(avgtime));
			tjs_int fps = Count * 1000 / (tick - StartTick);
			TVPAddLog(TJS_W("fps : ") + ttstr(fps));

			Count = 0;
		}
	}
	else
	{
		Count++;
	}


	ProcessTick = TVPGetRoughTickCount32();
#endif

	return TJS_S_TRUE;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPCrossFadeTransHandler::EndProcess()
{
	// notifies ending of the update
#ifdef TVP_TRANS_SHOW_FPS
	ProcessTime += TVPGetRoughTickCount32() - ProcessTick;
#endif

	if(Phase >= PhaseMax) return TJS_S_FALSE;
	return TJS_S_TRUE;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPCrossFadeTransHandler::Process(
		/*in,out*/tTVPDivisibleData *data)
{
	// process divided region of the entire layer bitmap

#ifdef TVP_TRANS_SHOW_FPS
	BlendTick = TVPGetRoughTickCount32();
#endif

	if(Phase == 0)
	{
		// completely source 1
		data->Dest = data->Src1;
		data->DestLeft = data->Src1Left;
		data->DestTop = data->Src1Top;
	}
	else if(Phase == PhaseMax)
	{
		// completety source 2
		data->Dest = data->Src2;
		data->DestLeft = data->Src2Left;
		data->DestTop = data->Src2Top;
	}
	else
	{
		// blend
		Blend(data);
	}

#ifdef TVP_TRANS_SHOW_FPS
	BlendTime += TVPGetRoughTickCount32() - BlendTick;
#endif

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void tTVPCrossFadeTransHandler::Blend(tTVPDivisibleData *data)
{
	// blend
#if 0
	tjs_uint8 *dest;
	const tjs_uint8 *src1;
	const tjs_uint8 *src2;
	tjs_int destpitch;
	tjs_int src1pitch;
	tjs_int src2pitch;

	data->Dest->GetScanLineForWrite(data->DestTop, (void**)&dest);
	data->Src1->GetScanLine(data->Src1Top, (const void**)&src1);
	data->Src2->GetScanLine(data->Src2Top, (const void**)&src2);

	data->Dest->GetPitchBytes(&destpitch);
	data->Src1->GetPitchBytes(&src1pitch);
	data->Src2->GetPitchBytes(&src2pitch);

	dest += data->DestLeft * sizeof(tjs_uint32);
	src1 += data->Src1Left * sizeof(tjs_uint32);
	src2 += data->Src2Left * sizeof(tjs_uint32);

	tjs_int h = data->Height;

	if(TVPIsTypeUsingAlpha(DestLayerType))
	{
		while(h--)
		{
			TVPConstAlphaBlend_SD_d((tjs_uint32*)dest, (const tjs_uint32*)src1,
				(const tjs_uint32*)src2, data->Width, Phase);
			dest += destpitch, src1 += src1pitch, src2 += src2pitch;
		}
	}
	else if(TVPIsTypeUsingAddAlpha(DestLayerType))
	{
		while(h--)
		{
			TVPConstAlphaBlend_SD_a((tjs_uint32*)dest, (const tjs_uint32*)src1,
				(const tjs_uint32*)src2, data->Width, Phase);
			dest += destpitch, src1 += src1pitch, src2 += src2pitch;
		}
	}
	else
	{
		while(h--)
		{
			TVPConstAlphaBlend_SD((tjs_uint32*)dest, (const tjs_uint32*)src1,
				(const tjs_uint32*)src2, data->Width, Phase);
			dest += destpitch, src1 += src1pitch, src2 += src2pitch;
		}
	}
#endif
	iTVPRenderMethod *method;
	int opa_id;
	if (TVPIsTypeUsingAlpha(DestLayerType)) {
		static iTVPRenderMethod *_method = TVPGetRenderManager()->GetRenderMethod("ConstAlphaBlend_SD_d");
		static int _opa_id = _method->EnumParameterID("opacity");
		method = _method; opa_id = _opa_id;
	} else if (TVPIsTypeUsingAddAlpha(DestLayerType)) {
		static iTVPRenderMethod *_method = TVPGetRenderManager()->GetRenderMethod("ConstAlphaBlend_SD_a");
		static int _opa_id = _method->EnumParameterID("opacity");
		method = _method; opa_id = _opa_id;
	} else {
		static iTVPRenderMethod *_method = TVPGetRenderManager()->GetRenderMethod("ConstAlphaBlend_SD");
		static int _opa_id = _method->EnumParameterID("opacity");
		method = _method; opa_id = _opa_id;
	}
	method->SetParameterOpa(opa_id, Phase);
	tRenderTexRectArray::Element src_tex[] = {
		tRenderTexRectArray::Element(data->Src1->GetTexture(), tTVPRect(data->Src1Left, data->Src1Top, data->Src1Left + data->Width, data->Src1Top + data->Height)),
		tRenderTexRectArray::Element(data->Src2->GetTexture(), tTVPRect(data->Src2Left, data->Src2Top, data->Src2Left + data->Width, data->Src2Top + data->Height))
	};
	TVPGetRenderManager()->OperateRect(method, data->Dest->GetTextureForRender(),
		nullptr, tTVPRect(data->DestLeft, data->DestTop, data->DestLeft + data->Width, data->DestTop + data->Height),
		tRenderTexRectArray(src_tex));
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPCrossFadeTransHandler::MakeFinalImage(
		/*in,out*/iTVPScanLineProvider ** dest,
		/*in*/iTVPScanLineProvider * src1,
		/*in*/iTVPScanLineProvider * src2)
{
	// final image is the source2 bitmap
	*dest = src2;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// universal transition handler
//---------------------------------------------------------------------------
class tTVPUniversalTransHandler : public tTVPCrossFadeTransHandler
{
	typedef tTVPCrossFadeTransHandler inherited;

	tjs_int Vague;
	iTVPScanLineProvider * Rule;
	//tjs_uint32 BlendTable[256];
	iTVPRenderMethod *Method;
	tjs_int MethodPhaseID, MethodVagueID;
public:
	tTVPUniversalTransHandler(
		iTVPSimpleOptionProvider *options,
		tTVPLayerType destlayertype, tjs_uint64 time, tjs_int vague,
		iTVPScanLineProvider *rule) :
			tTVPCrossFadeTransHandler(options, destlayertype, time, 255+vague)
	{
		Vague = vague;
		Rule = rule;
		Rule->AddRef();

		if (TVPIsTypeUsingAlpha(DestLayerType)) {
			static iTVPRenderMethod *_method = TVPGetRenderManager()->GetRenderMethod("UnivTransBlend_d");
			static int phase_id = _method->EnumParameterID("phase"), vague_id = _method->EnumParameterID("vague");
			Method = _method; MethodPhaseID = phase_id; MethodVagueID = vague_id;
		} else if (TVPIsTypeUsingAddAlpha(DestLayerType)) {
			static iTVPRenderMethod *_method = TVPGetRenderManager()->GetRenderMethod("UnivTransBlend_a");
			static int phase_id = _method->EnumParameterID("phase"), vague_id = _method->EnumParameterID("vague");
			Method = _method; MethodPhaseID = phase_id; MethodVagueID = vague_id;
		} else {
			static iTVPRenderMethod *_method = TVPGetRenderManager()->GetRenderMethod("UnivTransBlend");
			static int phase_id = _method->EnumParameterID("phase"), vague_id = _method->EnumParameterID("vague");
			Method = _method; MethodPhaseID = phase_id; MethodVagueID = vague_id;
		}
		Method->SetParameterInt(MethodVagueID, vague);
	}

	~tTVPUniversalTransHandler()
	{
		Rule->Release();
	}

	tjs_error TJS_INTF_METHOD StartProcess(tjs_uint64 tick);
		// tTVPCrossFadeTransHandler::blend override
	void Blend(tTVPDivisibleData *data);
		// tTVPCrossFadeTransHandler::blend override

};
//---------------------------------------------------------------------------
class tTVPUniversalTransHandlerProvider : public tTVPCrossFadeTransHandlerProvider
{
public:
	tTVPUniversalTransHandlerProvider() : tTVPCrossFadeTransHandlerProvider() {}
	virtual ~tTVPUniversalTransHandlerProvider() { };

	tjs_error TJS_INTF_METHOD GetName(
			/*out*/const tjs_char ** name)
	{
		if(name)
			{ *name = TJS_W("universal"); return TJS_S_OK; }
		else
			{ return TJS_E_FAIL; }

	}

	virtual iTVPBaseTransHandler * GetTransitionObject(
			/*in*/iTVPSimpleOptionProvider *options, // option provider
			/*in*/iTVPSimpleImageProvider *imagepro, // image provider
			/*in*/tTVPLayerType layertype,
			/*in*/tjs_uint src1w, tjs_uint src1h, // source 1 size
			/*in*/tjs_uint src2w, tjs_uint src2h) // source 2 size
	{
		tjs_error er;

		// retrieve "time" option
		tjs_int64 time;
		er = options->GetAsNumber(TJS_W("time"), (tjs_int64 *)&time);
		if(TJS_FAILED(er)) TVPThrowExceptionMessage(TVPSpecifyOption, TJS_W("time"));
		if(time < 2) time = 2; // too small time may cause problem

		// retrieve "vague" option
		tjs_int64 vague;
		er = options->GetAsNumber(TJS_W("vague"), (tjs_int64 *)&vague);
		if(TJS_FAILED(er)) vague = 64;

		// retrieve "rule" option and load it as an image
		const tjs_char *rulename;
		er = options->GetAsString(TJS_W("rule"), &rulename);
		if(TJS_FAILED(er)) TVPThrowExceptionMessage(TVPSpecifyOption, TJS_W("rule"));

		iTVPScanLineProvider *scpro;
		er = imagepro->LoadImage(rulename, 8, 0x02ffffff,
			src1w, src1h, &scpro);
		if(TJS_FAILED(er))
			TVPThrowExceptionMessage(TVPCannotLoadRuleGraphic, rulename);

		// create a transition object
		iTVPBaseTransHandler *ret;
		try
		{
			ret =  (iTVPBaseTransHandler *)
				(new tTVPUniversalTransHandler(options, layertype, time, static_cast<tjs_int>(vague),
					scpro));
		}
		catch(...)
		{
			scpro->Release();
			throw;
		}
		scpro->Release();

		return ret;
	}
};
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTVPUniversalTransHandler::StartProcess(
	tjs_uint64 tick)
{
	tjs_error er;
	er = inherited::StartProcess(tick);
	if(TJS_FAILED(er)) return er;

	// start one frame of the transition
	Method->SetParameterInt(MethodPhaseID, Phase);
#if 0
	// create blend table
	if(TVPIsTypeUsingAlpha(DestLayerType))
		TVPInitUnivTransBlendTable_d(BlendTable, Phase, Vague);
	else if(TVPIsTypeUsingAddAlpha(DestLayerType))
		TVPInitUnivTransBlendTable_a(BlendTable, Phase, Vague);
	else
		TVPInitUnivTransBlendTable(BlendTable, Phase, Vague);
#endif
	return er;
}
//---------------------------------------------------------------------------
void tTVPUniversalTransHandler::Blend(tTVPDivisibleData *data)
{
	// blend the image according with the rule graphic
#if 0
	tjs_uint8 *dest;
	const tjs_uint8 *src1;
	const tjs_uint8 *src2;
	const tjs_uint8 *rule;

	data->Dest->GetScanLineForWrite(data->DestTop, (void**)&dest);
	data->Src1->GetScanLine(data->Src1Top, (const void**)&src1);
	data->Src2->GetScanLine(data->Src2Top, (const void**)&src2);
	Rule->GetScanLine(data->Top, (const void**)&rule);

	tjs_int destpitch;
	tjs_int src1pitch;
	tjs_int src2pitch;
	tjs_int rulepitch;

	data->Dest->GetPitchBytes(&destpitch);
	data->Src1->GetPitchBytes(&src1pitch);
	data->Src2->GetPitchBytes(&src2pitch);
	Rule->GetPitchBytes(&rulepitch);

	dest += data->DestLeft * sizeof(tjs_uint32);
	src1 += data->Src1Left * sizeof(tjs_uint32);
	src2 += data->Src2Left * sizeof(tjs_uint32);
	rule += data->Left * sizeof(tjs_uint8);

	tjs_int h = data->Height;
	if(Vague >= 512)
	{
		if(TVPIsTypeUsingAlpha(DestLayerType))
		{
			while(h--)
			{
				TVPUnivTransBlend_d((tjs_uint32*)dest, (const tjs_uint32*)src1,
					(const tjs_uint32*)src2, rule, BlendTable, data->Width);
				dest += destpitch, src1 += src1pitch, src2 += src2pitch;
				rule += rulepitch;
			}
		}
		else if(TVPIsTypeUsingAddAlpha(DestLayerType))
		{
			while(h--)
			{
				TVPUnivTransBlend_a((tjs_uint32*)dest, (const tjs_uint32*)src1,
					(const tjs_uint32*)src2, rule, BlendTable, data->Width);
				dest += destpitch, src1 += src1pitch, src2 += src2pitch;
				rule += rulepitch;
			}
		}
		else
		{
			while(h--)
			{
				TVPUnivTransBlend((tjs_uint32*)dest, (const tjs_uint32*)src1,
					(const tjs_uint32*)src2, rule, BlendTable, data->Width);
				dest += destpitch, src1 += src1pitch, src2 += src2pitch;
				rule += rulepitch;
			}
		}
	}
	else
	{
		tjs_int src1lv = Phase;
		tjs_int src2lv = Phase - Vague;

		if(TVPIsTypeUsingAlpha(DestLayerType))
		{
			while(h--)
			{
				TVPUnivTransBlend_switch_d((tjs_uint32*)dest, (const tjs_uint32*)src1,
					(const tjs_uint32*)src2, rule, BlendTable, data->Width,
						src1lv, src2lv);
				dest += destpitch, src1 += src1pitch, src2 += src2pitch;
				rule += rulepitch;
			}
		}
		else if(TVPIsTypeUsingAddAlpha(DestLayerType))
		{
			while(h--)
			{
				TVPUnivTransBlend_switch_a((tjs_uint32*)dest, (const tjs_uint32*)src1,
					(const tjs_uint32*)src2, rule, BlendTable, data->Width,
						src1lv, src2lv);
				dest += destpitch, src1 += src1pitch, src2 += src2pitch;
				rule += rulepitch;
			}
		}
		else
		{
			while(h--)
			{
				TVPUnivTransBlend_switch((tjs_uint32*)dest, (const tjs_uint32*)src1,
					(const tjs_uint32*)src2, rule, BlendTable, data->Width,
						src1lv, src2lv);
				dest += destpitch, src1 += src1pitch, src2 += src2pitch;
				rule += rulepitch;
			}
		}
	}
#endif
	tRenderTexRectArray::Element src_tex[] = {
		tRenderTexRectArray::Element(data->Src1->GetTexture(), tTVPRect(data->Src1Left, data->Src1Top, data->Src1Left + data->Width, data->Src1Top + data->Height)),
		tRenderTexRectArray::Element(data->Src2->GetTexture(), tTVPRect(data->Src2Left, data->Src2Top, data->Src2Left + data->Width, data->Src2Top + data->Height)),
		tRenderTexRectArray::Element(Rule->GetTexture(), tTVPRect(data->Left, data->Top, data->Left + data->Width, data->Top + data->Height))
	};
	TVPGetRenderManager()->OperateRect(Method, data->Dest->GetTextureForRender(),
		nullptr, tTVPRect(data->DestLeft, data->DestTop, data->DestLeft + data->Width, data->DestTop + data->Height),
		tRenderTexRectArray(src_tex));
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// scroll transition handler
//---------------------------------------------------------------------------
static void TVPSLPCopyRect(iTVPScanLineProvider *destimg, tjs_int x, tjs_int y,
	iTVPScanLineProvider *srcimg, const tTVPRect &srcrect)
{
	// this function does not matter if the src==dest and copying area is
	// overlapped.
	// destimg and srcimg must be 32bpp bitmap.
#if 0
	tjs_uint8 *dest;
	const tjs_uint8 *src;
	tjs_int destpitch;
	tjs_int srcpitch;

	destimg->GetScanLineForWrite(y, (void**)&dest);
	srcimg->GetScanLine(srcrect.top, (const void**)&src);

	destimg->GetPitchBytes(&destpitch);
	srcimg->GetPitchBytes(&srcpitch);

	dest += x * sizeof(tjs_uint32);
	src += srcrect.left * sizeof(tjs_uint32);

	tjs_int h = srcrect.get_height();
	tjs_int bytes = srcrect.get_width() * sizeof(tjs_uint32);
	while(h--)
	{
		memcpy(dest, src, bytes);
		dest += destpitch, src += srcpitch;
	}
#endif
	static iTVPRenderMethod *method = TVPGetRenderManager()->GetRenderMethod("Copy");

	tRenderTexRectArray::Element src_tex[] = {
		tRenderTexRectArray::Element(srcimg->GetTexture(), srcrect)
	};

	TVPGetRenderManager()->OperateRect(method, destimg->GetTextureForRender(), nullptr,
		tTVPRect(x, y, x + srcrect.get_width(), y + srcrect.get_height()), tRenderTexRectArray(src_tex));
}
//---------------------------------------------------------------------------
class tTVPScrollTransHandler : public tTVPCrossFadeTransHandler
{
	typedef tTVPCrossFadeTransHandler inherited;

	tTVPScrollTransFrom From;
	tTVPScrollTransStay Stay;

public:
	tTVPScrollTransHandler(
		iTVPSimpleOptionProvider *options,
		tTVPLayerType layertype, tjs_uint64 time, tTVPScrollTransFrom from,
			tTVPScrollTransStay stay, tjs_int maxphase) :
			tTVPCrossFadeTransHandler(options, layertype, time, maxphase)
	{
		From = from;
		Stay = stay;
	}

	~tTVPScrollTransHandler()
	{
	}

	void Blend(tTVPDivisibleData *data);
		// tTVPCrossFadeTransHandler::blend override

};
//---------------------------------------------------------------------------
class tTVPScrollTransHandlerProvider : public tTVPCrossFadeTransHandlerProvider
{
public:
	tTVPScrollTransHandlerProvider() : tTVPCrossFadeTransHandlerProvider() {}
	virtual ~tTVPScrollTransHandlerProvider() { };

	tjs_error TJS_INTF_METHOD GetName(
			/*out*/const tjs_char ** name)
	{
		if(name)
			{ *name = TJS_W("scroll"); return TJS_S_OK; }
		else
			{ return TJS_E_FAIL; }

	}

	tjs_error TJS_INTF_METHOD StartTransition(
			/*in*/iTVPSimpleOptionProvider *options, // option provider
			/*in*/iTVPSimpleImageProvider *imagepro, // image provider
			/*in*/tTVPLayerType layertype, // destination layer type
			/*in*/tjs_uint src1w, tjs_uint src1h, // source 1 size
			/*in*/tjs_uint src2w, tjs_uint src2h, // source 2 size
			/*out*/tTVPTransType *type, // transition type
			/*out*/tTVPTransUpdateType * updatetype, // update typwe
			/*out*/iTVPBaseTransHandler ** handler // transition handler
			)
	{
		tjs_error er = tTVPCrossFadeTransHandlerProvider::StartTransition
			(options, imagepro, layertype, src1w, src1h, src2w, src2h,
			type, updatetype, handler);

		if(TJS_SUCCEEDED(er)) *updatetype = tutDivisible;

		return er;
	}


	virtual iTVPBaseTransHandler * GetTransitionObject(
			/*in*/iTVPSimpleOptionProvider *options, // option provider
			/*in*/iTVPSimpleImageProvider *imagepro, // image provider
			/*in*/tTVPLayerType layertype,
			/*in*/tjs_uint src1w, tjs_uint src1h, // source 1 size
			/*in*/tjs_uint src2w, tjs_uint src2h) // source 2 size
	{
		tjs_error er;
		tjs_int64 value;

		// retrieve "time" option
		tjs_int64 time;
		er = options->GetAsNumber(TJS_W("time"), &time);
		if(TJS_FAILED(er)) TVPThrowExceptionMessage(TVPSpecifyOption, TJS_W("time"));
		if(time < 2) time = 2; // too small time may cause problem

		// retrieve "from" option
		tTVPScrollTransFrom from;
		er = options->GetAsNumber(TJS_W("from"), &value);
		if(TJS_FAILED(er)) from = sttLeft; else from = (tTVPScrollTransFrom)value;

		// retrieve "stay" option
		tTVPScrollTransStay stay;
		er = options->GetAsNumber(TJS_W("stay"), &value);
		if(TJS_FAILED(er)) stay = ststNoStay; else stay = (tTVPScrollTransStay)value;

		// determine maximum phase count
		tjs_int maxphase = (from == sttLeft || from == sttRight)?src1w:src2h;

		// create a transition object
		iTVPBaseTransHandler *ret;
		ret =  (iTVPBaseTransHandler *)
			(new tTVPScrollTransHandler(options, layertype, time, from,
				stay, maxphase));
		return ret;
	}
};
//---------------------------------------------------------------------------
struct tTVPScrollDivisibleData : public tTVPDivisibleData
{
	void Blend(tTVPScrollTransFrom from, tTVPScrollTransStay stay, tjs_int phase);
};
//---------------------------------------------------------------------------
void tTVPScrollTransHandler::Blend(tTVPDivisibleData *data)
{
	((tTVPScrollDivisibleData*)data)->Blend(From, Stay, Phase);
}
//---------------------------------------------------------------------------
void tTVPScrollDivisibleData::Blend(tTVPScrollTransFrom from,
	tTVPScrollTransStay stay, tjs_int phase)
{
	// scroll the image
	tjs_int imagewidth;
	tjs_int imageheight;
	Src1->GetWidth(&imagewidth);
	Src1->GetHeight(&imageheight);
	tjs_int src1left = 0;
	tjs_int src1top = 0;
	tjs_int src2left = 0;
	tjs_int src2top = 0;

	switch(from)
	{
	case sttLeft:
		if(stay == ststNoStay)
			src1left = phase, src2left = phase - imagewidth;
		else if(stay == ststStayDest)
			src2left = phase - imagewidth;
		else if(stay == ststStaySrc)
			src1left = phase;
		break;
	case sttTop:
		if(stay == ststNoStay)
			src1top = phase, src2top = phase - imageheight;
		else if(stay == ststStayDest)
			src2top = phase - imageheight;
		else if(stay == ststStaySrc)
			src1top = phase;
		break;
	case sttRight:
		if(stay == ststNoStay)
			src1left = -phase, src2left = imagewidth - phase;
		else if(stay == ststStayDest)
			src2left = imagewidth - phase;
		else if(stay == ststStaySrc)
			src1left = -phase;
		break;
	case sttBottom:
		if(stay == ststNoStay)
			src1top = -phase, src2top = imageheight - phase;
		else if(stay == ststStayDest)
			src2top = imageheight - phase;
		else if(stay == ststStaySrc)
			src1top = -phase;
		break;
	}


	tTVPRect rdest(Left, Top, Width+Left, Height+Top);
	tTVPRect rs1(src1left, src1top, imagewidth + src1left, imageheight + src1top);
	tTVPRect rs2(src2left, src2top, imagewidth + src2left, imageheight + src2top);
	if(stay == ststNoStay)
	{
		// both layers have no priority than another.
		// nothing to do.
	}
	else if(stay == ststStayDest)
	{
		// Src2 has priority.
		if(from == sttLeft || from == sttRight)
		{
			if(rs2.right >= rs1.left && rs2.right < rs1.right)
				rs1.left = rs2.right;
			if(rs2.left >= rs1.left && rs2.left < rs1.right)
				rs1.right = rs2.left;
		}
		else
		{
			if(rs2.bottom >= rs1.top && rs2.bottom < rs1.bottom)
				rs1.top = rs2.bottom;
			if(rs2.top >= rs1.top && rs2.top < rs1.bottom)
				rs1.bottom = rs2.top;
		}
	}
	else if(stay == ststStaySrc)
	{
		// Src1 has priority.
		if(from == sttLeft || from == sttRight)
		{
			if(rs1.right >= rs2.left && rs1.right < rs2.right)
				rs2.left = rs1.right;
			if(rs1.left >= rs2.left && rs1.left < rs2.right)
				rs2.right = rs1.left;
		}
		else
		{
			if(rs1.bottom >= rs2.top && rs1.bottom < rs2.bottom)
				rs2.top = rs1.bottom;
			if(rs1.top >= rs2.top && rs1.top < rs2.bottom)
				rs2.bottom = rs1.top;
		}
	}

	// copy to destination image
	tTVPRect d;
	if(TVPIntersectRect(&d, rdest, rs1))
	{
		tjs_int dl = d.left - Left + DestLeft, dt = d.top - Top + DestTop;
		d.add_offsets(-src1left, -src1top);
		TVPSLPCopyRect(Dest, dl, dt, Src1, d);
	}
	if(TVPIntersectRect(&d, rdest, rs2))
	{
		tjs_int dl = d.left - Left + DestLeft, dt = d.top - Top + DestTop;
		d.add_offsets(-src2left, -src2top);
		TVPSLPCopyRect(Dest, dl, dt, Src2, d);
	}
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
static void TVPRegisterDefaultTransHandlerProvider()
{
	// register default internal transition handler providers.

	iTVPTransHandlerProvider *prov;

	prov = new tTVPCrossFadeTransHandlerProvider();
	TVPAddTransHandlerProvider(prov);
	prov->Release();

	prov = new tTVPUniversalTransHandlerProvider();
	TVPAddTransHandlerProvider(prov);
	prov->Release();

	prov = new tTVPScrollTransHandlerProvider();
	TVPAddTransHandlerProvider(prov);
	prov->Release();
}
//---------------------------------------------------------------------------

