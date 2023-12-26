#define  _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <algorithm>
#include <vector>
#include<string>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;
#include <functional>

#pragma comment(lib, "ws2_32.lib")
struct FileInfo {
    std::vector<std::string> names;
    std::vector<int> sizes;
    std::vector<std::string> creationDates; // Added creation dates
};

std::vector<FileInfo> info;
std::vector<std::string> paths;
SOCKET clientSocket;
void listDirectoryContents(fs::path path);
std::function<void(fs::path)> FuncName = [](fs::path p) {listDirectoryContents(p); };


std::string serializeFileInfo(const FileInfo& fileInfo) {
    std::ostringstream oss;

    for (size_t i = 0; i < fileInfo.names.size(); ++i) {
        oss << fileInfo.names[i] << ' ' << fileInfo.sizes[i] << ' ' << fileInfo.creationDates[i];
        if (i < fileInfo.names.size() - 1) {
            oss << '|';
        }
    }

    return oss.str();
}

void sendFileInfo(const FileInfo& fileInfo) {
    std::string serializedData = serializeFileInfo(fileInfo);
    const char* data = serializedData.c_str();
    size_t dataSize = serializedData.size() + 1;

    // Allocate dynamic memory for the char array
    char* dataArray = new char[dataSize];
    //char* s = std::to_string(dataSize).c_str();
    // Copy the serialized data to the dynamic char array
    std::strcpy(dataArray, data);
    dataArray[dataSize - 1] = '\0';

    // Send the data
    //send(clientSocket, )
    send(clientSocket, dataArray, dataSize, 0);
    std::cout << "Message sent" << std::endl;
    std::cout << dataArray << std::endl;
    // Clean up the allocated memory
    delete[] dataArray;
}

FileInfo getFileInfo(const std::string& directory, const std::string& extension) {
    FileInfo fileInfo;

    return fileInfo;
}
void sendPaths(SOCKET clientSocket) {
    char buffer[1024];

    std::size_t offset = 0;
    std::size_t numStrings = paths.size();
    memcpy(buffer, &numStrings, sizeof(std::size_t));
    offset += sizeof(std::size_t);
    for (std::size_t i = 0; i < numStrings; ++i) {
        const char* name = paths[i].c_str();
        std::size_t nameLength = paths[i].size();
        memcpy(buffer + offset, &nameLength, sizeof(std::size_t));
        offset += sizeof(std::size_t);
        memcpy(buffer + offset, name, nameLength);
        offset += nameLength;
    }
    send(clientSocket, buffer, sizeof(buffer), 0);
}

void DataNewerClock(std::function<void(fs::path)> a) {
    int duration = 5000;
    std::thread([a, duration]() {
        while (true) {
            info.clear();
            paths.clear();
            a("D:\\RIP");

            auto ms = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration);
            std::this_thread::sleep_until(ms);
        }
        }).detach();

}

void listDirectoryContents(fs::path path) {
    try {
        if (fs::exists(path) && fs::is_directory(path)) {
            int i = 0;
            FileInfo file; for (const auto& entry : fs::directory_iterator(path)) {
                if (fs::is_directory(entry)) {
                    listDirectoryContents(entry.path());
                }
                else {
                    file.names.push_back(entry.path().filename().string());
                    file.sizes.push_back(fs::is_directory(entry) ? 0 : static_cast<int>(fs::file_size(entry)));
                    auto creationTime = fs::last_write_time(entry);
                    auto creationTimePoint = std::chrono::time_point_cast<std::chrono::system_clock::duration>(creationTime);

                    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(creationTimePoint.time_since_epoch());
                    std::time_t creationTimeT = seconds.count();

                    std::tm creationTimeStruct = *std::localtime(&creationTimeT);

                    std::ostringstream creationTimeString;
                    creationTimeString << std::put_time(&creationTimeStruct, "%d/%m/2023 %H:%M");
                    file.creationDates.push_back(creationTimeString.str());

                   
               /*    std::cout << "-------------" << std::endl
                        << entry.path().string() << std::endl
                        << file.names[file.names.size()-1] << std::endl
                        << file.sizes[file.sizes.size() - 1] << std::endl
                        << file.creationDates[file.creationDates.size() - 1] << std::endl;
                     */
                }
                i++;
            }

            info.push_back(file);
           
            paths.push_back(path.string());
        }
        else {
            std::cerr << "Invalid directory path.\n";
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return -1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << "\n";
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return -1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server listening on port 12345...\n";
    DataNewerClock(FuncName);
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return -1;
    }
    std::cout << "Client connected\n";
    while (true) {

        char buffer[256];
        recv(clientSocket, buffer, sizeof(buffer), 0);

        std::string request(buffer);
        std::cout << "Received request: " << request << "\n";
        if (request == "") {
            sendPaths(clientSocket);
        }
        else {
            size_t pos = request.find('|');
            std::string path = request.substr(0, pos);
            std::string extension = request.substr(pos + 1);

            int n = 0;
            for (int i = 0; i < paths.size(); i++) {
                if (paths[i] == path) { n = i; }
            }
            FileInfo tmp;
            for (int i = 0; i < info[n].names.size(); i++) {
                size_t lastIndex = info[n].names[i].find_last_of('.');
                std::string strtemp = info[n].names[i].substr(lastIndex + 1);
                if (strtemp == extension) {
                    try {
                        tmp.names.push_back(info[n].names[i]);
                        tmp.sizes.push_back(info[n].sizes[i]);
                        tmp.creationDates.push_back(info[n].creationDates[i]);
                        //std::cerr << GetLastError() << std::endl;
                    }
                    catch (const std::exception& ex) {
                        std::cout << ex.what() << std::endl;
                    }
                }
            }

            for (int i = 0; i < tmp.names.size(); i++) {
                std::cout << "-------------" << std::endl << \
                    tmp.names[i] << std::endl << \
                    tmp.sizes[i] << std::endl << \
                    tmp.creationDates[i] << std::endl;
            }
            sendFileInfo(tmp);
        }

    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}