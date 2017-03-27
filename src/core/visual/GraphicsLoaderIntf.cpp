//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Graphics Loader ( loads graphic format from storage )
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include <stdlib.h>
#include "GraphicsLoaderIntf.h"
#include "LayerBitmapIntf.h"
#include "LayerIntf.h"
#include "StorageIntf.h"
#include "MsgIntf.h"
#include "tjsHashSearch.h"
#include "EventIntf.h"
#include "SysInitIntf.h"
#include "DebugIntf.h"
#include "tvpgl.h"
#include "TickCount.h"
#include "DetectCPU.h"
#include "UtilStreams.h"
#include "tjsDictionary.h"
#include "ScriptMgnIntf.h"
#include "RenderManager.h"
#include "ConfigManager/LocaleConfigManager.h"
#include <mutex>
#include <thread>
#include <condition_variable>
#include "Application.h"
#include "BitmapIntf.h"
#include "GraphicsLoadThread.h"
#include <complex>
#include <list>

void TVPLoadPVRv3(void* formatdata, void *callbackdata,
	tTVPGraphicSizeCallback sizecallback, tTVPGraphicScanLineCallback scanlinecallback,
	tTVPMetaInfoPushCallback metainfopushcallback, tTJSBinaryStream *src, tjs_int keyidx,
	tTVPGraphicLoadMode mode);
void TVPLoadHeaderPVRv3(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic);

static void TVPLoadGraphicRouter(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx, tTVPGraphicLoadMode mode) {
	uint8_t header[16];
	tjs_uint64 origSrcPos = src->GetPosition();
	if (src->Read(header, sizeof(header)) == sizeof(header)) {
		src->SetPosition(origSrcPos);
#define CALL_LOAD_FUNC(f) f(formatdata, callbackdata, sizecallback, scanlinecallback, metainfopushcallback, src, keyidx, mode)
		if (!memcmp(header, "BM", 2)) {
			return CALL_LOAD_FUNC(TVPLoadBMP);
		}
		if (!memcmp(header, "\x89PNG", 4)) {
			return CALL_LOAD_FUNC(TVPLoadPNG);
		}
		if (!memcmp(header, "TLG", 3)) {
			return CALL_LOAD_FUNC(TVPLoadTLG);
		}
		if (!memcmp(header, "\xFF\xD8\xFF", 3) &&
			header[3] >= 0xE0 && header[3] <= 0xEF ) {
			return CALL_LOAD_FUNC(TVPLoadJPEG);
		}
		if (!memcmp(header, "BPG", 3)) {
			return CALL_LOAD_FUNC(TVPLoadBPG);
		}
		if (!memcmp(header, "RIFF", 4) && !memcmp(header + 8, "WEBPVP8", 7)) {
			return CALL_LOAD_FUNC(TVPLoadWEBP);
		}
		if (!memcmp(header, "\x49\x49\xbc\x01", 4)) {
			return CALL_LOAD_FUNC(TVPLoadJXR);
		}
		if (!memcmp(header, "PVR\3", 4)) {
			return CALL_LOAD_FUNC(TVPLoadPVRv3);
		}
#undef CALL_LOAD_FUNC
	}
	TVPThrowExceptionMessage(TVPImageLoadError, TJS_W("Invalid image"));
}

static void TVPLoadHeaderRouter(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic) {
	uint8_t header[16];
	tjs_uint64 origSrcPos = src->GetPosition();
	if (src->Read(header, sizeof(header)) == sizeof(header)) {
		src->SetPosition(origSrcPos);
#define CALL_LOAD_FUNC(f) f(formatdata, src, dic)
		if (!memcmp(header, "BM", 2)) {
			return CALL_LOAD_FUNC(TVPLoadHeaderBMP);
		}
		if (!memcmp(header, "\x89PNG", 4)) {
			return CALL_LOAD_FUNC(TVPLoadHeaderPNG);
		}
		if (!memcmp(header, "TLG", 3)) {
			return CALL_LOAD_FUNC(TVPLoadHeaderTLG);
		}
		if (!memcmp(header, "\xFF\xD8\xFF", 3) &&
			header[3] >= 0xE0 && header[3] <= 0xEF) {
			return CALL_LOAD_FUNC(TVPLoadHeaderJPG);
		}
		if (!memcmp(header, "BPG", 3)) {
			return CALL_LOAD_FUNC(TVPLoadHeaderBPG);
		}
		if (!memcmp(header, "RIFF", 4) && !memcmp(header + 8, "WEBPVP8", 7)) {
			return CALL_LOAD_FUNC(TVPLoadHeaderWEBP);
		}
		if (!memcmp(header, "\x49\x49\xbc\x01", 4)) {
			return CALL_LOAD_FUNC(TVPLoadHeaderJXR);
		}
		if (!memcmp(header, "PVR\3", 4)) {
			return CALL_LOAD_FUNC(TVPLoadHeaderPVRv3);
		}
#undef CALL_LOAD_FUNC
	}
	TVPThrowExceptionMessage(TVPImageLoadError, TJS_W("Invalid image"));
}

//---------------------------------------------------------------------------


bool TVPAcceptSaveAsBMP( void* formatdata, const ttstr & type, class iTJSDispatch2** dic )
{
	bool result = false;
	if( type.StartsWith(TJS_W("bmp")) ) result = true;
	else if( type == TJS_W(".bmp") ) result = true;
	else if( type == TJS_W(".dib") ) result = true;
	if( result && dic ) {
		tTJSVariant result;
		TVPExecuteExpression(
			TJS_W("(const)%[")
			TJS_W("\"bpp\"=>(const)%[\"type\"=>\"select\",\"items\"=>(const)[\"32\",\"24\",\"8\"],\"desc\"=>\"bpp\",\"default\"=>0]")
			TJS_W("]"),
			NULL, &result );
		if( result.Type() == tvtObject ) {
			*dic = result.AsObject();
		}
		//*dic = TJSCreateDictionaryObject();
	}
	return result;
}
//---------------------------------------------------------------------------
// Graphics Format Management
//---------------------------------------------------------------------------

class tTVPGraphicType
{
public:
	tTJSHashTable<ttstr, tTVPGraphicHandlerType> Hash;
	std::vector<tTVPGraphicHandlerType> Handlers;

	static bool Avail;

	tTVPGraphicType()
	{
		// register some native-supported formats
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".bmp"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsBMP, TVPAcceptSaveAsBMP, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".dib"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsBMP, TVPAcceptSaveAsBMP, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".jpeg"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsJPG, TVPAcceptSaveAsJPG, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".jpg"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsJPG, TVPAcceptSaveAsJPG, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".jif"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsJPG, TVPAcceptSaveAsJPG, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".png"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsPNG, TVPAcceptSaveAsPNG, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".tlg"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsTLG, TVPAcceptSaveAsTLG, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".tlg5"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsTLG, TVPAcceptSaveAsTLG, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".tlg6"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsTLG, TVPAcceptSaveAsTLG, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".jxr"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, TVPSaveAsJXR, TVPAcceptSaveAsJXR, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".bpg"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, nullptr, nullptr, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".webp"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, nullptr, nullptr, NULL));
		Handlers.push_back(tTVPGraphicHandlerType(
			TJS_W(".pvr"), TVPLoadGraphicRouter, TVPLoadHeaderRouter, nullptr, nullptr, NULL));
		ReCreateHash();
		Avail = true;
	}

	~tTVPGraphicType()
	{
		Avail = false;
	}

	void ReCreateHash()
	{
		// re-create hash table for faster search

		std::vector<tTVPGraphicHandlerType>::iterator i;
		for(i = Handlers.begin();
			i!= Handlers.end(); i++)
		{
			Hash.Add(i->Extension, *i);
		}
	}

	void Register( const tTVPGraphicHandlerType& hander )
	{
		// register graphic format to the table.
		Handlers.push_back(hander);
		ReCreateHash();
	}

	void Unregister( const tTVPGraphicHandlerType& hander )
	{
		// unregister format from table.

		std::vector<tTVPGraphicHandlerType>::iterator i;

		if(Handlers.size() > 0)
		{
			//for(i = Handlers.end() -1; i >= Handlers.begin(); i--)
			for(i = Handlers.begin(); i != Handlers.end(); i++)
			{
				if(hander == *i)
				{
					Handlers.erase(i);
					break;
				}
			}
		}

		ReCreateHash();
	}

} static TVPGraphicType;
bool tTVPGraphicType::Avail = false;
//---------------------------------------------------------------------------
void TVPRegisterGraphicLoadingHandler(const ttstr & name,
	tTVPGraphicLoadingHandler loading,
	tTVPGraphicHeaderLoadingHandler header,
	tTVPGraphicSaveHandler save,
	tTVPGraphicAcceptSaveHandler accept,
	void * formatdata)
{
	// name must be un-capitalized
	if(TVPGraphicType.Avail)
	{
		TVPGraphicType.Register(tTVPGraphicHandlerType(name, loading, header, save, accept, formatdata));
	}
}
//---------------------------------------------------------------------------
void TVPUnregisterGraphicLoadingHandler(const ttstr & name,
	tTVPGraphicLoadingHandler loading,
	tTVPGraphicHeaderLoadingHandler header,
	tTVPGraphicSaveHandler save,
	tTVPGraphicAcceptSaveHandler accept,
	void * formatdata)
{
	// name must be un-capitalized
	if(TVPGraphicType.Avail)
	{
		TVPGraphicType.Unregister(tTVPGraphicHandlerType(name, loading, header, save, accept, formatdata));
	}
}
//---------------------------------------------------------------------------
void TVPRegisterGraphicLoadingHandler(const ttstr & name,
	tTVPGraphicLoadingHandlerForPlugin loading,
	tTVPGraphicHeaderLoadingHandlerForPlugin header,
	tTVPGraphicSaveHandlerForPlugin save,
	tTVPGraphicAcceptSaveHandler accept,
	void* formatdata)
{
	// name must be un-capitalized
	if(TVPGraphicType.Avail)
	{
		TVPGraphicType.Register(tTVPGraphicHandlerType(name, loading, header, save, accept, formatdata));
	}
}
//---------------------------------------------------------------------------
void TVPUnregisterGraphicLoadingHandler(const ttstr & name,
	tTVPGraphicLoadingHandlerForPlugin loading,
	tTVPGraphicHeaderLoadingHandlerForPlugin header,
	tTVPGraphicSaveHandlerForPlugin save,
	tTVPGraphicAcceptSaveHandler accept,
	void* formatdata)
{
	// name must be un-capitalized
	if(TVPGraphicType.Avail)
	{
		TVPGraphicType.Unregister(tTVPGraphicHandlerType(name, loading, header, save, accept, formatdata));
	}
}

tTVPGraphicHandlerType* TVPGuessGraphicLoadHandler(ttstr& name) { // with name has no extension
	// suggest registered extensions
	tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
	for (i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); /*i++*/)
	{
		ttstr newname = name + i.GetKey();
		if (TVPIsExistentStorage(newname))
		{
			// file found
			name = newname;
			break;
		}
		i++;
	}
	if (i.IsNull())
	{
		// not found
		return nullptr;
	}
	return &i.GetValue();
}
//---------------------------------------------------------------------------
tTVPGraphicHandlerType* TVPGetGraphicLoadHandler( const ttstr& ext )
{
	return TVPGraphicType.Hash.Find(ext);
}
/*
	loading handlers return whether the image contains an alpha channel.
*/
//---------------------------------------------------------------------------
const void* tTVPBitmapScanLineCallbackForSave(void *callbackdata, tjs_int y)
{
	tTVPBaseBitmap* image = (tTVPBaseBitmap*)callbackdata;
	return image->GetScanLine(y);
}
//---------------------------------------------------------------------------
void TVPLoadImageHeader( const ttstr & storagename, iTJSDispatch2** dic )
{
	if( dic == NULL ) return;

	ttstr ext = TVPExtractStorageExt(storagename);
	if(ext == TJS_W("")) TVPThrowExceptionMessage(TVPUnknownGraphicFormat, storagename);
	tTVPGraphicHandlerType * handler = TVPGraphicType.Hash.Find(ext);
	if(!handler) TVPThrowExceptionMessage(TVPUnknownGraphicFormat, storagename);

	tTVPStreamHolder holder(storagename); // open a storage named "storagename"
	handler->Header( holder.Get(), dic );
}
//---------------------------------------------------------------------------
void TVPSaveImage( const ttstr & storagename, const ttstr & mode, const iTVPBaseBitmap* image, iTJSDispatch2* meta )
{
	if(!image->Is32BPP())
		TVPThrowInternalError;

	tTVPGraphicHandlerType * handler;
	tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
	for(i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); i++)
	{
		handler = & i.GetValue();
		if( handler->AcceptSave( mode, NULL ) )
		{
			break;
		}
		else
		{
			handler = NULL;
		}
	}
	if( handler ) handler->Save( storagename, mode, image, meta );
	else TVPThrowExceptionMessage(TVPUnknownGraphicFormat, mode);
}
//---------------------------------------------------------------------------
bool TVPGetSaveOption( const ttstr & type, iTJSDispatch2** dic )
{
	tTVPGraphicHandlerType * handler;
	tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
	for(i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); i++ )
	{
		handler = & i.GetValue();
		if( handler->AcceptSave( type, dic ) )
		{
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// BMP loading handler
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#define TVP_BMP_READ_LINE_MAX 8
void TVPInternalLoadBMP(void *callbackdata,
	tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback,
	TVP_WIN_BITMAPINFOHEADER &bi,
	const tjs_uint8 *palsrc,
	tTJSBinaryStream * src,
	tjs_int keyidx,
	tTVPBMPAlphaType alphatype,
	tTVPGraphicLoadMode mode)
{
	// mostly taken ( but totally re-written ) from SDL,
	// http://www.libsdl.org/

	// TODO: only checked on Win32 platform


	if(bi.biSize == 12)
	{
		// OS/2
		bi.biCompression = BI_RGB;
		bi.biClrUsed = 1 << bi.biBitCount;
	}

	tjs_uint16 orgbitcount = bi.biBitCount;
	if(bi.biBitCount == 1 || bi.biBitCount == 4)
	{
		bi.biBitCount = 8;
	}

	switch(bi.biCompression)
	{
	case BI_RGB:
		// if there are no masks, use the defaults
		break; // use default
/*
		if( bf.bfOffBits == ( 14 + bi.biSize) )
		{
		}
		// fall through -- read the RGB masks
*/
	case BI_BITFIELDS:
		TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPBitFieldsNotSupported );

	default:
 		TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPCompressedBmpNotSupported );
	}

	// load palette
	tjs_uint32 palette[256];   // (msb) argb (lsb)
	if(orgbitcount <= 8)
	{
		if(bi.biClrUsed == 0) bi.biClrUsed = 1 << orgbitcount ;
		if(bi.biSize == 12)
		{
			// read OS/2 palette
			for(tjs_uint i = 0; i < bi.biClrUsed; i++)
			{
				palette[i] = (palsrc[0]<<16) + (palsrc[1]<<8) + (palsrc[2]<<0) +
					0xff000000;
				palsrc += 3;
			}
		}
		else
		{
			// read Windows palette
			for(tjs_uint i = 0; i<bi.biClrUsed; i++)
			{
				palette[i] = (palsrc[0]<<16) + (palsrc[1]<<8) + (palsrc[2]<<0) +
					0xff000000;
					// we assume here that the palette's unused segment is useless.
					// fill it with 0xff ( = completely opaque )
				palsrc += 4;
			}
		}

		if(mode == glmGrayscale)
		{
			TVPDoGrayScale(palette, 256);
		}

		if(keyidx != -1)
		{
			// if color key by palette index is specified
			palette[keyidx&0xff] &= 0x00ffffff; // make keyidx transparent
		}
	}
	else
	{
		if(mode == glmPalettized)
			TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPUnsupportedColorModeForPalettImage );
	}

	tjs_int height;
	height = bi.biHeight<0?-bi.biHeight:bi.biHeight;
		// positive value of bi.biHeight indicates top-down DIB

	tTVPGraphicPixelFormat pixfmt;
	switch (orgbitcount) {
	case 32: pixfmt = gpfRGBA; break;
	case 24: case 16: case 15:
		pixfmt = gpfRGB; break;
	default:
		pixfmt = gpfPalette; break;
	}
	sizecallback(callbackdata, bi.biWidth, height, pixfmt);

	tjs_int pitch;
	pitch = (((bi.biWidth * orgbitcount) + 31) & ~31) /8;
	tjs_uint8 *readbuf = (tjs_uint8 *)TJSAlignedAlloc(pitch * TVP_BMP_READ_LINE_MAX, 4);
	tjs_uint8 *buf;
	tjs_int bufremain = 0;
	try
	{
		// process per a line
		tjs_int src_y = 0;
		tjs_int dest_y;
		if(bi.biHeight>0) dest_y = bi.biHeight-1; else dest_y = 0;

		for(; src_y < height; src_y++)
		{
			if(bufremain == 0)
			{
				tjs_int remain = height - src_y;
				tjs_int read_lines = remain > TVP_BMP_READ_LINE_MAX ?
					TVP_BMP_READ_LINE_MAX : remain;
				src->ReadBuffer(readbuf, pitch * read_lines);
				bufremain = read_lines;
				buf = readbuf;
			}

			void *scanline = scanlinecallback(callbackdata, dest_y);
			if(!scanline) break;

			switch(orgbitcount)
			{
				// convert pixel format
			case 1:
				if(mode == glmPalettized)
				{
					TVPBLExpand1BitTo8Bit(
						(tjs_uint8*)scanline,
						(tjs_uint8*)buf, bi.biWidth);
				}
				else if(mode == glmGrayscale)
				{
					TVPBLExpand1BitTo8BitPal(
						(tjs_uint8*)scanline,
						(tjs_uint8*)buf, bi.biWidth, palette);
				}
				else
				{
					TVPBLExpand1BitTo32BitPal(
						(tjs_uint32*)scanline,
						(tjs_uint8*)buf, bi.biWidth, palette);
				}
				break;

			case 4:
				if(mode == glmPalettized)
				{
					TVPBLExpand4BitTo8Bit(
						(tjs_uint8*)scanline,
						(tjs_uint8*)buf, bi.biWidth);
				}
				else if(mode == glmGrayscale)
				{
					TVPBLExpand4BitTo8BitPal(
						(tjs_uint8*)scanline,
						(tjs_uint8*)buf, bi.biWidth, palette);
				}
				else
				{
					TVPBLExpand4BitTo32BitPal(
						(tjs_uint32*)scanline,
						(tjs_uint8*)buf, bi.biWidth, palette);
				}
				break;

			case 8:
				if(mode == glmPalettized)
				{
					// intact copy
					memcpy(scanline, buf, bi.biWidth);
				}
				else
				if(mode == glmGrayscale)
				{
					// convert to grayscale
					TVPBLExpand8BitTo8BitPal(
						(tjs_uint8*)scanline,
						(tjs_uint8*)buf, bi.biWidth, palette);
				}
				else
				{
					TVPBLExpand8BitTo32BitPal(
						(tjs_uint32*)scanline,
						(tjs_uint8*)buf, bi.biWidth, palette);
				}
				break;

			case 15:
			case 16:
				if(mode == glmGrayscale)
				{
					TVPBLConvert15BitTo8Bit(
						(tjs_uint8*)scanline,
						(tjs_uint16*)buf, bi.biWidth);
				}
				else
				{
					TVPBLConvert15BitTo32Bit(
						(tjs_uint32*)scanline,
						(tjs_uint16*)buf, bi.biWidth);
				}
				break;

			case 24:
				if(mode == glmGrayscale)
				{
					TVPBLConvert24BitTo8Bit(
						(tjs_uint8*)scanline,
						(tjs_uint8*)buf, bi.biWidth);
				}
				else
				{
					TVPBLConvert24BitTo32Bit(
						(tjs_uint32*)scanline,
						(tjs_uint8*)buf, bi.biWidth);
				}
				break;

			case 32:
				if(mode == glmGrayscale)
				{
					TVPBLConvert32BitTo8Bit(
						(tjs_uint8*)scanline,
						(tjs_uint32*)buf, bi.biWidth);
				}
				else
				{
					if(alphatype == batNone)
					{
						// alpha channel is not given by the bitmap.
						// destination alpha is filled with 255.
						TVPBLConvert32BitTo32Bit_NoneAlpha(
							(tjs_uint32*)scanline,
							(tjs_uint32*)buf, bi.biWidth);
					}
					else if(alphatype == batMulAlpha)
					{
						// this is the TVP native representation of the alpha channel.
						// simply copy from the buffer.
						TVPBLConvert32BitTo32Bit_MulAddAlpha(
							(tjs_uint32*)scanline,
							(tjs_uint32*)buf, bi.biWidth);
					}
					else if(alphatype == batAddAlpha)
					{
						// this is alternate representation of the alpha channel,
						// this must be converted to TVP native representation.
						TVPBLConvert32BitTo32Bit_AddAlpha(
							(tjs_uint32*)scanline,
							(tjs_uint32*)buf, bi.biWidth);

					}
				}
				break;
			}

			scanlinecallback(callbackdata, -1); // image was written

			if(bi.biHeight>0) dest_y--; else dest_y++;
			buf += pitch;
			bufremain--;
		}


	}
	catch(...)
	{
		TJSAlignedDealloc(readbuf);
		throw;
	}

	TJSAlignedDealloc(readbuf);
}
//---------------------------------------------------------------------------
void TVPLoadBMP(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode)
{
	// Windows BMP Loader
	// mostly taken ( but totally re-written ) from SDL,
	// http://www.libsdl.org/

	// TODO: only checked in Win32 platform



	tjs_uint64 firstpos = src->GetPosition();

	// check the magic
	tjs_uint8 magic[2];
	src->ReadBuffer(magic, 2);
	if(magic[0] != TJS_N('B') || magic[1] != TJS_N('M'))
		TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPNotWindowsBmp );

	// read the BITMAPFILEHEADER
	TVP_WIN_BITMAPFILEHEADER bf;
	bf.bfSize = src->ReadI32LE();
	bf.bfReserved1 = src->ReadI16LE();
	bf.bfReserved2 = src->ReadI16LE();
	bf.bfOffBits = src->ReadI32LE();

	// read the BITMAPINFOHEADER
	TVP_WIN_BITMAPINFOHEADER bi;
	bi.biSize = src->ReadI32LE();
	if(bi.biSize == 12)
	{
		// OS/2 Bitmap
		memset(&bi, 0, sizeof(bi));
		bi.biWidth = (tjs_uint32)src->ReadI16LE();
		bi.biHeight = (tjs_uint32)src->ReadI16LE();
		bi.biPlanes = src->ReadI16LE();
		bi.biBitCount = src->ReadI16LE();
		bi.biClrUsed = 1 << bi.biBitCount;
	}
	else if(bi.biSize == 40)
	{
		// Windows Bitmap
		bi.biWidth = src->ReadI32LE();
		bi.biHeight = src->ReadI32LE();
		bi.biPlanes = src->ReadI16LE();
		bi.biBitCount = src->ReadI16LE();
		bi.biCompression = src->ReadI32LE();
		bi.biSizeImage = src->ReadI32LE();
		bi.biXPelsPerMeter = src->ReadI32LE();
		bi.biYPelsPerMeter = src->ReadI32LE();
		bi.biClrUsed = src->ReadI32LE();
		bi.biClrImportant = src->ReadI32LE();
	}
	else
	{
		TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPUnsupportedHeaderVersion );
	}


	// load palette
	tjs_int palsize = (bi.biBitCount <= 8) ?
		((bi.biClrUsed == 0 ? (1<<bi.biBitCount) : bi.biClrUsed) *
		((bi.biSize == 12) ? 3:4)) : 0;  // bi.biSize == 12 ( OS/2 palette )
	tjs_uint8 *palette = NULL;

	if(palsize) palette = new tjs_uint8 [palsize];

	try
	{
		src->ReadBuffer(palette, palsize);
		src->SetPosition(firstpos + bf.bfOffBits);

		TVPInternalLoadBMP(callbackdata, sizecallback, scanlinecallback,
			bi, palette, src, keyidx, batMulAlpha, mode);
	}
	catch(...)
	{
		if(palette) delete [] palette;
		throw;
	}
	if(palette) delete [] palette;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// BMP saving handler
//---------------------------------------------------------------------------
static void TVPWriteLE16(tTJSBinaryStream * stream, tjs_uint16 number)
{
	tjs_uint8 data[2];
	data[0] = number & 0xff;
	data[1] = (number >> 8) & 0xff;
	stream->WriteBuffer(data, 2);
}
//---------------------------------------------------------------------------
static void TVPWriteLE32(tTJSBinaryStream * stream, tjs_uint32 number)
{
	tjs_uint8 data[4];
	data[0] = number & 0xff;
	data[1] = (number >> 8) & 0xff;
	data[2] = (number >> 16) & 0xff;
	data[3] = (number >> 24) & 0xff;
	stream->WriteBuffer(data, 4);
}
//---------------------------------------------------------------------------
void TVPSaveAsBMP( void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* bmp, const ttstr & mode, iTJSDispatch2* meta )
{
	tjs_int pixelbytes;

	if(mode == TJS_W("bmp32") || mode == TJS_W("bmp"))
		pixelbytes = 4;
	else if(mode == TJS_W("bmp24"))
		pixelbytes = 3;
	else if(mode == TJS_W("bmp8"))
		pixelbytes = 1;
	else
		pixelbytes = 4;

	if( meta )
	{
		tTJSVariant val;
		tjs_error er = meta->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("bpp"), NULL, &val, meta);
		if(TJS_SUCCEEDED(er))
		{
			tjs_int index = (tjs_int)val.AsInteger();
			switch( index ) {
			case 0: pixelbytes = 4; break;
			case 1: pixelbytes = 3; break;
			case 2: pixelbytes = 1; break;
			};
		}
	}

	// open stream
	tTJSBinaryStream *stream = dst;
	tjs_uint8 * buf = NULL;

	try
	{
		TVPClearGraphicCache();

		// prepare header
		tjs_uint bmppitch = bmp->GetWidth() * pixelbytes;
		bmppitch = (((bmppitch - 1) >> 2) + 1) << 2;

		TVPWriteLE16(stream, 0x4d42);  /* bfType */
		TVPWriteLE32(stream, sizeof(TVP_WIN_BITMAPFILEHEADER) +
				sizeof(TVP_WIN_BITMAPINFOHEADER) + bmppitch * bmp->GetHeight() +
				(pixelbytes == 1 ? 1024 : 0)); /* bfSize */
		TVPWriteLE16(stream, 0); /* bfReserved1 */
		TVPWriteLE16(stream, 0); /* bfReserved2 */
		TVPWriteLE32(stream, sizeof(TVP_WIN_BITMAPFILEHEADER) +
				sizeof(TVP_WIN_BITMAPINFOHEADER)+
				(pixelbytes == 1 ? 1024 : 0)); /* bfOffBits */

		TVPWriteLE32(stream, sizeof(TVP_WIN_BITMAPINFOHEADER)); /* biSize */
		TVPWriteLE32(stream, bmp->GetWidth()); /* biWidth */
		TVPWriteLE32(stream, bmp->GetHeight()); /* biHeight */
		TVPWriteLE16(stream, 1); /* biPlanes */
		TVPWriteLE16(stream, pixelbytes * 8); /* biBitCount */
		TVPWriteLE32(stream, BI_RGB); /* biCompression */
		TVPWriteLE32(stream, 0); /* biSizeImage */
		TVPWriteLE32(stream, 0); /* biXPelsPerMeter */
		TVPWriteLE32(stream, 0); /* biYPelsPerMeter */
		TVPWriteLE32(stream, 0); /* biClrUsed */
		TVPWriteLE32(stream, 0); /* biClrImportant */

		// write palette
		if(pixelbytes == 1)
		{
			tjs_uint8 palette[1024];
			tjs_uint8 * p = palette;
			for(tjs_int i = 0; i < 256; i++)
			{
				p[0] = TVP252DitherPalette[0][i];
				p[1] = TVP252DitherPalette[1][i];
				p[2] = TVP252DitherPalette[2][i];
				p[3] = 0;
				p += 4;
			}
			stream->WriteBuffer(palette, 1024);
		}

		// write bitmap body
		for(tjs_int y = bmp->GetHeight() - 1; y >= 0; y --)
		{
			if(!buf) buf = new tjs_uint8[bmppitch];
			if(pixelbytes == 4)
			{
				TVPReverseRGB((tjs_uint32 *)buf, (const tjs_uint32 *)bmp->GetScanLine(y), bmp->GetWidth());
			}
			else if(pixelbytes == 1)
			{
				TVPDither32BitTo8Bit(buf, (const tjs_uint32*)bmp->GetScanLine(y),
					bmp->GetWidth(), 0, y);  
			}
			else
			{
				const tjs_uint8 *src = (const tjs_uint8 *)bmp->GetScanLine(y);
				tjs_uint8 *dest = buf;
				tjs_int w = bmp->GetWidth();
				for(tjs_int x = 0; x < w; x++)
				{
					dest[0] = src[2];
					dest[1] = src[1];
					dest[2] = src[0];
					dest += 3;
					src += 4;
				}
			}
			stream->WriteBuffer(buf, bmppitch);
		}
	}
	catch(...)
	{
		if(buf) delete [] buf;
		throw;
	}
	if(buf) delete [] buf;
}
//---------------------------------------------------------------------------

void TVPLoadHeaderBMP( void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic )
{
	tjs_uint64 firstpos = src->GetPosition();

	// check the magic
	tjs_uint8 magic[2];
	src->ReadBuffer(magic, 2);
	if(magic[0] != TJS_N('B') || magic[1] != TJS_N('M'))
		TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPNotWindowsBmp );

	// read the BITMAPFILEHEADER
	TVP_WIN_BITMAPFILEHEADER bf;
	bf.bfSize = src->ReadI32LE();
	bf.bfReserved1 = src->ReadI16LE();
	bf.bfReserved2 = src->ReadI16LE();
	bf.bfOffBits = src->ReadI32LE();

	// read the BITMAPINFOHEADER
	TVP_WIN_BITMAPINFOHEADER bi;
	bi.biSize = src->ReadI32LE();
	if(bi.biSize == 12)
	{
		// OS/2 Bitmap
		memset(&bi, 0, sizeof(bi));
		bi.biWidth = (tjs_uint32)src->ReadI16LE();
		bi.biHeight = (tjs_uint32)src->ReadI16LE();
		bi.biPlanes = src->ReadI16LE();
		bi.biBitCount = src->ReadI16LE();
		bi.biClrUsed = 1 << bi.biBitCount;
	}
	else if(bi.biSize == 40)
	{
		// Windows Bitmap
		bi.biWidth = src->ReadI32LE();
		bi.biHeight = src->ReadI32LE();
		bi.biPlanes = src->ReadI16LE();
		bi.biBitCount = src->ReadI16LE();
		bi.biCompression = src->ReadI32LE();
		bi.biSizeImage = src->ReadI32LE();
		bi.biXPelsPerMeter = src->ReadI32LE();
		bi.biYPelsPerMeter = src->ReadI32LE();
		bi.biClrUsed = src->ReadI32LE();
		bi.biClrImportant = src->ReadI32LE();
	}
	else
	{
		TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPUnsupportedHeaderVersion );
	}

	tjs_int palsize = (bi.biBitCount <= 8) ?
		((bi.biClrUsed == 0 ? (1<<bi.biBitCount) : bi.biClrUsed) *
		((bi.biSize == 12) ? 3:4)) : 0;  // bi.biSize == 12 ( OS/2 palette )
	palsize = palsize > 0 ? 1 : 0;

	*dic = TJSCreateDictionaryObject();
	tTJSVariant val(bi.biWidth);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("width"), 0, &val, (*dic) );
	val = tTJSVariant(bi.biHeight);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("height"), 0, &val, (*dic) );
	val = tTJSVariant(bi.biBitCount);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("bpp"), 0, &val, (*dic) );
	val = tTJSVariant(palsize);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("palette"), 0, &val, (*dic) );
}

//---------------------------------------------------------------------------
// TVPLoadGraphic related
//---------------------------------------------------------------------------
enum tTVPLoadGraphicType
{
	lgtFullColor, // full 32bit color
	lgtPalGray, // palettized or grayscale
	lgtMask // mask
};
struct tTVPLoadGraphicData
{
	ttstr Name;
	tTVPBitmap *Dest;
	tTVPLoadGraphicType Type;
	tjs_int ColorKey;
	tjs_uint8 *Buffer;
	tjs_uint ScanLineNum;
	tjs_uint DesW;
	tjs_uint DesH;
	tjs_uint OrgW;
	tjs_uint OrgH;
	tjs_uint BufW;
	tjs_uint BufH;
	bool NeedMetaInfo;
	std::vector<tTVPGraphicMetaInfoPair> * MetaInfo;
};
//---------------------------------------------------------------------------
static int TVPLoadGraphic_SizeCallback(void *callbackdata, tjs_uint w,
	tjs_uint h, tTVPGraphicPixelFormat fmt)
{
	tTVPLoadGraphicData * data = (tTVPLoadGraphicData *)callbackdata;

	// check size
	data->OrgW = w;
	data->OrgH = h;
	if(data->DesW && w < data->DesW) w = data->DesW;
	if(data->DesH && h < data->DesH) h = data->DesH;
	data->BufW = w;
	data->BufH = h;

	// create buffer
	if(data->Type == lgtMask)
	{
		// mask ( _m ) load

		// check the image previously loaded
		if(data->Dest->GetWidth() != w || data->Dest->GetHeight() != h)
			TVPThrowExceptionMessage(TVPMaskSizeMismatch);

		// allocate line buffer
		data->Buffer = new tjs_uint8 [w];
		data->Dest->IsOpaque = false;
		return w;
	}
	else
	{
		// normal load or province load
		if (!data->Dest) {
			data->Dest = new tTVPBitmap(w, h, data->Type == lgtFullColor ? 32 : 8);
		} else if (data->Dest->GetWidth() != w || data->Dest->GetHeight() != h) {
			data->Dest->Release();
			data->Dest = new tTVPBitmap(w, h, data->Type == lgtFullColor ? 32 : 8);
		}
		switch(fmt) {
		case gpfLuminance:
		case gpfRGB:
			data->Dest->IsOpaque = true; break;
		case gpfPalette:
		case gpfRGBA:
			data->Dest->IsOpaque = false; break;
		}
#if 0
		data->Dest->Recreate(w, h, data->Type!=lgtFullColor?8:32);
#endif
		return data->Dest->GetPitch();
	}
}
//---------------------------------------------------------------------------
static void * TVPLoadGraphic_ScanLineCallback(void *callbackdata, tjs_int y)
{
	tTVPLoadGraphicData * data = (tTVPLoadGraphicData *)callbackdata;

	if(y >= 0)
	{
		// query of line buffer

		data->ScanLineNum = y;
		if(data->Type == lgtMask)
		{
			// mask
			return data->Buffer;
		}
		else
		{
			// return the scanline for writing
			return data->Dest->GetScanLine(y);
		}
	}
	else
	{
		// y==-1 indicates the buffer previously returned was written

		if(data->Type == lgtMask)
		{
			// mask

				// tile for horizontal direction
				tjs_uint i;
			for(i = data->OrgW; i<data->BufW; i+=data->OrgW)
				{
					tjs_uint w = data->BufW - i;
					w = w > data->OrgW ? data->OrgW : w;
				memcpy(data->Buffer + i, data->Buffer, w);
				}

				// bind mask buffer to main image buffer ( and tile for vertical )
			for(i = data->ScanLineNum; i<data->BufH; i+=data->OrgH)
				{
					TVPBindMaskToMain(
					(tjs_uint32*)data->Dest->GetScanLine(i),
					data->Buffer, data->BufW);
				}
			return NULL;
			}
		else if(data->Type == lgtFullColor)
		{
			tjs_uint32 * sl =
				(tjs_uint32*)data->Dest->GetScanLine(data->ScanLineNum);
			if((data->ColorKey & 0xff000000) == 0x00000000)
			{
					// make alpha from color key
					TVPMakeAlphaFromKey(
					sl,
						data->BufW,
						data->ColorKey);
				}

					// tile for horizontal direction
					tjs_uint i;
			for(i = data->OrgW; i<data->BufW; i+=data->OrgW)
					{
						tjs_uint w = data->BufW - i;
						w = w > data->OrgW ? data->OrgW : w;
						memcpy(sl + i, sl, w * sizeof(tjs_uint32));
					}

					// tile for vertical direction
			for(i = data->ScanLineNum + data->OrgH; i<data->BufH; i+=data->OrgH)
					{
						memcpy(
							(tjs_uint32*)data->Dest->GetScanLine(i),
							sl,
					data->BufW * sizeof(tjs_uint32) );
					}

			return NULL;
		}
		else if(data->Type == lgtPalGray)
		{
			// nothing to do
			if(data->OrgW < data->BufW || data->OrgH < data->BufH)
			{
					tjs_uint8 * sl =
						(tjs_uint8*)data->Dest->GetScanLine(data->ScanLineNum);
					tjs_uint i;

					// tile for horizontal direction
				for(i = data->OrgW; i<data->BufW; i+=data->OrgW)
					{
						tjs_uint w = data->BufW - i;
						w = w > data->OrgW ? data->OrgW : w;
						memcpy(sl + i, sl, w * sizeof(tjs_uint8));
					}

					// tile for vertical direction
				for(i = data->ScanLineNum + data->OrgH; i<data->BufH; i+=data->OrgH)
					{
						memcpy(
							(tjs_uint8*)data->Dest->GetScanLine(i),
							sl,
							data->BufW * sizeof(tjs_uint8));
					}
				}

			return NULL;
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
static void TVPLoadGraphic_MetaInfoPushCallback(void *callbackdata,
	const ttstr & name, const ttstr & value)
{
	tTVPLoadGraphicData * data = (tTVPLoadGraphicData *)callbackdata;

	if(data->NeedMetaInfo)
	{
		if(!data->MetaInfo) data->MetaInfo = new std::vector<tTVPGraphicMetaInfoPair>();
		data->MetaInfo->emplace_back(name, value);
	}
}
//---------------------------------------------------------------------------
//static int _USERENTRY TVPColorCompareFunc(const void *_a, const void *_b)
static int TVPColorCompareFunc(const void *_a, const void *_b)
{
	tjs_uint32 a = *(const tjs_uint32*)_a;
	tjs_uint32 b = *(const tjs_uint32*)_b;

	if(a<b) return -1;
	if(a==b) return 0;
	return 1;
}
//---------------------------------------------------------------------------
static void TVPMakeAlphaFromAdaptiveColor(tTVPBitmap *dest)
{
	// make adaptive color key and make alpha from it.
	// adaptive color key is most used(popular) color at first line of the
	// graphic.
	if(!dest->Is32bit()) return;

	// copy first line to buffer
	tjs_int w = dest->GetWidth();
	tjs_int pitch = std::abs(dest->GetPitch());
	tjs_uint32 * buffer = new tjs_uint32[pitch];

	try
	{
		const void *d = dest->GetScanLine(0);

		memcpy(buffer, d, pitch);
		tjs_int i;
		for(i = w -1; i>=0; i--) buffer[i] &= 0xffffff;
		buffer[w] = (tjs_uint32)-1; // a sentinel

		// sort by color
		qsort(buffer, dest->GetWidth(), sizeof(tjs_uint32), TVPColorCompareFunc);

		// find most used color
		tjs_int maxlen = 0;
		tjs_uint32 maxlencolor = 0;
		tjs_uint32 pcolor = (tjs_uint32)-1;
		tjs_int l = 0;
		for(i = 0; i< w+1; i++)
		{
			if(buffer[i] != pcolor)
			{
				if(maxlen < l)
				{
					maxlen = l;
					maxlencolor = pcolor;
					l = 0;
				}
			}
			else
			{
				l++;
			}
			pcolor = buffer[i];
		}

		if(maxlencolor == (tjs_uint32)-1)
		{
			// may color be not found...
			maxlencolor = 0; // black is a default colorkey
		}

		// make alpha from maxlencolor
		tjs_int h;
		for(h = dest->GetHeight()-1; h>=0; h--)
		{
			TVPMakeAlphaFromKey((tjs_uint32*)dest->GetScanLine(h),
				w, maxlencolor);

		}

	}
	catch(...)
	{
		delete [] buffer;
		throw;
	}

	delete [] buffer;
}
//---------------------------------------------------------------------------
static void TVPDoAlphaColorMat(tTVPBitmap *dest, tjs_uint32 color)
{
	// Do alpha matting.
	// 'mat' means underlying color of the image. This function piles
	// specified color under the image, then blend. The output image
	// will be totally opaque. This function always assumes the image
	// has pixel value for alpha blend mode, not additive alpha blend mode.
	if(!dest->Is32bit()) return;

	tjs_int w = dest->GetWidth();
	tjs_int h = dest->GetHeight();

	for(tjs_int y = 0; y < h; y++)
	{
		tjs_uint32 * buffer = (tjs_uint32*)dest->GetScanLine(y);
		TVPAlphaColorMat(buffer, color, w);
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
iTJSDispatch2 * TVPMetaInfoPairsToDictionary(
	std::vector<tTVPGraphicMetaInfoPair> *vec)
{
	if(!vec) return NULL;
	std::vector<tTVPGraphicMetaInfoPair>::iterator i;
	iTJSDispatch2 *dic = TJSCreateDictionaryObject();
	try
	{
		for(i = vec->begin(); i != vec->end(); i++)
		{
			tTJSVariant val(i->Value);
			dic->PropSet(TJS_MEMBERENSURE, i->Name.c_str(), 0,
				&val, dic);
		}
	}
	catch(...)
	{
		dic->Release();
		throw;
	}
	return dic;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// Graphics Cache Management
//---------------------------------------------------------------------------
bool TVPAllocGraphicCacheOnHeap = false;
	// this allocates graphic cache's store memory on heap, rather than
	// sharing bitmap object. ( since sucking win9x cannot have so many bitmap
	// object at once, WinNT/2000 is ok. )
	// this will take more time for memory copying.
//---------------------------------------------------------------------------
struct tTVPGraphicsSearchData
{
	ttstr Name;
	tjs_int32 KeyIdx; // color key index
	tTVPGraphicLoadMode Mode; // image mode
	tjs_uint DesW; // desired width ( 0 for original size )
	tjs_uint DesH; // desired height ( 0 for original size )

	bool operator == (const tTVPGraphicsSearchData &rhs) const
	{
		return KeyIdx == rhs.KeyIdx && Mode == rhs.Mode &&
			Name == rhs.Name && DesW == rhs.DesW && DesH == rhs.DesH;
	}
};
//---------------------------------------------------------------------------
class tTVPGraphicsSearchHashFunc
{
public:
	static tjs_uint32 Make(const tTVPGraphicsSearchData &val)
	{
		tjs_uint32 v = tTJSHashFunc<ttstr>::Make(val.Name);

		v ^= val.KeyIdx + (val.KeyIdx >> 23);
		v ^= (val.Mode << 30);
		v ^= val.DesW + (val.DesW >> 8);
		v ^= val.DesH + (val.DesH >> 8);
		return v;
	}
};
//---------------------------------------------------------------------------
class tTVPGraphicImageData
{
private:
    tTVPBitmap *Bitmap;
    iTVPTexture2D *Texture = nullptr;
	tjs_uint8 * RawData;
	tjs_int Width;
	tjs_int Height;
	tjs_int PixelSize;

public:
	ttstr ProvinceName;

	std::vector<tTVPGraphicMetaInfoPair> * MetaInfo;

private:
	tjs_int RefCount;
	tjs_uint Size;

public:
	tTVPGraphicImageData()
	{
		RefCount = 1; Size = 0; Bitmap = NULL; RawData = NULL;
		MetaInfo = nullptr;
	}
	~tTVPGraphicImageData()
	{
		if (Bitmap) Bitmap->Release();
		if (Texture) Texture->Release();
		if(RawData) delete [] RawData;
		if(MetaInfo) delete MetaInfo;
	}

	void AssignBitmap(tTVPBitmap *bmp)
	{
		if(Bitmap) delete Bitmap, Bitmap = NULL;
		if(RawData) delete [] RawData, RawData = NULL;
		if (Texture) Texture->Release(), Texture = nullptr;

		Width = bmp->GetWidth();
		Height = bmp->GetHeight();
		PixelSize = bmp->GetBPP() / 8;
		Size =  Width*Height*PixelSize;

		if(!TVPAllocGraphicCacheOnHeap)
		{
			// simply assin to Bitmap
			Bitmap = bmp;
            Bitmap->AddRef();
		}
		else
		{
			// allocate heap and copy to it
			tjs_int h = Height;
			RawData = new tjs_uint8 [ Size ];
			tjs_uint8 *p = RawData;
			tjs_int rawpitch = Width * PixelSize;
			for(h--; h>=0; h--)
			{
				memcpy(p, bmp->GetScanLine(h), rawpitch);
				p += rawpitch;
			}
		}
	}

	void AssignTexture(iTVPTexture2D *tex) {
		if (Bitmap) delete Bitmap, Bitmap = NULL;
		if (RawData) delete[] RawData, RawData = NULL;
		if (Texture) Texture->Release(), Texture = nullptr;

		Width = tex->GetWidth();
		Height = tex->GetHeight();
		PixelSize = (int)tex->GetFormat();
		Size = Width*Height*PixelSize;

		Texture = tex;
		Texture->AddRef();
	}

	void AssignToBitmap(tTVPBaseBitmap *bmp) const
	{
		if(!TVPAllocGraphicCacheOnHeap)
		{
			// simply assign to Bitmap
			if(Bitmap) bmp->AssignBitmap(Bitmap);
		}
		else
		{
			// copy from the rawdata heap
			if(RawData)
			{
				bmp->Recreate(Width, Height, PixelSize==4?32:8);
				tjs_int h = Height;
				tjs_uint8 *p = RawData;
				tjs_int rawpitch = Width * PixelSize;
				for(h--; h>=0; h--)
				{
					memcpy(bmp->GetScanLineForWrite(h), p, rawpitch);
					p += rawpitch;
				}
			}
		}
	}

	void AssignToTexture(iTVPBaseBitmap *dst)
    {
        if(!Texture && Bitmap) {
            int bmpw = Bitmap->GetWidth(), bmph = Bitmap->GetHeight();
            TVPTextureFormat::e format;
            switch(PixelSize) {
            case 1:
                format = TVPTextureFormat::Gray;
                break;
            case 3:
                format = TVPTextureFormat::RGB;
                break;
            case 4:
                format = TVPTextureFormat::RGBA;
                break;
            default:
                return;
            }
			Texture = TVPGetRenderManager()->CreateTexture2D(Bitmap);
			if (Bitmap) Bitmap->Release(), Bitmap = nullptr;
			if (RawData) delete[] RawData, RawData = nullptr;
        }

        if (Texture)
            dst->AssignTexture(Texture);
    }

	tjs_uint GetSize() const { return Size; }

	void AddRef() { RefCount ++; }
	void Release()
	{
		if(RefCount == 1)
		{
			delete this;
		}
		else
		{
			RefCount--;
		}
	}
};
//---------------------------------------------------------------------------
typedef tTJSRefHolder<tTVPGraphicImageData> tTVPGraphicImageHolder;

typedef
tTJSHashTable<tTVPGraphicsSearchData, tTVPGraphicImageHolder, tTVPGraphicsSearchHashFunc>
	tTVPGraphicCache;
tTVPGraphicCache TVPGraphicCache;
static bool TVPGraphicCacheEnabled = false;
static tjs_uint64 TVPGraphicCacheLimit = 0;
static tjs_uint64 TVPGraphicCacheTotalBytes = 0;
tjs_uint64 TVPGraphicCacheSystemLimit = 0; // maximum possible value of  TVPGraphicCacheLimit
//---------------------------------------------------------------------------
tjs_uint64 TVPGetGraphicCacheTotalBytes() {
	return TVPGraphicCacheTotalBytes;
}
//---------------------------------------------------------------------------
static void TVPCheckGraphicCacheLimit()
{
	while(TVPGraphicCacheTotalBytes > TVPGraphicCacheLimit)
	{
		// chop last graphics
		tTVPGraphicCache::tIterator i;
		i = TVPGraphicCache.GetLast();
		if(!i.IsNull())
		{
			tjs_uint size = i.GetValue().GetObjectNoAddRef()->GetSize();
			TVPGraphicCacheTotalBytes -= size;
			TVPGraphicCache.ChopLast(1);
		}
		else
		{
			break;
		}
	}
}
//---------------------------------------------------------------------------
void TVPClearGraphicCache()
{
	TVPGraphicCache.Clear();
	TVPGraphicCacheTotalBytes = 0;
}
static tTVPAtExit
	TVPUninitMessageLoad(TVP_ATEXIT_PRI_RELEASE, TVPClearGraphicCache);
//---------------------------------------------------------------------------
struct tTVPClearGraphicCacheCallback : public tTVPCompactEventCallbackIntf
{
	virtual void TJS_INTF_METHOD OnCompact(tjs_int level)
	{
		if(level >= TVP_COMPACT_LEVEL_MINIMIZE)
		{
			// clear the font cache on application minimize
			TVPClearGraphicCache();
		}
	}
} static TVPClearGraphicCacheCallback;
static bool TVPClearGraphicCacheCallbackInit = false;
//---------------------------------------------------------------------------
void TVPPushGraphicCache( const ttstr& nname, tTVPBitmap* bmp, std::vector<tTVPGraphicMetaInfoPair>* meta )
{
	if( TVPGraphicCacheEnabled ) {
		// graphic compact initialization
		if(!TVPClearGraphicCacheCallbackInit)
		{
			TVPAddCompactEventHook(&TVPClearGraphicCacheCallback);
			TVPClearGraphicCacheCallbackInit = true;
		}

		tTVPGraphicImageData* data = NULL;
		try {
			tjs_uint32 hash;
			tTVPGraphicsSearchData searchdata;

			searchdata.Name = nname;
			searchdata.KeyIdx = TVP_clNone;
			searchdata.Mode = glmNormal;
			searchdata.DesW = 0;
			searchdata.DesH = 0;

			hash = tTVPGraphicCache::MakeHash(searchdata);

			data = new tTVPGraphicImageData();
			data->AssignBitmap(bmp);
			data->ProvinceName = TJS_W("");
			data->MetaInfo = meta;
			meta = NULL;

			// check size limit
			TVPCheckGraphicCacheLimit();

			// push into hash table
			tjs_uint datasize = data->GetSize();
			TVPGraphicCacheTotalBytes += datasize;
			tTVPGraphicImageHolder holder(data);
			TVPGraphicCache.AddWithHash(searchdata, hash, holder);
		} catch(...) {
			if(meta) delete meta;
			if(data) data->Release();
			throw;
		}
		if(data) data->Release();
	} else {
		if( meta ) delete meta;
	}
}
//---------------------------------------------------------------------------
bool TVPCheckImageCache( const ttstr& nname, tTVPBaseBitmap* dest, tTVPGraphicLoadMode mode, tjs_uint dw, tjs_uint dh, tjs_int32 keyidx, iTJSDispatch2** metainfo )
{
	tjs_uint32 hash;
	tTVPGraphicsSearchData searchdata;
	if(TVPGraphicCacheEnabled)
	{
		searchdata.Name = nname;
		searchdata.KeyIdx = keyidx;
		searchdata.Mode = mode;
		searchdata.DesW = dw;
		searchdata.DesH = dh;

		hash = tTVPGraphicCache::MakeHash(searchdata);

		tTVPGraphicImageHolder * ptr =
			TVPGraphicCache.FindAndTouchWithHash(searchdata, hash);
		if(ptr)
		{
			// found in cache
			ptr->GetObjectNoAddRef()->AssignToBitmap(dest);
			if(metainfo)
				*metainfo = TVPMetaInfoPairsToDictionary(ptr->GetObjectNoAddRef()->MetaInfo);
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
// åüçıÇæÇØÇ∑ÇÈ
bool TVPHasImageCache( const ttstr& nname, tTVPGraphicLoadMode mode, tjs_uint dw, tjs_uint dh, tjs_int32 keyidx )
{
	tjs_uint32 hash;
	tTVPGraphicsSearchData searchdata;
	if(TVPGraphicCacheEnabled)
	{
		searchdata.Name = nname;
		searchdata.KeyIdx = keyidx;
		searchdata.Mode = mode;
		searchdata.DesW = dw;
		searchdata.DesH = dh;

		hash = tTVPGraphicCache::MakeHash(searchdata);

		tTVPGraphicImageHolder * ptr =
			TVPGraphicCache.FindAndTouchWithHash(searchdata, hash);
		if(ptr)
		{
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
tTVPGraphicHandlerType* TVPFindGraphicLoadHandler(ttstr &_name, ttstr *maskname, ttstr *provincename) {
	// graphic compact initialization
	if (!TVPClearGraphicCacheCallbackInit)
	{
		TVPAddCompactEventHook(&TVPClearGraphicCacheCallback);
		TVPClearGraphicCacheCallbackInit = true;
	}

	// search according with its extension
	tjs_int namelen = _name.GetLen();
	ttstr name(_name);

	ttstr ext = TVPExtractStorageExt(name);
	int extlen = ext.GetLen();
	tTVPGraphicHandlerType * handler;

	if (ext.IsEmpty()) {
		// missing extension
		// suggest registered extensions
		tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
		for (i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); /*i++*/)
		{
			ttstr newname = name + i.GetKey();
			if (TVPIsExistentStorage(newname))
			{
				// file found
				name = newname;
				break;
			}
			i++;
		}
		if (i.IsNull())
		{
			// not found
			ttstr fmt = LocaleConfigManager::GetInstance()->GetText("err_cannot_suggest_graph_ext");
			TVPThrowExceptionMessage(fmt.c_str(), name);
		}

		handler = &i.GetValue();
	} else {
		handler = TVPGraphicType.Hash.Find(ext);
		if (!handler) {
			static const ttstr ext_bmp(TJS_W(".bmp"));
			handler = TVPGraphicType.Hash.Find(ext_bmp);
		}
	}

	if (!handler) TVPThrowExceptionMessage(TVPUnknownGraphicFormat, name);
	ttstr retname(name);

	if (maskname) {
		// mask image handling ( addding _m suffix with the filename )
		while (true) {
			name = ttstr(_name, namelen - extlen) + TJS_W("_m") + ext;
			if (ext.IsEmpty()) {
				// missing extension
				// suggest registered extensions
				tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
				for (i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); /*i++*/) {
					ttstr newname = name;
					newname += i.GetKey();
					if (TVPIsExistentStorage(newname))
					{
						// file found
						name = newname;
						break;
					}
					i++;
				}
				if (i.IsNull()) {
					// not found
					maskname->Clear();
					break;
				}
				*maskname = name;
				break;
			} else {
				if (!TVPIsExistentStorage(name)) {
					// not found
					ext.Clear();
					continue; // retry searching
				}
				*maskname = name;
				break;
			}
		}
	}
	if (provincename) {
		// set province name
		*provincename = ttstr(_name, namelen - extlen) + TJS_W("_p");

		// search extensions
		tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
		for (i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); /*i++*/) {
			ttstr newname = *provincename + i.GetKey();
			if (TVPIsExistentStorage(newname))
			{
				// file found
				*provincename = newname;
				break;
			}
			i++;
		}
		if (i.IsNull()) {
			// not found
			provincename->Clear();
		}
	}

	_name = retname;
	return handler;
}
//---------------------------------------------------------------------------
static tTVPBitmap* TVPInternalLoadBitmap(const ttstr &_name,
	tjs_uint32 keyidx, tjs_uint desw, tjs_int desh, std::vector<tTVPGraphicMetaInfoPair> * * MetaInfo,
		tTVPGraphicLoadMode mode, ttstr *provincename)
{
	// name must be normalized.
	// if "provincename" is non-null, this function set it to province storage
	// name ( with _p suffix ) for convinience.
	// desw and desh are desired size. if the actual picture is smaller than
	// the given size, the graphic is to be tiled. give 0,0 to obtain default
	// size graphic.

	ttstr name(_name), maskname;
	tTVPGraphicHandlerType * handler = TVPFindGraphicLoadHandler(name, &maskname, mode == glmNormal ? provincename : nullptr);
	tTVPStreamHolder holder(name); // open a storage named "name"

	// load the image
	tTVPLoadGraphicData data;
	data.Dest = nullptr;
	data.ColorKey = keyidx;
	data.Type = mode == glmNormal? lgtFullColor : lgtPalGray;
	data.Name = name;
	data.DesW = desw;
	data.DesH = desh;
	data.NeedMetaInfo = true;
	data.MetaInfo = NULL;

	bool keyadapt = (keyidx == TVP_clAdapt);
	bool doalphacolormat = TVP_Is_clAlphaMat(keyidx);
	tjs_uint32 alphamatcolor = TVP_get_clAlphaMat(keyidx);

	if(TVP_Is_clPalIdx(keyidx))
	{
		// pass the palette index number to the handler.
		// ( since only Graphic Loading Handler can process the palette information )
		keyidx = TVP_get_clPalIdx(keyidx);
	}
	else
	{
		keyidx = -1;
	}

	handler->Load(handler->FormatData, (void*)&data, TVPLoadGraphic_SizeCallback,
		TVPLoadGraphic_ScanLineCallback, TVPLoadGraphic_MetaInfoPushCallback,
		holder.Get(), keyidx, mode);

	*MetaInfo = data.MetaInfo;

	if(keyadapt && mode == glmNormal)
	{
		// adaptive color key
		TVPMakeAlphaFromAdaptiveColor(data.Dest);
	}

	if(mode != glmNormal) return data.Dest;

	if(!maskname.IsEmpty() && (handler = TVPFindGraphicLoadHandler(maskname, nullptr, nullptr)))
	{
		// open the mask file
		holder.Open(maskname);

		// fill "data"'s member
	    data.Type = lgtMask;
//	    data.Name = name;
		data.Buffer = NULL;
		data.DesW = desw;
		data.DesH = desh;
		data.NeedMetaInfo = false;

	    try
	    {
			// load image via handler
			handler->Load(handler->FormatData, (void*)&data,
				TVPLoadGraphic_SizeCallback, TVPLoadGraphic_ScanLineCallback,
				NULL,
				holder.Get(), -1, glmGrayscale);
	    }
		catch(...)
	    {
			if(data.Buffer) delete [] data.Buffer;
            if(data.Dest) data.Dest->Release();
			throw;
		}

	    if(data.Buffer) delete [] data.Buffer;
	}

	// do color matting
	if(doalphacolormat)
	{
		// alpha color mat
		TVPDoAlphaColorMat(data.Dest, alphamatcolor);
	}

	return data.Dest;
}
//---------------------------------------------------------------------------
iTVPTexture2D* TVPLoadPVRv3(tTJSBinaryStream *s, const std::function<void(const ttstr&, const tTJSVariant&)> &cb);
static iTVPTexture2D *TVPInternalLoadTexture(const ttstr &_name,
	std::vector<tTVPGraphicMetaInfoPair> * * MetaInfo, ttstr *provincename) {
	ttstr name(_name), maskname;
	tTVPGraphicHandlerType * handler = TVPFindGraphicLoadHandler(name, &maskname, provincename);
	if (!maskname.IsEmpty()) {
		// mask merge is not supported
		return nullptr;
	}
	tTVPStreamHolder holder(name);
	return TVPLoadPVRv3(holder.Get(), [MetaInfo](const ttstr& k, const tTJSVariant& v) {
		if (!*MetaInfo) *MetaInfo = new std::vector<tTVPGraphicMetaInfoPair>;
		(*MetaInfo)->emplace_back(k, v);
	});
}

void TVPLoadGraphicProvince(tTVPBaseBitmap *dest, const ttstr &name, tjs_int keyidx,
    tjs_uint desw, tjs_uint desh)
{
    tjs_uint32 hash;
    ttstr nname = TVPNormalizeStorageName(name);
    tTVPGraphicsSearchData searchdata;
	if (TVPGraphicCacheEnabled) {
        searchdata.Name = nname;
        searchdata.KeyIdx = keyidx;
        searchdata.Mode = glmPalettized;
        searchdata.DesW = desw;
        searchdata.DesH = desh;

        hash = tTVPGraphicCache::MakeHash(searchdata);

        tTVPGraphicImageHolder * ptr =
            TVPGraphicCache.FindAndTouchWithHash(searchdata, hash);
        if (ptr) {
            // found in cache
            ptr->GetObjectNoAddRef()->AssignToBitmap(dest);
            return;
        }
    }
    // not found

    // load into dest
	tTVPGraphicImageData * data = nullptr;

    ttstr pn;
	std::vector<tTVPGraphicMetaInfoPair> * mi = nullptr;
    try {
        tTVPBitmap *bmp = TVPInternalLoadBitmap(nname, keyidx, desw, desh, &mi, glmPalettized, &pn);
        dest->AssignBitmap(bmp);
        if (TVPGraphicCacheEnabled) {
            data = new tTVPGraphicImageData();
            data->AssignBitmap(bmp);
            data->ProvinceName = pn;
            data->MetaInfo = mi; // now mi is managed under tTVPGraphicImageData
			mi = nullptr;

            // check size limit
            TVPCheckGraphicCacheLimit();

            // push into hash table
            tjs_uint datasize = data->GetSize();
            //			if(datasize < TVPGraphicCacheLimit)
            //			{
            TVPGraphicCacheTotalBytes += datasize;
            tTVPGraphicImageHolder holder(data);
            TVPGraphicCache.AddWithHash(searchdata, hash, holder);
            //			}
        }
        bmp->Release();
    }
    catch (...) {
        if (mi) delete mi;
        if (data) data->Release();
        throw;
    }

    if (mi) delete mi;
    if (data) data->Release();
}

//---------------------------------------------------------------------------
// TVPLoadGraphic (to texture), return size
//---------------------------------------------------------------------------
int TVPLoadGraphic(iTVPBaseBitmap *dest, const ttstr &name, tjs_int32 keyidx,
	tjs_uint desw, tjs_uint desh,
	tTVPGraphicLoadMode mode, ttstr *provincename, iTJSDispatch2 ** metainfo)
{
	// loading with cache management
	ttstr nname = TVPNormalizeStorageName(name);
	tjs_uint32 hash;
	tTVPGraphicsSearchData searchdata;

	if(TVPGraphicCacheEnabled)
	{
		searchdata.Name = nname;
		searchdata.KeyIdx = keyidx;
		searchdata.Mode = mode;
		searchdata.DesW = desw;
		searchdata.DesH = desh;

		hash = tTVPGraphicCache::MakeHash(searchdata);

		tTVPGraphicImageHolder * ptr =
			TVPGraphicCache.FindAndTouchWithHash(searchdata, hash);
		if(ptr)
		{
			// found in cache
			if (dest)
				ptr->GetObjectNoAddRef()->AssignToTexture(dest);
			if(provincename) *provincename = ptr->GetObjectNoAddRef()->ProvinceName;
			if(metainfo)
				*metainfo = TVPMetaInfoPairsToDictionary(ptr->GetObjectNoAddRef()->MetaInfo);
            return ptr->GetObjectNoAddRef()->GetSize();
		}
	}

	// not found
#if defined( WIN32 )&& defined(_DEBUG)
	TVPAddLog(TJS_W("load graphic: ") + nname);
#endif

	// load into dest
	tTVPGraphicImageData * data = NULL;

	ttstr pn;
	std::vector<tTVPGraphicMetaInfoPair> * mi = nullptr;
    int ret = 0;
	try
	{
		tTVPBitmap *bmp = nullptr;
		iTVPTexture2D *texture = nullptr;
		if (mode == glmNormal && keyidx == TVP_clNone && !desw && !desh) {
			texture = TVPInternalLoadTexture(nname, &mi, &pn);
		}
		if (!texture) {
			bmp = TVPInternalLoadBitmap(nname, keyidx, desw, desh, &mi, mode, &pn);
		}

		if(provincename) *provincename = pn;
		if(metainfo)
			*metainfo = TVPMetaInfoPairsToDictionary(mi);

		if(TVPGraphicCacheEnabled)
		{
			data = new tTVPGraphicImageData();
			if (texture) {
				data->AssignTexture(texture);
			} else {
				data->AssignBitmap(bmp);
			}
            if(dest) {
                data->AssignToTexture(dest);
            }
			data->ProvinceName = pn;
			data->MetaInfo = mi; // now mi is managed under tTVPGraphicImageData
			mi = NULL;

			// check size limit
			TVPCheckGraphicCacheLimit();

			// push into hash table
			tjs_uint datasize = data->GetSize();
//			if(datasize < TVPGraphicCacheLimit)
//			{
				TVPGraphicCacheTotalBytes += datasize;
				tTVPGraphicImageHolder holder(data);
				TVPGraphicCache.AddWithHash(searchdata, hash, holder);
//			}
		} else if(dest) {
			tTVPGraphicImageData data;
			if (texture) {
				data.AssignTexture(texture);
			} else {
				data.AssignBitmap(bmp);
			}
			data.AssignToTexture(dest);
		}
		if (texture) {
			ret = texture->GetInternalWidth() * texture->GetInternalHeight() * 4; // assume that always RGBA
			texture->Release();
		} else {
			ret = bmp->GetWidth() * bmp->GetHeight() * bmp->GetBPP() / 8;
			bmp->Release();
		}
	}
	catch(...)
	{
		if(mi) delete mi;
		if(data) data->Release();
		throw;
	}

	if(mi) delete mi;
	if(data) data->Release();

    return ret;
}
//---------------------------------------------------------------------------
#ifdef _MSC_VER
extern "C"
__declspec(dllimport) int __stdcall WideCharToMultiByte(
unsigned int CodePage,
unsigned long dwFlags,
const wchar_t* lpWideCharStr,
int cchWideChar,
char *lpMultiByteStr,
int cbMultiByte,
void* lpDefaultChar,
int* lpUsedDefaultChar
);
#endif
class TVPGraphicPreload {
public:
	struct tImgInfo {
		ttstr filename;
		tTVPGraphicHandlerType *handler;
		tRefHolder<tTJSBinaryStream> Stream;
		tImgInfo() : handler(nullptr) {}
	};

    struct tItem {
		tImgInfo main, mask;
		tTVPGraphicsSearchData searchdata;
		ttstr provincename;
    };

    struct tParam {
        std::vector<tItem> items; // must be TVPNormalizeStorageName(name);
        tjs_int limit;
    };

    static TVPGraphicPreload* Instance() {
        static TVPGraphicPreload *instance;
        if(!instance) instance = new TVPGraphicPreload;
        return instance;
    }

    void addTask(tParam *param) { // called from main thread
		{
			tTJSCSH lock(CSTask);
			ReqInterrupt = true;
			clearTask(); // drop all unfinished tasks
			TaskList.emplace_back(param);
		}
		TaskCond.notify_one();
    }

private:
    std::list<tParam*> TaskList;
    std::condition_variable TaskCond;
	std::mutex TaskMutex;
    tTJSCriticalSection CSTask;
    bool ReqInterrupt;
    
    std::thread *ThreadID;
    TVPGraphicPreload()
    {
//         TaskCond = SDL_CreateCond();
//         TaskMutex = SDL_CreateMutex();
		ThreadID = new std::thread(std::bind(&TVPGraphicPreload::_thread_loop, this));
        ReqInterrupt = false;
    }

    void _thread_loop() {
        while(true) {
            ReqInterrupt = false;
            tParam* CurrentTask = nullptr;
			{
				tTJSCSH lock(CSTask);
				if (!TaskList.empty()) {
					CurrentTask = TaskList.front();
					TaskList.pop_front();
				}
			}
            if(!CurrentTask) {
				std::unique_lock<std::mutex> lk(TaskMutex);
				TaskCond.wait(lk);
                continue;
            }

            unsigned int totalSize = 0;
            for(auto it = CurrentTask->items.begin(); it != CurrentTask->items.end(); ++it) {
                if(ReqInterrupt) break;
                totalSize += loadOneGraph(*it);
                if(totalSize >= CurrentTask->limit) break;
            }
            delete CurrentTask;
        }
    }

//     static int _thread_entry(void *arg) {
//         static_cast<TVPGraphicPreload*>(arg)->_thread_loop();
//         return 0;
//     }

    void clearTask() {
        for(auto it = TaskList.begin(); it != TaskList.end(); ++it) {
            delete *it;
        }
        TaskList.clear();
    }

    unsigned int loadOneGraph(const tItem &item)
    {
		// TODO move cache operation to main thread
		tjs_uint32 hash = tTVPGraphicCache::MakeHash(item.searchdata);
        tTVPGraphicImageHolder * ptr =
			TVPGraphicCache.FindAndTouchWithHash(item.searchdata, hash);
        if (ptr) {
            return ptr->GetObjectNoAddRef()->GetSize(); // already in cache
        }
#ifdef WIN32
		char buf[16384] = { 0 };
		ttstr msg = TJS_W("Touching Image: ") + item.main.filename;
		WideCharToMultiByte(/*CP_ACP*/0, 0, msg.c_str(), -1, buf, sizeof(buf), nullptr, 0);
		puts(buf);
		//TVPAddLog(TJS_W("Touching Image: ") + item.main.filename);
#endif

        // load into dest
		tTVPGraphicImageData * data = nullptr;

		std::vector<tTVPGraphicMetaInfoPair> * mi = nullptr;
        int ret = 0;
        try
        {
            tTVPBitmap *bmp;
            // from TVPInternalLoadGraphic
            {
                //tTVPStreamHolder holder(item.imgfile); // open a storage named "name"

                // load the image
                tTVPLoadGraphicData data;
				data.Dest = nullptr;
                data.ColorKey = TVP_clNone;
                data.Type = lgtFullColor;
				data.Name = item.main.filename;
                data.DesW = 0;
                data.DesH = 0;
                data.NeedMetaInfo = true;
				data.MetaInfo = nullptr;

				(item.main.handler->Load)(item.main.handler->FormatData, (void*)&data, TVPLoadGraphic_SizeCallback,
                    TVPLoadGraphic_ScanLineCallback, TVPLoadGraphic_MetaInfoPushCallback,
					item.main.Stream.get(), -1, glmNormal);

                mi = data.MetaInfo;

                if(item.mask.handler && item.mask.Stream) {
                    // open the mask file
                    //holder.Open(item.maskfile);

                    // fill "data"'s member
                    data.Type = lgtMask;
					data.Name = item.mask.filename;
					data.Buffer = nullptr;
                    data.DesW = 0;
                    data.DesH = 0;
                    data.NeedMetaInfo = false;

					(item.mask.handler->Load)(item.mask.handler->FormatData, (void*)&data,
                        TVPLoadGraphic_SizeCallback, TVPLoadGraphic_ScanLineCallback, nullptr,
						item.mask.Stream.get(), -1, glmGrayscale);
                }

                bmp = data.Dest;
            }

            data = new tTVPGraphicImageData();
            data->AssignBitmap(bmp);
            data->ProvinceName = item.provincename;
            data->MetaInfo = mi; // now mi is managed under tTVPGraphicImageData
			mi = nullptr;

            // check size limit
            TVPCheckGraphicCacheLimit();

            // push into hash table
            tjs_uint datasize = data->GetSize();
			TVPGraphicCacheTotalBytes += datasize;
			tTVPGraphicImageHolder holder(data);
			TVPGraphicCache.AddWithHash(item.searchdata, hash, holder);
            ret = bmp->GetWidth() * bmp->GetHeight() * bmp->GetBPP() / 8;
            bmp->Release();
        }
        catch(...)
        {
            if(mi) delete mi;
            if(data) data->Release();
            throw;
        }

        if(mi) delete mi;
        if(data) data->Release();

        return ret;
    }
};

class tBitmapForAsyncTouch : public tTJSNI_Bitmap {
	typedef tTJSNI_Bitmap inherit;
public:
	tBitmapForAsyncTouch() {
		Construct(0, nullptr, nullptr);
	}
	virtual void SetLoading(bool load) override {
		inherit::SetLoading(load);
		if (!load) {
			::Application->PostUserMessage([this](){
				Invalidate();
				Destruct();
			});
		}
	}
};


//---------------------------------------------------------------------------
// TVPTouchImages
//---------------------------------------------------------------------------
void TVPTouchImages(const std::vector<ttstr> & storages, tjs_int64 limit,
	tjs_uint64 timeout)
{
	// preload graphic files into the cache.
	// "limit" is a limit memory for preload, in bytes.
	// this function gives up when "timeout" (in ms) expired.
	// currently this function only loads normal graphics.
	// (univ.trans rule graphics nor province image may not work properly)

	if(!TVPGraphicCacheLimit || !TVPGraphicCacheEnabled) return;

	tjs_uint64 limitbytes;
	if(limit >= 0)
	{
		if( (tjs_uint64)limit > TVPGraphicCacheLimit || limit == 0)
			limitbytes = TVPGraphicCacheLimit;
		else
			limitbytes = limit;
	}
	else
	{
		// negative value of limit indicates remaining bytes after loading
		if((tjs_uint64)-limit >= TVPGraphicCacheLimit) return;
		limitbytes = TVPGraphicCacheLimit + limit;
	}
	if (/*!timeout &&*/ storages.size()/* > 1*/) { // using async touching for multi images
		for (const ttstr &name : storages) {
			ttstr nname = TVPNormalizeStorageName(name);
			tjs_uint32 hash;
			tTVPGraphicsSearchData searchdata;
			if (TVPGraphicCacheEnabled) {
				searchdata.Name = nname;
				searchdata.KeyIdx = TVP_clNone;
				searchdata.Mode = glmNormal;
				searchdata.DesW = 0;
				searchdata.DesH = 0;

				hash = tTVPGraphicCache::MakeHash(searchdata);

				tTVPGraphicImageHolder * ptr =
					TVPGraphicCache.FindAndTouchWithHash(searchdata, hash);
				if (ptr) {
					// found in cache
					continue;
				}
			}
			Application->GetAsyncImageLoader()->PushLoadQueue(nullptr, new tBitmapForAsyncTouch(), nname);
		}
		return;
	}
#if 0
    // async version
    TVPGraphicPreload::tParam *param = new TVPGraphicPreload::tParam;
    param->limit = limitbytes;
    for(auto it = storages.begin(); it != storages.end(); ++it) {
        TVPGraphicPreload::tItem item;
		item.main.handler = nullptr;
		ttstr name = TVPNormalizeStorageName(*it);
		tTVPGraphicsSearchData &searchdata = item.searchdata;
		searchdata.Name = name;
		searchdata.KeyIdx = TVP_clNone;
		searchdata.Mode = glmNormal;
		searchdata.DesW = 0;
		searchdata.DesH = 0;
		ttstr ext = TVPExtractStorageExt(name);
        if(ext.IsEmpty()) { // to avoid handler searching
            tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
            for(i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); /*i++*/)
            {
                ttstr newname = name + i.GetKey();
                if(TVPIsExistentStorage(newname))
                {
                    // file found
					item.main.filename = newname;
					item.main.handler = &i.GetValue();
                    break;
                }
                i++;
            }
			if (i.IsNull() || !item.main.handler)
            {
                // not found
                //TVPThrowExceptionMessage(TVPCannotSuggestGraphicExtension, name);
				continue;
            }
        } else {
			item.main.filename = name;
			item.main.handler = TVPGraphicType.Hash.Find(ext);
			if (!item.main.handler) continue;
		}

        ttstr provincename = ttstr(name, name.length()-ext.length()) + TJS_W("_p");
        // search extensions
        tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
        for(i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); /*i++*/)
        {
            ttstr newname = provincename + i.GetKey();
            if(TVPIsExistentStorage(newname))
            {
                // file found
                item.provincename = newname;
                break;
            }
            i++;
        }
        // mask image handling ( addding _m suffix with the filename )
        while(true)
        {
            name = ttstr(name, name.length()-ext.length()) + TJS_W("_m") + ext;
            if(ext.IsEmpty())
            {
                // missing extension
                // suggest registered extensions
                tTJSHashTable<ttstr, tTVPGraphicHandlerType>::tIterator i;
                for(i = TVPGraphicType.Hash.GetFirst(); !i.IsNull(); /*i++*/)
                {
                    ttstr newname = name;
                    newname += i.GetKey();
                    if(TVPIsExistentStorage(newname))
                    {
                        // file found
                        item.mask.filename = newname;
						item.mask.handler = &i.GetValue();
                        break;
                    }
                    i++;
                }
                break;
            }
            else
            {
                if(!TVPIsExistentStorage(name))
                {
                    // not found
                    ext.Clear();
                    continue; // retry searching
                }
				item.mask.handler = TVPGraphicType.Hash.Find(ext);
                break;
            }
        }

		item.main.Stream = TVPCreateStream(item.main.filename, 0);
		if (item.main.Stream) {
			if (item.mask.handler) item.mask.Stream = TVPCreateStream(item.mask.filename, 0);
			param->items.emplace_back(item);
		}
    }
    TVPGraphicPreload::Instance()->addTask(param);
    return;
#endif
	tjs_int count = 0;
	tjs_uint64 bytes = 0;
	tjs_uint64 starttime = TVPGetTickCount();
	tjs_uint64 limittime = starttime + timeout;
	//tTVPBaseBitmap tmp(32, 32, 32);
 	ttstr statusstr( (const tjs_char*)TVPInfoTouching );
	bool first = true;
	while((tjs_uint)count < storages.size())
	{
		if(timeout && TVPGetTickCount() >= limittime)
		{
			statusstr += (const tjs_char*)TVPAbortedTimeOut;
			break;
		}
		if(bytes >= limitbytes)
		{
			statusstr += (const tjs_char*)TVPAbortedLimitByte;
			break;
		}

		try
		{
			if(!first) statusstr += TJS_W(", ");
			first = false;
			statusstr += storages[count];

			bytes += TVPLoadGraphic(nullptr, storages[count++], TVP_clNone,
				0, 0, glmNormal, NULL); // load image
		}
		catch(eTJS &e)
		{
			statusstr += TJS_W("(error!:");
			statusstr += e.GetMessage();
			statusstr += TJS_W(")");
		}
		catch(...)
		{
			// ignore all errors
		}
	}

	// re-touch graphic cache to ensure that more earlier graphics in storages
	// array can get more priority in cache order.
	count--;
	for(;count >= 0; count--)
	{
		tTVPGraphicsSearchData searchdata;
		searchdata.Name = TVPNormalizeStorageName(storages[count]);
		searchdata.KeyIdx = TVP_clNone;
		searchdata.Mode = glmNormal;
		searchdata.DesW = 0;
		searchdata.DesH = 0;

		tjs_uint32 hash = tTVPGraphicCache::MakeHash(searchdata);

		TVPGraphicCache.FindAndTouchWithHash(searchdata, hash);
	}

	statusstr += TJS_W(" (elapsed ");
	statusstr += ttstr((tjs_int)(TVPGetTickCount() - starttime));
	statusstr += TJS_W("ms)");

	TVPAddLog(statusstr);
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// TVPSetGraphicCacheLimit
//---------------------------------------------------------------------------
void TVPSetGraphicCacheLimit(tjs_uint64 limit)
{
	// set limit of graphic cache by total bytes.
	if(limit == 0 )
	{
		TVPGraphicCacheLimit = limit;
		TVPGraphicCacheEnabled = false;
	}
	else if(limit == -1)
	{
		TVPGraphicCacheLimit = TVPGraphicCacheSystemLimit;
		TVPGraphicCacheEnabled = true;
	}
	else
	{
		if(limit > TVPGraphicCacheSystemLimit)
			limit = TVPGraphicCacheSystemLimit;
		TVPGraphicCacheLimit = limit;
		TVPGraphicCacheEnabled = true;
	}

	if (TVPGraphicCacheLimit > 512 * 1024 * 1024) // max for 512M
		TVPGraphicCacheLimit = 512 * 1024 * 1024;

	TVPCheckGraphicCacheLimit();
}
//---------------------------------------------------------------------------
tjs_uint64 TVPGetGraphicCacheLimit()
{
	return TVPGraphicCacheLimit;
}
//---------------------------------------------------------------------------




