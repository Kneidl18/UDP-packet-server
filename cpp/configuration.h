//
// Created by Andreas Auer on 19/03/24.
//

#ifndef PP_CONFIGURATION_H
#define PP_CONFIGURATION_H

typedef enum { SLAVE, MASTER } Config;

// timeout waiting for an ACK
#define TIMEOUT 1000

// default port number
#define PORT_NUMBER 8080

// maximum data per packet
#define MAX_DATA_LEN 1000.0
#define BUFFER_LEN (uint) MAX_DATA_LEN

// millisecond a packet is kept before it's deleted from the vector
#define PACKET_TIMEOUT 1000

#endif // PP_CONFIGURATION_H
