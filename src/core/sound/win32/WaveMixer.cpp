#include "tjsCommHead.h"
#include "WaveMixer.h"
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include "AL/alc.h"
#include "AL/alext.h"
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

static ALCdevice *TVPPrimarySoundHandle = nullptr;
static ALCcontext *s_ALContext = nullptr;

extern "C" void TVPInitSoundASM();
extern "C" void TVPInitSoundASM() {}

void TVPInitDirectSound()
{
	//TVPInitSoundOptions();

	// set primary buffer 's sound format
	if (!TVPPrimarySoundHandle)
	{
		TVPInitSoundASM();
		ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
		if (enumeration == AL_FALSE) {
			// enumeration not supported
			TVPPrimarySoundHandle = alcOpenDevice(NULL);
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
			TVPPrimarySoundHandle = alcOpenDevice(alldev[0].c_str());
		}

		s_ALContext = alcCreateContext(TVPPrimarySoundHandle, NULL);
		alcMakeContextCurrent(s_ALContext);
	}
}

void TVPUninitDirectSound()
{
	if (TVPPrimarySoundHandle) {
		if (s_ALContext) {
			//alDeleteSources(TVP_MAX_AUDIO_COUNT, _alSources);

			alcMakeContextCurrent(nullptr);
			alcDestroyContext(s_ALContext);
			s_ALContext = nullptr;
		}
		alcCloseDevice(TVPPrimarySoundHandle);
		TVPPrimarySoundHandle = nullptr;
	}
}

struct ALSoundImpl {
	ALSoundImpl() {
		alGenSources(1, &_alSource);
		alGenBuffers(TVPAL_BUFFER_COUNT, _bufferIds);
	}
	~ALSoundImpl() {
		alDeleteBuffers(TVPAL_BUFFER_COUNT, _bufferIds);
		alDeleteSources(1, &_alSource);
	}
	ALuint _alSource;
	ALenum _alFormat;
	ALuint _bufferIds[TVPAL_BUFFER_COUNT];
};

TVPALSoundWrap::TVPALSoundWrap() {
	if (!TVPPrimarySoundHandle) TVPInitDirectSound();
	_impl = new ALSoundImpl;
	_inPlaying = false;
	_bufferIdx = -1;
	_samplesProcessed = 0;
}

TVPALSoundWrap::~TVPALSoundWrap() {
	Stop();
	delete _impl;
}

TVPALSoundWrap * TVPALSoundWrap::Create(const tTVPWaveFormat &desired) {
	TVPALSoundWrap* ret = new TVPALSoundWrap();
	ret->Init(desired);
//	TVPAddSoundMixer(ret);

	return ret;
}

void TVPALSoundWrap::Init(const tTVPWaveFormat &desired) {
	SoundFormat = desired;
	alSourcef(_impl->_alSource, AL_GAIN, 1.0f);
	//InitSoundMixer(desired);
// 	assert(desired.BigEndian == false);
// 	assert(desired.Signed == true);
	//alSourceQueueBuffers(_impl->_alSource, TVPAL_BUFFER_COUNT, _bufferIds);
	if (desired.Channels == 1) {
		switch (desired.BitsPerSample) {
		case 8:
			_impl->_alFormat = AL_FORMAT_MONO8;
			break;
		case 16:
			_impl->_alFormat = AL_FORMAT_MONO16;
			break;
		default:
			assert(false);
		}
	} else if (desired.Channels == 2) {
		switch (desired.BitsPerSample) {
		case 8:
			_impl->_alFormat = AL_FORMAT_STEREO8;
			break;
		case 16:
			_impl->_alFormat = AL_FORMAT_STEREO16;
			break;
		default:
			assert(false);
		}
	} else {
		assert(false);
	}
	_bufferIdx = -1;
	//_tagPoint = 0;
}

bool TVPALSoundWrap::IsBufferValid() {
	ALint processed = 0;
	alGetSourcei(_impl->_alSource, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0) return true;
	ALint queued = 0;
	alGetSourcei(_impl->_alSource, AL_BUFFERS_QUEUED, &queued);
	return queued < TVPAL_BUFFER_COUNT;
}

void TVPALSoundWrap::AppendBuffer(const void *buf, unsigned int len/*, int tag*/) {
	if (len <= 0) return;
	std::lock_guard<std::mutex> lk(_mutex);

	/* First remove any processed buffers. */
	ALint processed = 0;
	alGetSourcei(_impl->_alSource, AL_BUFFERS_PROCESSED, &processed);
	if (processed > 0) {
		ALuint ids[TVPAL_BUFFER_COUNT];
		alSourceUnqueueBuffers(_impl->_alSource, processed, ids);
		for (int i = 0; i < processed; ++i) {
			for (int j = 0; j < TVPAL_BUFFER_COUNT; ++j) {
				if (_impl->_bufferIds[j] == ids[i])
				{
					_samplesProcessed += _bufferSize[j] / SoundFormat.BytesPerSample / SoundFormat.Channels;
					break;
				}
			}
		}
	}

	/* Refill the buffer queue. */
	ALint queued = 0;
	alGetSourcei(_impl->_alSource, AL_BUFFERS_QUEUED, &queued);
#if 0
	tjs_uint32 tick = TVPGetTickTime();
	int ms = tick % 1000; tick /= 1000;
	int sec = tick % 60; tick /= 60;
	int min = tick % 60; tick /= 60;
	int hour = tick;
	std::stringstream timestr;
	if (hour) timestr << std::setw(2) << std::setfill('0') << hour << ":";
	if (hour || min) timestr << std::setw(2) << std::setfill('0') << min << ":";
	timestr << std::setw(2) << std::setfill('0') << sec << ".";
	timestr << std::setw(3) << std::setfill('0') << ms;
	std::string stime = timestr.str();

	ALint state;
	alGetSourcei(_impl->_alSource, AL_SOURCE_STATE, &state);
	printf("[oal]%s id=%d stat=%d queue=%d proced=%f[s]\n", stime.c_str(), _impl->_alSource, state,
		queued, (float)_samplesProcessed / SoundFormat.BytesPerSample / SoundFormat.SamplesPerSec / SoundFormat.Channels);
#endif
	if (queued >= TVPAL_BUFFER_COUNT)
		return;
	++_bufferIdx;
	if (_bufferIdx >= TVPAL_BUFFER_COUNT) _bufferIdx = 0;
	ALuint bufid = _impl->_bufferIds[_bufferIdx];
	alBufferData(bufid, _impl->_alFormat, buf, len, SoundFormat.SamplesPerSec);
	alSourceQueueBuffers(_impl->_alSource, 1, &bufid);
	//_tags[_bufferIdx] = tag;
	_bufferSize[_bufferIdx] = len;
}

void TVPALSoundWrap::Reset() {
	Rewind();
}

void TVPALSoundWrap::Pause() {
	alSourcePause(_impl->_alSource);
	_inPlaying = false;
}

void TVPALSoundWrap::Play() {
	ALenum state;
	alGetSourcei(_impl->_alSource, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING) {
// 		ALint processed = 0, queued;
// 		alGetSourcei(_impl->_alSource, AL_BUFFERS_PROCESSED, &processed);
// 		alGetSourcei(_impl->_alSource, AL_BUFFERS_QUEUED, &queued);
// 		if (queued + processed != _attachedBuffers) {
// 			alSourceStop(_impl->_alSource);
// 			_attachedBuffers = 0;
// 		}
// 		std::vector<unsigned char> silentbuff;
// 		silentbuff.resize();
		//_tagPoint = 0;
		alSourcePlay(_impl->_alSource);
	}

	_inPlaying = true;
}

void TVPALSoundWrap::Stop() {
	alSourceStop(_impl->_alSource);
	alSourcei(_impl->_alSource, AL_BUFFER, 0);
	_bufferIdx = -1;
	//_tagPoint = 0;
	_inPlaying = false;
	_samplesProcessed = 0;
}

void TVPALSoundWrap::SetVolume(float volume) {
	alSourcef(_impl->_alSource, AL_GAIN, volume);
}

float TVPALSoundWrap::GetVolume() {
	float volume = 0;
	alGetSourcef(_impl->_alSource, AL_GAIN, &volume);
	return volume;
}

void TVPALSoundWrap::SetPan(float pan) {
	float sourcePosAL[] = { pan, 0.0f, 0.0f };
	alSourcefv(_impl->_alSource, AL_POSITION, sourcePosAL);
}

float TVPALSoundWrap::GetPan() {
	float sourcePosAL[3];
	alGetSourcefv(_impl->_alSource, AL_POSITION, sourcePosAL);
	return sourcePosAL[0];
}

tjs_uint TVPALSoundWrap::GetCurrentSampleOffset() {
	ALint offset = 0;
	alGetSourcei(_impl->_alSource, AL_SAMPLE_OFFSET, &offset);

// 	printf("offset = %f(%d)\n", (float)(_samplesProcessed + offset) / SoundFormat.SamplesPerSec,
// 		_samplesProcessed + offset);
	return _samplesProcessed + offset;
}

bool TVPALSoundWrap::IsPlaying() {
	ALenum state;
	alGetSourcei(_impl->_alSource, AL_SOURCE_STATE, &state);
	return state == AL_PLAYING;
}

void TVPALSoundWrap::Update() {
	while (IsBufferValid() && DoFillBuffer());
	if (_inPlaying && !IsPlaying()) Play(); // out of buffer
}

bool TVPALSoundWrap::DoFillBuffer()
{
	return false;
}

void TVPALSoundWrap::SetPosition(float x, float y, float z) {
	float sourcePosAL[] = { x, y, z };
	alSourcefv(_impl->_alSource, AL_POSITION, sourcePosAL);
}

int TVPALSoundWrap::GetNextBufferIndex() {
	int n = _bufferIdx + 1;
	if (n >= TVPAL_BUFFER_COUNT) n = 0;
	return n;
}

int TVPALSoundWrap::GetRemainBuffers() {
	ALint processed, queued = 0;
	alGetSourcei(_impl->_alSource, AL_BUFFERS_PROCESSED, &processed);
	alGetSourcei(_impl->_alSource, AL_BUFFERS_QUEUED, &queued);
	return queued - processed;
}

int TVPALSoundWrap::GetValidBuffers() {
	return TVPAL_BUFFER_COUNT - GetRemainBuffers();
}

void TVPALSoundWrap::SetSampleOffset(tjs_uint n /*= 0*/) {
	_samplesProcessed = n;
}

tjs_uint TVPALSoundWrap::GetUnprocessedSamples() {
	std::lock_guard<std::mutex> lk(_mutex);
	ALint offset = 0, queued = 0;
	alGetSourcei(_impl->_alSource, AL_BYTE_OFFSET, &offset);
	alGetSourcei(_impl->_alSource, AL_BUFFERS_QUEUED, &queued);
	int remainBuffers = queued;
	if (remainBuffers == 0) return 0;
	tjs_int total = -offset;
	for (int i = 0; i < remainBuffers; ++i) {
		int idx = _bufferIdx + 1 - remainBuffers + i;
		if (idx >= TVPAL_BUFFER_COUNT) idx -= TVPAL_BUFFER_COUNT;
		else if (idx < 0) idx += TVPAL_BUFFER_COUNT;
		total += _bufferSize[idx];
	}
	return total / SoundFormat.BytesPerSample / SoundFormat.Channels;
}

void TVPALSoundWrap::Rewind() {
	std::lock_guard<std::mutex> lk(_mutex);
	alSourceRewind(_impl->_alSource);
	alSourcei(_impl->_alSource, AL_BUFFER, 0);
	_samplesProcessed = 0;
}

void TVPALSoundWrap::Release() {
	delete this;
}
