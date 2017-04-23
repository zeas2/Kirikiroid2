#pragma once
#include "Clock.h"
#include "Thread.h"
#include "StreamInfo.h"
#include "Demux.h"
#include "Timer.h"
#include "Geometry.h"
#include "MessageQueue.h"
#include "ProcessInfo.h"
#include "IVideoPlayer.h"
#include <mutex>
#include <vector>
#include <memory>
#include "VideoRenderer.h"

#ifdef ANDROID
//#define HAS_OMXPLAYER 1
#endif
#ifdef HAS_OMXPLAYER
#include "../omxplayer/OMXCore.h"
#include "../omxplayer/OMXClock.h"
#else

// dummy class to avoid ifdefs where calls are made
class OMXClock
{
public:
	bool OMXInitialize(KRMovie::CDVDClock *clock) { return false; }
	void OMXDeinitialize() {}
	bool OMXIsPaused() { return false; }
	bool OMXStop(bool lock = true) { return false; }
	bool OMXStep(int steps = 1, bool lock = true) { return false; }
	bool OMXReset(bool has_video, bool has_audio, bool lock = true) { return false; }
	double OMXMediaTime(bool lock = true) { return 0.0; }
	double OMXClockAdjustment(bool lock = true) { return 0.0; }
	bool OMXMediaTime(double pts, bool lock = true) { return false; }
	bool OMXPause(bool lock = true) { return false; }
	bool OMXResume(bool lock = true) { return false; }
	bool OMXSetSpeed(int speed, bool lock = true, bool pause_resume = false) { return false; }
	bool OMXFlush(bool lock = true) { return false; }
	bool OMXStateExecute(bool lock = true) { return false; }
	void OMXStateIdle(bool lock = true) {}
	bool HDMIClockSync(bool lock = true) { return false; }
	void OMXSetSpeedAdjust(double adjust, bool lock = true) {}
};
#endif

struct SOmxPlayerState
{
	OMXClock av_clock;              // openmax clock component
//	EINTERLACEMETHOD interlace_method; // current deinterlace method
	bool bOmxWaitVideo;             // whether we need to wait for video to play out on EOS
	bool bOmxWaitAudio;             // whether we need to wait for audio to play out on EOS
	bool bOmxSentEOFs;              // flag if we've send EOFs to audio/video players
	float threshold;                // current fifo threshold required to come out of buffering
	unsigned int last_check_time;   // we periodically check for gpu underrun
	double stamp;                   // last media timestamp
};

class tTJSNI_Window;
class tTJSNI_VideoOverlay;
struct IStream;

enum class KRMovieEvent {
	None,
	Aborted,
	Stopped,
	Ended,
	Update,
};

NS_KRMOVIE_BEGIN

struct CCurrentStream
{
	int64_t demuxerId; // demuxer's id of current playing stream
	int id;     // id of current playing stream
	int source;
	double dts;    // last dts from demuxer, used to find disncontinuities
	double dur;    // last frame expected duration
	int dispTime; // display time from input stream
	CDVDStreamInfo hint;   // stream hints, used to notice stream changes
	void* stream; // pointer or integer, identifying stream playing. if it changes stream changed
	int changes; // remembered counter from stream to track codec changes
	bool inited;
	unsigned int packets;
	IDVDStreamPlayer::ESyncState syncState;
	double starttime;
	double cachetime;
	double cachetotal;
	const StreamType type;
	const int player;
	// stuff to handle starting after seek
	double startpts;
	double lastdts;

	enum
	{
		AV_SYNC_NONE,
		AV_SYNC_CHECK,
		AV_SYNC_CONT,
		AV_SYNC_FORCE
	} avsync;

	CCurrentStream(StreamType t, int i)
		: type(t)
		, player(i)
	{
		Clear();
	}

	void Clear()
	{
		id = -1;
		demuxerId = -1;
		source = STREAM_SOURCE_NONE;
		dts = DVD_NOPTS_VALUE;
		dur = DVD_NOPTS_VALUE;
		hint.Clear();
		stream = NULL;
		changes = 0;
		inited = false;
		packets = 0;
		syncState = IDVDStreamPlayer::SYNC_STARTING;
		starttime = DVD_NOPTS_VALUE;
		startpts = DVD_NOPTS_VALUE;
		lastdts = DVD_NOPTS_VALUE;
		avsync = AV_SYNC_FORCE;
	}

	double dts_end()
	{
		if (dts == DVD_NOPTS_VALUE)
			return DVD_NOPTS_VALUE;
		if (dur == DVD_NOPTS_VALUE)
			return dts;
		return dts + dur;
	}
};

struct SPlayerState
{
	SPlayerState() { Clear(); }
	void Clear()
	{
		timestamp = 0;
		time = 0;
		time_total = 0;
		time_offset = 0;
		dts = DVD_NOPTS_VALUE;
		player_state = "";
		isInMenu = false;
		hasMenu = false;
		canrecord = false;
		recording = false;
		canpause = false;
		canseek = false;
		caching = false;
		cache_bytes = 0;
		cache_level = 0.0;
		cache_delay = 0.0;
		cache_offset = 0.0;
		lastSeek = 0;
	}

	double timestamp;         // last time of update
	double lastSeek;          // time of last seek
	double time_offset;       // difference between time and pts

	double time;              // current playback time
	double time_total;        // total playback time
	double dts;               // last known dts

	std::string player_state; // full player state
	bool isInMenu;
	bool hasMenu;

	bool canrecord;           // can input stream record
	bool recording;           // are we currently recording
	bool canpause;            // pvr: can pause the current playing item
	bool canseek;             // pvr: can seek in the current playing item
	bool caching;

	int64_t cache_bytes;   // number of bytes current's cached
	double cache_level;   // current estimated required cache level
	double cache_delay;   // time until cache is expected to reach estimated level
	double cache_offset;  // percentage of file ahead of current position
};

struct SelectionStream {
	StreamType   type = STREAM_NONE;
	int          type_index = 0;
	std::string  filename;
//	std::string  filename2;  // for vobsub subtitles, 2 files are necessary (idx/sub)
//	std::string  language;
	std::string  name;
	CDemuxStream::EFlags flags = CDemuxStream::FLAG_NONE;
	int          source = 0;
	int          id = 0;
	int64_t      demuxerId = -1;
	std::string  codec;
	int          channels = 0;
	int          bitrate = 0;
	int          width = 0;
	int          height = 0;
	CRect        SrcRect;
	CRect        DestRect;
	std::string  stereo_mode;
	float        aspect_ratio = 0.0f;
};

typedef std::vector<SelectionStream> SelectionStreams;

class IDemux;
class BasePlayer;
class InputStream;
class CSelectionStreams
{
	SelectionStream  m_invalid;
public:
	CSelectionStreams()
	{
		m_invalid.id = -1;
		m_invalid.source = STREAM_SOURCE_NONE;
		m_invalid.type = STREAM_NONE;
	}
	std::vector<SelectionStream> m_Streams;
	std::recursive_mutex m_section;

	int              IndexOf(StreamType type, int source, int64_t demuxerId, int id);
//	int              IndexOf(StreamType type, const Player& p);
	int              Count(StreamType type) { return IndexOf(type, STREAM_SOURCE_NONE, -1, -1) + 1; }
	int              CountSource(StreamType type, StreamSource source) const;
	SelectionStream& Get(StreamType type, int index);
	bool             Get(StreamType type, CDemuxStream::EFlags flag, SelectionStream& out);

	SelectionStreams Get(StreamType type);
	template<typename Compare> SelectionStreams Get(StreamType type, Compare compare)
	{
		SelectionStreams streams = Get(type);
		std::stable_sort(streams.begin(), streams.end(), compare);
		return streams;
	}

	void             Clear(StreamType type, StreamSource source);
	int              Source(StreamSource source, const std::string & filename);

	void             Update(SelectionStream& s);
	void             Update(InputStream* input, IDemux* demuxer, const std::string & filename2 = "");
};

class IDVDStreamPlayerVideo;
class IDVDStreamPlayerAudio;
class BasePlayer : public CThread, public IRenderMsg
{
public:
	void Play();
	void Stop();
	void Pause();

	// IRenderMsg
	virtual void VideoParamsChange() override;
	virtual void GetDebugInfo(std::string &audio, std::string &video, std::string &general) override;
	virtual void UpdateClockSync(bool enabled) override;
	virtual void UpdateRenderInfo(CRenderInfo &info) override;
	virtual CBaseRenderer* CreateRenderer() override { return m_pRenderer; }

public:
	BasePlayer(CBaseRenderer *renderer);
	virtual ~BasePlayer();
	bool OpenFromStream(IStream *stream, const tjs_char * streamname, const tjs_char *type, uint64_t size);
	bool CloseInputStream();
	bool IsPlaying() const;
	bool CanSeek();

	virtual void OnPlayBackSeek(int iTime, int iOffset) {}

//	virtual void OnStartup() {}
	virtual void OnExit();
	virtual void Process();

	void CreatePlayers();
	void DestroyPlayers();

	bool OpenStream(CCurrentStream& current, int64_t demuxerId, int iStream, int source, bool reset = true);
	bool OpenAudioStream(CDVDStreamInfo& hint, bool reset = true);
	bool OpenVideoStream(CDVDStreamInfo& hint, bool reset = true);

	bool CloseStream(CCurrentStream& current, bool bWaitForBuffers);

	bool CheckIsCurrent(CCurrentStream& current, CDemuxStream* stream, DemuxPacket* pkg);
	void ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket);
	void ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket);
	void ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket);

	void SetPlaySpeed(int iSpeed);
	int GetPlaySpeed() { return m_playSpeed; }

	enum ECacheState
	{
		CACHESTATE_DONE = 0,
		CACHESTATE_FULL,     // player is filling up the demux queue
		CACHESTATE_INIT,     // player is waiting for first packet of each stream
		CACHESTATE_PLAY,     // player is waiting for players to not be stalled
		CACHESTATE_FLUSH,    // temporary state player will choose startup between init or full
	};
	void SetCaching(ECacheState state);
	double GetQueueTime();
	bool GetCachingTimes(double& play_left, double& cache_left, double& file_offset);

	void SetSpeed(double speed);
	double GetSpeed();
	void FrameMove() { m_renderManager.FrameMove(); }
	bool IsStopped() { return m_bStop; }
	void SeekTime(int64_t iTimeMS);
	int64_t GetTime();
	int GetCurrentFrame();
	int GetVideoStreamCount();
	int GetVideoStream();
	int GetAudioStreamCount();
	int GetAudioStream();

	void FlushBuffers(bool queued, double pts = DVD_NOPTS_VALUE, bool accurate = true, bool sync = true);

	void HandleMessages();
	void HandlePlaySpeed();
	void SynchronizeDemuxer();
	bool CheckContinuity(CCurrentStream& current, DemuxPacket* pPacket);
	bool CheckSceneSkip(CCurrentStream& current);
	bool CheckPlayerInit(CCurrentStream& current);
	void UpdateCorrection(DemuxPacket* pkt, double correction);
	void UpdateTimestamps(CCurrentStream& current, DemuxPacket* pPacket);
	IDVDStreamPlayer* GetStreamPlayer(unsigned int player);
	void SendPlayerMessage(CDVDMsg* pMsg, unsigned int target);

	bool ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream);
	bool IsValidStream(CCurrentStream& stream);
	bool IsBetterStream(CCurrentStream& current, CDemuxStream* stream);
	void CheckBetterStream(CCurrentStream& current, CDemuxStream* stream);
	void CheckStreamChanges(CCurrentStream& current, CDemuxStream* stream);
	//	bool CheckDelayedChannelEntry(void);

	void OpenInputStream(IStream *stream, const std::string& filename);
	bool OpenDemuxStream();
	void CloseDemuxer();
	void OpenDefaultStreams(bool reset = true);

	void UpdatePlayState(double timeout);
	void UpdateStreamInfos();

	bool IsStop() { return m_bStopStatus; }
	double GetFPS() { return (double)m_CurrentVideo.hint.fpsrate / m_CurrentVideo.hint.fpsscale; }
	int64_t GetTotalTime() { return llrint(m_State.time_total); }
	void GetVideoSize(long *width, long *height);
	CDVDMessageQueue& GetMessageQueue() { return m_messenger; }
	void SetCallback(const std::function<void(KRMovieEvent, void*)>& func) { m_callback = func; }
	IDVDStreamPlayerAudio *GetAudioPlayer() { return m_VideoPlayerAudio; }
	double GetClock() { return m_clock.GetClock(); }

	void SetLoopSegement(int beginFrame, unsigned int endFrame);

private:
	void OnActive();
	void OnDeactive();
//	bool		Shutdown = false;
	bool		m_bStopStatus = true;
	std::string m_strFileName;

	bool m_players_created = false;
	bool m_bAbortRequest = false;

	ECacheState  m_caching;
	Timer m_cachingTimer;
	Timer m_ChannelEntryTimeOut;
	std::unique_ptr<CProcessInfo> m_processInfo;

	CCurrentStream m_CurrentAudio;
	CCurrentStream m_CurrentVideo;

	CSelectionStreams m_SelectionStreams;

	std::atomic_int m_playSpeed;
	std::atomic_int m_newPlaySpeed;
	double m_origSpeed = 0;
	int m_streamPlayerSpeed = DVD_PLAYSPEED_NORMAL;
	struct SSpeedState
	{
		double  lastpts;  // holds last display pts during ff/rw operations
		int64_t lasttime;
		int lastseekpts;
		double  lastabstime;
	} m_SpeedState;

	int m_errorCount = 0;
	double m_offset_pts = 0.0;

	CDVDMessageQueue m_messenger;     // thread messenger

	IDVDStreamPlayerVideo *m_VideoPlayerVideo = nullptr;
	IDVDStreamPlayerAudio *m_VideoPlayerAudio = nullptr;

	CDVDClock m_clock;
//	CDVDOverlayContainer m_overlayContainer;

	InputStream* m_pInputStream = nullptr;  // input stream for current playing file
	IDemux* m_pDemuxer = nullptr;            // demuxer for current playing file

	CRenderManager m_renderManager;

	SPlayerState m_State;
	CCriticalSection m_StateSection;
	Timer m_syncTimer;

	std::condition_variable m_ready;

	bool m_HasVideo = false;
	bool m_HasAudio = false;

	struct SOmxPlayerState m_OmxPlayerState;
	bool m_omxplayer_mode = false;            // using omxplayer acceleration

	Timer m_player_status_timer;
	CBaseRenderer *m_pRenderer;
	std::function<void(KRMovieEvent, void*)> m_callback;

	int m_iLoopSegmentBegin = -1;
	unsigned int m_iLoopSegmentEnd = -1;
};
NS_KRMOVIE_END