
#ifndef __FREE_TYPE_FONT_RASTERIZER_H__
#define __FREE_TYPE_FONT_RASTERIZER_H__

#include "tjsCommHead.h"
#include "CharacterData.h"
#include "FontRasterizer.h"

class FreeTypeFontRasterizer : public FontRasterizer {
	tjs_int RefCount;
	class tFreeTypeFace* Face; //!< Faceオブジェクト
	class tFreeTypeFace* FaceFallback = nullptr;
	class tTVPNativeBaseBitmap * LastBitmap;
	tTVPFont CurrentFont;
	void ApplyFallbackFace();

public:
	FreeTypeFontRasterizer();
	virtual ~FreeTypeFontRasterizer();
	void AddRef();
	void Release();
	void ApplyFont( class tTVPNativeBaseBitmap *bmp, bool force );
	void ApplyFont( const struct tTVPFont& font );
	void GetTextExtent(tjs_char ch, tjs_int &w, tjs_int &h);
	tjs_int GetAscentHeight();
	tTVPCharacterData* GetBitmap( const tTVPFontAndCharacterData & font, tjs_int aofsx, tjs_int aofsy );
	void GetGlyphDrawRect( const ttstr & text, struct tTVPRect& area );
};

#endif // __FREE_TYPE_FONT_RASTERIZER_H__
