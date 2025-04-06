//Resources:
//https://beej.us/guide/bgnet/html

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <unordered_map>


using namespace std;

mutex fuelMutex;
unordered_map<long long, double> clientPrevFuelReading;
mutex clientMapMutex;


class AircraftData{

public:

    long long clientUniqueId;
    string transmittedData;
    double fuelData;
    string dateTime;
    string packetType;
    static double prevFuelReading;

    AircraftData() {

        clientUniqueId = 0;
        transmittedData = "";
        fuelData = 0.0;
        dateTime = "";
        packetType = "";
    }

    AircraftData(long long id, string data, double fuel, string date_time, string type) {

        clientUniqueId = id;
        transmittedData = data;
        fuelData = fuel;
        dateTime = date_time;
        packetType = type;
    }

     //############# SYS-010 - b ###############
     static AircraftData deserialize(string& data) {

         AircraftData ad;
         stringstream ss(data);
         string item;

         vector<string> items;

         while (getline(ss, item, '|')) {
             items.push_back(item);
         }

         if (items.size() >= 5) {

             ad.clientUniqueId = stoll(items[0]);
             ad.transmittedData = items[1];
             ad.fuelData = stod(items[2]);
             ad.dateTime = items[3];
             ad.packetType = items[4];
         }

         return ad;
     }

     //https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring/20262860#20262860
     std::string trim(std::string str)
     {
         size_t end = str.find_last_not_of(" \n\r\t");
         if (end != string::npos)
             str.resize(end + 1);

         size_t start = str.find_first_not_of(" \n\r\t");
         if (start != string::npos)
             str = str.substr(start);

         return str;
     }


     //############# SYS-010 - b ###############
     void parseData() {
         // Trim whitespace from the transmitted data
         string dataLine = trim(transmittedData);

         vector<string> items;
         stringstream ss(dataLine);
         string item;

         while (getline(ss, item, ',')) {
             item = trim(item);
             items.push_back(item);
         }

         // saving fuel data to a file named with ClientUniqueId
         string filename = to_string(clientUniqueId) + ".txt";
         ofstream outfile;
         double consumption = 0.0;

         // Parse values from items
         if (dataLine.find("FUEL TOTAL QUANTITY") != string::npos) {
             if (items.size() == 3) {
                 dateTime = items[1];
                 fuelData = stod(items[2]);
             }
         }
         else {
             if (items.size() == 2) {
                 dateTime = items[0];
                 fuelData = stod(items[1]);
             }
         }

         // Compute consumption using a per-client map
         {
             lock_guard<mutex> lock(clientMapMutex);

             auto it = clientPrevFuelReading.find(clientUniqueId);
             if (dataLine.find("FUEL TOTAL QUANTITY") != string::npos) {
                 consumption = 0.0;
                 clientPrevFuelReading[clientUniqueId] = fuelData;
             }
             else {
                 if (it == clientPrevFuelReading.end()) {
                     consumption = 0.0;
                     clientPrevFuelReading[clientUniqueId] = fuelData;
                 }
                 else {
                     consumption = it->second - fuelData;
                     it->second = fuelData;
                 }
             }
         }

         // Open file in appropriate mode
         if (dataLine.find("FUEL TOTAL QUANTITY") != string::npos) {
             outfile.open(filename, ios::out);
         }
         else {
             outfile.open(filename, ios::app);
         }

         //############# SYS-010 - c ###############
         if (outfile.is_open()) {
             if (fuelData != 0.000000) {
                 outfile << fixed << setprecision(6) << consumption << "\n";
                 outfile.close();
             }
         }
         else {
             cerr << "File could not be opened: " << filename << endl;
         }
     }


};


void calculateAverageFuelConsumption(int clientUniqueId) {

    string fileName = to_string(clientUniqueId) + ".txt";
    ifstream inputFile(fileName);

    if (!inputFile.is_open()) {

        cerr << "File could not be opened: " << fileName << endl;

    }

    double sum = 0.0;
    int count = 0;
    double value;

    while (inputFile >> value) {
        sum += value;
        count++;
    }

    inputFile.close();

    if (count > 0) {

        double average = sum / count;

        string fileName = to_string(clientUniqueId) + ".txt";
        ofstream outfile(fileName, ios::app);

        cout << "Average Fuel Consumption for flight " << clientUniqueId << " is: "<< fixed << setprecision(6) << average << endl;

        if (outfile.is_open()) {

            outfile << "Average:" << fixed << setprecision(6) << average << "\n";
            outfile.close();

        }
    }

}


void processPacketLines(string& buffer) {

    size_t pos = 0;

    while ((pos = buffer.find('\n')) != string::npos) {

        string packet = buffer.substr(0, pos);

        buffer.erase(0, pos + 1);

        if (packet.empty()) {
            continue;
        }

        AircraftData ad = AircraftData::deserialize(packet);

        if (ad.packetType == "EOF") {

            cout << "ClientID:" << ad.clientUniqueId << " | Data Received: \"" << ad.transmittedData << "\" | Packet Type: EOF" << endl;
            cout << "All data from Client has been received." << endl;
            calculateAverageFuelConsumption(ad.clientUniqueId);
            return;
        }

        else {
            cout << "ClientID:" << ad.clientUniqueId << " | Data Received: \"" << ad.transmittedData << "\" | Packet Type: Data" << endl;
        }

        //############# SYS-010 - b ###############
        ad.parseData();

    }
}


//handling client connection (receiving data)
void handleConnection(int clientSocket) {


    char recvBuffer[1024] = { 0 };
    string lineBuffer;

    while (true) {
        int bytesReceived = recv(clientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);

        if (bytesReceived <= 0) {
            break;
        }

        lineBuffer.append(recvBuffer, bytesReceived);

        processPacketLines(lineBuffer);
        
        if (lineBuffer.find("EOF") != string::npos) {
            cout << "All data from Client has been received." << endl;
            break;
        }
    }

    closesocket(clientSocket);

}

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

    //listening for unlimited number of connections
    listen(serverSocket, INT_MAX);

    //############# SYS-001 ###############
    while (true) {

        int clientSocket = accept(serverSocket, nullptr, nullptr);
        thread newThread(handleConnection, clientSocket);
        newThread.detach();
    }

    // to stop app from closing
    getchar();
   
    // closing socket
    closesocket(serverSocket);
    WSACleanup();
    return 0;

}