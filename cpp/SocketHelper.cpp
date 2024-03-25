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

#define MAX_DATA_LEN (65527.0 - sizeof(Packet))
#define PORT_NUMBER 10000

std::random_device rd; // obtain a random number from hardware
std::mt19937 gen(rd()); // seed the generator
std::uniform_int_distribution<> distr(0, pow(2, 32)); // define the range

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
    std::cout << "number of packets for this message is " << n << std::endl;

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
    pushToPacketQueue(endPacket);

    msgSend = false;
    return true;
}

bool SocketHelper::msgOut(){
    return msgSend;
}

void SocketHelper::createSocket(int portNum, int *socket1, int *socket2) {
    // create a socket
    // socket(int domain, int type, int protocol)
    *socket1 = socket(AF_INET, SOCK_STREAM, 0);
    if (*socket1 < 0) {
        std::cerr << "error opening socket";
        exit(1);
    }

    // clear address structure
    bzero((char *) &serv_addr, sizeof(serv_addr));

    /* setup the host_addr structure for use in bind call */
    // server byte order
    serv_addr.sin_family = AF_INET;

    // automatically be filled with current host's IP address
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // convert short integer value for port must be converted into network byte order
    serv_addr.sin_port = htons(portNum);

    // bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
    // bind() passes file descriptor, the address structure,
    // and the length of the address structure
    // This bind() call will bind  the socket to the current IP address on port, portno
    if (bind(*socket1, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR on binding";
        exit(1);
    }

    // This listen() call tells the socket to listen to the incoming connections.
    // The listen() function places all incoming connection into a backlog queue
    // until accept() call accepts the connection.
    // Here, we set the maximum size for the backlog queue to 5.
    listen(*socket1,5);

    // The accept() call actually accepts an incoming connection
    size_t clilen = sizeof(cli_addr);

    // This accept() function will write the connecting client's address info
    // into the the address structure and the size of that structure is clilen.
    // The accept() returns a new socket file descriptor for the accepted connection.
    // So, the original socket file descriptor can continue to be used
    // for accepting new connections while the new socker file descriptor is used for
    // communicating with the connected client.
    *socket2 = accept(*socket1,
                           (struct sockaddr *) &cli_addr, reinterpret_cast<socklen_t *>(&clilen));
    if (*socket2 < 0) {
        std::cerr << "ERROR on accept";
        exit(1);
    }

    std::cout << "server: got connection from " << inet_ntoa(cli_addr.sin_addr) << " port " << ntohs(cli_addr.sin_port);
    std::cout << std::endl;
}

void SocketHelper::runMaster(){
    int socket1, socket2;
    createSocket(PORT_NUMBER, &socket1, &socket2);

    while (!packetQueue.empty()){
        auto elem = packetQueue.front();
        packetQueue.pop();
        std::visit([&socket1, &socket2](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Packet *>) {
                std::cout << *arg;
                send(socket2, reinterpret_cast<void *> (arg), sizeof(Packet), 0);
            }
            else if constexpr (std::is_same_v<T, StartPacket *>){
                std::cout << *arg;
                send(socket2, reinterpret_cast<void *> (arg), sizeof(StartPacket), 0);
            }
            else if constexpr (std::is_same_v<T, EndPacket *>){
                std::cout << *arg;
                send(socket2, reinterpret_cast<void *> (arg), sizeof(EndPacket), 0);
            }
            else
                std::cout << "unknown variance" << std::endl;
        }, elem);

        // TODO: fix possible memory leak with packetHeader and data
        // delete elem->packetHeader;
        // delete elem->data;
        // delete elem;
    }

    msgSend = true;
}

void SocketHelper::runSlave(){

}

void SocketHelper::run(const bool *run){
    while (*run) {
        switch (CONFIGURATION) {
            case MASTER:
                runMaster();
                std::cout << "sending msg" << std::endl;
                break;
            case SLAVE:
                runSlave();
                break;
            default:
                std::cout << "server config unknown" << std::endl;
                exit(1);
        }
    }
}


