#include "tjsCommHead.h"
#include "WaveMixer.h"
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include "AL/alc.h"
#include "AL/alext.h"
#endif
#ifdef __ANDROID__
#include "oboe/Oboe.h"
#endif
#include <string.h>
#include <math.h>
#include "WaveImpl.h"
#include <assert.h>
#include "SysInitIntf.h"
#include "TickCount.h"
#include <sstream>
#include <iomanip>
#include "DebugIntf.h"
#include "SDL.h"
#include <unordered_set>
#include <algorithm>

class iTVPAudioRenderer;
static iTVPAudioRenderer *TVPAudioRenderer;

template<int ch>
void MixAudioS16CPP(void *dst, const void *src, int samples, int16_t *volume) {
	int16_t *dst16 = (int16_t *)dst;
	const int16_t *src16 = (const int16_t *)src;
	while (samples--) {
		for (int i = 0; i < ch; ++i) {
			int src_sample = *src16++;
			src_sample = (src_sample * volume[i]) >> 14;
			int dest_sample = *dst16;
			if (src_sample > 0 && dest_sample > 0) {
				dest_sample = src_sample + dest_sample - ((dest_sample * src_sample + 0x8000) >> 15);
			} else if (src_sample < 0 && dest_sample < 0) {
				dest_sample = src_sample + dest_sample + ((dest_sample * src_sample) >> 15);
			} else {
				dest_sample += src_sample;
			}
			*dst16++ = dest_sample;
		}
	}
}

template<int ch>
void MixAudioF32CPP(void *dst, const void *src, int samples, int16_t *volume) {
	float *dst32 = (float *)dst;
	const float *src32 = (const float *)src;
	const float fmaxvolume = 1.0f / 16384/*tTVPSoundBuffer::MAX_VOLUME*/;
	float fvolume[ch];
	for (int i = 0; i < ch; ++i)
		fvolume[i] = volume[i] * fmaxvolume;
	while (samples--) {
		for (int i = 0; i < ch; ++i) {
			float src_sample = SDL_SwapFloatLE(*src32++) * fvolume[i];
			float dest_sample = SDL_SwapFloatLE(*dst32);
			if (src_sample > 0 && dest_sample > 0) {
				dest_sample = src_sample + dest_sample - dest_sample * src_sample;
			} else if (src_sample < 0 && dest_sample < 0) {
				dest_sample = src_sample + dest_sample + dest_sample * src_sample;
			} else {
				dest_sample += src_sample;
			}
			*(dst32++) = SDL_SwapFloatLE(dest_sample);
		}
	}
}

typedef void(FAudioMix)(void *dst, const void *src, int samples, int16_t *volume);
static FAudioMix *_AudioMixS16[8] = { // 7.1 max
	&MixAudioS16CPP<1>,
	&MixAudioS16CPP<2>,
	&MixAudioS16CPP<3>,
	&MixAudioS16CPP<4>,
	&MixAudioS16CPP<5>,
	&MixAudioS16CPP<6>,
	&MixAudioS16CPP<7>,
	&MixAudioS16CPP<8>
};
static FAudioMix *_AudioMixF32[8] = { // 7.1 max
	&MixAudioF32CPP<1>,
	&MixAudioF32CPP<2>,
	&MixAudioF32CPP<3>,
	&MixAudioF32CPP<4>,
	&MixAudioF32CPP<5>,
	&MixAudioF32CPP<6>,
	&MixAudioF32CPP<7>,
	&MixAudioF32CPP<8>
};

extern "C" void TVPWaveMixer_ASM_Init(FAudioMix **func16, FAudioMix **func32);

class tTVPSoundBuffer : public iTVPSoundBuffer {
public:
	bool _playing = false;
	float _volume = 1;
	float _pan = 0;
	const signed int MAX_VOLUME = 16384; // limit in signed 16bit
	int16_t _volume_raw[8];
	SDL_AudioCVT *_cvt = nullptr;
	std::vector<uint8_t> _cvtbuf;
	int _frame_size = 0;

	void RecalcVolume() {
		if (_pan > 0) {
			_volume_raw[0] = (1.0f - _pan) * _volume * MAX_VOLUME;
		} else {
			_volume_raw[0] = _volume * MAX_VOLUME;
		}
		if (_pan < 0) {
			_volume_raw[1] = (_pan + 1.0f) * _volume * MAX_VOLUME;
		} else {
			_volume_raw[1] = _volume * MAX_VOLUME;
		}
		_volume_raw[2] = _volume_raw[0]; // for SIMD
		_volume_raw[3] = _volume_raw[1];
	}
	std::mutex _buffer_mtx;
	std::deque<std::vector<uint8_t> > _buffers;
	tjs_uint _sendedFrontBuffer = 0;
	tjs_uint _sendedSamples = 0, _inCachedSamples = 0;

	tTVPSoundBuffer(int framesize, SDL_AudioCVT *cvt) : _frame_size(framesize), _cvt(cvt) {
		RecalcVolume();
		if (cvt) {
			_cvtbuf.resize(/*2352*/ 2400 * 2 * 4 * _cvt->len_mult); // IEEE f.32 stereo 48000kHz
			_cvt->buf = &_cvtbuf.front();
		}
	}
	virtual ~tTVPSoundBuffer();
	virtual void Release() override { delete this; }
	virtual void Play() override {
		_playing = true;
	}
	virtual void Pause() override {
		_playing = false;
	}
	virtual void Stop() override {
		_playing = false;
		Reset();
	}
	virtual void Reset() override {
		std::lock_guard<std::mutex> lk(_buffer_mtx);
		_buffers.clear();
		_inCachedSamples = 0;
		_sendedFrontBuffer = 0;
		_sendedSamples = 0;
	}
	virtual bool IsPlaying() override {
		return _playing;
	}
	virtual void SetVolume(float v) override {
		_volume = v;
		RecalcVolume();
	}
	virtual float GetVolume() override { return _volume; }
	virtual void SetPan(float v) override {
		_pan = v;
		RecalcVolume();
	}
	virtual float GetPan() override { return _pan; }
	virtual void AppendBuffer(const void *_inbuf, unsigned int inlen/*, int tag = 0*/) override {
		if (_cvt) {
			std::vector<uint8_t> buffer;
			uint8_t* inbuf = (uint8_t*)_inbuf;
			int buflen = _frame_size * 2352;
			_cvt->len = buflen;
			while (inlen > buflen) { // fill 2352 samples to fit 48k/44.1k
				memcpy(_cvt->buf, inbuf, buflen);
				SDL_ConvertAudio(_cvt);
				buffer.insert(buffer.end(), _cvt->buf, _cvt->buf + _cvt->len_cvt);
				inlen -= buflen;
				inbuf += buflen;
			}
			if (inlen > 0) {
				int buflen = inlen;
				memcpy(_cvt->buf, inbuf, buflen);
				_cvt->len = buflen;
				SDL_ConvertAudio(_cvt);
				buffer.insert(buffer.end(), _cvt->buf, _cvt->buf + _cvt->len_cvt);
			}
			std::lock_guard<std::mutex> lk(_buffer_mtx);
			_inCachedSamples += buffer.size() / _frame_size;
			_buffers.emplace_back();
			_buffers.back().swap(buffer);
		} else {
			std::lock_guard<std::mutex> lk(_buffer_mtx);
			_buffers.emplace_back((uint8_t*)_inbuf, ((uint8_t*)_inbuf) + inlen);
			_inCachedSamples += inlen / _frame_size;
		}
	}
	virtual bool IsBufferValid() override {
		return true; // unlimited buffer size
		//return !_buffers.empty(); // thread safe if read only
	}
	virtual tjs_uint GetLatencySamples() override;
// 	virtual void SetSampleOffset(tjs_uint n) override {
// 		_sendedSamples = n;
// 	}
	virtual int GetRemainBuffers() override {
		return _buffers.size();
	}
	virtual tjs_uint GetCurrentPlaySamples() override {
		return _sendedSamples - GetLatencySamples();
	}
	virtual float GetLatencySeconds() override;

	void FillBuffer(uint8_t *out, int len);
};

class iTVPAudioRenderer {
protected:
	SDL_AudioSpec _spec;
	std::mutex _streams_mtx;
	std::unordered_set<tTVPSoundBuffer*> _streams;
	int _frame_size = 0;

public:
	iTVPAudioRenderer() {
		memset(&_spec, 0, sizeof(_spec));
		_spec.freq = 48000;
		_spec.format = AUDIO_S16;
		_spec.channels = 2;
		_spec.callback = [](void *p, Uint8 *s, int l) {
			memset(s, 0, l);
			((iTVPAudioRenderer*)p)->FillBuffer(s, l);
		};
		_spec.userdata = this;
		_spec.size = 4;
		_frame_size = 4;
	}
	void InitMixer() {
		if (SDL_Init(SDL_INIT_AUDIO) < 0) { // for format converter
			SDL_Log("Fail to initialize audio.");
			return;
		}
	}

	FAudioMix *DoMixAudio;
	void SetupMixer() {
		TVPWaveMixer_ASM_Init(_AudioMixS16, _AudioMixF32);
		if (_spec.format == AUDIO_S16LSB) {
			DoMixAudio = _AudioMixS16[_spec.channels - 1];
		} else if (_spec.format == AUDIO_F32LSB) {
			DoMixAudio = _AudioMixF32[_spec.channels - 1];
		} else {
			DoMixAudio = [](void *dst, const void *src, int samples, int16_t *volume) {};
		}
	}

	virtual bool Init() = 0;

	virtual tTVPSoundBuffer* CreateStream(tTVPWaveFormat &fmt, int bufcount) {
		SDL_AudioSpec spec;
		memset(&spec, 0, sizeof(spec));
		spec.freq = fmt.SamplesPerSec;
		spec.channels = fmt.Channels;
		if (fmt.IsFloat) {
			spec.format = AUDIO_F32LSB;
		} else {
			switch (fmt.BitsPerSample) {
			case 8: spec.format = AUDIO_S8; break;
			case 16: spec.format = AUDIO_S16LSB; break;
			case 32: spec.format = AUDIO_S32LSB; break;
			default: return nullptr;
			}
		}
		SDL_AudioCVT *cvt = nullptr;
		if (spec.freq != _spec.freq ||
			spec.channels != _spec.channels ||
			spec.format != _spec.format) {
			cvt = new SDL_AudioCVT;
			int err = SDL_BuildAudioCVT(cvt,
				spec.format, spec.channels, spec.freq,
				_spec.format, _spec.channels, _spec.freq);
			if (err != 1) {
				delete cvt; return nullptr;
			}
		}

		tTVPSoundBuffer* s = new tTVPSoundBuffer(fmt.BytesPerSample * fmt.Channels, cvt);
		std::lock_guard<std::mutex> lk(_streams_mtx);
		_streams.emplace(s);
		return s;
	}

	void ReleaseStream(tTVPSoundBuffer* s) {
		std::lock_guard<std::mutex> lk(_streams_mtx);
		_streams.erase(s);
	}

	void FillBuffer(Uint8 *buf, int len) {
		// memset(buf, 0, len);
		std::lock_guard<std::mutex> lk(_streams_mtx);
		for (tTVPSoundBuffer* s : _streams) {
			s->FillBuffer(buf, len);
		}
	}

	int MixAudio(uint8_t *dst, uint8_t *src, int len, int16_t *vol) {
		int samples = len / _frame_size;
		DoMixAudio(dst, src, samples, vol);
		return samples;
	}

	const SDL_AudioSpec& GetSpec() {
		return _spec;
	}

	virtual int32_t GetUnprocessedSamples() { return 0; }
};

tTVPSoundBuffer::~tTVPSoundBuffer()
{
	Stop();
	TVPAudioRenderer->ReleaseStream(this);
	if (_cvt) delete _cvt;
}

tjs_uint tTVPSoundBuffer::GetLatencySamples()
{
	int32_t samples = TVPAudioRenderer->GetUnprocessedSamples();
	return samples + _inCachedSamples;
}

float tTVPSoundBuffer::GetLatencySeconds()
{
	return GetLatencySamples() / TVPAudioRenderer->GetSpec().freq;
}

void tTVPSoundBuffer::FillBuffer(uint8_t *out, int len)
{
	if (!_playing) return;
	std::lock_guard<std::mutex> lk(_buffer_mtx);
	while (len > 0 && !_buffers.empty()) {
		std::vector<uint8_t> &buf = _buffers.front();
		if (buf.size() > _sendedFrontBuffer) {
			int n = std::min((size_t)len, buf.size() - _sendedFrontBuffer);
			int samples = TVPAudioRenderer->MixAudio(out, &buf.front() + _sendedFrontBuffer, n, _volume_raw);
			_sendedSamples += samples;
			_inCachedSamples -= samples;
			_sendedFrontBuffer += n;
			out += n;
			len -= n;
		}
		if (_sendedFrontBuffer >= buf.size()) {
			_sendedFrontBuffer = 0;
			_buffers.pop_front();
		}
	}
}

class tTVPAudioRendererSDL : public iTVPAudioRenderer {
	SDL_AudioDeviceID _playback_id;

public:
	bool Init() override {
		InitMixer();
		_playback_id = SDL_OpenAudioDevice(nullptr, false, &_spec, &_spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
		if (_playback_id <= 0) {
			SDL_Log("Fail to open audio @%dHz.", _spec.freq);
			return false;
		}
		_frame_size = SDL_AUDIO_BITSIZE(_spec.format) / 8 * _spec.channels;
		SDL_Log("Audio Device: %s", SDL_GetCurrentAudioDriver());
		SDL_PauseAudioDevice(_playback_id, false);
		SetupMixer();
		return true;
	}
};

#ifdef __ANDROID__
class tTVPAudioRendererOboe : public iTVPAudioRenderer, public oboe::AudioStreamCallback {
	oboe::AudioStream *_oboeAudioStream = nullptr;

public:
	virtual ~tTVPAudioRendererOboe() {
		if (_oboeAudioStream) delete _oboeAudioStream;
	}

	bool Init() override {
		InitMixer();
		// Create a builder
		oboe::AudioStreamBuilder builder;
		//builder.setFormat(oboe::AudioFormat::I16);
		builder.setChannelCount(2);
		//builder.setSampleRate(oboe::kUnspecified);
		builder.setCallback(this);
		// 	builder.setPerformanceMode(PerformanceMode::None);
		// 	builder.setSharingMode(SharingMode::Shared);
		oboe::Result result = builder.openStream(&_oboeAudioStream);
// 		if (result != oboe::Result::OK) {
// 			// try down sample rate
// 			_spec.freq = 44100;
// 			builder.setSampleRate(_spec.freq);
// 			result = builder.openStream(&_oboeAudioStream);
// 		}
		if (result == oboe::Result::OK) {
			_spec.freq = _oboeAudioStream->getSampleRate();
			switch (_oboeAudioStream->getFormat()) {
			case oboe::AudioFormat::I16: _spec.format = AUDIO_S16LSB; break;
			case oboe::AudioFormat::Float: _spec.format = AUDIO_F32LSB; break;
			}
			_frame_size = SDL_AUDIO_BITSIZE(_spec.format) / 8 * _spec.channels;
			_oboeAudioStream->requestStart();
			SDL_Log("Audio Device: Oboe @%dHz", _spec.freq);
			SetupMixer();
			return true;
		}
		SDL_Log("Fail to open Oboe audio");
		// SetupSDL();
		return false;
	}

	virtual oboe::DataCallbackResult onAudioReady(
		oboe::AudioStream *oboeStream,
		void *audioData,
		int32_t numFrames) override {
		int len = _frame_size * numFrames;
		memset(audioData, 0, _frame_size * numFrames);
		//if (oboeStream == _oboeAudioStream)
			FillBuffer((uint8_t*)audioData, len);
// 		else
// 			fillCaptureBuffer((uint8_t*)audioData, /*Mono*/2 * numFrames);
		return oboe::DataCallbackResult::Continue;
	}

	virtual int32_t GetUnprocessedSamples() {
		int64_t hardwareFrameIndex;
		int64_t timeNanoseconds;
		oboe::Result result = _oboeAudioStream->getTimestamp(CLOCK_MONOTONIC, &hardwareFrameIndex, &timeNanoseconds);
		if (result != oboe::Result::OK) { // OpenSL TODO accumulate calc
			return 0;
		}
		int64_t appFrameIndex = _oboeAudioStream->getFramesWritten();
		return appFrameIndex - hardwareFrameIndex;
	}
};
#endif

class tTVPSoundBufferAL : public tTVPSoundBuffer {
	typedef tTVPSoundBuffer inherit;

	ALuint _alSource;
	ALenum _alFormat;
	ALuint *_bufferIds, *_bufferIds2;
	tjs_uint *_bufferSize;
	tjs_uint _bufferCount;
	int _bufferIdx = -1;
	tTVPWaveFormat _format;
public:
	tTVPSoundBufferAL(tTVPWaveFormat &desired, int bufcount)
		: tTVPSoundBuffer(desired.BytesPerSample * desired.Channels, nullptr), _bufferCount(bufcount)
	{
		_bufferIds = new ALuint[bufcount];
		_bufferIds2 = new ALuint[bufcount];
		_bufferSize = new tjs_uint[bufcount];
		_format = desired;
		alGenSources(1, &_alSource);
		alGenBuffers(_bufferCount, _bufferIds);
		alSourcef(_alSource, AL_GAIN, 1.0f);
		if (desired.Channels == 1) {
			switch (desired.BitsPerSample) {
			case 8:
				_alFormat = AL_FORMAT_MONO8;
				break;
			case 16:
				_alFormat = AL_FORMAT_MONO16;
				break;
			default:
				assert(false);
			}
		} else if (desired.Channels == 2) {
			switch (desired.BitsPerSample) {
			case 8:
				_alFormat = AL_FORMAT_STEREO8;
				break;
			case 16:
				_alFormat = AL_FORMAT_STEREO16;
				break;
			default:
				assert(false);
			}
		} else {
			assert(false);
		}
	}

	virtual ~tTVPSoundBufferAL() {
		alDeleteBuffers(_bufferCount, _bufferIds);
		alDeleteSources(1, &_alSource);
		delete[]_bufferIds;
		delete[]_bufferIds2;
		delete[]_bufferSize;
	}

	bool IsBufferValid() override {
		ALint processed = 0;
		alGetSourcei(_alSource, AL_BUFFERS_PROCESSED, &processed);
		if (processed > 0) return true;
		ALint queued = 0;
		alGetSourcei(_alSource, AL_BUFFERS_QUEUED, &queued);
		return queued < _bufferCount;
	}

	virtual void AppendBuffer(const void *buf, unsigned int len/*, int tag = 0*/) override {
		if (len <= 0) return;
		std::lock_guard<std::mutex> lk(_buffer_mtx);

		/* First remove any processed buffers. */
		ALint processed = 0;
		alGetSourcei(_alSource, AL_BUFFERS_PROCESSED, &processed);
		if (processed > 0) {
			alSourceUnqueueBuffers(_alSource, processed, _bufferIds2);
            checkerr("alSourceUnqueueBuffers");
			for (int i = 0; i < processed; ++i) {
				for (int j = 0; j < _bufferCount; ++j) {
					if (_bufferIds[j] == _bufferIds2[i])
					{
						_sendedSamples += _bufferSize[j] / _frame_size;
						break;
					}
				}
			}
		}

		/* Refill the buffer queue. */
		ALint queued = 0;
		alGetSourcei(_alSource, AL_BUFFERS_QUEUED, &queued);

		if (queued >= _bufferCount)
			return;
		++_bufferIdx;
		if (_bufferIdx >= _bufferCount) _bufferIdx = 0;
		ALuint bufid = _bufferIds[_bufferIdx];
		alBufferData(bufid, _alFormat, buf, len, _format.SamplesPerSec);
        checkerr("alBufferData");
		alSourceQueueBuffers(_alSource, 1, &bufid);
        checkerr("alSourceQueueBuffers");
		//_tags[_bufferIdx] = tag;
		_bufferSize[_bufferIdx] = len;
	}

	void Reset() override {
		inherit::Reset();
		std::lock_guard<std::mutex> lk(_buffer_mtx);
		alSourceRewind(_alSource);
		alSourcei(_alSource, AL_BUFFER, 0);
	}

	void Pause() override {
		alSourcePause(_alSource);
        checkerr("Pause");
		_playing = false;
	}
    
    static void checkerr(const char *funcname);

	void Play() override {
		ALenum state;
		alGetSourcei(_alSource, AL_SOURCE_STATE, &state);
        checkerr("Play");
		if (state != AL_PLAYING) {
			alSourcePlay(_alSource);
            checkerr("Play");
		}

		_playing = true;
	}

	void Stop() override {
		alSourceStop(_alSource);
        checkerr("Stop");
		Reset();
		_bufferIdx = -1;
		_playing = false;
	}

	void SetVolume(float volume) override {
		alSourcef(_alSource, AL_GAIN, volume);
        checkerr("SetVolume");
	}

	float GetVolume() override {
		float volume = 0;
		alGetSourcef(_alSource, AL_GAIN, &volume);
		return volume;
	}

	void SetPan(float pan) override {
		float sourcePosAL[] = { pan, 0.0f, 0.0f };
		alSourcefv(_alSource, AL_POSITION, sourcePosAL);
	}

	float GetPan() override {
		float sourcePosAL[3];
		alGetSourcefv(_alSource, AL_POSITION, sourcePosAL);
		return sourcePosAL[0];
	}

	bool IsPlaying() override {
		ALenum state;
		alGetSourcei(_alSource, AL_SOURCE_STATE, &state);
		return state == AL_PLAYING;
	}

	void SetPosition(float x, float y, float z) override {
		float sourcePosAL[] = { x, y, z };
		alSourcefv(_alSource, AL_POSITION, sourcePosAL);
        checkerr("SetPosition");
	}

	int GetRemainBuffers() override {
		ALint processed, queued = 0;
		alGetSourcei(_alSource, AL_BUFFERS_PROCESSED, &processed);
		alGetSourcei(_alSource, AL_BUFFERS_QUEUED, &queued);
		return queued - processed;
	}

	tjs_uint GetLatencySamples() override {
		std::lock_guard<std::mutex> lk(_buffer_mtx);
		ALint offset = 0, queued = 0;
		alGetSourcei(_alSource, AL_BYTE_OFFSET, &offset);
		alGetSourcei(_alSource, AL_BUFFERS_QUEUED, &queued);
		int remainBuffers = queued;
		if (remainBuffers == 0) return 0;
		tjs_int total = -offset;
		for (int i = 0; i < remainBuffers; ++i) {
			int idx = _bufferIdx + 1 - remainBuffers + i;
			if (idx >= _bufferCount) idx -= _bufferCount;
			else if (idx < 0) idx += _bufferCount;
			total += _bufferSize[idx];
		}
		return total / _frame_size;
	}

	virtual float GetLatencySeconds() override {
		return (float)GetLatencySamples() / _format.SamplesPerSec;
	}

	virtual tjs_uint GetCurrentPlaySamples() override {
		ALint offset = 0;
		alGetSourcei(_alSource, AL_SAMPLE_OFFSET, &offset);
		return _sendedSamples + offset;
	}
};

class tTVPAudioRendererAL : public iTVPAudioRenderer {
	ALCdevice *_device = nullptr;
	ALCcontext *_context = nullptr;
public:
	virtual ~tTVPAudioRendererAL() {
		if (_context) {
			//alDeleteSources(TVP_MAX_AUDIO_COUNT, _alSources);
			alcMakeContextCurrent(nullptr);
			alcDestroyContext(_context);
		}
		if (_device)
			alcCloseDevice(_device);
	}
	bool Init() override {
		ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
		if (enumeration == AL_FALSE) {
			// enumeration not supported
			_device = alcOpenDevice(NULL);
		} else {
			// enumeration supported
			const ALCchar *devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
			std::vector<std::string> alldev;
			ttstr log(TJS_W("(info) Sound Driver/Device found : "));
			while (*devices) {
				TVPAddImportantLog(log + devices);
				alldev.emplace_back(devices);
				devices += alldev.back().length();
			}
			_device = alcOpenDevice(alldev[0].c_str());
		}
		if (!_device) return false;

		_context = alcCreateContext(_device, NULL);
		alcMakeContextCurrent(_context);

		return true;
	}

	virtual tTVPSoundBuffer* CreateStream(tTVPWaveFormat &fmt, int bufcount) override {
		tTVPSoundBuffer* s = new tTVPSoundBufferAL(fmt, bufcount);
		_streams.emplace(s);
		return s;
	}
    
    ALCcontext *GetContext() {
        return _context;
    }
};

void tTVPSoundBufferAL::checkerr(const char *funcname) {
#if _DEBUG
    ALCcontext *ctx = static_cast<tTVPAudioRendererAL*> (TVPAudioRenderer)->GetContext();
    if (alcGetCurrentContext() != ctx) {
        alcMakeContextCurrent(ctx);
    }
    ALenum err = alGetError();
    if (AL_NO_ERROR == err) return;
    SDL_Log("%s OpenAL Error %X", funcname, err);
#endif
}

static iTVPAudioRenderer *CreateAudioRenderer() {
	iTVPAudioRenderer *renderer = nullptr;
#ifdef __ANDROID__
	renderer = new tTVPAudioRendererOboe;
	if (renderer->Init()) return renderer;
	delete renderer;
#elif defined(_MSC_VER) && 0
	renderer = new tTVPAudioRendererSDL;
	renderer->Init();
	return renderer;
#endif
	renderer = new tTVPAudioRendererAL;
	renderer->Init();
	return renderer;
}

void TVPInitDirectSound(int freq)
{
	if (!TVPAudioRenderer) {
		TVPAudioRenderer = CreateAudioRenderer();
	}
	//TVPInitSoundOptions();
}

void TVPUninitDirectSound()
{
	// nothing to do
}

iTVPSoundBuffer* TVPCreateSoundBuffer(tTVPWaveFormat &fmt, int bufcount)
{
	return TVPAudioRenderer->CreateStream(fmt, bufcount);
}
#if 0
int TVPALSoundWrap::GetNextBufferIndex() {
	int n = _bufferIdx + 1;
	if (n >= TVPAL_BUFFER_COUNT) n = 0;
	return n;
}

void TVPALSoundWrap::SetSampleOffset(tjs_uint n /*= 0*/) {
	_samplesProcessed = n;
}
#endif
