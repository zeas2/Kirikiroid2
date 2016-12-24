#pragma once
#include "Thread.h"
#include "IVideoPlayer.h"
#include "AudioCodec.h"
#include "BitstreamStats.h"
#include "MessageQueue.h"
#include "Timer.h"
#include "Clock.h"
#include "AudioDevice.h"

NS_KRMOVIE_BEGIN
class CDVDClock;
class CVideoPlayerAudio : public CThread, public IDVDStreamPlayerAudio
{
public:
	CVideoPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent, CProcessInfo &processInfo);
	virtual ~CVideoPlayerAudio();

	bool OpenStream(CDVDStreamInfo &hints);
	void CloseStream(bool bWaitForBuffers);

	void SetSpeed(int speed);
	void Flush(bool sync);

	// waits until all available data has been rendered
	bool AcceptsData() ;
	bool HasData() const                                  { return m_messageQueue.GetDataSize() > 0; }
	int  GetLevel()                                  { return m_messageQueue.GetLevel(); }
	bool IsInited() const                                 { return m_messageQueue.IsInited(); }
	void SendMessage(CDVDMsg* pMsg, int priority = 0)     { m_messageQueue.Put(pMsg, priority); }
	void FlushMessages()                                  { m_messageQueue.Flush(); }

//	void SetDynamicRangeCompression(long drc)             { m_dvdAudio.SetDynamicRangeCompression(drc); }
	float GetDynamicRangeAmplification() const            { return 0.0f; }


	std::string GetPlayerInfo();
	int GetAudioBitrate();
	int GetAudioChannels();

	// holds stream information for current playing stream
	CDVDStreamInfo m_streaminfo;

	double GetCurrentPts()                            { CSingleLock lock(m_info_section); return m_info.pts; }

	bool IsStalled() const                            { return m_stalled; }
	bool IsPassthrough() override;
	virtual CDVDAudio* GetOutputDevice() override { return &m_dvdAudio; }

protected:

	virtual void OnStartup();
	virtual void OnExit();
	virtual void Process();

	void UpdatePlayerInfo();
	void OpenStream(CDVDStreamInfo &hints, CDVDAudioCodec* codec);
	//! Switch codec if needed. Called when the sample rate gotten from the
	//! codec changes, in which case we may want to switch passthrough on/off.
	bool SwitchCodecIfNeeded();
//	float GetCurrentAttenuation()                         { return m_dvdAudio.GetCurrentAttenuation(); }

	CDVDMessageQueue m_messageQueue;
	CDVDMessageQueue& m_messageParent;

	double m_audioClock;

	CDVDAudio m_dvdAudio; // audio output device
	CDVDClock* m_pClock; // dvd master clock
	CDVDAudioCodec* m_pAudioCodec; // audio codec
	BitstreamStats m_audioStats;

	int m_speed;
	bool m_stalled;
	bool m_silence;
	bool m_paused;
	IDVDStreamPlayer::ESyncState m_syncState;
	Timer m_syncTimer;

	bool OutputPacket(DVDAudioFrame &audioframe);

	//SYNC_DISCON, SYNC_SKIPDUP, SYNC_RESAMPLE
	int    m_synctype;
	int    m_setsynctype;
	int    m_prevsynctype; //so we can print to the log

	void   SetSyncType(bool passthrough);

	bool   m_prevskipped;
	double m_maxspeedadjust;

	struct SInfo
	{
		SInfo()
		: pts(DVD_NOPTS_VALUE)
		, passthrough(false)
		{}

		std::string      info;
		double           pts;
		bool             passthrough;
	};

	CCriticalSection m_info_section;
	SInfo            m_info;
};

NS_KRMOVIE_END