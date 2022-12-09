#include "Coordinator.h"
#include "Constants.h"
#include "Packet_m.h"

Define_Module(Coordinator);

void Coordinator::initialize()
{
    int startingNode;
    std::string startingTime;
    std::string coordinatorInput;

    // Open coordinator.txt file to read its data
    std::ifstream inputFile("../inputs/coordinator.txt");

    // Since coordinator.txt file only contains one line, then no need to use while()
    std::getline(inputFile, coordinatorInput);

    // The first part will be the index of the starting node
    startingNode = std::stoi(strtok((char *)(coordinatorInput.c_str()), " "));

    // The second part will be the starting time
    startingTime = strtok(NULL, " ");

    // Close the input file
    inputFile.close();

    // Send a start message to the starting node that was read from the input file
    Packet_Base *packet = new Packet_Base(START_SIGNAL);
    packet->setPayload(START_SIGNAL);
    send(packet, COORDINATOR_OUTPUTS, startingNode);
}

void Coordinator::handleMessage(cMessage *msg)
{
}
