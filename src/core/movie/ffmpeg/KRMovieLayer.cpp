#include "KRMovieLayer.h"
#include "VideoCodec.h"
#include "LayerBitmapIntf.h"
#include "Application.h"
#include "VideoOvlImpl.h"
extern "C" {
#include "libswscale/swscale.h"
}

NS_KRMOVIE_BEGIN

VideoPresentLayer::~VideoPresentLayer()
{
	TVPRemoveContinuousEventHook(this);
}

tTVPBaseTexture* VideoPresentLayer::GetFrontBuffer()
{
	BitmapPicture pic;
	if (!m_usedPicture) {
		return nullptr;
	}
	{
		std::lock_guard<std::mutex> lk(m_mtxPicture);
		BitmapPicture &picbuf = m_picture[m_curPicture];
		picbuf.swap(pic);
		m_curPicture = (m_curPicture + 1) & (MAX_BUFFER_COUNT - 1);
		--m_usedPicture;
		assert(m_usedPicture >= 0);
		m_condPicture.notify_all();
	}
	FrameMove();
	int n = m_nCurBmpBuff;
	m_nCurBmpBuff = !m_nCurBmpBuff;
	m_BmpBits[n]->Update(pic.data[0], pic.width * 4, 0, 0, pic.width, pic.height);
	return m_BmpBits[n];
}

void VideoPresentLayer::SetVideoBuffer(tTVPBaseTexture *buff1, tTVPBaseTexture *buff2, long size)
{
	m_BmpBits[0] = buff1;
	m_BmpBits[1] = buff2;
	m_nCurBmpBuff = 0;
//	TVPAddContinuousEventHook(this);
}

void VideoPresentLayer::OnContinuousCallback(tjs_uint64 tick)
{
	if (!m_usedPicture) return;
	double m_curpts = m_pPlayer->GetClock() / DVD_TIME_BASE;
	{
		std::lock_guard<std::mutex> lk(m_mtxPicture);
		BitmapPicture &picbuf = m_picture[m_curPicture];
		// check pts
		if (picbuf.pts > m_curpts) { // present in future
			return;
		}
	}
#if 0
	do { // skip frame
		pic.Clear();
		picbuf.swap(pic);
		m_curPicture = (m_curPicture + 1) & (MAX_BUFFER_COUNT - 1);
		--m_usedPicture;
	} while (m_usedPicture > 0 && m_curpts >= m_picture[m_curPicture].pts);
	assert(m_usedPicture >= 0);
#endif
	OnPlayEvent(KRMovieEvent::Update, nullptr);
}

int VideoPresentLayer::AddVideoPicture(DVDVideoPicture &pic, int index)
{
	// from other thread
	if (pic.format != RENDER_FMT_YUV420P) return -2;
	if (pic.pts == DVD_NOPTS_VALUE) return 0;

	if (m_usedPicture >= MAX_BUFFER_COUNT) {
		std::unique_lock<std::mutex> lk(m_mtxPicture);
		m_condPicture.wait(lk);
	}
	if (m_usedPicture >= MAX_BUFFER_COUNT) return -1;

	int width = pic.iWidth, height = pic.iHeight;

	uint8_t *data = (uint8_t*)TJSAlignedAlloc(width * height * 4, 4);
	int datasize = width * 4;

	img_convert_ctx = sws_getCachedContext(
		img_convert_ctx, width, height, AV_PIX_FMT_YUV420P, width, height,
		AV_PIX_FMT_RGBA, /*sws_flags*/SWS_FAST_BILINEAR, NULL, NULL, NULL);
	assert(img_convert_ctx);
	int processed = sws_scale(img_convert_ctx, pic.data, pic.iLineSize, 0, pic.iHeight, &data, &datasize);

	{
		std::lock_guard<std::mutex> lk(m_mtxPicture);
		BitmapPicture &picbuf = m_picture[(m_curPicture + m_usedPicture) & (MAX_BUFFER_COUNT - 1)];
		picbuf.Clear();
		picbuf.width = width;
		picbuf.height = height;
		picbuf.data[0] = data;
		picbuf.pts = pic.pts / DVD_TIME_BASE;
		++m_usedPicture;
	}

	return MAX_BUFFER_COUNT - m_usedPicture;
}

void MoviePlayerLayer::BuildGraph(tTJSNI_VideoOverlay* callbackwin, IStream *stream, const tjs_char * streamname, const tjs_char *type, uint64_t size)
{
	m_pCallbackWin = callbackwin;
	m_pPlayer->SetCallback(std::bind(&MoviePlayerLayer::OnPlayEvent, this, std::placeholders::_1, std::placeholders::_2));
	m_pPlayer->OpenFromStream(stream, streamname, type, size);
}

void MoviePlayerLayer::OnPlayEvent(KRMovieEvent msg, void *p)
{
	if (msg == KRMovieEvent::Update) {
		NativeEvent ev(WM_GRAPHNOTIFY);
		ev.WParam = EC_UPDATE;
		int frame; GetFrame(&frame);
		ev.LParam = frame;
		m_pCallbackWin->WndProc(ev); // in the same thread
	} else if (msg == KRMovieEvent::Ended) {
		NativeEvent ev(WM_GRAPHNOTIFY);
		ev.WParam = EC_COMPLETE;
		ev.LParam = 0;
		m_pCallbackWin->PostEvent(ev);
	}
}

void MoviePlayerLayer::Play()
{
	inherit::Play();
	TVPAddContinuousEventHook(this);
}

NS_KRMOVIE_END
