#include "tjsCommHead.h"
#include "GraphicsLoaderIntf.h"
#include "MsgIntf.h"
#include "tjsDictionary.h"
#include <memory>

extern "C" {
#include "libbpg/libbpg.h"
}

extern void TVPInitLibAVCodec();

struct CBPGDecoderContext {
	BPGDecoderContext *ctx;
	CBPGDecoderContext() {
		TVPInitLibAVCodec();
		ctx = bpg_decoder_open();
	}
	~CBPGDecoderContext() {
		bpg_decoder_close(ctx);
	}
	BPGDecoderContext *get() { return ctx; }
};

void TVPLoadBPG(void* formatdata, void *callbackdata,
	tTVPGraphicSizeCallback sizecallback, tTVPGraphicScanLineCallback scanlinecallback,
	tTVPMetaInfoPushCallback metainfopushcallback, tTJSBinaryStream *src, tjs_int keyidx,
	tTVPGraphicLoadMode mode)
{
	CBPGDecoderContext img;
	int datasize = src->GetSize();
	std::unique_ptr<uint8_t[]> data(new uint8_t[datasize]);
	src->ReadBuffer(data.get(), datasize);

	if (bpg_decoder_decode(img.get(), data.get(), datasize) < 0) {
		TVPThrowExceptionMessage(TJS_W("Invalid BPG image"));
	}

	BPGImageInfo img_info;

	bpg_decoder_get_info(img.get(), &img_info);

	sizecallback(callbackdata, img_info.width, img_info.height, img_info.has_alpha ? gpfRGBA : gpfRGB);
	bpg_decoder_start(img.get(), BPG_OUTPUT_FORMAT_RGBA32);
	if (glmNormal == mode) {
		for (uint32_t y = 0; y < img_info.height; y++) {
			bpg_decoder_get_line(img.get(), (uint8_t*)scanlinecallback(callbackdata, y));
		}
	} else if (glmGrayscale == mode) {
		for (uint32_t y = 0; y < img_info.height; y++) {
			bpg_decoder_get_gray_line(img.get(), (uint8_t*)scanlinecallback(callbackdata, y));
		}
	}
	scanlinecallback(callbackdata, -1); // image was written
}

void TVPLoadHeaderBPG(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic) {

	CBPGDecoderContext img;
	int datasize = src->GetSize();
	std::unique_ptr<uint8_t[]> data(new uint8_t[datasize]);
	src->ReadBuffer(data.get(), datasize);

	if (bpg_decoder_decode(img.get(), data.get(), datasize) < 0) {
		TVPThrowExceptionMessage(TJS_W("Invalid BPG image"));
	}
	BPGImageInfo img_info;
	bpg_decoder_get_info(img.get(), &img_info);

	*dic = TJSCreateDictionaryObject();
	tTJSVariant val((tjs_int32)img_info.width);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("width"), 0, &val, (*dic));
	val = tTJSVariant((tjs_int32)img_info.height);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("height"), 0, &val, (*dic));
	val = tTJSVariant(img_info.has_alpha ? 32 : 24);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("bpp"), 0, &val, (*dic));
}

