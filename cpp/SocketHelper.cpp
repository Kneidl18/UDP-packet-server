//
// Created by Andreas Auer on 19/03/24.
//

#include "SocketHelper.h"
#include <chrono>
#include <iomanip>
#include <random>
#include <cmath>
#include <arpa/inet.h>
#include <print>
#include <sys/fcntl.h>
#include <fstream>
#include <utility>
#include "md5.h"

#define PORT_NUMBER 8080

std::random_device rd; // obtain a random number from hardware
std::mt19937 gen(rd()); // seed the generator
std::uniform_int_distribution<> distr(0, (int) pow(2, 31)); // define the range, leave 2^31 packets open

/**
 * set ip address and port for specific dst port/ip addr
 * @param dstIp
 * @param port
 */
void SocketHelper::setIpSettings (uint8_t *dstIp, size_t port){
    this->dstIpAddr = new sockaddr_in;
    this->dstIpAddr->sin_family = AF_INET;
    this->dstIpAddr->sin_port = htons(port);

    memcpy(&this->dstIpAddr->sin_addr, dstIp, 4);
}

/**
 * set the output directory
 * @param outDir
 */
void SocketHelper::setOutputDirPath (std::string outDir){
    this->outputDir = std::move(outDir);
}

/**
 * fills the referenced packetHeader with the transmissionId and
 * sequence number
 * @param pHeader
 * @param tId
 * @param seqNum
 */
void SocketHelper::fillPacketHeader (PacketHeader *pHeader, uint16_t tId, uint32_t seqNum){
    pHeader->transmissionId = tId;
    pHeader->sequenceNumber = seqNum;
}

/**
 * fill the packet with the referenced packetHeader, data and dataLen
 * @param packet
 * @param packetHeader
 * @param data
 * @param dataLen
 */
void SocketHelper::fillPacket (Packet *packet, PacketHeader *packetHeader, uint8_t *data, size_t dataLen) {
    memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));
    packet->data = data;
    packet->dataLen = dataLen;
}

/**
 * fill the start packet with a packet-header and a filename
 * @param packet
 * @param packetHeader
 * @param n
 * @param fileName
 * @param nameLen
 */
void SocketHelper::fillStartPacket (StartPacket *packet, PacketHeader *packetHeader, size_t n, uint8_t *fileName,
                                    size_t nameLen){
    memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));
    // sequenceNumberMax = start + numberPackets + endPacket(1)
    packet->sequenceNumberMax = packetHeader->sequenceNumber + n + 1;
    packet->fileName = fileName;
    packet->nameLen = std::min(nameLen, (size_t) 256);
}

/**
 * fill endPacket with header and checksum
 * @param packet
 * @param packetHeader
 * @param checksum
 */
void SocketHelper::fillEndPacket (EndPacket *packet, PacketHeader *packetHeader, const uint8_t *checksum){
    memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));

    for (int i = 0; i < 16; i++){
        packet->checksum[i] = checksum[i];
    }
}

/**
 * increase the sequence number by 1 (each packet has increasing sequence numbers
 * @param header
 */
void SocketHelper::increaseSequenceNumber (PacketHeader *header) {
    header->sequenceNumber++;
}

/**
 * calculates the checksum of the data and the filename
 * @param endPackage packet struct to edit
 * @param data pointer to data
 * @param dataLen
 * @param fileName pointer to file name
 * @param fileNameLen
 */
void SocketHelper::calcChecksum (EndPacket *endPacket, uint8_t *data, size_t dataLen, uint8_t *fileName,
                                 size_t fileNameLen) {

    // std::string tmp = std::string(data, dataLen);
    md5::md5_t md5_o;

    md5_o.process(fileName, fileNameLen);
    md5_o.process(data, dataLen);

    md5_o.finish(endPacket->checksum);
}

/**
 * calculates the checksum of the data and the filename
 * @param transmission transmission to calculate checksum from (data and filename)
 */
void SocketHelper::calcChecksumFromTransmission (Transmission *transmission, EndPacket *endPacket) {

    // std::string tmp = std::string(data, dataLen);
    md5::md5_t md5_o;
    auto *startPacket = std::get<StartPacket *> (transmission->transmission.front());

    // process the filename
    md5_o.process(startPacket->fileName, startPacket->nameLen);

    for (auto i = 1; i < transmission->transmission.size() - 1; i++){
        auto *packet = std::get<Packet *> (transmission->transmission[i]);
        md5_o.process(packet->data, packet->dataLen);
    }

    md5_o.finish(endPacket->checksum);
}

/**
 * saves the content of the transmission to the filename specified in the startPacket
 * the directory to store the packet in is in the object variable outputDir
 * @param startPacket
 * @param packets
 * @return
 */
bool SocketHelper::saveTransmissionToFile (Transmission *t) {
    auto startPacket = std::get<StartPacket *> (t->transmission.front());

    // create output file dir from received filename and stored file path
    std::string outFilePath = outputDir;
    outFilePath += std::string(reinterpret_cast<char *> (startPacket->fileName));

    if (!std::filesystem::exists(outputDir.c_str())) {
        std::filesystem::create_directory(outputDir.c_str());
    }

    // don't worry about seg fault because of fileName being an uint8_t
    // as it points to a 65000 byte char array there is always a \0 at the end
    std::ofstream output_file(outFilePath);
    std::ostream_iterator<std::string> output_iterator(output_file, "\n");

    if (!output_file.is_open()){
        // file couldn't be opened...
        std::cerr << "couldn't open file to store incoming message" << std::endl;
        return false;
    }

    for (int i = 1; i < t->transmission.size() - 1; i++){
        for (int j = 0; j < std::get<Packet *> (t->transmission[i])->dataLen; j++)
            output_file << std::get<Packet *> (t->transmission[i])->data[j];
    }

    return true;
}

/**
 * saves the content of the packets to the filename specified in the startPacket
 * the directory to store the packet in is in the object variable outputDir
 * @param startPacket
 * @param packets
 * @return
 */
bool SocketHelper::savePacketsToFile (StartPacket *startPacket, Packet *packets) {
    size_t n = startPacket->sequenceNumberMax - startPacket->packetHeader.sequenceNumber - 1;

    // create output file dir from received filename and stored file path
    std::string outFilePath = outputDir;
    outFilePath += std::string(reinterpret_cast<char *> (startPacket->fileName));

    if (!std::filesystem::exists(outputDir.c_str())) {
        std::filesystem::create_directory(outputDir.c_str());
    }

    // don't worry about seg fault because of fileName being an uint8_t
    // as it points to a 65000 byte char array there is always a \0 at the end
    std::ofstream output_file(outFilePath);
    std::ostream_iterator<std::string> output_iterator(output_file, "\n");

    if (!output_file.is_open()){
        // file couldn't be opened...
        std::cerr << "couldn't open file to store incoming message" << std::endl;
        return false;
    }
    for (int i = 0; i < n; i++){
        for (int j = 0; j < packets[i].dataLen; j++)
            output_file << packets[i].data[j];
    }

    return true;
}

/**
 * sort the packets[] using insertion sort (should be pretty fast
 * as the packets should be in correct order)
 * @param packets
 * @param n
 * @return
 */
void SocketHelper::sortPackets (Packet *packets, size_t n){
    Packet key;
    for (int i = 1; i < n; i++){
        int j = i - 1;
        key = packets[i];
        while (j >= 0 && packets[j].packetHeader.sequenceNumber > key.packetHeader.sequenceNumber) {
            packets[j+1] = packets[j];
            j--;
        }
        packets[j + 1] = key;
    }
}

/**
 * helper to sort packets by the sequence number
 * @param a first packet
 * @param b second packet
 * @return a < b (sequence number)
 */
bool compareBySequenceNumber(PacketVariant &a, PacketVariant &b){
    return std::get<Packet *> (a)->packetHeader.sequenceNumber <
            std::get<Packet *> (b)->packetHeader.sequenceNumber;
}

/**
 * sort the packets in a transmission
 * we expect the first and last packet to be in the correct spot
 * @param t transmission to sort
 */
void SocketHelper::sortPackets (Transmission *t) {
    std::sort(t->transmission.begin()+1, t->transmission.end()-1, compareBySequenceNumber);
}

/**
 * checks the correctness of the received packets (also checksum)
 * @param startPacket
 * @param packets array of all packets (amount should be seqNumMax - seqNumStart
 * @param endPacket
 * @return true if all packets are correct, false if there is a problem with the packets (also checks checksum)
 */
bool SocketHelper::checkCorrectnessOfTransmission (Transmission *t){
    auto startPacket = std::get<StartPacket *> (t->transmission.front());
    uint32_t sequenceNumberEnd = startPacket->sequenceNumberMax;
    uint32_t sequenceNumberStart = startPacket->packetHeader.sequenceNumber;

    for (int i = 1; i < sequenceNumberEnd - sequenceNumberStart; i++) {
        if (std::get<Packet *> (t->transmission[i])->packetHeader.sequenceNumber != sequenceNumberStart + i){
            // sequence number doesn't seem to match nth element
            std::cerr << "packets are missing" << std::endl;
            // exit(1);
            return false;
        }
    }

    // calcChecksum(&dummy, data, dataPointer, startPacket->fileName, startPacket->nameLen);
    EndPacket dummy;
    calcChecksumFromTransmission(t, &dummy);

    auto endPacket = std::get<EndPacket *> (t->transmission[t->transmission.size() - 1]);
    for (int i = 0; i < 16; i++){
        if (dummy.checksum[i] != endPacket->checksum[i]){
            std::cerr << "error in checksum" << std::endl;
            return false;
        }
    }

    return true;
}

/**
 * checks the correctness of the received packets (also checksum)
 * @param startPacket
 * @param packets array of all packets (amount should be seqNumMax - seqNumStart
 * @param endPacket
 * @return true if all packets are correct, false if there is a problem with the packets (also checks checksum)
 */
bool SocketHelper::checkCorrectnessOfPackets (StartPacket *startPacket, Packet *packets, EndPacket *endPacket){
    uint32_t sequenceNumberEnd = startPacket->sequenceNumberMax;
    uint32_t sequenceNumberStart = startPacket->packetHeader.sequenceNumber;

    for (int i = 0; i < sequenceNumberEnd - sequenceNumberStart - 1; i++) {
        if (packets[i].packetHeader.sequenceNumber != sequenceNumberStart + i + 1){
            // sequence number doesn't seem to match nth element
            std::cerr << "packets are missing" << std::endl;
            // exit(1);
            return false;
        }
    }

    if (endPacket->packetHeader.sequenceNumber != sequenceNumberEnd){
        // sequence number of the endPacket doesn't match sequenceNumberMax from Start Header
        std::cerr << "sequence number of end packet doesnt' match sequence number written in startPacket" << std::endl;
        // exit(1);
        return false;
    }

    EndPacket dummy;
    uint8_t data[(uint32_t) (sizeof(Packet) + MAX_DATA_LEN) * (sequenceNumberEnd - sequenceNumberStart - 1)];
    size_t dataPointer = 0;

    for (int i = 0; i < sequenceNumberEnd - sequenceNumberStart - 1; i++){
        auto tmpData = packets[i].data;
        memcpy(data + dataPointer, packets[i].data, packets[i].dataLen);
        dataPointer += packets[i].dataLen;
    }

    calcChecksum(&dummy, data, dataPointer, startPacket->fileName, startPacket->nameLen);

    for (int i = 0; i < 16; i++){
        if (dummy.checksum[i] != endPacket->checksum[i]){
            std::cerr << "error in checksum" << std::endl;
            return false;
        }
    }

    return true;
}

/**
 * pushes a package to the queue
 * @param packet pointer to the packet to be pushed
 * @return true if success, false if fail
 */
bool SocketHelper::pushToPacketQueue(PacketVariant packet) {
    packetQueue.push(packet);
    return packetQueue.back() == packet;
}

/**
 * push an incoming msg to the incoming msg queue to then process it
 * @param buffer
 * @param len
 * @return true if success, false if fail
 */
bool SocketHelper::pushToIncomingQueue(char *buffer, ssize_t len){
    if (len < sizeof(PacketHeader))
        // packets seems to be short...
        return false;

    // check if there is an open transmission existing in the transmission vector
    for (auto i : incomingTransmission){
        auto packet = std::get<StartPacket *> (i->transmission.front());

        // check if the packet belongs to the current transmission
        if (packet->packetHeader.sequenceNumber > reinterpret_cast<PacketHeader *> (buffer)->sequenceNumber
            || packet->sequenceNumberMax < reinterpret_cast<PacketHeader *> (buffer)->sequenceNumber){
            continue;
        }

        // packet is of this transmission, adding it
        // separating last packet from normal packet
        if (packet->sequenceNumberMax == reinterpret_cast<PacketHeader *> (buffer)->sequenceNumber) {
            auto *ep = new EndPacket {};
            memcpy(ep, buffer, sizeof(EndPacket));
            i->transmission.emplace_back(ep);

            // it's the last packet of this transmission
            i->transmissionComplete = true;
        }
        else{
            // create a new Packet and copy the data from the buffer into the packet
            auto *p = new Packet {};
            p->dataLen = len - sizeof(PacketHeader);
            p->data = (uint8_t *) calloc (sizeof(uint8_t), p->dataLen);
            memcpy(&p->packetHeader, buffer, sizeof(PacketHeader));
            memcpy(p->data, buffer + sizeof(PacketHeader), p->dataLen);

            // push the packet to the transmission vector
            i->transmission.emplace_back(p);
        }

        // reset time to keep the transmission open, as a new packet arrived
        i->openTime = std::chrono::system_clock::now();
        return true;
    }

    // if the packet wasn't added to an existing transmission, it ought
    // to be a new transmission
    auto t = new Transmission{};

    // create a new StartPacket and copy the data into it
    auto *sp = new StartPacket {};
    memcpy(&sp->packetHeader, buffer, sizeof(PacketHeader));
    sp->sequenceNumberMax = reinterpret_cast<StartPacket *> (buffer)->sequenceNumberMax;

    // calculate the length of the filename
    sp->nameLen = len - sizeof(PacketHeader) - sizeof(uint32_t);
    // create an uint8_t array and copy the filename into the allocated storage
    sp->fileName = (uint8_t *) calloc (sizeof(uint8_t), sp->nameLen);
    memcpy(sp->fileName, buffer + len - sp->nameLen, sp->nameLen);

    // load the data into the transmission struct
    t->transmissionComplete = false;
    memcpy(&t->header, buffer, sizeof(PacketHeader));
    t->sequenceNumMax = reinterpret_cast<StartPacket *> (buffer)->sequenceNumberMax;
    t->transmission.emplace_back(sp);
    t->openTime = std::chrono::system_clock::now(); // add the time when the transmission first appears

    // push the transmission to the transmission vector
    incomingTransmission.push_back(t);
    return true;
}

/**
 * check all transmissions for finished and timed out transmissions
 * calls processIncomingMsg for every finished transmission
 */
void SocketHelper::checkFinishedTransmission(){
    // check each transmission in the transmission vector for a finished
    // or a timed out transmission
    ssize_t count = 0;

    for (auto i : incomingTransmission){
        if (i->transmissionComplete) {
            // this transmission is complete, process it
            processIncomingMsg(i);

            // make sure all packets and the vector element itself are deleted
            // erase the packets in the transmission vector
            i->transmission.erase(i->transmission.begin(), i->transmission.end());
            // erase the transmission from the incoming transmission vector
            incomingTransmission.erase(incomingTransmission.begin() + count);
            count--;
            if (verboseOutput)
                std::cout << "received and processed a complete transmission" << std::endl;
        }
        else if ((double) (std::chrono::system_clock::now() - i->openTime).count() * 1000 > PACKET_TIMEOUT) {
            // the packet timed out, remove it

            // erase the packets in the transmission vector
            i->transmission.erase(i->transmission.begin(), i->transmission.end());
            // erase the transmission from the incoming transmission vector
            incomingTransmission.erase(incomingTransmission.begin() + count);
            count--;
            if (verboseOutput)
                std::cout << "delete a transmission due to timeout" << std::endl;
        }

        count++;
    }
}

/**
 * concatenate incoming messages to readable packages
 * each incoming packet is one packet of the transmission
 * convert them one after another to a packet and search
 * for the final packet
 */
void SocketHelper::processIncomingMsg(Transmission *t) {
    // the packets could be in wrong order
    sortPackets(t);

    // if the packets or the checksum doesn't match
    if (!checkCorrectnessOfTransmission (t)) {
        std::cerr << "checksum wrong or other problem with packets, deleting and skipping" << std::endl;
        return;
    }

    if (verboseOutput) {
        // print the packets for testing
        std::cout << "incoming packages: " << *std::get<StartPacket *>(t->transmission.front());
        std::cout << "amount of packets: " << t->transmission.size() - 2 << std::endl;
        /*for (auto i = 1; i < t->transmission.size() - 1; i++){
            std::cout << std::get<Packet *> (t->transmission[i]);
        }*/
        std::cout << *std::get<EndPacket *>(t->transmission[t->transmission.size() - 1]);
    }

    saveTransmissionToFile(t);
}

/**
 * send a message to the selected ip and port
 * @param data data to send
 * @param dataLen length of the data to send
 * @param fileName filename of the data
 * @param fileNameLen length of the filename
 * @return true if success, false otherwise
 */
bool SocketHelper::sendMsg (uint8_t *data, size_t dataLen, uint8_t *fileName, size_t fileNameLen){
    // calculate the number of packets necessary
    int n = ceil((double) dataLen / MAX_DATA_LEN);
    // std::cout << "number of packets for this message is " << n << std::endl;

    // create start packet
    auto *packetHeader = new PacketHeader;
    auto *startPacket = new StartPacket;
    fillPacketHeader(packetHeader, transmissionId++, distr(gen));
    fillStartPacket(startPacket, packetHeader, n, fileName, fileNameLen);
    pushToPacketQueue(startPacket);

    increaseSequenceNumber(packetHeader);

    // split data into n packets with max size of 65527 byte

    size_t dataLenCopy = dataLen;
    for (int i = 0; i < n; i++) {
        auto *packet = new Packet;
        memcpy(&packet->packetHeader, packetHeader, sizeof(PacketHeader));
        packet->data = data + (size_t) (i * MAX_DATA_LEN);
        packet->dataLen = std::min(dataLenCopy, (size_t) MAX_DATA_LEN);

        dataLenCopy -= MAX_DATA_LEN;
        pushToPacketQueue(packet);
        increaseSequenceNumber(packetHeader);
    }

    // create end packet
    auto *endPacket = new EndPacket;
    uint8_t checksum[16];
    fillEndPacket(endPacket, packetHeader, checksum);
    calcChecksum(endPacket, data, dataLen, fileName, fileNameLen);
    pushToPacketQueue(endPacket);

    msgSend = false;
    return true;
}

/**
 * check if the message has been sent
 * @return true if sent, false if not
 */
bool SocketHelper::msgOut() const{
    return msgSend;
}

/**
 * create a socket for sending
 * @param socket1 pointer to socket number
 */
void SocketHelper::createSocketSend(int *socket1) {
    // creating socket
    *socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    int con;

    // specifying address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT_NUMBER);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // sending connection request
    if (dstIpAddr == nullptr)
        con = connect(*socket1, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    else {
        con = connect(*socket1, (struct sockaddr *) dstIpAddr, sizeof(sockaddr_in));
    }

    if (con < 0){
        std::cerr << "error connecting to socket" << std::endl;
        exit(1);
    }
}

/**
 * create a socket for receiving
 * @param socket1 pointer to socker number (being replaced)
 */
void SocketHelper::createSocketRecv(int *socket1) {
    // create a socket:
    // socket(int domain, int type, int protocol)
    *socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    if (*socket1 < 0) {
        std::cerr << "error opening socket" << std::endl;
        exit(1);
    }

    // clear address structure
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_NUMBER);

    if (dstIpAddr == nullptr) {
        if (bind(*socket1, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "ERROR on binding" << std::endl;
            exit(1);
        }
        if (verboseOutput)
            std::cout << "listening on: " << inet_ntoa(serv_addr.sin_addr) << " port " << ntohs(serv_addr.sin_port) << std::endl;
    }
    else{
        if (bind (*socket1, (struct sockaddr *) dstIpAddr, sizeof(serv_addr)) < 0){
            std::cerr << "ERROR on binding" << std::endl;
            exit(1);
        }
        if (verboseOutput)
            std::cout << "listening on: " << inet_ntoa(dstIpAddr->sin_addr) << " port " << ntohs(dstIpAddr->sin_port) << std::endl;
    }

    listen(*socket1, 5);

    fcntl(*socket1, F_SETFL, O_NONBLOCK);
}

/**
 * run the sending loop
 */
void SocketHelper::runMaster(){
    int socket1;
    createSocketSend(&socket1);

    if (verboseOutput && dstIpAddr != nullptr) {
        std::cout << "sending packet to: addr->" << inet_ntoa(dstIpAddr->sin_addr);
        std::cout << " port->" << ntohs(dstIpAddr->sin_port) << std::endl;
    }

    while (!packetQueue.empty()){
        auto elem = packetQueue.front();
        packetQueue.pop();

        try {
            // send the packet
            std::visit([&socket1, this](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, Packet *>) {
                    // std::cout << *arg;

                    // send the msg
                    char packetAsArray[BUFFER_LEN + sizeof(Packet)];
                    memcpy(packetAsArray, arg, sizeof(PacketHeader));
                    for (int i = 0; i < reinterpret_cast<Packet *> (arg)->dataLen; i++) {
                        packetAsArray[i + sizeof(PacketHeader)] = reinterpret_cast<Packet *> (arg)->data[i];
                    }

                    ssize_t n;
                    int maxTryCount = 0; // to avoid an endless loop
                    do {
                        try {
                            n = send(socket1, packetAsArray,
                                     sizeof(PacketHeader) + reinterpret_cast<Packet *> (arg)->dataLen, 0);
                            maxTryCount++;
                        }
                        catch (std::exception &e){
                            std::cerr << e.what() << std::endl;
                        }
                    } while (n < 0 && maxTryCount < 10 && usleep(10000) == 0);

                    if (verboseOutput)
                        std::cout << "packet sent " << n << " bytes" << std::endl;

                    auto *p = reinterpret_cast<Packet *> (arg);
                    // delete p->data;
                    delete p;
                } else if constexpr (std::is_same_v<T, StartPacket *>) {
                    if (verboseOutput)
                        std::cout << *arg;

                    // send the msg
                    char packetAsArray[BUFFER_LEN];
                    // load packetHeader and sequence number into buffer
                    memcpy(packetAsArray, arg, sizeof(PacketHeader) + sizeof(uint32_t));
                    for (int i = 0; i < reinterpret_cast<StartPacket *> (arg)->nameLen; i++) {
                        packetAsArray[i + sizeof(PacketHeader) + sizeof(uint32_t)] =
                                reinterpret_cast<StartPacket *> (arg)->fileName[i];
                    }
                    ssize_t n = send(socket1, packetAsArray,
                                     sizeof(uint32_t) + sizeof(PacketHeader) +
                                     reinterpret_cast<StartPacket *> (arg)->nameLen, 0);

                    if (verboseOutput)
                        std::cout << "start packet sent " << n << " bytes" << std::endl;
                    auto *p = reinterpret_cast<StartPacket *> (arg);
                    delete p->fileName;
                    delete p;
                } else if constexpr (std::is_same_v<T, EndPacket *>) {
                    if (verboseOutput)
                        std::cout << *arg;

                    // send the msg
                    char packetAsArray[sizeof (EndPacket)];
                    memcpy(packetAsArray, arg, sizeof(EndPacket));
                    ssize_t n = send(socket1, packetAsArray, sizeof(EndPacket), 0);
                    if (verboseOutput)
                        std::cout << "end packet sent " << n << " bytes" << std::endl;

                    auto *p = reinterpret_cast<EndPacket *> (arg);
                    delete p;
                } else {
                    std::cerr << "unknown variance" << std::endl;
                }
            }, elem);
        }
        catch (std::exception &e){
            std::cerr << "exception on send visit: " << e.what() << std::endl;
        }
    }

    msgSend = true;
}

/**
 * run receiving loop
 * @param run receive as long as true
 */
void SocketHelper::runSlave(const bool *run){
    int socket1;
    createSocketRecv(&socket1);
    sockaddr_in lastCon{};

    while (*run) {
        char buffer[BUFFER_LEN + sizeof(Packet)];
        // size_t n = read(socket2, buffer, BUFFER_LEN-1);
        sockaddr cli_addr_sock{};
        socklen_t cli_len = sizeof(cli_addr_sock);
        ssize_t n = recvfrom(socket1, buffer, BUFFER_LEN + sizeof(Packet) - 1, 0, (struct sockaddr *) &cli_addr_sock, &cli_len);

        if (n < 1) {
            // usleep(10);
            continue;
        }

        auto currentCliAddr = reinterpret_cast<sockaddr_in *> (&cli_addr_sock);
        if (verboseOutput && lastCon.sin_port != currentCliAddr->sin_port &&
                lastCon.sin_addr.s_addr != currentCliAddr->sin_addr.s_addr) {

            std::cout << "server: got connection from ";
            std::cout << inet_ntoa(currentCliAddr->sin_addr) << " port ";
            std::cout << ntohs(currentCliAddr->sin_port) << std::endl;

            memcpy(&lastCon, currentCliAddr, sizeof(sockaddr_in));
        }

        // msg comes in multiple goes
        // have to concatenate messages
        pushToIncomingQueue(buffer, n);
        checkFinishedTransmission();
    }

    // closing socket
    close(socket1);
}

/**
 * run the code
 * @param run true as long as running
 * @param config master or slave
 */
void SocketHelper::run(const bool *run, Config config){
    while (*run) {
        switch (config) {
            case MASTER:
                runMaster();
                break;
            case SLAVE:
                runSlave(run);
                break;
            default:
                std::cerr << "server config unknown" << std::endl;
                exit(1);
        }
    }
}
