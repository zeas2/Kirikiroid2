//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Thread base class
//---------------------------------------------------------------------------
#define NOMINMAX
#include "tjsCommHead.h"

//#include <process.h>
#include <algorithm>

#include "ThreadIntf.h"
#include "ThreadImpl.h"
#include "MsgIntf.h"
#include "DebugIntf.h"

#if defined(CC_TARGET_OS_IPHONE) || defined(__aarch64__)
#else
//#define USING_THREADPOOL11
#endif

#ifdef USING_THREADPOOL11
#include "threadpool11/pool.hpp"
#endif
#include <thread>

//---------------------------------------------------------------------------
// tTVPThread : a wrapper class for thread
//---------------------------------------------------------------------------
tTVPThread::tTVPThread(bool suspended)
{
	Terminated = false;
	Suspended = suspended;

	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0) {
		TVPThrowInternalError;
	}
	if (pthread_create(&Handle, &attr, StartProc, this) != 0) {
		pthread_attr_destroy(&attr);
		TVPThrowInternalError;
	}
	pthread_attr_destroy(&attr);
}
//---------------------------------------------------------------------------
tTVPThread::~tTVPThread()
{
	//CloseHandle(Handle);
}
//---------------------------------------------------------------------------
void * tTVPThread::StartProc(void * arg)
{
	tTVPThread* _this = ((tTVPThread*)arg);
	if (_this->Suspended) {
		std::unique_lock<std::mutex> lk(_this->_mutex);
		_this->_cond.wait(lk);
	}
	_this->Execute();
	TVPOnThreadExited();
	return nullptr;
}
//---------------------------------------------------------------------------
void tTVPThread::WaitFor()
{
	void *result;
	pthread_join(Handle, &result);
}
//---------------------------------------------------------------------------
tTVPThreadPriority tTVPThread::GetPriority()
{
	sched_param npri = { 0 };
	int policy = 0;
	pthread_getschedparam(Handle, &policy, &npri);

	switch (policy)
	{
	case 5/*SCHED_IDLE*/: return ttpIdle;
	case 3/*SCHED_BATCH*/: return (npri.sched_priority == 0) ? ttpLowest : ttpLower;
	case 0/*SCHED_NORMAL*/:
		switch (npri.sched_priority) {
		case 0: return ttpNormal;
		case 1: return ttpHigher;
		case 2: return ttpHighest;
		}
		break;
	case 2/*SCHED_RR*/: return ttpTimeCritical;
	}

	return ttpNormal;
}
//---------------------------------------------------------------------------
void tTVPThread::SetPriority(tTVPThreadPriority pri)
{
	sched_param npri = { 0 }; //SCHED_NORMAL
	int policy = 0;
	switch (pri)
	{
	case ttpIdle:			policy = 5/*SCHED_IDLE*/;			break;
	case ttpLowest:			policy = 3/*SCHED_BATCH*/;	npri.sched_priority = 0;		break;
	case ttpLower:			policy = 3/*SCHED_BATCH*/;	npri.sched_priority = 1;        break;
	case ttpNormal:			policy = 0/*SCHED_NORMAL*/;		break;
	case ttpHigher:			policy = 0/*SCHED_NORMAL*/;	npri.sched_priority = 1;    break;
	case ttpHighest:		policy = 0/*SCHED_NORMAL*/;	npri.sched_priority = 2;	break;
	case ttpTimeCritical:	policy = 2/*SCHED_RR*/;	    break;
	}
	pthread_setschedparam(Handle, policy, &npri);
}
//---------------------------------------------------------------------------
// void tTVPThread::Suspend()
// {
// 	SuspendThread(Handle);
// }
//---------------------------------------------------------------------------
void tTVPThread::Resume()
{
	Suspended = false;
	_cond.notify_one();
	//while((tjs_int32)ResumeThread(Handle) > 1) ;
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// tTVPThreadEvent
//---------------------------------------------------------------------------
void tTVPThreadEvent::Set()
{
	std::unique_lock<std::mutex> lk(Mutex);
	Handle.notify_one();
}
//---------------------------------------------------------------------------
void tTVPThreadEvent::WaitFor(tjs_uint timeout)
{
	// wait for event;
	// returns true if the event is set, otherwise (when timed out) returns false.

	std::unique_lock<std::mutex> lk(Mutex);
	if (timeout != 0) {
		Handle.wait_for(lk, std::chrono::milliseconds(timeout));
	} else {
		Handle.wait(lk);
	}
#if 0
	DWORD state = WaitForSingleObject(Handle, timeout == 0 ? INFINITE : timeout);

	if(state == WAIT_OBJECT_0) return true;
	return false;
#endif
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
tjs_int TVPDrawThreadNum = 1;

static std::vector<tjs_int> TVPProcesserIdList;
static tjs_int TVPThreadTaskNum, TVPThreadTaskCount;

//---------------------------------------------------------------------------
static tjs_int GetProcesserNum(void)
{
  static tjs_int processor_num = 0;
  if (! processor_num) {
	  processor_num = std::thread::hardware_concurrency();
	tjs_char tmp[34];
	TVPAddLog(ttstr(TJS_W("Detected CPU core(s): ")) + TJS_tTVInt_to_str(processor_num, tmp));
  }
  return processor_num;
}

tjs_int TVPGetProcessorNum(void)
{
  return GetProcesserNum();
}

//---------------------------------------------------------------------------
tjs_int TVPGetThreadNum(void)
{
  tjs_int threadNum = TVPDrawThreadNum ? TVPDrawThreadNum : GetProcesserNum();
  threadNum = std::min(threadNum, TVPMaxThreadNum);
  return threadNum;
}

//---------------------------------------------------------------------------
void TVPExecThreadTask(int numThreads, TVP_THREAD_TASK_FUNC func)
{
  if (numThreads == 1) {
    func(0);
    return;
  }
#if !defined(USING_THREADPOOL11)
#pragma omp parallel for schedule(static)
  for (int i = 0; i < numThreads; ++i)
	  func(i);
#else
  static threadpool11::Pool pool;
  std::vector<std::future<void>> futures;
  for (int i = 0; i < numThreads; ++i) {
	  futures.emplace_back(pool.postWork<void>(std::bind(func, i)));
  }
  for (auto& it : futures)
	  it.get();
#endif
#if 0
  ThreadInfo *threadInfo;
  threadInfo = TVPThreadList[TVPThreadTaskCount++];
  threadInfo->lpStartAddress = func;
  threadInfo->lpParameter = param;
  InterlockedIncrement(&TVPRunningThreadCount);
  while (ResumeThread(threadInfo->thread) == 0)
    Sleep(0);
#endif
}
//---------------------------------------------------------------------------

std::vector<std::function<void()>> _OnThreadExitedEvents;

void TVPOnThreadExited() {
	for (const auto &ev : _OnThreadExitedEvents) {
		ev();
	}
}

void TVPAddOnThreadExitEvent(const std::function<void()> &ev)
{
	_OnThreadExitedEvents.emplace_back(ev);
}
