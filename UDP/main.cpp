#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <ws2tcpip.h>
#include <thread>
#include <csignal>
#include <sstream>
#include <unordered_map>
#pragma comment(lib, "Ws2_32.lib")

using std::unordered_map;
using std::cerr;
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::this_thread::sleep_for;
using std::chrono::seconds;
using std::stringstream;

int getOwnIP(uint8_t* buf)
{
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
        cerr << "Error " << WSAGetLastError() <<
            " when getting local host name." << endl;
        return 1;
    }

    struct addrinfo hints = { 0 }, * res = nullptr, * ptr = nullptr;
    hints.ai_family = AF_INET;  // IPv4

    if (getaddrinfo(ac, nullptr, &hints, &res) != 0) {
        cerr << "Yow! Bad host lookup." << endl;
        return 1;
    }

    char ipstr[INET_ADDRSTRLEN];

    for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
        struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
        inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipstr, sizeof(ipstr));
        cout << "Address: " << ipstr << endl;
        unsigned char* bytes = (unsigned char*)&sockaddr_ipv4->sin_addr.s_addr;
        cout << "Bytes: ";
        for (int i = 0; i < 4; i++) {
            buf[i] = bytes[i];
            printf("%02x", bytes[i]);
            if (i < 3) {
                cout << ".";
            }
        }
        cout << "\n";
    }

    freeaddrinfo(res);  // Free the linked list

    return 0;
}

unordered_map<string, string> students;
void buffer2Info(uint8_t* buf) {
    cout << "Student: ";
    stringstream ss;
    for (int i = 0; i < 4; i++) {
        ss << (int)buf[18 + i];
        if (i < 3) {
            ss << '.';
        }
    }
    string ipAddress = ss.str();
    cout << ipAddress;
    


    ss.str(string());

    ss << " | ";
    for (int i = 0; i < buf[26]; i++) {
        ss << buf[30 + i];
    }
    ss << " | ";
    int i = 0;
    while (true) {
        ss << buf[98 + i];
        i++;
        if (buf[98 + i] == 0) {
            break;
        }
    }
    string studentInfo = ss.str();
    
    if (students[ipAddress] == studentInfo) {
        std::cout << " --> duplicate student!\n";
    }
    else {
        students[ipAddress] = studentInfo;
        cout << studentInfo;
        cout << "\n";
    }
    
}

SOCKET s = INVALID_SOCKET;
void signalHandler(int signalCode) {
    closesocket(s);
    exit(signalCode);
}

int main() {
    signal(SIGINT, signalHandler);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        return 1;
    }


    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }
    bool broadcast = true;
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) {
        cerr << "Failed to set socket option: " << WSAGetLastError() << endl;
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
        cerr << "Broadcast send failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }
    cout << "\n";
    uint8_t* messageBuffer = new uint8_t[512];
    memset(messageBuffer, 0, sizeof(messageBuffer));
    sockaddr from;
    int fromlen = 16;
    memset(&from, 0, sizeof(from));
    fromlen = 16;

    int timeout = 1000;
    
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    while (true) {
        int sendResult = sendto(s, reinterpret_cast<const char*>(message), 22, 0, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
        if (sendResult == SOCKET_ERROR) {
            cerr << "Broadcast send failed: " << WSAGetLastError() << endl;
            closesocket(s);
            WSACleanup();
            return 1;
        }
        memset(messageBuffer, 0, sizeof(messageBuffer));
        int recvLength = recvfrom(s, reinterpret_cast<char*>(messageBuffer), 0xFF, 0, &from, &fromlen);
        if (recvLength == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAETIMEDOUT) {
                cerr << "recvfrom timed out." << "\n";
            }
            else {
                cerr << "recvfrom failed: " << WSAGetLastError() << "\n";
            }
        }
        else {
            buffer2Info(messageBuffer);
            sleep_for(seconds(1));
        }
    }
    
    closesocket(s);
    WSACleanup();
    return 0;
}
using std::string;
