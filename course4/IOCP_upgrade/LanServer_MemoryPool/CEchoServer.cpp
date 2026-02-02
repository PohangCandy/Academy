#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "CEchoServer.h"
#include "CPacketForMultiThread.h"

#define SERVERPORT (6000)
#define BUFSIZE (1024 * 1024)
#define MSG_SIZE (8)

//------------------------------------
//메시지 프로토콜
// 헤더 2Byte (길이)
// 데이터 8Byte(에코)
//------------------------------------
struct Msg {
    short header = 0;
    char payload[MSG_SIZE] = {};
};

//----------------------
// 특정 대역의 IP를 차단하고 싶을때, 들어온 IP와 일치하면 block
// 특정 대역의 IP만 허용시키고 싶을때, 들어온 IP와 일치하면 true
//----------------------
bool CEchoServer::OnConnectionRequest(std::string IP, int Port)
{
    //if (_serverMode == QA)
    //{

    //}
    //else
    //{

    //}
    return true;
}

void CEchoServer::OnClientJoin(SOCKADDR_IN clientaddr, SessionID s)
{
    //printf("[Echo 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
}

void CEchoServer::OnClientLeave(SessionID s)
{
    //캐릭터 삭제같은걸 넣으면 될 것 같은데 에코에선 딱히 할게 없는 것으로 보임.
}

void CEchoServer::OnRecv(SessionID sessionID, CPacket* pPacket)
{
	//네트워크에서 넘긴 길이만큼 추출
	//Msg recvMsg; 굳이 동적할당 해야할까? 어차피 송신 링버퍼에 복사가 되었다면 문제 없는게 정상임.
	Msg* recvMsg = new Msg;

	//이런식으로 GetData가 아니라 패킷에서 긁어서 넣어줘야 할 것 같은데..
	int len = pPacket->GetDataSize();
	int ret = pPacket->GetData(recvMsg->payload, len);
	if (ret == 0)
	{
		while (1)
		{
			printf("[Contents] 패킷에 남은 메시지 없는데 추출 시도함.\n");
		}
	}
	else if (len != sizeof(recvMsg->payload))
	{
		while (1)
		{
			//악의적인 클라로 간주하고 끊는게 맞음. Disconnect하면 될 듯
			printf("[Contents] 메시지와 패킷의 양식이 다름, 메시지  : %d , 패킷 : %d \n", sizeof(recvMsg->payload), len);
		}
	}
	else if(len != ret)
	{
		while (1)
		{
			//악의적인 클라로 간주하고 끊는게 맞음. Disconnect하면 될 듯
			printf("[Contents] 패킷에서 추출한 크기가 예상과 다름. 요청  : %d , 실제 : %d \n", len, ret);
		}
	}

	//printf("[Contents] : 수신 메시지 내용 %lld\n", (long long)recvMsg->payload);

	//추출한 메시지에 헤더를 붙여서 SendPakcet
	recvMsg->header = sizeof(recvMsg->payload);

	CPacket* pSendPacket = CPacket::Alloc();
	pSendPacket->operator<<(recvMsg->header);
	pSendPacket->PutData(recvMsg->payload, recvMsg->header);
	int sendret = SendPacket(sessionID, pSendPacket);

	//세션이 이제 여기서 삭제되는 경우는 없음.
	if (sendret == 0)
	{
		while (1)
		{
			printf("[Contents] 네트워크 송신 버퍼가 꽉 참\n");
		}
	}

	delete recvMsg;
}

void CEchoServer::OnError(int errorcode, char*)
{

}
