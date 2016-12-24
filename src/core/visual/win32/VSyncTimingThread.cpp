
#include "tjsCommHead.h"

#include <mmsystem.h>

#include "VSyncTimingThread.h"
#include "WindowImpl.h"
#include "EventIntf.h"
#include "UserEvent.h"
#include "DebugIntf.h"
#include "MsgIntf.h"

//---------------------------------------------------------------------------
tTVPVSyncTimingThread::tTVPVSyncTimingThread(tTJSNI_Window* owner)
	 : tTVPThread(true), EventQueue(this,&tTVPVSyncTimingThread::Proc), OwnerWindow(owner)
{
	SleepTime = 1;
	LastVBlankTick = 0;
	VSyncInterval = 16; // 約60FPS
	Enabled = false;
	EventQueue.Allocate();
	MeasureVSyncInterval();
	Resume();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
tTVPVSyncTimingThread::~tTVPVSyncTimingThread()
{
	Terminate();
	Resume();
	Event.Set();
	WaitFor();
	EventQueue.Deallocate();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void tTVPVSyncTimingThread::Execute()
{
	while(!GetTerminated())
	{
		// SleepTime と LastVBlankTick を得る
		DWORD sleep_time, last_vblank_tick;
		{	// thread-protected
			tTJSCriticalSectionHolder holder(CS);
			sleep_time = SleepTime;
			last_vblank_tick = LastVBlankTick;
		}

		// SleepTime 分眠る
		// LastVBlankTick から起算し、SleepTime 分眠る
		DWORD sleep_start_tick = timeGetTime();

		DWORD sleep_time_adj = sleep_start_tick - last_vblank_tick;

		if(sleep_time_adj < sleep_time)
		{
			::Sleep(sleep_time - sleep_time_adj);
		}
		else
		{
			// 普通、メインスレッド内で Event.Set() したならば、
			// タイムスライス(長くて10ms) が終わる頃は
			// ここに来ているはずである。
			// sleep_time は通常 10ms より長いので、
			// ここに来るってのは異常。
			// よほどシステムが重たい状態になってると考えられる。
			// そこで立て続けに イベントをポストするわけにはいかないので
			// 適当な時間(本当に適当) 眠る。
			::Sleep(5);
		}

		// イベントをポストする
		NativeEvent ev(TVP_EV_VSYNC_TIMING_THREAD);
		ev.LParam = (LPARAM)sleep_start_tick;
		EventQueue.PostEvent(ev);

		Event.WaitFor(0x7fffffff); // vsync まで待つ
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void tTVPVSyncTimingThread::Proc( NativeEvent& ev )
{
	if(ev.Message != TVP_EV_VSYNC_TIMING_THREAD) {
		EventQueue.HandlerDefault(ev);
		return;
	}
	if( OwnerWindow == NULL ) return;

	// tTVPVSyncTimingThread から投げられたメッセージ

	tjs_int in_vblank = 0;
	tjs_int delayed = 0;
	bool supportvwait = OwnerWindow->WaitForVBlank( &in_vblank, &delayed );
	if( supportvwait == false )
	{	// VBlank待ちはサポートされていないので、気にせずそのまま進行(待ち時間はいい加減だが気にしないことにする)
		in_vblank = 0;
		delayed = 0;
	}

	// タイマの時間原点を設定する
	if(!delayed)
	{
		tTJSCriticalSectionHolder holder(CS);
		LastVBlankTick = timeGetTime(); // これが次に眠る時間の起算点になる
	}
	else
	{
		tTJSCriticalSectionHolder holder(CS);
		LastVBlankTick += VSyncInterval; // これが次に眠る時間の起算点になる(おおざっぱ)
		if((long) (timeGetTime() - (LastVBlankTick + SleepTime)) <= 0)
		{
			// 眠った後、次に起きようとする時間がすでに過去なので眠れません
			LastVBlankTick = timeGetTime(); // 強制的に今の時刻にします
		}
	}

	// 画面の更新を行う (DrawDeviceのShowメソッドを呼ぶ)
	OwnerWindow->DeliverDrawDeviceShow();

	// もし vsync 待ちを行う直前、すでに vblank に入っていた場合は、
	// 待つ時間が長すぎたと言うことである
	if(in_vblank)
	{
		// その場合は SleepTime を減らす
		tTJSCriticalSectionHolder holder(CS);
		if(SleepTime > 8) SleepTime --;
	}
	else
	{
		// vblank で無かった場合は二つの場合が考えられる
		// 1. vblank 前だった
		// 2. vblank 後だった
		// どっちかは分からないが
		// SleepTime を増やす。ただしこれが VSyncInterval を超えるはずはない。
		tTJSCriticalSectionHolder holder(CS);
		SleepTime ++;
		if(SleepTime > VSyncInterval) SleepTime = VSyncInterval;
	}

	// タイマを起動する
	Event.Set();

	// ContinuousHandler を呼ぶ
	// これは十分な時間をとれるよう、vsync 待ちの直後に呼ばれる
	TVPProcessContinuousHandlerEventFlag = true; // set flag to invoke continuous handler on next idle
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void tTVPVSyncTimingThread::MeasureVSyncInterval()
{
	TVPEnsureDirect3DObject();

	DWORD vsync_interval = 10000;
	DWORD vsync_rate = 0;

	HDC dc = ::GetDC(0);
	vsync_rate = ::GetDeviceCaps(dc, VREFRESH);
	::ReleaseDC(0, dc);

	if(vsync_rate != 0)
		vsync_interval = 1000 / vsync_rate;
	else
		vsync_interval = 0;

	TVPAddLog( TVPFormatMessage(TVPRoughVsyncIntervalReadFromApi,ttstr((int)vsync_interval)) );

	// vsync 周期は適切っぽい？
	if(vsync_interval < 6 || vsync_interval > 66)
	{
		TVPAddLog( (const tjs_char*)TVPRoughVsyncIntervalStillSeemsWrong );
		vsync_interval = 16;
	}

	VSyncInterval = vsync_interval;
}
//---------------------------------------------------------------------------
