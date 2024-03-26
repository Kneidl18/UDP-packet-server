//
// Created by Andreas Auer on 19/03/24.
//

#include "SocketHelper.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <random>
#include <cmath>
#include <arpa/inet.h>
#include <print>
#include <sys/fcntl.h>

#define MAX_DATA_LEN (65527.0 - sizeof(Packet))
#define PORT_NUMBER 8080
#define BUFFER_LEN 1024

std::random_device rd; // obtain a random number from hardware
std::mt19937 gen(rd()); // seed the generator
std::uniform_int_distribution<> distr(0, pow(2, 32)); // define the range


/**
 * set ip address and port for specific dst port/ip addr
 * @param dstIpAddr
 * @param port
 */
void SocketHelper::setIpSettings(uint8_t *dstIpAddr, size_t port){
    this->dstIpAddr = new sockaddr_in;
    this->dstIpAddr->sin_family = AF_INET;
    this->dstIpAddr->sin_port = htons(port);

    memcpy(&this->dstIpAddr->sin_addr, dstIpAddr, 4);
}

/**
 * fills the referenced packetHeader with the transmissionId and
 * sequence number
 * @param pHeader
 * @param tId
 * @param seqNum
 */
void SocketHelper::fillPacketHeader(PacketHeader *pHeader, uint16_t tId, uint32_t seqNum){
    pHeader->transmissionId = tId;
    pHeader->sequenceNumber = seqNum;
}

/**
 * fill the packet with the referenced packetHeader, data and datalen
 * @param packet
 * @param packetHeader
 * @param data
 * @param dataLen
 */
void SocketHelper::fillPacket (Packet *packet, PacketHeader *packetHeader, uint8_t *data, size_t dataLen) {
    memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));
    packet->data = data;
    packet->dataLen = dataLen;
}

void SocketHelper::fillStartPacket (StartPacket *packet, PacketHeader *packetHeader, size_t n, uint8_t *fileName,
                                    size_t nameLen){
    memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));
    // sequenceNumberMax = start + numberPackets + endPacket(1)
    packet->sequenceNumberMax = packetHeader->sequenceNumber + n + 1;
    packet->fileName = fileName;
    packet->nameLen = nameLen;
}

void SocketHelper::fillEndPacket (EndPacket *packet, PacketHeader *packetHeader, uint8_t *checksum){
    memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));

    for (int i = 0; i < 16; i++){
        packet->checksum[i] = checksum[i];
    }
}

/**
 * increase the sequence number by 1 (each packet has increasing sequence numbers
 * @param header
 */
void SocketHelper::increaseSequenceNumber(PacketHeader *header) {
    header->sequenceNumber++;
}

// TODO: i would suggest to make the checksum of the filename + data (all concatenated together)
/**
 * calculates the checksum of the data and the filename
 * @param endPackage packet struct to edit
 * @param data pointer to data
 * @param dataLen
 * @param fileName pointer to file name
 * @param fileNameLen
 */
void SocketHelper::calcChecksum (EndPacket *endPacket, uint8_t *data, size_t dataLen, uint8_t *fileName,
                                 size_t fileNameLen) {

    for (auto &i : endPacket->checksum) {
        i = 0;
    }
}

/**
 * pushes a package to the queue
 * @param packet pointer to the packet to be pushed
 * @return true if success, false if fail
 */
bool SocketHelper::pushToPacketQueue(packetVariant packet) {
    packetQueue.push(packet);
    return packetQueue.back() == packet;
}

/**
 * push an incoming msg to the incoming msg queue to then process it
 * @param buffer
 * @param len
 * @return true if success, false if fail
 */
bool SocketHelper::pushToIncommingQueue(char *buffer, ssize_t len){

}

// TODO: fileNameLen 1-256 byte!!
// TODO: 65527 Byte of data per package
/**
 *
 * @param data
 * @param dataLen
 * @param fileName
 * @param fileNameLen
 * @return
 */
bool SocketHelper::sendMsg (uint8_t *data, size_t dataLen, uint8_t *fileName, size_t fileNameLen){
    // calculate the number of packets necessary
    int n = ceil(dataLen / MAX_DATA_LEN);
    // std::cout << "number of packets for this message is " << n << std::endl;

    // create start packet
    auto *packetHeader = new PacketHeader;
    auto *startPacket = new StartPacket;
    fillPacketHeader(packetHeader, transmissionId++, distr(gen));
    fillStartPacket(startPacket, packetHeader, n, fileName, fileNameLen);
    pushToPacketQueue(startPacket);

    increaseSequenceNumber(packetHeader);

    // split data into n packets with max size of 65527 byte

    for (int i = 0; i < n; i++) {
        auto *packet = new Packet;
        memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));
        packet->data = data + (size_t) (i * MAX_DATA_LEN);
        packet->dataLen = std::min(dataLen, (size_t) MAX_DATA_LEN);

        pushToPacketQueue(packet);
        increaseSequenceNumber(packetHeader);
    }

    // create end packet
    auto *endPacket = new EndPacket;
    uint8_t checksum[16];
    fillEndPacket(endPacket, packetHeader, checksum);
    calcChecksum(endPacket, data, dataLen, fileName, fileNameLen);
    pushToPacketQueue(endPacket);

    msgSend = false;
    return true;
}

bool SocketHelper::msgOut(){
    return msgSend;
}

void SocketHelper::createSocketSend(int portNum, int *socket1) {
    // creating socket
    *socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    int con;

    // specifying address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNum);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // sending connection request
    if (dstIpAddr == nullptr)
        con = connect(*socket1, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    else {
        con = connect(*socket1, (struct sockaddr *) dstIpAddr, sizeof(sockaddr_in));
    }

    if (con < 0){
        std::cerr << "error connecting to socket" << std::endl;
        exit(1);
    }
}

void SocketHelper::createSocketRecv(int portNum, int *socket1) {
    // create a socket
    // socket(int domain, int type, int protocol)
    *socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    if (*socket1 < 0) {
        std::cerr << "error opening socket" << std::endl;
        exit(1);
    }

    // clear address structure
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portNum);

    if (dstIpAddr == nullptr) {
        if (bind(*socket1, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "ERROR on binding" << std::endl;
            exit(1);
        }
        std::cout << "listening on: " << inet_ntoa(serv_addr.sin_addr) << " port " << ntohs(serv_addr.sin_port) << std::endl;
    }
    else{
        if (bind (*socket1, (struct sockaddr *) dstIpAddr, sizeof(serv_addr)) < 0){
            std::cerr << "ERROR on binding" << std::endl;
            exit(1);
        }
        std::cout << "listening on: " << inet_ntoa(dstIpAddr->sin_addr) << " port " << ntohs(dstIpAddr->sin_port) << std::endl;
    }

    listen(*socket1, 10);

    fcntl(*socket1, F_SETFL, O_NONBLOCK);
}

void SocketHelper::runMaster(){
    int socket1;
    createSocketSend(PORT_NUMBER, &socket1);

    if (dstIpAddr != nullptr) {
        std::cout << "sending packet to: addr->" << inet_ntoa(dstIpAddr->sin_addr);
        std::cout << " port->" << ntohs(dstIpAddr->sin_port) << std::endl;
    }

    while (!packetQueue.empty()){
        auto elem = packetQueue.front();
        packetQueue.pop();
        std::visit([&socket1, this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Packet *>) {
                std::cout << *arg;

                // send the msg
                send(socket1, reinterpret_cast<void *> (arg), sizeof(Packet), 0);

                Packet *p = reinterpret_cast<Packet *> (arg);
                // delete p->data;
                delete p;
            }
            else if constexpr (std::is_same_v<T, StartPacket *>){
                std::cout << *arg;

                // send the msg
                send(socket1, reinterpret_cast<void *> (arg), sizeof(Packet), 0);

                StartPacket *p = reinterpret_cast<StartPacket *> (arg);
                delete p->fileName;
                delete p;
            }
            else if constexpr (std::is_same_v<T, EndPacket *>){
                std::cout << *arg;

                // send the msg
                send(socket1, reinterpret_cast<void *> (arg), sizeof(Packet), 0);

                EndPacket *p = reinterpret_cast<EndPacket *> (arg);
                delete p;
            }
            else
                std::cout << "unknown variance" << std::endl;
        }, elem);
    }

    msgSend = true;
}

void SocketHelper::runSlave(const bool *run){
    int socket1;
    createSocketRecv(PORT_NUMBER, &socket1);
    socketNum = socket1;

    while (*run) {
        char buffer[BUFFER_LEN];
        // size_t n = read(socket2, buffer, BUFFER_LEN-1);
        sockaddr cli_addr_sock{};
        socklen_t cli_len = sizeof(cli_addr_sock);
        ssize_t n = recvfrom(socket1, buffer, BUFFER_LEN - 1, 0, (struct sockaddr *) &cli_addr_sock, &cli_len);

        if (n < 1) {
            continue;
        }

        std::cout << "server: got connection from ";
        std::cout << inet_ntoa(reinterpret_cast<sockaddr_in *> (&cli_addr_sock)->sin_addr) << " port ";
        std::cout << ntohs(reinterpret_cast<sockaddr_in *> (&cli_addr_sock)->sin_port) << std::endl;

        std::cout << "incoming msg: ";
        for (int i = 0; i < n; i++) {
            std::cout << std::hex << (int) buffer[i];
        }
        std::cout << std::endl;

        // TODO: process incomming msg
        // msg comes in multiple goes
        // have to concatenate msges
        pushToIncommingQueue(buffer, n);
    }

    // closing socket
    close(socket1);
}

void SocketHelper::run(const bool *run, Config config){
    while (*run) {
        switch (config) {
            case MASTER:
                runMaster();
                break;
            case SLAVE:
                runSlave(run);
                break;
            default:
                std::cout << "server config unknown" << std::endl;
                exit(1);
        }
    }
}


