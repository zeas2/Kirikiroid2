#pragma once
#include "KRMovieDef.h"
#include "TickCount.h"

NS_KRMOVIE_BEGIN
class Timer {
	unsigned int startTime;
	unsigned int totalWaitTime;
public:
	static const unsigned int InfiniteValue;
	inline Timer() : startTime(0), totalWaitTime(0) {}
	inline Timer(unsigned int millisecondsIntoTheFuture) : startTime(TVPGetRoughTickCount32()), totalWaitTime(millisecondsIntoTheFuture) {}

	inline void Set(unsigned int millisecondsIntoTheFuture) { startTime = TVPGetRoughTickCount32(); totalWaitTime = millisecondsIntoTheFuture; }
	inline bool IsTimePast() const { return totalWaitTime == InfiniteValue ? false : (totalWaitTime == 0 ? true : (TVPGetRoughTickCount32() - startTime) >= totalWaitTime); }

	inline unsigned int MillisLeft() const
	{
		if (totalWaitTime == InfiniteValue)
			return InfiniteValue;
		if (totalWaitTime == 0)
			return 0;
		unsigned int timeWaitedAlready = (TVPGetRoughTickCount32() - startTime);
		return (timeWaitedAlready >= totalWaitTime) ? 0 : (totalWaitTime - timeWaitedAlready);
	}
	
	inline void SetExpired() { totalWaitTime = 0; }
	inline void SetInfinite() { totalWaitTime = InfiniteValue; }
	inline bool IsInfinite(void) const { return (totalWaitTime == InfiniteValue); }
	inline unsigned int GetInitialTimeoutValue(void) const { return totalWaitTime; }
	inline unsigned int GetStartTime(void) const { return startTime; }
};
NS_KRMOVIE_END
