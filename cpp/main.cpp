#include <iostream>
#include <thread>
#include <fstream>

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
        for (int j = i; j < filePath.length(); j++){
            (*fileName + (j - i))[0] = filePath[j];
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

    std::cout << "reading file content: ";
    if (file.is_open()){
        while (file){
            // load the next character, print it and save it to the data[]
            uint8_t nextChar = file.get();
            std::cout << " " << nextChar << ":";
            std::cout << std::hex << (int) nextChar;
            (*data + count++)[0] = nextChar;
        }
    }
    else{
        file.close();
        std::cerr << "couldn't open file; exiting" << std::endl;
        delete *data;
        exit(1);
    }

    std::cout << std::endl;
    file.close();
    return dataLen;
}

/**
 * parse the incoming arguments
 * @param argc amount of arguments passed
 * @param argv arguments as char[]
 * @return 1 if the command is listen, 2 if the command is send
 */
int parseArgs(int argc, char *argv[], std::string *filePath){
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
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 1){
        std::cerr << "usage: simpleServer [--listen] [--send <pathToFile>]" << std::endl;
        exit(1);
    }

    std::string filePath;
    int runCommand = parseArgs(argc, argv, &filePath);

    auto *sh = new SocketHelper();
    bool run;

    if (runCommand == 0 || runCommand == 1) {
        std::thread socketThreadListen(&SocketHelper::run, sh, &run);

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

        run = true;
        std::thread socketThread(&SocketHelper::run, sh, &run);

        // wait for the message to be sent
        while (!sh->msgOut()){}

        std::cout << "stopping socket sending thread" << std::endl;
        run = false;
        socketThread.join();
        delete data;
    }

    delete sh;
    std::cout << "exit" << std::flush;
    return 0;
}
