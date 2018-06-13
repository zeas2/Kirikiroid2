#include "VideoPlayer.h"
#include <cmath>
#include <algorithm>
#include "DemuxFFmpeg.h"
#include "VideoPlayerVideo.h"
#include "VideoPlayerAudio.h"
#include "DemuxPacket.h"
#include "InputStream.h"
#include "krmovie.h"
#include "TimeUtils.h"
#include "tjsConfig.h"
#include "Application.h"
#include <cstdlib>
#include <iterator>
#include "platform/CCPlatformConfig.h"
#include "AEStream.h"
#include "WaveMixer.h"

#ifdef HAS_OMXPLAYER
#include "../omxplayer/OMXPlayerAudio.h"
#include "../omxplayer/OMXPlayerVideo.h"
#include "../omxplayer/OMXHelper.h"
#endif

void TVPInitLibAVCodec();

NS_KRMOVIE_BEGIN

void CSelectionStreams::Clear(StreamType type, StreamSource source)
{
	std::lock_guard<std::recursive_mutex> lock(m_section);
	auto new_end = std::remove_if(m_Streams.begin(), m_Streams.end(),
		[type, source](const SelectionStream &stream)
	{
		return (type == STREAM_NONE || stream.type == type) &&
			(source == 0 || stream.source == source);
	});
	m_Streams.erase(new_end, m_Streams.end());
}

int CSelectionStreams::IndexOf(StreamType type, int source, int64_t demuxerId, int id)
{
	std::lock_guard<std::recursive_mutex> lock(m_section);
	int count = -1;
	for (size_t i = 0; i < m_Streams.size(); i++)
	{
		if (type && m_Streams[i].type != type)
			continue;
		count++;
		if (source && m_Streams[i].source != source)
			continue;
		if (id < 0)
			continue;
		if (m_Streams[i].id == id && m_Streams[i].demuxerId == demuxerId)
			return count;
	}
	if (id < 0)
		return count;
	else
		return -1;
}

SelectionStream& CSelectionStreams::Get(StreamType type, int index)
{
	std::lock_guard<std::recursive_mutex> lock(m_section);
	int count = -1;
	for (size_t i = 0; i < m_Streams.size(); i++)
	{
		if (m_Streams[i].type != type)
			continue;
		count++;
		if (count == index)
			return m_Streams[i];
	}
	return m_invalid;
}

std::vector<SelectionStream> CSelectionStreams::Get(StreamType type)
{
	std::vector<SelectionStream> streams;
	std::copy_if(m_Streams.begin(), m_Streams.end(), std::back_inserter(streams),
		[type](const SelectionStream &stream)
	{
		return stream.type == type;
	});
	return streams;
}

bool CSelectionStreams::Get(StreamType type, CDemuxStream::EFlags flag, SelectionStream& out)
{
	std::lock_guard<std::recursive_mutex> lock(m_section);
	for (size_t i = 0; i < m_Streams.size(); i++)
	{
		if (m_Streams[i].type != type)
			continue;
		if ((m_Streams[i].flags & flag) != flag)
			continue;
		out = m_Streams[i];
		return true;
	}
	return false;
}

int CSelectionStreams::Source(StreamSource source, const std::string & filename)
{
	std::lock_guard<std::recursive_mutex> lock(m_section);
	int index = source - 1;
	for (size_t i = 0; i < m_Streams.size(); i++)
	{
		SelectionStream &s = m_Streams[i];
		if (STREAM_SOURCE_MASK(s.source) != source)
			continue;
		// if it already exists, return same
		if (s.filename == filename)
			return s.source;
		if (index < s.source)
			index = s.source;
	}
	// return next index
	return index + 1;
}

void CSelectionStreams::Update(SelectionStream& s)
{
	std::lock_guard<std::recursive_mutex> lock(m_section);
	int index = IndexOf(s.type, s.source, s.demuxerId, s.id);
	if (index >= 0)
	{
		SelectionStream& o = Get(s.type, index);
		s.type_index = o.type_index;
		o = s;
	} else
	{
		s.type_index = Count(s.type);
		m_Streams.push_back(s);
	}
}

void CSelectionStreams::Update(InputStream* input, IDemux* demuxer, const std::string &filename2)
{
	if (demuxer)
	{
		int source;
		const std::string& filename = input->GetFileName();
		if (input) /* hack to know this is sub decoder */
			source = Source(STREAM_SOURCE_DEMUX, filename);
// 		else if (!filename2.empty())
// 			source = Source(STREAM_SOURCE_DEMUX_SUB, filename);
		else
			source = Source(STREAM_SOURCE_VIDEOMUX, filename);

		for (auto stream : demuxer->GetStreams())
		{
			/* skip streams with no type */
			if (stream->type == STREAM_NONE)
				continue;
			/* make sure stream is marked with right source */
			stream->source = source;

			SelectionStream s;
			s.source = source;
			s.type = stream->type;
			s.id = stream->uniqueId;
			s.demuxerId = stream->demuxerId;
//			s.language = g_LangCodeExpander.ConvertToISO6392T(stream->language);
			s.flags = stream->flags;
			s.filename = filename;
//			s.filename2 = filename2;
			s.name = stream->GetStreamName();
			s.codec = demuxer->GetStreamCodecName(stream->demuxerId, stream->uniqueId);
			s.channels = 0; // Default to 0. Overwrite if STREAM_AUDIO below.
			if (stream->type == STREAM_VIDEO)
			{
				s.width = ((CDemuxStreamVideo*)stream)->iWidth;
				s.height = ((CDemuxStreamVideo*)stream)->iHeight;
				s.stereo_mode = ((CDemuxStreamVideo*)stream)->stereo_mode;
			}
			if (stream->type == STREAM_AUDIO)
			{
				std::string type;
				type = ((CDemuxStreamAudio*)stream)->GetStreamType();
				if (type.length() > 0)
				{
					if (s.name.length() > 0)
						s.name += " - ";
					s.name += type;
				}
				s.channels = ((CDemuxStreamAudio*)stream)->iChannels;
			}
			Update(s);
		}
	}
// 	CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
// 	CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
}

BasePlayer::BasePlayer(CBaseRenderer *renderer)
	: m_CurrentAudio(STREAM_AUDIO, VideoPlayer_AUDIO)
	, m_CurrentVideo(STREAM_VIDEO, VideoPlayer_VIDEO)
	, m_messenger("player")
	, m_renderManager(m_clock, this)
	, m_processInfo(CProcessInfo::CreateInstance())
	, m_pRenderer(renderer)
{
	TVPInitDirectSound(); // to avoid initialize in other thread
	TVPInitLibAVCodec();
	m_playSpeed = DVD_PLAYSPEED_NORMAL;
	m_newPlaySpeed = DVD_PLAYSPEED_NORMAL;
	m_streamPlayerSpeed = DVD_PLAYSPEED_NORMAL;
	m_caching = CACHESTATE_DONE;
	memset(&m_SpeedState, 0, sizeof(m_SpeedState));
#if 0
	::Application->RegisterActiveEvent(this, [](void* p, eTVPActiveEvent ev){
		switch (ev) {
		case eTVPActiveEvent::onActive:
			static_cast<BasePlayer*>(p)->OnActive();
			break;
		case eTVPActiveEvent::onDeactive:
			static_cast<BasePlayer*>(p)->OnDeactive();
			break;
		}
	});
#endif
}

BasePlayer::~BasePlayer() {
	CloseInputStream();
	DestroyPlayers();
	::Application->RegisterActiveEvent(this, nullptr);
}

void BasePlayer::Play()
{
	m_bStopStatus = false;

	if (!m_ThreadId)
		Create();
	
	if (GetSpeed() == 0)
		SetSpeed(1/*m_newPlaySpeed*/);
}

void BasePlayer::Stop()
{
	m_bStopStatus = true;
	// pause and rewind
	SetSpeed(0);
	SeekTime(0);
}

void BasePlayer::Pause()
{
	if (GetSpeed() != 0)
		SetSpeed(0);
}

void BasePlayer::GetVideoSize(long *width, long *height)
{
	std::lock_guard<std::recursive_mutex> lock(m_SelectionStreams.m_section);
	int streamId = GetVideoStream();

	if (streamId < 0) {
		*width = 0;
		*height = 0;
		return;
	}

	SelectionStream& s = m_SelectionStreams.Get(STREAM_VIDEO, streamId);

	*width = s.width;
	*height = s.height;
}

void BasePlayer::SetLoopSegement(int beginFrame, unsigned int endFrame)
{
	m_iLoopSegmentBegin = beginFrame;
	m_iLoopSegmentEnd = endFrame;
}

void BasePlayer::OnDeactive()
{
	m_origSpeed = GetSpeed();
	m_clock.Pause(true);
}

void BasePlayer::OnActive()
{
	m_clock.Pause(false);
	SetSpeed(m_origSpeed);
}

void BasePlayer::VideoParamsChange()
{
	m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_AVCHANGE));
}

void BasePlayer::GetDebugInfo(std::string &audio, std::string &video, std::string &general)
{
// 	audio = m_VideoPlayerAudio->GetPlayerInfo();
// 	video = m_VideoPlayerVideo->GetPlayerInfo();
// 	GetGeneralInfo(general);
}

void BasePlayer::UpdateClockSync(bool enabled)
{
	m_processInfo->SetRenderClockSync(enabled);
}

void BasePlayer::UpdateRenderInfo(CRenderInfo &info)
{
	m_processInfo->UpdateRenderInfo(info);
}

bool BasePlayer::OpenFromStream(IStream *stream, const tjs_char * streamname, const tjs_char *type, uint64_t size)
{
	if (IsRunning())
		CloseInputStream();

	TJS::tTJSNarrowStringHolder holder(streamname);
	std::string filename = holder.operator const tjs_nchar *();

	m_bAbortRequest = false;
	SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

	m_State.Clear();
	memset(&m_SpeedState, 0, sizeof(m_SpeedState));
//	m_UpdateApplication = 0;
	m_offset_pts = 0;
	m_CurrentAudio.lastdts = DVD_NOPTS_VALUE;
	m_CurrentVideo.lastdts = DVD_NOPTS_VALUE;

//	m_PlayerOptions = options;
//	m_item = file;
	// Try to resolve the correct mime type
//	m_item.SetMimeTypeForInternetFile();

//	m_ready.Reset();

//	m_renderManager.PreInit();

	m_CurrentVideo.Clear();
	m_CurrentAudio.Clear();
// 	m_CurrentSubtitle.Clear();
// 	m_CurrentTeletext.Clear();
// 	m_CurrentRadioRDS.Clear();

	m_messenger.Init();

//	CUtil::ClearTempFonts();
	m_playSpeed = DVD_PLAYSPEED_PAUSE; // pause at begin
	m_newPlaySpeed = DVD_PLAYSPEED_PAUSE;
	m_streamPlayerSpeed = DVD_PLAYSPEED_PAUSE;

	OpenInputStream(stream, filename);
	OpenDemuxStream();
	CreatePlayers();
#if HAS_OMXPLAYER
	if (m_omxplayer_mode)
	{
		if (!m_OmxPlayerState.av_clock.OMXInitialize(&m_clock))
			m_bAbortRequest = true;
#if 0
		if (CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_ADJUSTREFRESHRATE) != ADJUST_REFRESHRATE_OFF)
			m_OmxPlayerState.av_clock.HDMIClockSync();
#endif
		m_OmxPlayerState.av_clock.OMXStateIdle();
		m_OmxPlayerState.av_clock.OMXStateExecute();
		m_OmxPlayerState.av_clock.OMXStop();
		m_OmxPlayerState.av_clock.OMXPause();
	}
#endif
	OpenDefaultStreams();

	UpdatePlayState(0);

	SetCaching(CACHESTATE_FLUSH);

	// Playback might have been stopped due to some error
	if (m_bStop || m_bAbortRequest)
		return false;

	// ready to play
	m_renderManager.Configure();
	return true;
}

bool BasePlayer::CloseInputStream()
{
//	CLog::Log(LOGNOTICE, "CVideoPlayer::CloseFile()");

	// set the abort request so that other threads can finish up
	m_bAbortRequest = true;

	// tell demuxer to abort
	if (m_pDemuxer)
		m_pDemuxer->Abort();

// 	if (m_pSubtitleDemuxer)
// 		m_pSubtitleDemuxer->Abort();

	SAFE_RELEASE(m_pInputStream);

// 	if (m_pInputStream)
// 		m_pInputStream->Abort();

//	CLog::Log(LOGNOTICE, "VideoPlayer: waiting for threads to exit");

	// wait for the main thread to finish up
	// since this main thread cleans up all other resources and threads
	// we are done after the StopThread call
	StopThread();

	m_HasVideo = false;
	m_HasAudio = false;
//	m_canTempo = false;

//	CLog::Log(LOGNOTICE, "VideoPlayer: finished waiting");
//	m_renderManager.UnInit();
	return true;
}

bool BasePlayer::IsPlaying() const
{
	return !m_bStop;
}

bool BasePlayer::CanSeek()
{
	CSingleLock lock(m_StateSection);
	return m_State.canseek;
}

void BasePlayer::OnExit()
{
//	CLog::Log(LOGNOTICE, "CVideoPlayer::OnExit()");

	// set event to inform openfile something went wrong in case openfile is still waiting for this event
	SetCaching(CACHESTATE_DONE);

	// close each stream
//	if (!m_bAbortRequest) CLog::Log(LOGNOTICE, "VideoPlayer: eof, waiting for queues to empty");
	CloseStream(m_CurrentAudio, !m_bAbortRequest);
	CloseStream(m_CurrentVideo, !m_bAbortRequest);

	// the generalization principle was abused for subtitle player. actually it is not a stream player like
	// video and audio. subtitle player does not run on its own thread, hence waitForBuffers makes
	// no sense here. waitForBuffers is abused to clear overlay container (false clears container)
	// subtitles are added from video player. after video player has finished, overlays have to be cleared.
//	CloseStream(m_CurrentSubtitle, false);  // clear overlay container

// 	CloseStream(m_CurrentTeletext, !m_bAbortRequest);
// 	CloseStream(m_CurrentRadioRDS, !m_bAbortRequest);

	// destroy objects
	SAFE_DELETE(m_pDemuxer);
// 	SAFE_DELETE(m_pSubtitleDemuxer);
// 	SAFE_DELETE(m_pCCDemuxer);
	SAFE_RELEASE(m_pInputStream);

	// clean up all selection streams
	m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NONE);

	m_messenger.End();
#ifdef HAS_OMXPLAYER
	if (m_omxplayer_mode)
	{
		m_OmxPlayerState.av_clock.OMXStop();
		m_OmxPlayerState.av_clock.OMXStateIdle();
		m_OmxPlayerState.av_clock.OMXDeinitialize();
	}
#endif
	m_bStop = true;
	m_bStopStatus = true;
	// if we didn't stop playing, advance to the next item in xbmc's playlist
//	if (m_PlayerOptions.identify == false)
	{
		// already ended, don't send message
//		if (m_callback) m_callback(KRMovieEvent::Stopped, nullptr);
	}

	// set event to inform openfile something went wrong in case openfile is still waiting for this event
	m_ready.notify_all();

//	CFFmpegLog::ClearLogLevel();
}

bool BasePlayer::IsValidStream(CCurrentStream& stream)
{
	if (stream.id < 0)
		return true; // we consider non selected as valid

	int source = STREAM_SOURCE_MASK(stream.source);
	if (source == STREAM_SOURCE_TEXT)
		return true;
#if 0
	if (source == STREAM_SOURCE_DEMUX_SUB)
	{
		CDemuxStream* st = m_pSubtitleDemuxer->GetStream(stream.demuxerId, stream.id);
		if (st == NULL || st->disabled)
			return false;
		if (st->type != stream.type)
			return false;
		return true;
	}
#endif
	if (source == STREAM_SOURCE_DEMUX)
	{
		CDemuxStream* st = m_pDemuxer->GetStream(stream.demuxerId, stream.id);
		if (st == NULL || st->disabled)
			return false;
		if (st->type != stream.type)
			return false;
		
		return true;
	}
#if 0
	if (source == STREAM_SOURCE_VIDEOMUX)
	{
		CDemuxStream* st = m_pCCDemuxer->GetStream(stream.id);
		if (st == NULL || st->disabled)
			return false;
		if (st->type != stream.type)
			return false;
		return true;
	}
#endif

	return false;
}

bool BasePlayer::IsBetterStream(CCurrentStream& current, CDemuxStream* stream)
{
	if (stream->disabled)
		return false;
	if (stream->source == current.source &&
		stream->uniqueId == current.id &&
		stream->demuxerId == current.demuxerId)
		return false;

	if (stream->type != current.type)
		return false;
#if 0
	if (current.type == STREAM_SUBTITLE)
		return false;
#endif
	if (current.id < 0)
		return true;

	return false;
}

void BasePlayer::CheckBetterStream(CCurrentStream& current, CDemuxStream* stream)
{
	IDVDStreamPlayer* player = GetStreamPlayer(current.player);
	if (!IsValidStream(current) && (player == NULL || player->IsStalled()))
		CloseStream(current, true);

	if (IsBetterStream(current, stream))
		OpenStream(current, stream->demuxerId, stream->uniqueId, stream->source);
}

void BasePlayer::CheckStreamChanges(CCurrentStream& current, CDemuxStream* stream)
{
	if (current.stream != (void*)stream
		|| current.changes != stream->changes)
	{
		/* check so that dmuxer hints or extra data hasn't changed */
		/* if they have, reopen stream */

		if (current.hint != CDVDStreamInfo(*stream, true))
		{
			m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
			m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
			OpenDefaultStreams(false);
		}

		current.stream = (void*)stream;
		current.changes = stream->changes;
	}
}

void BasePlayer::Process()
{
	while (!m_bAbortRequest)
	{
#ifdef HAS_OMXPLAYER
		if (m_omxplayer_mode && OMXDoProcessing(m_OmxPlayerState, m_playSpeed, m_VideoPlayerVideo, m_VideoPlayerAudio, m_CurrentAudio, m_CurrentVideo, m_HasVideo, m_HasAudio, m_renderManager))
		{
			CloseStream(m_CurrentVideo, false);
			OpenStream(m_CurrentVideo, m_CurrentVideo.demuxerId, m_CurrentVideo.id, m_CurrentVideo.source);
			if (m_State.canseek)
			{
				CDVDMsgPlayerSeek::CMode mode;
				mode.time = (int)GetTime();
				mode.backward = true;
				mode.flush = true;
				mode.accurate = true;
				mode.sync = true;
				m_messenger.Put(new CDVDMsgPlayerSeek(mode));
			}
		}
#endif

		// handle messages send to this thread, like seek or demuxer reset requests
		HandleMessages();

		if (m_bAbortRequest)
			break;

		// should we open a new demuxer?
		if (!m_pDemuxer)
		{
			if (OpenDemuxStream() == false)
			{
				m_bAbortRequest = true;
				break;
			}

			// on channel switch we don't want to close stream players at this
			// time. we'll get the stream change event later
			OpenDefaultStreams();

//			UpdateApplication(0);
			UpdatePlayState(0);
		}

		// handle eventual seeks due to playspeed
		HandlePlaySpeed();

		// update player state
		UpdatePlayState(200);

		// update application with our state
//		UpdateApplication(1000);

		// make sure we run subtitle process here
//		m_VideoPlayerSubtitle->Process(m_clock.GetClock() + m_State.time_offset - m_VideoPlayerVideo->GetSubtitleDelay(), m_State.time_offset);

// 		if (CheckDelayedChannelEntry())
// 			continue;

		// if the queues are full, no need to read more
		if ((!m_VideoPlayerAudio->AcceptsData() && m_CurrentAudio.id >= 0) ||
			(!m_VideoPlayerVideo->AcceptsData() && m_CurrentVideo.id >= 0))
		{
			Sleep(10);
			continue;
		}

		// always yield to players if they have data levels > 50 percent
		if ((m_VideoPlayerAudio->GetLevel() > 50 || m_CurrentAudio.id < 0)
			&& (m_VideoPlayerVideo->GetLevel() > 50 || m_CurrentVideo.id < 0))
			Sleep(0);

		DemuxPacket* pPacket = NULL;
		CDemuxStream *pStream = NULL;
		ReadPacket(pPacket, pStream);
		if (pPacket && !pStream)
		{
			/* probably a empty packet, just free it and move on */
			DemuxPacket::Free(pPacket);
			continue;
		}

		if (!pPacket)
		{
			// when paused, demuxer could be be returning empty
			if (m_playSpeed == DVD_PLAYSPEED_PAUSE)
				continue;

#ifdef HAS_OMXPLAYER
			// make sure we tell all players to finish it's data
			if (m_omxplayer_mode && !m_OmxPlayerState.bOmxSentEOFs)
			{
				if (m_CurrentAudio.inited)
					m_OmxPlayerState.bOmxWaitAudio = true;

				if (m_CurrentVideo.inited)
					m_OmxPlayerState.bOmxWaitVideo = true;

				m_OmxPlayerState.bOmxSentEOFs = true;
			}
#endif

			if (m_iLoopSegmentBegin != -1) { // process loop info
				double start = DVD_NOPTS_VALUE;
				if (m_pDemuxer && m_pDemuxer->SeekTime(m_iLoopSegmentBegin / GetFPS() * DVD_PLAYSPEED_NORMAL, true, &start)) {
					continue;
				}
			}

			if (m_CurrentAudio.inited)
				m_VideoPlayerAudio->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
			if (m_CurrentVideo.inited)
				m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
// 			if (m_CurrentSubtitle.inited)
// 				m_VideoPlayerSubtitle->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
// 			if (m_CurrentTeletext.inited)
// 				m_VideoPlayerTeletext->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
// 			if (m_CurrentRadioRDS.inited)
// 				m_VideoPlayerRadioRDS->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));

			m_CurrentAudio.inited = false;
			m_CurrentVideo.inited = false;
// 			m_CurrentSubtitle.inited = false;
// 			m_CurrentTeletext.inited = false;
// 			m_CurrentRadioRDS.inited = false;

			// if we are caching, start playing it again
			SetCaching(CACHESTATE_DONE);

			// while players are still playing, keep going to allow seekbacks
			if (m_VideoPlayerAudio->HasData()
				|| m_VideoPlayerVideo->HasData())
			{
				Sleep(100);
				continue;
			}
#ifdef HAS_OMXPLAYER
			if (m_omxplayer_mode && OMXStillPlaying(m_OmxPlayerState.bOmxWaitVideo, m_OmxPlayerState.bOmxWaitAudio, m_VideoPlayerVideo->IsEOS(), m_VideoPlayerAudio->IsEOS()))
			{
				Sleep(100);
				continue;
			}
#endif
			
			// TODO process loop info
			SetSpeed(0);
			SeekTime(0); // rewind
			if (m_callback) m_callback(KRMovieEvent::Ended, nullptr);
			continue;
		}

		// it's a valid data packet, reset error counter
		m_errorCount = 0;

		// see if we can find something better to play
		CheckBetterStream(m_CurrentAudio, pStream);
		CheckBetterStream(m_CurrentVideo, pStream);
// 		CheckBetterStream(m_CurrentSubtitle, pStream);
// 		CheckBetterStream(m_CurrentTeletext, pStream);
// 		CheckBetterStream(m_CurrentRadioRDS, pStream);

		// demux video stream
#if 0
		if (CSettings::GetInstance().GetBool(CSettings::SETTING_SUBTITLES_PARSECAPTIONS) && CheckIsCurrent(m_CurrentVideo, pStream, pPacket))
		{
			if (m_pCCDemuxer)
			{
				bool first = true;
				while (!m_bAbortRequest)
				{
					DemuxPacket *pkt = m_pCCDemuxer->Read(first ? pPacket : NULL);
					if (!pkt)
						break;

					first = false;
					if (m_pCCDemuxer->GetNrOfStreams() != m_SelectionStreams.CountSource(STREAM_SUBTITLE, STREAM_SOURCE_VIDEOMUX))
					{
						m_SelectionStreams.Clear(STREAM_SUBTITLE, STREAM_SOURCE_VIDEOMUX);
						m_SelectionStreams.Update(NULL, m_pCCDemuxer, "");
						OpenDefaultStreams(false);
					}
					CDemuxStream *pSubStream = m_pCCDemuxer->GetStream(pkt->iStreamId);
					if (pSubStream && m_CurrentSubtitle.id == pkt->iStreamId && m_CurrentSubtitle.source == STREAM_SOURCE_VIDEOMUX)
						ProcessSubData(pSubStream, pkt);
					else
						CDVDDemuxUtils::FreeDemuxPacket(pkt);
				}
			}
		}

		if (IsInMenuInternal())
		{
			if (CDVDInputStream::IMenus* menu = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
			{
				double correction = menu->GetTimeStampCorrection();
				if (pPacket->dts > correction)
					pPacket->dts -= correction;
				if (pPacket->pts > correction)
					pPacket->pts -= correction;
			}
			if (m_dvd.syncClock)
			{
				m_clock.Discontinuity(pPacket->dts);
				m_dvd.syncClock = false;
			}
		}
#endif
		// process the packet
		ProcessPacket(pStream, pPacket);

		// check if in a cut or commercial break that should be automatically skipped
//		CheckAutoSceneSkip();
#if 0
		// update the player info for streams
		if (m_player_status_timer.IsTimePast())
		{
			m_player_status_timer.Set(500);
			UpdateStreamInfos();
		}
#endif
	}
}

void BasePlayer::CreatePlayers()
{
#ifdef HAS_OMXPLAYER
	bool omx_suitable = !OMXPlayerUnsuitable(m_HasVideo, m_HasAudio, m_pDemuxer, m_pInputStream, m_SelectionStreams);
	if (m_omxplayer_mode != omx_suitable)
	{
		DestroyPlayers();
		m_omxplayer_mode = omx_suitable;
	}
#endif
	if (m_players_created)
		return;

#ifdef HAS_OMXPLAYER
	if (m_omxplayer_mode)
	{
		m_VideoPlayerVideo = new OMXPlayerVideo(&m_OmxPlayerState.av_clock, &m_overlayContainer, m_messenger, m_renderManager, *m_processInfo);
		m_VideoPlayerAudio = new OMXPlayerAudio(&m_OmxPlayerState.av_clock, m_messenger, *m_processInfo);
	} else
#endif
	{
		m_VideoPlayerVideo = new CVideoPlayerVideo(&m_clock, /*&m_overlayContainer,*/ m_messenger, m_renderManager, *m_processInfo);
		m_VideoPlayerAudio = new CVideoPlayerAudio(&m_clock, m_messenger, *m_processInfo);
	}
	// 	m_VideoPlayerSubtitle = new CVideoPlayerSubtitle(&m_overlayContainer, *m_processInfo);
	// 	m_VideoPlayerTeletext = new CDVDTeletextData(*m_processInfo);
	// 	m_VideoPlayerRadioRDS = new CDVDRadioRDSData(*m_processInfo);
	m_players_created = true;
}

void BasePlayer::DestroyPlayers()
{
	if (!m_players_created)
		return;

	delete m_VideoPlayerVideo;
	delete m_VideoPlayerAudio;
// 	delete m_VideoPlayerSubtitle;
// 	delete m_VideoPlayerTeletext;
// 	delete m_VideoPlayerRadioRDS;

	m_players_created = false;
}

bool BasePlayer::OpenStream(CCurrentStream& current, int64_t demuxerId, int iStream, int source, bool reset /*= true*/)
{
	CDemuxStream* stream = NULL;
	CDVDStreamInfo hint;

//	CLog::Log(LOGNOTICE, "Opening stream: %i source: %i", iStream, source);
#if 0
	if (STREAM_SOURCE_MASK(source) == STREAM_SOURCE_DEMUX_SUB)
	{
		int index = m_SelectionStreams.IndexOf(current.type, source, demuxerId, iStream);
		if (index < 0)
			return false;
		SelectionStream st = m_SelectionStreams.Get(current.type, index);

		if (!m_pSubtitleDemuxer || m_pSubtitleDemuxer->GetFileName() != st.filename)
		{
			CLog::Log(LOGNOTICE, "Opening Subtitle file: %s", st.filename.c_str());
			std::unique_ptr<CDVDDemuxVobsub> demux(new CDVDDemuxVobsub());
			if (!demux->Open(st.filename, source, st.filename2))
				return false;
			m_pSubtitleDemuxer = demux.release();
		}

		double pts = m_VideoPlayerVideo->GetCurrentPts();
		if (pts == DVD_NOPTS_VALUE)
			pts = m_CurrentVideo.dts;
		if (pts == DVD_NOPTS_VALUE)
			pts = 0;
		pts += m_offset_pts;
		if (!m_pSubtitleDemuxer->SeekTime((int)(1000.0 * pts / (double)DVD_TIME_BASE)))
			CLog::Log(LOGDEBUG, "%s - failed to start subtitle demuxing from: %f", __FUNCTION__, pts);
		stream = m_pSubtitleDemuxer->GetStream(demuxerId, iStream);
		if (!stream || stream->disabled)
			return false;

		m_pSubtitleDemuxer->EnableStream(demuxerId, iStream, true);

		hint.Assign(*stream, true);
	} else
#endif
		if (STREAM_SOURCE_MASK(source) == STREAM_SOURCE_TEXT)
	{
		int index = m_SelectionStreams.IndexOf(current.type, source, demuxerId, iStream);
		if (index < 0)
			return false;

		hint.Clear();
		hint.filename = m_SelectionStreams.Get(current.type, index).filename;
		hint.fpsscale = m_CurrentVideo.hint.fpsscale;
		hint.fpsrate = m_CurrentVideo.hint.fpsrate;
	} else if (STREAM_SOURCE_MASK(source) == STREAM_SOURCE_DEMUX)
	{
		if (!m_pDemuxer)
			return false;

		stream = m_pDemuxer->GetStream(demuxerId, iStream);
		if (!stream || stream->disabled)
			return false;

		m_pDemuxer->EnableStream(demuxerId, iStream, true);

		hint.Assign(*stream, true);
#if 0
	} else if (STREAM_SOURCE_MASK(source) == STREAM_SOURCE_VIDEOMUX)
	{
		if (!m_pCCDemuxer)
			return false;

		stream = m_pCCDemuxer->GetStream(iStream);
		if (!stream || stream->disabled)
			return false;

		hint.Assign(*stream, false);
#endif
	}

	bool res;
	switch (current.type)
	{
	case STREAM_AUDIO:
		res = OpenAudioStream(hint, reset);
		break;
	case STREAM_VIDEO:
		res = OpenVideoStream(hint, reset);
		break;
#if 0
	case STREAM_SUBTITLE:
		res = OpenSubtitleStream(hint);
		break;
	case STREAM_TELETEXT:
		res = OpenTeletextStream(hint);
		break;
	case STREAM_RADIO_RDS:
		res = OpenRadioRDSStream(hint);
		break;
#endif
	default:
		res = false;
		break;
	}

	if (res)
	{
		current.id = iStream;
		current.demuxerId = demuxerId;
		current.source = source;
		current.hint = hint;
		current.stream = (void*)stream;
		current.lastdts = DVD_NOPTS_VALUE;
		if (current.avsync != CCurrentStream::AV_SYNC_FORCE)
			current.avsync = CCurrentStream::AV_SYNC_CHECK;
		if (stream)
			current.changes = stream->changes;
	} else
	{
		if (stream)
		{
			/* mark stream as disabled, to disallaw further attempts*/
//			CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, stream->uniqueId);
			stream->disabled = true;
		}
	}

// 	CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
// 	CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();

	return res;
}

bool BasePlayer::OpenAudioStream(CDVDStreamInfo& hint, bool reset)
{
	IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentAudio.player);
	if (player == nullptr)
		return false;

	if (m_CurrentAudio.id < 0 ||
		m_CurrentAudio.hint != hint)
	{
		if (!player->OpenStream(hint))
			return false;

		static_cast<IDVDStreamPlayerAudio*>(player)->SetSpeed(m_streamPlayerSpeed);
		m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_STARTING;
		m_CurrentAudio.packets = 0;
	} else if (reset)
		player->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET), 0);

	m_HasAudio = true;

	return true;
}

bool BasePlayer::OpenVideoStream(CDVDStreamInfo& hint, bool reset)
{
// 	if (hint.stereo_mode.empty())
// 		hint.stereo_mode = CStereoscopicsManager::GetInstance().DetectStereoModeByString(m_item.GetPath());

	SelectionStream& s = m_SelectionStreams.Get(STREAM_VIDEO, 0);

// 	if (hint.flags & AV_DISPOSITION_ATTACHED_PIC)
// 		return false;
	
	IDVDStreamPlayer* player = GetStreamPlayer(m_CurrentVideo.player);
	if (player == nullptr)
		return false;

	if (m_CurrentVideo.id < 0 ||
		m_CurrentVideo.hint != hint)
	{
// 		if (hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_H264)
// 			SAFE_DELETE(m_pCCDemuxer);

		if (!player->OpenStream(hint))
			return false;

		s.stereo_mode = static_cast<IDVDStreamPlayerVideo*>(player)->GetStereoMode();
		if (s.stereo_mode == "mono")
			s.stereo_mode = "";

		static_cast<IDVDStreamPlayerVideo*>(player)->SetSpeed(m_streamPlayerSpeed);
		m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_STARTING;
		m_CurrentVideo.packets = 0;
	} else if (reset)
		player->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET), 0);

	m_HasVideo = true;

	// open CC demuxer if video is mpeg2
// 	if ((hint.codec == AV_CODEC_ID_MPEG2VIDEO || hint.codec == AV_CODEC_ID_H264) && !m_pCCDemuxer)
// 	{
// 		m_pCCDemuxer = new CDVDDemuxCC(hint.codec);
// 		m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_VIDEOMUX);
// 	}

	return true;
}

bool BasePlayer::CloseStream(CCurrentStream& current, bool bWaitForBuffers)
{
	if (current.id < 0)
		return false;

//	CLog::Log(LOGNOTICE, "Closing stream player %d", current.player);

	if (bWaitForBuffers)
		SetCaching(CACHESTATE_DONE);

	if (m_pDemuxer && STREAM_SOURCE_MASK(current.source) == STREAM_SOURCE_DEMUX)
		m_pDemuxer->EnableStream(current.demuxerId, current.id, false);

	IDVDStreamPlayer* player = GetStreamPlayer(current.player);
	if (player)
	{
		if ((current.type == STREAM_AUDIO && current.syncState != IDVDStreamPlayer::SYNC_INSYNC) ||
			(current.type == STREAM_VIDEO && current.syncState != IDVDStreamPlayer::SYNC_INSYNC))
			bWaitForBuffers = false;
		player->CloseStream(bWaitForBuffers);
	}

	current.Clear();
	return true;
}

bool BasePlayer::CheckIsCurrent(CCurrentStream& current, CDemuxStream* stream, DemuxPacket* pkg)
{
	if (current.id == pkg->iStreamId
		&& current.demuxerId == stream->demuxerId
		&& current.source == stream->source
		&& current.type == stream->type)
		return true;
	else
		return false;
}

void BasePlayer::ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket)
{
	// process packet if it belongs to selected stream.
	// for dvd's don't allow automatic opening of streams*/

	if (CheckIsCurrent(m_CurrentAudio, pStream, pPacket))
		ProcessAudioData(pStream, pPacket);
	else if (CheckIsCurrent(m_CurrentVideo, pStream, pPacket))
		ProcessVideoData(pStream, pPacket);
// 	else if (CheckIsCurrent(m_CurrentSubtitle, pStream, pPacket))
// 		ProcessSubData(pStream, pPacket);
// 	else if (CheckIsCurrent(m_CurrentTeletext, pStream, pPacket))
// 		ProcessTeletextData(pStream, pPacket);
// 	else if (CheckIsCurrent(m_CurrentRadioRDS, pStream, pPacket))
// 		ProcessRadioRDSData(pStream, pPacket);
	else
	{
		DemuxPacket::Free(pPacket); // free it since we won't do anything with it
	}
}

void BasePlayer::ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
	CheckStreamChanges(m_CurrentAudio, pStream);

	bool checkcont = CheckContinuity(m_CurrentAudio, pPacket);
	UpdateTimestamps(m_CurrentAudio, pPacket);

	if (checkcont && (m_CurrentAudio.avsync == CCurrentStream::AV_SYNC_CHECK))
		m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_NONE;

	bool drop = false;
	if (CheckPlayerInit(m_CurrentAudio))
		drop = true;
	
	m_VideoPlayerAudio->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
	m_CurrentAudio.packets++;
}

void BasePlayer::ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
	CheckStreamChanges(m_CurrentVideo, pStream);
	bool checkcont = false;

	if (pPacket->iSize != 4) //don't check the EOF_SEQUENCE of stillframes
	{
		checkcont = CheckContinuity(m_CurrentVideo, pPacket);
		UpdateTimestamps(m_CurrentVideo, pPacket);
	}
	if (checkcont && (m_CurrentVideo.avsync == CCurrentStream::AV_SYNC_CHECK))
		m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_NONE;

	bool drop = false;
	if (CheckPlayerInit(m_CurrentVideo))
		drop = true;

	if (CheckSceneSkip(m_CurrentVideo))
		drop = true;

	m_VideoPlayerVideo->SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
	m_CurrentVideo.packets++;
}

void BasePlayer::SetPlaySpeed(int speed)
{
	if (IsPlaying())
		m_messenger.Put(new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed));
	else
	{
		m_playSpeed = speed;
		m_newPlaySpeed = speed;
		m_streamPlayerSpeed = speed;
	}
}

void BasePlayer::FlushBuffers(bool queued, double pts, bool accurate, bool sync)
{
//	CLog::Log(LOGDEBUG, "CVideoPlayer::FlushBuffers - flushing buffers");

	double startpts;
	if (accurate
#ifdef HAS_OMXPLAYER
		&& !m_omxplayer_mode
#endif
		)
		startpts = pts;
	else
		startpts = DVD_NOPTS_VALUE;

	if (sync)
	{
		m_CurrentAudio.inited = false;
		m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_FORCE;
		m_CurrentVideo.inited = false;
		m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_FORCE;
// 		m_CurrentSubtitle.inited = false;
// 		m_CurrentTeletext.inited = false;
// 		m_CurrentRadioRDS.inited = false;
	}

	m_CurrentAudio.dts = DVD_NOPTS_VALUE;
	m_CurrentAudio.startpts = startpts;
	m_CurrentAudio.packets = 0;

	m_CurrentVideo.dts = DVD_NOPTS_VALUE;
	m_CurrentVideo.startpts = startpts;
	m_CurrentVideo.packets = 0;

// 	m_CurrentSubtitle.dts = DVD_NOPTS_VALUE;
// 	m_CurrentSubtitle.startpts = startpts;
// 	m_CurrentSubtitle.packets = 0;
// 
// 	m_CurrentTeletext.dts = DVD_NOPTS_VALUE;
// 	m_CurrentTeletext.startpts = startpts;
// 	m_CurrentTeletext.packets = 0;
// 
// 	m_CurrentRadioRDS.dts = DVD_NOPTS_VALUE;
// 	m_CurrentRadioRDS.startpts = startpts;
// 	m_CurrentRadioRDS.packets = 0;

	if (queued)
	{
		m_VideoPlayerAudio->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
		m_VideoPlayerVideo->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
// 		m_VideoPlayerSubtitle->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
// 		m_VideoPlayerTeletext->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));
// 		m_VideoPlayerRadioRDS->SendMessage(new CDVDMsg(CDVDMsg::GENERAL_RESET));

		int sources = 0;
		if (m_VideoPlayerVideo->IsInited()) sources |= SYNCSOURCE_VIDEO;
		if (m_VideoPlayerAudio->IsInited()) sources |= SYNCSOURCE_AUDIO;

		CDVDMsgGeneralSynchronize* msg = new CDVDMsgGeneralSynchronize(10 * 1000, sources);
		m_VideoPlayerAudio->SendMessage(msg->AddRef(), 1);
		m_VideoPlayerVideo->SendMessage(msg->AddRef(), 1);
		msg->Wait(m_bStop, 0);
		msg->Release();
	} else
	{
		m_VideoPlayerAudio->Flush(sync);
		m_VideoPlayerVideo->Flush(sync);
// 		m_VideoPlayerSubtitle->Flush();
// 		m_VideoPlayerTeletext->Flush();
// 		m_VideoPlayerRadioRDS->Flush();

		// clear subtitle and menu overlays
	//	m_overlayContainer.Clear();

		if (m_playSpeed == DVD_PLAYSPEED_NORMAL
			|| m_playSpeed == DVD_PLAYSPEED_PAUSE)
		{
			// make sure players are properly flushed, should put them in stalled state
			int sources = 0;
			if (m_VideoPlayerVideo->IsInited()) sources |= SYNCSOURCE_VIDEO;
			if (m_VideoPlayerAudio->IsInited()) sources |= SYNCSOURCE_AUDIO;
			CDVDMsgGeneralSynchronize* msg = new CDVDMsgGeneralSynchronize(1000, sources);
			m_VideoPlayerAudio->SendMessage(msg->AddRef(), 1);
			m_VideoPlayerVideo->SendMessage(msg->AddRef(), 1);
			msg->Wait(m_bStop, 0);
			msg->Release();

			// purge any pending PLAYER_STARTED messages
			m_messenger.Flush(CDVDMsg::PLAYER_STARTED);

			// we should now wait for init cache
			SetCaching(CACHESTATE_FLUSH);
			if (sync)
			{
				m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_STARTING;
				m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_STARTING;
			}
		}

		if (pts != DVD_NOPTS_VALUE && sync)
			m_clock.Discontinuity(pts);
		UpdatePlayState(0);
	}
#ifdef HAS_OMXPLAYER
	if (m_omxplayer_mode)
	{
		m_OmxPlayerState.av_clock.OMXFlush();
		if (!queued)
			m_OmxPlayerState.av_clock.OMXStop();
		m_OmxPlayerState.av_clock.OMXPause();
		m_OmxPlayerState.av_clock.OMXMediaTime(0.0);
	}
#endif
}

void BasePlayer::HandleMessages()
{
	CDVDMsg* pMsg;

	while (m_messenger.Get(&pMsg, 0) == MSGQ_OK)
	{

		if (pMsg->IsType(CDVDMsg::PLAYER_SEEK) &&
			m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK) == 0 &&
			m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK_CHAPTER) == 0)
		{
			CDVDMsgPlayerSeek &msg(*((CDVDMsgPlayerSeek*)pMsg));

			if (!m_State.canseek)
			{
				m_processInfo->SetStateSeeking(false);
				pMsg->Release();
				continue;
			}

			// skip seeks if player has not finished the last seek
			if (m_CurrentVideo.id >= 0 &&
				m_CurrentVideo.syncState != IDVDStreamPlayer::SYNC_INSYNC)
			{
				double now = m_clock.GetAbsoluteClock();
				if (m_playSpeed == DVD_PLAYSPEED_NORMAL &&
					DVD_TIME_TO_MSEC(now - m_State.lastSeek) < 2000 &&
					!msg.GetAccurate())
				{
					m_processInfo->SetStateSeeking(false);
					pMsg->Release();
					continue;
				}
			}

			if (!msg.GetTrickPlay())
			{
//				g_infoManager.SetDisplayAfterSeek(100000);
				if (msg.GetFlush())
					SetCaching(CACHESTATE_FLUSH);
			}

			double start = DVD_NOPTS_VALUE;

			int time = msg.GetTime();
			if (msg.GetRelative())
				time = GetTime() + time;

//			time = msg.GetRestore() ? m_Edl.RestoreCutTime(time) : time;

			// if input stream doesn't support ISeekTime, convert back to pts
			//! @todo
			//! After demuxer we add an offset to input pts so that displayed time and clock are
			//! increasing steadily. For seeking we need to determine the boundaries and offset
			//! of the desired segment. With the current approach calculated time may point
			//! to nirvana
//			if (m_pInputStream->GetIPosTime() == nullptr)
				time -= DVD_TIME_TO_MSEC(m_State.time_offset);

//			CLog::Log(LOGDEBUG, "demuxer seek to: %d", time);
			if (m_pDemuxer && m_pDemuxer->SeekTime(time, msg.GetBackward(), &start)) {
// 				CLog::Log(LOGDEBUG, "demuxer seek to: %d, success", time);
// 				if (m_pSubtitleDemuxer)
// 				{
// 					if (!m_pSubtitleDemuxer->SeekTime(time, msg.GetBackward()))
// 						CLog::Log(LOGDEBUG, "failed to seek subtitle demuxer: %d, success", time);
// 				}
				// dts after successful seek
				if (start == DVD_NOPTS_VALUE)
					start = DVD_MSEC_TO_TIME(time) - m_State.time_offset;

				m_State.dts = start;
				m_State.lastSeek = m_clock.GetAbsoluteClock();

				FlushBuffers(!msg.GetFlush(), start, msg.GetAccurate(), msg.GetSync());
			} else if (m_pDemuxer) {
//				CLog::Log(LOGDEBUG, "VideoPlayer: seek failed or hit end of stream");
				// dts after successful seek
				if (start == DVD_NOPTS_VALUE)
					start = DVD_MSEC_TO_TIME(time) - m_State.time_offset;

				m_State.dts = start;

				FlushBuffers(false, start, false, true);
				if (m_playSpeed != DVD_PLAYSPEED_PAUSE)
				{
					SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
				}
			}

			// set flag to indicate we have finished a seeking request
// 			if (!msg.GetTrickPlay())
// 				g_infoManager.SetDisplayAfterSeek();

			// dvd's will issue a HOP_CHANNEL that we need to skip
// 			if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
// 				m_dvd.state = DVDSTATE_SEEK;

			m_processInfo->SetStateSeeking(false);
#if 0
		} else if (pMsg->IsType(CDVDMsg::PLAYER_SEEK_CHAPTER) &&
			m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK) == 0 &&
			m_messenger.GetPacketCount(CDVDMsg::PLAYER_SEEK_CHAPTER) == 0) {
			g_infoManager.SetDisplayAfterSeek(100000);
			SetCaching(CACHESTATE_FLUSH);

			CDVDMsgPlayerSeekChapter &msg(*((CDVDMsgPlayerSeekChapter*)pMsg));
			double start = DVD_NOPTS_VALUE;
			double offset = 0;
			int64_t beforeSeek = GetTime();

			// This should always be the case.
			if (m_pDemuxer && m_pDemuxer->SeekChapter(msg.GetChapter(), &start))
			{
				FlushBuffers(false, start, true);
				offset = DVD_TIME_TO_MSEC(start) - beforeSeek;
				m_callback.OnPlayBackSeekChapter(msg.GetChapter());
			}

			g_infoManager.SetDisplayAfterSeek(2500, offset);
#endif
		} else if (pMsg->IsType(CDVDMsg::DEMUXER_RESET)) {
			m_CurrentAudio.stream = NULL;
			m_CurrentVideo.stream = NULL;
//			m_CurrentSubtitle.stream = NULL;

			// we need to reset the demuxer, probably because the streams have changed
			if (m_pDemuxer)
				m_pDemuxer->Reset();
// 			if (m_pSubtitleDemuxer)
// 				m_pSubtitleDemuxer->Reset();
		} else if (pMsg->IsType(CDVDMsg::PLAYER_SET_AUDIOSTREAM)) {
			CDVDMsgPlayerSetAudioStream* pMsg2 = (CDVDMsgPlayerSetAudioStream*)pMsg;

			SelectionStream& st = m_SelectionStreams.Get(STREAM_AUDIO, pMsg2->GetStreamId());
			if (st.source != STREAM_SOURCE_NONE)
			{
				{
					CloseStream(m_CurrentAudio, false);
					OpenStream(m_CurrentAudio, st.demuxerId, st.id, st.source);
//					AdaptForcedSubtitles();

					CDVDMsgPlayerSeek::CMode mode;
					mode.time = (int)GetTime();
					mode.backward = true;
					mode.flush = true;
					mode.accurate = true;
					mode.trickplay = true;
					mode.sync = true;
					m_messenger.Put(new CDVDMsgPlayerSeek(mode));
				}
			}
		} else if (pMsg->IsType(CDVDMsg::PLAYER_SET_VIDEOSTREAM)){
			CDVDMsgPlayerSetVideoStream* pMsg2 = (CDVDMsgPlayerSetVideoStream*)pMsg;

			SelectionStream& st = m_SelectionStreams.Get(STREAM_VIDEO, pMsg2->GetStreamId());
			if (st.source != STREAM_SOURCE_NONE)
			{
				{
					CloseStream(m_CurrentVideo, false);
					OpenStream(m_CurrentVideo, st.demuxerId, st.id, st.source);
					CDVDMsgPlayerSeek::CMode mode;
					mode.time = (int)GetTime();
					mode.backward = true;
					mode.flush = true;
					mode.accurate = true;
					mode.trickplay = true;
					mode.sync = true;
					m_messenger.Put(new CDVDMsgPlayerSeek(mode));
				}
			}
#if 0
		} else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM)) {
			CDVDMsgPlayerSetSubtitleStream* pMsg2 = (CDVDMsgPlayerSetSubtitleStream*)pMsg;

			SelectionStream& st = m_SelectionStreams.Get(STREAM_SUBTITLE, pMsg2->GetStreamId());
			if (st.source != STREAM_SOURCE_NONE)
			{
				if (st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
				{
					CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
					if (pStream->SetActiveSubtitleStream(st.id))
					{
						m_dvd.iSelectedSPUStream = -1;
						CloseStream(m_CurrentSubtitle, false);
					}
				} else
				{
					CloseStream(m_CurrentSubtitle, false);
					OpenStream(m_CurrentSubtitle, st.demuxerId, st.id, st.source);
				}
			}
		} else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE))
		{
			CDVDMsgBool* pValue = (CDVDMsgBool*)pMsg;
			SetSubtitleVisibleInternal(pValue->m_value);
		} else if (pMsg->IsType(CDVDMsg::PLAYER_SET_STATE)) {
			g_infoManager.SetDisplayAfterSeek(100000);
			SetCaching(CACHESTATE_FLUSH);

			CDVDMsgPlayerSetState* pMsgPlayerSetState = (CDVDMsgPlayerSetState*)pMsg;

			if (CDVDInputStream::IMenus* ptr = dynamic_cast<CDVDInputStream::IMenus*>(m_pInputStream))
			{
				if (ptr->SetState(pMsgPlayerSetState->GetState()))
				{
					m_dvd.state = DVDSTATE_NORMAL;
					m_dvd.iDVDStillStartTime = 0;
					m_dvd.iDVDStillTime = 0;
				}
			}

			g_infoManager.SetDisplayAfterSeek();
		} else if (pMsg->IsType(CDVDMsg::PLAYER_SET_RECORD))
		{
			CDVDInputStreamPVRManager* input = dynamic_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
			if (input)
				input->Record(*(CDVDMsgBool*)pMsg);
#endif
		} else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) {
			FlushBuffers(false);
		} else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED)) {
			int speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;

			// correct our current clock, as it would start going wrong otherwise
			if (m_State.timestamp > 0)
			{
				double offset;
				offset = m_clock.GetAbsoluteClock() - m_State.timestamp;
				offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
				offset = DVD_TIME_TO_MSEC(offset);
				if (offset > 1000)
					offset = 1000;
				if (offset < -1000)
					offset = -1000;
				m_State.time += offset;
				m_State.timestamp = m_clock.GetAbsoluteClock();
			}

			// 			if (speed != DVD_PLAYSPEED_PAUSE && m_playSpeed != DVD_PLAYSPEED_PAUSE && speed != m_playSpeed)
			// 				m_callback.OnPlayBackSpeedChanged(speed / DVD_PLAYSPEED_NORMAL);

			// notifiy GUI, skins may want to show the seekbar
			//			g_infoManager.SetDisplayAfterSeek();

			// do a seek after rewind, clock is not in sync with current pts
			if ((speed == DVD_PLAYSPEED_NORMAL) &&
				(m_playSpeed != DVD_PLAYSPEED_NORMAL) &&
				(m_playSpeed != DVD_PLAYSPEED_PAUSE)
				/* && !m_omxplayer_mode*/)
			{
				int64_t iTime = (int64_t)DVD_TIME_TO_MSEC(m_clock.GetClock() + m_State.time_offset);
				if (m_State.time != DVD_NOPTS_VALUE)
					iTime = m_State.time;

				CDVDMsgPlayerSeek::CMode mode;
				mode.time = iTime;
				mode.backward = m_playSpeed < 0;
				mode.flush = true;
				mode.accurate = false;
				mode.trickplay = true;
				mode.sync = true;
				m_messenger.Put(new CDVDMsgPlayerSeek(mode));
			}
#ifdef HAS_OMXPLAYER
			// !!! omx alterative code path !!!
			// should be done differently
			if (m_omxplayer_mode)
			{
				// when switching from trickplay to normal, we may not have a full set of reference frames
				// in decoder and we may get corrupt frames out. Seeking to current time will avoid this.
				if ((speed != DVD_PLAYSPEED_PAUSE && speed != DVD_PLAYSPEED_NORMAL) ||
					(m_playSpeed != DVD_PLAYSPEED_PAUSE && m_playSpeed != DVD_PLAYSPEED_NORMAL))
				{
					CDVDMsgPlayerSeek::CMode mode;
					mode.time = (int)GetTime();
					mode.backward = (speed < 0);
					mode.flush = true;
					mode.accurate = true;
					mode.restore = false;
					mode.trickplay = true;
					mode.sync = true;
					m_messenger.Put(new CDVDMsgPlayerSeek(mode));
				} else
				{
					m_OmxPlayerState.av_clock.OMXPause();
				}

				m_OmxPlayerState.av_clock.OMXSetSpeed(speed);
				CLog::Log(LOGDEBUG, "%s::%s CDVDMsg::PLAYER_SETSPEED speed : %d (%d)", "CVideoPlayer", __FUNCTION__, speed, static_cast<int>(m_playSpeed));
			}
#endif
			m_playSpeed = speed;
			m_newPlaySpeed = speed;
			m_caching = CACHESTATE_DONE;
			m_clock.SetSpeed(speed);
			m_VideoPlayerAudio->SetSpeed(speed);
			m_VideoPlayerVideo->SetSpeed(speed);
			m_streamPlayerSpeed = speed;
			if (m_pDemuxer)
				m_pDemuxer->SetSpeed(speed);
#if 0
		} else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_SELECT_NUMBER) &&
			m_messenger.GetPacketCount(CDVDMsg::PLAYER_CHANNEL_SELECT_NUMBER) == 0)
		{
			FlushBuffers(false);
			CDVDInputStreamPVRManager* input = dynamic_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
			//! @todo find a better solution for the "otherStreaHack"
			//! a stream is not sopposed to be terminated before demuxer
			if (input && input->IsOtherStreamHack())
			{
				CloseDemuxer();
			}
			if (input && input->SelectChannelByNumber(static_cast<CDVDMsgInt*>(pMsg)->m_value))
			{
				CloseDemuxer();
				m_playSpeed = DVD_PLAYSPEED_NORMAL;

				// when using fast channel switching some shortcuts are taken which
				// means we'll have to update the view mode manually
				m_renderManager.SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);
			} else
			{
				CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
				CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_STOP);
			}
			ShowPVRChannelInfo();
		} else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_SELECT) &&
			m_messenger.GetPacketCount(CDVDMsg::PLAYER_CHANNEL_SELECT) == 0)
		{
			FlushBuffers(false);
			CDVDInputStreamPVRManager* input = dynamic_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
			if (input && input->IsOtherStreamHack())
			{
				CloseDemuxer();
			}
			if (input && input->SelectChannel(static_cast<CDVDMsgType <CPVRChannelPtr> *>(pMsg)->m_value))
			{
				CloseDemuxer();
				m_playSpeed = DVD_PLAYSPEED_NORMAL;
			} else
			{
				CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
				CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_STOP);
			}
			g_PVRManager.SetChannelPreview(false);
			ShowPVRChannelInfo();
		} else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREV) ||
			pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_PREV))
		{
			CDVDInputStreamPVRManager* input = dynamic_cast<CDVDInputStreamPVRManager*>(m_pInputStream);
			if (input)
			{
				bool bSwitchSuccessful(false);
				bool bShowPreview(pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_NEXT) ||
					pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_PREV) ||
					CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT) > 0);

				if (!bShowPreview)
				{
					g_infoManager.SetDisplayAfterSeek(100000);
					FlushBuffers(false);
					if (input->IsOtherStreamHack())
					{
						CloseDemuxer();
					}
				}

				if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_NEXT))
					bSwitchSuccessful = input->NextChannel(bShowPreview);
				else
					bSwitchSuccessful = input->PrevChannel(bShowPreview);

				if (bSwitchSuccessful)
				{
					if (bShowPreview)
					{
						UpdateApplication(0);

						if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREVIEW_PREV))
							m_ChannelEntryTimeOut.SetInfinite();
						else
							m_ChannelEntryTimeOut.Set(CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT));
					} else
					{
						m_ChannelEntryTimeOut.SetInfinite();
						CloseDemuxer();
						m_playSpeed = DVD_PLAYSPEED_NORMAL;

						g_infoManager.SetDisplayAfterSeek();
						g_PVRManager.SetChannelPreview(false);

						// when using fast channel switching some shortcuts are taken which
						// means we'll have to update the view mode manually
						m_renderManager.SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);
					}
					ShowPVRChannelInfo();
				} else
				{
					CLog::Log(LOGWARNING, "%s - failed to switch channel. playback stopped", __FUNCTION__);
					CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_STOP);
				}
			}
		} else if (pMsg->IsType(CDVDMsg::GENERAL_GUI_ACTION))
			OnAction(((CDVDMsgType<CAction>*)pMsg)->m_value);
#endif
		} else if (pMsg->IsType(CDVDMsg::PLAYER_STARTED)) {
			SStartMsg& msg = ((CDVDMsgType<SStartMsg>*)pMsg)->m_value;
			if (msg.player == VideoPlayer_AUDIO)
			{
				m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_WAITSYNC;
				m_CurrentAudio.cachetime = msg.cachetime;
				m_CurrentAudio.cachetotal = msg.cachetotal;
				m_CurrentAudio.starttime = msg.timestamp;
			}
			if (msg.player == VideoPlayer_VIDEO)
			{
				m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_WAITSYNC;
				m_CurrentVideo.cachetime = msg.cachetime;
				m_CurrentVideo.cachetotal = msg.cachetotal;
				m_CurrentVideo.starttime = msg.timestamp;
			}
//			CLog::Log(LOGDEBUG, "CVideoPlayer::HandleMessages - player started %d", msg.player);
#if 0
		} else if (pMsg->IsType(CDVDMsg::SUBTITLE_ADDFILE))
		{
			int id = AddSubtitleFile(((CDVDMsgType<std::string>*) pMsg)->m_value);
			if (id >= 0)
			{
				SetSubtitle(id);
				SetSubtitleVisibleInternal(true);
			}
#endif
		} else if (pMsg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE)) {
			((CDVDMsgGeneralSynchronize*)pMsg)->Wait(100, SYNCSOURCE_PLAYER);
// 			if ()
// 				CLog::Log(LOGDEBUG, "CVideoPlayer - CDVDMsg::GENERAL_SYNCHRONIZE");
		} else if (pMsg->IsType(CDVDMsg::PLAYER_AVCHANGE)) {
			UpdateStreamInfos();
// 			CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
// 			CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
		}

		pMsg->Release();
	}
}

void BasePlayer::SetCaching(ECacheState state)
{
	if (state == CACHESTATE_FLUSH)
	{
		double level, delay, offset;
		if (GetCachingTimes(level, delay, offset))
			state = CACHESTATE_FULL;
		else
			state = CACHESTATE_INIT;
	}

	if (m_caching == state)
		return;

//	CLog::Log(LOGDEBUG, "CVideoPlayer::SetCaching - caching state %d", state);
	if (state == CACHESTATE_FULL ||
		state == CACHESTATE_INIT)
	{
		m_clock.SetSpeed(DVD_PLAYSPEED_PAUSE);
#ifdef HAS_OMXPLAYER
		if (m_omxplayer_mode)
			m_OmxPlayerState.av_clock.OMXPause();
#endif
		m_VideoPlayerAudio->SetSpeed(DVD_PLAYSPEED_PAUSE);
		m_VideoPlayerVideo->SetSpeed(DVD_PLAYSPEED_PAUSE);
		m_streamPlayerSpeed = DVD_PLAYSPEED_PAUSE;

//		m_pInputStream->ResetScanTimeout((unsigned int)CSettings::GetInstance().GetInt(CSettings::SETTING_PVRPLAYBACK_SCANTIME) * 1000);

		m_cachingTimer.Set(5000);
	}

	if (state == CACHESTATE_PLAY ||
		(state == CACHESTATE_DONE && m_caching != CACHESTATE_PLAY))
	{
		m_clock.SetSpeed(m_playSpeed);
		m_VideoPlayerAudio->SetSpeed(m_playSpeed);
		m_VideoPlayerVideo->SetSpeed(m_playSpeed);
		m_streamPlayerSpeed = m_playSpeed;
//		m_pInputStream->ResetScanTimeout(0);
	}
	m_caching = state;

	m_clock.SetSpeedAdjust(0);
#ifdef HAS_OMXPLAYER
	if (m_omxplayer_mode)
		m_OmxPlayerState.av_clock.OMXSetSpeedAdjust(0);
#endif
}

double BasePlayer::GetQueueTime()
{
	int a = m_VideoPlayerAudio->GetLevel();
	int v = m_VideoPlayerVideo->GetLevel();
	return std::max(a, v) * 8000.0 / 100;
}

bool BasePlayer::GetCachingTimes(double& level, double& delay, double& offset)
{
	return false;
#if 0
	if (!m_pInputStream || !m_pDemuxer)
		return false;

	SCacheStatus status;
	if (!m_pInputStream->GetCacheStatus(&status))
		return false;

	uint64_t &cached = status.forward;
	unsigned &currate = status.currate;
	unsigned &maxrate = status.maxrate;
	float &cache_level = status.level;

	int64_t length = m_pInputStream->GetLength();
	int64_t remain = length - m_pInputStream->Seek(0, SEEK_CUR);

	if (length <= 0 || remain < 0)
		return false;

	double play_sbp = DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) / length;
	double queued = 1000.0 * GetQueueTime() / play_sbp;

	delay = 0.0;
	level = 0.0;
	offset = (double)(cached + queued) / length;

	if (currate == 0)
		return true;

	double cache_sbp = 1.1 * (double)DVD_TIME_BASE / currate;          /* underestimate by 10 % */
	double play_left = play_sbp  * (remain + queued);                  /* time to play out all remaining bytes */
	double cache_left = cache_sbp * (remain - cached);                 /* time to cache the remaining bytes */
	double cache_need = std::max(0.0, remain - play_left / cache_sbp); /* bytes needed until play_left == cache_left */

	delay = cache_left - play_left;

	/* NOTE: We can only reliably test for low readrate, when the cache is not
	* already *near* full. This is because as soon as it's full the average-
	* rate will become approximately the current-rate which can flag false
	* low read-rate conditions. To work around this we don't check the currate at 100%
	* but between 80% and 90%
	*/
	if (cache_level > 0.8 && cache_level < 0.9 && currate < maxrate)
	{
//		CLog::Log(LOGDEBUG, "Readrate %u is too low with %u required", currate, maxrate);
		level = -1.0;                          /* buffer is full & our read rate is too low  */
	} else
		level = (cached + queued) / (cache_need + queued);

	return true;
#endif
}

void BasePlayer::SetSpeed(double speed)
{
	if (!CanSeek())
		return;

	m_newPlaySpeed = speed * DVD_PLAYSPEED_NORMAL;
	SetPlaySpeed(speed * DVD_PLAYSPEED_NORMAL);
}

double BasePlayer::GetSpeed()
{
	if (m_playSpeed != m_newPlaySpeed)
		return (double)m_newPlaySpeed / DVD_PLAYSPEED_NORMAL;

	return (double)m_playSpeed / DVD_PLAYSPEED_NORMAL;
}

void BasePlayer::SeekTime(int64_t iTime)
{
	int seekOffset = (int)(iTime - GetTime());

	CDVDMsgPlayerSeek::CMode mode;
	mode.time = (int)iTime;
	mode.backward = true;
	mode.flush = true;
	mode.accurate = true;
	mode.trickplay = false;
	mode.sync = true;

	m_messenger.Put(new CDVDMsgPlayerSeek(mode));
	SynchronizeDemuxer();
	OnPlayBackSeek((int)iTime, seekOffset);
}

// return the time in milliseconds
int64_t BasePlayer::GetTime()
{
#if 0 // TODO delay tunning ?
	if (m_VideoPlayerAudio) {
		CDVDAudio *audioDev = m_VideoPlayerAudio->GetOutputDevice();
		if (audioDev && audioDev->m_pAudioStream) {
			iTVPSoundBuffer *alsound = audioDev->m_pAudioStream->GetNativeImpl();
			int remainSamples = alsound->GetUnprocessedSamples();
// 			double audio_clk = is->audio_clock - (double)remainSamples / is->audio_tgt.freq;
// 			set_clock_at(&is->audclk, audio_clk, is->audio_clock_serial, av_gettime() / 1000000.0);
// 			sync_clock_to_slave(&is->extclk, &is->audclk);
		}
	}
#endif
	CSingleLock lock(m_StateSection);
	double offset = 0;
	const double limit = DVD_MSEC_TO_TIME(500);
	if (m_State.timestamp > 0)
	{
		offset = m_clock.GetAbsoluteClock() - m_State.timestamp;
		offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
		if (offset > limit)
			offset = limit;
		if (offset < -limit)
			offset = -limit;
	}
	return llrint(m_State.time + DVD_TIME_TO_MSEC(offset));
}

int BasePlayer::GetCurrentFrame()
{
	// TODO accuracy
	return GetTime() * GetFPS() / DVD_PLAYSPEED_NORMAL;
}

int BasePlayer::GetVideoStream()
{
	return m_SelectionStreams.IndexOf(STREAM_VIDEO, m_CurrentVideo.source, m_CurrentVideo.demuxerId, m_CurrentVideo.id);
}

int BasePlayer::GetVideoStreamCount()
{
	return m_SelectionStreams.Count(STREAM_VIDEO);
}

int BasePlayer::GetAudioStreamCount()
{
	return m_SelectionStreams.Count(STREAM_AUDIO);
}

int BasePlayer::GetAudioStream()
{
	return m_SelectionStreams.IndexOf(STREAM_AUDIO, m_CurrentAudio.source, m_CurrentAudio.demuxerId, m_CurrentAudio.id);
}

void BasePlayer::HandlePlaySpeed()
{
// 	bool isInMenu = IsInMenuInternal();
// 
// 	if (isInMenu && m_caching != CACHESTATE_DONE)
// 		SetCaching(CACHESTATE_DONE);

	if (m_caching == CACHESTATE_FULL)
	{
		double level, delay, offset;
		if (GetCachingTimes(level, delay, offset))
		{
			if (level < 0.0)
			{
			//	CGUIDialogKaiToast::QueueNotification(g_localizeStrings.Get(21454), g_localizeStrings.Get(21455));
				SetCaching(CACHESTATE_INIT);
			}
			if (level >= 1.0)
				SetCaching(CACHESTATE_INIT);
		} else
		{
			if ((!m_VideoPlayerAudio->AcceptsData() && m_CurrentAudio.id >= 0) ||
				(!m_VideoPlayerVideo->AcceptsData() && m_CurrentVideo.id >= 0))
				SetCaching(CACHESTATE_INIT);
		}
	}

	if (m_caching == CACHESTATE_INIT)
	{
		// if all enabled streams have been inited we are done
		if ((m_CurrentVideo.id >= 0 || m_CurrentAudio.id >= 0) &&
			(m_CurrentVideo.id < 0 || m_CurrentVideo.syncState != IDVDStreamPlayer::SYNC_STARTING) &&
			(m_CurrentAudio.id < 0 || m_CurrentAudio.syncState != IDVDStreamPlayer::SYNC_STARTING))
			SetCaching(CACHESTATE_PLAY);

		// handle exceptions
		if (m_CurrentAudio.id >= 0 && m_CurrentVideo.id >= 0)
		{
			if ((!m_VideoPlayerAudio->AcceptsData() || !m_VideoPlayerVideo->AcceptsData()) &&
				m_cachingTimer.IsTimePast())
			{
				SetCaching(CACHESTATE_DONE);
			}
		}
	}

	if (m_caching == CACHESTATE_PLAY)
	{
		// if all enabled streams have started playing we are done
		if ((m_CurrentVideo.id < 0 || !m_VideoPlayerVideo->IsStalled()) &&
			(m_CurrentAudio.id < 0 || !m_VideoPlayerAudio->IsStalled()))
			SetCaching(CACHESTATE_DONE);
	}

	if (m_caching == CACHESTATE_DONE)
	{
		if (m_playSpeed == DVD_PLAYSPEED_NORMAL /*&& !isInMenu*/)
		{
			// take action is audio or video stream is stalled
			if (((m_VideoPlayerAudio->IsStalled() && m_CurrentAudio.inited) ||
				(m_VideoPlayerVideo->IsStalled() && m_CurrentVideo.inited)) &&
				m_syncTimer.IsTimePast())
			{
// 				if (m_pInputStream->IsRealtime())
// 				{
// 					if ((m_CurrentAudio.id >= 0 && m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC && m_VideoPlayerAudio->IsStalled()) ||
// 						(m_CurrentVideo.id >= 0 && m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_INSYNC && m_VideoPlayerVideo->GetLevel() == 0))
// 					{
// 						CLog::Log(LOGDEBUG, "Stream stalled, start buffering. Audio: %d - Video: %d",
// 							m_VideoPlayerAudio->GetLevel(), m_VideoPlayerVideo->GetLevel());
// 						FlushBuffers(false);
// 					}
// 				} else
				{
					// start caching if audio and video have run dry
					if (m_VideoPlayerAudio->GetLevel() <= 50 &&
						m_VideoPlayerVideo->GetLevel() <= 50)
					{
						SetCaching(CACHESTATE_FULL);
					} else if (m_CurrentAudio.id >= 0 && m_CurrentAudio.inited &&
						m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_INSYNC &&
						m_VideoPlayerAudio->GetLevel() == 0)
					{
					//	CLog::Log(LOGDEBUG, "CVideoPlayer::HandlePlaySpeed - audio stream stalled, triggering re-sync");
						FlushBuffers(false);
						CDVDMsgPlayerSeek::CMode mode;
						mode.time = (int)GetTime();
						mode.backward = false;
						mode.flush = true;
						mode.accurate = true;
						mode.sync = true;
						m_messenger.Put(new CDVDMsgPlayerSeek(mode));
					}
				}
			}
			// care for live streams
// 			else if (m_pInputStream->IsRealtime())
// 			{
// 				if (m_CurrentAudio.id >= 0)
// 				{
// 					double adjust = -1.0; // a unique value
// 					if (m_clock.GetSpeedAdjust() >= 0 && m_VideoPlayerAudio->GetLevel() < 5)
// 						adjust = -0.05;
// 
// 					if (m_clock.GetSpeedAdjust() < 0 && m_VideoPlayerAudio->GetLevel() > 10)
// 						adjust = 0.0;
// 
// 					if (adjust != -1.0)
// 					{
// 						m_clock.SetSpeedAdjust(adjust);
// 						if (m_omxplayer_mode)
// 							m_OmxPlayerState.av_clock.OMXSetSpeedAdjust(adjust);
// 					}
// 				}
// 			}
		}
	}

	// sync streams to clock
	if ((m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC) ||
		(m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC))
	{
		unsigned int threshold = 20;
// 		if (m_pInputStream->IsRealtime())
// 			threshold = 40;

		bool video = m_CurrentVideo.id < 0 || (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC) ||
			(m_CurrentVideo.packets == 0 && m_CurrentAudio.packets > threshold);
		bool audio = m_CurrentAudio.id < 0 || (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC) ||
			(m_CurrentAudio.packets == 0 && m_CurrentVideo.packets > threshold);

		if (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC &&
			m_CurrentAudio.avsync == CCurrentStream::AV_SYNC_CONT)
		{
			m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_INSYNC;
			m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_NONE;
			m_VideoPlayerAudio->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, m_clock.GetClock()), 1);
		} else if (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC &&
			m_CurrentVideo.avsync == CCurrentStream::AV_SYNC_CONT)
		{
			m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_INSYNC;
			m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_NONE;
			m_VideoPlayerVideo->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, m_clock.GetClock()), 1);
		} else if (video && audio)
		{
			double clock = 0;
// 			if (m_CurrentAudio.syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
// 				CLog::Log(LOGDEBUG, "VideoPlayer::Sync - Audio - pts: %f, cache: %f, totalcache: %f",
// 				m_CurrentAudio.starttime, m_CurrentAudio.cachetime, m_CurrentAudio.cachetotal);
// 			if (m_CurrentVideo.syncState == IDVDStreamPlayer::SYNC_WAITSYNC)
// 				CLog::Log(LOGDEBUG, "VideoPlayer::Sync - Video - pts: %f, cache: %f, totalcache: %f",
// 				m_CurrentVideo.starttime, m_CurrentVideo.cachetime, m_CurrentVideo.cachetotal);

			if (m_CurrentVideo.starttime != DVD_NOPTS_VALUE && m_CurrentVideo.packets > 0 &&
				m_playSpeed == DVD_PLAYSPEED_PAUSE)
			{
				clock = m_CurrentVideo.starttime;
			} else if (m_CurrentAudio.starttime != DVD_NOPTS_VALUE && m_CurrentAudio.packets > 0)
			{
// 				if (m_pInputStream->IsRealtime())
// 					clock = m_CurrentAudio.starttime - m_CurrentAudio.cachetotal - DVD_MSEC_TO_TIME(400);
// 				else
					clock = m_CurrentAudio.starttime - m_CurrentAudio.cachetime;
				if (m_CurrentVideo.starttime != DVD_NOPTS_VALUE &&
					(m_CurrentVideo.packets > 0) &&
					m_CurrentVideo.starttime - m_CurrentVideo.cachetotal < clock)
				{
					clock = m_CurrentVideo.starttime - m_CurrentVideo.cachetotal;
				}
			} else if (m_CurrentVideo.starttime != DVD_NOPTS_VALUE && m_CurrentVideo.packets > 0)
			{
				clock = m_CurrentVideo.starttime - m_CurrentVideo.cachetotal;
			}
#ifdef HAS_OMXPLAYER
			if (m_omxplayer_mode)
			{
				CLog::Log(LOGDEBUG, "%s::%s player started RESET", "CVideoPlayer", __FUNCTION__);
				m_OmxPlayerState.av_clock.OMXReset(m_CurrentVideo.id >= 0, m_playSpeed != DVD_PLAYSPEED_NORMAL && m_playSpeed != DVD_PLAYSPEED_PAUSE ? false : (m_CurrentAudio.id >= 0));
			}
#endif
			m_clock.Discontinuity(clock);
			m_CurrentAudio.syncState = IDVDStreamPlayer::SYNC_INSYNC;
			m_CurrentAudio.avsync = CCurrentStream::AV_SYNC_NONE;
			m_CurrentVideo.syncState = IDVDStreamPlayer::SYNC_INSYNC;
			m_CurrentVideo.avsync = CCurrentStream::AV_SYNC_NONE;
			m_VideoPlayerAudio->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, clock), 1);
			m_VideoPlayerVideo->SendMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_RESYNC, clock), 1);
			SetCaching(CACHESTATE_DONE);

			m_syncTimer.Set(3000);
		} else
		{
			// exceptions for which stream players won't start properly
			// 1. videoplayer has not detected a keyframe within lenght of demux buffers
			if (m_CurrentAudio.id >= 0 && m_CurrentVideo.id >= 0 &&
				!m_VideoPlayerAudio->AcceptsData() &&
				m_VideoPlayerVideo->IsStalled())
			{
			//	CLog::Log(LOGWARNING, "VideoPlayer::Sync - stream player video does not start, flushing buffers");
				FlushBuffers(false);
			}
		}
	}

	// handle ff/rw
	if (m_playSpeed != DVD_PLAYSPEED_NORMAL && m_playSpeed != DVD_PLAYSPEED_PAUSE)
	{
// 		if (isInMenu)
// 		{
// 			// this can't be done in menu
// 			SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
// 
// 		} else
		{
			bool check = true;

			// only check if we have video
			if (m_CurrentVideo.id < 0 || m_CurrentVideo.syncState != IDVDStreamPlayer::SYNC_INSYNC)
				check = false;
			// video message queue either initiated or already seen eof
			else if (m_CurrentVideo.inited == false && m_playSpeed >= 0)
				check = false;
			// don't check if time has not advanced since last check
			else if (m_SpeedState.lasttime == GetTime())
				check = false;
			// skip if frame at screen has no valid timestamp
			else if (m_VideoPlayerVideo->GetCurrentPts() == DVD_NOPTS_VALUE)
				check = false;
			// skip if frame on screen has not changed
			else if (m_SpeedState.lastpts == m_VideoPlayerVideo->GetCurrentPts() &&
				(m_SpeedState.lastpts > m_State.dts || m_playSpeed > 0))
				check = false;

			if (check)
			{
				m_SpeedState.lastpts = m_VideoPlayerVideo->GetCurrentPts();
				m_SpeedState.lasttime = GetTime();
				m_SpeedState.lastabstime = m_clock.GetAbsoluteClock();
				// check how much off clock video is when ff/rw:ing
				// a problem here is that seeking isn't very accurate
				// and since the clock will be resynced after seek
				// we might actually not really be playing at the wanted
				// speed. we'd need to have some way to not resync the clock
				// after a seek to remember timing. still need to handle
				// discontinuities somehow

				double error;
				error = m_clock.GetClock() - m_SpeedState.lastpts;
				error *= m_playSpeed / abs(m_playSpeed);

				// allow a bigger error when going ff, the faster we go
				// the the bigger is the error we allow
				if (m_playSpeed > DVD_PLAYSPEED_NORMAL)
				{
					int errorwin = m_playSpeed / DVD_PLAYSPEED_NORMAL;
					if (errorwin > 8)
						errorwin = 8;
					error /= errorwin;
				}

				if (error > DVD_MSEC_TO_TIME(1000))
				{
					error = (int)DVD_TIME_TO_MSEC(m_clock.GetClock()) - m_SpeedState.lastseekpts;

					if (std::abs(error) > 1000 || (m_VideoPlayerVideo->IsRewindStalled() && std::abs(error) > 100))
					{
					//	CLog::Log(LOGDEBUG, "CVideoPlayer::Process - Seeking to catch up, error was: %f", error);
						m_SpeedState.lastseekpts = (int)DVD_TIME_TO_MSEC(m_clock.GetClock());
						int direction = (m_playSpeed > 0) ? 1 : -1;
						int iTime = DVD_TIME_TO_MSEC(m_clock.GetClock() + m_State.time_offset + 1000000.0 * direction);
						CDVDMsgPlayerSeek::CMode mode;
						mode.time = iTime;
						mode.backward = (GetPlaySpeed() < 0);
						mode.flush = true;
						mode.accurate = false;
						mode.restore = false;
						mode.trickplay = true;
						mode.sync = false;
						m_messenger.Put(new CDVDMsgPlayerSeek(mode));
					}
				}
			}
		}
	}
}

void BasePlayer::SynchronizeDemuxer()
{
	if (IsCurrentThread())
		return;
	if (!m_messenger.IsInited())
		return;

	CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(500, SYNCSOURCE_PLAYER);
	m_messenger.Put(message->AddRef());
	message->Wait(m_bStop, 0);
	message->Release();
}

static void UpdateLimits(double& minimum, double& maximum, double dts)
{
	if (dts == DVD_NOPTS_VALUE)
		return;
	if (minimum == DVD_NOPTS_VALUE || minimum > dts) minimum = dts;
	if (maximum == DVD_NOPTS_VALUE || maximum < dts) maximum = dts;
}

bool BasePlayer::CheckContinuity(CCurrentStream& current, DemuxPacket* pPacket)
{
	if (m_playSpeed < DVD_PLAYSPEED_PAUSE)
		return false;

	if (pPacket->dts == DVD_NOPTS_VALUE || current.dts == DVD_NOPTS_VALUE)
		return false;

	double mindts = DVD_NOPTS_VALUE, maxdts = DVD_NOPTS_VALUE;
	UpdateLimits(mindts, maxdts, m_CurrentAudio.dts);
	UpdateLimits(mindts, maxdts, m_CurrentVideo.dts);
	UpdateLimits(mindts, maxdts, m_CurrentAudio.dts_end());
	UpdateLimits(mindts, maxdts, m_CurrentVideo.dts_end());

	/* if we don't have max and min, we can't do anything more */
	if (mindts == DVD_NOPTS_VALUE || maxdts == DVD_NOPTS_VALUE)
		return false;

	double correction = 0.0;
	if (pPacket->dts > maxdts + DVD_MSEC_TO_TIME(1000))
	{
// 		CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - resync forward :%d, prev:%f, curr:%f, diff:%f"
// 			, current.type, current.dts, pPacket->dts, pPacket->dts - maxdts);
		correction = pPacket->dts - maxdts;
	}

	/* if it's large scale jump, correct for it after having confirmed the jump */
	if (pPacket->dts + DVD_MSEC_TO_TIME(500) < current.dts_end())
	{
// 		CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - resync backward :%d, prev:%f, curr:%f, diff:%f"
// 			, current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);
		correction = pPacket->dts - current.dts_end();
	} else if (pPacket->dts < current.dts)
	{
// 		CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - wrapback :%d, prev:%f, curr:%f, diff:%f"
// 			, current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);
	}

	double lastdts = pPacket->dts;
	if (correction != 0.0)
	{
		// we want the dts values of two streams to close, or for one to be invalid (e.g. from a missing audio stream)
		double this_dts = pPacket->dts;
		double that_dts = current.type == STREAM_AUDIO ? m_CurrentVideo.lastdts : m_CurrentAudio.lastdts;

		if (m_CurrentAudio.id == -1 || m_CurrentVideo.id == -1 ||
			current.lastdts == DVD_NOPTS_VALUE ||
			fabs(this_dts - that_dts) < DVD_MSEC_TO_TIME(1000))
		{
			m_offset_pts += correction;
			UpdateCorrection(pPacket, correction);
			lastdts = pPacket->dts;
		//	CLog::Log(LOGDEBUG, "CVideoPlayer::CheckContinuity - update correction: %f", correction);
		} else
		{
			// not sure yet - flags the packets as unknown until we get confirmation on another audio/video packet
			pPacket->dts = DVD_NOPTS_VALUE;
			pPacket->pts = DVD_NOPTS_VALUE;
		}
	} else
	{
		if (current.avsync == CCurrentStream::AV_SYNC_CHECK)
			current.avsync = CCurrentStream::AV_SYNC_CONT;
	}
	current.lastdts = lastdts;
	return true;
}


bool BasePlayer::CheckSceneSkip(CCurrentStream& current)
{
	return false;
#if 0
	if (!m_Edl.HasCut())
		return false;

	if (current.dts == DVD_NOPTS_VALUE)
		return false;

	if (current.inited == false)
		return false;

	CEdl::Cut cut;
	return m_Edl.InCut(DVD_TIME_TO_MSEC(current.dts + m_offset_pts), &cut) && cut.action == CEdl::CUT;
#endif
}

bool BasePlayer::CheckPlayerInit(CCurrentStream& current)
{
	if (current.inited)
		return false;

	if (current.startpts != DVD_NOPTS_VALUE)
	{
		if (current.dts == DVD_NOPTS_VALUE)
		{
		//	CLog::Log(LOGDEBUG, "%s - dropping packet type:%d dts:%f to get to start point at %f", __FUNCTION__, current.player, current.dts, current.startpts);
			return true;
		}

		if ((current.startpts - current.dts) > DVD_SEC_TO_TIME(20))
		{
		//	CLog::Log(LOGDEBUG, "%s - too far to decode before finishing seek", __FUNCTION__);
			if (m_CurrentAudio.startpts != DVD_NOPTS_VALUE)
				m_CurrentAudio.startpts = current.dts;
			if (m_CurrentVideo.startpts != DVD_NOPTS_VALUE)
				m_CurrentVideo.startpts = current.dts;
// 			if (m_CurrentSubtitle.startpts != DVD_NOPTS_VALUE)
// 				m_CurrentSubtitle.startpts = current.dts;
// 			if (m_CurrentTeletext.startpts != DVD_NOPTS_VALUE)
// 				m_CurrentTeletext.startpts = current.dts;
// 			if (m_CurrentRadioRDS.startpts != DVD_NOPTS_VALUE)
// 				m_CurrentRadioRDS.startpts = current.dts;
		}

		if (current.dts < current.startpts)
		{
		//	CLog::Log(LOGDEBUG, "%s - dropping packet type:%d dts:%f to get to start point at %f", __FUNCTION__, current.player, current.dts, current.startpts);
			return true;
		}
	}

	if (current.dts != DVD_NOPTS_VALUE)
	{
		current.inited = true;
		current.startpts = current.dts;
	}
	return false;
}

void BasePlayer::UpdateCorrection(DemuxPacket* pkt, double correction)
{
	if (pkt->dts != DVD_NOPTS_VALUE)
		pkt->dts -= correction;
	if(pkt->pts != DVD_NOPTS_VALUE)
		pkt->pts -= correction;
}

void BasePlayer::UpdateTimestamps(CCurrentStream& current, DemuxPacket* pPacket)
{
	double dts = current.dts;
	/* update stored values */
	if (pPacket->dts != DVD_NOPTS_VALUE)
		dts = pPacket->dts;
	else if (pPacket->pts != DVD_NOPTS_VALUE)
		dts = pPacket->pts;

	/* calculate some average duration */
	if (pPacket->duration != DVD_NOPTS_VALUE)
		current.dur = pPacket->duration;
	else if (dts != DVD_NOPTS_VALUE && current.dts != DVD_NOPTS_VALUE)
		current.dur = 0.1 * (current.dur * 9 + (dts - current.dts));

	current.dts = dts;

	current.dispTime = pPacket->dispTime;
}

IDVDStreamPlayer* BasePlayer::GetStreamPlayer(unsigned int target)
{
	if (target == VideoPlayer_AUDIO)
		return m_VideoPlayerAudio;
	if (target == VideoPlayer_VIDEO)
		return m_VideoPlayerVideo;
// 	if (target == VideoPlayer_SUBTITLE)
// 		return m_VideoPlayerSubtitle;
// 	if (target == VideoPlayer_TELETEXT)
// 		return m_VideoPlayerTeletext;
// 	if (target == VideoPlayer_RDS)
// 		return m_VideoPlayerRadioRDS;
	return NULL;
}

void BasePlayer::SendPlayerMessage(CDVDMsg* pMsg, unsigned int target)
{
	IDVDStreamPlayer* player = GetStreamPlayer(target);
	if(player)
		player->SendMessage(pMsg, 0);
}

bool BasePlayer::ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream)
{
	// check if we should read from subtitle demuxer
#if 0
	if (m_pSubtitleDemuxer && m_VideoPlayerSubtitle->AcceptsData())
	{
		packet = m_pSubtitleDemuxer->Read();

		if (packet)
		{
			UpdateCorrection(packet, m_offset_pts);
			if (packet->iStreamId < 0)
				return true;

			stream = m_pSubtitleDemuxer->GetStream(packet->demuxerId, packet->iStreamId);
			if (!stream)
			{
				CLog::Log(LOGERROR, "%s - Error demux packet doesn't belong to a valid stream", __FUNCTION__);
				return false;
			}
			if (stream->source == STREAM_SOURCE_NONE)
			{
				m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX_SUB);
				m_SelectionStreams.Update(NULL, m_pSubtitleDemuxer);
			}
			return true;
		}
	}
#endif
#ifdef HAS_OMXPLAYER
	if (m_omxplayer_mode)
	{
		// reset eos state when we get a packet (e.g. for case of seek after eos)
		if (packet && stream)
		{
			m_OmxPlayerState.bOmxWaitVideo = false;
			m_OmxPlayerState.bOmxWaitAudio = false;
			m_OmxPlayerState.bOmxSentEOFs = false;
		}
	}
#endif
	// read a data frame from stream.
	if (m_pDemuxer)
		packet = m_pDemuxer->Read();

	if (packet)
	{
		// stream changed, update and open defaults
		if (packet->iStreamId == DMX_SPECIALID_STREAMCHANGE)
		{
			m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
			m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
			OpenDefaultStreams(false);

			// reevaluate HasVideo/Audio, we may have switched from/to a radio channel
			if (m_CurrentVideo.id < 0)
				m_HasVideo = false;
			if (m_CurrentAudio.id < 0)
				m_HasAudio = false;

			return true;
		}

		UpdateCorrection(packet, m_offset_pts);

		if (packet->iStreamId < 0)
			return true;

		if (m_pDemuxer)
		{
			stream = m_pDemuxer->GetStream(packet->demuxerId, packet->iStreamId);
			if (!stream)
			{
//				CLog::Log(LOGERROR, "%s - Error demux packet doesn't belong to a valid stream", __FUNCTION__);
				return false;
			}
			if (stream->source == STREAM_SOURCE_NONE)
			{
				m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
				m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
			}
		}
		return true;
	}
	return false;
}

void BasePlayer::OpenInputStream(IStream *stream, const std::string& filename)
{
	SAFE_RELEASE(m_pInputStream);
// 	if (m_pInputStream)
// 		delete m_pInputStream;

//	CLog::Log(LOGNOTICE, "Creating InputStream");

	// correct the filename if needed
//	std::string filename(m_item.GetPath());

	m_pInputStream = new InputStream(stream, filename);
	
//	SetAVDelay(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioDelay);
//	SetSubTitleDelay(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleDelay);
	m_clock.Reset();
//	m_dvd.Clear();
	m_errorCount = 0;
	m_ChannelEntryTimeOut.SetInfinite();
}

bool BasePlayer::OpenDemuxStream()
{
	CloseDemuxer();

//	CLog::Log(LOGNOTICE, "Creating Demuxer");

//	m_pDemuxer = DemuxerFactory::CreateDemuxer(m_pInputStream);
	CDVDDemuxFFmpeg *demuxer = new CDVDDemuxFFmpeg;
	demuxer->Open(m_pInputStream);
	m_pDemuxer = demuxer;

// 	if (!m_pDemuxer)
// 	{
//		CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
// 		return false;
// 	}

	m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
//	m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NAV);
	m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);

// 	STATSTG strstat;
// 	m_pInputStream->Stat(&strstat, STATFLAG_NONAME);
// 
// 	int64_t len = strstat.cbSize.QuadPart;
// 	int64_t tim = m_pDemuxer->GetStreamLength();
// 	if (len > 0 && tim > 0)
// 		m_pInputStream->SetReadRate((unsigned int)(len * 1000 / tim));

	m_offset_pts = 0;

	return true;
}

void BasePlayer::CloseDemuxer()
{
	delete m_pDemuxer;
	m_pDemuxer = nullptr;
	m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);

//	CServiceBroker::GetDataCacheCore().SignalAudioInfoChange();
//	CServiceBroker::GetDataCacheCore().SignalVideoInfoChange();
}

static int GetCodecPriority(const std::string &codec)
{
	/*
	* Technically flac, truehd, and dtshd_ma are equivalently good as they're all lossless. However,
	* ffmpeg can't decode dtshd_ma losslessy yet.
	*/
	if (codec == "flac") // Lossless FLAC
		return 7;
	if (codec == "truehd") // Dolby TrueHD
		return 6;
	if (codec == "dtshd_ma") // DTS-HD Master Audio (previously known as DTS++)
		return 5;
	if (codec == "dtshd_hra") // DTS-HD High Resolution Audio
		return 4;
	if (codec == "eac3") // Dolby Digital Plus
		return 3;
	if (codec == "dca") // DTS
		return 2;
	if (codec == "ac3") // Dolby Digital
		return 1;
	return 0;
}

#define PREDICATE_RETURN(lh, rh) \
  do { \
    if((lh) != (rh)) \
      return (lh) > (rh); \
  } while(0)

static bool PredicateAudioPriority(const SelectionStream& lh, const SelectionStream& rh)
{
// 	PREDICATE_RETURN(lh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioStream
// 		, rh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioStream);

// 	if (!StringUtils::EqualsNoCase(CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_AUDIOLANGUAGE), "original"))
// 	{
// 		std::string audio_language = g_langInfo.GetAudioLanguage();
// 		PREDICATE_RETURN(g_LangCodeExpander.CompareISO639Codes(audio_language, lh.language)
// 			, g_LangCodeExpander.CompareISO639Codes(audio_language, rh.language));
// 
// 		bool hearingimp = CSettings::GetInstance().GetBool(CSettings::SETTING_ACCESSIBILITY_AUDIOHEARING);
// 		PREDICATE_RETURN(!hearingimp ? !(lh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED) : lh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED
// 			, !hearingimp ? !(rh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED) : rh.flags & CDemuxStream::FLAG_HEARING_IMPAIRED);
// 
// 		bool visualimp = CSettings::GetInstance().GetBool(CSettings::SETTING_ACCESSIBILITY_AUDIOVISUAL);
// 		PREDICATE_RETURN(!visualimp ? !(lh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED) : lh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED
// 			, !visualimp ? !(rh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED) : rh.flags & CDemuxStream::FLAG_VISUAL_IMPAIRED);
// 	}

// 	if (CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_PREFERDEFAULTFLAG))
// 	{
// 		PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
// 			, rh.flags & CDemuxStream::FLAG_DEFAULT);
// 	}

	PREDICATE_RETURN(lh.channels
		, rh.channels);

	PREDICATE_RETURN(GetCodecPriority(lh.codec)
		, GetCodecPriority(rh.codec));

	PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
		, rh.flags & CDemuxStream::FLAG_DEFAULT);

	return false;
}

static bool PredicateVideoPriority(const SelectionStream& lh, const SelectionStream& rh)
{
// 	PREDICATE_RETURN(lh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_VideoStream
// 		, rh.type_index == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_VideoStream);

	PREDICATE_RETURN(lh.flags & CDemuxStream::FLAG_DEFAULT
		, rh.flags & CDemuxStream::FLAG_DEFAULT);
	return false;
}

void BasePlayer::OpenDefaultStreams(bool reset)
{
	bool valid;

	// open video stream
	valid = false;

	for (const auto &stream : m_SelectionStreams.Get(STREAM_VIDEO, PredicateVideoPriority))
	{
		if (OpenStream(m_CurrentVideo, stream.demuxerId, stream.id, stream.source, reset))
		{
			valid = true;
			break;
		}
	}
	if (!valid)
	{
		CloseStream(m_CurrentVideo, true);
		m_processInfo->ResetVideoCodecInfo();
	}

	// open audio stream
	valid = false;
//	if (!m_PlayerOptions.video_only)
	{
		for (const auto &stream : m_SelectionStreams.Get(STREAM_AUDIO, PredicateAudioPriority))
		{
			if (OpenStream(m_CurrentAudio, stream.demuxerId, stream.id, stream.source, reset))
			{
				valid = true;
				break;
			}
		}
	}

	if (!valid)
	{
		CloseStream(m_CurrentAudio, true);
		m_processInfo->ResetAudioCodecInfo();
	}
#if 0
	// enable  or disable subtitles
	bool visible = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_SubtitleOn;

	// open subtitle stream
	SelectionStream as = m_SelectionStreams.Get(STREAM_AUDIO, GetAudioStream());
	PredicateSubtitlePriority psp(as.language);
	valid = false;
	CloseStream(m_CurrentSubtitle, false);
	for (const auto &stream : m_SelectionStreams.Get(STREAM_SUBTITLE, psp))
	{
		if (OpenStream(m_CurrentSubtitle, stream.demuxerId, stream.id, stream.source))
		{
			valid = true;
			if (!psp.relevant(stream))
				visible = false;
			else if (stream.flags & CDemuxStream::FLAG_FORCED)
				visible = true;
			break;
		}
	}
	if (!valid)
		CloseStream(m_CurrentSubtitle, false);

	if (!dynamic_cast<CDVDInputStreamNavigator*>(m_pInputStream) || m_PlayerOptions.state.empty())
		SetSubtitleVisibleInternal(visible); // only set subtitle visibility if state not stored by dvd navigator, because navigator will restore it (if visible)

	// open teletext stream
	valid = false;
	for (const auto &stream : m_SelectionStreams.Get(STREAM_TELETEXT))
	{
		if (OpenStream(m_CurrentTeletext, stream.demuxerId, stream.id, stream.source))
		{
			valid = true;
			break;
		}
	}
	if (!valid)
		CloseStream(m_CurrentTeletext, false);

	// open RDS stream
	valid = false;
	for (const auto &stream : m_SelectionStreams.Get(STREAM_RADIO_RDS))
	{
		if (OpenStream(m_CurrentRadioRDS, stream.demuxerId, stream.id, stream.source))
		{
			valid = true;
			break;
		}
	}
	if (!valid)
		CloseStream(m_CurrentRadioRDS, false);

	// disable demux streams
	if (m_item.IsRemote() && m_pDemuxer)
	{
		for (auto &stream : m_SelectionStreams.m_Streams)
		{
			if (STREAM_SOURCE_MASK(stream.source) == STREAM_SOURCE_DEMUX)
			{
				if (stream.id != m_CurrentVideo.id &&
					stream.id != m_CurrentAudio.id &&
					stream.id != m_CurrentSubtitle.id &&
					stream.id != m_CurrentTeletext.id &&
					stream.id != m_CurrentRadioRDS.id)
				{
					m_pDemuxer->EnableStream(stream.demuxerId, stream.id, false);
				}
			}
		}
	}
#endif
}

void BasePlayer::UpdatePlayState(double timeout)
{
	if (m_State.timestamp != 0 &&
		m_State.timestamp + DVD_MSEC_TO_TIME(timeout) > m_clock.GetAbsoluteClock())
		return;

	SPlayerState state(m_State);

	state.dts = DVD_NOPTS_VALUE;
	if (m_CurrentVideo.dts != DVD_NOPTS_VALUE)
		state.dts = m_CurrentVideo.dts;
	else if (m_CurrentAudio.dts != DVD_NOPTS_VALUE)
		state.dts = m_CurrentAudio.dts;
	else if (m_CurrentVideo.startpts != DVD_NOPTS_VALUE)
		state.dts = m_CurrentVideo.startpts;
	else if (m_CurrentAudio.startpts != DVD_NOPTS_VALUE)
		state.dts = m_CurrentAudio.startpts;

	if (m_pDemuxer)
	{
// 		if (IsInMenuInternal())
// 			state.chapter = 0;
// 		else
// 			state.chapter = m_pDemuxer->GetChapter();

//		state.chapters.clear();
//		if (m_pDemuxer->GetChapterCount() > 0)
// 		{
// 			for (int i = 0; i < m_pDemuxer->GetChapterCount(); ++i)
// 			{
// 				std::string name;
// 				m_pDemuxer->GetChapterName(name, i + 1);
// 				state.chapters.push_back(make_pair(name, m_pDemuxer->GetChapterPos(i + 1)));
// 			}
// 		}

		state.time = DVD_TIME_TO_MSEC(m_clock.GetClock(false));
		state.time_total = m_pDemuxer->GetStreamLength();
	}

	state.canpause = true;
	state.canseek = true;
	state.isInMenu = false;
	state.hasMenu = false;

	if (m_pInputStream)
	{
		// override from input stream if needed

// 		CDVDInputStream::IDisplayTime* pDisplayTime = m_pInputStream->GetIDisplayTime();
// 		if (pDisplayTime && pDisplayTime->GetTotalTime() > 0)
// 		{
// 			if (state.dts != DVD_NOPTS_VALUE)
// 			{
// 				int dispTime = 0;
// 				if (m_CurrentVideo.dispTime)
// 					dispTime = m_CurrentVideo.dispTime;
// 				else if (m_CurrentAudio.dispTime)
// 					dispTime = m_CurrentAudio.dispTime;
// 
// 				state.time_offset = DVD_MSEC_TO_TIME(dispTime) - state.dts;
// 			}
// 			state.time += DVD_TIME_TO_MSEC(state.time_offset);
// 			state.time_total = pDisplayTime->GetTotalTime();
// 		} else
		{
			state.time_offset = 0;
		}
		
		state.canpause = true;
		state.canseek = true;
	}

	if (state.time_total <= 0)
		state.canseek = false;

	if (m_caching > CACHESTATE_DONE && m_caching < CACHESTATE_PLAY)
		state.caching = true;
	else
		state.caching = false;

	double level, delay, offset;
	if (GetCachingTimes(level, delay, offset))
	{
		state.cache_delay = std::max(0.0, delay);
		state.cache_level = std::max(0.0, std::min(1.0, level));
		state.cache_offset = offset;
	} else
	{
		state.cache_delay = 0.0;
		state.cache_level = std::min(1.0, GetQueueTime() / 8000.0);
		state.cache_offset = GetQueueTime() / state.time_total;
	}
#if 0
	SCacheStatus status;
	if (m_pInputStream && m_pInputStream->GetCacheStatus(&status))
	{
		state.cache_bytes = status.forward;
		if (state.time_total)
			state.cache_bytes += m_pInputStream->GetLength() * (int64_t)(GetQueueTime() / state.time_total);
	} else
#endif
		state.cache_bytes = 0;

	state.timestamp = m_clock.GetAbsoluteClock();

	CSingleLock lock(m_StateSection);
	m_State = state;
}

void BasePlayer::UpdateStreamInfos()
{
	if (!m_pDemuxer)
		return;

	std::lock_guard<std::recursive_mutex> lock(m_SelectionStreams.m_section);
	int streamId;
	std::string retVal;

	// video
	streamId = GetVideoStream();

	if (streamId >= 0 && streamId < GetVideoStreamCount())
	{
		SelectionStream& s = m_SelectionStreams.Get(STREAM_VIDEO, streamId);
		s.bitrate = m_VideoPlayerVideo->GetVideoBitrate();
		s.aspect_ratio = m_renderManager.GetAspectRatio();
		CRect viewRect;
		m_renderManager.GetVideoRect(s.SrcRect, s.DestRect, viewRect);
		CDemuxStream* stream = m_pDemuxer->GetStream(m_CurrentVideo.demuxerId, m_CurrentVideo.id);
		if (stream && stream->type == STREAM_VIDEO)
		{
			{
				s.width = static_cast<CDemuxStreamVideo*>(stream)->iWidth;
				s.height = static_cast<CDemuxStreamVideo*>(stream)->iHeight;
			}
			s.stereo_mode = m_VideoPlayerVideo->GetStereoMode();
			if (s.stereo_mode == "mono")
				s.stereo_mode = "";
		}
	}

	// audio
	streamId = GetAudioStream();

	if (streamId >= 0 && streamId < GetAudioStreamCount())
	{
		SelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, streamId);
		s.bitrate = m_VideoPlayerAudio->GetAudioBitrate();
		s.channels = m_VideoPlayerAudio->GetAudioChannels();

		CDemuxStream* stream = m_pDemuxer->GetStream(m_CurrentAudio.demuxerId, m_CurrentAudio.id);
		if (stream && stream->type == STREAM_AUDIO)
		{
			s.codec = m_pDemuxer->GetStreamCodecName(stream->demuxerId, stream->uniqueId);
		}
	}
}

NS_KRMOVIE_END
