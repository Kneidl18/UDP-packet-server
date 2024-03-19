//
// Created by Andreas Auer on 19/03/24.
//

#include "SocketHelper.h"
#include <iostream>

void fillPacket(uint16_t tId, uint32_t seqNum, uint8_t *data, size_t dataLen);
// TODO: fileNameLen 1-256 byte!!
bool sendMsg(uint8_t *data, size_t dataLen, uint8_t *fileName, size_t fileNameLen);

// TODO: i would suggest to make the checksum of the filename + data (all concatenated together)
/**
 * calculates the checksum of the data and the filename
 */
void calcChecksum();


void run(){
    switch (CONFIGURATION){
        case MASTER:
        case SLAVE:
        case MASTER_SLAVE:
        default:
            std::cout << "server config unknown" << std::endl;
            exit(1);
            break;
    }
}
