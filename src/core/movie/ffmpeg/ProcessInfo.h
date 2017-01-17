#pragma once
#include "KRMovieDef.h"
#include "Thread.h"
#include "RenderFormats.h"
#include <string>

NS_KRMOVIE_BEGIN
class CProcessInfo
{
public:
	static CProcessInfo* CreateInstance();
	virtual ~CProcessInfo();

	// player video info
	void ResetVideoCodecInfo();
	void SetVideoDecoderName(std::string name, bool isHw);
	std::string GetVideoDecoderName();
	bool IsVideoHwDecoder();
	void SetVideoDeintMethod(std::string method);
	std::string GetVideoDeintMethod();
	void SetVideoPixelFormat(std::string pixFormat);
	std::string GetVideoPixelFormat();
	void SetVideoDimensions(int width, int height);
	void GetVideoDimensions(int &width, int &height);
	void SetVideoFps(float fps);
	float GetVideoFps();
	void SetVideoDAR(float dar);
	float GetVideoDAR();
//	virtual EINTERLACEMETHOD GetFallbackDeintMethod();
//	virtual void SetSwDeinterlacingMethods();
//	void UpdateDeinterlacingMethods(std::list<EINTERLACEMETHOD> &methods);
//	bool Supports(EINTERLACEMETHOD method);
//	void SetDeinterlacingMethodDefault(EINTERLACEMETHOD method);
//	EINTERLACEMETHOD GetDeinterlacingMethodDefault();

	// player audio info
	void ResetAudioCodecInfo();
	void SetAudioDecoderName(std::string name);
	std::string GetAudioDecoderName();
	void SetAudioChannels(std::string channels);
	std::string GetAudioChannels();
	void SetAudioSampleRate(int sampleRate);
	int GetAudioSampleRate();
	void SetAudioBitsPerSample(int bitsPerSample);
	int GetAudioBitsPerSample();
	virtual bool AllowDTSHDDecode();

	// render info
	void SetRenderClockSync(bool enabled);
	bool IsRenderClockSync();
	void UpdateRenderInfo(CRenderInfo &info);

	// player states
	void SetStateSeeking(bool active);
	bool IsSeeking();

protected:
	CProcessInfo();

	// player video info
	bool m_videoIsHWDecoder;
	std::string m_videoDecoderName;
	std::string m_videoDeintMethod;
	std::string m_videoPixelFormat;
	int m_videoWidth;
	int m_videoHeight;
	float m_videoFPS;
	float m_videoDAR;
// 	std::list<EINTERLACEMETHOD> m_deintMethods;
// 	EINTERLACEMETHOD m_deintMethodDefault;
	CCriticalSection m_videoCodecSection;

	// player audio info
	std::string m_audioDecoderName;
	std::string m_audioChannels;
	int m_audioSampleRate;
	int m_audioBitsPerSample;
	CCriticalSection m_audioCodecSection;

	// render info
	CCriticalSection m_renderSection;
	bool m_isClockSync;
	CRenderInfo m_renderInfo;

	// player states
	CCriticalSection m_stateSection;
	bool m_stateSeeking;
};
NS_KRMOVIE_END