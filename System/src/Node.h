#ifndef __SYSTEM_NODE_H_
#define __SYSTEM_NODE_H_

#include <omnetpp.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <utility>
#include <bitset>
#include "Packet_m.h"

using namespace omnetpp;

class Node : public cSimpleModule
{
private:
    /* CONSTANTS */
    int WS;    // window size
    int TO;    // timeout
    double PT; // processing time
    double TD; // transmission delay
    double ED; // channel error delay
    double DD; // channel duplication delay
    double LP; // ACK/NACK loss probability

    // node index
    int nodeIndex;

    // output file path
    std::string outputPath = "../output/";

    // The name of the output file
    std::string outputFileName;

    // A file stream to write the output to
    std::ofstream outputFile;

    // A vector of string pairs to store the errors binary string and the message
    std::vector<std::pair<std::string, std::string>> data;

    // A pointer to the index of the message that should be sent
    int index = 0;

    std::vector<bool> read;

    // Start of window
    int start = 0;

    // Expected sequence number
    int expectedSeqNum = 0;

    // create a vector for timeout messages
    std::vector<Packet_Base *> timeoutMsgs;

    int receivedCount = 0;

protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
    void readFileMsg();
    std::string framing(std::string payload);
    char getParity(std::string frame);
    Packet_Base *createPacket(Packet_Base *oldPacket, std::string newPayload);
    void sendPacket(Packet_Base *packet, double delay, bool lost);
    int receivePacket(Packet_Base *packet);
    bool checkParity(std::string frame, char expectedParity);
    void sendAck(Packet_Base *packet);
    void sendNAck(Packet_Base *packet);
    bool receiveAck(Packet_Base *packet);
    void checkTimeout(int msgIndex);
    void delayPacket(std::string event, double delay, int expectedSeqNum);
    void handleSending(Packet_Base *packet);
    void log(omnetpp::simtime_t time, std::bitset<4> trailer, Packet_Base *packet, bool isModification, bool isLoss, int isDuplication, bool isDelay);
};

#endif