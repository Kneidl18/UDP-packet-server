//
// Created by Andreas Auer on 19/03/24.
//

#ifndef PP_CONFIGURATION_H
#define PP_CONFIGURATION_H

typedef enum { SLAVE, MASTER } Config;

#define CONFIGURATION SLAVE

#define MAX_SLIDING_WINDOW_SIZE 10
#define MIN_SLIDING_WINDOW_SIZE 1

#define PORT_NUMBER 8080

#endif // PP_CONFIGURATION_H
