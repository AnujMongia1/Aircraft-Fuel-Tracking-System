//Resources:
//https://beej.us/guide/bgnet/html
//https://www.geeksforgeeks.org/socket-programming-in-cpp/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include<iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

int main() {

    WSADATA wsaData;


    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }

    //creating socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    //binding socket to address
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    //listening for unlimited number of connections (SYS-001)
    listen(serverSocket, INT_MAX);

    int clientSocket = accept(serverSocket, nullptr, nullptr);

    //printing message from clients
    char buffer[1024] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);
    cout << "Mesasage from Client: " << buffer << endl;

    // to stop app from closing
    getchar();
   
    // closing socket
    closesocket(serverSocket);
    WSACleanup();

}