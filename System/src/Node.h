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

    // Index of the node (0 or 1)
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

    // Start of window
    int start = 0;

    // Expected sequence number
    int expectedSeqNum = 0;

    // A vector of boolean flags to indicate if a packet has been received or not
    std::vector<bool> read;

    // Vector for processed messages
    std::vector<Packet_Base *> processedMsgs;

    // Vector for duplicated messages
    std::vector<Packet_Base *> duplicatedMsgs;

    // Vector for timeout messages
    std::vector<Packet_Base *> timeoutMsgs;

    // Vector for control messages
    std::vector<Packet_Base *> controlMsgs;

    // Number of received packets
    int receivedCount = 0;

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
    void readFileMsg();
    std::string framing(std::string payload);
    char getParity(std::string frame);
    bool checkParity(std::string frame, char expectedParity);
    Packet_Base *createPacket(Packet_Base *packet, std::string newPayload);
    void sendPacket(Packet_Base *packet, double delay, bool isLost);
    int receivePacket(Packet_Base *packet);
    void sendControlPacket(Packet_Base *packet, short signal);
    bool receiveAck(Packet_Base *packet);
    void notifyAfter(std::string event, double delay, int ack);
    void handleSending(Packet_Base *packet);
    void printPacketRead(omnetpp::simtime_t time);
    void printPacketSent(omnetpp::simtime_t time, Packet_Base *packet, std::bitset<4> trailer, int modificationIndex, bool isLost, int duplicationType, bool isDelayed);
    void printPacketTimedOut(omnetpp::simtime_t time, int seqNum);
    void printControlPacketSent(omnetpp::simtime_t time, Packet_Base *packet, bool isLost);
};

#endif
