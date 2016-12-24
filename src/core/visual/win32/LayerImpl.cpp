//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Layer Management
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "LayerImpl.h"
#include "MsgIntf.h"

#include "TVPColor.h"

//---------------------------------------------------------------------------
//  convert color identifier or TVP system color to/from actual color
//---------------------------------------------------------------------------
tjs_uint32 TVPToActualColor(tjs_uint32 color)
{
	if(color & 0xff000000)
	{
		color = ColorToRGB( color ); // system color to RGB
		// convert byte order to 0xRRGGBB since ColorToRGB's return value is in
		// a format of 0xBBGGRR.
		return ((color&0xff)<<16) + (color&0xff00) + ((color&0xff0000)>>16);
	}
	else
	{
		return color;
	}
}
//---------------------------------------------------------------------------
tjs_uint32 TVPFromActualColor(tjs_uint32 color)
{
	color &= 0xffffff;
	return color;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSNI_Layer
//---------------------------------------------------------------------------
tTJSNI_Layer::tTJSNI_Layer(void)
{
}
//---------------------------------------------------------------------------
tTJSNI_Layer::~tTJSNI_Layer()
{
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSNI_Layer::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	return tTJSNI_BaseLayer::Construct(numparams, param, tjs_obj);
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD
tTJSNI_Layer::Invalidate()
{
	tTJSNI_BaseLayer::Invalidate();
}
//---------------------------------------------------------------------------
#if 0
#pragma pack(push, 1)
HRGN tTJSNI_Layer::CreateMaskRgn(tjs_uint threshold)
{
	// create a region according with the mask value of the layer bitmap.
	// based on  <builder-ctl@venus13.aid.kyushu-id.ac.jp> builder ML 14870
	// Mr. Nakamura's code

	struct tRegionData
	{
		DWORD Size, iType, Count, RgnSize;
		RECT Bounds;
		RECT Rects[4000];

		HRGN CreateRegion()
		{
			XFORM Form;
			Form.eM11 = 1; Form.eM12 = 0; Form.eM21 = 0; Form.eM22 = 1;
			Form.eDx = 0; Form.eDy = 0;
			RgnSize = Count * 16;
			return ExtCreateRegion(&Form, 32 + Count * 16, (RGNDATA *)this);
		}
	};

	int i, j, w, h;
	int sx, ex;
	tTVPBaseBitmap *MainImage = GetMainImage();
	const tjs_uint32 *pRGBQ;
	HRGN Rgn;
	std::vector<HRGN> RgnList;
	tRegionData Data;

	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	w = MainImage->GetWidth(); h = MainImage->GetHeight();

	Data.Size = 32; Data.iType = RDH_RECTANGLES;
	Data.Count = 0;
	Data.Bounds.left = 0;
	Data.Bounds.top = 0;
	Data.Bounds.right = w;
	Data.Bounds.bottom = h;

	for (j = 0; j < h; j++)
	{
		pRGBQ = (const tjs_uint32 *)MainImage->GetScanLine(j);
		sx = -1; /*ex = -1;*/

		for (i = 0; i < w; i++)
		{

			if ( (*pRGBQ>>24) >= threshold )
			{
				if ( sx == -1 ) sx = i;
			}
			else
			{
				ex = i;
				if (sx != -1 )
				{
					if (Data.Count == 4000)
					{
						Rgn = Data.CreateRegion();
						RgnList.push_back(Rgn);
						Data.Count = 0;
					}
					RECT &r = Data.Rects[Data.Count];
					r.left = sx;
					r.top = j;
					r.right = ex;
					r.bottom = j + 1;
					Data.Count++;
					sx = -1; /*ex = -1;*/
				}
			}
			pRGBQ++;
		}
		if ( sx != -1)
		{
			if (Data.Count == 4000)
			{
				Rgn = Data.CreateRegion();
				RgnList.push_back(Rgn);
				Data.Count = 0;
			}
			RECT &r = Data.Rects[Data.Count];
			r.left = sx;
			r.top = j;
			r.right = w;
			r.bottom = j + 1;
			Data.Count++;
		}
	}

	if (Data.Count > 0)
	{
		Rgn = Data.CreateRegion();
		RgnList.push_back(Rgn);
	}

	Rgn = CreateRectRgn(0, 0, 0, 0);
	for (tjs_uint i = 0; i < RgnList.size(); i++)
	{
		CombineRgn(Rgn, Rgn, HRGN(RgnList[i]), RGN_OR);
		DeleteObject(HRGN(RgnList[i]));
	}

	return Rgn;
}
#pragma pack(pop)
//---------------------------------------------------------------------------
#endif



//---------------------------------------------------------------------------
// tTJSNC_Layer::CreateNativeInstance : returns proper instance object
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_Layer::CreateNativeInstance()
{
	return new tTJSNI_Layer();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPCreateNativeClass_Layer
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Layer()
{
	return new tTJSNC_Layer();
}
//---------------------------------------------------------------------------

// utility functions for custom plugins
tTJSNI_Layer* tTJSNI_Layer::FromVariant(const tTJSVariant& var) {
	return FromObject(var.AsObjectNoAddRef());
}

tTJSNI_Layer* tTJSNI_Layer::FromObject(iTJSDispatch2* obj)
{
	tTJSNI_Layer * lay = nullptr;
	if (obj) {
		if (TJS_FAILED(obj->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&lay)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	return lay;
}
