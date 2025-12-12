#include "CEchoServer.h"


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
    printf("[Echo 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
}

void CEchoServer::OnClientLeave(SessionID s)
{
    //캐릭터 삭제같은걸 넣으면 될 것 같은데 에코에선 딱히 할게 없는 것으로 보임.
}

void CEchoServer::OnRecv(SessionID s, CPacket* pPacket)
{

}

void CEchoServer::OnError(int errorcode, char*)
{

}
