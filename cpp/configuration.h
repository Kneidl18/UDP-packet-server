//
// Created by Andreas Auer on 19/03/24.
//

#ifndef PP_CONFIGURATION_H
#define PP_CONFIGURATION_H

typedef enum {
    SLAVE,
    MASTER,
    MASTER_SLAVE
}Config;

#define CONFIGURATION MASTER_SLAVE

#endif //PP_CONFIGURATION_H
