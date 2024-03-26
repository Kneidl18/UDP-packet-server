//
// Created by Andreas Auer on 19/03/24.
//

#include "SocketHelper.h"
#include <chrono>
#include <iomanip>
#include <random>
#include <cmath>
#include <arpa/inet.h>
#include <print>
#include <sys/fcntl.h>
#include <fstream>
#include <utility>
#include "md5.h"

#define MAX_DATA_LEN 5000.0
#define PORT_NUMBER 8080

std::random_device rd; // obtain a random number from hardware
std::mt19937 gen(rd()); // seed the generator
std::uniform_int_distribution<> distr(0, (int) pow(2, 32)); // define the range


/**
 * set ip address and port for specific dst port/ip addr
 * @param dstIp
 * @param port
 */
void SocketHelper::setIpSettings(uint8_t *dstIp, size_t port){
    this->dstIpAddr = new sockaddr_in;
    this->dstIpAddr->sin_family = AF_INET;
    this->dstIpAddr->sin_port = htons(port);

    memcpy(&this->dstIpAddr->sin_addr, dstIp, 4);
}

void SocketHelper::setOutputDirPath (std::string outDir){
    this->outputDir = std::move(outDir);
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
 * fill the packet with the referenced packetHeader, data and dataLen
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
    packet->nameLen = std::min(nameLen, (size_t) 256);
}

void SocketHelper::fillEndPacket (EndPacket *packet, PacketHeader *packetHeader, const uint8_t *checksum){
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

    // std::string tmp = std::string(data, dataLen);
    md5::md5_t md5_o;

    md5_o.process(fileName, fileNameLen);
    md5_o.process(data, dataLen);

    md5_o.finish(endPacket->checksum);
}

bool SocketHelper::savePacketsToFile(StartPacket *startPacket, Packet *packets) {
    size_t n = startPacket->sequenceNumberMax - startPacket->packetHeader.sequenceNumber - 1;

    // create output file dir from received filename and stored file path
    std::string outFilePath = outputDir;
    outFilePath += std::string(reinterpret_cast<char *> (startPacket->fileName));

    if (!std::filesystem::exists(outputDir.c_str())) {
        std::filesystem::create_directory(outputDir.c_str());
    }

    // don't worry about seg fault because of fileName being an uint8_t
    // as it points to a 65000 byte char array there is always a \0 at the end
    std::ofstream output_file(outFilePath);
    std::ostream_iterator<std::string> output_iterator(output_file, "\n");

    if (!output_file.is_open()){
        // file couldn't be opened...
        std::cerr << "couldn't open file to store incoming message" << std::endl;
        return false;
    }
    for (int i = 0; i < n; i++){
        for (int j = 0; j < packets[i].dataLen; j++)
            output_file << packets[i].data[j];
    }

    return true;
}

/**
 * sort the packets[] using insertion sort (should be pretty fast
 * as the packets should be in correct order)
 * @param packets
 * @param n
 * @return
 */
void SocketHelper::sortPackets (Packet *packets, size_t n){
    Packet key;
    for (int i = 1; i < n; i++){
        int j = i - 1;
        key = packets[i];
        while (j >= 0 && packets[j].packetHeader.sequenceNumber > key.packetHeader.sequenceNumber) {
            packets[j+1] = packets[j];
            j--;
        }
        packets[j + 1] = key;
    }
}

/**
 * checks the correctness of the received packets (also checksum)
 * @param startPacket
 * @param packets array of all packets (amount should be seqNumMax - seqNumStart
 * @param endPacket
 * @return true if all packets are correct, false if there is a problem with the packets (also checks checksum)
 */
bool SocketHelper::checkCorrectnessOfPackets (StartPacket *startPacket, Packet *packets, EndPacket *endPacket){
    uint32_t sequenceNumberEnd = startPacket->sequenceNumberMax;
    uint32_t sequenceNumberStart = startPacket->packetHeader.sequenceNumber;

    for (int i = 0; i < sequenceNumberEnd - sequenceNumberStart - 1; i++) {
        if (packets[i].packetHeader.sequenceNumber != sequenceNumberStart + i + 1){
            // sequence number doesn't seem to match nth element
            std::cerr << "packets are missing" << std::endl;
            // exit(1);
            return false;
        }
    }

    if (endPacket->packetHeader.sequenceNumber != sequenceNumberEnd){
        // sequence number of the endPacket doesn't match sequenceNumberMax from Start Header
        std::cerr << "sequence number of end packet doesnt' match sequence number written in startPacket" << std::endl;
        // exit(1);
        return false;
    }

    EndPacket dummy;
    uint8_t data[(uint32_t) MAX_DATA_LEN * (sequenceNumberEnd - sequenceNumberStart - 1)];
    size_t dataPointer = 0;

    for (int i = 0; i < sequenceNumberEnd - sequenceNumberStart - 1; i++){
        memcpy(data + dataPointer, packets[i].data, packets[i].dataLen);
        dataPointer += packets[i].dataLen;
    }

    calcChecksum(&dummy, data, dataPointer, startPacket->fileName, startPacket->nameLen);

    for (int i = 0; i < 16; i++){
        if (dummy.checksum[i] != endPacket->checksum[i]){
            std::cerr << "error in checksum" << std::endl;
            return false;
        }
    }

    return true;
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
bool SocketHelper::pushToIncomingQueue(char *buffer, ssize_t len){
    auto *p = new IncomingPacket;

    memcpy(p->buffer, buffer, len);
    p->len = len;

    incomingPacketList.push_back(p);
    return true;
}

/**
 * concatenate incoming messages to readable packages
 * each incoming packet is one packet of the transmission
 * convert them one after another to a packet and search
 * for the final packet
 */
void SocketHelper::processIncomingMsg(){
    // load the first packet; calculate the amount of packets from the start packet
    IncomingPacket *first = incomingPacketList.front();

    StartPacket startPacket;
    // copy the packetHeader and the sequenceNumberMax
    memcpy(&startPacket, first, sizeof(PacketHeader) + sizeof(uint32_t));

    startPacket.fileName = reinterpret_cast<uint8_t *>(&first->buffer[sizeof(uint32_t) + sizeof(PacketHeader)]);
    startPacket.nameLen = first->len - sizeof(PacketHeader) - sizeof(uint32_t);

    // calculate the amount of packets
    size_t n = startPacket.sequenceNumberMax - startPacket.packetHeader.sequenceNumber;

    if (incomingPacketList.size() < n + 1){
        // apparently not all packets have yet arrived. waiting for all to arrive
        return;
    }

    // copy all packets from the incoming vector
    Packet packets[n - 1];
    for (int i = 1; i < n; i++){
        memcpy(&packets[i - 1], incomingPacketList[i], sizeof(PacketHeader));
        packets[i - 1].data = reinterpret_cast<uint8_t *>(&incomingPacketList[i]->buffer[sizeof(PacketHeader)]);
        packets[i - 1].dataLen = incomingPacketList[i]->len - sizeof(PacketHeader);
    }
    // the packets could be in wrong order
    sortPackets(packets, n - 1);

    // the nth packet is the end packet
    EndPacket endPacket;
    memcpy(&endPacket, incomingPacketList[n], sizeof(EndPacket));

    // if the packets or the checksum doesn't match
    if (!checkCorrectnessOfPackets(&startPacket, packets, &endPacket)){
        std::cerr << "checksum wrong or other problem with packets, deleting and skipping" << std::endl;
    }

    std::cout << "incoming packages: " << startPacket;
    for (auto i : packets){
        std::cout << i;
    }
    std::cout << endPacket;

    savePacketsToFile(&startPacket, packets);

    for (int i = 0; i < n; i++) {
        incomingPacketList.erase(incomingPacketList.begin());
    }
}

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
    int n = ceil((double) dataLen / MAX_DATA_LEN);
    // std::cout << "number of packets for this message is " << n << std::endl;

    // create start packet
    auto *packetHeader = new PacketHeader;
    auto *startPacket = new StartPacket;
    fillPacketHeader(packetHeader, transmissionId++, distr(gen));
    fillStartPacket(startPacket, packetHeader, n, fileName, fileNameLen);
    pushToPacketQueue(startPacket);

    increaseSequenceNumber(packetHeader);

    // split data into n packets with max size of 65527 byte

    size_t dataLenCopy = dataLen;
    for (int i = 0; i < n; i++) {
        auto *packet = new Packet;
        memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));
        packet->data = data + (size_t) (i * MAX_DATA_LEN);
        packet->dataLen = std::min(dataLenCopy, (size_t) MAX_DATA_LEN);

        dataLenCopy -= MAX_DATA_LEN;
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

bool SocketHelper::msgOut() const{
    return msgSend;
}

void SocketHelper::createSocketSend(int *socket1) {
    // creating socket
    *socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    int con;

    // specifying address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT_NUMBER);
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

void SocketHelper::createSocketRecv(int *socket1) {
    // create a socket:
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
    serv_addr.sin_port = htons(PORT_NUMBER);

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
    createSocketSend(&socket1);

    if (dstIpAddr != nullptr) {
        std::cout << "sending packet to: addr->" << inet_ntoa(dstIpAddr->sin_addr);
        std::cout << " port->" << ntohs(dstIpAddr->sin_port) << std::endl;
    }

    while (!packetQueue.empty()){
        auto elem = packetQueue.front();
        packetQueue.pop();
        std::visit([&socket1](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Packet *>) {
                // std::cout << *arg;

                // send the msg
                char packetAsArray[BUFFER_LEN];
                memcpy(packetAsArray, arg, sizeof(PacketHeader));
                for (int i = 0; i < reinterpret_cast<Packet *> (arg)->dataLen; i++){
                    packetAsArray[i + sizeof(PacketHeader)] = reinterpret_cast<Packet *> (arg)->data[i];
                }

                ssize_t n;
                int maxTryCount = 0; // to avoid an endless loop
                do {
                    n = send(socket1, packetAsArray,
                             sizeof(PacketHeader) + reinterpret_cast<Packet *> (arg)->dataLen, 0);
                    maxTryCount++;
                } while (n < 0 && maxTryCount < 10);

                std::cout << "packet sent " << n << " bytes" << std::endl;

                auto *p = reinterpret_cast<Packet *> (arg);
                // delete p->data;
                delete p;
            }
            else if constexpr (std::is_same_v<T, StartPacket *>){
                std::cout << *arg;

                // send the msg
                char packetAsArray[BUFFER_LEN];
                // load packetHeader and sequence number into buffer
                memcpy(packetAsArray, arg, sizeof(PacketHeader) + sizeof(uint32_t));
                for (int i = 0; i < reinterpret_cast<StartPacket *> (arg)->nameLen; i++){
                    packetAsArray[i + sizeof(PacketHeader) + sizeof(uint32_t)] =
                            reinterpret_cast<StartPacket *> (arg)->fileName[i];
                }
                ssize_t n = send(socket1, packetAsArray,
                     sizeof(uint32_t) + sizeof(PacketHeader) + reinterpret_cast<StartPacket *> (arg)->nameLen, 0);

                std::cout << "start packet sent " << n << " bytes" << std::endl;
                auto *p = reinterpret_cast<StartPacket *> (arg);
                delete p->fileName;
                delete p;
            }
            else if constexpr (std::is_same_v<T, EndPacket *>){
                std::cout << *arg;

                // send the msg
                char packetAsArray[BUFFER_LEN];
                memcpy(packetAsArray, arg, sizeof(EndPacket ));
                ssize_t n = send(socket1, packetAsArray, sizeof(EndPacket), 0);
                std::cout << "end packet sent " << n << " bytes" << std::endl;

                auto *p = reinterpret_cast<EndPacket *> (arg);
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
    createSocketRecv(&socket1);

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

        /*
        std::cout << "incoming msg: ";
        for (int i = 0; i < n; i++) {
            std::cout << std::hex << (int) buffer[i];
        }
        std::cout << std::endl;
         */

        // msg comes in multiple goes
        // have to concatenate messages
        pushToIncomingQueue(buffer, n);
        processIncomingMsg();
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


