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
#include "PacketStructure.h"

class SocketHelper {
private:
    using packetVariant = std::variant<Packet *, StartPacket *, EndPacket *>;
    std::queue<packetVariant> packetQueue;
    uint16_t transmissionId = 0;
    bool msgSend = true;
    struct sockaddr_in serv_addr;
    struct sockaddr_in *dstIpAddr = nullptr;

    void fillPacketHeader(PacketHeader *packetHeader, uint16_t tId, uint32_t seqNum);
    void fillPacket(Packet *packet, PacketHeader *packetHeader, uint8_t *data, size_t dataLen);
    void fillStartPacket(StartPacket *packet, PacketHeader *packetHeader, size_t n, uint8_t *fileName, size_t nameLen);
    void fillEndPacket(EndPacket *packet, PacketHeader *packetHeader, uint8_t *checksum);

    void calcChecksum(EndPacket *endPacket, uint8_t *data, size_t dataLen, uint8_t *fileName,
                      size_t fileNameLen);

    bool pushToPacketQueue(packetVariant packet);

    void createSocketRecv(int portNum, int *socket1);
    void createSocketSend(int portNum, int *socket1);

    void runMaster();
    void runSlave(const bool *run);

    void increaseSequenceNumber(PacketHeader *header);

public:
    int socketNum;

    void run(const bool *run, Config config);

    bool sendMsg(uint8_t *data, size_t dataLen, uint8_t *fileName, size_t fileNameLen);

    bool msgOut();

    void setIpSettings(uint8_t *dstIpAddr, size_t port);

    bool pushToIncommingQueue(char *buffer, ssize_t len);
};


#endif //PP_SOCKETHELPER_H
