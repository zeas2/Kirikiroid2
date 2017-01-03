
#include "tjsCommHead.h"

#include "GraphicsLoaderIntf.h"
#include "LayerBitmapIntf.h"
#include "StorageIntf.h"
#include "MsgIntf.h"
#include "tvpgl.h"

#include "png.h"
//#include "pngstruct.h"
//#include "pnginfo.h"

#include "DebugIntf.h"
#include "tjsDictionary.h"
#include "ScriptMgnIntf.h"

bool TVPAcceptSaveAsPNG(void* formatdata, const ttstr & type, class iTJSDispatch2** dic )
{
	bool result = false;
	if( type.StartsWith(TJS_W("png")) ) result = true;
	else if( type == TJS_W(".png") ) result = true;

	if( result && dic ) {
#if 0
		tTJSVariant result;
		TVPExecuteExpression(
			TJS_W("(const)%["),
			TJS_W("\"offs_x\"=>(const)%[\"type\"=>\"integer\",\"desc\"=>\"offset x\",\"default\"=>0]")
			TJS_W("\"offs_y\"=>(const)%[\"type\"=>\"integer\",\"desc\"=>\"offset y\",\"default\"=>0]")
			TJS_W("\"offs_unit\"=>(const)%[\"type\"=>\"select text\",\"items\"=>[\"pixel\",\"micrometer\"],\"desc\"=>\"offset unit\",\"default\"=>\"pixel\"]")
			TJS_W("]"),
			NULL, &result );
		if( result.Type() == tvtObject ) {
			*dic = result.AsObject();
		}
#endif
		// タグ書き出し等には対応していない
		*dic = TJSCreateDictionaryObject();
	}
	return result;
}
//---------------------------------------------------------------------------
// PNG loading handler
//---------------------------------------------------------------------------
static ttstr PNG_tag_offs_x(TJS_W("offs_x"));
static ttstr PNG_tag_offs_y(TJS_W("offs_y"));
static ttstr PNG_tag_offs_unit(TJS_W("offs_unit"));
static ttstr PNG_tag_reso_x(TJS_W("reso_x"));
static ttstr PNG_tag_reso_y(TJS_W("reso_y"));
static ttstr PNG_tag_reso_unit(TJS_W("reso_unit"));
static ttstr PNG_tag_vpag_w(TJS_W("vpag_w"));
static ttstr PNG_tag_vpag_h(TJS_W("vpag_h"));
static ttstr PNG_tag_vpag_unit(TJS_W("vpag_unit"));
static ttstr PNG_tag_pixel(TJS_W("pixel"));
static ttstr PNG_tag_micrometer(TJS_W("micrometer"));
static ttstr PNG_tag_meter(TJS_W("meter"));
static ttstr PNG_tag_unknown(TJS_W("unknown"));
//---------------------------------------------------------------------------
// meta callback information structure used by  PNG_read_chunk_callback
struct PNG_read_chunk_callback_user_struct
{
	void * callbackdata;
    tTVPMetaInfoPushCallback metainfopushcallback;
};
//---------------------------------------------------------------------------
// user_malloc_fn
static png_voidp PNG_malloc(png_structp ps, png_size_t size)
{
	return malloc(size);
}
//---------------------------------------------------------------------------
// user_free_fn
static void PNG_free (png_structp ps,void* /* png_structp*/ mem)
{
	free(mem);
}
//---------------------------------------------------------------------------
// user_error_fn
static void PNG_error (png_structp ps, png_const_charp msg)
{
	TVPThrowExceptionMessage(TVPPNGLoadError, msg);
}
//---------------------------------------------------------------------------
// user_warning_fn
static void PNG_warning (png_structp ps, png_const_charp msg)
{
	// do nothing
	//TVPAddLog( TJS_W("PNG warning:") );
	//TVPAddLog( msg );
}
//---------------------------------------------------------------------------
// user_read_data
static void PNG_read_data(png_structp png_ptr,png_bytep data,png_size_t length)
{
	((tTJSBinaryStream *)png_get_io_ptr(png_ptr))->ReadBuffer((void*)data, (tjs_uint)length);
}
//---------------------------------------------------------------------------
// read_row_callback
static void PNG_read_row_callback(png_structp png_ptr,png_uint_32 row,int pass)
{

}
//---------------------------------------------------------------------------
// read_chunk_callback
static int PNG_read_chunk_callback(png_structp png_ptr,png_unknown_chunkp chunk)
{
	// handle vpAg chunk (this will contain the virtual page size of the image)
	// vpAg chunk can be embeded by ImageMagick -trim option etc.
	// we don't care about how the chunk bit properties are being provided.
	if(	(chunk->name[0] == 0x76/*'v'*/ || chunk->name[0] == 0x56/*'V'*/) &&
		(chunk->name[1] == 0x70/*'p'*/ || chunk->name[1] == 0x50/*'P'*/) &&
		(chunk->name[2] == 0x61/*'a'*/ || chunk->name[2] == 0x41/*'A'*/) &&
		(chunk->name[3] == 0x67/*'g'*/ || chunk->name[3] == 0x47/*'G'*/) && chunk->size >= 9)
	{
		PNG_read_chunk_callback_user_struct * user_struct =
			reinterpret_cast<PNG_read_chunk_callback_user_struct *>(png_get_user_chunk_ptr(png_ptr));
		// vpAg found
		/*
			uint32 width
			uint32 height
			uchar unit
		*/
		// be careful because the integers are stored in network byte order
		#define PNG_read_be32(a) (((tjs_uint32)(a)[0]<<24)+\
			((tjs_uint32)(a)[1]<<16)+((tjs_uint32)(a)[2]<<8)+\
			((tjs_uint32)(a)[3]))
		tjs_uint32 width  = PNG_read_be32(chunk->data+0);
		tjs_uint32 height = PNG_read_be32(chunk->data+4);
		tjs_uint8  unit   = chunk->data[8];

		// push information into meta-info
		user_struct->metainfopushcallback(user_struct->callbackdata, PNG_tag_vpag_w, ttstr((tjs_int)width));
		user_struct->metainfopushcallback(user_struct->callbackdata, PNG_tag_vpag_h, ttstr((tjs_int)height));
		switch(unit)
		{
		case PNG_OFFSET_PIXEL:
			user_struct->metainfopushcallback(user_struct->callbackdata, PNG_tag_vpag_unit, PNG_tag_pixel);
			break;
		case PNG_OFFSET_MICROMETER:
			user_struct->metainfopushcallback(user_struct->callbackdata, PNG_tag_vpag_unit, PNG_tag_micrometer);
			break;
		default:
			user_struct->metainfopushcallback(user_struct->callbackdata, PNG_tag_vpag_unit, PNG_tag_unknown);
			break;
		}
		return 1; // chunk read success
	}
	return 0; // did not recognize
}
//---------------------------------------------------------------------------
void TVPLoadPNG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode)
{
	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL;
	png_infop end_info=NULL;

	png_uint_32 i;

	png_bytep *row_pointers=NULL;
	tjs_uint8 *image=NULL;


	try
	{
		// create png_struct
		png_ptr=png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
			(png_voidp)NULL, (png_error_ptr)PNG_error, (png_error_ptr)PNG_warning,
			(png_voidp)NULL, (png_malloc_ptr)PNG_malloc, (png_free_ptr)PNG_free);
		//png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, (png_voidp)NULL, (png_error_ptr)PNG_error, (png_error_ptr)PNG_warning );
		if( !png_ptr ) TVPThrowExceptionMessage(TVPPNGLoadError, (const tjs_char*)TVPLibpngError );

		// set read_chunk_callback
		PNG_read_chunk_callback_user_struct read_chunk_callback_user_struct;
		read_chunk_callback_user_struct.callbackdata = callbackdata;
		read_chunk_callback_user_struct.metainfopushcallback = metainfopushcallback;
		png_set_read_user_chunk_fn(png_ptr,
			reinterpret_cast<void*>(&read_chunk_callback_user_struct),
			(png_user_chunk_ptr)PNG_read_chunk_callback);
		png_set_keep_unknown_chunks(png_ptr, 2, NULL, 0);
			// keep only if safe-to-copy chunks, for all unknown chunks

		// create png_info
		info_ptr=png_create_info_struct(png_ptr);
		if( !info_ptr ) TVPThrowExceptionMessage(TVPPNGLoadError, (const tjs_char*)TVPLibpngError );

		// create end_info
		end_info=png_create_info_struct(png_ptr);
		if( !end_info ) TVPThrowExceptionMessage(TVPPNGLoadError, (const tjs_char*)TVPLibpngError );

		// set stream interface
		png_set_read_fn(png_ptr,(png_voidp)src, (png_rw_ptr)PNG_read_data);

		// set read_row_callback
		png_set_read_status_fn(png_ptr, (png_read_status_ptr)PNG_read_row_callback);

		// set png_read_info
		png_read_info(png_ptr,info_ptr);

		// retrieve IHDR
		png_uint_32 width,height;
		int bit_depth,color_type,interlace_type,compression_type,filter_type;
		png_get_IHDR(png_ptr,info_ptr,&width,&height,&bit_depth,&color_type,
			&interlace_type,&compression_type,&filter_type);

		if(bit_depth==16) png_set_strip_16(png_ptr);

		// retrieve offset information
		png_int_32 offset_x, offset_y;
		int offset_unit_type;
		if(metainfopushcallback &&
			png_get_oFFs(png_ptr, info_ptr, &offset_x, &offset_y, &offset_unit_type))
		{
			// push offset information into metainfo data
			metainfopushcallback(callbackdata, PNG_tag_offs_x, ttstr((tjs_int)offset_x));
			metainfopushcallback(callbackdata, PNG_tag_offs_y, ttstr((tjs_int)offset_y));
			switch(offset_unit_type)
			{
			case PNG_OFFSET_PIXEL:
				metainfopushcallback(callbackdata, PNG_tag_offs_unit, PNG_tag_pixel);
				break;
			case PNG_OFFSET_MICROMETER:
				metainfopushcallback(callbackdata, PNG_tag_offs_unit, PNG_tag_micrometer);
				break;
			default:
				metainfopushcallback(callbackdata, PNG_tag_offs_unit, PNG_tag_unknown);
				break;
			}
		}
		
		png_uint_32 reso_x, reso_y;
		int reso_unit_type;
		if(metainfopushcallback &&
			png_get_pHYs(png_ptr, info_ptr, &reso_x, &reso_y, &reso_unit_type))
		{
			// push offset information into metainfo data
			metainfopushcallback(callbackdata, PNG_tag_reso_x, ttstr((tjs_int)reso_x));
			metainfopushcallback(callbackdata, PNG_tag_reso_y, ttstr((tjs_int)reso_y));
			switch(reso_unit_type)
			{
			case PNG_RESOLUTION_METER:
				metainfopushcallback(callbackdata, PNG_tag_reso_unit, PNG_tag_meter);
				break;
			default:
				metainfopushcallback(callbackdata, PNG_tag_reso_unit, PNG_tag_unknown);
				break;
			}
		}


		bool do_convert_rgb_gray = false;

		if(mode == glmPalettized)
		{
			// convert the image to palettized one if needed
			if(bit_depth > 8)
				TVPThrowExceptionMessage(
					TVPPNGLoadError, (const tjs_char*)TVPUnsupportedColorTypePalette );

			if(color_type == PNG_COLOR_TYPE_PALETTE)
			{
				png_set_packing(png_ptr);
			}

			if(color_type == PNG_COLOR_TYPE_GRAY) png_set_expand_gray_1_2_4_to_8(png_ptr);
		}
		else if(mode == glmGrayscale)
		{
			// convert the image to grayscale
			if(color_type == PNG_COLOR_TYPE_PALETTE)
			{
				png_set_palette_to_rgb(png_ptr);
			//	png_set_bgr(png_ptr);
				if (bit_depth < 8)
					png_set_packing(png_ptr);
				do_convert_rgb_gray = true; // manual conversion
			}
			if(color_type == PNG_COLOR_TYPE_GRAY &&
				bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
			if(color_type == PNG_COLOR_TYPE_RGB ||
				color_type == PNG_COLOR_TYPE_RGB_ALPHA)
				png_set_rgb_to_gray_fixed(png_ptr, 1, 0, 0);
			if(color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
				color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
				png_set_strip_alpha(png_ptr);
		}
		else
		{
			// glmNormal
			// convert the image to full color ( 32bits ) one if needed

			if(color_type == PNG_COLOR_TYPE_PALETTE)
			{
				if(keyidx == -1)
				{
					if(png_get_valid(png_ptr, info_ptr,PNG_INFO_tRNS))
					{
						// set expansion with palettized picture
						png_set_palette_to_rgb(png_ptr);
						png_set_tRNS_to_alpha(png_ptr);
						color_type=PNG_COLOR_TYPE_RGB_ALPHA;
					}
					else
					{
						png_set_palette_to_rgb(png_ptr);
						color_type=PNG_COLOR_TYPE_RGB;
					}
				}
				else
				{
					png_byte trans = (png_byte) keyidx;
					png_set_tRNS(png_ptr, info_ptr, &trans, 1, 0);
						// make keyidx transparent color.
					png_set_palette_to_rgb(png_ptr);
					png_set_tRNS_to_alpha(png_ptr);
					color_type=PNG_COLOR_TYPE_RGB_ALPHA;

				}
			}

			switch(color_type)
			{
			case PNG_COLOR_TYPE_GRAY_ALPHA:
				png_set_gray_to_rgb(png_ptr);
				color_type=PNG_COLOR_TYPE_RGB_ALPHA;
				png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
				break;
			case PNG_COLOR_TYPE_GRAY:
				png_set_expand(png_ptr);
				png_set_gray_to_rgb(png_ptr);
				color_type=PNG_COLOR_TYPE_RGB;
				png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
				break;
			case PNG_COLOR_TYPE_RGB_ALPHA:
			//	png_set_bgr(png_ptr);
				break;
			case PNG_COLOR_TYPE_RGB:
			//	png_set_bgr(png_ptr);
				png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
				break;
			default:
				TVPThrowExceptionMessage(
 					TVPPNGLoadError, (const tjs_char*)TVPUnsupportedColorType );
			}
		}


		// size checking
		if(width>=65536 || height>=65536)
		{
			// too large image to handle
			TVPThrowExceptionMessage(
				TVPPNGLoadError, (const tjs_char*)TVPTooLargeImage );
		}


		// call png_read_update_info
		png_read_update_info(png_ptr,info_ptr);

		// set size
		sizecallback(callbackdata, width, height, color_type == PNG_COLOR_TYPE_RGB_ALPHA ? gpfRGBA : gpfRGB);

		// load image
		if (png_get_interlace_type(png_ptr,info_ptr) == PNG_INTERLACE_NONE)
		{
			// non-interlace
			if(do_convert_rgb_gray)
			{
				png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
				image = new tjs_uint8[rowbytes];
			}
#if 1
			if( !do_convert_rgb_gray ) {
				for(i=0; i<height; i++) {
					void *scanline = scanlinecallback(callbackdata, i);
					if(!scanline) break;
					png_read_row(png_ptr, (png_bytep)scanline, NULL);
					scanlinecallback(callbackdata, -1);
				}
			} else {
				for(i=0; i<height; i++) {
					void *scanline = scanlinecallback(callbackdata, i);
					if(!scanline) break;
					png_read_row(png_ptr, (png_bytep)image, NULL);
					TVPBLConvert24BitTo8Bit(
						(tjs_uint8*)scanline,
						(tjs_uint8*)image, width);
					scanlinecallback(callbackdata, -1);
				}
			}
#else
			for(i=0; i<height; i++)
			{
				void *scanline = scanlinecallback(callbackdata, i);
				if(!scanline) break;
				if(!do_convert_rgb_gray)
				{
					png_read_row(png_ptr, (png_bytep)scanline, NULL);
				}
				else
				{
					png_read_row(png_ptr, (png_bytep)image, NULL);
					TVPBLConvert24BitTo8Bit(
						(tjs_uint8*)scanline,
						(tjs_uint8*)image, width);
				}
				scanlinecallback(callbackdata, -1);
			}
#endif
			// finish loading
			png_read_end(png_ptr,info_ptr);
		}
		else
		{
			// interlace handling
			// load the image at once

			row_pointers = new png_bytep[height];
			png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
			image = new tjs_uint8[rowbytes * height];
			for(i=0; i<height; i++)
			{
				row_pointers[i] = image+ i*rowbytes;
			}

			// loads image
			png_read_image(png_ptr, row_pointers);

			// finish loading
			png_read_end(png_ptr, info_ptr);

			// set the pixel data
			for(i=0; i<height; i++)
			{
				void *scanline = scanlinecallback(callbackdata, i);
				if(!scanline) break;
				if(!do_convert_rgb_gray)
				{
					memcpy(scanline, row_pointers[i], rowbytes);
				}
				else
				{
					TVPBLConvert24BitTo8Bit(
						(tjs_uint8*)scanline,
						(tjs_uint8*)row_pointers[i], width);
				}
				scanlinecallback(callbackdata, -1);
			}
		}
	}
	catch(...)
	{
		png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
		if(row_pointers) delete [] row_pointers;
		if(image) delete [] image;
		throw;
	}

	png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
	if(row_pointers) delete [] row_pointers;
	if(image) delete [] image;

}
//---------------------------------------------------------------------------
/**
 * PNG書き込みフラッシュ
 * @param png_ptr : PNG情報
 */
static void PNG_write_flash( png_structp png_ptr )
{
	// 何もしない
}
//---------------------------------------------------------------------------
/**
 * PNG書き込み
 * @param png_ptr : PNG情報
 * @param buf : 書き込みデータ
 * @param size : データサイズ
 */
static void PNG_write_write( png_structp png_ptr, png_bytep buf, png_size_t size )
{
    tTJSBinaryStream* stream=(tTJSBinaryStream*)png_get_io_ptr(png_ptr);
    stream->WriteBuffer(buf,(tjs_uint)size);
}
//---------------------------------------------------------------------------
/**
 * PNG書き込み
 * フルカラーでの書き込みのみ対応
 * @param dst : 出力先
 * @param image : 書き出しイメージデータ
 * @param mode : モード (現在はpngのみ可)
 * @param meta : 追加オプション (現在は無視)
 */
void TVPSaveAsPNG(void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* image, const ttstr & mode, iTJSDispatch2* meta )
{
	int bpp = 32;
	if( mode.StartsWith(TJS_W("png")) && mode.length() > 3 ) {
		if( mode == TJS_W("png24") ) {
			bpp = 24;
		}
	}

	tjs_uint height = image->GetHeight();
	tjs_uint width = image->GetWidth();
	if( height == 0 || width == 0 ) TVPThrowInternalError;

	// open stream
	tTJSBinaryStream *stream = dst;

	try {
		TVPClearGraphicCache();

		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
		if( !png_ptr ) {
			TVPThrowExceptionMessage( TVPPngSaveError );
		}
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if( !info_ptr ) {
			png_destroy_write_struct(&png_ptr,NULL);
			TVPThrowExceptionMessage( TVPPngSaveError );
		}

		png_set_write_fn( png_ptr, (png_voidp)stream, (png_rw_ptr)PNG_write_write, (png_flush_ptr)PNG_write_flash );

		png_set_IHDR( png_ptr, info_ptr, width, height, 8, (bpp == 32 ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB), PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );
	
		png_color_8 sig_bit;
		sig_bit.red = 8;
		sig_bit.green = 8;
		sig_bit.blue = 8;
		sig_bit.gray = 0;
		sig_bit.alpha = bpp == 32 ? 8 : 0;
		png_set_sBIT( png_ptr, info_ptr, &sig_bit );

		/* ----- インフォメーションヘッダー書出し */
		png_write_info( png_ptr, info_ptr );
	//	png_set_bgr( png_ptr );

		/* ----- ピクセル書出し */
		tjs_uint width_byte = (bpp*width)/8;
		tjs_uint8* buff = new tjs_uint8[width_byte];
		if( bpp == 32 ) {
			for( tjs_uint32 y = 0; y < height; y++ ) {
				memcpy( buff, image->GetScanLine(y), width_byte );
				png_write_row( png_ptr, (png_bytep)buff );
			}
		} else {
			for( tjs_uint32 y = 0; y < height; y++ ) {
				const tjs_uint8* src = reinterpret_cast<const tjs_uint8*>(image->GetScanLine(y));
				tjs_uint8* dst = buff;
				for( tjs_uint32 x = 0; x < width; x++ ) {
					*dst = *src; dst++; src++;
					*dst = *src; dst++; src++;
					*dst = *src; dst++; src++;
					src++;
				}
				png_write_row( png_ptr, (png_bytep)buff );
			}
		}
		/* ----- 書き出しの終了、後始末 */
		png_write_end( png_ptr, info_ptr );
		png_destroy_write_struct( &png_ptr, &info_ptr );
		delete []buff;
	} catch(...) {
		throw;
	}
}

void TVPLoadHeaderPNG(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic )
{
	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL;
	png_infop end_info=NULL;
	try
	{
		// create png_struct
		png_ptr=png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
			(png_voidp)NULL, (png_error_ptr)PNG_error, (png_error_ptr)PNG_warning,
			(png_voidp)NULL, (png_malloc_ptr)PNG_malloc, (png_free_ptr)PNG_free);

		if( !png_ptr ) TVPThrowExceptionMessage(TVPPNGLoadError, (const tjs_char*)TVPLibpngError );

		// create png_info
		info_ptr=png_create_info_struct(png_ptr);
		if( !info_ptr ) TVPThrowExceptionMessage(TVPPNGLoadError, (const tjs_char*)TVPLibpngError );

		// create end_info
		end_info=png_create_info_struct(png_ptr);
		if( !end_info ) TVPThrowExceptionMessage(TVPPNGLoadError, (const tjs_char*)TVPLibpngError );

		// set stream interface
		png_set_read_fn(png_ptr,(png_voidp)src, (png_rw_ptr)PNG_read_data);

		// set read_row_callback
		png_set_read_status_fn(png_ptr, (png_read_status_ptr)PNG_read_row_callback);

		// set png_read_info
		png_read_info(png_ptr,info_ptr);

		// retrieve IHDR
		png_uint_32 width,height;
		int bit_depth,color_type,interlace_type,compression_type,filter_type;
		png_get_IHDR(png_ptr,info_ptr,&width,&height,&bit_depth,&color_type,
			&interlace_type,&compression_type,&filter_type);
		switch(color_type) {
		case PNG_COLOR_TYPE_GRAY:	bit_depth *= 1; break;
		case PNG_COLOR_TYPE_PALETTE:bit_depth *= 1; break;
		case PNG_COLOR_TYPE_RGB:	bit_depth *= 3; break;
		case PNG_COLOR_TYPE_RGBA:	bit_depth *= 4; break;
		case PNG_COLOR_TYPE_GA:		bit_depth *= 2; break;
		}

		*dic = TJSCreateDictionaryObject();
		tTJSVariant val((tjs_int64)width);
		(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("width"), 0, &val, (*dic) );
		val = tTJSVariant((tjs_int64)height);
		(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("height"), 0, &val, (*dic) );
		val = tTJSVariant(bit_depth);
		(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("bpp"), 0, &val, (*dic) );
		// color_type,interlace_type,compression_type,filter_type も入れた方がいいが……

		// retrieve offset information
		png_int_32 offset_x, offset_y;
		int offset_unit_type;
		if( png_get_oFFs(png_ptr, info_ptr, &offset_x, &offset_y, &offset_unit_type) )
		{
			val = tTJSVariant(offset_x);
			(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("offset x"), 0, &val, (*dic) );
			val = tTJSVariant(offset_y);
			(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("offset y"), 0, &val, (*dic) );
			switch(offset_unit_type)
			{
			case PNG_OFFSET_PIXEL:
				val = tTJSVariant(TJS_W("pixel"));
				break;
			case PNG_OFFSET_MICROMETER:
				val = tTJSVariant(TJS_W("micro meter"));
				break;
			default:
				val = tTJSVariant(TJS_W("unknown"));
				break;
			}
			(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("offset unit"), 0, &val, (*dic) );
		}

		png_uint_32 reso_x, reso_y;
		int reso_unit_type;
		if( png_get_pHYs(png_ptr, info_ptr, &reso_x, &reso_y, &reso_unit_type) )
		{
			val = tTJSVariant((tjs_int64)reso_x);
			(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("resolution x"), 0, &val, (*dic) );
			val = tTJSVariant((tjs_int64)reso_y);
			(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("resolution y"), 0, &val, (*dic) );
			switch(reso_unit_type)
			{
			case PNG_RESOLUTION_METER:
				val = tTJSVariant(TJS_W("meter"));
				break;
			default:
				val = tTJSVariant(TJS_W("unknown"));
				break;
			}
			(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("resolution unit"), 0, &val, (*dic) );
		}
	}
	catch(...)
	{
		png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
		if( *dic ) (*dic)->Release();
		throw;
	}
	png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
}

