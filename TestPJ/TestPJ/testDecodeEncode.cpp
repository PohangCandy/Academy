#include "stdafx.h"
#include "CPacketForMultiThread.h"
#include "CRingBuffer.h"
#include "CommonProtocol.h"

bool Decode(PacketHeader* pHeader, char* pc);

int main()
{
	char testSentence[] = ""
}

bool Decode(PacketHeader* pHeader, char* pc)
{
	unsigned int checksum = 0;
	unsigned char beforeparaP = 0;
	unsigned char afterparaP = 0;
	unsigned char encodeP = 0;


	int payLoadSize = pHeader->Len;

	int checkSumSize = sizeof(PacketHeader::CheckSum);
	char* pPacketChar = &pc[sizeof(PacketHeader) - checkSumSize];

	afterparaP = *pPacketChar ^ (encodeP + dfPACKET_KEY + 1);
	encodeP = *pPacketChar;

	*pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + 1);
	beforeparaP = afterparaP;

	for (int i = 0; i < payLoadSize; i++)
	{
		char* pPacketChar = &pc[sizeof(PacketHeader) + i];

		afterparaP = *pPacketChar ^ (encodeP + dfPACKET_KEY + (i + 2));
		encodeP = *pPacketChar;

		*pPacketChar = afterparaP ^ (beforeparaP + pHeader->RandKey + (i + 2));
		beforeparaP = afterparaP;

		checksum += *pPacketChar % 256;
		checksum %= 256;
	}



	//복호화가 제대로 이루어졌는지 확인
	if (pHeader->CheckSum != checksum)
	{
		printf("[Decode] checksum이 일치하지 않음. 복호화가 제대로 이루어지지 않음.\n");
		return false;
	}
	return true;
}