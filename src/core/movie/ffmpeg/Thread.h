#pragma once
#include "KRMovieDef.h"
#include <atomic>
#include <condition_variable>
#include <thread>
#include <mutex>

NS_KRMOVIE_BEGIN
typedef std::recursive_mutex CCriticalSection;
typedef std::unique_lock<std::recursive_mutex> CSingleLock;
class CThread {
public:
	CThread();
	virtual ~CThread();
	void Create();
	bool IsRunning() const { return m_bRunning; }
	void StopThread(bool bWait = true);
	void Sleep(unsigned int milliseconds);
	bool IsCurrentThread();

protected:
	int entry();
	virtual void OnStartup() {}
	virtual void Process() = 0;
	virtual void OnExit() {}

	std::thread *m_ThreadId = nullptr;
	std::atomic<bool> m_bStop, m_bRunning;
	std::mutex m_mtxStopEvent;
	std::condition_variable m_StopEvent;
	CCriticalSection m_CriticalSection;
};

class CEvent {
public:
	void Set() { m_cond.notify_all(); }
	void Reset() {}
	bool WaitMSec(unsigned int milliSeconds) {
		CSingleLock lock(mutex);
		return m_cond.wait_for(lock, std::chrono::milliseconds(milliSeconds)) != std::cv_status::timeout;
	}
	void Wait() {
		CSingleLock lock(mutex);
		m_cond.wait(lock);
	}

protected:
	CCriticalSection mutex;
	std::condition_variable_any m_cond;
};
NS_KRMOVIE_END