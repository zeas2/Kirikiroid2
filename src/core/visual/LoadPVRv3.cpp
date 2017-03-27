#include "tjsCommHead.h"
#include "GraphicsLoaderIntf.h"
#include "tjsDictionary.h"
#include "MsgIntf.h"
#include "ogl/etcpak.h"
#include "ogl/pvr.h"
#include "UtilStreams.h"
#include "RenderManager.h"

// pvr format only used as normal picture or univ trans rule ( not as province image )

static ttstr TVPUnserializeString(tTJSBinaryStream * s) {
	tjs_uint16 l = s->ReadI16LE();
	std::vector<tjs_char> strbuf; strbuf.resize(l + 1);
	l += l;
	if (s->Read(&strbuf.front(), l) != l) {
		return ttstr();
	}
	return &strbuf.front();
}

static tTJSVariant TVPUnserializeVariable(tTJSBinaryStream * s) {
	tjs_uint8 typ = 0;
	s->ReadBuffer(&typ, 1);
	switch (typ) {
	case tvtVoid:
		return tTJSVariant();
	case tvtString:
		return TVPUnserializeString(s);
	}
}

static int TVPUnserializePVRv3Metadata(tTJSBinaryStream *s, const std::function<void(const ttstr&, const tTJSVariant&)> &cb) {
	char tmp[4];
	s->ReadBuffer(tmp, 4);
	if (memcmp(tmp, "tags", 4)) return 0;
	tjs_uint32 count = s->ReadI32LE();
	for (tjs_uint32 i = 0; i < count; ++i) {
		ttstr name = TVPUnserializeString(s);
		if (name.IsEmpty()) {
			return i;
		}
		cb(name, TVPUnserializeVariable(s));
	}
	return count;
}

// load texture directly
iTVPTexture2D* TVPLoadPVRv3(tTJSBinaryStream *src, const std::function<void(const ttstr&, const tTJSVariant&)> &cb) {
	iTVPTexture2D* ret = TVPGetRenderManager()->CreateTexture2D(src);
	if (!ret) return nullptr;
	src->SetPosition(0);
	PVRv3Header hdr;
	src->ReadBuffer(&hdr, sizeof(hdr));
	if (hdr.metadataLength > 8) {
		tTVPMemoryStream metadata; metadata.SetSize(hdr.metadataLength);
		src->ReadBuffer(metadata.GetInternalBuffer(), hdr.metadataLength);
		TVPUnserializePVRv3Metadata(&metadata, cb);
	}
}

void TVPLoadPVRv3(void* formatdata, void *callbackdata,
	tTVPGraphicSizeCallback sizecallback, tTVPGraphicScanLineCallback scanlinecallback,
	tTVPMetaInfoPushCallback metainfopushcallback, tTJSBinaryStream *src, tjs_int keyidx,
	tTVPGraphicLoadMode mode) {
	if (mode == glmPalettized) {
		TVPThrowExceptionMessage(TJS_W("PVR Load Error/Unsupport palettized image"));
	}
	PVRv3Header hdr;
	src->ReadBuffer(&hdr, sizeof(hdr));
	if (hdr.version != 0x03525650) {
		TVPThrowExceptionMessage(TJS_W("PVR Load Error/Invalid image"));
	}

	tjs_uint64 dataoff = src->GetPosition() + hdr.metadataLength;

	if (hdr.metadataLength > 8) {
		tTVPMemoryStream metadata; metadata.SetSize(hdr.metadataLength);
		src->ReadBuffer(metadata.GetInternalBuffer(), hdr.metadataLength);
		TVPUnserializePVRv3Metadata(&metadata, [metainfopushcallback, callbackdata](const ttstr& k, const tTJSVariant& v) {
			metainfopushcallback(callbackdata, k, v);
		});
	}

	src->SetPosition(dataoff);
	int blkh = hdr.height / 4 + !!(hdr.height & 3);
	int blkw = hdr.width / 4 + !!(hdr.width & 3);
	int blksize = 0;
	tTVPGraphicPixelFormat fmt;
	switch (hdr.pixelFormat) {
	case 22: // ETC2_RGB
		blksize = 8;
		fmt = gpfRGB;
		break;
	case 23: // ETC2_RGBA
		blksize = 16;
		fmt = gpfRGBA;
		break;
	default:
		TVPThrowExceptionMessage(TJS_W("PVR Load Error/Unsupport format [%1]"),
			ttstr((tjs_int)hdr.pixelFormat));
	}
	// ignore mipmap
	size_t size = blkw * blkh * blksize;
	tjs_int pitch = sizecallback(callbackdata, hdr.width, hdr.height, fmt);
	std::vector<unsigned char> buf; buf.resize(size);
	src->ReadBuffer(&buf.front(), size);
	void *pixel = scanlinecallback(callbackdata, 0);
	switch (hdr.pixelFormat) {
	case 22: // ETC2_RGB
		ETCPacker::decode(&buf.front(), pixel, pitch, hdr.height, blkw, blkh);
		break;
	case 23: // ETC2_RGBA
		ETCPacker::decodeWithAlpha(&buf.front(), pixel, pitch, hdr.height, blkw, blkh);
		break;
	}
	scanlinecallback(callbackdata, -1);
}

void TVPLoadHeaderPVRv3(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic) {
	PVRv3Header hdr;
	src->ReadBuffer(&hdr, sizeof(hdr));
	if (hdr.version != 0x03525650) {
		TVPThrowExceptionMessage(TJS_W("PVR Load Error/Invalid image"));
	}
	*dic = TJSCreateDictionaryObject();
	tTJSVariant val((tjs_int32)hdr.width);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("width"), 0, &val, (*dic));
	val = tTJSVariant((tjs_int32)hdr.height);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("height"), 0, &val, (*dic));
	tjs_int64 bpp = 0;
	switch (hdr.pixelFormat) {
	case 22: // ETC2_RGB
		bpp = 24;
		break;
	case 23: // ETC2_RGBA
		bpp = 32;
		break;
	default:
		TVPThrowExceptionMessage(TJS_W("PVR Load Error/Unsupport format [%1]"),
			ttstr((tjs_int)hdr.pixelFormat));
	}
	if (hdr.metadataLength > 8) {
		tTVPMemoryStream metadata; metadata.SetSize(hdr.metadataLength);
		src->ReadBuffer(metadata.GetInternalBuffer(), hdr.metadataLength);
		TVPUnserializePVRv3Metadata(&metadata, [dic](const ttstr& k, const tTJSVariant& v) {
			(*dic)->PropSet(TJS_MEMBERENSURE, k.c_str(), const_cast<ttstr&>(k).GetHint(), &v, (*dic));
		});
	}
	val = tTJSVariant(bpp);
	(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("bpp"), 0, &val, (*dic));
}
