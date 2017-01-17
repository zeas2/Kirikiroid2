#pragma once
#include "KRMovieDef.h"
#include "Message.h"
#include <condition_variable>
#include <list>
#include <algorithm>
#include "Thread.h"
#include <string>

NS_KRMOVIE_BEGIN

struct DVDMessageListItem
{
	DVDMessageListItem(CDVDMsg* msg, int prio)
	{
		message = msg->AddRef();
		priority = prio;
	}
	DVDMessageListItem()
	{
		message = NULL;
		priority = 0;
	}
	DVDMessageListItem(const DVDMessageListItem&) = delete;
	~DVDMessageListItem()
	{
		if (message)
			message->Release();
	}

	DVDMessageListItem& operator=(const DVDMessageListItem&) = delete;

	CDVDMsg* message;
	int priority;
};

enum MsgQueueReturnCode
{
	MSGQ_OK = 1,
	MSGQ_TIMEOUT = 0,
	MSGQ_ABORT = -1, // negative for legacy, not an error actually
	MSGQ_NOT_INITIALIZED = -2,
	MSGQ_INVALID_MSG = -3,
	MSGQ_OUT_OF_MEMORY = -4
};
#define MSGQ_IS_ERROR(c)    (c < 0)

class CDVDMessageQueue
{
public:
	CDVDMessageQueue(const std::string &owner);
	virtual ~CDVDMessageQueue();
	void Init();
	void Flush(CDVDMsg::Message message = CDVDMsg::DEMUXER_PACKET);
	void Abort();
	void End();

	MsgQueueReturnCode Put(CDVDMsg* pMsg, int priority = 0, bool front = true);

	MsgQueueReturnCode Get(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds, int &priority);
	MsgQueueReturnCode Get(CDVDMsg** pMsg, unsigned int iTimeoutInMilliSeconds) {
		int priority = 0;
		return Get(pMsg, iTimeoutInMilliSeconds, priority);
	}

	int GetDataSize() const { return m_iDataSize; }
	int GetTimeSize();
	unsigned GetPacketCount(CDVDMsg::Message type);
	bool ReceivedAbortRequest() { return m_bAbortRequest; }
	void WaitUntilEmpty();

	// non messagequeue related functions
	bool IsFull() { return GetLevel() == 100; }
	int GetLevel();

	void SetMaxDataSize(int iMaxDataSize) { m_iMaxDataSize = iMaxDataSize; }
	void SetMaxTimeSize(double sec);
	int GetMaxDataSize() const { return m_iMaxDataSize; }
	double GetMaxTimeSize() const { return m_TimeSize; }
	bool IsInited() const { return m_bInitialized; }
	bool IsDataBased() const;

private:
	std::mutex m_mtxEvent;
	std::condition_variable m_hEvent;
	CCriticalSection m_section;

	std::atomic<bool> m_bAbortRequest;
	bool m_bInitialized;
	bool m_drain;

	int m_iDataSize;
	double m_TimeFront;
	double m_TimeBack;
	double m_TimeSize;

	int m_iMaxDataSize;
	std::string m_owner;

	std::list<DVDMessageListItem> m_messages;
	std::list<DVDMessageListItem> m_prioMessages;
};
NS_KRMOVIE_END