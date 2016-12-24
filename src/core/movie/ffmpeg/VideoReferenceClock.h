#pragma once
#include "Thread.h"

NS_KRMOVIE_BEGIN
class CVideoSync;

class CVideoReferenceClock /*: public CThread*/
{
  public:
    CVideoReferenceClock();
    virtual ~CVideoReferenceClock();

    int64_t GetTime(bool interpolated = true);
    void    SetSpeed(double Speed);
    double  GetSpeed();
    double  GetRefreshRate(double* interval = nullptr);
    bool    GetClockInfo(int& MissedVblanks, double& ClockSpeed, double& RefreshRate);

  private:
  //  void    Process() override;
    void Start();
    void    UpdateRefreshrate();
    void    UpdateClock(int NrVBlanks, bool CheckMissed);
    double  UpdateInterval() const;
    int64_t TimeOfNextVblank() const;
    static void CBUpdateClock(int NrVBlanks, uint64_t time, CVideoReferenceClock *clock);

    int64_t m_CurrTime;          //the current time of the clock when using vblank as clock source
    int64_t m_LastIntTime;       //last interpolated clock value, to make sure the clock doesn't go backwards
    double  m_CurrTimeFract;     //fractional part that is lost due to rounding when updating the clock
    double  m_ClockSpeed;        //the frequency of the clock set by VideoPlayer
    int64_t m_ClockOffset;       //the difference between the vblank clock and systemclock, set when vblank clock is stopped
    int64_t m_SystemFrequency;   //frequency of the systemclock

    bool    m_UseVblank;         //set to true when vblank is used as clock source
    double  m_RefreshRate;       //current refreshrate
    int     m_MissedVblanks;     //number of clock updates missed by the vblank clock
    int     m_TotalMissedVblanks;//total number of clock updates missed, used by codec information screen
    int64_t m_VblankTime;        //last time the clock was updated when using vblank as clock

    CCriticalSection m_CritSection;

//    CVideoSync *m_pVideoSync;
};
NS_KRMOVIE_END