#include "tjsCommHead.h"
#include "GraphicsLoaderIntf.h"
#include "tjsDictionary.h"
#include "MsgIntf.h"
#include "ogl/etcpak.h"
#include "ogl/pvr.h"
#include "UtilStreams.h"
#include "RenderManager.h"
#include "tvpgl.h"
#include "base/pvr.h"
#include "LayerBitmapIntf.h"

// pvr format only used as normal picture or univ trans rule ( not as province image )

static ttstr TVPUnserializeString(tTJSBinaryStream * s) {
	tjs_uint16 l = s->ReadI16LE();
	std::vector<char> strbuf; strbuf.resize(l + 1);
	if (s->Read(&strbuf.front(), l) != l) { // in utf-8
		return ttstr();
	}
	return &strbuf.front();
}

static tTJSVariant TVPUnserializePVRv3Variable(tTJSBinaryStream * s) {
	tjs_uint8 typ = 0;
	s->ReadBuffer(&typ, 1);
	switch (typ) {
	case tvtVoid:
		return tTJSVariant();
	case tvtString:
		return TVPUnserializeString(s);
	case tvtInteger:
		return (tjs_int64)s->ReadI64LE();
	case tvtReal:
	{
		tTVReal v;
		s->ReadBuffer(&v, sizeof(v));
		return v;
	}
	default:
		return tTJSVariant();
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
		cb(name, TVPUnserializePVRv3Variable(s));
	}
	return count;
}

static unsigned int TVPSerializePVRv3String(tTJSBinaryStream *dst, const std::string &str) {
	tjs_uint16 l = str.size();
	dst->Write(&l, sizeof(l));
	dst->Write(str.c_str(), l);
	return l + 2;
}

static unsigned int TVPSerializePVRv3Variable(tTJSBinaryStream *dst, const tTJSVariant &v) {
	tjs_uint8 ty = v.Type();
	dst->WriteBuffer(&ty, sizeof(ty));
	tjs_int64 intv;
	tTVReal fltv;
	switch (v.Type()) {
	case tvtString:
		return TVPSerializePVRv3String(dst, ttstr(v).AsStdString()) + 1;
	case tvtInteger:
		intv = v.AsInteger();
		dst->WriteBuffer(&intv, sizeof(intv));
		return 9;
	case tvtReal:
		fltv = v.AsReal();
		dst->WriteBuffer(&fltv, sizeof(fltv));
		return 9;
	default:
		return 1;
	}
}

static unsigned int TVPSerializePVRv3Metadata(tTJSBinaryStream *dst, const std::vector<std::pair<ttstr, tTJSVariant> >& tags) {
	dst->WriteBuffer("tags", 4);
	tjs_uint32 count = tags.size();
	dst->WriteBuffer(&count, sizeof(count));
	unsigned int totalSize = 8;
	for (const std::pair<ttstr, tTJSVariant> &it : tags) {
		totalSize += TVPSerializePVRv3String(dst, it.first.AsStdString());
		totalSize += TVPSerializePVRv3Variable(dst, it.second);
	}
	return totalSize;
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
	return ret;
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
	unsigned int eachblkw = 0, eachblkh = 0;
	unsigned int blksize = 0;
	tTVPGraphicPixelFormat fmt;
	switch ((PVR3TexturePixelFormat)hdr.pixelFormat) {
	case PVR3TexturePixelFormat::PVRTC2BPP_RGB:
		eachblkw = 8; eachblkh = 4; blksize = 8; fmt = gpfRGB; break;
	case PVR3TexturePixelFormat::PVRTC2BPP_RGBA:
		eachblkw = 8; eachblkh = 4; blksize = 8; fmt = gpfRGBA; break;
	case PVR3TexturePixelFormat::PVRTC4BPP_RGB:
		eachblkw = 4; eachblkh = 4; blksize = 8; fmt = gpfRGB; break;
	case PVR3TexturePixelFormat::PVRTC4BPP_RGBA:
		eachblkw = 4; eachblkh = 4; blksize = 8; fmt = gpfRGBA; break;
	case PVR3TexturePixelFormat::ETC1:
	case PVR3TexturePixelFormat::ETC2_RGB:
		eachblkw = 4; eachblkh = 4; blksize = 8; fmt = gpfRGB; break;
	case PVR3TexturePixelFormat::ETC2_RGBA:
		eachblkw = 4; eachblkh = 4; blksize = 16; fmt = gpfRGBA; break;
	case PVR3TexturePixelFormat::BGRA8888:
	case PVR3TexturePixelFormat::RGBA8888:
		eachblkw = 1; eachblkh = 1; blksize = 4; fmt = gpfRGBA; break;
	case PVR3TexturePixelFormat::RGB888:
		eachblkw = 1; eachblkh = 1; blksize = 3; fmt = gpfRGB; break;
	case PVR3TexturePixelFormat::L8:
		eachblkw = 1; eachblkh = 1; blksize = 1; fmt = gpfRGB; break;
	default:
		TVPThrowExceptionMessage(TJS_W("PVR Load Error/Unsupport format [%1]"),
			ttstr((tjs_int)hdr.pixelFormat));
	}
	// ignore mipmap
	unsigned int blkh = hdr.height / eachblkh + !!(hdr.height & (eachblkh - 1));
	unsigned int blkw = hdr.width / eachblkw + !!(hdr.width & (eachblkw - 1));
	size_t size = blkw * blkh * blksize;
	tjs_int pitch = sizecallback(callbackdata, hdr.width, hdr.height, fmt);
	std::vector<unsigned char> buf; buf.resize(size);
	unsigned char *psrc = &buf.front();
	src->SetPosition(dataoff);
	src->ReadBuffer(psrc, size);
	void *pixel = scanlinecallback(callbackdata, 0);
	unsigned char *pdst = (unsigned char *)pixel;
	switch ((PVR3TexturePixelFormat)hdr.pixelFormat) {
	case PVR3TexturePixelFormat::PVRTC2BPP_RGB:
	case PVR3TexturePixelFormat::PVRTC2BPP_RGBA:
		PVRTDecompressPVRTC(psrc, pitch / 4, hdr.height, pixel, true);
		break;
	case PVR3TexturePixelFormat::PVRTC4BPP_RGB:
	case PVR3TexturePixelFormat::PVRTC4BPP_RGBA:
		PVRTDecompressPVRTC(psrc, pitch / 4, hdr.height, pixel, false);
		break;
	case PVR3TexturePixelFormat::ETC1:
	case PVR3TexturePixelFormat::ETC2_RGB:
		ETCPacker::decode(psrc, pixel, pitch, hdr.height, blkw, blkh);
		break;
	case PVR3TexturePixelFormat::ETC2_RGBA:
		ETCPacker::decodeWithAlpha(psrc, pixel, pitch, hdr.height, blkw, blkh);
		break;
	case PVR3TexturePixelFormat::BGRA8888:
		for (uint32_t y = 0; y < hdr.height; ++y) {
			TVPReverseRGB((tjs_uint32 *)pixel, (const tjs_uint32 *)psrc, hdr.width);
			pdst += pitch;
			psrc += hdr.width * 4;
		}
		break;
	case PVR3TexturePixelFormat::RGBA8888:
		for (uint32_t y = 0; y < hdr.height; ++y) {
			memcpy(pixel, psrc, hdr.width * 4);
			pdst += pitch;
			psrc += hdr.width * 4;
		}
		break;
	case PVR3TexturePixelFormat::RGB888:
		for (uint32_t y = 0; y < hdr.height; ++y) {
			TVPConvert24BitTo32Bit((tjs_uint32 *)pixel, (const tjs_uint8 *)psrc, hdr.width);
			pdst += pitch;
			psrc += hdr.width * 3;
		}
		break;
	case PVR3TexturePixelFormat::L8:
		for (uint32_t y = 0; y < hdr.height; ++y) {
			TVPExpand8BitTo32BitGray((tjs_uint32 *)pixel, (const tjs_uint8 *)psrc, hdr.width);
			pdst += pitch;
			psrc += hdr.width;
		}
		break;
	case PVR3TexturePixelFormat::RGBA4444: // not support yet
	case PVR3TexturePixelFormat::RGBA5551:
	case PVR3TexturePixelFormat::RGB565:
	case PVR3TexturePixelFormat::A8:
	case PVR3TexturePixelFormat::LA88:
	default:
		TVPThrowExceptionMessage(TJS_W("PVR Load Error/Unsupport format [%1]"),
			ttstr((tjs_int)hdr.pixelFormat));
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
	switch ((PVR3TexturePixelFormat)hdr.pixelFormat) {
	case PVR3TexturePixelFormat::ETC1:
	case PVR3TexturePixelFormat::ETC2_RGB:
	case PVR3TexturePixelFormat::RGB888:
	case PVR3TexturePixelFormat::PVRTC2BPP_RGB:
	case PVR3TexturePixelFormat::PVRTC4BPP_RGB:
		bpp = 24; break;
	case PVR3TexturePixelFormat::PVRTC2BPP_RGBA:
	case PVR3TexturePixelFormat::PVRTC4BPP_RGBA:
	case PVR3TexturePixelFormat::ETC2_RGBA:
	case PVR3TexturePixelFormat::BGRA8888:
	case PVR3TexturePixelFormat::RGBA8888:
		bpp = 32; break;
	case PVR3TexturePixelFormat::L8:
		bpp = 8; break;
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

void TVPSavePVRv3(void* formatdata, tTJSBinaryStream* dst, const iTVPBaseBitmap* image, const ttstr & mode, iTJSDispatch2* meta) {
	std::vector<std::pair<ttstr, tTJSVariant> > tags;
	if (meta) {
		struct MetaDictionaryEnumCallback : public tTJSDispatch {
			std::vector<std::pair<ttstr, tTJSVariant> >& Tags;
			MetaDictionaryEnumCallback(std::vector<std::pair<ttstr, tTJSVariant> >& tags) : Tags(tags) {}
			tjs_error TJS_INTF_METHOD FuncCall(tjs_uint32 flag, const tjs_char * membername,
				tjs_uint32 *hint, tTJSVariant *result, tjs_int numparams,
				tTJSVariant **param, iTJSDispatch2 *objthis) {
				// called from tTJSCustomObject::EnumMembers
				if (numparams < 3) return TJS_E_BADPARAMCOUNT;

				// hidden members are not processed
				tjs_uint32 flags = (tjs_int)*param[1];
				if (flags & TJS_HIDDENMEMBER) {
					if (result) *result = (tjs_int)1;
					return TJS_S_OK;
				}
				// push items
				Tags.emplace_back(*param[0], *param[2]);
				if (result) *result = (tjs_int)1;
				return TJS_S_OK;
			}
		} callback(tags);
		tTJSVariantClosure clo(&callback, NULL);
		meta->EnumMembers(TJS_IGNOREPROP, &clo, meta);
	}
	tTVPMemoryStream memstr;
	const void *pixeldata = image->GetScanLine(0);
	int w = image->GetWidth(), h = image->GetHeight(), pitch = image->GetPitchBytes();
	PVR3TexturePixelFormat pixelFormat = PVR3TexturePixelFormat::RGBA8888;
	if (mode == TJS_W("ETC2_RGB")) {
		pixelFormat = PVR3TexturePixelFormat::ETC2_RGB;
		// drop alpha data
		size_t size;
		tjs_uint8 *data = (tjs_uint8 *)ETCPacker::convert(pixeldata, w, h, pitch, true, size);
		memstr.WriteBuffer(data, size);
	} else if (mode == TJS_W("ETC2_RGBA")) {
		pixelFormat = PVR3TexturePixelFormat::ETC2_RGBA;
		size_t size;
		tjs_uint8 *data = (tjs_uint8 *)ETCPacker::convertWithAlpha(pixeldata, w, h, pitch, size);
		memstr.WriteBuffer(data, size);
	} else if (mode == TJS_W("ETC1")) {
		pixelFormat = PVR3TexturePixelFormat::ETC1;
		// drop alpha data
		size_t size;
		tjs_uint8 *data = (tjs_uint8 *)ETCPacker::convert(pixeldata, w, h, pitch, false, size);
		memstr.WriteBuffer(data, size);
	} else { // direct save as RGBA8888
		for (int y = 0; y < h; ++y) {
			memstr.WriteBuffer(image->GetScanLine(y), w * 4);
		}
	}
	dst->WriteBuffer("PVR\3", 4);
	dst->WriteBuffer("\0\0\0\0", 4); // no premultiply alpha
	dst->WriteBuffer(&pixelFormat, sizeof(pixelFormat));
	dst->WriteBuffer("\0\0\0\0", 4); // colorSpace
	dst->WriteBuffer("\0\0\0\0", 4); // channelType
	dst->WriteBuffer(&h, sizeof(h));
	dst->WriteBuffer(&w, sizeof(w));
	dst->WriteBuffer("\1\0\0\0", 4); // depth
	dst->WriteBuffer("\1\0\0\0", 4); // numberOfSurfaces
	dst->WriteBuffer("\1\0\0\0", 4); // numberOfFaces
	dst->WriteBuffer("\1\0\0\0", 4); // numberOfMipmaps
	if (tags.empty()) {
		dst->WriteBuffer("\0\0\0\0", 4); // metadataLength
	} else {
		TVPSerializePVRv3Metadata(dst, tags);
	}
	dst->WriteBuffer(memstr.GetInternalBuffer(), memstr.GetSize());
}
