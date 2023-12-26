#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <cstring>
#include <sstream>
#include <string>
#include<ctime>

#pragma comment(lib, "ws2_32.lib")
using namespace std;
vector<string>paths;
struct FileInfo {
    std::vector<std::string> names;
    std::vector<int> sizes;
    std::vector<std::string> creationDates;
};
FileInfo files;
SOCKET clientSocket;

void deserializeFileInfo(const std::string& serializedData) {
    files.names.clear();
    files.creationDates.clear();
    files.sizes.clear();

    istringstream iss(serializedData);
    string entry;

    while (std::getline(iss, entry, '|')) {
        istringstream entryStream(entry);
        string name;
        int size;
        string date;
        string date2;
        entryStream >> name >> size >> date >> date2;
        date += " " + date2;
        files.names.push_back(name);
        files.sizes.push_back(size);
        files.creationDates.push_back(date);
    }
}

std::vector<std::string> deserializeStringVector(const char* buffer) {
    std::vector<std::string> result;
    std::size_t numStrings;
    memcpy(&numStrings, buffer, sizeof(std::size_t));
    std::size_t offset = sizeof(std::size_t);
    for (std::size_t i = 0; i < numStrings; ++i) {
        std::size_t strSize;
        memcpy(&strSize, buffer + offset, sizeof(std::size_t));
        offset += sizeof(std::size_t);
        std::string str(buffer + offset, strSize);
        offset += strSize;
        result.push_back(str);
    }

    return result;
}

void receiveFileInfo() {
    char buffer[1024];  // Adjust the buffer size accordingly
   // int bytesReceived = 0;
   // int totalBytesExpected = sizeof(buffer);

    // Loop until all expected bytes are received
  /*  while (bytesReceived < 1024) {
        int result = recv(clientSocket, buffer + bytesReceived, totalBytesExpected - bytesReceived, 0);
        if (result == -1) {
            std::cerr << "Error receiving data from socket\n";
            break;
        }
        else if (result == 0) {
            std::cerr << "Connection closed by the peer\n";
            break;
        }

        bytesReceived += result;
    }*/
    recv(clientSocket, buffer, sizeof(buffer), 0);

    // Process the received data (deserializeFileInfo)
    std::string receivedData(buffer);
    deserializeFileInfo(receivedData);
   // std::cout << receivedData << std::endl;  // Debugging: Output received data
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return -1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << "\n";
        WSACleanup();
        return -1;
    }
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    serverAddr.sin_port = htons(12345);
    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }
    int c = -1;
    while (c != 2) {
        cout << "1 - choose directory" << endl << "2 - quit" << endl;
        cin >> c;
        int f = -1;
        string a, g;
        FileInfo fileInfo;
        switch (c) {
        case 1:
            send(clientSocket, "", 1, 0);
            char buffer[1024];
            recv(clientSocket, buffer, sizeof(buffer), 0);
            paths.clear();
            paths = deserializeStringVector(buffer);
            f = -1;
            while (f == -1 || f == 0) {
                for (int i = 0; i < paths.size(); i++) {
                    cout << i + 1 << " - " << paths[i] << endl;
                }
                cin >> f;
            }
            cout << "Enter extension" << endl;
            cin >> a;
            g = paths[f - 1] + "|" + a;
            send(clientSocket, g.c_str(), g.size() + 1, 0);
            receiveFileInfo();
            if (files.names.size() == 0) {
                std::cout << "Directory is empty" << endl;
            }
            else {
                for (int i = 0; i < files.names.size(); i++) {
                    cout << "---------------------" << endl;
                    cout << "Name: " << files.names[i] << endl\
                        << "Size: " << files.sizes[i] << endl \
                        << "Date:" << files.creationDates[i] << endl;
                }
                cout << "---------------------" << endl;
            }
            break;
        case 2:  closesocket(clientSocket);
            WSACleanup();
            exit(0);
            break;
        }
    }
    return 0;
}