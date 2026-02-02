#include "stdafx.h"
#include "CPacketForMultiThread.h"
#include "CRingBuffer.h"

#pragma pack(push,1)
struct MsgHeader
{
	char Code; //(1byte)
	short Len; //(2byte)
	char RandKey; //(1byte)
	char CheckSum; //(1byte)
};
#pragma pack(pop)

bool Decode(MsgHeader* pHeader, char* pc);

int main()
{
	CPacket* cp = CPacket::Alloc();
	//메시지 헤더 5바이트 네트워크 헤더로 있다고 가정
	cp->setMsgHeadetSize(5);

	char testChar[] = "aaaaaaaaaabbbbbbbbbbcccccccccc1234567890abcdefghijklmn\0";
	cp->PutData(testChar, 55);
	cp->Encode(0xa9);

	CRingBuffer rb(1024);
	//char* ptemp = cp->GetBufferPtr();
	rb.Enqueue(cp->GetBufferPtr(), cp->GetDataSize());

	//수신 링버퍼 메시지 수신 과정 모방
	while (1)
	{
		//메시지 헤더 먼저 읽기
		if (rb.GetUseSize() >= sizeof(MsgHeader))
		{
			MsgHeader header;
			rb.Peek((char*) &header, sizeof(MsgHeader));
			//메시지 페이로드 길이 읽기
			if (rb.GetUseSize() >= sizeof(MsgHeader) + header.Len)
			{
				//디코딩
				if (Decode(&header, rb.GetFrontBufferPtr()))
				{
					//네트워크 헤더 제거한 나머지 컨텐츠에게 패킷에 담아서 넘겨주기
					CPacket* contentPacket = CPacket::Alloc();
					contentPacket->PutData(rb.GetFrontBufferPtr() + sizeof(MsgHeader), header.Len);
					contentPacket->AddRef();
					//OnRecv에 해당 패킷 넘겨주기
				}
				else
				{
					//디코딩이 실패했다면? 해당 메시지를 그냥 폐기하는게 맞을 것으로 생각함.
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
}

bool Decode(MsgHeader* pHeader, char* pc) 
{

	unsigned char checksum = 0;
	unsigned char beforeparaP = 0;
	unsigned char afterparaP = 0;
	unsigned char beforeDecodeP = 0;
	unsigned char afterDecodeP = 0;

	int payLoadSize = pHeader->Len;
	for (int i = 0; i < payLoadSize; i++)
	{
		char* pPacketChar = &pc[sizeof(MsgHeader) + i];
		afterDecodeP = *pPacketChar;

		afterparaP = *pPacketChar ^ (beforeDecodeP + pHeader->Code + (i + 1));
		beforeDecodeP = afterDecodeP;

		*pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey  + (i + 1));
		beforeparaP = afterparaP;
		
		checksum += *pPacketChar % 256;
	}

	//복호화가 제대로 이루어졌는지 확인
	if (pHeader->CheckSum != checksum)
	{
		printf("[Decode] checksum이 일치하지 않음. 복호화가 제대로 이루어지지 않음.\n");
		return false;
	}
	return true;
}