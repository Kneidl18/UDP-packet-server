//
// Created by Andreas Auer on 19/03/24.
//

#ifndef PP_PACKETSTRUCTURE_H
#define PP_PACKETSTRUCTURE_H

#include <cstdint>
#include <bitset>
#include <variant>
#include <chrono>

#define MAX_DATA_LEN 9000.0
#define BUFFER_LEN (uint) MAX_DATA_LEN
#define PACKET_TIMEOUT 50000 // millisecond a packet is kept before it's deleted from the vector

/**
 * structs for the packets
 */
typedef struct{
    uint16_t transmissionId;
    uint32_t sequenceNumber;
} PacketHeader;

typedef struct{
    PacketHeader packetHeader;
    uint8_t *data;
    size_t dataLen;
} Packet;

typedef struct{
    PacketHeader packetHeader;
    uint32_t sequenceNumberMax;
    uint8_t *fileName;
    size_t nameLen;
} StartPacket;

typedef struct{
    PacketHeader packetHeader;
    uint8_t checksum[16];
} EndPacket;

using PacketVariant = std::variant<Packet *, StartPacket *, EndPacket *>;

typedef struct{
    PacketHeader header;
    uint32_t sequenceNumMax;
    std::vector<PacketVariant> transmission;
    bool transmissionComplete;
    std::chrono::system_clock::time_point openTime;
} Transmission;

/**
 * overwrite << to pretty-print a packet
 */
inline std::ostream& operator << (std::ostream& o, Packet& p) {
    std::bitset<16> trans(p.packetHeader.transmissionId);
    std::bitset<32> sequence(p.packetHeader.sequenceNumber);


    o << "packet: tId->" << p.packetHeader.transmissionId << "\tsNr->" << p.packetHeader.sequenceNumber << "\tdata->";
    for (auto i = 0; i < p.dataLen && i < 100; i++){
        o << p.data[i];
    }
    o << std::endl;

    /*
    o << "packet: tId->" << trans << "\tsNr->" << sequence << "\tdata->";
    for (auto i = 0; i < p.dataLen; i++){
        std::bitset<8> x(p.data[i]);
        o << x << " ";
    }
    o << std::endl;
     */

    return o;
}

/**
 * overwrite << to pretty-print a packet
 */
inline std::ostream& operator << (std::ostream& o, StartPacket & p) {
    std::bitset<16> trans(p.packetHeader.transmissionId);
    std::bitset<32> sequence(p.packetHeader.sequenceNumber);
    std::bitset<32> maxSequence(p.sequenceNumberMax);

    o << std::endl;
    o << "start packet: tId->" << p.packetHeader.transmissionId << "\tsNr->" << p.packetHeader.sequenceNumber;
    o << "\tmaxSNr->" << p.sequenceNumberMax << "\tfileName->" << p.fileName << std::endl;

    /*
    o << "packet: tId->" << trans << "\tsNr->" << sequence << "\tmaxSNr->" << maxSequence;
    o << std::endl;
    */
    return o;
}

/**
 * overwrite << to pretty-print a packet
 */
inline std::ostream& operator << (std::ostream& o, EndPacket & p) {
    std::bitset<16> trans(p.packetHeader.transmissionId);
    std::bitset<32> sequence(p.packetHeader.sequenceNumber);

    o << "end packet:   tId->" << p.packetHeader.transmissionId << "\tsNr->" << p.packetHeader.sequenceNumber;
    o << "\tchecksum->";
    for (auto i : p.checksum)
        o << std::hex << (int) i;

    std::cout << std::endl << std::endl;

    /*
    o << "packet: tId->" << trans << "\tsNr->" << sequence << "\tchecksum->";
    for (unsigned char i : p.checksum){
        std::bitset<8> x(i);
        o << x << " ";
    }
    o << std::endl;
     */

    return o;
}

#endif //PP_PACKETSTRUCTURE_H
