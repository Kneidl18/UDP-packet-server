#include <iostream>
#include <thread>
#include <fstream>
#include <print>

#include "SocketHelper.h"

void printMenu(){
    std::cout << "Your options are (0)exit, (1)send message, (2)" << std::endl;
}

/**
 * load the filename into the param filename
 * by searching the path for the last / and
 * point the fileName to 1 char after that
 * /; if there is no / then
 * filename = filePath
 * @param filePath
 * @param fileName
 */
void getFileNameFromPath(std::string filePath, char **fileName){
    // start searching for the fileName at the pre-last character
    for (int i = (int) filePath.length() - 2; i >= 0; i--){
        if (filePath[i] != '/') {
            continue;
        }

        *fileName = (char *) calloc (sizeof(char), filePath.length() - i + 1);
        for (int j = i; j < filePath.length() - 1; j++){
            (*fileName + (j - i))[0] = filePath[j + 1];
        }
        break;
    }
}

/**
 * get size of bytes in file
 * @param filename
 * @return
 */
std::ifstream::pos_type filesize(std::string filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

/**
 * load the data from the file into data
 * @param data
 * @param filePath
 * @return
 */
size_t loadData(uint8_t **data, std::string filePath){
    std::ifstream file (filePath);
    /*
    if (filePath.find('/') == std::string::npos){
        // if no / is found in filepath
        // e.g. if it's a relative path
        // like 'test.txt'
        file.open("./" + filePath);
    }
    else {
        // if it's an absolute path
        file.open(filePath);
    }
     */

    size_t dataLen = filesize(filePath);
    *data = (uint8_t *) malloc (dataLen);

    size_t count = 0;

    // std::cout << "reading file content: ";
    if (file.is_open()){
        while (file){
            // load the next character, print it and save it to the data[]
            uint8_t nextChar = file.get();
      //       std::cout << " " << nextChar << ":";
        //     std::cout << std::hex << (int) nextChar;
            (*data + count++)[0] = nextChar;
        }
    }
    else{
        file.close();
        std::cerr << "couldn't open file; exiting" << std::endl;
        delete *data;
        exit(1);
    }

    // std::cout << std::endl;
    file.close();
    return dataLen;
}

/**
 * parse the incoming arguments
 * @param argc amount of arguments passed
 * @param argv arguments as char[]
 * @return 1 if the command is listen, 2 if the command is send
 */
int parseArgs(int argc, char *argv[], std::string *filePath, size_t *port, uint8_t *ipAddr, std::string *outputDirPath){
    for (int i = 0; i < argc; i++){
        if (!strcmp("--listen", argv[i])) {
            // the command is listening
            return 1;
        }
        else if (!strcmp("--send", argv[i])){
            // the command is sending
            *filePath += argv[i + 1];
            while (++i < argc - 1){
                *filePath += " ";
                *filePath += argv[i + 1];
            }

            return 2;
        }
        else if (!strcmp("-o", argv[i])){
            // the command is sending
            *outputDirPath += argv[i + 1];
            while (!strcmp("--send", argv[++i]) && i < argc){
                *outputDirPath += " ";
                *outputDirPath += argv[i + 1];
            }

            std::cout << "output filepath: " << *outputDirPath << std::endl;
            i--;
        }
        else if (!strcmp("-p", argv[i])){
            // specified port
            char *portAsString = argv[i + 1];
            *port = strtol(portAsString, nullptr, 10);
            std::cout << "input port: " << *port << std::endl;

            // globally skip the value after -p
            i++;
        }
        else if (!strcmp("--ip", argv[i])){
            // specified ip, only used when sending
            char *ipAsString = argv[i + 1];

            std::cout << "input ip addr: ";
            // convert dot notated addr to 4 byte addr
            for (auto j = 0; j < 4; j++){
                // find dot, convert data up to dot
                char *pos = strchr(ipAsString, '.');
                char ip[4] = {0, 0, 0, 0};

                if (pos == nullptr){
                    // error maximum amount of numbers per ipAddr byte is 3
                    pos = strchr(ipAsString, '\0');

                    if (pos == nullptr){
                        // it's also not the last of the 4 bytes
                        std::cout << "error in ip addr" << std::endl;
                        exit(1);
                    }
                }

                memcpy(ip, ipAsString, pos - ipAsString);
                auto tmp = (uint8_t) strtol(ip, nullptr, 10);
                ipAddr[j] = tmp;
                std::cout << (int) ipAddr[j] << ":";
                ipAsString = pos + 1;
            }

            std::cout << std::endl;
            // globally skip the value after --ip
            i++;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2){
        std::cerr << "usage: simpleServer [-p <port>] [--ip <ip_addr>] [-o <output path>]";
        std::cerr << "[--listen] [--send <pathToFile>]" << std::endl;
        exit(1);
    }

    std::string filePath;
    std::string outputDirPath;
    uint8_t dstIpAddr[4] = {0, 0, 0, 0};
    size_t port = 0;
    int runCommand = parseArgs(argc, argv, &filePath, &port, dstIpAddr, &outputDirPath);

    auto *sh = new SocketHelper();
    bool run = true;

    if (runCommand == 0 || runCommand == 1) {
        if (port != 0)
            sh->setIpSettings(dstIpAddr, port);

        std::thread socketThreadListen(&SocketHelper::run, sh, &run, SLAVE);

        std::string command;
        while (command != "exit"){
            std::cout << "options are 'exit'" << std::endl;
            std::cin >> command;
        }

        std::cout << "stopping socket listening thread" << std::endl;
        run = false;
        socketThreadListen.join();
    }
    else if (runCommand == 2) {
        uint8_t *data = nullptr;
        char *fileName = nullptr;

        size_t dataLen = loadData(&data, std::string(filePath));
        getFileNameFromPath(filePath, &fileName);

        if (data == nullptr || fileName == nullptr){
            std::cerr << "data of filename is nullptr!! data: " << data << " filename: " << fileName << std::endl;
            exit(1);
        }

        sh->sendMsg(data, dataLen, (uint8_t *) fileName, strlen(fileName));
        if (port != 0)
            sh->setIpSettings(dstIpAddr, port);

        std::thread socketThreadSending(&SocketHelper::run, sh, &run, MASTER);

        // wait for the message to be sent
        while (!sh->msgOut()){}

        std::cout << "stopping socket sending thread" << std::endl;
        run = false;
        socketThreadSending.join();
        delete data;
    }

    delete sh;
    std::cout << "exit" << std::flush;
    return 0;
}
