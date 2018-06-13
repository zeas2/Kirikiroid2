#pragma once
#define NOMINMAX
#include "VideoPlayer.h"
#include "krmovie.h"
#include "ComplexRect.h"
struct SwsContext;
class iTVPSoundBuffer;
class TVPYUVSprite;

namespace cocos2d {
	class Sprite;
	class Node;
}

NS_KRMOVIE_BEGIN
#define MAX_BUFFER_COUNT 4

class TVPMoviePlayer : public iTVPVideoOverlay, public CBaseRenderer {
public:
	~TVPMoviePlayer();
	virtual void AddRef() override { RefCount++; }
	virtual void Release() override;

	virtual void SetVisible(bool b) override { Visible = b; }
	virtual void Play() override { m_pPlayer->Play(); }
	virtual void Stop() override { m_pPlayer->Stop(); }
	virtual void Pause() override { m_pPlayer->Pause(); }
	virtual void SetPosition(uint64_t tick) override;
	virtual void GetPosition(uint64_t *tick) override;
	virtual void GetStatus(tTVPVideoStatus *status) override;

	virtual void Rewind() override;
	virtual void SetFrame(int f) override;
	virtual void GetFrame(int *f) override;
	virtual void GetFPS(double *f) override;
	virtual void GetNumberOfFrame(int *f) override;
	virtual void GetTotalTime(int64_t *t) override;

	virtual void GetVideoSize(long *width, long *height) override;

	virtual void SetPlayRate(double rate) override;
	virtual void GetPlayRate(double *rate) override;

	virtual void SetAudioBalance(long balance) override;
	virtual void GetAudioBalance(long *balance) override;
	virtual void SetAudioVolume(long volume) override;
	virtual void GetAudioVolume(long *volume) override;

	virtual void GetNumberOfAudioStream(unsigned long *streamCount) override;
	virtual void SelectAudioStream(unsigned long num) override;
	virtual void GetEnableAudioStreamNum(long *num) override;
	virtual void DisableAudioStream(void) override;

	virtual void GetNumberOfVideoStream(unsigned long *streamCount) override;
	virtual void SelectVideoStream(unsigned long num) override;
	virtual void GetEnableVideoStreamNum(long *num) override;

	// TODO
	virtual void SetStopFrame(int frame) override {}
	virtual void GetStopFrame(int *frame) override {}
	virtual void SetDefaultStopFrame() override {}

	// function for overlay mode
	virtual void SetWindow(class tTJSNI_Window* window) override {}
	virtual void SetMessageDrainWindow(void* window) override {}
	virtual void SetRect(int l, int t, int r, int b) override {}

	// function for layer mode
	virtual tTVPBaseTexture* GetFrontBuffer() override { return nullptr; }
	virtual void SetVideoBuffer(tTVPBaseTexture *buff1, tTVPBaseTexture *buff2, long size) override {}

	// function for mixer mode
	virtual void SetMixingBitmap(class tTVPBaseTexture *dest, float alpha) override {}
	virtual void ResetMixingBitmap() override {}

	virtual void SetMixingMovieAlpha(float a) override {}
	virtual void GetMixingMovieAlpha(float *a) override { *a = 1.0f; }
	virtual void SetMixingMovieBGColor(unsigned long col) override {}
	virtual void GetMixingMovieBGColor(unsigned long *col) override { *col = 0xFF000000; }

	virtual void PresentVideoImage() override {}

	virtual void GetContrastRangeMin(float *v) override {}
	virtual void GetContrastRangeMax(float *v) override {}
	virtual void GetContrastDefaultValue(float *v) override {}
	virtual void GetContrastStepSize(float *v) override {}
	virtual void GetContrast(float *v) override {}
	virtual void SetContrast(float v) override {}

	virtual void GetBrightnessRangeMin(float *v) override {}
	virtual void GetBrightnessRangeMax(float *v) override {}
	virtual void GetBrightnessDefaultValue(float *v) override {}
	virtual void GetBrightnessStepSize(float *v) override {}
	virtual void GetBrightness(float *v) override {}
	virtual void SetBrightness(float v) override {}

	virtual void GetHueRangeMin(float *v) override {}
	virtual void GetHueRangeMax(float *v) override {}
	virtual void GetHueDefaultValue(float *v) override {}
	virtual void GetHueStepSize(float *v) override {}
	virtual void GetHue(float *v) override {}
	virtual void SetHue(float v) override {}

	virtual void GetSaturationRangeMin(float *v) override {}
	virtual void GetSaturationRangeMax(float *v) override {}
	virtual void GetSaturationDefaultValue(float *v) override {}
	virtual void GetSaturationStepSize(float *v) override {}
	virtual void GetSaturation(float *v) override {}
	virtual void SetSaturation(float v) override {}

	virtual void SetLoopSegement(int beginFrame, int endFrame) override;

	virtual int AddVideoPicture(DVDVideoPicture &pic, int index) override;
	virtual int WaitForBuffer(volatile std::atomic_bool&bStop, int timeout = 0) override;
	virtual void Flush() override;

	bool IsPlaying() const { return m_pPlayer->IsPlaying(); }
	void FrameMove();

protected:
	TVPMoviePlayer();
	iTVPSoundBuffer* GetSoundDevice();

	uint32_t	RefCount = 1;
	bool		Visible = false;

	BasePlayer *m_pPlayer = nullptr;

	struct BitmapPicture {
		ERenderFormat fmt;
		union {
			uint8_t *data[4];
			uint8_t *rgba;
			uint8_t *yuv[3];
		};
		int width = 0; // pitch = width * 4
		int height = 0;
		double pts;
		BitmapPicture() {
			fmt = RENDER_FMT_NONE;
			for (int i = 0; i < sizeof(data) / sizeof(data[0]); ++i)
				data[i] = nullptr;
		}
		~BitmapPicture() { Clear(); }
		void swap(BitmapPicture &r);
		void Clear();
	};
	BitmapPicture m_picture[MAX_BUFFER_COUNT];
	int m_curPicture = 0, m_usedPicture = 0;
	std::mutex m_mtxPicture;
	std::condition_variable m_condPicture;
	struct SwsContext *img_convert_ctx = nullptr;
	double m_curpts = 0;
};

class VideoPresentOverlay : public TVPMoviePlayer // cocos2d compatible video display overlay
{
protected:
	cocos2d::Node *m_pRootNode = nullptr;
	TVPYUVSprite *m_pSprite = nullptr;

	~VideoPresentOverlay();
	void ClearNode();
public:
	void PresentPicture(float dt);
	virtual void Stop() override;
	virtual void Play() override;

protected:
	virtual const tTVPRect &GetBounds() = 0;
};

class MoviePlayerOverlay : public VideoPresentOverlay {
	tTJSNI_VideoOverlay* m_pCallbackWin = nullptr;
	tTJSNI_Window *m_pOwnerWindow = nullptr;

	void OnPlayEvent(KRMovieEvent msg, void *p);

public:
	~MoviePlayerOverlay();
	virtual void SetWindow(class tTJSNI_Window* window) override;

	void BuildGraph(tTJSNI_VideoOverlay* callbackwin, IStream *stream,
		const tjs_char * streamname, const tjs_char *type, uint64_t size);

	virtual const tTVPRect &GetBounds() override;
	virtual void SetVisible(bool b) override;
};

class VideoPresentOverlay2 : public VideoPresentOverlay {
	std::function<const tTVPRect &()> m_funcGetBounds;
public:
	virtual const tTVPRect &GetBounds() override { return m_funcGetBounds(); }
	void SetFuncGetBounds(const std::function<const tTVPRect &()>& func) { m_funcGetBounds = func; }
	BasePlayer *GetPlayer() { return m_pPlayer; }
	void SetRootNode(cocos2d::Node *node);
	cocos2d::Node *GetRootNode() { return m_pRootNode; }
	static VideoPresentOverlay2 *create();
};

NS_KRMOVIE_END
