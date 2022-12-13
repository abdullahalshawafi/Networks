#include "Coordinator.h"
#include "Constants.h"
#include "Packet_m.h"

Define_Module(Coordinator);

void Coordinator::initialize()
{
    int startingNode;
    double startingTime;

    // Open coordinator.txt file to read its data
    std::ifstream inputFile("../inputs/coordinator.txt");

    // The first part will be the index of the starting node
    inputFile >> startingNode;

    // The second part will be the starting time
    inputFile >> startingTime;

    // Close the input file
    inputFile.close();

    // Send a start message to the starting node that was read from the input file
    Packet_Base *packet = new Packet_Base(START_SIGNAL);
    packet->setPayload(START_SIGNAL);
    sendDelayed(packet, startingTime, COORDINATOR_OUTPUTS, startingNode);
}

void Coordinator::handleMessage(cMessage *msg)
{
}
