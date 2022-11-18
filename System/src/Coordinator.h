#ifndef __SYSTEM_COORDINATOR_H_
#define __SYSTEM_COORDINATOR_H_

#include <omnetpp.h>
#include <fstream>

using namespace omnetpp;

class Coordinator : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

#endif
