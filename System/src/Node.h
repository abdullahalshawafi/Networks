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
    int WS;
    int TO;

    // A file stream to write the output to
    std::ofstream outputFile;

    // A vector of string pairs to store the errors binary string and the message
    std::vector<std::pair<std::string, std::string>> data;

    // A pointer to the index of the message that should be sent
    int index = 0;

    // Start of window
    int start = 0;

    // Expected sequence number
    int expectedSeqNum = 0;

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    void readFileMsg();
    std::string framing(std::string payload);
    char getParity(std::string frame);
    Packet_Base *createPacket(Packet_Base *oldPacket, std::string newPayload);
    void sendPacket(Packet_Base *packet);
    int receivePacket(Packet_Base *packet);
    bool checkParity(std::string frame, char expectedParity);
    void sendAck(Packet_Base *packet, int seqNum);
    bool receiveAck(Packet_Base *packet);
    void checkTimeout(int msgIndex);
    void delayMessage(std::string s);
};

#endif