//
// Created by salaheltenhy on 12/30/22.
//
#include <utility>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <string>
#include <thread>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <chrono>
#include <bits/stdc++.h>

using namespace std;
#ifndef ASSIGMENT_2_SERVER_H
#define ASSIGMENT_2_SERVER_H


class Server {
public:
    struct packet {
        uint16_t cksum;
        uint16_t len;
        uint32_t seqno;
        char data [500];
    };
    struct not_sent_packet{
        int seqno;
        chrono::time_point<chrono::system_clock> timer;
        bool done;
    };
    struct ack_packet {
        uint16_t cksum;
        uint16_t len;
        uint32_t ackno;
    };

    static void handle_client_request(int server_socket, int client_fd, sockaddr_in client_addr, char rec_buffer [] , int bufferSize);
    long checkFileExistence(string fileName);
    vector<string> readDataFromFile(string fileName);
    void sendTheData_HandleCongesion (int client_fd, struct sockaddr_in client_addr , vector<string> data);
    packet create_packet_data(string packetString, int seqNum);
    bool send_packet(int client_fd, struct sockaddr_in client_addr , string temp_packet_string, int seqNum);
    vector<string> readArgsFile();
    bool DropTheDatagram();
    bool CorruptTheDatagram();
    uint16_t get_data_checksum (string content, uint16_t len , uint32_t seqno);
    uint16_t get_ack_checksum (uint16_t len , uint32_t ackno);

    enum fsm_state {slow_start, congestion_avoidance, fast_recovery};
    int port, RandomSeedGen;
    double PLP;
    vector<not_sent_packet> not_sent_packets;
    vector<packet> sent_packets;

};


#endif //ASSIGMENT_2_SERVER_H
