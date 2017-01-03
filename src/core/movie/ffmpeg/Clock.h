#pragma once
#include "VideoReferenceClock.h"
#include <memory>

NS_KRMOVIE_BEGIN

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE 0xFFF0000000000000

#define DVD_TIME_TO_MSEC(x) ((int)((double)(x) * 1000 / DVD_TIME_BASE))
#define DVD_SEC_TO_TIME(x)  ((double)(x) * DVD_TIME_BASE)
#define DVD_MSEC_TO_TIME(x) ((double)(x) * DVD_TIME_BASE / 1000)

#define DVD_PLAYSPEED_PAUSE       0       // frame stepping
#define DVD_PLAYSPEED_NORMAL      1000

class CDVDClock {
public:
	CDVDClock();
	~CDVDClock();

	double GetClock(bool interpolated = true);
	double GetClock(double& absolute, bool interpolated = true);

	double ErrorAdjust(double error, const char* log);
	void Discontinuity(double clock, double absolute);
	void Discontinuity(double clock = 0LL)
	{
		Discontinuity(clock, GetAbsoluteClock());
	}

	void Reset() { m_bReset = true; }
	void SetSpeed(int iSpeed);
	void SetSpeedAdjust(double adjust);
	double GetSpeedAdjust();

	double GetClockSpeed(); /**< get the current speed of the clock relative normal system time */

  /* tells clock at what framerate video is, to  *
   * allow it to adjust speed for a better match */
	int UpdateFramerate(double fps, double* interval = NULL);

	void SetMaxSpeedAdjust(double speed);

	double GetAbsoluteClock(bool interpolated = true);
	double GetFrequency() { return (double)m_systemFrequency; }

	bool GetClockInfo(int& MissedVblanks, double& ClockSpeed, double& RefreshRate) const;
	void SetVsyncAdjust(double adjustment);
	double GetVsyncAdjust();
	void Pause(bool pause);

protected:
	double SystemToAbsolute(int64_t system);
	int64_t AbsoluteToSystem(double absolute);
	double SystemToPlaying(int64_t system);

	CCriticalSection m_critSection;
	int64_t m_systemUsed;
	int64_t m_startClock;
	int64_t m_pauseClock;
	double m_iDisc;
	bool m_bReset;
	bool m_paused;
	int m_speedAfterPause;
	std::unique_ptr<CVideoReferenceClock> m_videoRefClock;

	int64_t m_systemFrequency;
	int64_t m_systemOffset;
	CCriticalSection m_systemsection;

	int64_t m_systemAdjust;
	int64_t m_lastSystemTime;
	double m_speedAdjust;
	double m_vSyncAdjust;
	double m_frameTime;

	double m_maxspeedadjust;
	CCriticalSection m_speedsection;
};

NS_KRMOVIE_END