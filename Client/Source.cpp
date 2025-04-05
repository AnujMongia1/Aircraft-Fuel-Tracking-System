//Resources:
//https://beej.us/guide/bgnet/html

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>
#include <fstream>
#include <iomanip>

using namespace std;

class AircraftData {

public:

    int clientUniqueId;
    string transmittedData;
    double fuelData;
    string dateTime;
    string packetType;

    AircraftData() {

        clientUniqueId = 0;
        transmittedData = "";
        fuelData = 0.0;
        dateTime = "";
        packetType = "";
    }

    AircraftData(int id, string data, double fuel, string date_time, string type) {

        clientUniqueId = id;
        transmittedData = data;
        fuelData = fuel;
        dateTime = date_time;
        packetType = type;

    }

    string serializePacket() {

        stringstream ss;

        ss << fixed << setprecision(6) << clientUniqueId << "|" << transmittedData << "|" << fuelData << "|" << dateTime << "|" << packetType;
        return ss.str();

    }

    //############# SYS-050 ###############
    static int generateUniqueClientId() {

        srand(static_cast<unsigned>(time(0)));
        return 10000 + rand() % 90000;

    }

};


int main()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }

    // creating a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0) {

        cout << "Failed to create a socket." << endl;
        WSACleanup();
        return -1;

    }

    // specifying server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    //making connection to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {

        cout << "Connection to server failed." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return -1;

    }

    //############# SYS-050 ###############
    int uniqueId = AircraftData::generateUniqueClientId();
    cout << "Client Unique ID: " << uniqueId << endl;

    // sending data
    ifstream inputFile("Telem_2023_3_12 14_56_40.txt");
    string line;
    //############# SYS-040 - a ###############
    while (getline(inputFile, line)) {

        AircraftData data(uniqueId, line, 0.0, "0", "aircraft_data");
        //############# SYS-040 - b ###############
        string dataPacketToSend = data.serializePacket() + "\n"; //changed to add newline
        //############# SYS-040 - c ###############
        send(clientSocket, dataPacketToSend.c_str(), dataPacketToSend.length(), 0);
        cout << "ClientID:" << uniqueId << " | Data Sent: \"" << line << "\" | Packet Type: Data" << endl;

    }

    AircraftData lastPacket(uniqueId, "EOF", 0.0, "0", "EOF");

    //############# SYS-040 - b ###############
    string lastPacketToSend = lastPacket.serializePacket() + "\n"; //changed to add newline
    send(clientSocket, lastPacketToSend.c_str(), lastPacketToSend.length(), 0);
    cout << "ClientID:" << uniqueId << " | Data Sent: \"EOF\" | Packet Type: EOF" << endl;
    
    inputFile.close();
    cout << "All data has been transferred!, you may close the application!";
    getchar();
    // closing socket
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
