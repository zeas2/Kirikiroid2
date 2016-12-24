//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Image resampler
//---------------------------------------------------------------------------
#ifndef ResamplerH
#define ResamplerH


//---------------------------------------------------------------------------
class iTVPBaseBitmap;
extern void TVPResampleImage(const tTVPRect &cliprect, iTVPBaseBitmap *dest, const tTVPRect &destrect, const iTVPBaseBitmap *src, const tTVPRect &srcrect,
	tTVPBBStretchType type, tjs_real typeopt, tTVPBBBltMethod method, tjs_int opa, bool hda);
//---------------------------------------------------------------------------

#endif