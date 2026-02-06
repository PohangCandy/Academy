#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>   // Windows
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    in_addr addr1{};
    in_addr addr2{};

    inet_pton(AF_INET, "192.168.0.10", &addr1);
    inet_pton(AF_INET, "10.0.0.5", &addr2);

    const char* ip1 = inet_ntoa(addr1);
    const char* ip2 = inet_ntoa(addr2);

    std::cout << "ip1 string : " << ip1 << std::endl;
    std::cout << "ip2 string : " << ip2 << std::endl;

    std::cout << "ip1 ptr    : " << static_cast<const void*>(ip1) << std::endl;
    std::cout << "ip2 ptr    : " << static_cast<const void*>(ip2) << std::endl;

    if (ip1 == ip2)
        std::cout << "ip1 == ip2 (same pointer)" << std::endl;
    else
        std::cout << "ip1 != ip2 (different pointer)" << std::endl;

    WSACleanup();
    return 0;
}
