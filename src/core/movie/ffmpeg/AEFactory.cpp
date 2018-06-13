#include "AEFactory.h"
#include "AEStream.h"
#include "WaveMixer.h"
#include "Clock.h"
extern "C" {
#include <libswresample/swresample.h>
}
#include "Timer.h"
#include <algorithm>
#include <assert.h>

NS_KRMOVIE_BEGIN

static int64_t GetLayoutByChannels(int nChannel) {
	switch (nChannel) {
	case 1:
		return AV_CH_LAYOUT_MONO;
	case 2:
		return AV_CH_LAYOUT_STEREO;
	case 3:
		return AV_CH_LAYOUT_2POINT1;
	case 4:
		return AV_CH_LAYOUT_QUAD;
	case 5:
		return AV_CH_LAYOUT_5POINT0;
	case 6:
		return AV_CH_LAYOUT_5POINT1;
	case 7:
		return AV_CH_LAYOUT_6POINT1;
	case 8:
		return AV_CH_LAYOUT_7POINT1;
	default:
		return 0;
	}
}

class CAEStreamAL : public IAEStream {
	iTVPSoundBuffer *m_impl = nullptr;
	AEAudioFormat m_format;
	IAEClockCallback *m_cbClock;
	double m_lastPts = 0;
	struct SwrContext *swr_ctx = nullptr;
	AVSampleFormat swr_tgtFormat;
	unsigned int src_buffer_count = 0;
	unsigned int tgt_frameSize = 0;
	uint8_t *audio_buf = nullptr;
	unsigned int audio_buf_size = 0;
	std::mutex _mutex;
	std::condition_variable _cond;
	Timer _timer;

	int InitResample(AEAudioFormat &audioFormat) {
		uint64_t layout = GetLayoutByChannels(audioFormat.m_channelLayout.Count());
		AVSampleFormat srcFormat;
		switch (audioFormat.m_dataFormat) {
		case AE_FMT_U8: srcFormat = AV_SAMPLE_FMT_U8; break;
		case AE_FMT_S16LE: srcFormat = AV_SAMPLE_FMT_S16; break;
		case AE_FMT_S16NE: srcFormat = AV_SAMPLE_FMT_S16; break;
		case AE_FMT_S32LE: srcFormat = AV_SAMPLE_FMT_S32; break;
		case AE_FMT_S32NE: srcFormat = AV_SAMPLE_FMT_S32; break;
		case AE_FMT_DOUBLE: srcFormat = AV_SAMPLE_FMT_DBL; break;
		case AE_FMT_FLOAT: srcFormat = AV_SAMPLE_FMT_FLT; break;
		case AE_FMT_U8P: srcFormat = AV_SAMPLE_FMT_U8P; break;
		case AE_FMT_S16NEP: srcFormat = AV_SAMPLE_FMT_S16P; break;
		case AE_FMT_S32NEP: srcFormat = AV_SAMPLE_FMT_S32P; break;
		case AE_FMT_DOUBLEP: srcFormat = AV_SAMPLE_FMT_DBLP; break;
		case AE_FMT_FLOATP: srcFormat = AV_SAMPLE_FMT_FLTP; break;
		default: throw new std::invalid_argument("unknown sample format");
		}
		switch (audioFormat.m_dataFormat) {
		case AE_FMT_S16NEP: case AE_FMT_S32NEP: case AE_FMT_DOUBLEP: case AE_FMT_FLOATP:
			src_buffer_count = audioFormat.m_channelLayout.Count(); break;
		default: src_buffer_count = 1; break;
		}
		int bitsPerSample = 0;
		switch (audioFormat.m_dataFormat) {
		case AE_FMT_U8: case AE_FMT_U8P:
			swr_tgtFormat = AV_SAMPLE_FMT_U8;
			bitsPerSample = 8;
			break;
		default:
			swr_tgtFormat = AV_SAMPLE_FMT_S16;
			bitsPerSample = 16;
			break;
		}
		swr_ctx = swr_alloc_set_opts(NULL, layout, swr_tgtFormat, audioFormat.m_sampleRate,
			layout, srcFormat, audioFormat.m_sampleRate, 0, NULL);
		tgt_frameSize = av_get_bytes_per_sample(swr_tgtFormat) * m_format.m_channelLayout.Count();
		int result = swr_init(swr_ctx);
		assert(swr_ctx && result >= 0);
		return bitsPerSample;
	}

public:
	CAEStreamAL(AEAudioFormat &audioFormat, unsigned int options, IAEClockCallback *clock) {
		m_format = audioFormat;
		m_cbClock = clock;
		tTVPWaveFormat format;
		format.SamplesPerSec = audioFormat.m_sampleRate;
		format.Channels = audioFormat.m_channelLayout.Count();
		format.BitsPerSample = 0;
		switch (audioFormat.m_dataFormat) {
		case AE_FMT_U8:
			format.BitsPerSample = 8;
			break;
		case AE_FMT_S16LE:
			format.BitsPerSample = 16;
			break;
		default:
			format.BitsPerSample = InitResample(audioFormat);
			break;
		}
		format.BytesPerSample = format.BitsPerSample / 8;
		format.TotalSamples = 0;
		format.TotalTime = 0;
		format.SpeakerConfig = 0;
		format.IsFloat = false;
		format.Seekable = false;
		m_impl = TVPCreateSoundBuffer(format, 8);
	}

	virtual ~CAEStreamAL() {
		{
//			std::unique_lock<std::mutex> lk(_mutex);
			if (swr_ctx) {
				swr_free(&swr_ctx);
			}
			if (audio_buf) {
				av_freep(&audio_buf);
			}
			if (m_impl) {
				delete m_impl;
				m_impl = nullptr;
			}
		}
		_cond.notify_all();
	}

	virtual unsigned int AddData(const uint8_t* const *data, unsigned int offset, unsigned int frames, double pts) override {
		_timer.Set(1000);
		while (/*m_impl &&*/ !m_impl->IsBufferValid()) {
			std::unique_lock<std::mutex> lk(_mutex);
			_cond.wait_for(lk, std::chrono::milliseconds(10));
			if (_timer.IsTimePast()) return 0;
		}
//		if (!m_impl) return 0; // should not be
#if 0
		if (m_lastPts != 0) {
			if (std::abs(m_lastPts - pts) > 1000) {
				assert(false && "out of sync");
			}
		}

		pts += (double)(frames) / m_format.m_sampleRate;
		m_lastPts = pts;
#endif

		if (swr_ctx) {
			uint32_t srcoff = offset * (m_format.m_frameSize / src_buffer_count);
			const uint8_t *in[8];
			for (unsigned int i = 0; i < src_buffer_count; ++i) {
				in[i] = data[i] + srcoff;
			}
			int out_count = frames + 256;
			int out_size = av_samples_get_buffer_size(NULL, m_format.m_channelLayout.Count(),
				out_count, swr_tgtFormat, 0);
			av_fast_malloc(&audio_buf, &audio_buf_size, out_size);
			int len2 = swr_convert(swr_ctx, &audio_buf, out_count, in, frames);
			if (len2 == out_count) {
				av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
				swr_init(swr_ctx);
			}
			m_impl->AppendBuffer(audio_buf, len2 * tgt_frameSize);
		} else {
			m_impl->AppendBuffer(data[0] + offset * m_format.m_frameSize, frames * m_format.m_frameSize);
		}

		if (!m_impl->IsPlaying()) { // out of buffer
			m_impl->Play();
		}
		return frames;
	}

	virtual double GetDelay() override {
		return (double)m_impl->GetLatencySeconds();
	}

	virtual CAESyncInfo GetSyncInfo() override {
		CAESyncInfo info; // TODO
		info.delay = 0;
		info.error = 0;
		info.rr = 0;
		info.errortime = 0;
		info.state = CAESyncInfo::SYNC_OFF;
		return info;
	}

	virtual double GetCacheTime() override {
		return 1; // TODO
	}

	virtual double GetCacheTotal() override {
		// return std::max(GetDelay(), (double)TVPAL_BUFFER_COUNT);
		return GetDelay();
	}

	virtual void Pause() override { m_impl->Pause(); }
	virtual void Resume() override { m_impl->Play(); }
	virtual bool IsSuspended() { return !m_impl->IsPlaying(); }
	virtual void Drain(bool wait) {} // TODO
	virtual void Flush() {
		m_impl->Reset();
	}

	virtual iTVPSoundBuffer* GetNativeImpl() override { return m_impl; }
};


bool CAEFactory::SupportsRaw(AEAudioFormat &format)
{
  // check if passthrough is enabled
//   if (!CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH))
//     return false;
#if 0
  // fixed config disabled passthrough
  if (CSettings::GetInstance().GetInt(CSettings::SETTING_AUDIOOUTPUT_CONFIG) == AE_CONFIG_FIXED)
    return false;

  // check if the format is enabled in settings
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_AC3 && !CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH))
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_512 && !CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH))
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_1024 && !CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH))
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTS_2048 && !CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH))
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD_CORE && !CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH))
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_EAC3 && !CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH))
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_TRUEHD && !CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH))
    return false;
  if (format.m_streamInfo.m_type == CAEStreamInfo::STREAM_TYPE_DTSHD && !CSettings::GetInstance().GetBool(CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH))
    return false;
  if(AE)
    return AE->SupportsRaw(format);
#endif
  // refer to the format in TVPALSoundWrap::Init
  switch (format.m_dataFormat) {
  case AE_FMT_U8:
  case AE_FMT_S16LE:
	  break;
  default:
	  return false;
  }

  //if (format.m_channelLayout.Count() > 2) return false;
  //if (format.m_sampleRate > 48000) return false;

  return true;
}

IAEStream *CAEFactory::MakeStream(AEAudioFormat &audioFormat, unsigned int options, IAEClockCallback *clock)
{
//   if(AE)
//     return AE->MakeStream(audioFormat, options, clock);
	return new CAEStreamAL(audioFormat, options, clock);
  return NULL;
}

bool CAEFactory::FreeStream(IAEStream *stream)
{
//   if(AE)
//     return AE->FreeStream(stream);
	delete static_cast<CAEStreamAL*>(stream);
  return true;
}

NS_KRMOVIE_END