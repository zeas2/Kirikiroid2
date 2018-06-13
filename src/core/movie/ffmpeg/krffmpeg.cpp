#ifdef _MSC_VER
#include <windows.h>
#endif
#include <thread>
#include "krmovie.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
}
#include <mutex>
#include "MsgIntf.h"
#include "StorageIntf.h"
#include "VideoOvlImpl.h"
#include "KRMoviePlayer.h"
#include "KRMovieLayer.h"

extern std::thread::id TVPMainThreadID;

static int lockmgr(void **arg, enum AVLockOp op)
{
	std::mutex **mtx = (std::mutex**)arg;
	switch (op) {
	case AV_LOCK_CREATE:
		*mtx = new std::mutex();
		if (!*mtx)
			return 1;
		break;
	case AV_LOCK_OBTAIN:
		(*mtx)->lock();
		break;
	case AV_LOCK_RELEASE:
		(*mtx)->unlock();
		break;
	case AV_LOCK_DESTROY:
		delete *mtx;
		break;
	default:
		return 1;
	}
	return 0;
}

static bool FFInitilalized = false;
void TVPInitLibAVCodec() {
	if (!FFInitilalized) {
		/* register all codecs, demux and protocols */
		if (av_lockmgr_register(lockmgr)) {
			TVPThrowExceptionMessage(TJS_W("Could not initialize lock manager!"));
		}
		avcodec_register_all();
		av_register_all();
		avfilter_register_all();
		avformat_network_init();
	//	av_log_set_callback(ff_avutil_log);
		FFInitilalized = true;
	}
}

void GetVideoOverlayObject(
	tTJSNI_VideoOverlay* callbackwin, IStream *stream, const tjs_char * streamname,
	const tjs_char *type, uint64_t size, iTVPVideoOverlay **out)
{
	*out = new KRMovie::MoviePlayerOverlay;

	if (*out)
		static_cast<KRMovie::MoviePlayerOverlay*>(*out)->BuildGraph(callbackwin, stream, streamname, type, size);
}

void GetVideoLayerObject(
	tTJSNI_VideoOverlay* callbackwin, IStream *stream, const tjs_char * streamname,
	const tjs_char *type, uint64_t size, iTVPVideoOverlay **out)
{
	*out = new KRMovie::MoviePlayerLayer;

	if (*out)
		static_cast<KRMovie::MoviePlayerLayer*>(*out)->BuildGraph(callbackwin, stream, streamname, type, size);
}

void GetMixingVideoOverlayObject(
	tTJSNI_VideoOverlay* callbackwin, IStream *stream, const tjs_char * streamname,
	const tjs_char *type, uint64_t size, iTVPVideoOverlay **out)
{
	*out = new KRMovie::MoviePlayerOverlay;

	if (*out)
		static_cast<KRMovie::MoviePlayerOverlay*>(*out)->BuildGraph(callbackwin, stream, streamname, type, size);
}

void GetMFVideoOverlayObject(
	tTJSNI_VideoOverlay* callbackwin, IStream *stream, const tjs_char * streamname,
	const tjs_char *type, uint64_t size, iTVPVideoOverlay **out)
{
	*out = new KRMovie::MoviePlayerOverlay;

	if (*out)
		static_cast<KRMovie::MoviePlayerOverlay*>(*out)->BuildGraph(callbackwin, stream, streamname, type, size);
}

static int AVReadFunc(void *opaque, uint8_t *buf, int buf_size)
{
	TJS::tTJSBinaryStream *stream = (TJS::tTJSBinaryStream *)opaque;
	return stream->Read(buf, buf_size);
}

static int64_t AVSeekFunc(void *opaque, int64_t offset, int whence)
{
	TJS::tTJSBinaryStream *stream = (TJS::tTJSBinaryStream *)opaque;
	switch (whence)
	{
	case AVSEEK_SIZE:
		return stream->GetSize();
	default:
		return stream->Seek(offset, whence & 0xFF);
	}
}

bool TVPCheckIsVideoFile(const char *uri) {
	TVPInitLibAVCodec();
	tTJSBinaryStream* stream = TVPCreateStream(uri, TJS_BS_READ);
	int bufSize = 32 * 1024;
	if (stream->GetSize() < bufSize) {
		delete stream;
		return false;
	}
	AVIOContext *pIOCtx = avio_alloc_context(
		(unsigned char *)av_malloc(bufSize + AVPROBE_PADDING_SIZE),
		bufSize,  // internal Buffer and its size
		false,                  // bWriteable (1=true,0=false) 
		stream,             // user data ; will be passed to our callback functions
		AVReadFunc,
		nullptr,                  // Write callback function (not used in this example) 
		AVSeekFunc);
	if (!pIOCtx) {
		return false;
	}
	AVInputFormat *fmt = NULL;
	av_probe_input_buffer2(pIOCtx, &fmt, uri, NULL, 0, 0);
	bool ret = false;
	if (fmt) {
		AVFormatContext *ic = avformat_alloc_context();
		ic->interrupt_callback.callback = nullptr;
		ic->interrupt_callback.opaque = nullptr;
		ic->pb = pIOCtx;
		if (avformat_open_input(&ic, "", fmt, nullptr) == 0) {
			if (avformat_find_stream_info(ic, nullptr) == 0) {
				int vid = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
				if (vid >= 0)
					ret = true;
			}
		}
		avformat_free_context(ic);
	}
	av_free(pIOCtx);
	delete stream;
	return ret;
}