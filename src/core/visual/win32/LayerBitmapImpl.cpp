//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Base Layer Bitmap implementation
//---------------------------------------------------------------------------
#define _USE_MATH_DEFINES
#include "tjsCommHead.h"

#include <memory>
#include <stdlib.h>
#include <math.h>

#include "LayerBitmapIntf.h"
#include "LayerBitmapImpl.h"
#include "MsgIntf.h"
#include "ComplexRect.h"
#include "tvpgl.h"
#include "tjsHashSearch.h"
#include "EventIntf.h"
#include "SysInitImpl.h"
#include "StorageIntf.h"
#include "DebugIntf.h"
//#include "WindowFormUnit.h"
void TVPInitWindowOptions();
#include "UtilStreams.h"
#include "ConfigManager/IndividualConfigManager.h"

//#include "FontSelectFormUnit.h"

#include "StringUtil.h"
//#include "TVPSysFont.h"
#include "CharacterData.h"
#include "PrerenderedFont.h"
#include "FontSystem.h"
#include "visual/FreeType.h"
#include "FreeTypeFontRasterizer.h"
//#include "GDIFontRasterizer.h"
#include "BitmapBitsAlloc.h"
#include "RenderManager.h"

//---------------------------------------------------------------------------
// prototypes
//---------------------------------------------------------------------------
void TVPClearFontCache();
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// default FONT retrieve function
//---------------------------------------------------------------------------
FontSystem* TVPFontSystem = NULL;
static tjs_int TVPGlobalFontStateMagic = 0;
	// this is for checking global font status' change


enum {
	FONT_RASTER_FREE_TYPE,
//	FONT_RASTER_GDI,
	FONT_RASTER_EOT
};
static FontRasterizer* TVPFontRasterizers[FONT_RASTER_EOT];
static bool TVPFontRasterizersInit = false;
static tjs_int TVPCurrentFontRasterizers = FONT_RASTER_FREE_TYPE;
//static tjs_int TVPCurrentFontRasterizers = FONT_RASTER_GDI;
void TVPInializeFontRasterizers() {
	if( TVPFontRasterizersInit == false ) {
		TVPFontRasterizers[FONT_RASTER_FREE_TYPE] = new FreeTypeFontRasterizer();
//		TVPFontRasterizers[FONT_RASTER_GDI] = new GDIFontRasterizer();

		TVPFontSystem = new FontSystem();
		TVPFontRasterizersInit = true;
	}
}
void TVPUninitializeFontRasterizers() {
	for( tjs_int i = 0; i < FONT_RASTER_EOT; i++ ) {
		if( TVPFontRasterizers[i] ) {
			TVPFontRasterizers[i]->Release();
			TVPFontRasterizers[i] = NULL;
		}
	}
	if( TVPFontSystem ) {
		delete TVPFontSystem;
		TVPFontSystem = NULL;
	}
}
static tTVPAtExit
	TVPUninitializeFontRaster(TVP_ATEXIT_PRI_RELEASE, TVPUninitializeFontRasterizers);

void TVPSetFontRasterizer( tjs_int index ) {
	if( TVPCurrentFontRasterizers != index && index >= 0 && index < FONT_RASTER_EOT ) {
		TVPCurrentFontRasterizers = index;
		TVPClearFontCache(); // ラスタライザが切り替わる時、キャッシュはクリアしてしまう
		TVPGlobalFontStateMagic++; // ApplyFont が走るようにする
	}
}
tjs_int TVPGetFontRasterizer() {
	return TVPCurrentFontRasterizers;
}
FontRasterizer* GetCurrentRasterizer() {
	return TVPFontRasterizers[TVPCurrentFontRasterizers];
}

//---------------------------------------------------------------------------
#define TVP_CH_MAX_CACHE_COUNT 1300
#define TVP_CH_MAX_CACHE_COUNT_LOW 100
#define TVP_CH_MAX_CACHE_HASH_SIZE 512
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// Pre-rendered font management
//---------------------------------------------------------------------------
tTJSHashTable<ttstr, tTVPPrerenderedFont *> TVPPrerenderedFonts;



//---------------------------------------------------------------------------
// tTVPPrerenderedFontMap
//---------------------------------------------------------------------------
struct tTVPPrerenderedFontMap
{
	tTVPFont Font; // mapped font
	tTVPPrerenderedFont * Object; // prerendered font object
};
static std::vector<tTVPPrerenderedFontMap> TVPPrerenderedFontMapVector;
//---------------------------------------------------------------------------
void TVPMapPrerenderedFont(const tTVPFont & font, const ttstr & storage)
{
	// map specified font to specified prerendered font
	ttstr fn = TVPSearchPlacedPath(storage);

	// search or retrieve specified storage
	tTVPPrerenderedFont * object;

	tTVPPrerenderedFont ** found = TVPPrerenderedFonts.Find(fn);
	if(!found)
	{
		// not yet exist; create
		object = new tTVPPrerenderedFont(fn);
	}
	else
	{
		// already exist
		object = *found;
		object->AddRef();
	}

	// search existing mapped font
	std::vector<tTVPPrerenderedFontMap>::iterator i;
	for(i = TVPPrerenderedFontMapVector.begin();
		i !=TVPPrerenderedFontMapVector.end(); i++)
	{
		if(i->Font == font)
		{
			// found font
			// replace existing
			i->Object->Release();
			i->Object = object;
			break;
		}
	}
	if(i == TVPPrerenderedFontMapVector.end())
	{
		// not found
		tTVPPrerenderedFontMap map;
		map.Font = font;
		map.Object = object;
		TVPPrerenderedFontMapVector.push_back(map); // add
	}

	TVPGlobalFontStateMagic ++; // increase magic number

	TVPClearFontCache(); // clear font cache
}
//---------------------------------------------------------------------------
void TVPUnmapPrerenderedFont(const tTVPFont & font)
{
	// unmap specified font
	std::vector<tTVPPrerenderedFontMap>::iterator i;
	for(i = TVPPrerenderedFontMapVector.begin();
		i !=TVPPrerenderedFontMapVector.end(); i++)
	{
		if(i->Font == font)
		{
			// found font
			// replace existing
			i->Object->Release();
			TVPPrerenderedFontMapVector.erase(i);
			TVPGlobalFontStateMagic ++; // increase magic number
			TVPClearFontCache();
			return;
		}
	}
}
//---------------------------------------------------------------------------
static void TVPUnmapAllPrerenderedFonts()
{
	// unmap all prerendered fonts
	std::vector<tTVPPrerenderedFontMap>::iterator i;
	for(i = TVPPrerenderedFontMapVector.begin();
		i !=TVPPrerenderedFontMapVector.end(); i++)
	{
		i->Object->Release();
	}
	TVPPrerenderedFontMapVector.clear();
	TVPGlobalFontStateMagic ++; // increase magic number
}
//---------------------------------------------------------------------------
static tTVPAtExit TVPUnmapAllPrerenderedFontsAtExit
	(TVP_ATEXIT_PRI_PREPARE, TVPUnmapAllPrerenderedFonts);
//---------------------------------------------------------------------------
static tTVPPrerenderedFont * TVPGetPrerenderedMappedFont(const tTVPFont &font)
{
	// search mapped prerendered font
	std::vector<tTVPPrerenderedFontMap>::iterator i;
	for(i = TVPPrerenderedFontMapVector.begin();
		i !=TVPPrerenderedFontMapVector.end(); i++)
	{
		if(i->Font == font)
		{
			// found font
			// replace existing
			i->Object->AddRef();

			// note that the object is AddRefed
			return i->Object;
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
typedef tTJSRefHolder<tTVPCharacterData> tTVPCharacterDataHolder;

typedef
tTJSHashCache<tTVPFontAndCharacterData, tTVPCharacterDataHolder,
	tTVPFontHashFunc, TVP_CH_MAX_CACHE_HASH_SIZE> tTVPFontCache;
tTVPFontCache TVPFontCache(TVP_CH_MAX_CACHE_COUNT);
//---------------------------------------------------------------------------
void TVPSetFontCacheForLowMem()
{
	// set character cache limit
	TVPFontCache.SetMaxCount(TVP_CH_MAX_CACHE_COUNT_LOW);
}
//---------------------------------------------------------------------------
void TVPClearFontCache()
{
	TVPFontCache.Clear();
}
//---------------------------------------------------------------------------
struct tTVPClearFontCacheCallback : public tTVPCompactEventCallbackIntf
{
	virtual void TJS_INTF_METHOD OnCompact(tjs_int level)
	{
		if(level >= TVP_COMPACT_LEVEL_MINIMIZE)
		{
			// clear the font cache on application minimize
			TVPClearFontCache();
		}
	}
} static TVPClearFontCacheCallback;
static bool TVPClearFontCacheCallbackInit = false;
//---------------------------------------------------------------------------
static tTVPCharacterData * TVPGetCharacter(const tTVPFontAndCharacterData & font,
	tTVPNativeBaseBitmap *bmp, tTVPPrerenderedFont *pfont, tjs_int aofsx, tjs_int aofsy)
{
	// returns specified character data.
	// draw a character if needed.

	// compact interface initialization
	if(!TVPClearFontCacheCallbackInit)
	{
		TVPAddCompactEventHook(&TVPClearFontCacheCallback);
		TVPClearFontCacheCallbackInit = true;
	}

	// make hash and search over cache
	tjs_uint32 hash = tTVPFontCache::MakeHash(font);

	tTVPCharacterDataHolder * ptr = TVPFontCache.FindAndTouchWithHash(font, hash);
	if(ptr)
	{
		// found in the cache
		return ptr->GetObject();
	}

	// not found in the cache

	// look prerendered font
	const tTVPPrerenderedCharacterItem *pitem = NULL;
	if(pfont)
		pitem = pfont->Find(font.Character);

	if(pitem)
	{
		// prerendered font
		tTVPCharacterData *data = new tTVPCharacterData();
		data->BlackBoxX = pitem->Width;
		data->BlackBoxY = pitem->Height;
		data->Metrics.CellIncX = pitem->IncX;
		data->Metrics.CellIncY = pitem->IncY;
		data->OriginX = pitem->OriginX + aofsx;
		data->OriginY = -pitem->OriginY + aofsy;

		data->Antialiased = font.Antialiased;

		data->FullColored = false;

		data->Blured = font.Blured;
		data->BlurWidth = font.BlurWidth;
		data->BlurLevel = font.BlurLevel;

		try
		{
			if(data->BlackBoxX && data->BlackBoxY)
			{
				// render
				tjs_int newpitch =  (((pitem->Width -1)>>2)+1)<<2;
				data->Pitch = newpitch;

				data->Alloc(newpitch * data->BlackBoxY);

				pfont->Retrieve(pitem, data->GetData(), newpitch);

				// apply blur
				if(font.Blured) data->Blur(); // nasty ...

				// add to hash table
				tTVPCharacterDataHolder holder(data);
				TVPFontCache.AddWithHash(font, hash, holder);
			}
		}
		catch(...)
		{
			data->Release();
			throw;
		}

		return data;
	}
	else
	{
		// render font
		tTVPCharacterData *data = GetCurrentRasterizer()->GetBitmap( font, aofsx, aofsy );

		// add to hash table
		tTVPCharacterDataHolder holder(data);
		TVPFontCache.AddWithHash(font, hash, holder);
		return data;
	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPBitmap : internal bitmap object
//---------------------------------------------------------------------------
/*
	important:
	Note that each lines must be started at tjs_uint32 ( 4bytes ) aligned address.
	This is the default Windows bitmap allocate behavior.
*/
tTVPBitmap::tTVPBitmap(tjs_uint width, tjs_uint height, tjs_uint bpp)
{
	// tTVPBitmap constructor

	TVPInitWindowOptions(); // ensure window/bitmap usage options are initialized

	RefCount = 1;

	Allocate(width, height, bpp); // allocate initial bitmap
}
//---------------------------------------------------------------------------
tTVPBitmap::tTVPBitmap(tjs_uint width, tjs_uint height, tjs_uint bpp, void* bits)
{
	// tTVPBitmap constructor
	TVPInitWindowOptions(); // ensure window/bitmap usage options are initialized

	RefCount = 1;

	BitmapInfo = new BitmapInfomation( width, height, bpp );
	Width = width;
	Height = height;
	PitchBytes = BitmapInfo->GetPitchBytes();
	PitchStep = PitchBytes;

	// set bitmap bits
	try
	{
		Bits = bits;
		if( bpp == 8 ) {
			Palette = new tjs_uint[DEFAULT_PALETTE_COUNT];
			ActualPalCount = 0;
		} else {
			Palette = NULL;
			ActualPalCount = 0;
		}
	}
	catch(...)
	{
		delete BitmapInfo;
		BitmapInfo = NULL;
		throw;
	}
}
//---------------------------------------------------------------------------
tTVPBitmap::~tTVPBitmap()
{
	tTVPBitmapBitsAlloc::Free(Bits);
	delete BitmapInfo;
	if( Palette ) delete Palette;
}
//---------------------------------------------------------------------------
tTVPBitmap::tTVPBitmap(const tTVPBitmap & r)
{
	// constructor for cloning bitmap
	TVPInitWindowOptions(); // ensure window/bitmap usage options are initialized

	RefCount = 1;

	// allocate bitmap which has the same metrics to r
	Allocate(r.GetWidth(), r.GetHeight(), r.GetBPP());

	// copy BitmapInfo
	*BitmapInfo = *r.BitmapInfo;

	// copy Bits
	if(r.Bits) memcpy(Bits, r.Bits, r.BitmapInfo->GetImageSize() );
	if(r.Palette) {
		memcpy(Palette, r.Palette, sizeof(tjs_uint)*DEFAULT_PALETTE_COUNT );
		ActualPalCount = r.ActualPalCount;
	}

	// copy pitch
	PitchBytes = r.PitchBytes;
	PitchStep = r.PitchStep;
}
//---------------------------------------------------------------------------
void tTVPBitmap::Allocate(tjs_uint width, tjs_uint height, tjs_uint bpp)
{
	// allocate bitmap bits
	// bpp must be 8 or 32

	// create BITMAPINFO
	BitmapInfo = new BitmapInfomation( width, height, bpp );

	Width = width;
	Height = height;
	PitchBytes = BitmapInfo->GetPitchBytes();
	PitchStep = PitchBytes;

	// allocate bitmap bits
	try
	{
		Bits = tTVPBitmapBitsAlloc::Alloc(BitmapInfo->GetImageSize(), width, height);
		if( bpp == 8 ) {
			Palette = new tjs_uint[DEFAULT_PALETTE_COUNT];
			ActualPalCount = 0;
		} else {
			Palette = NULL;
			ActualPalCount = 0;
		}
	}
	catch(...)
	{
		delete BitmapInfo;
		BitmapInfo = NULL;
		throw;
	}
}
//---------------------------------------------------------------------------
void * tTVPBitmap::GetScanLine(tjs_uint l) const
{
	if((tjs_int)l>=BitmapInfo->GetHeight() )
	{
		TVPThrowExceptionMessage(TVPScanLineRangeOver, ttstr((tjs_int)l),
			ttstr((tjs_int)BitmapInfo->GetHeight()-1));
	}

	return l * PitchBytes + (tjs_uint8*)Bits;
}
//---------------------------------------------------------------------------
void tTVPBitmap::SetPaletteCount( tjs_uint count ) {
	if( !Is8bit() ) TVPThrowExceptionMessage(TVPInvalidOperationFor32BPP);
	if( count >= DEFAULT_PALETTE_COUNT )
		TVPThrowExceptionMessage(TJSRangeError);

	ActualPalCount = count;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPNativeBaseBitmap
//---------------------------------------------------------------------------
tTVPNativeBaseBitmap::tTVPNativeBaseBitmap(/*tjs_uint w, tjs_uint h, tjs_uint bpp*/)
{
	TVPInializeFontRasterizers();
	// TVPFontRasterizer->AddRef(); TODO

	// TVPConstructDefaultFont();
	Font = TVPFontSystem->GetDefaultFont();
	PrerenderedFont = NULL;
	//LogFont = TVPDefaultLOGFONT;
	FontChanged = true;
	GlobalFontState = -1;
	TextWidth = TextHeight = 0;
	//Bitmap = new tTVPBitmap(w, h, bpp);
}
//---------------------------------------------------------------------------
tTVPNativeBaseBitmap::tTVPNativeBaseBitmap(const tTVPNativeBaseBitmap & r)
{
	TVPInializeFontRasterizers();
	// TVPFontRasterizer->AddRef(); TODO

	Bitmap = r.Bitmap;
	if (Bitmap) Bitmap->AddRef();

	Font = r.Font;
	PrerenderedFont = NULL;
	//LogFont = TVPDefaultLOGFONT;
	FontChanged = true;
	TextWidth = TextHeight = 0;
}
//---------------------------------------------------------------------------
tTVPNativeBaseBitmap::~tTVPNativeBaseBitmap()
{
	if (Bitmap) Bitmap->Release();
	if(PrerenderedFont) PrerenderedFont->Release();

	// TVPFontRasterizer->Release(); TODO
}
//---------------------------------------------------------------------------
tjs_uint tTVPNativeBaseBitmap::GetWidth() const
{
	return Bitmap->GetWidth();
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::SetWidth(tjs_uint w)
{
	SetSize(w, Bitmap->GetHeight());
}
//---------------------------------------------------------------------------
tjs_uint tTVPNativeBaseBitmap::GetHeight() const
{
	return Bitmap->GetHeight();
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::SetHeight(tjs_uint h)
{
	SetSize(Bitmap->GetWidth(), h);
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::SetSize(tjs_uint w, tjs_uint h, bool keepimage)
{
	if(Bitmap->GetWidth() != w || Bitmap->GetHeight() != h)
	{
		// create a new bitmap and copy existing bitmap
		iTVPTexture2D *newbitmap;
		if (keepimage)
			newbitmap = GetRenderManager()->CreateTexture2D(w, h, Bitmap);
		else
			newbitmap = GetRenderManager()->CreateTexture2D(nullptr, 0, w, h, Bitmap->GetFormat());
#if 0
		tTVPBitmap *newbitmap = new tTVPBitmap(w, h, Bitmap->GetBPP());

		if(keepimage)
		{
			tjs_int pixelsize = Bitmap->Is32bit() ? 4 : 1;
			tjs_int lh = h < Bitmap->GetHeight() ?
				h : Bitmap->GetHeight();
			tjs_int lw = w < Bitmap->GetWidth() ?
				w : Bitmap->GetWidth();
			tjs_int cs = lw * pixelsize;
			tjs_int i;
			for(i = 0; i < lh; i++)
			{
				void * ds = newbitmap->GetScanLine(i);
				void * ss = Bitmap->GetScanLine(i);

				memcpy(ds, ss, cs);
			}
			if( pixelsize == 1 )
				memcpy(newbitmap->GetPalette(), Bitmap->GetPalette(), sizeof(tjs_uint)*tTVPBitmap::DEFAULT_PALETTE_COUNT);
		}
#endif
		Bitmap->Release();
		Bitmap = newbitmap;

		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::SetSizeAndImageBuffer(tTVPBitmap* bmp)
{
	// create a new bitmap and copy existing bitmap
	iTVPTexture2D *newbitmap = GetRenderManager()->CreateTexture2D(bmp);
	Bitmap->Release();
	Bitmap = newbitmap;
	FontChanged = true;
}
//---------------------------------------------------------------------------
tjs_uint tTVPNativeBaseBitmap::GetBPP() const
{
	switch (Bitmap->GetFormat()) {
	case TVPTextureFormat::Gray:
		return 8;
	case TVPTextureFormat::RGBA:
		return 32;
	case TVPTextureFormat::RGB:
		return 24;
	default:
		// error !
		return 0;
	}
#if 0
	return Bitmap->GetBPP();
#endif
}
//---------------------------------------------------------------------------
bool tTVPNativeBaseBitmap::Is32BPP() const
{
	return Bitmap->GetFormat() == TVPTextureFormat::RGBA;
}
//---------------------------------------------------------------------------
bool tTVPNativeBaseBitmap::Is8BPP() const
{
	return Bitmap->GetFormat() == TVPTextureFormat::Gray;
}
//---------------------------------------------------------------------------
bool tTVPNativeBaseBitmap::Assign(const tTVPNativeBaseBitmap &rhs)
{
	if(this == &rhs || Bitmap == rhs.Bitmap) return false;

	Bitmap->Release();
	Bitmap = rhs.Bitmap;
	Bitmap->AddRef();

	Font = rhs.Font;
	FontChanged = true; // informs internal font information is invalidated


	return true; // changed
}
//---------------------------------------------------------------------------
bool tTVPNativeBaseBitmap::AssignBitmap(const tTVPNativeBaseBitmap &rhs)
{
	// assign only bitmap
	if(this == &rhs || Bitmap == rhs.Bitmap) return false;

	Bitmap->Release();
	Bitmap = rhs.Bitmap;
	Bitmap->AddRef();

	// font information are not copyed
	FontChanged = true; // informs internal font information is invalidated

	return true;
}
bool tTVPNativeBaseBitmap::AssignTexture(iTVPTexture2D *tex)
{
	if (Bitmap == tex) return false;

	Bitmap->Release();
	Bitmap = tex;// CreateTexture2D(bmp);
	Bitmap->AddRef();

    // font information are not copyed
    FontChanged = true; // informs internal font information is invalidated

    return true;
}
//---------------------------------------------------------------------------
const void * tTVPNativeBaseBitmap::GetScanLine(tjs_uint l) const
{
	return Bitmap->GetScanLineForRead(l);
}
//---------------------------------------------------------------------------
void * tTVPNativeBaseBitmap::GetScanLineForWrite(tjs_uint l)
{
	Independ();
	return Bitmap->GetScanLineForWrite(l);
}
//---------------------------------------------------------------------------
tjs_int tTVPNativeBaseBitmap::GetPitchBytes() const
{
	if (Bitmap)
		return Bitmap->GetPitch();
	return 0;
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::Independ()
{
	// sever Bitmap's image sharing
	if (Bitmap->IsIndependent() && !Bitmap->IsStatic()) return;
	iTVPTexture2D *newb = GetRenderManager()->CreateTexture2D(Bitmap->GetWidth(), Bitmap->GetHeight(), Bitmap);
	Bitmap->Release();
	Bitmap = newb;
	FontChanged = true; // informs internal font information is invalidated
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::IndependNoCopy()
{
	// indepent the bitmap, but not to copy the original bitmap
	if (!Bitmap->IsStatic() && Bitmap->IsIndependent()) return;
	Recreate();
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::Recreate()
{
	Recreate(Bitmap->GetWidth(), Bitmap->GetHeight(), Bitmap->GetFormat() == TVPTextureFormat::Gray ? 8 : 32);
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::Recreate(tjs_uint w, tjs_uint h, tjs_uint bpp)
{
	Bitmap->Release();
	Bitmap = GetRenderManager()->CreateTexture2D(nullptr, 0, w, h, bpp == 8 ? TVPTextureFormat::Gray : TVPTextureFormat::RGBA);
	FontChanged = true; // informs internal font information is invalidated
}

bool tTVPNativeBaseBitmap::IsIndependent() const
{
	return Bitmap->IsIndependent() && !Bitmap->IsStatic();
}
#if 0
//---------------------------------------------------------------------------
tjs_uint tTVPNativeBaseBitmap::GetPalette( tjs_uint index ) const {
	if( !Is8BPP() ) TVPThrowExceptionMessage(TVPInvalidOperationFor32BPP);
	if( index >= Bitmap->GetPaletteCount() )
		TVPThrowExceptionMessage(TJSRangeError);

	return Bitmap->GetPalette()[index];
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::SetPalette( tjs_uint index, tjs_uint color ) {
	if( !Is8BPP() ) TVPThrowExceptionMessage(TVPInvalidOperationFor32BPP);
	if( index >= Bitmap->GetPaletteCount() ) {
		Bitmap->SetPaletteCount( index+1 );
	}
	Bitmap->GetPalette()[index] = color;
}
#endif
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::ApplyFont()
{
	// apply font
	if(FontChanged || GlobalFontState != TVPGlobalFontStateMagic)
	{
		Independ();

		FontChanged = false;
		GlobalFontState = TVPGlobalFontStateMagic;
		CachedText.Clear();
		TextWidth = TextHeight = 0;

		if(PrerenderedFont) PrerenderedFont->Release();
		PrerenderedFont = TVPGetPrerenderedMappedFont(Font);

		// compute ascent offset
		GetCurrentRasterizer()->ApplyFont( this, true );
		tjs_int ascent = GetCurrentRasterizer()->GetAscentHeight();
		RadianAngle = Font.Angle * (M_PI/1800);
		double angle90 = RadianAngle + M_PI_2;
		AscentOfsX = static_cast<tjs_int>(-cos(angle90) * ascent);
		AscentOfsY = static_cast<tjs_int>(sin(angle90) * ascent);

		// compute font hash
		FontHash = tTJSHashFunc<ttstr>::Make(Font.Face);
		FontHash ^= Font.Height ^ Font.Flags ^ Font.Angle;
	}
	else
	{
		GetCurrentRasterizer()->ApplyFont( this, false );
	}
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::SetFont(const tTVPFont &font)
{
	Font = font;
	FontChanged = true;
}
//---------------------------------------------------------------------------
extern void TVPGetAllFontList(std::vector<ttstr>& list);
void tTVPNativeBaseBitmap::GetFontList(tjs_uint32 flags, std::vector<ttstr> &list)
{
	ApplyFont();
	std::vector<ttstr> ansilist;
	TVPGetAllFontList(ansilist);
	for(std::vector<ttstr>::iterator i = ansilist.begin(); i != ansilist.end(); i++)
		list.push_back(*i);
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::MapPrerenderedFont(const ttstr & storage)
{
	ApplyFont();
	TVPMapPrerenderedFont(Font, storage);
	FontChanged = true;
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::UnmapPrerenderedFont()
{
	ApplyFont();
	TVPUnmapPrerenderedFont(Font);
	FontChanged = true;
}
//---------------------------------------------------------------------------
struct tTVPDrawTextData
{
	tTVPRect rect;
	tjs_int bmppitch;
	tjs_int opa;
	bool holdalpha;
	tTVPBBBltMethod bltmode;
};

static iTVPTexture2D *_CharacterTexture = nullptr, *_CharacterTextureRGBA = nullptr;

bool tTVPNativeBaseBitmap::InternalBlendText(
    tTVPCharacterData *data, tTVPDrawTextData *dtdata, tjs_uint32 color, const tTVPRect &srect, tTVPRect &drect)
{
    // blend to the bitmap
    tjs_int pitch = data->Pitch;
    //tjs_uint8 *sl = (tjs_uint8*)GetScanLineForWrite(drect.top);
    tjs_int h = drect.bottom - drect.top;
    tjs_int w = drect.right - drect.left;
	tjs_uint8 *bp = data->GetData() + pitch * srect.top;

	iTVPRenderMethod * method = nullptr;
	int opa_id, clr_id;
#define GEMTHOD_OPA_CLR(n) \
	static iTVPRenderMethod *_method = TVPGetRenderManager()->GetRenderMethod(#n); \
	static int _opa_id = _method->EnumParameterID("opacity"); \
	static int _clr_id = _method->EnumParameterID("color"); \
	method = _method; opa_id = _opa_id; clr_id = _clr_id;

	static bool fastGPURoute = !TVPIsSoftwareRenderManager()
		&& !IndividualConfigManager::GetInstance()->GetValueBool("ogl_accurate_render", false);

	iTVPTexture2D *pTexSrc;
	if (fastGPURoute && dtdata->bltmode == bmAlphaOnAlpha && dtdata->opa > 0) {
		// convert to addalpha bitmap
		tTVPBitmap* tmp = new tTVPBitmap(w, h, 32);
		tjs_int spitch = pitch;
		tjs_int dpitch = tmp->GetPitch();
		tjs_uint8 *src = bp;
		tjs_uint8* dst = (tjs_uint8*)tmp->GetBits();
		for (tjs_int y = 0; y < h; ++y) {
			for (tjs_int x = 0; x < w; ++x) {
				((tjs_uint32*)dst)[x] = (color & 0xFFFFFF) | (src[x] << 24);
			}
			TVPConvertAlphaToAdditiveAlpha((tjs_uint32*)dst, w);
			dst += dpitch;
			src += spitch;
		}
		if (_CharacterTextureRGBA) {
			if (_CharacterTextureRGBA->GetFormat() != TVPTextureFormat::RGBA) {
				_CharacterTextureRGBA->Release();
				_CharacterTextureRGBA = nullptr;
			}
		}
		if (!_CharacterTextureRGBA) {
			_CharacterTextureRGBA = GetRenderManager()->CreateTexture2D(tmp->GetBits(), dpitch, w, h, TVPTextureFormat::RGBA);
		} else if (_CharacterTextureRGBA->GetInternalWidth() < w || _CharacterTextureRGBA->GetInternalHeight() < h) {
			_CharacterTextureRGBA->Release();
			_CharacterTextureRGBA = GetRenderManager()->CreateTexture2D(tmp->GetBits(), dpitch, w, h, TVPTextureFormat::RGBA);
		} else {
			_CharacterTextureRGBA->Update(tmp->GetBits(), TVPTextureFormat::RGBA, dpitch, tTVPRect(0, 0, w, h));
		}

		tmp->Release();

		GEMTHOD_OPA_CLR(AlphaBlend_a);
		method->SetParameterOpa(opa_id, dtdata->opa);
		pTexSrc = _CharacterTextureRGBA;
	} else {
		if (dtdata->bltmode == bmAlphaOnAlpha) {
			if (dtdata->opa > 0) {
				GEMTHOD_OPA_CLR(ApplyColorMap_d);
			} else {
				// opacity removal
				GEMTHOD_OPA_CLR(RemoveOpacity);
			}
		} else if (dtdata->bltmode == bmAlphaOnAddAlpha) {
			GEMTHOD_OPA_CLR(ApplyColorMap_a);
		} else {
			GEMTHOD_OPA_CLR(ApplyColorMap);
		}

		// blend to the texture
		if (!_CharacterTexture) {
			_CharacterTexture = GetRenderManager()->CreateTexture2D(bp, pitch, w, h, TVPTextureFormat::Gray);
		} else if (_CharacterTexture->GetInternalWidth() < w || _CharacterTexture->GetInternalHeight() < h) {
			_CharacterTexture->Release();
			_CharacterTexture = GetRenderManager()->CreateTexture2D(bp, pitch, w, h, TVPTextureFormat::Gray);
		} else {
			_CharacterTexture->Update(bp, TVPTextureFormat::Gray, pitch, tTVPRect(0, 0, w, h));
		}

		method->SetParameterOpa(opa_id, dtdata->opa);
		method->SetParameterColor4B(clr_id, color);

		pTexSrc = _CharacterTexture;
	}
#if 0
    if (pShader->isBlendEnabled() || !IsIndependent()) {
		iTVPTexture2D *origTex = GetTexture();
		iTVPTexture2D *tex = GetTextureForRender();
        TVPRenderTexture2(pShader,
            tex, drect,
            origTex, drect,
            _CharacterTexture, texRect(0, 0, w, h));
    } else { // optimize for independent texture
		iTVPTexture2D *tmptex = TVPCreateTextureForRender(drect.get_width(), drect.get_height());
		iTVPTexture2D *tex = GetTexture();
        texRect rc(drect);
        rc.x = 0; rc.y = 0;
        TVPCopyTexture(
            tmptex, rc,
            tex, drect);
        TVPRenderTexture2(pShader,
            tex, drect,
            tmptex, rc,
            _CharacterTexture, texRect(0, 0, w, h));
        tmptex->Release();
    }
#endif
	tRenderTexRectArray::Element src_tex[] = {
		tRenderTexRectArray::Element(pTexSrc, tTVPRect(0, 0, w, h))
	};
	TVPGetRenderManager()->OperateRect(method,
		GetTextureForRender(method->IsBlendTarget(), &drect), nullptr, drect,
		tRenderTexRectArray(src_tex));
    return true;
}

bool tTVPNativeBaseBitmap::InternalDrawText(tTVPCharacterData *data, tjs_int x,
	tjs_int y, tjs_uint32 color, tTVPDrawTextData *dtdata, tTVPRect &drect)
{
	// setup destination and source rectangle
	drect.left = x + data->OriginX;
	drect.top = y + data->OriginY;
	drect.right = drect.left + data->BlackBoxX;
	drect.bottom = drect.top + data->BlackBoxY;

	tTVPRect srect;
	srect.left = srect.top = 0;
	srect.right = data->BlackBoxX;
	srect.bottom = data->BlackBoxY;

	// check boundary
	if(drect.left < dtdata->rect.left)
	{
		srect.left += (dtdata->rect.left - drect.left);
		drect.left = dtdata->rect.left;
	}

	if(drect.right > dtdata->rect.right)
	{
		srect.right -= (drect.right - dtdata->rect.right);
		drect.right = dtdata->rect.right;
	}

	if(srect.left >= srect.right) return false; // not drawable

	if(drect.top < dtdata->rect.top)
	{
		srect.top += (dtdata->rect.top - drect.top);
		drect.top = dtdata->rect.top;
	}

	if(drect.bottom > dtdata->rect.bottom)
	{
		srect.bottom -= (drect.bottom - dtdata->rect.bottom);
		drect.bottom = dtdata->rect.bottom;
	}

	if(srect.top >= srect.bottom) return false; // not drawable

    return InternalBlendText(data, dtdata, color, srect, drect);
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::DrawGlyph(iTJSDispatch2* glyph, const tTVPRect &destrect, tjs_int x, tjs_int y,
			tjs_uint32 color, tTVPBBBltMethod bltmode, tjs_int opa,
			bool holdalpha, bool aa, tjs_int shlevel,
			tjs_uint32 shadowcolor,
			tjs_int shwidth, tjs_int shofsx, tjs_int shofsy,
			tTVPComplexRect *updaterects )
{
	if(!Is32BPP()) TVPThrowExceptionMessage(TVPInvalidOperationFor8BPP);

	if(bltmode == bmAlphaOnAlpha)
	{
		if(opa < -255) opa = -255;
		if(opa > 255) opa = 255;
	}
	else
	{
		if(opa < 0) opa = 0;
		if(opa > 255 ) opa = 255;
	}

	if(opa == 0) return; // nothing to do


	tjs_int itemcount;
	tTJSVariant tmp;
	if(TJS_SUCCEEDED(glyph->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("count"), 0, &tmp, glyph)))
		itemcount = tmp;
	else
		itemcount = 0;

	if( itemcount < 8 ) TVPThrowExceptionMessage( TVPFaildGlyphForDrawGlyph );

	enum {
		GLYPH_WIDTH,
		GLYPH_HEIGHT,
		GLYPH_ORIGINX,
		GLYPH_ORIGINY,
		GLYPH_INCX,
		GLYPH_INCY,
		GLYPH_INC,
		GLYPH_BITMAP,
		GLYPH_COLORS,
		GLYPH_EOT
	};
	tjs_int glyphitem[7];
	for( tjs_int i = 0; i < 7; i++ ) {
		if(TJS_FAILED(glyph->PropGetByNum(TJS_MEMBERMUSTEXIST, i, &tmp, glyph)))
			TVPThrowExceptionMessage( TVPFaildGlyphForDrawGlyph );
		glyphitem[i] = tmp;
	}

	if(TJS_FAILED(glyph->PropGetByNum(TJS_MEMBERMUSTEXIST, GLYPH_BITMAP, &tmp, glyph)))
		TVPThrowExceptionMessage( TVPFaildGlyphForDrawGlyph );

	tjs_int numcolor = 256;
	if( itemcount >= 9 ) {
		if(TJS_FAILED(glyph->PropGetByNum(TJS_MEMBERMUSTEXIST, GLYPH_COLORS, &tmp, glyph)))
			TVPThrowExceptionMessage( TVPFaildGlyphForDrawGlyph );
		numcolor = tmp;
	}
	tTJSVariantOctet *o = tmp.AsOctetNoAddRef();

	Independ();
	ApplyFont();

	tTVPDrawTextData dtdata;
	dtdata.rect = destrect;
	dtdata.bmppitch = GetPitchBytes();
	dtdata.bltmode = bltmode;
	dtdata.opa = opa;
	dtdata.holdalpha = holdalpha;

	tTVPCharacterData* data = NULL;
	tTVPCharacterData* shadow = NULL;
	try {
		tGlyphMetrics metrics;
		metrics.CellIncX = glyphitem[GLYPH_INCX];
		metrics.CellIncY = glyphitem[GLYPH_INCY];
		data = new tTVPCharacterData( o->GetData(), glyphitem[GLYPH_WIDTH],
			glyphitem[GLYPH_ORIGINX] + AscentOfsX, -glyphitem[GLYPH_ORIGINY] + AscentOfsY,
			glyphitem[GLYPH_WIDTH], glyphitem[GLYPH_HEIGHT],
			metrics, numcolor > 256 );

		data->Antialiased = aa;
		data->Blured = false;
		data->BlurWidth = shwidth;
		data->BlurLevel = shlevel;
		data->Gray = numcolor;
		if( shlevel != 0 ) {
			if( shlevel == 255 && shwidth == 0 ) {
				// normal shadow
				shadow = data;
				shadow->AddRef();
			} else {
				// blured shadow
				shadow = new tTVPCharacterData( o->GetData(), glyphitem[GLYPH_WIDTH],
					glyphitem[GLYPH_ORIGINX] + AscentOfsX, -glyphitem[GLYPH_ORIGINY] + AscentOfsY,
					glyphitem[GLYPH_WIDTH], glyphitem[GLYPH_HEIGHT],
					metrics, numcolor > 256 );
				shadow->Antialiased = aa;
				shadow->Blured = true;
				shadow->BlurWidth = shwidth;
				shadow->BlurLevel = shlevel;
				shadow->Gray = numcolor;
				if( !shadow->FullColored ) shadow->Blur();
			}
		}

		if(data)
		{

			if(data->BlackBoxX != 0 && data->BlackBoxY != 0)
			{
				tTVPRect drect;
				tTVPRect shadowdrect;

				bool shadowdrawn = false;

				if(shadow)
				{
					shadowdrawn = InternalDrawText(shadow, x + shofsx, y + shofsy,
						shadowcolor, &dtdata, shadowdrect);
				}

				bool drawn = InternalDrawText(data, x, y, color, &dtdata, drect);
				if(updaterects)
				{
					if(!shadowdrawn)
					{
						if(drawn) updaterects->Or(drect);
					}
					else
					{
						if(drawn)
						{
							tTVPRect d;
							TVPUnionRect(&d, drect, shadowdrect);
							updaterects->Or(d);
						}
						else
						{
							updaterects->Or(shadowdrect);
						}
					}
				}
			}
		}
	}
	catch(...)
	{
		if(data) data->Release();
		if(shadow) shadow->Release();
		throw;
	}

	if(data) data->Release();
	if(shadow) shadow->Release();
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::DrawTextSingle(const tTVPRect &destrect,
	tjs_int x, tjs_int y, const ttstr &text,
		tjs_uint32 color, tTVPBBBltMethod bltmode, tjs_int opa,
			bool holdalpha, bool aa, tjs_int shlevel,
			tjs_uint32 shadowcolor,
			tjs_int shwidth, tjs_int shofsx, tjs_int shofsy,
			tTVPComplexRect *updaterects)
{
	// text drawing function for single character

	if(!Is32BPP()) TVPThrowExceptionMessage(TVPInvalidOperationFor8BPP);

	if(bltmode == bmAlphaOnAlpha)
	{
		if(opa < -255) opa = -255;
		if(opa > 255) opa = 255;
	}
	else
	{
		if(opa < 0) opa = 0;
		if(opa > 255 ) opa = 255;
	}

	if(opa == 0) return; // nothing to do

	Independ();

	ApplyFont();

	const tjs_char *p = text.c_str();
	tTVPDrawTextData dtdata;
	dtdata.rect = destrect;
	dtdata.bmppitch = GetPitchBytes();
	dtdata.bltmode = bltmode;
	dtdata.opa = opa;
	dtdata.holdalpha = holdalpha;

	tTVPFontAndCharacterData font;
	font.Font = Font;
	font.Antialiased = aa;
	font.Hinting = true;
	font.BlurLevel = shlevel;
	font.BlurWidth = shwidth;
	font.FontHash = FontHash;

	font.Character = *p;

	font.Blured = false;
	tTVPCharacterData * shadow = NULL;
	tTVPCharacterData * data = NULL;

	try
	{
		data = TVPGetCharacter(font, this, PrerenderedFont, AscentOfsX, AscentOfsY);

		if(shlevel != 0)
		{
			if(shlevel == 255 && shwidth == 0)
			{
				// normal shadow
				shadow = data;
				shadow->AddRef();
			}
			else
			{
				// blured shadow
				font.Blured = true;
				shadow =
					TVPGetCharacter(font, this, PrerenderedFont, AscentOfsX, AscentOfsY);
			}
		}


		if(data)
		{

			if(data->BlackBoxX != 0 && data->BlackBoxY != 0)
			{
				tTVPRect drect;
				tTVPRect shadowdrect;

				bool shadowdrawn = false;

				if(shadow)
				{
					shadowdrawn = InternalDrawText(shadow, x + shofsx, y + shofsy,
						shadowcolor, &dtdata, shadowdrect);
				}

				bool drawn = InternalDrawText(data, x, y, color, &dtdata, drect);
				if(updaterects)
				{
					if(!shadowdrawn)
					{
						if(drawn) updaterects->Or(drect);
					}
					else
					{
						if(drawn)
						{
							tTVPRect d;
							TVPUnionRect(&d, drect, shadowdrect);
							updaterects->Or(d);
						}
						else
						{
							updaterects->Or(shadowdrect);
						}
					}
				}
			}
		}
	}
	catch(...)
	{
		if(data) data->Release();
		if(shadow) shadow->Release();
		throw;
	}

	if(data) data->Release();
	if(shadow) shadow->Release();
}
//---------------------------------------------------------------------------
// structure for holding data for a character
struct tTVPCharacterDrawData
{
	tTVPCharacterData * Data; // main character data
	tTVPCharacterData * Shadow; // shadow character data
	tjs_int X, Y;
	tTVPRect ShadowRect;
	bool ShadowDrawn;

	tTVPCharacterDrawData(
		tTVPCharacterData * data,
		tTVPCharacterData * shadow,
		tjs_int x, tjs_int y)
	{
		Data = data;
		Shadow = shadow;
		X = x;
		Y = y;
		ShadowDrawn = false;

		if(Data) Data->AddRef();
		if(Shadow) Shadow->AddRef();
	}

	~tTVPCharacterDrawData()
	{
		if(Data) Data->Release();
		if(Shadow) Shadow->Release();
	}

	tTVPCharacterDrawData(const tTVPCharacterDrawData & rhs)
	{
		Data = Shadow = NULL;
		*this = rhs;
	}

	void operator = (const tTVPCharacterDrawData & rhs)
	{
		X = rhs.X;
		Y = rhs.Y;
		ShadowRect = rhs.ShadowRect;
		ShadowDrawn = rhs.ShadowDrawn;

		if(Data != rhs.Data)
		{
			if(Data) Data->Release();
			Data = rhs.Data;
			if(Data) Data->AddRef();
		}
		if(Shadow != rhs.Shadow)
		{
			if(Shadow) Shadow->Release();
			Shadow = rhs.Shadow;
			if(Shadow) Shadow->AddRef();
		}
	}
};
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::DrawTextMultiple(const tTVPRect &destrect,
	tjs_int x, tjs_int y, const ttstr &text,
		tjs_uint32 color, tTVPBBBltMethod bltmode, tjs_int opa,
			bool holdalpha, bool aa, tjs_int shlevel,
			tjs_uint32 shadowcolor,
			tjs_int shwidth, tjs_int shofsx, tjs_int shofsy,
			tTVPComplexRect *updaterects)
{
	// text drawing function for multiple characters

	if(!Is32BPP()) TVPThrowExceptionMessage(TVPInvalidOperationFor8BPP);

	if(bltmode == bmAlphaOnAlpha)
	{
		if(opa < -255) opa = -255;
		if(opa > 255) opa = 255;
	}
	else
	{
		if(opa < 0) opa = 0;
		if(opa > 255 ) opa = 255;
	}

	if(opa == 0) return; // nothing to do

	Independ();

	ApplyFont();

	const tjs_char *p = text.c_str();
	tTVPDrawTextData dtdata;
	dtdata.rect = destrect;
	dtdata.bmppitch = GetPitchBytes();
	dtdata.bltmode = bltmode;
	dtdata.opa = opa;
	dtdata.holdalpha = holdalpha;

	tTVPFontAndCharacterData font;
	font.Font = Font;
	font.Antialiased = aa;
	font.Hinting = true;
	font.BlurLevel = shlevel;
	font.BlurWidth = shwidth;
	font.FontHash = FontHash;


	std::vector<tTVPCharacterDrawData> drawdata;
	drawdata.reserve(text.GetLen());

	// prepare all drawn characters
	while(*p) // while input string is remaining
	{
		font.Character = *p;

		font.Blured = false;
		tTVPCharacterData * data = NULL;
		tTVPCharacterData * shadow = NULL;
		try
		{
			data =
				TVPGetCharacter(font, this, PrerenderedFont, AscentOfsX, AscentOfsY);

			if(data)
			{
				if(shlevel != 0)
				{
					if(shlevel == 255 && shwidth == 0)
					{
						// normal shadow
						// shadow is the same as main character data
						shadow = data;
						shadow->AddRef();
					}
					else
					{
						// blured shadow
						font.Blured = true;
						shadow =
							TVPGetCharacter(font, this, PrerenderedFont, AscentOfsX, AscentOfsY);
					}
				}


				if(data->BlackBoxX != 0 && data->BlackBoxY != 0)
				{
					// append to array
					drawdata.push_back(tTVPCharacterDrawData(data, shadow, x, y));
				}

				// step to the next character position
				x += data->Metrics.CellIncX;
				if(data->Metrics.CellIncY != 0)
				{
					// Windows 9x returns negative CellIncY.
					// so we must verify whether CellIncY is proper.
					if(Font.Angle < 1800)
					{
						if(data->Metrics.CellIncY > 0) data->Metrics.CellIncY = - data->Metrics.CellIncY;
					}
					else
					{
						if(data->Metrics.CellIncY < 0) data->Metrics.CellIncY = - data->Metrics.CellIncY;
					}
					y += data->Metrics.CellIncY;
				}
			}
		}
		catch(...)
		{
			 if(data) data->Release();
			 if(shadow) shadow->Release();
			 throw;
		}
		if(data) data->Release();
		if(shadow) shadow->Release();

		p++;
	}

	// draw shadows first
	if(shlevel != 0)
	{
		for(std::vector<tTVPCharacterDrawData>::iterator i = drawdata.begin();
			i != drawdata.end(); i++)
		{
			tTVPCharacterData * shadow = i->Shadow;

			if(shadow)
			{
				i->ShadowDrawn = InternalDrawText(shadow, i->X + shofsx, i->Y + shofsy,
					shadowcolor, &dtdata, i->ShadowRect);
			}
		}
	}

	// then draw main characters
	// and compute returning update rectangle
	for(std::vector<tTVPCharacterDrawData>::iterator i = drawdata.begin();
		i != drawdata.end(); i++)
	{
		tTVPCharacterData * data = i->Data;
		tTVPRect drect;

		bool drawn = InternalDrawText(data, i->X, i->Y, color, &dtdata, drect);
		if(updaterects)
		{
			if(!i->ShadowDrawn)
			{
				if(drawn) updaterects->Or(drect);
			}
			else
			{
				if(drawn)
				{
					tTVPRect d;
					TVPUnionRect(&d, drect, i->ShadowRect);
					updaterects->Or(d);
				}
				else
				{
					updaterects->Or(i->ShadowRect);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::GetTextSize(const ttstr & text)
{
	ApplyFont();

	if(text != CachedText)
	{
		CachedText = text;

		if(PrerenderedFont)
		{
			tjs_uint width = 0;
			const tjs_char *buf = text.c_str();
			while(*buf)
			{
				const tTVPPrerenderedCharacterItem * item =
					PrerenderedFont->Find(*buf);
				if(item != NULL)
				{
					width += item->Inc;
				}
				else
				{
					tjs_int w, h;
					GetCurrentRasterizer()->GetTextExtent( *buf, w, h );
					width += w;
				}
				buf++;
			}
			TextWidth = width;
			TextHeight = std::abs(Font.Height);
		}
		else
		{
			tjs_uint width = 0;
			const tjs_char *buf = text.c_str();

			while(*buf)
			{
				tjs_int w, h;
				GetCurrentRasterizer()->GetTextExtent( *buf, w, h );
				width += w;
				buf++;
			}
			TextWidth = width;
			TextHeight = std::abs(Font.Height);
		}
	}
}
//---------------------------------------------------------------------------
tjs_int tTVPNativeBaseBitmap::GetTextWidth(const ttstr & text)
{
	GetTextSize(text);
	return TextWidth;
}
//---------------------------------------------------------------------------
tjs_int tTVPNativeBaseBitmap::GetTextHeight(const ttstr & text)
{
	GetTextSize(text);
	return TextHeight;
}
//---------------------------------------------------------------------------
double tTVPNativeBaseBitmap::GetEscWidthX(const ttstr & text)
{
	GetTextSize(text);
	return cos(RadianAngle) * TextWidth;
}
//---------------------------------------------------------------------------
double tTVPNativeBaseBitmap::GetEscWidthY(const ttstr & text)
{
	GetTextSize(text);
	return sin(RadianAngle) * (-TextWidth);
}
//---------------------------------------------------------------------------
double tTVPNativeBaseBitmap::GetEscHeightX(const ttstr & text)
{
	GetTextSize(text);
	return sin(RadianAngle) * TextHeight;
}
//---------------------------------------------------------------------------
double tTVPNativeBaseBitmap::GetEscHeightY(const ttstr & text)
{
	GetTextSize(text);
	return cos(RadianAngle) * TextHeight;
}
//---------------------------------------------------------------------------
void tTVPNativeBaseBitmap::GetFontGlyphDrawRect( const ttstr & text, struct tTVPRect& area )
{
	ApplyFont();
	GetCurrentRasterizer()->GetGlyphDrawRect( text, area );
}
iTVPTexture2D * tTVPNativeBaseBitmap::GetTextureForRender(bool isBlendTarget, const tTVPRect *rc) {
	if (isBlendTarget || !rc) Independ();
	else {
		int w = Bitmap->GetWidth(), h = Bitmap->GetHeight();
		if (rc->left == 0 && rc->top == 0 && rc->right >= w && rc->bottom >= h) {
			IndependNoCopy();
		} else {
			Independ();
		}
	}
	return GetTexture();
}
//---------------------------------------------------------------------------
