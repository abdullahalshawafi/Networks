#include "Node.h"
#include "Constants.h"

Define_Module(Node);

void Node::initialize()
{
    std::remove("../output/output.txt");
}

void Node::handleMessage(cMessage *msg)
{
    std::ofstream outputFile("../output/output.txt", std::ios_base::app);

    // If the message is the start signal
    if (strcmp(msg->getName(), START_SIGNAL) == 0)
    {
        std::string inputMessage;

        std::ifstream inputFile;

        // If we are in Node 0, then read the messages from input0.txt
        if (strcmp(getName(), NODE0) == 0)
        {
            inputFile.open("../inputs/input0.txt");
        }
        // Else if we are in Node 1, then read messages from input1.txt
        else if (strcmp(getName(), NODE1) == 0)
        {
            inputFile.open("../inputs/input1.txt");
        }

        // Read messages from the input file
        while(std::getline(inputFile, inputMessage))
        {
            // The first part will be errors
            std::string errors = inputMessage.substr(0, ERRORS_BITS_COUNT);

            // The rest of the data will be the message to be sent
            std::string message = inputMessage.substr(ERRORS_BITS_COUNT + 1);

            // Store the errors-message pair in the data vector
            this->data.emplace_back(std::make_pair(errors, message));
        }

        // Close the input file
        inputFile.close();

        // Send the first message to the other node
        cMessage *msg = new cMessage(this->data[this->index++].second.c_str());
        send(msg, NODE_OUTPUT);

        // Write the sent message to the output file
        outputFile << "Sent: " << msg->getName() << endl;
    }
    // If the message is an ACK signal, then send the next message
    else if (strcmp(msg->getName(), ACK_SIGNAL) == 0)
    {
        // Send the next message to the other node if we didn't reach the end of the vector yet
        if (this->index < this->data.size())
        {
            cMessage *msg = new cMessage(this->data[this->index++].second.c_str());
            send(msg, NODE_OUTPUT);

            // Write the sent message to the output file
            outputFile << "Sent: " << msg->getName() << endl;
        }
    }
    else
    {
        // Write the received message to the output file
        outputFile << "Received: " << msg->getName() << endl;

        // Send an ACK signal to the sender node to send the next message
        cMessage *msg = new cMessage(ACK_SIGNAL);
        send(msg, NODE_OUTPUT);
    }

    // Close the output file
    outputFile.close();
}
