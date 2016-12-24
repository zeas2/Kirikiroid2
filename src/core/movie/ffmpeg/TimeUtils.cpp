#include "TimeUtils.h"

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

#ifdef _MSC_VER
#include <windows.h>
#endif
#include <time.h>
#include "tp_stub.h"
#include "TickCount.h"

NS_KRMOVIE_BEGIN
int64_t CurrentHostCounter(void)
{
	return TVPGetRoughTickCount32();
#ifdef _MSC_VER
	LARGE_INTEGER PerformanceCount;
	QueryPerformanceCounter(&PerformanceCount);
	return((int64_t)PerformanceCount.QuadPart);
#elif defined(CLOCK_MONOTONIC_RAW)
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC_RAW, &now);
	return(((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec);
#elif defined(CLOCK_MONOTONIC)
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return(((int64_t)now.tv_sec * 1000000000L) + now.tv_nsec);
#else // xcode ?
	return((int64_t)CVGetCurrentHostTime());
#endif
}

int64_t CurrentHostFrequency(void)
{
	return 1000;
#ifdef _MSC_VER
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	return((int64_t)Frequency.QuadPart);
#elif defined(CLOCK_MONOTONIC_RAW) || defined(CLOCK_MONOTONIC)
	return (int64_t)1000000000L;
#else
	return((int64_t)CVGetHostClockFrequency());
#endif
}

unsigned int CTimeUtils::frameTime = 0;

void CTimeUtils::UpdateFrameTime(bool flip)
{
	unsigned int currentTime = TVPGetRoughTickCount32();
  unsigned int last = frameTime;
  while (frameTime < currentTime)
  {
	  frameTime += (unsigned int)(1000 / 60/*g_graphicsContext.GetFPS()*/);
    // observe wrap around
    if (frameTime < last)
      break;
  }
}

unsigned int CTimeUtils::GetFrameTime()
{
  return frameTime;
}
#if 0
CDateTime CTimeUtils::GetLocalTime(time_t time)
{
  CDateTime result;

  tm *local;
#ifdef HAVE_LOCALTIME_R
  tm res = {};
  local = localtime_r(&time, &res); // Conversion to local time
#else
  local = localtime(&time); // Conversion to local time
#endif
  /*
   * Microsoft implementation of localtime returns NULL if on or before epoch.
   * http://msdn.microsoft.com/en-us/library/bf12f0hc(VS.80).aspx
   */
  if (local)
    result = *local;
  else
    result = time; // Use the original time as close enough.

  return result;
}
#endif
NS_KRMOVIE_END