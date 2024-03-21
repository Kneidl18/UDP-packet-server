//
// Created by Andreas Auer on 19/03/24.
//

#ifndef PP_PACKETSTRUCTURE_H
#define PP_PACKETSTRUCTURE_H

#include <cstdint>
#include <bitset>

typedef struct{
uint16_t transmissionId;
uint32_t sequenceNumber;
uint8_t *data;
size_t dataLen;
} Packet;

typedef struct{
   uint16_t transmissionId;
   uint32_t sequenceNumber;
   uint32_t sequenceNumberMax;
   char *fileName;
   size_t nameLen;
} StartPacket;

typedef struct{
    uint16_t transmissionId;
    uint32_t sequenceNumber;
    uint8_t checksum[16];
} EndPacket;

std::ostream& operator << (std::ostream& o, Packet& p) {
    std::bitset<16> trans(p.transmissionId);
    std::bitset<32> sequence(p.sequenceNumber);

    o << "packet: tId->" << p.transmissionId << "\tsNr->" << p.sequenceNumber << "\tdata->";
    for (auto i = 0; i < p.dataLen; i++){
        o << p.data[i];
    }
    o << std::endl;

    o << "packet: tId->" << trans << "\tsNr->" << sequence << "\tdata->";
    for (auto i = 0; i < p.dataLen; i++){
        std::bitset<8> x(p.data[i]);
        o << x << " ";
    }
    o << std::endl << std::endl;

    return o;
}

#endif //PP_PACKETSTRUCTURE_H
