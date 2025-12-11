#pragma once
class CRingBuffer;

//------------------------------------
// 메시지 큐
// 워커 스레드가 수신한 메시지를 락을 걸고 메시지 큐에 던져 놓으면
// 컨텐츠 스레드가 일어나서 해당 메시지를 가져가 처리하도록 한다.
//------------------------------------
class MessageQueue
{
public:
	MessageQueue();
	MessageQueue(int bufSize);
	~MessageQueue();

	int enqMsgbuf(char* c, int len);

	int deqMsgbuf(char* c, int len);

private:
	CRITICAL_SECTION msg_cs;
	CRingBuffer* MsgBuf;
};