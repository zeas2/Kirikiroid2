#include <thread>
extern "C" {
#include "libswscale/swscale.h"
}
#include "cocos2d.h"
#include "KRMoviePlayer.h"
#include "VideoCodec.h"
#include "CodecUtils.h"
#include "AudioDevice.h"
#include "WaveMixer.h"
#include "WindowImpl.h"
#include "VideoOvlImpl.h"
#include "cocos2d/YUVSprite.h"

extern std::thread::id TVPMainThreadID;

NS_KRMOVIE_BEGIN

TVPMoviePlayer::TVPMoviePlayer()
{
	m_pPlayer = new BasePlayer(this);
}

TVPMoviePlayer::~TVPMoviePlayer()
{
	delete m_pPlayer;
	if (img_convert_ctx) sws_freeContext(img_convert_ctx), img_convert_ctx = nullptr;
}

void TVPMoviePlayer::Release()
{
	if (RefCount == 1)
		delete this;
	else
		RefCount--;
}

void TVPMoviePlayer::SetPosition(uint64_t tick)
{
	m_pPlayer->SeekTime(tick);
}

void TVPMoviePlayer::GetPosition(uint64_t *tick)
{
	if (tick) *tick = m_pPlayer->GetTime();
}

void TVPMoviePlayer::GetStatus(tTVPVideoStatus *status)
{
	if (m_pPlayer->IsStop()) *status = vsStopped;
	else if (m_pPlayer->GetSpeed() == 0) *status = vsPaused;
	else *status = vsPlaying;
	//	else *status = vsProcessing;
}

void TVPMoviePlayer::Rewind()
{
	SetPosition(0);
}

void TVPMoviePlayer::SetFrame(int f)
{
	// TODO seek accurately
	m_pPlayer->SeekTime(f / m_pPlayer->GetFPS() * DVD_PLAYSPEED_NORMAL);
}

void TVPMoviePlayer::GetFrame(int *f)
{
	*f = m_pPlayer->GetCurrentFrame();
}

void TVPMoviePlayer::GetFPS(double *f)
{
	*f = m_pPlayer->GetFPS();
}

void TVPMoviePlayer::GetNumberOfFrame(int *f)
{
	*f = m_pPlayer->GetTotalTime() * m_pPlayer->GetFPS() / DVD_PLAYSPEED_NORMAL;
}

void TVPMoviePlayer::GetTotalTime(int64_t *t)
{
	*t = m_pPlayer->GetTotalTime();
}

void TVPMoviePlayer::GetVideoSize(long *width, long *height)
{
	m_pPlayer->GetVideoSize(width, height);
}

void TVPMoviePlayer::SetPlayRate(double rate) {
	m_pPlayer->SetSpeed(rate);
}

void TVPMoviePlayer::GetPlayRate(double *rate) {
	*rate = m_pPlayer->GetSpeed();
}

iTVPSoundBuffer* TVPMoviePlayer::GetSoundDevice() {
	IDVDStreamPlayerAudio *audioplayer = m_pPlayer->GetAudioPlayer();
	if (!audioplayer) return nullptr;
	IAEStream *audiostream = audioplayer->GetOutputDevice()->m_pAudioStream;
	if (!audiostream) return nullptr;
	return audiostream->GetNativeImpl();
}

void TVPMoviePlayer::GetAudioBalance(long *balance)
{
	iTVPSoundBuffer* alsound = GetSoundDevice();
	if (alsound) {
		*balance = alsound->GetPan() * 100000;
	}
}

void TVPMoviePlayer::SetAudioBalance(long balance)
{
	iTVPSoundBuffer* alsound = GetSoundDevice();
	if (alsound) {
		alsound->SetPan(balance / 100000.0f);
	}
}

void TVPMoviePlayer::SetAudioVolume(long volume)
{
	iTVPSoundBuffer* alsound = GetSoundDevice();
	if (alsound) alsound->SetVolume(volume / 100000.f);
}

void TVPMoviePlayer::GetAudioVolume(long *volume)
{
	iTVPSoundBuffer* alsound = GetSoundDevice();
	if (alsound) *volume = alsound->GetVolume() * 100000;
}

void TVPMoviePlayer::GetNumberOfAudioStream(unsigned long *streamCount) {
	*streamCount = m_pPlayer->GetAudioStreamCount();
}

void TVPMoviePlayer::SelectAudioStream(unsigned long iStream) {
	m_pPlayer->GetMessageQueue().Put(new CDVDMsgPlayerSetAudioStream(iStream));
	m_pPlayer->SynchronizeDemuxer();
}

void TVPMoviePlayer::GetEnableAudioStreamNum(long *num) {
	*num = m_pPlayer->GetAudioStream();
}

void TVPMoviePlayer::DisableAudioStream(void) {
	// TODO
}

void TVPMoviePlayer::GetNumberOfVideoStream(unsigned long *streamCount) {
	*streamCount = m_pPlayer->GetVideoStreamCount();
}

void TVPMoviePlayer::SelectVideoStream(unsigned long iStream) {
	m_pPlayer->GetMessageQueue().Put(new CDVDMsgPlayerSetVideoStream(iStream));
	m_pPlayer->SynchronizeDemuxer();
}

void TVPMoviePlayer::GetEnableVideoStreamNum(long *num) {
	*num = m_pPlayer->GetVideoStream();
}

int TVPMoviePlayer::WaitForBuffer(volatile std::atomic_bool&bStop, int timeout)
{
	int remainBuf = MAX_BUFFER_COUNT - m_usedPicture;
	if (remainBuf > 0) return remainBuf;
	std::unique_lock<std::mutex> lk(m_mtxPicture);
	while (!bStop && MAX_BUFFER_COUNT <= m_usedPicture && timeout > 0) {
		timeout -= 10;
		m_condPicture.wait_for(lk, std::chrono::milliseconds(10));
	}
	return MAX_BUFFER_COUNT - m_usedPicture - 1;
}

void TVPMoviePlayer::Flush()
{
	std::unique_lock<std::mutex> lk(m_mtxPicture);
	for (int i = 0; i < MAX_BUFFER_COUNT; ++i) {
		m_picture[i].Clear();
	}
	m_curpts = 0.0;
	m_usedPicture = 0;
}

void TVPMoviePlayer::FrameMove()
{
	m_pPlayer->FrameMove();
}

void TVPMoviePlayer::SetLoopSegement(int beginFrame, int endFrame)
{
	m_pPlayer->SetLoopSegement(beginFrame, endFrame);
}

int TVPMoviePlayer::AddVideoPicture(DVDVideoPicture &pic, int index)
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
	// YUV data passthrough
	int yuvwidth[3] = { width, width / 2, width / 2 };
	int yuvheight[3] = { height, height / 2, height / 2 };
	uint8_t *yuvdata[3] = { nullptr };
	for (int i = 0; i < sizeof(yuvdata) / sizeof(yuvdata[0]); ++i) {
		int size = yuvwidth[i] * yuvheight[i];
		yuvdata[i] = (uint8_t*)TJSAlignedAlloc(size, 4);
		if (yuvwidth[i] == pic.iLineSize[i]) {
			memcpy(yuvdata[i], pic.data[i], size);
		} else {
			uint8_t *d = yuvdata[i], *s = pic.data[i];
			for (int y = 0; y < yuvheight[i]; ++y) {
				memcpy(d, s, yuvwidth[i]);
				d += yuvwidth[i];
				s += pic.iLineSize[i];
			}
		}
	}

	{
		std::lock_guard<std::mutex> lk(m_mtxPicture);
		BitmapPicture &picbuf = m_picture[(m_curPicture + m_usedPicture) & (MAX_BUFFER_COUNT - 1)];
		picbuf.Clear();
		picbuf.width = width;
		picbuf.height = height;
		for (int i = 0; i < sizeof(yuvdata) / sizeof(yuvdata[0]); ++i) {
			picbuf.yuv[i] = yuvdata[i];
		}
		picbuf.pts = pic.pts / DVD_TIME_BASE;
		++m_usedPicture;
		return MAX_BUFFER_COUNT - m_usedPicture;
	}

	// 	const static std::string sckey("present");
	// 	m_pRootNode->scheduleOnce(std::bind(&PlayerOverlay::PresentPicture, this, std::placeholders::_1), 0, sckey);
}


VideoPresentOverlay::~VideoPresentOverlay()
{
	ClearNode();
}

void VideoPresentOverlay::ClearNode()
{
	if (m_pRootNode) {
		m_pRootNode->removeFromParent(), m_pRootNode = nullptr;
		m_pSprite = nullptr;
	}
}

void VideoPresentOverlay::PresentPicture(float dt)
{
	BitmapPicture pic;
	if (!m_usedPicture) {
		return;
	} else {
		std::lock_guard<std::mutex> lk(m_mtxPicture);
		BitmapPicture &picbuf = m_picture[m_curPicture];
		// check pts
		if (m_curpts == 0.0) {
			m_curpts = picbuf.pts;
		} else {
			m_curpts += dt;
			if (picbuf.pts > m_curpts) { // present in future
				return;
			}
		}
		do { // skip frame
			pic.Clear();
			m_picture[m_curPicture].swap(pic);
			m_curPicture = (m_curPicture + 1) & (MAX_BUFFER_COUNT - 1);
			--m_usedPicture;
		} while (m_usedPicture > 0 && m_curpts >= m_picture[m_curPicture].pts);
		assert(m_usedPicture >= 0);
		m_condPicture.notify_all();
	}
	FrameMove();
	if (!pic.rgba) {
		return;
	}
	if (!Visible) {
		m_pRootNode->setVisible(false);
		return;
	} else {
		m_pRootNode->setVisible(true);
	}
	if (!m_pSprite) {
		m_pSprite = TVPYUVSprite::create();
		m_pSprite->setAnchorPoint(cocos2d::Vec2(0, 1));
		m_pRootNode->addChild(m_pSprite);
	}
	cocos2d::Size videoSize(pic.width, pic.height);
	m_pSprite->updateTextureData(
		pic.data[0], pic.width, pic.height,
		pic.data[1], pic.width / 2, pic.height / 2,
		pic.data[2], pic.width / 2, pic.height / 2
		);
	const tTVPRect & rc = GetBounds();
	float scaleX = rc.get_width() / videoSize.width;
	float scaleY = rc.get_height() / videoSize.height;
	if (scaleX != m_pSprite->getScaleX())
		m_pSprite->setScaleX(scaleX);
	if (scaleY != m_pSprite->getScaleY())
		m_pSprite->setScaleY(scaleY);
	cocos2d::Vec2 pos = m_pSprite->getPosition();
	int top = m_pRootNode->getParent()->getContentSize().height - rc.top;
	if ((int)pos.x != rc.left || (int)pos.y != top) {
		m_pSprite->setPosition(rc.left, top);
	}
}

void KRMovie::VideoPresentOverlay::Play()
{
	if (m_pSprite) m_pSprite->setVisible(true);
	TVPMoviePlayer::Play();
}

void KRMovie::VideoPresentOverlay::Stop()
{
	if (m_pSprite) m_pSprite->setVisible(false);
	TVPMoviePlayer::Stop();
}

MoviePlayerOverlay::~MoviePlayerOverlay()
{
	assert(std::this_thread::get_id() == TVPMainThreadID);
	delete m_pPlayer; m_pPlayer = nullptr;
}

void MoviePlayerOverlay::SetWindow(tTJSNI_Window* window)
{
	ClearNode();
	m_pOwnerWindow = window;
	cocos2d::Node *parent = m_pOwnerWindow->GetForm()->GetPrimaryArea();
	parent->addChild((m_pRootNode = cocos2d::Node::create()));
	m_pRootNode->setContentSize(cocos2d::Size::ZERO);
	const static std::string sckey("update video");
	m_pRootNode->schedule([this](float dt){
		PresentPicture(dt);
		//		m_renderManager.Render();
	}, sckey);
}

void MoviePlayerOverlay::BuildGraph(tTJSNI_VideoOverlay* callbackwin, IStream *stream, const tjs_char * streamname, const tjs_char *type, uint64_t size)
{
	m_pCallbackWin = callbackwin;
	m_pPlayer->SetCallback(std::bind(&MoviePlayerOverlay::OnPlayEvent, this, std::placeholders::_1, std::placeholders::_2));
	m_pPlayer->OpenFromStream(stream, streamname, type, size);
}

const tTVPRect & MoviePlayerOverlay::GetBounds()
{
	return m_pCallbackWin->GetBounds();
}

void KRMovie::MoviePlayerOverlay::SetVisible(bool b)
{
	VideoPresentOverlay::SetVisible(b);
	if (m_pRootNode) {
		m_pRootNode->setVisible(b);
	}
}

void MoviePlayerOverlay::OnPlayEvent(KRMovieEvent msg, void *p)
{
	if (msg == KRMovieEvent::Ended) {
		NativeEvent ev(WM_GRAPHNOTIFY);
		ev.WParam = EC_COMPLETE;
		ev.LParam = 0;
		m_pCallbackWin->PostEvent(ev);
	}
}

void VideoPresentOverlay::BitmapPicture::swap(BitmapPicture &r)
{
	std::swap(data, r.data);
	std::swap(width, r.width);
	std::swap(height, r.height);
}

void TVPMoviePlayer::BitmapPicture::Clear()
{
	for (int i = 0; i < sizeof(data) / sizeof(data[0]); ++i)
		if (data[i]) TJSAlignedDealloc(data[i]), data[i] = nullptr;
}

void VideoPresentOverlay2::SetRootNode(cocos2d::Node *node)
{
	ClearNode();
	m_pRootNode = node;
}

VideoPresentOverlay2 * VideoPresentOverlay2::create()
{
	return new VideoPresentOverlay2;
}

NS_KRMOVIE_END
