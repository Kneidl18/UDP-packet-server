//
// Created by Andreas Auer on 19/03/24.
//

#ifndef PP_SOCKETHELPER_H
#define PP_SOCKETHELPER_H

#include "configuration.h"
#include <iostream>
#include <queue>
#include <netinet/in.h>
#include <sys/socket.h>
#include <list>
#include "PacketStructure.h"

class SocketHelper {
private:
    std::queue<packetVariant> packetQueue;
    uint16_t transmissionId = 0;
    bool msgSend = true;
    struct sockaddr_in serv_addr;
    struct sockaddr_in *dstIpAddr = nullptr;
    // std::vector<IncomingPacket *> incomingPacketList;
    std::vector<Transmission *> incomingTransmission;
    std::string outputDir;

    static void fillPacketHeader(PacketHeader *packetHeader, uint16_t tId, uint32_t seqNum);
    static void fillPacket(Packet *packet, PacketHeader *packetHeader, uint8_t *data, size_t dataLen);
    static void fillStartPacket(StartPacket *packet, PacketHeader *packetHeader, size_t n, uint8_t *fileName, size_t nameLen);
    static void fillEndPacket(EndPacket *packet, PacketHeader *packetHeader, const uint8_t *checksum);
    static void increaseSequenceNumber(PacketHeader *header);

    static void calcChecksum(EndPacket *endPacket, uint8_t *data, size_t dataLen, uint8_t *fileName,
                      size_t fileNameLen);
    void calcChecksumFromTransmission(Transmission *transmission, EndPacket *endPacket);
    static bool checkCorrectnessOfPackets(StartPacket *startPacket, Packet *packets, EndPacket *endPacket);

    bool pushToPacketQueue(packetVariant packet);
    bool pushToIncomingQueue(char *buffer, ssize_t len);

    void createSocketRecv(int *socket1);
    void createSocketSend(int *socket1);

    void runMaster();
    void runSlave(const bool *run);

    void processIncomingMsg(Transmission *t);
    void checkFinishedTransmission();

    static void sortPackets(Packet *packets, size_t n);

    bool savePacketsToFile(StartPacket *startPacket, Packet *packets);

public:
    void run(const bool *run, Config config);

    bool sendMsg(uint8_t *data, size_t dataLen, uint8_t *fileName, size_t fileNameLen);

    bool msgOut() const;

    void setIpSettings(uint8_t *dstIp, size_t port);

    void setOutputDirPath(std::string outDir);

    void calcChecksumFromPackets(StartPacket *startPacket, Packet *packets, EndPacket *endPacket, size_t packetAmount);

};


#endif //PP_SOCKETHELPER_H
