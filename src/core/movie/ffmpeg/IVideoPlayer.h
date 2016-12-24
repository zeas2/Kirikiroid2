#pragma once
#include "KRMovieDef.h"
#include "ProcessInfo.h"
#include "StreamInfo.h"
#include "Message.h"
#include <string>

NS_KRMOVIE_BEGIN

#define VideoPlayer_AUDIO    1
#define VideoPlayer_VIDEO    2
// #define VideoPlayer_SUBTITLE 3
// #define VideoPlayer_TELETEXT 4
// #define VideoPlayer_RDS      5

struct SStartMsg
{
	double timestamp;
	int player;
	double cachetime;
	double cachetotal;
};

class IDVDStreamPlayer
{
public:
	IDVDStreamPlayer(CProcessInfo &processInfo) : m_processInfo(processInfo) {};
	virtual ~IDVDStreamPlayer() {}
	virtual bool OpenStream(CDVDStreamInfo &hint) = 0;
	virtual void CloseStream(bool bWaitForBuffers) = 0;
	virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
	virtual void FlushMessages() = 0;
	virtual bool IsInited() const = 0;
	virtual bool AcceptsData() = 0;
	virtual bool IsStalled() const = 0;

	enum ESyncState
	{
		SYNC_STARTING,
		SYNC_WAITSYNC,
		SYNC_INSYNC
	};
protected:
	CProcessInfo &m_processInfo;
};

class CDVDVideoCodec;

class IDVDStreamPlayerVideo : public IDVDStreamPlayer
{
public:
	IDVDStreamPlayerVideo(CProcessInfo &processInfo) : IDVDStreamPlayer(processInfo) {};
	~IDVDStreamPlayerVideo() {}
	virtual bool OpenStream(CDVDStreamInfo &hint) = 0;
	virtual void CloseStream(bool bWaitForBuffers) = 0;
	virtual void Flush(bool sync) = 0;
	virtual bool AcceptsData() = 0;
	virtual bool HasData() const = 0;
	virtual int  GetLevel() = 0;
	virtual bool IsInited() const = 0;
	virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
	virtual void EnableSubtitle(bool bEnable) = 0;
	virtual bool IsSubtitleEnabled() = 0;
//	virtual void EnableFullscreen(bool bEnable) = 0;
	virtual double GetSubtitleDelay() = 0;
	virtual void SetSubtitleDelay(double delay) = 0;
	virtual bool IsStalled() const = 0;
	virtual bool IsRewindStalled() const { return false; }
	virtual double GetCurrentPts() = 0;
	virtual double GetOutputDelay() = 0;
	virtual std::string GetPlayerInfo() = 0;
	virtual int GetVideoBitrate() = 0;
	virtual std::string GetStereoMode() = 0;
	virtual void SetSpeed(int iSpeed) = 0;
	virtual int  GetDecoderBufferSize() { return 0; }
	virtual int  GetDecoderFreeSpace() = 0;
	virtual bool IsEOS() { return false; };
};

class CDVDAudioCodec;
class IDVDStreamPlayerAudio : public IDVDStreamPlayer
{
public:
	IDVDStreamPlayerAudio(CProcessInfo &processInfo) : IDVDStreamPlayer(processInfo) {};
	~IDVDStreamPlayerAudio() {}
	virtual bool OpenStream(CDVDStreamInfo &hints) = 0;
	virtual void CloseStream(bool bWaitForBuffers) = 0;
	virtual void SetSpeed(int speed) = 0;
	virtual void Flush(bool sync) = 0;
	virtual bool AcceptsData() = 0;
	virtual bool HasData() const = 0;
	virtual int  GetLevel() = 0;
	virtual bool IsInited() const = 0;
	virtual void SendMessage(CDVDMsg* pMsg, int priority = 0) = 0;
	virtual void SetVolume(float fVolume) {};
	virtual void SetMute(bool bOnOff) {};
//	virtual void SetDynamicRangeCompression(long drc) = 0;
	virtual std::string GetPlayerInfo() = 0;
	virtual int GetAudioBitrate() = 0;
	virtual int GetAudioChannels() = 0;
	virtual double GetCurrentPts() = 0;
	virtual bool IsStalled() const = 0;
	virtual bool IsPassthrough() = 0;
	virtual float GetDynamicRangeAmplification() const = 0;
	virtual bool IsEOS() { return false; };
	virtual class CDVDAudio* GetOutputDevice() = 0;
};

NS_KRMOVIE_END