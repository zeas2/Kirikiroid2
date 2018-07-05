//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Graphics Loader ( loads graphic format from storage )
//---------------------------------------------------------------------------

#ifndef GraphicsLoaderIntfH
#define GraphicsLoaderIntfH


#include "drawable.h"

class tTVPBaseBitmap;
namespace TJS {
    class tTJSBinaryStream;
}

enum tTVPGraphicPixelFormat
{
	gpfLuminance,
	gpfPalette,
	gpfRGB,
	gpfRGBA
};

/*[*/
//---------------------------------------------------------------------------
// Graphic Loading Handler Type
//---------------------------------------------------------------------------
typedef int (*tTVPGraphicSizeCallback) // return line pitch
	(void *callbackdata, tjs_uint w, tjs_uint h, tTVPGraphicPixelFormat fmt);
/*
	callback type to inform the image's size.
	call this once before TVPGraphicScanLineCallback.
*/

typedef void * (*tTVPGraphicScanLineCallback)
	(void *callbackdata, tjs_int y);
/*
	callback type to ask the scanline buffer for the decoded image, per a line.
	returning null can stop the processing.

	passing of y=-1 notifies the scan line image had been written to the buffer that
	was given by previous calling of TVPGraphicScanLineCallback. in this time,
	this callback function must return NULL.
*/

typedef const void * (*tTVPGraphicSaveScanLineCallback)
	(void *callbackdata, tjs_int y);

typedef void (*tTVPMetaInfoPushCallback)
	(void *callbackdata, const ttstr & name, const ttstr & value);
/*
	callback type to push meta-information of the image.
	this can be null.
*/

enum tTVPGraphicLoadMode
{
	glmNormal, // normal, ie. 32bit ARGB graphic
	glmPalettized, // palettized 8bit mode
	glmGrayscale // grayscale 8bit mode
};
/*]*/

typedef void (*tTVPGraphicLoadingHandler)(void* formatdata,
	void *callbackdata,
	tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback,
	tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src,
	tjs_int32 keyidx,
	tTVPGraphicLoadMode mode);
/*
	format = format specific data given at TVPRegisterGraphicLoadingHandler
	dest = destination callback function
	src = source stream
	keyidx = color key for less than or equal to 8 bit image
	mode = if glmPalettized, the output image must be an 8bit color (for province
	   image. so the color is not important. color index must be preserved).
	   if glmGrayscale, the output image must be an 8bit grayscale image.
	   otherwise the output image must be a 32bit full-color with opacity.

	color key does not overrides image's alpha channel ( if the image has )

	the function may throw an exception if error.
*/

typedef void (*tTVPGraphicHeaderLoadingHandler)(void* formatdata,  tTJSBinaryStream *src, class iTJSDispatch2** dic );
typedef void (*tTVPGraphicSaveHandler)(void* formatdata,  tTJSBinaryStream* dst, const class iTVPBaseBitmap* image,
	const ttstr & mode, class iTJSDispatch2* meta );

/*[*/
typedef bool (*tTVPGraphicAcceptSaveHandler)(void* formatdata, const ttstr & type, class iTJSDispatch2** dic );
/*]*/

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Graphics Format Management
//---------------------------------------------------------------------------
void TVPRegisterGraphicLoadingHandler(const ttstr & name,
	tTVPGraphicLoadingHandler loading,
	tTVPGraphicHeaderLoadingHandler header,
	tTVPGraphicSaveHandler save,
	tTVPGraphicAcceptSaveHandler accept,
	void* formatdata);
void TVPUnregisterGraphicLoadingHandler(const ttstr & name,
	tTVPGraphicLoadingHandler loading,
	tTVPGraphicHeaderLoadingHandler header,
	tTVPGraphicSaveHandler save,
	tTVPGraphicAcceptSaveHandler accept,
	void* formatdata);


/*[*/
/* For grahpic load and save */
typedef void (*tTVPGraphicLoadingHandlerForPlugin)(void* formatdata,
	void *callbackdata,
	tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback,
	tTVPMetaInfoPushCallback metainfopushcallback,
	struct IStream *src,
	tjs_int32 keyidx,
	tTVPGraphicLoadMode mode);
typedef void (*tTVPGraphicHeaderLoadingHandlerForPlugin)(void* formatdata, struct IStream* src, class iTJSDispatch2** dic );
typedef void (*tTVPGraphicSaveHandlerForPlugin)(void* formatdata, void* callbackdata, struct IStream* dst, const ttstr & mode,
	tjs_uint width, tjs_uint height,
	tTVPGraphicSaveScanLineCallback scanlinecallback,
	class iTJSDispatch2* meta );
/*]*/

TJS_EXP_FUNC_DEF( void, TVPRegisterGraphicLoadingHandler, (const ttstr & name,
	tTVPGraphicLoadingHandlerForPlugin loading,
	tTVPGraphicHeaderLoadingHandlerForPlugin header,
	tTVPGraphicSaveHandlerForPlugin save,
	tTVPGraphicAcceptSaveHandler accept,
	void* formatdata));

TJS_EXP_FUNC_DEF( void, TVPUnregisterGraphicLoadingHandler, (const ttstr & name,
	tTVPGraphicLoadingHandlerForPlugin loading,
	tTVPGraphicHeaderLoadingHandlerForPlugin header,
	tTVPGraphicSaveHandlerForPlugin save,
	tTVPGraphicAcceptSaveHandler accept,
	void* formatdata));
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// default handlers
//---------------------------------------------------------------------------
extern void TVPLoadBMP(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode);

extern void TVPLoadJPEG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode);

extern void TVPLoadPNG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode);

extern void TVPLoadJXR(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode);

extern void TVPLoadTLG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx, tTVPGraphicLoadMode mode);
//---------------------------------------------------------------------------
extern void TVPLoadWEBP(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode);

extern void TVPLoadBPG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx, tTVPGraphicLoadMode mode);

//---------------------------------------------------------------------------
// Image header handler
// dic = %[
//    "width" => (int)width,
//    "height" => (int)height,
//    "bpp" => (int)bpp, // (option)
//    "palette" => (bool)palette, // (option)
//    "grayscale" => (bool)grayscale, // (option)
//    "comment" => (string)comment, // (option)
//    etc...
// ]
//---------------------------------------------------------------------------
extern void TVPLoadHeaderBMP(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic );
extern void TVPLoadHeaderJPG(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic );
extern void TVPLoadHeaderPNG(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic );
extern void TVPLoadHeaderJXR(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic );
extern void TVPLoadHeaderTLG(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic);
extern void TVPLoadHeaderWEBP(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic);
extern void TVPLoadHeaderBPG(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Image saving handler
//---------------------------------------------------------------------------
extern void TVPSaveAsBMP(void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* image, const ttstr & mode, iTJSDispatch2* meta);
extern void TVPSaveAsPNG(void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* image, const ttstr & mode, iTJSDispatch2* meta);
extern void TVPSaveAsJPG(void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* image, const ttstr & mode, iTJSDispatch2* meta);
extern void TVPSaveAsJXR(void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* image, const ttstr & mode, iTJSDispatch2* meta);
extern void TVPSaveAsTLG(void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* image, const ttstr & mode, iTJSDispatch2* meta);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// is accept
//---------------------------------------------------------------------------
extern bool TVPAcceptSaveAsBMP(void* formatdata, const ttstr & type, iTJSDispatch2** dic );
extern bool TVPAcceptSaveAsPNG(void* formatdata, const ttstr & type, iTJSDispatch2** dic );
extern bool TVPAcceptSaveAsJPG(void* formatdata, const ttstr & type, iTJSDispatch2** dic );
extern bool TVPAcceptSaveAsJXR(void* formatdata, const ttstr & type, iTJSDispatch2** dic );
extern bool TVPAcceptSaveAsTLG(void* formatdata, const ttstr & type, iTJSDispatch2** dic );
//---------------------------------------------------------------------------

void TVPSaveTextureAsBMP(tTJSBinaryStream* dst, class iTVPTexture2D* bmp, const ttstr & mode = TJS_W(""), iTJSDispatch2* meta = nullptr);
void TVPSaveTextureAsBMP(const ttstr &path, class iTVPTexture2D* tex, const ttstr &mode = TJS_W(""), iTJSDispatch2* meta = nullptr);

//---------------------------------------------------------------------------
// JPEG loading handler
//---------------------------------------------------------------------------
enum tTVPJPEGLoadPrecision
{
	jlpLow,
	jlpMedium,
	jlpHigh
};

extern tTVPJPEGLoadPrecision TVPJPEGLoadPrecision;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Graphics cache management
//---------------------------------------------------------------------------
extern bool TVPAllocGraphicCacheOnHeap;
	// this allocates graphic cache's store memory on heap, rather than
	// shareing bitmap object. ( since sucking win9x cannot have so many bitmap
	// object at once, WinNT/2000 is ok. )
	// this will take more time for memory copying.
extern void TVPSetGraphicCacheLimit(tjs_uint64 limit);
	// set graphic cache size limit by bytes.
	// limit == 0 disables the cache system.
	// limit == -1 sets the limit to TVPGraphicCacheSystemLimit
extern tjs_uint64 TVPGetGraphicCacheLimit();

extern tjs_uint64 TVPGraphicCacheSystemLimit;
	// maximum possible value of Graphic Cache Limit

TJS_EXP_FUNC_DEF(void, TVPClearGraphicCache, ());
	// clear graphic cache


extern void TVPTouchImages(const std::vector<ttstr> & storages, tjs_int64 limit,
	tjs_uint64 timeout);

//---------------------------------------------------------------------------




class iTVPBaseBitmap;
//---------------------------------------------------------------------------
// TVPLoadGraphic
//---------------------------------------------------------------------------
extern int TVPLoadGraphic(iTVPBaseBitmap *dest, const ttstr &name, tjs_int keyidx,
	tjs_uint desw, tjs_uint desh,
	tTVPGraphicLoadMode mode, ttstr *provincename = NULL, iTJSDispatch2 ** metainfo = NULL);
	// throws exception when this function can not handle the file
//---------------------------------------------------------------------------

extern void TVPLoadGraphicProvince(tTVPBaseBitmap *dest, const ttstr &name, tjs_int keyidx,
	tjs_uint desw, tjs_uint desh);


//---------------------------------------------------------------------------
// BMP loading interface
//---------------------------------------------------------------------------

#ifndef BI_RGB // avoid re-define error on Win32
	#define BI_RGB			0
	#define BI_RLE8			1
	#define BI_RLE4			2
	#define BI_BITFIELDS	3
#endif

#pragma pack(push, 1)
struct TVP_WIN_BITMAPFILEHEADER
{
	tjs_uint16	bfType;
	tjs_uint32	bfSize;
	tjs_uint16	bfReserved1;
	tjs_uint16	bfReserved2;
	tjs_uint32	bfOffBits;
};
struct TVP_WIN_BITMAPINFOHEADER
{
	tjs_uint32	biSize;
	tjs_int		biWidth;
	tjs_int		biHeight;
	tjs_uint16	biPlanes;
	tjs_uint16	biBitCount;
	tjs_uint32	biCompression;
	tjs_uint32	biSizeImage;
	tjs_int		biXPelsPerMeter;
	tjs_int		biYPelsPerMeter;
	tjs_uint32	biClrUsed;
	tjs_uint32	biClrImportant;
};
#pragma pack(pop)

enum tTVPBMPAlphaType
{
	// this specifies alpha channel treatment if the bitmap is 32bpp.
	// note that TVP currently does not support new (V4 or V5) bitmap header
	batNone, // plugin does not return alpha channel.
	batMulAlpha, // returns alpha channel, d = d * alpha + s * (1-alpha)
	batAddAlpha // returns alpha channel, d = d * alpha + s
};



extern void TVPInternalLoadBMP(void *callbackdata,
	tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback,
	TVP_WIN_BITMAPINFOHEADER &bi,
	const tjs_uint8 *palsrc,
	tTJSBinaryStream * src,
	tjs_int keyidx,
	tTVPBMPAlphaType alphatype,
	tTVPGraphicLoadMode mode);

extern const void* tTVPBitmapScanLineCallbackForSave(void *callbackdata, tjs_int y);

struct tTVPGraphicHandlerType
{
	bool IsPlugin;
	ttstr Extension;
	union {
		tTVPGraphicLoadingHandler LoadHandler;
		tTVPGraphicLoadingHandlerForPlugin LoadHandlerPlugin;
	};
	union {
		tTVPGraphicHeaderLoadingHandler HeaderHandler;
		tTVPGraphicHeaderLoadingHandlerForPlugin HeaderHandlerPlugin;
	};
	union {
		tTVPGraphicSaveHandler SaveHandler;
		tTVPGraphicSaveHandlerForPlugin SaveHandlerPlugin;
	};
	tTVPGraphicAcceptSaveHandler AcceptHandler;
	void * FormatData;

	tTVPGraphicHandlerType(const ttstr &ext,
		tTVPGraphicLoadingHandler loading,
		tTVPGraphicHeaderLoadingHandler header,
		tTVPGraphicSaveHandler save,
		tTVPGraphicAcceptSaveHandler accept,
		void * data) : IsPlugin(false)
	{
		Extension = ext;
		LoadHandler = loading;
		HeaderHandler = header;
		SaveHandler = save;
		AcceptHandler = accept;
		FormatData = data;
	}

	tTVPGraphicHandlerType(const ttstr &ext,
		tTVPGraphicLoadingHandlerForPlugin loading,
		tTVPGraphicHeaderLoadingHandlerForPlugin header,
		tTVPGraphicSaveHandlerForPlugin save,
		tTVPGraphicAcceptSaveHandler accept,
		void * data) : IsPlugin(true)
	{
		Extension = ext;
		LoadHandlerPlugin = loading;
		HeaderHandlerPlugin = header;
		SaveHandlerPlugin = save;
		AcceptHandler = accept;
		FormatData = data;
	}

	tTVPGraphicHandlerType(const tTVPGraphicHandlerType & ref)
	{
		IsPlugin = ref.IsPlugin;
		if( IsPlugin )
		{
			LoadHandlerPlugin = ref.LoadHandlerPlugin;
			HeaderHandlerPlugin = ref.HeaderHandlerPlugin;
			SaveHandlerPlugin = ref.SaveHandlerPlugin;
		}
		else
		{
			LoadHandler = ref.LoadHandler;
			HeaderHandler = ref.HeaderHandler;
			SaveHandler = ref.SaveHandler;
		}
		AcceptHandler = ref.AcceptHandler;
		Extension = ref.Extension;
		FormatData = ref.FormatData;
	}

	bool operator == (const tTVPGraphicHandlerType & ref) const
	{
		return FormatData == ref.FormatData &&
			IsPlugin == ref.IsPlugin &&
			LoadHandler == ref.LoadHandler &&
			HeaderHandler == ref.HeaderHandler &&
			SaveHandler == ref.SaveHandler &&
			AcceptHandler == ref.AcceptHandler &&
			Extension == ref.Extension;
	}
	void Load( void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback, tTVPGraphicScanLineCallback scanlinecallback,
		tTVPMetaInfoPushCallback metainfopushcallback, tTJSBinaryStream *src, tjs_int32 keyidx, tTVPGraphicLoadMode mode);
	void Save( const ttstr & storagename, const ttstr & mode, const iTVPBaseBitmap* image, iTJSDispatch2* meta );
	void Header( tTJSBinaryStream *src, iTJSDispatch2** dic );
	bool AcceptSave( const ttstr & type, iTJSDispatch2** dic )
	{
		if( AcceptHandler == NULL ) return false;
		return AcceptHandler( FormatData, type, dic );
	}
};
struct tTVPGraphicMetaInfoPair
{
	ttstr Name;
	ttstr Value;
	tTVPGraphicMetaInfoPair(const ttstr &name, const ttstr &value) :
		Name(name), Value(value) {;}
};

extern iTJSDispatch2 * TVPMetaInfoPairsToDictionary( std::vector<tTVPGraphicMetaInfoPair> *vec );
extern void TVPPushGraphicCache( const ttstr& nname, class tTVPBitmap* bmp,
	std::vector<tTVPGraphicMetaInfoPair>* meta );
extern tTVPGraphicHandlerType* TVPGetGraphicLoadHandler( const ttstr& ext );
extern bool TVPCheckImageCache( const ttstr& nname, tTVPBaseBitmap* dest,
	tTVPGraphicLoadMode mode, tjs_uint dw, tjs_uint dh, tjs_int32 keyidx,
	iTJSDispatch2** metainfo );
extern bool TVPHasImageCache( const ttstr& nname, tTVPGraphicLoadMode mode,
	tjs_uint dw, tjs_uint dh, tjs_int32 keyidx );
extern void TVPLoadImageHeader( const ttstr & storagename, iTJSDispatch2** dic );
extern void TVPSaveImage(const ttstr & storagename, const ttstr & mode, const iTVPBaseBitmap* image, iTJSDispatch2* meta);
extern bool TVPGetSaveOption( const ttstr & type, iTJSDispatch2** dic );
//---------------------------------------------------------------------------



#endif
