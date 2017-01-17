//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TLG5/6 decoder
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "GraphicsLoaderIntf.h"
#include "MsgIntf.h"
#include "tjsUtils.h"
#include "tvpgl.h"
#include "tjsDictionary.h"

#include <stdlib.h>

/*
	TLG5:
		Lossless graphics compression method designed for very fast decoding
		speed.

	TLG6:
		Lossless/near-lossless graphics compression method which is designed
		for high compression ratio and faster decoding. Decoding speed is
		somewhat slower than TLG5 because the algorithm is much more complex
		than TLG5. Though, the decoding speed (using SSE enabled code) is
		about 20 times faster than JPEG2000 lossless mode (using JasPer
		library) while the compression ratio can beat or compete with it.
		Summary of compression algorithm is described in
		environ/win32/krdevui/tpc/tlg6/TLG6Saver.cpp
		(in Japanese).
*/



//---------------------------------------------------------------------------
// TLG5 loading handler
//---------------------------------------------------------------------------
void TVPLoadTLG5(void* formatdata, void *callbackdata,
	tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback,
	tTJSBinaryStream *src,
	tjs_int keyidx,
	tTVPGraphicLoadMode mode)
{
	// load TLG v5.0 lossless compressed graphic
	if(mode != glmNormal)
		TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPTlgUnsupportedUniversalTransitionRule );

	unsigned char mark[12];
	tjs_int width, height, colors, blockheight;
	src->ReadBuffer(mark, 1);
	colors = mark[0];
	width = src->ReadI32LE();
	height = src->ReadI32LE();
	blockheight = src->ReadI32LE();

	if(colors != 3 && colors != 4)
		TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPUnsupportedColorType );

	int blockcount = (int)((height - 1) / blockheight) + 1;

	// skip block size section
	src->SetPosition(src->GetPosition() + blockcount * sizeof(tjs_uint32));


	// decomperss
	sizecallback(callbackdata, width, height, colors == 3 ? gpfRGB : gpfRGBA);

	tjs_uint8 *inbuf = NULL;
	tjs_uint8 *outbuf[4];
	tjs_uint8 *text = NULL;
	tjs_int r = 0;
	for(int i = 0; i < colors; i++) outbuf[i] = NULL;

	try
	{
		text = (tjs_uint8*)TJSAlignedAlloc(4096+32, 4) + 16;
		memset(text, 0, 4096);

		inbuf = (tjs_uint8*)TJSAlignedAlloc(blockheight * width + 10+16, 4);
		for(tjs_int i = 0; i < colors; i++)
			outbuf[i] = (tjs_uint8*)TJSAlignedAlloc(blockheight * width + 10+16, 4);

		tjs_uint8 *prevline = NULL;
		for(tjs_int y_blk = 0; y_blk < height; y_blk += blockheight)
		{
			// read file and decompress
			for(tjs_int c = 0; c < colors; c++)
			{
				src->ReadBuffer(mark, 1);
				tjs_int size;
				size = src->ReadI32LE();
				if(mark[0] == 0)
				{
					// modified LZSS compressed data
					src->ReadBuffer(inbuf, size);
					r = TVPTLG5DecompressSlide(outbuf[c], inbuf, size, text, r);
				}
				else
				{
					// raw data
					src->ReadBuffer(outbuf[c], size);
				}
			}

			// compose colors and store
			tjs_int y_lim = y_blk + blockheight;
			if(y_lim > height) y_lim = height;
			tjs_uint8 * outbufp[4];
			for(tjs_int c = 0; c < colors; c++) outbufp[c] = outbuf[c];
			for(tjs_int y = y_blk; y < y_lim; y++)
			{
				tjs_uint8 *current = (tjs_uint8*)scanlinecallback(callbackdata, y);
				tjs_uint8 *current_org = current;
				if(prevline)
				{
					// not first line
					switch(colors)
					{
					case 3:
						TVPTLG5ComposeColors3To4(current, prevline, outbufp, width);
						outbufp[0] += width; outbufp[1] += width;
						outbufp[2] += width;
						break;
					case 4:
						TVPTLG5ComposeColors4To4(current, prevline, outbufp, width);
						outbufp[0] += width; outbufp[1] += width;
						outbufp[2] += width; outbufp[3] += width;
						break;
					}
				}
				else
				{
					// first line
					switch(colors)
					{
					case 3:
						for(tjs_int pr = 0, pg = 0, pb = 0, x = 0;
							x < width; x++)
						{
							tjs_int r = outbufp[0][x];
							tjs_int g = outbufp[1][x];
							tjs_int b = outbufp[2][x];
							b += g; r += g;
							0[current++] = pb += b;
							0[current++] = pg += g;
							0[current++] = pr += r;
							0[current++] = 0xff;
						}
						outbufp[0] += width;
						outbufp[1] += width;
						outbufp[2] += width;
						break;
					case 4:
						for(tjs_int pr = 0, pg = 0, pb = 0, pa = 0, x = 0;
							x < width; x++)
						{
							tjs_int r = outbufp[0][x];
							tjs_int g = outbufp[1][x];
							tjs_int b = outbufp[2][x];
							tjs_int a = outbufp[3][x];
							b += g; r += g;
							0[current++] = pb += b;
							0[current++] = pg += g;
							0[current++] = pr += r;
							0[current++] = pa += a;
						}
						outbufp[0] += width;
						outbufp[1] += width;
						outbufp[2] += width;
						outbufp[3] += width;
						break;
					}
				}
				scanlinecallback(callbackdata, -1);

				prevline = current_org;
			}

		}
	}
	catch(...)
	{
		if(inbuf) TJSAlignedDealloc(inbuf);
		if(text) TJSAlignedDealloc(text - 16);
		for(tjs_int i = 0; i < colors; i++)
			if(outbuf[i]) TJSAlignedDealloc(outbuf[i]);
		throw;
	}
	if(inbuf) TJSAlignedDealloc(inbuf);
	if(text) TJSAlignedDealloc(text - 16);
	for(tjs_int i = 0; i < colors; i++)
		if(outbuf[i]) TJSAlignedDealloc(outbuf[i]);
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TLG6 loading handler
//---------------------------------------------------------------------------
void TVPLoadTLG6(void* formatdata, void *callbackdata,
	tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback,
	tTJSBinaryStream *src,
	tjs_int keyidx,
	bool palettized)
{
	// load TLG v6.0 lossless/near-lossless compressed graphic
#if 0
	if(palettized)
		TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPTlgUnsupportedUniversalTransitionRule );
#endif
	unsigned char buf[12];

	src->ReadBuffer(buf, 4);

	tjs_int colors = buf[0]; // color component count

	if(colors != 1 && colors != 4 && colors != 3)
		TVPThrowExceptionMessage(TVPTLGLoadError,
			ttstr(TVPUnsupportedColorCount) + ttstr((int)colors)  );

	if(buf[1] != 0) // data flag
		TVPThrowExceptionMessage(TVPTLGLoadError,
			(const tjs_char*)TVPDataFlagMustBeZero );

	if(buf[2] != 0) // color type  (currently always zero)
		TVPThrowExceptionMessage(TVPTLGLoadError,
			ttstr(TVPUnsupportedColorTypeColon) + ttstr((int)buf[1])  );

	if(buf[3] != 0) // external golomb table (currently always zero)
		TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPUnsupportedExternalGolombBitLengthTable );

	tjs_int width, height;

	width = src->ReadI32LE();
	height = src->ReadI32LE();

	tjs_int max_bit_length;

	max_bit_length = src->ReadI32LE();

	// set destination size
	sizecallback(callbackdata, width, height, colors == 3 ? gpfRGB : gpfRGBA);

	// compute some values
	tjs_int x_block_count = (tjs_int)((width - 1)/ TVP_TLG6_W_BLOCK_SIZE) + 1;
	tjs_int y_block_count = (tjs_int)((height - 1)/ TVP_TLG6_H_BLOCK_SIZE) + 1;
	tjs_int main_count = width / TVP_TLG6_W_BLOCK_SIZE;
	tjs_int fraction = width -  main_count * TVP_TLG6_W_BLOCK_SIZE;

	// prepare memory pointers
	tjs_uint8 *bit_pool = NULL;
	tjs_uint32 *pixelbuf = NULL; // pixel buffer
	tjs_uint8 *filter_types = NULL;
	tjs_uint8 *LZSS_text = NULL;
	tjs_uint32 *zeroline = NULL;

	tjs_uint32 *tmpline[2] = { nullptr, nullptr };
	tjs_uint8 *grayline;
	try
	{
		// allocate memories
		bit_pool = (tjs_uint8 *)TJSAlignedAlloc(max_bit_length / 8 + 5, 4);
		pixelbuf = (tjs_uint32 *)TJSAlignedAlloc(
			sizeof(tjs_uint32) * width * TVP_TLG6_H_BLOCK_SIZE + 1, 4);
		filter_types = (tjs_uint8 *)TJSAlignedAlloc(
			x_block_count * y_block_count+16, 4);
		zeroline = (tjs_uint32 *)TJSAlignedAlloc(width * sizeof(tjs_uint32), 4);
		LZSS_text = (tjs_uint8*)TJSAlignedAlloc(4096+32, 4) + 16;


		// initialize zero line (virtual y=-1 line)
		TVPFillARGB(zeroline, width, colors==3?0xff000000:0x00000000);
			// 0xff000000 for colors=3 makes alpha value opaque

		// initialize LZSS text (used by chroma filter type codes)
		{
			tjs_uint32 *p = (tjs_uint32*)LZSS_text;
			for(tjs_uint32 i = 0; i < 32*0x01010101; i+=0x01010101)
			{
				for(tjs_uint32 j = 0; j < 16*0x01010101; j+=0x01010101)
					p[0] = i, p[1] = j, p += 2;
			}
		}

		// read chroma filter types.
		// chroma filter types are compressed via LZSS as used by TLG5.
		{
			tjs_int inbuf_size = src->ReadI32LE();
			tjs_uint8* inbuf = (tjs_uint8*)TJSAlignedAlloc(inbuf_size+16, 4);
			try
			{
				src->ReadBuffer(inbuf, inbuf_size);
				TVPTLG5DecompressSlide(filter_types, inbuf, inbuf_size,
					LZSS_text, 0);
			}
			catch(...)
			{
				TJSAlignedDealloc(inbuf);
				throw;
			}
			TJSAlignedDealloc(inbuf);
		}

		// for each horizontal block group ...
		tjs_uint32 *prevline = zeroline;
		for(tjs_int y = 0; y < height; y += TVP_TLG6_H_BLOCK_SIZE)
		{
			tjs_int ylim = y + TVP_TLG6_H_BLOCK_SIZE;
			if(ylim >= height) ylim = height;

			tjs_int pixel_count = (ylim - y) * width;

			// decode values
			for(tjs_int c = 0; c < colors; c++)
			{
				// read bit length
				tjs_int bit_length = src->ReadI32LE();

				// get compress method
				int method = (bit_length >> 30)&3;
				bit_length &= 0x3fffffff;

				// compute byte length
				tjs_int byte_length = bit_length / 8;
				if(bit_length % 8) byte_length++;

				// read source from input
				src->ReadBuffer(bit_pool, byte_length);

				// decode values
				// two most significant bits of bitlength are
				// entropy coding method;
				// 00 means Golomb method,
				// 01 means Gamma method (not yet suppoted),
				// 10 means modified LZSS method (not yet supported),
				// 11 means raw (uncompressed) data (not yet supported).

				switch(method)
				{
				case 0:
					if(c == 0 && colors != 1)
						TVPTLG6DecodeGolombValuesForFirst((tjs_int8*)pixelbuf,
							pixel_count, bit_pool);
					else
						TVPTLG6DecodeGolombValues((tjs_int8*)pixelbuf + c,
							pixel_count, bit_pool);
					break;
				default:
					TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPUnsupportedEntropyCodingMethod );
				}
			}

			// for each line
			unsigned char * ft =
				filter_types + (y / TVP_TLG6_H_BLOCK_SIZE)*x_block_count;
			int skipbytes = (ylim-y)*TVP_TLG6_W_BLOCK_SIZE;

			for(int yy = y; yy < ylim; yy++)
			{
				tjs_uint32* curline;
				if (!palettized)
					curline = (tjs_uint32*)scanlinecallback(callbackdata, yy);
				else {
					if (!tmpline[0]) {
						tmpline[0] = (tjs_uint32*)TJSAlignedAlloc(sizeof(tjs_uint32)* width, 4);
						tmpline[1] = (tjs_uint32*)TJSAlignedAlloc(sizeof(tjs_uint32)* width, 4);
					}
					curline = tmpline[yy & 1];
					grayline = (tjs_uint8*)scanlinecallback(callbackdata, yy);
				}

				int dir = (yy&1)^1;
				int oddskip = ((ylim - yy -1) - (yy-y));
				if(main_count)
				{
					int start =
						((width < TVP_TLG6_W_BLOCK_SIZE) ? width : TVP_TLG6_W_BLOCK_SIZE) *
							(yy - y);
					TVPTLG6DecodeLine(
						prevline,
						curline,
						width,
						main_count,
						ft,
						skipbytes,
						pixelbuf + start, colors==3?0xff000000:0, oddskip, dir);
				}

				if(main_count != x_block_count)
				{
					int ww = fraction;
					if(ww > TVP_TLG6_W_BLOCK_SIZE) ww = TVP_TLG6_W_BLOCK_SIZE;
					int start = ww * (yy - y);
					TVPTLG6DecodeLineGeneric(
						prevline,
						curline,
						width,
						main_count,
						x_block_count,
						ft,
						skipbytes,
						pixelbuf + start, colors==3?0xff000000:0, oddskip, dir);
				}

				prevline = curline;
				if (palettized) {
					for (int x = 0; x < width; ++x) {
						grayline[x] = curline[x] & 0xFF; // red -> lumi
					}
				}
				scanlinecallback(callbackdata, -1);
			}

		}
	}
	catch(...)
	{
		if(bit_pool) TJSAlignedDealloc(bit_pool);
		if(pixelbuf) TJSAlignedDealloc(pixelbuf);
		if(filter_types) TJSAlignedDealloc(filter_types);
		if(zeroline) TJSAlignedDealloc(zeroline);
		if(LZSS_text) TJSAlignedDealloc(LZSS_text - 16);
		if (tmpline[0]) {
			TJSAlignedDealloc(tmpline[0]);
			TJSAlignedDealloc(tmpline[1]);
		}
		throw;
	}
	if(bit_pool) TJSAlignedDealloc(bit_pool);
	if(pixelbuf) TJSAlignedDealloc(pixelbuf);
	if(filter_types) TJSAlignedDealloc(filter_types);
	if(zeroline) TJSAlignedDealloc(zeroline);
	if(LZSS_text) TJSAlignedDealloc(LZSS_text - 16);
	if (tmpline[0]) {
		TJSAlignedDealloc(tmpline[0]);
		TJSAlignedDealloc(tmpline[1]);
	}
}





//---------------------------------------------------------------------------
// TLG loading handler
//---------------------------------------------------------------------------
static void TVPInternalLoadTLG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode)
{
	// read header
	unsigned char mark[12];
	src->ReadBuffer(mark, 11);

	// check for TLG raw data
	if(!memcmp("TLG5.0\x00raw\x1a\x00", mark, 11))
	{
		TVPLoadTLG5(formatdata, callbackdata, sizecallback,
			scanlinecallback, src, keyidx, mode);
	}
	else if(!memcmp("TLG6.0\x00raw\x1a\x00", mark, 11))
	{
		TVPLoadTLG6(formatdata, callbackdata, sizecallback,
			scanlinecallback, src, keyidx, mode!=glmNormal);
	}
	else
	{
		TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPInvalidTlgHeaderOrVersion );
	}
}
//---------------------------------------------------------------------------

void TVPLoadTLG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode)
{
	// read header
	unsigned char mark[12];
	src->ReadBuffer(mark, 11);

	// check for TLG0.0 sds
	if(!memcmp("TLG0.0\x00sds\x1a\x00", mark, 11))
	{
		// read TLG0.0 Structured Data Stream

		// TLG0.0 SDS tagged data is simple "NAME=VALUE," string;
		// Each NAME and VALUE have length:content expression.
		// eg: 4:LEFT=2:20,3:TOP=3:120,4:TYPE=1:3,
		// The last ',' cannot be ommited.
		// Each string (name and value) must be encoded in utf-8.

		// read raw data size
		tjs_uint rawlen = src->ReadI32LE();

		// try to load TLG raw data
		TVPInternalLoadTLG(formatdata, callbackdata, sizecallback,
			scanlinecallback, metainfopushcallback, src, keyidx, mode);

		// seek to meta info data point
		src->Seek(rawlen + 11 + 4, TJS_BS_SEEK_SET);

		// read tag data
		while(true)
		{
			char chunkname[4];
			if(4 != src->Read(chunkname, 4)) break;
				// cannot read more
			tjs_uint chunksize = src->ReadI32LE();
			if(!memcmp(chunkname, "tags", 4))
			{
				// tag information
				char *tag = NULL;
				char *name = NULL;
				char *value = NULL;
				try
				{
					tag = new char [chunksize + 1];
					src->ReadBuffer(tag, chunksize);
					tag[chunksize] = 0;
					if(metainfopushcallback)
					{
						const char *tagp = tag;
						const char *tagp_lim = tag + chunksize;
						while(tagp < tagp_lim)
						{
							tjs_uint namelen = 0;
							while(*tagp >= '0' && *tagp <= '9')
								namelen = namelen * 10 + *tagp - '0', tagp++;
							if(*tagp != ':') TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPTlgMalformedTagMissionColonAfterNameLength );
							tagp ++;
							name = new char [namelen + 1];
							memcpy(name, tagp, namelen);
							name[namelen] = '\0';
							tagp += namelen;
							if(*tagp != '=') TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPTlgMalformedTagMissionEqualsAfterName );
							tagp++;
							tjs_uint valuelen = 0;
							while(*tagp >= '0' && *tagp <= '9')
								valuelen = valuelen * 10 + *tagp - '0', tagp++;
							if(*tagp != ':') TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPTlgMalformedTagMissionColonAfterVaueLength );
							tagp++;
							value = new char [valuelen + 1];
							memcpy(value, tagp, valuelen);
							value[valuelen] = '\0';
							tagp += valuelen;
 							if(*tagp != ',') TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPTlgMalformedTagMissionCommaAfterTag );
							tagp++;

							// insert into name-value pairs ... TODO: utf-8 decode
							metainfopushcallback(callbackdata, ttstr(name), ttstr(value));

							delete [] name, name = NULL;
							delete [] value, value = NULL;
						}
					}
				}
				catch(...)
				{
					if(tag) delete [] tag;
					if(name) delete [] name;
					if(value) delete [] value;
					throw;
				}

				if(tag) delete [] tag;
				if(name) delete [] name;
				if(value) delete [] value;
			}
			else
			{
				// skip the chunk
				src->SetPosition(src->GetPosition() + chunksize);
			}
		} // while

	}
	else
	{
		src->Seek(0, TJS_BS_SEEK_SET); // rewind

		// try to load TLG raw data
		TVPInternalLoadTLG(formatdata, callbackdata, sizecallback,
			scanlinecallback, metainfopushcallback, src, keyidx, mode);
	}


}
//---------------------------------------------------------------------------
void TVPLoadHeaderTLG(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic )
{
	if( dic == NULL ) return;

	// read header
	unsigned char mark[12];
	src->ReadBuffer(mark, 11);

	tjs_int width = 0;
	tjs_int height = 0;
	tjs_int colors = 0;
	// check for TLG0.0 sds
	if(!memcmp("TLG0.0\x00sds\x1a\x00", mark, 11))
	{
		// read raw data size
		tjs_uint rawlen = src->ReadI32LE();
		src->ReadBuffer(mark, 11);
		if(!memcmp("TLG5.0\x00raw\x1a\x00", mark, 11))
		{
			src->ReadBuffer(mark, 1);
			colors = mark[0];
			width = src->ReadI32LE();
			height = src->ReadI32LE();
		}
		else if(!memcmp("TLG6.0\x00raw\x1a\x00", mark, 11))
		{
			src->ReadBuffer(mark, 4);
			colors = mark[0]; // color component count
			width = src->ReadI32LE();
			height = src->ReadI32LE();
		}
		else
		{
			TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPInvalidTlgHeaderOrVersion );
		}
	}
	else if(!memcmp("TLG5.0\x00raw\x1a\x00", mark, 11))
	{
		src->ReadBuffer(mark, 1);
		colors = mark[0];
		width = src->ReadI32LE();
		height = src->ReadI32LE();
	}
	else if(!memcmp("TLG6.0\x00raw\x1a\x00", mark, 11))
	{
		src->ReadBuffer(mark, 4);
		colors = mark[0]; // color component count
		width = src->ReadI32LE();
		height = src->ReadI32LE();
	}
	else
	{
		TVPThrowExceptionMessage(TVPTLGLoadError, (const tjs_char*)TVPInvalidTlgHeaderOrVersion );
	}
	tjs_int bpp = colors * 8;
	*dic = TJSCreateDictionaryObject();
	tTJSVariant val(width);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("width"), 0, &val, (*dic) );
	val = tTJSVariant(height);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("height"), 0, &val, (*dic) );
	val = tTJSVariant(bpp);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("bpp"), 0, &val, (*dic) );
}
//---------------------------------------------------------------------------
