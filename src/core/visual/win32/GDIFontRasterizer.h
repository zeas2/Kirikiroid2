
#ifndef __GDI_FONT_RASTERIZER_H__
#define __GDI_FONT_RASTERIZER_H__

#include "tjsCommHead.h"
#include "CharacterData.h"
#include "FontRasterizer.h"

class GDIFontRasterizer : public FontRasterizer {
	tjs_int RefCount;
	class tTVPSysFont* FontDC;
	class tTVPSysFont* NonBoldFontDC;
	class tTVPNativeBaseBitmap* LastBitmap;
	LOGFONT CurentLOGFONT;

	bool ChUseResampling; // whether to use resampling anti-aliasing
	bool ChAntialiasMethodInit;
	
	enum tTVPChAntialiasMethod
	{	camAPI,	camResample4, camResample8, camSubpixelRGB, camSubpixelBGR };
	tTVPChAntialiasMethod ChAntialiasMethod;

	void InitChAntialiasMethod();
public:
	GDIFontRasterizer();
	virtual ~GDIFontRasterizer();
	void AddRef();
	void Release();
	void ApplyFont( class tTVPNativeBaseBitmap *bmp, bool force );
	void ApplyFont( const struct tTVPFont& font );
	void GetTextExtent(tjs_char ch, tjs_int &w, tjs_int &h);
	tjs_int GetAscentHeight();
	class tTVPCharacterData* GetBitmap( const struct tTVPFontAndCharacterData & font, tjs_int aofsx, tjs_int aofsy );
	void GetGlyphDrawRect( const ttstr & text, struct tTVPRect& area );
};

#endif // __GDI_FONT_RASTERIZER_H__
