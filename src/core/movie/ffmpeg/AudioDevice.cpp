#include "AudioDevice.h"
#include "Clock.h"
#include "AudioCodec.h"
#include "AEFactory.h"
#include "AEAudioFormat.h"
#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif
#include "AEStreamData.h"
#include <thread>
NS_KRMOVIE_BEGIN
CDVDAudio::CDVDAudio(CDVDClock *clock) : m_pClock(clock)
{
  m_pAudioStream = NULL;
  m_bPassthrough = false;
  m_iBitsPerSample = 0;
  m_sampeRate = 0;
  m_bPaused = true;
  m_playingPts = DVD_NOPTS_VALUE; //silence coverity uninitialized warning, is set elsewhere
  m_timeOfPts = 0.0; //silence coverity uninitialized warning, is set elsewhere
  m_syncError = 0.0;
  m_syncErrorTime = 0;
}

CDVDAudio::~CDVDAudio()
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);
  if (m_pAudioStream)
    CAEFactory::FreeStream(m_pAudioStream);
}

bool CDVDAudio::Create(const DVDAudioFrame &audioframe, AVCodecID codec, bool needresampler)
{
//   CLog::Log(LOGNOTICE,
//     "Creating audio stream (codec id: %i, channels: %i, sample rate: %i, %s)",
//     codec,
//     audioframe.format.m_channelLayout.Count(),
//     audioframe.format.m_sampleRate,
//     audioframe.passthrough ? "pass-through" : "no pass-through"
//   );

  // if passthrough isset do something else
	std::lock_guard<std::recursive_mutex> lock(m_critSection);
  unsigned int options = needresampler && !audioframe.passthrough ? AESTREAM_FORCE_RESAMPLE : 0;
  options |= AESTREAM_PAUSED;

  AEAudioFormat format = audioframe.format;
  m_pAudioStream = CAEFactory::MakeStream(
    format,
    options,
    this
  );
  if (!m_pAudioStream)
    return false;

  m_sampeRate = audioframe.format.m_sampleRate;
  m_iBitsPerSample = audioframe.bits_per_sample;
  m_bPassthrough = audioframe.passthrough;
  m_channelLayout = audioframe.format.m_channelLayout;

//   if (m_pAudioStream->HasDSP())
//     m_pAudioStream->SetFFmpegInfo(audioframe.profile, audioframe.matrix_encoding, audioframe.audio_service_type);

  //SetDynamicRangeCompression((long)(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_VolumeAmplification * 100));

  return true;
}

void CDVDAudio::Destroy()
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);

  if (m_pAudioStream)
    CAEFactory::FreeStream(m_pAudioStream);

  m_pAudioStream = NULL;
  m_sampeRate = 0;
  m_iBitsPerSample = 0;
  m_bPassthrough = false;
  m_bPaused = true;
  m_playingPts = DVD_NOPTS_VALUE;
}

unsigned int CDVDAudio::AddPackets(const DVDAudioFrame &audioframe)
{
  m_bAbort = false;

  std::unique_lock<std::recursive_mutex> lock(m_critSection);

  if(!m_pAudioStream)
    return 0;

  CAESyncInfo info = m_pAudioStream->GetSyncInfo();
  if (info.state == CAESyncInfo::SYNC_INSYNC)
  {
    unsigned int newTime = info.errortime;
    if (newTime != m_syncErrorTime)
    {
      m_syncErrorTime = info.errortime;
      m_syncError = info.error / 1000 * DVD_TIME_BASE;
      m_resampleRatio = info.rr;
    }
  }
  else
  {
    m_syncErrorTime = 0;
    m_syncError = 0.0;
  }

  //Calculate a timeout when this definitely should be done
  double timeout;
  timeout  = DVD_SEC_TO_TIME(m_pAudioStream->GetDelay()) + audioframe.duration;
  timeout += DVD_SEC_TO_TIME(1.0);
  timeout += m_pClock->GetAbsoluteClock();

  unsigned int total = audioframe.nb_frames;
  unsigned int frames = audioframe.nb_frames;
  unsigned int offset = 0;
  do
  {
    double pts = (offset == 0) ? audioframe.pts / DVD_TIME_BASE * 1000 : 0.0;
    unsigned int copied = m_pAudioStream->AddData(audioframe.data, offset, frames, pts);
    offset += copied;
    frames -= copied;
    if (frames <= 0)
      break;

    if (copied == 0 && timeout < m_pClock->GetAbsoluteClock())
    {
  //    CLog::Log(LOGERROR, "CDVDAudio::AddPacketsRenderer - timeout adding data to renderer");
      break;
    }

	lock.unlock();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
    lock.lock();
  } while (!m_bAbort);

  m_playingPts = audioframe.pts + audioframe.duration - GetDelay();
  m_timeOfPts = m_pClock->GetAbsoluteClock();

  return total - frames;
}

void CDVDAudio::Drain()
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Drain(true);
}

// void CDVDAudio::SetVolume(float volume)
// {
//   CSingleLock lock (m_critSection);
//   if (m_pAudioStream)
//     m_pAudioStream->SetVolume(volume);
// }

// void CDVDAudio::SetDynamicRangeCompression(long drc)
// {
//   CSingleLock lock (m_critSection);
//   if (m_pAudioStream)
//     m_pAudioStream->SetAmplification(powf(10.0f, (float)drc / 2000.0f));
//}

// float CDVDAudio::GetCurrentAttenuation()
// {
//   CSingleLock lock (m_critSection);
//   if (m_pAudioStream)
//     return m_pAudioStream->GetVolume();
//   else
//     return 1.0f;
// }

void CDVDAudio::Pause()
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Pause();
 // CLog::Log(LOGDEBUG,"CDVDAudio::Pause - pausing audio stream");
  m_playingPts = DVD_NOPTS_VALUE;
}

void CDVDAudio::Resume()
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);
  if (m_pAudioStream)
    m_pAudioStream->Resume();
 // CLog::Log(LOGDEBUG,"CDVDAudio::Resume - resume audio stream");
}

double CDVDAudio::GetDelay()
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);

  double delay = 0.3;
  if(m_pAudioStream)
    delay = m_pAudioStream->GetDelay();

  return delay * DVD_TIME_BASE;
}

void CDVDAudio::Flush()
{
  m_bAbort = true;

  std::lock_guard<std::recursive_mutex> lock(m_critSection);
  if (m_pAudioStream)
  {
    m_pAudioStream->Flush();
 //   CLog::Log(LOGDEBUG,"CDVDAudio::Flush - flush audio stream");
  }
  m_playingPts = DVD_NOPTS_VALUE;
  m_syncError = 0.0;
  m_syncErrorTime = 0;
}

void CDVDAudio::AbortAddPackets()
{
  m_bAbort = true;
}

bool CDVDAudio::IsValidFormat(const DVDAudioFrame &audioframe)
{
  if(!m_pAudioStream)
    return false;

  if(audioframe.passthrough != m_bPassthrough)
    return false;

  if(m_sampeRate != audioframe.format.m_sampleRate ||
     m_iBitsPerSample != audioframe.bits_per_sample ||
     m_channelLayout != audioframe.format.m_channelLayout)
    return false;

  return true;
}

double CDVDAudio::GetCacheTime()
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);
  if(!m_pAudioStream)
    return 0.0;

  double delay = 0.0;
  if(m_pAudioStream)
    delay = m_pAudioStream->GetCacheTime();

  return delay;
}

double CDVDAudio::GetCacheTotal()
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);
  if(!m_pAudioStream)
    return 0.0;
  return m_pAudioStream->GetCacheTotal();
}

double CDVDAudio::GetPlayingPts()
{
  if (m_playingPts == DVD_NOPTS_VALUE)
    return 0.0;

  double now = m_pClock->GetAbsoluteClock();
  double diff = now - m_timeOfPts;
  double cache = GetCacheTime();
  double played = 0.0;

  if (diff < cache)
    played = diff;
  else
    played = cache;

  m_timeOfPts = now;
  m_playingPts += played;
  return m_playingPts;
}

double CDVDAudio::GetSyncError()
{
  return m_syncError;
}

void CDVDAudio::SetSyncErrorCorrection(double correction)
{
  m_syncError += correction;
}

double CDVDAudio::GetResampleRatio()
{
  return m_resampleRatio;
}

void CDVDAudio::SetResampleMode(int mode)
{
	std::lock_guard<std::recursive_mutex> lock(m_critSection);
  if(m_pAudioStream)
  {
    m_pAudioStream->SetResampleMode(mode);
  }
}

double CDVDAudio::GetClock()
{
  if (m_pClock)
    return (m_pClock->GetClock() + m_pClock->GetVsyncAdjust()) / DVD_TIME_BASE * 1000;
  else
    return 0.0;
}

double CDVDAudio::GetClockSpeed()
{
  if (m_pClock)
    return m_pClock->GetClockSpeed();
  else
    return 1.0;
}
NS_KRMOVIE_END