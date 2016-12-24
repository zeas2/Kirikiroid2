//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "tTVPDrawable" definition
//---------------------------------------------------------------------------

#ifndef __DRAWABLE_H__
#define __DRAWABLE_H__


/*[*/
//---------------------------------------------------------------------------
// layer / blending types
//---------------------------------------------------------------------------
enum tTVPLayerType
{
	ltBinder = 0,
	ltCoverRect = 1,
	ltOpaque = 1, // the same as ltCoverRect
	ltTransparent = 2, // alpha blend
	ltAlpha = 2, // the same as ltTransparent
	ltAdditive = 3,
	ltSubtractive = 4,
	ltMultiplicative = 5,
	ltEffect = 6,
	ltFilter = 7,
	ltDodge = 8,
	ltDarken = 9,
	ltLighten = 10,
	ltScreen = 11,
	ltAddAlpha = 12, // additive alpha blend
	ltPsNormal = 13,
	ltPsAdditive = 14,
	ltPsSubtractive = 15,
	ltPsMultiplicative = 16,
	ltPsScreen = 17,
	ltPsOverlay = 18,
	ltPsHardLight = 19,
	ltPsSoftLight = 20,
	ltPsColorDodge = 21,
	ltPsColorDodge5 = 22,
	ltPsColorBurn = 23,
	ltPsLighten = 24,
	ltPsDarken = 25,
	ltPsDifference = 26,
	ltPsDifference5 = 27,
	ltPsExclusion = 28
};
//---------------------------------------------------------------------------
static bool inline TVPIsTypeUsingAlpha(tTVPLayerType type)
	{
		return
			type == ltAlpha				||
			type == ltPsNormal			||
			type == ltPsAdditive		||
			type == ltPsSubtractive		||
			type == ltPsMultiplicative	||
			type == ltPsScreen			||
			type == ltPsOverlay			||
			type == ltPsHardLight		||
			type == ltPsSoftLight		||
			type == ltPsColorDodge		||
			type == ltPsColorDodge5		||
			type == ltPsColorBurn		||
			type == ltPsLighten			||
			type == ltPsDarken			||
			type == ltPsDifference		||
			type == ltPsDifference5		||
			type == ltPsExclusion		;
	}

static bool inline TVPIsTypeUsingAddAlpha(tTVPLayerType type)
	{
		return type == ltAddAlpha;
	}

static bool inline TVPIsTypeUsingAlphaChannel(tTVPLayerType type)
	{
		return
			TVPIsTypeUsingAddAlpha(type) ||
			TVPIsTypeUsingAlpha(type);
	}
//---------------------------------------------------------------------------



/*]*/


//---------------------------------------------------------------------------
// tTVPDrawable definition
//---------------------------------------------------------------------------
class tTVPBaseTexture;
struct tTVPRect;
class tTVPDrawable
{
public:
	// base class of draw-able objects.
	virtual tTVPBaseTexture * GetDrawTargetBitmap(const tTVPRect &rect,
		tTVPRect &cliprect) = 0;
		// returns target bitmap which has given size
		// (rect.get_width() * rect.get_height()).
		// put actually to be drawn rectangle to "cliprect"

	virtual tTVPLayerType GetTargetLayerType() = 0;
		// returns target layer type

	virtual void DrawCompleted(const tTVPRect &destrect,
		tTVPBaseTexture *bmp, const tTVPRect &cliprect,
		tTVPLayerType type, tjs_int opacity) = 0;
		// call this when the drawing is completed, passing
		// the bitmap containing the image, and its clip region.
};
//---------------------------------------------------------------------------



#endif
