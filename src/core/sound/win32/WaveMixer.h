#pragma once
#include "WaveIntf.h"
#include <list>
#include <mutex>

#define TVPAL_BUFFER_COUNT 4

struct ALSoundImpl;
void TVPInitDirectSound(int freq = 48000);
void TVPUninitDirectSound();

class iTVPSoundBuffer {
public:
	virtual ~iTVPSoundBuffer() {}
	virtual void Release() = 0;
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void Stop() = 0;
	virtual void Reset() = 0;
	virtual bool IsPlaying() = 0;
	virtual void SetVolume(float v) = 0;
	virtual float GetVolume() = 0;
	virtual void SetPan(float v) = 0;
	virtual float GetPan() = 0;
	virtual void AppendBuffer(const void *buf, unsigned int len/*, int tag = 0*/) = 0;
	virtual bool IsBufferValid() = 0;
	virtual tjs_uint GetCurrentPlaySamples() = 0;
	virtual tjs_uint GetLatencySamples() = 0;
	virtual float GetLatencySeconds() { return 0; }
//	virtual void SetSampleOffset(tjs_uint n) = 0;
	virtual int GetRemainBuffers() = 0;
//	virtual int GetNextBufferIndex() = 0;
	virtual void SetPosition(float x, float y, float z) {
		// not implemented
	}
};
iTVPSoundBuffer* TVPCreateSoundBuffer(tTVPWaveFormat &fmt, int bufcount);
#if 0
class TVPALSoundWrap : public iTVPSoundBuffer {
private:
	ALSoundImpl *_impl;
	tjs_uint _bufferSize[TVPAL_BUFFER_COUNT];
	//int _tags[TVPAL_BUFFER_COUNT];
	int _bufferIdx;
	//int _tagPoint;
	tjs_uint _samplesProcessed;
	tTVPWaveFormat SoundFormat;
	bool _inPlaying;

	std::mutex _mutex;
protected:
	TVPALSoundWrap();

public:
	virtual ~TVPALSoundWrap();
	static TVPALSoundWrap * Create(const tTVPWaveFormat &desired);
	void Release();
	void Update(); // callback from TVPSoundMixerList
	virtual bool DoFillBuffer();
	void Init(const tTVPWaveFormat &desired);

	int GetNextBufferIndex();
	int GetRemainBuffers();
	int GetValidBuffers();

	void Reset();
	void Rewind();
	void Pause();
	void Play();
	void Stop();
	bool IsBufferValid();
	void AppendBuffer(const void *buf, unsigned int len/*, int tag = 0*/);
	void SetVolume(float vol); // 0 ~ 1
	float GetVolume();
	void SetPan(float pan); // -1 ~ 1
	float GetPan();
	tjs_uint GetCurrentSampleOffset();
	void SetSampleOffset(tjs_uint n = 0);
	tjs_uint GetUnprocessedSamples();
	bool IsPlaying();
	void SetPosition(float x, float y, float z);
};
#endif