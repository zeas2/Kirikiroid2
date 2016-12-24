#pragma once
#include "KRMoviePlayer.h"
#include "EventIntf.h"

NS_KRMOVIE_BEGIN

class VideoPresentLayer : public TVPMoviePlayer, public tTVPContinuousEventCallbackIntf {
protected:
	tTVPBaseTexture *m_BmpBits[2];
	int             m_nCurBmpBuff = 0;

public:
	~VideoPresentLayer();
	virtual tTVPBaseTexture* GetFrontBuffer() override;
	virtual void SetVideoBuffer(tTVPBaseTexture *buff1, tTVPBaseTexture *buff2, long size) override;

	virtual void OnContinuousCallback(tjs_uint64 tick) override;
	virtual void OnPlayEvent(KRMovieEvent msg, void *p) = 0;
	virtual int AddVideoPicture(DVDVideoPicture &pic, int index) override;
};

class MoviePlayerLayer : public VideoPresentLayer {
	typedef VideoPresentLayer inherit;
	tTJSNI_VideoOverlay* m_pCallbackWin = nullptr;

public:
	void BuildGraph(tTJSNI_VideoOverlay* callbackwin, IStream *stream, const tjs_char * streamname, const tjs_char *type, uint64_t size);
	virtual void OnPlayEvent(KRMovieEvent msg, void *p) override;
	virtual void Play() override;
};

NS_KRMOVIE_END