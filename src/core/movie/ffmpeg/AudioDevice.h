#pragma once
#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include "AEChannelInfo.h"
#include "AEStream.h"
#include "Thread.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

NS_KRMOVIE_BEGIN

typedef struct stDVDAudioFrame DVDAudioFrame;

class CDVDClock;

class CDVDAudio : IAEClockCallback
{
public:
  CDVDAudio(CDVDClock *clock);
  ~CDVDAudio();

 // void SetVolume(float fVolume);
 // void SetDynamicRangeCompression(long drc);
 // float GetCurrentAttenuation();
  void Pause();
  void Resume();
  bool Create(const DVDAudioFrame &audioframe, AVCodecID codec, bool needresampler);
  bool IsValidFormat(const DVDAudioFrame &audioframe);
  void Destroy();
  unsigned int AddPackets(const DVDAudioFrame &audioframe);
  double GetPlayingPts();
  double GetCacheTime();
  double GetCacheTotal(); // returns total amount the audio device can buffer
  double GetDelay(); // returns the time it takes to play a packet if we add one at this time
  double GetSyncError();
  void SetSyncErrorCorrection(double correction);
  double GetResampleRatio();
  void SetResampleMode(int mode);
  void Flush();
  void Drain();
  void AbortAddPackets();

  double GetClock();
  double GetClockSpeed();
  IAEStream *m_pAudioStream;

protected:

  double m_playingPts;
  double m_timeOfPts;
  double m_syncError;
  unsigned int m_syncErrorTime;
  double m_resampleRatio;
  std::recursive_mutex m_critSection;

  unsigned int m_sampeRate;
  int m_iBitsPerSample;
  bool m_bPassthrough;
  CAEChannelInfo m_channelLayout;
  bool m_bPaused;

  std::atomic_bool m_bAbort;
  CDVDClock *m_pClock;
};
NS_KRMOVIE_END