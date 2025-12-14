#include "MessageQueue.h"

#include "CRingBuffer.h"

MessageQueue::MessageQueue()
{
	InitializeCriticalSection(&msg_cs);
	MsgBuf = nullptr;
}

MessageQueue::MessageQueue(int bufSize)
{
	InitializeCriticalSection(&msg_cs);
	MsgBuf = new CRingBuffer(bufSize + 1);
}

MessageQueue::~MessageQueue()
{
	DeleteCriticalSection(&msg_cs);
}

int MessageQueue::enqMsgbuf(char* c, int len)
{
	int ret;
	EnterCriticalSection(&msg_cs);
	ret = MsgBuf->Enqueue(c, len);
	LeaveCriticalSection(&msg_cs);
	return ret;
}

int MessageQueue::deqMsgbuf(char* c, int len)
{
	int ret;
	EnterCriticalSection(&msg_cs);
	ret = MsgBuf->Dequeue(c, len);
	LeaveCriticalSection(&msg_cs);
	return ret;
}

char* MessageQueue::GetFrontBufPtr(void)
{

	return nullptr;
}
