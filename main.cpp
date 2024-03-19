#include <iostream>

#include "PacketStructure.h"
#include "configuration.h"





int main() {
    auto *myPacket = new Packet;
    char data[] = "hi this is some data";

    myPacket->transmissionId = 0b1001;
    myPacket->sequenceNumber = 0b10111101;
    myPacket->data = (uint8_t *) data;
    myPacket->dataLen = strlen(data);

    std::cout << *myPacket;

    delete myPacket;
    return 0;
}
