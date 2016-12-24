
#ifndef __VSYNC_TIMING_THREAD_H__
#define __VSYNC_TIMING_THREAD_H__

#include "ThreadIntf.h"
#include "NativeEventQueue.h"

//---------------------------------------------------------------------------
// VSync用のタイミングを発生させるためのスレッド
//---------------------------------------------------------------------------
class tTVPVSyncTimingThread : public tTVPThread
{
	tjs_uint32 SleepTime;
	tTVPThreadEvent Event;
	tTJSCriticalSection CS;
	tjs_uint32 VSyncInterval; //!< VSync の間隔(参考値)
	tjs_uint32 LastVBlankTick; //!< 最後の vblank の時間

	bool Enabled;

	NativeEventQueue<tTVPVSyncTimingThread> EventQueue;

	class tTJSNI_Window* OwnerWindow;
public:
	tTVPVSyncTimingThread(class tTJSNI_Window* owner);
	~tTVPVSyncTimingThread();

protected:
	void Execute();
	void Proc( NativeEvent& ev );

public:
	void MeasureVSyncInterval(); // VSyncInterval を計測する
};
//---------------------------------------------------------------------------

#endif // __VSYNC_TIMING_THREAD_H__
