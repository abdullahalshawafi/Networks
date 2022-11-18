#ifndef __SYSTEM_NODE_H_
#define __SYSTEM_NODE_H_

#include <omnetpp.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <utility>

using namespace omnetpp;

class Node : public cSimpleModule
{
private:
    // A vector of string pairs to store the errors binary string and the message
    std::vector<std::pair<std::string, std::string>> data;

    // A pointer to the index of the message that should be sent
    int index = 0;
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

#endif
