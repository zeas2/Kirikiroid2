//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Wave Player implementation
//---------------------------------------------------------------------------
#ifndef WaveImplH
#define WaveImplH

#define DIRECTSOUND_VERSION 0x0300
#if 0
#include <mmsystem.h>
#include <dsound.h>
#include <ks.h>
#include <ksmedia.h>
#endif
#include "WaveIntf.h"
#include "WaveLoopManager.h"

/*[*/
//---------------------------------------------------------------------------
// IDirectSound former declaration
//---------------------------------------------------------------------------
#ifndef __DSOUND_INCLUDED__
struct IDirectSound;
#endif
class iTVPSoundBuffer;


/*]*/

//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------

#define TVP_WSB_ACCESS_FREQ (8)  // wave sound buffer access frequency (hz)

#define TVP_TIMEOFS_INVALID_VALUE ((tjs_int)(- 2147483648LL)) // invalid value for 32bit time offset

//---------------------------------------------------------------------------

TJS_EXP_FUNC_DEF(void, TVPReleaseDirectSound, ());
TJS_EXP_FUNC_DEF(IDirectSound *, TVPGetDirectSound, ());
extern void TVPResetVolumeToAllSoundBuffer();
extern void TVPSetWaveSoundBufferUse3DMode(bool b);
extern bool TVPGetWaveSoundBufferUse3DMode();
extern void TVPWaveSoundBufferCommitSettings();
#if 0
extern tjs_int TVPVolumeToDSAttenuate(tjs_int volume);
extern tjs_int TVPDSAttenuateToVolume(tjs_int att);
extern tjs_int TVPPanToDSAttenuate(tjs_int volume);
extern tjs_int TVPDSAttenuateToPan(tjs_int att);
#endif
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNI_WaveSoundBuffer : Wave Native Instance
//---------------------------------------------------------------------------
class tTVPWaveLoopManager;
class tTVPWaveSoundBufferDecodeThread;
class tTJSNI_WaveSoundBuffer : public tTJSNI_BaseWaveSoundBuffer
{
	typedef  tTJSNI_BaseWaveSoundBuffer inherited;

public:
	tTJSNI_WaveSoundBuffer();
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

	//-- buffer management ------------------------------------------------
private:
	iTVPSoundBuffer* SoundBuffer;
#if 0
	LPDIRECTSOUND3DBUFFER Sound3DBuffer;
#endif
	void ThrowSoundBufferException(const ttstr &reason);
	void TryCreateSoundBuffer(bool use3d);
	void CreateSoundBuffer();
	void DestroySoundBuffer();
	void ResetSoundBuffer();
	void ResetSamplePositions();

#if 0
	WAVEFORMATEXTENSIBLE Format;
#endif
	tTVPWaveFormat C_InputFormat;
	tTVPWaveFormat InputFormat;

	tjs_int BufferBytes;
	tjs_int AccessUnitBytes;
	tjs_int AccessUnitSamples;
	tjs_int L2AccessUnitBytes;

	tjs_int L2BufferUnits;
	tjs_int L1BufferUnits;

	tjs_int Level2BufferSize;
	tjs_uint8 *Level2Buffer;
public:
	void FreeDirectSoundBuffer(bool disableevent = true)
	{
		// called at exit ( system uninitialization )
		bool b = CanDeliverEvents;
		if(disableevent)
			CanDeliverEvents = false; // temporarily disables event derivering
		Stop();
		DestroySoundBuffer();
		CanDeliverEvents = b;
	}


	//-- playing stuff ----------------------------------------------------
private:
	tTJSCriticalSection BufferCS;
	tTJSCriticalSection L2BufferCS;
	tTJSCriticalSection L2BufferRemainCS;

public:
	tTJSCriticalSection & GetBufferCS() { return BufferCS; }

private:
	tTVPWaveDecoder * Decoder;
	tTVPWaveSoundBufferDecodeThread * Thread;
public:
	bool ThreadCallbackEnabled;
private:
	bool BufferPlaying; // whether this sound buffer is playing
	bool DSBufferPlaying; // whether the DS buffer is 'actually' playing
	bool Paused;

	bool UseVisBuffer;

	tjs_int SoundBufferPrevReadPos;
	tjs_int SoundBufferWritePos;
	tjs_int PlayStopPos; // in bytes

	tjs_int L2BufferReadPos; // in unit
	tjs_int L2BufferWritePos;
	tjs_int L2BufferRemain;
	bool L2BufferEnded;
	tjs_uint8 *VisBuffer; // buffer for visualization
	tjs_int *L2BufferDecodedSamplesInUnit;
	tTVPWaveSegmentQueue *L1BufferSegmentQueues;
	std::vector<tTVPWaveLabel> LabelEventQueue;
	tjs_int64 *L1BufferDecodeSamplePos;
	tTVPWaveSegmentQueue *L2BufferSegmentQueues;

	tjs_int64 DecodePos; // decoded samples from directsound buffer play
	tjs_int64 LastCheckedDecodePos; // last sured position (-1 for not checked) and 
	tjs_uint64 LastCheckedTick; // last sured tick time

	bool Looping;

	void Clear();

	tjs_uint Decode(void *buffer, tjs_uint bufsamplelen,
		tTVPWaveSegmentQueue & segments);

public:
	bool FillL2Buffer(bool firstwrite, bool fromdecodethread);

private:
	void PrepareToReadL2Buffer(bool firstread);
	tjs_uint ReadL2Buffer(void *buffer,
		tTVPWaveSegmentQueue & segments);

	void FillDSBuffer(tjs_int writepos,
		tTVPWaveSegmentQueue & segments);
public:
	bool FillBuffer(bool firstwrite = false, bool allowpause = true);

private:
	void ResetLastCheckedDecodePos(uint32_t pp = (uint32_t) - 1);

public:
	tjs_int FireLabelEventsAndGetNearestLabelEventStep(tjs_int64 tick);
	tjs_int GetNearestEventStep();
	void FlushAllLabelEvents();

private:
	void StartPlay();
	void StopPlay();

public:
	void Play();
	void Stop();
	void SetBufferPaused(bool bPaused);

	bool GetPaused() const { return Paused; }
	void SetPaused(bool b);

	tjs_int GetBitsPerSample() const {
		return InputFormat.BitsPerSample;
#if 0
		if(Format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
			return Format.Samples.wValidBitsPerSample;
		else
			return Format.Format.wBitsPerSample;
#endif
	}
	tjs_int GetChannels() const { return InputFormat.Channels; }

protected:
	void TimerBeatHandler(); // override

public:
	void Open(const ttstr & storagename);

public:
	void SetLooping(bool b);
	bool GetLooping() const { return Looping; }

    tjs_uint64 GetSamplePosition();
	void SetSamplePosition(tjs_uint64 pos);

    tjs_uint64 GetPosition();
	void SetPosition(tjs_uint64 pos);

	tjs_uint64 GetTotalTime();

	//-- volume/pan/3D position/freq stuff -------------------------------------
private:
	tjs_int Volume;
	tjs_int Volume2;
	tjs_int Frequency;
	static tjs_int GlobalVolume;
	static tTVPSoundGlobalFocusMode GlobalFocusMode;

	bool BufferCanControlPan;
	bool BufferCanControlFrequency;
	tjs_int Pan; // -100000 .. 0 .. 100000
	D3DVALUE PosX, PosY, PosZ; // 3D position

public:
	void SetVolumeToSoundBuffer();

public:
	void SetVolume(tjs_int v);
	tjs_int GetVolume() const { return Volume; }
	void SetVolume2(tjs_int v);
	tjs_int GetVolume2() const { return Volume2; }
	void SetPan(tjs_int v);
	tjs_int GetPan() const { return Pan; }
	static void SetGlobalVolume(tjs_int v);
	static tjs_int GetGlobalVolume() { return GlobalVolume; }
	static void SetGlobalFocusMode(tTVPSoundGlobalFocusMode b);
	static tTVPSoundGlobalFocusMode GetGlobalFocusMode()
		{ return GlobalFocusMode; }

private:
	void Set3DPositionToBuffer();
public:
	void SetPos(D3DVALUE x, D3DVALUE y, D3DVALUE z);
	void SetPosX(D3DVALUE v);
	D3DVALUE GetPosX() const {return PosX;}
	void SetPosY(D3DVALUE v);
	D3DVALUE GetPosY() const {return PosY;}
	void SetPosZ(D3DVALUE v);
	D3DVALUE GetPosZ() const {return PosZ;}

private:
	void SetFrequencyToBuffer();
public:
	tjs_int GetFrequency() const { return Frequency; }
	void SetFrequency(tjs_int freq);

	//-- visualization stuff ----------------------------------------------
public:
	void SetUseVisBuffer(bool b);
	bool GetUseVisBuffer() const { return UseVisBuffer; }

protected:
	void ResetVisBuffer(); // reset or recreate visualication buffer
	void DeallocateVisBuffer();

	void CopyVisBuffer(tjs_int16 *dest, const tjs_uint8 *src,
		tjs_int numsamples, tjs_int channels);
public:
	tjs_int GetVisBuffer(tjs_int16 *dest, tjs_int numsamples, tjs_int channels,
		tjs_int aheadsamples);
};
//---------------------------------------------------------------------------

#endif
