#include "CPacketRingBuffer.h"
#include "CPacketForMultiThread.h"

#define TESTCNT (100)

//패킷 링버퍼에 패킷을 담고, 포인터에 있는 패킷을 꺼내서 확인
int main()
  {
	CPacketRingBuffer rb(1024);
	for (int i = 0; i < TESTCNT; i++)
	{
		char c = i;
		CPacket* cp = CPacket::Alloc();
		cp->PutData(&c, sizeof(c));
		rb.Enqueue(cp);
	}

	for (int i = 0; i < TESTCNT; i++)
	{
		CPacket* cp;
		rb.Dequeue(cp);

		char c;
		cp->GetData(&c, sizeof(c));
		if (c != i)
		{
			__debugbreak();
		}
	}
	printf("이상 없이 종료");
}
