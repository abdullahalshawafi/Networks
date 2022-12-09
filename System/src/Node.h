#ifndef __SYSTEM_NODE_H_
#define __SYSTEM_NODE_H_

#include <omnetpp.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <utility>
#include <bitset>

using namespace omnetpp;

class Node : public cSimpleModule
{
private:
    // A vector of string pairs to store the errors binary string and the message
    std::vector<std::pair<std::string, std::string>> data;

    // A pointer to the index of the message that should be sent
    int index = 0;
    // initialize the sequence number to 0
    int sequenceNumber = 0;
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    std::string framing(std::string payload);
    char getParity(std::string frame);
    bool checkParity(std::string frame, char expectedParity);
};

#endif
