//
// Created by salaheltenhy on 12/30/22.
//

#include "Client.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string>
#include <bits/stdc++.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

using namespace std;

static const int MAXIMUM_SEGMENT_SIZE = 508;

/* Data only packets */
struct packet {
    uint16_t checkSum;
    uint16_t len;
    uint32_t seqNo;
    char data [500];
};

/* Ack-only packets are only 8 bytes */
struct ackPacket {
    uint16_t checkSum;
    uint16_t len;
    uint32_t ackNo;
};

/**
 * creates a packet for the client that contains the name of the file
 * @param fileName: the name of the file
 * @return p: the packet of the client
 */
packet createPacket(const string& fileName) {
    struct packet p{};
    strcpy(p.data, fileName.c_str());
    p.seqNo = 0;
    p.checkSum = 0;
    p.len = fileName.length() + sizeof(p.checkSum) + sizeof(p.len) + sizeof(p.seqNo);
    return p;
}

/**
 * gets the acknowledge check sum
 * @param len: the length of the acknowledge
 * @param ackNo: the acknowledge number
 * @return
 */
uint16_t getAckChecksum(uint16_t len, uint32_t ackNo) {
    uint32_t sum = len + ackNo;
    while(sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    auto checkSum = (uint16_t) (~sum);
    return checkSum;
}

/**
 * gets the data check sum
 * @param content: the content of the data
 * @param len: the length of the content
 * @param seqNo: the sequence number of the acknowledge
 * @return checkSum: the check sum of the data
 */
uint16_t getDataChecksum(const string& content, uint16_t len, uint32_t seqNo) {
    uint32_t sum = len + seqNo;
    int n = content.length();
    char arr[n + 1];
    strcpy(arr, content.c_str());
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    auto checkSum = (uint16_t) (~sum);
    return checkSum;
}

/**
 * sends the acknowledge from the client to the server
 * @param clientSocket: the socket of the client
 * @param serverAddress: the address of the server
 * @param seqNum: the sequence number of the acknowledge
 */
void sendAck(int clientSocket, struct sockaddr_in serverAddress, int seqNum) {
    struct ackPacket ack{};
    ack.ackNo = seqNum;
    ack.len = sizeof(ack);
    ack.checkSum = getAckChecksum(ack.len, ack.ackNo);
    char* ackBuf = new char[MAXIMUM_SEGMENT_SIZE];
    memset(ackBuf, 0, MAXIMUM_SEGMENT_SIZE);
    memcpy(ackBuf, &ack, sizeof(ack));
    ssize_t bytesSent = sendto(clientSocket, ackBuf, MAXIMUM_SEGMENT_SIZE, 0,
                               (struct sockaddr *)&serverAddress, sizeof(struct sockaddr));
    if (bytesSent == -1) {
        perror("error in sending the ack ! ");
        exit(1);
    }
    cout << "Ack for packet seq. Num " << seqNum << " is sent." << endl << flush;
}

/**
 * reads the input file of commands
 * @return commands: vector {IP_Address, port, fileName}
 */
vector<string> readCommand() {
    string fileName = "command.txt";
    vector<string> commands;
    string line;
    ifstream f;
    f.open(fileName);
    while(getline(f, line)) {
        commands.push_back(line);
    }
    return commands;
}

/**
 * writes the file named fileName with the content
 * @param fileName: the name of the file
 * @param content: the contents of the file
 */
void writeFile (const string& fileName, const string& content) {
    ofstream f_stream(fileName.c_str());
    f_stream.write(content.c_str(), content.length());
}

int main() {
    // reads the input commands
    vector<string> commands = readCommand();
    string IP_Address = commands[0];
    int port = stoi(commands[1]);
    string fileName = commands[2];
    struct sockaddr_in server_address{};
    int clientSocket;
    memset(&clientSocket, 0, sizeof(clientSocket));
    if ((clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Error in creating the Socket!");
        exit(1);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    cout << "File Name is: " << fileName << "\nThe length of the Name : " << fileName.size() << "\n" << flush;

    // creates a packet for the client
    struct packet fileNamePacket = createPacket(fileName);
    char* buffer = new char[MAXIMUM_SEGMENT_SIZE ];
    memset(buffer, 0, MAXIMUM_SEGMENT_SIZE );
    memcpy(buffer, &fileNamePacket, sizeof(fileNamePacket));
    ssize_t bytesSent = sendto(clientSocket, buffer, MAXIMUM_SEGMENT_SIZE ,
                               0, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
    if (bytesSent == -1) {
        perror("Error in sending the file name!");
        exit(1);
    }
    else {
        cout << "Client Sent The file Name Successfully\n" << flush;
    }
    // receive the ACK
    char recBuffer[MAXIMUM_SEGMENT_SIZE];
    socklen_t addrLen = sizeof(server_address);
    ssize_t receivedBytes = recvfrom(clientSocket, recBuffer, MAXIMUM_SEGMENT_SIZE,
                                     0, (struct sockaddr*)&server_address, &addrLen);
    if (receivedBytes < 0) {
        perror("Error in receiving file name ack!");
        exit(1);
    }
    auto* ackPacket = (struct ackPacket*) recBuffer;
    cout << "Number of packets " << ackPacket->len << "\n";
    long numberOfPackets = ackPacket->len;
    string fileContents[numberOfPackets];
    bool received[numberOfPackets] = {false};
    int i = 1;
    int expectedSeqNum = 0;
    while (i <= numberOfPackets) {
        memset(recBuffer, 0, MAXIMUM_SEGMENT_SIZE);
        ssize_t bytesReceived = recvfrom(clientSocket, recBuffer, MAXIMUM_SEGMENT_SIZE,
                                         0, (struct sockaddr*)&server_address, &addrLen);
        if (bytesReceived == -1) {
            perror("Error receiving data packet.");
            break;
        }
        auto* dataPacket = (struct packet*) recBuffer;
        cout <<"packet " << i << " received\n" << flush;
        cout << "Sequence Number: " << dataPacket->seqNo << "\n" << flush;
        int len = dataPacket->len;
        for (int j = 0 ; j < len ; j++) {
            fileContents[dataPacket->seqNo] += dataPacket->data[j];
        }
        if (getDataChecksum(fileContents[dataPacket->seqNo], dataPacket->len, dataPacket->seqNo) != dataPacket->checkSum){
            cout << "corrupted data packet !\n" << flush;
        }
        sendAck(clientSocket, server_address, dataPacket->seqNo);
        i++;
    }
    string content = "";
    for (i = 0; i < numberOfPackets ; i++) {
        content += fileContents[i];
    }
    writeFile(fileName, content);
    cout << "File is received successfully.\n" << flush;
    return 0;
}
