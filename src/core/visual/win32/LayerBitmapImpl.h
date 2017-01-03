//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Base Layer Bitmap implementation
//---------------------------------------------------------------------------
#ifndef LayerBitmapImplH
#define LayerBitmapImplH

#include "tvpfontstruc.h"
#include "ComplexRect.h"

#include "BitmapInfomation.h"

//---------------------------------------------------------------------------
extern void TVPSetFontCacheForLowMem();
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTVPBitmap : internal bitmap object
//---------------------------------------------------------------------------
class tTVPBitmap
{
public:
	static const tjs_int DEFAULT_PALETTE_COUNT = 256;
private:
	tjs_int RefCount;

	void * Bits; // pointer to bitmap bits
	BitmapInfomation *BitmapInfo; // DIB information

	tjs_int PitchBytes; // bytes required in a line
	tjs_int PitchStep; // step bytes to next(below) line
	tjs_int Width; // actual width
	tjs_int Height; // actual height

	tjs_int ActualPalCount;
	tjs_uint* Palette;

public:
	tTVPBitmap(tjs_uint width, tjs_uint height, tjs_uint bpp);
	// for async load
	// @param bits : tTVPBitmapBitsAlloc::Allocで確保したものを使用すること
	tTVPBitmap(tjs_uint width, tjs_uint height, tjs_uint bpp, void* bits);

	tTVPBitmap(const tTVPBitmap & r);

	~tTVPBitmap();

	void Allocate(tjs_uint width, tjs_uint height, tjs_uint bpp);

	void AddRef(void)
	{
		RefCount ++;
	}

	void Release(void)
	{
		if(RefCount == 1)
			delete this;
		else
			RefCount--;
	}

	tjs_uint GetWidth() const { return Width; }
	tjs_uint GetHeight() const { return Height; }

	tjs_uint GetBPP() const { return BitmapInfo->GetBPP(); }
	bool Is32bit() const { return BitmapInfo->Is32bit(); }
	bool Is8bit() const { return BitmapInfo->Is8bit(); }


	void * GetScanLine(tjs_uint l) const;

	tjs_int GetPitch() const { return PitchStep; }

	bool IsIndependent() const { return RefCount == 1; }

	const BitmapInfomation* GetBitmapInfomation() const { return BitmapInfo; }
#ifdef _WIN32
	const TVPBITMAPINFO * GetBITMAPINFO() const { return BitmapInfo->GetBITMAPINFO(); }
	const TVPBITMAPINFOHEADER * GetBITMAPINFOHEADER() const { return (const TVPBITMAPINFOHEADER*)(BitmapInfo->GetBITMAPINFO()); }
#endif

	const void * GetBits() const { return Bits; }
	const tjs_uint* GetPalette() const { return Palette; };
	tjs_uint* GetPalette() { return Palette; };
	tjs_uint GetPaletteCount() const { return ActualPalCount; };
	void SetPaletteCount( tjs_uint count );

	bool IsOpaque = false;
};
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPNativeBaseBitmap
//---------------------------------------------------------------------------
class iTVPTexture2D;
class tTVPComplexRect;
class tTVPCharacterData;
struct tTVPDrawTextData;
class tTVPPrerenderedFont;
class tTVPNativeBaseBitmap
{
public:
	tTVPNativeBaseBitmap(/*tjs_uint w, tjs_uint h, tjs_uint bpp*/);
	tTVPNativeBaseBitmap(const tTVPNativeBaseBitmap & r);
	virtual ~tTVPNativeBaseBitmap();

	/* metrics */
	tjs_uint GetWidth() const ;
	void SetWidth(tjs_uint w);
	tjs_uint GetHeight() const;
	void SetHeight(tjs_uint h);

	void SetSize(tjs_uint w, tjs_uint h, bool keepimage = true);
	// for async load
	// @param bits : tTVPBitmapBitsAlloc::Allocで確保したものを使用すること
	void SetSizeAndImageBuffer(tTVPBitmap* bmp);

	/* color depth */
	tjs_uint GetBPP() const;

	bool Is32BPP() const;
	bool Is8BPP() const;
	bool IsOpaque() const;

	/* assign */
	bool Assign(const tTVPNativeBaseBitmap &rhs) ;
	bool AssignBitmap(const tTVPNativeBaseBitmap &rhs) ; // assigns only bitmap
	bool AssignTexture(iTVPTexture2D *tex);

	/* scan line */
	const void * GetScanLine(tjs_uint l) const;
	void * GetScanLineForWrite(tjs_uint l);
	tjs_int GetPitchBytes() const;


	/* object lifetime management */
	void Independ();
	void IndependNoCopy();
	void Recreate();
	void Recreate(tjs_uint w, tjs_uint h, tjs_uint bpp);

	bool IsIndependent() const;

	/* other utilities */
	iTVPTexture2D * GetTexture() const { return Bitmap; }
	virtual iTVPTexture2D *GetTextureForRender(bool isBlendTarget, const tTVPRect *rc);
#if 0
	tjs_uint GetPalette( tjs_uint index ) const;
	void SetPalette( tjs_uint index, tjs_uint color );
#endif
	/* font and text functions */
private:
	tTVPFont Font;
	bool FontChanged;
	tjs_int GlobalFontState;

	// v--- these can be recreated in ApplyFont if FontChanged flag is set
	tTVPPrerenderedFont *PrerenderedFont;
	tjs_int AscentOfsX;
	tjs_int AscentOfsY;
	double RadianAngle;
	tjs_uint32 FontHash;
	// ^---

	void ApplyFont();

public:
	void SetFont(const tTVPFont &font);
	const tTVPFont & GetFont() const { return Font; };

	void GetFontList(tjs_uint32 flags, std::vector<ttstr> &list);

	void MapPrerenderedFont(const ttstr & storage);
	void UnmapPrerenderedFont();

private:
    bool InternalBlendText(tTVPCharacterData *data, tTVPDrawTextData *dtdata, tjs_uint32 color, const tTVPRect &srect, tTVPRect &drect);

	bool InternalDrawText(tTVPCharacterData *data, tjs_int x,
		tjs_int y, tjs_uint32 shadowcolor,tTVPDrawTextData *dtdata, tTVPRect &drect);

public:
	void DrawTextSingle(const tTVPRect &destrect, tjs_int x, tjs_int y, const ttstr &text,
		tjs_uint32 color, tTVPBBBltMethod bltmode, tjs_int opa = 255,
			bool holdalpha = true, bool aa = true, tjs_int shlevel = 0,
			tjs_uint32 shadowcolor = 0,
			tjs_int shwidth = 0, tjs_int shofsx = 0, tjs_int shofsy = 0,
			tTVPComplexRect *updaterects = NULL);
	void DrawTextMultiple(const tTVPRect &destrect, tjs_int x, tjs_int y, const ttstr &text,
		tjs_uint32 color, tTVPBBBltMethod bltmode, tjs_int opa = 255,
			bool holdalpha = true, bool aa = true, tjs_int shlevel = 0,
			tjs_uint32 shadowcolor = 0,
			tjs_int shwidth = 0, tjs_int shofsx = 0, tjs_int shofsy = 0,
			tTVPComplexRect *updaterects = NULL);
	void DrawText(const tTVPRect &destrect, tjs_int x, tjs_int y, const ttstr &text,
		tjs_uint32 color, tTVPBBBltMethod bltmode, tjs_int opa = 255,
			bool holdalpha = true, bool aa = true, tjs_int shlevel = 0,
			tjs_uint32 shadowcolor = 0,
			tjs_int shwidth = 0, tjs_int shofsx = 0, tjs_int shofsy = 0,
			tTVPComplexRect *updaterects = NULL)
	{
		tjs_int len = text.GetLen();
		if(len == 0) return;
		if(len >= 2)
			DrawTextMultiple(
				destrect, x, y, text,
				color, bltmode, opa,
				holdalpha, aa, shlevel,
				shadowcolor, shwidth, shofsx, shofsy,
				updaterects);
		else    /* if len == 1 */
			DrawTextSingle(
				destrect, x, y, text,
				color, bltmode, opa,
				holdalpha, aa, shlevel,
				shadowcolor, shwidth, shofsx, shofsy,
				updaterects);
	}
	void DrawGlyph(iTJSDispatch2* glyph, const tTVPRect &destrect, tjs_int x, tjs_int y,
			tjs_uint32 color, tTVPBBBltMethod bltmode, tjs_int opa = 255,
			bool holdalpha = true, bool aa = true, tjs_int shlevel = 0,
			tjs_uint32 shadowcolor = 0,
			tjs_int shwidth = 0, tjs_int shofsx = 0, tjs_int shofsy = 0,
			tTVPComplexRect *updaterects = NULL);


private:
	tjs_int TextWidth;
	tjs_int TextHeight;
	ttstr CachedText;

	void GetTextSize(const ttstr & text);

public:
	tjs_int GetTextWidth(const ttstr & text);
	tjs_int GetTextHeight(const ttstr & text);
	double GetEscWidthX(const ttstr & text);
	double GetEscWidthY(const ttstr & text);
	double GetEscHeightX(const ttstr & text);
	double GetEscHeightY(const ttstr & text);
	void GetFontGlyphDrawRect( const ttstr & text, struct tTVPRect& area );

protected:
	//tTVPBitmap *Bitmap;
	iTVPTexture2D *Bitmap;
public:
	void operator =(const tTVPNativeBaseBitmap &rhs) { Assign(rhs); }
	virtual class iTVPRenderManager* GetRenderManager() = 0;
};
//---------------------------------------------------------------------------


#endif
