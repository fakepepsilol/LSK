#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

int getOwnIP(uint8_t* buf)
{
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
        std::cerr << "Error " << WSAGetLastError() <<
            " when getting local host name." << std::endl;
        return 1;
    }

    struct addrinfo hints = { 0 }, * res = nullptr, * ptr = nullptr;
    hints.ai_family = AF_INET;  // IPv4

    if (getaddrinfo(ac, nullptr, &hints, &res) != 0) {
        std::cerr << "Yow! Bad host lookup." << std::endl;
        return 1;
    }

    char ipstr[INET_ADDRSTRLEN];

    for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
        struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
        inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipstr, sizeof(ipstr));
        std::cout << "Address: " << ipstr << std::endl;
        unsigned char* bytes = (unsigned char*)&sockaddr_ipv4->sin_addr.s_addr;
        std::cout << "Bytes: ";
        for (int i = 0; i < 4; i++) {
            buf[i] = bytes[i];
            std::printf("%02x", bytes[i]);
            if (i < 3) {
                std::cout << ".";
            }
        }
        std::cout << "\n";
    }

    freeaddrinfo(res);  // Free the linked list

    return 0;
}

void buffer2Info(uint8_t* buf) {
    std::cout << "Student: ";
    for (int i = 0; i < 4; i++) {
        std::cout << (int)buf[18 + i];
        if (i < 3) {
            std::cout << '.';
        }
    }
    std::cout << " | ";
    for (int i = 0; i < buf[26]; i++) {
        std::cout << buf[30 + i];
    }
    std::cout << " | ";
    int i = 0;
    while (true) {
        std::cout << buf[98 + i];
        i++;
        if (buf[98 + i] == 0) {
            break;
        }
    }
    std::cout << "\n";
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }


    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    bool broadcast = true;
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) {
        std::cerr << "Failed to set socket option: " << WSAGetLastError() << std::endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(796);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    uint8_t message[] = { 0x05, 0x02, 0x01, 0x00, 0xda, 0xd1, 0x16, 0x00, 0x00, 0x00, /**/0x00, 0x00, 0x00, 0x00/**/, 0x00, 0x00, 0xda, 0xd4, 0x00, 0x00, 0xda, 0xd4 };
    getOwnIP(message + 10);
    int sendResult = sendto(s, reinterpret_cast<const char*>(message), 22, 0, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "Broadcast send failed: " << WSAGetLastError() << std::endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }
    uint8_t* messageBuffer = new uint8_t[512];
    memset(messageBuffer, 0, sizeof(messageBuffer));
    sockaddr from;
    int fromlen = 16;
    memset(&from, 0, sizeof(from));
    fromlen = 16;
    int recvLength = recvfrom(s, reinterpret_cast<char*>(messageBuffer), 0xFF, 0, &from, &fromlen);
    std::cout << "\n";
    buffer2Info(messageBuffer);
    closesocket(s);
    WSACleanup();
    return 0;
}
